/*--------------------------------------------------------------------------
 * File: HypreDriverSStruct.cc
 *
 * Implementation of a wrapper of a Hypre solvers working with the Hypre
 * SStruct system interface.
 *--------------------------------------------------------------------------*/

#include <sci_defs/hypre_defs.h>

#include <Packages/Uintah/CCA/Components/Solvers/HypreDriverSStruct.h>
#include <Packages/Uintah/CCA/Components/Solvers/MatrixUtil.h>

#include <Packages/Uintah/Core/Grid/Level.h>
#include <Packages/Uintah/Core/Grid/Patch.h>
#include <Packages/Uintah/Core/Grid/Variables/CCVariable.h>
#include <Packages/Uintah/Core/Grid/Variables/CellIterator.h>
#include <Packages/Uintah/Core/Grid/Variables/Stencil7.h>
#include <Packages/Uintah/Core/Grid/Variables/VarTypes.h>
#include <Packages/Uintah/Core/Grid/Variables/AMRInterpolate.h>
#include <Packages/Uintah/Core/Exceptions/ProblemSetupException.h>
#include <Packages/Uintah/Core/Parallel/ProcessorGroup.h>
#include <Packages/Uintah/Core/Parallel/Parallel.h>

#include <Core/Geometry/IntVector.h>
#include <Core/Math/MiscMath.h>
#include <Core/Util/DebugStream.h>

#include <iomanip>
#include <iostream>

using namespace Uintah;
using namespace std;

#define test 

//__________________________________
//  To turn on normal output
//  setenv SCI_DEBUG "HYPRE_DOING_COUT:+"

static DebugStream cout_doing("HYPRE_DOING_COUT", false);
static DebugStream cout_dbg("HYPRE_DBG", false);

//___________________________________________________________________
HypreDriverSStruct::HyprePatch::HyprePatch(const Patch* patch,
                                           const int matl) :
  _patch(patch), 
  _matl(matl),
  _level(patch->getLevel()->getIndex()),
  _low(patch->getInteriorCellLowIndex()),
  _high(patch->getInteriorCellHighIndex()-IntVector(1,1,1))
  // Note: we need to subtract (1,1,1) from high because our loops are
  // cell = low; cell <= high.
{
}

//___________________________________________________________________
// HypreDriverSStruct::HyprePatch bogus patch constructor
//   for when there are no patches on a level for a processor
//   this is a one celled "patch"
//___________________________________________________________________
HypreDriverSStruct::HyprePatch::HyprePatch(const int level,
                                           const int matl) :
  _patch(0), 
  _matl(matl),
  _level(level),
  _low(IntVector(-9*(level+1),-9,-9)),
  _high(IntVector(-9*(level+1),-9,-9))
{
}

HypreDriverSStruct::HyprePatch::~HyprePatch(void)
{}


//___________________________________________________________________
// HypreDriverSStruct destructor
//___________________________________________________________________
HypreDriverSStruct::~HypreDriverSStruct(void)

{
  cout << Parallel::getMPIRank() <<" HypreDriverSStruct destructor" << "\n";
  //printDataStatus();
  // Destroy matrix, RHS, solution objects
  if (_exists[SStructA] >= SStructAssembled) {
    HYPRE_SStructMatrixDestroy(_HA);
    _exists[SStructA] = SStructDestroyed;
  }
  if (_exists[SStructB] >= SStructAssembled) {
    HYPRE_SStructVectorDestroy(_HB);
    _exists[SStructB] = SStructDestroyed;
  }
  if (_exists[SStructX] >= SStructAssembled) {
    HYPRE_SStructVectorDestroy(_HX);
    _exists[SStructX] = SStructDestroyed;
  }

  // Destroy graph objects
  if (_exists[SStructGraph] >= SStructAssembled) {
    HYPRE_SStructGraphDestroy(_graph);
    _exists[SStructGraph] = SStructDestroyed;
  }

  // Destroying grid, stencil
  if (_exists[SStructStencil] >= SStructCreated) {
    HYPRE_SStructStencilDestroy(_stencil);
    _exists[SStructStencil] = SStructDestroyed;
  }
  if (_vars) {
    delete _vars;
  }
  if (_exists[SStructGrid] >= SStructAssembled) {
    HYPRE_SStructGridDestroy(_grid);
    _exists[SStructGrid] = SStructDestroyed;
  }
}
//______________________________________________________________________
void
HypreDriverSStruct::printMatrix(const string& fileName )
{
  if (!_params->printSystem) return;
  HYPRE_SStructMatrixPrint((fileName + ".sstruct").c_str(), _HA, 0);
  if (_requiresPar) {
    HYPRE_ParCSRMatrixPrint(_HA_Par, (fileName + ".par").c_str());
    // Print CSR matrix in IJ format, base 1 for rows and cols
#if HAVE_HYPRE_1_9
    HYPRE_ParCSRMatrixPrintIJ(_HA_Par, 1, 1, (fileName + ".ij").c_str());
#else
    cerr << "Warning: this Hypre version does not support printing the "
         << "matrix in IJ format to a file, skipping this printout" << "\n";
#endif // #if HAVE_HYPRE_1_9
  }
}
//______________________________________________________________________
void
HypreDriverSStruct::printRHS(const string& fileName )
{
  if (!_params->printSystem) return;
  HYPRE_SStructVectorPrint(fileName.c_str(), _HB, 0);
  if (_requiresPar) {
    HYPRE_ParVectorPrint(_HB_Par, (fileName + ".par").c_str());
  }
}
//______________________________________________________________________
void
HypreDriverSStruct::printSolution(const string& fileName )
{
  if (!_params->printSystem) return;
  HYPRE_SStructVectorPrint(fileName.c_str(), _HX, 0);
  if (_requiresPar) {
    HYPRE_ParVectorPrint(_HX_Par, (fileName + ".par").c_str());
  }
}
//______________________________________________________________________
void
HypreDriverSStruct::gatherSolutionVector(void)
{
  HYPRE_SStructVectorGather(_HX);
} 
//______________________________________________________________________
//
void
HypreDriverSStruct::printDataStatus(void)
{
  cout << "Hypre SStruct interface data status:" << "\n";
  for (unsigned i = 0; i < _exists.size(); i++) {
    cout << "_exists[" << i << "] = " << _exists[i] << "\n";
  }
}

//#####################################################################
// class HypreDriverSStruct implementation for CC variable type
//#####################################################################

static const int CC_NUM_VARS = 1; // # Hypre var types that we use in CC solves
static const int CC_VAR = 0;      // Hypre CC variable type index


//___________________________________________________________________
// Function HypreDriverSStruct::makeLinearSystem_CC~
// Construct the linear system for CC variables (e.g. pressure),
// for the Hypre Struct interface (1-level problem / uniform grid).
// We set up the matrix at all patches of the "level" data member.
// matl is a fake material index. We always have one material here,
// matl=0 (pressure).
//  - 
//___________________________________________________________________
void
HypreDriverSStruct::makeLinearSystem_CC(const int matl)
{
  cout << Parallel::getMPIRank() << "------------------------------ HypreDriverSStruct::makeLinearSystem_CC()" << "\n";
  ASSERTEQ(sizeof(Stencil7), 7*sizeof(double));
  
  //__________________________________
  // Set up the grid
  cout << _pg->myrank() << " Setting up the grid" << "\n";
  // Create an empty grid in 3 dimensions with # parts = numLevels.
  const int numDims = 3;
  const int numLevels = _level->getGrid()->numLevels();
  
  HYPRE_SStructGridCreate(_pg->getComm(), numDims, numLevels, &_grid);

  _exists[SStructGrid] = SStructCreated;
  _vars = new HYPRE_SStructVariable[CC_NUM_VARS];
  _vars[CC_VAR] = HYPRE_SSTRUCT_VARIABLE_CELL; // We use only cell centered var

  // if my processor doesn't have patches on a given level, then we need to create
  // some bogus (irrelevent inexpensive) data so hypre doesn't crash.
  vector<bool> useBogusLevelData(numLevels, true);

  for (int p = 0 ; p < _patches->size(); p++) {
    HyprePatch_CC hpatch(_patches->get(p),matl);
    hpatch.addToGrid(_grid,_vars);
    useBogusLevelData[_patches->get(p)->getLevel()->getIndex()] = false;
  }
  
  
  for (int l = 0; l < numLevels; l++) {
    if (useBogusLevelData[l]) {
      HyprePatch_CC hpatch(l, matl);
      hpatch.addToGrid(_grid, _vars);
    }
  }

  HYPRE_SStructGridAssemble(_grid);
  _exists[SStructGrid] = SStructAssembled;

  //==================================================================
  // Create the stencil
  //==================================================================
  if (_params->symmetric) { 
    // Match the ordering of stencil elements in Hypre and Stencil7. 
    // Ordering:                x- y- z- diagonal
    // Or in terms of Stencil7: w  s  b  p
    _stencilSize = numDims+1;
    int offsets[4][numDims] = {{-1,0,0},
                               {0,-1,0},
                               {0,0,-1},
                               {0,0,0}};

    HYPRE_SStructStencilCreate(numDims, _stencilSize, &_stencil);
    for (int i = 0; i < _stencilSize; i++) {
      HYPRE_SStructStencilSetEntry(_stencil, i, offsets[i], 0);
    }
  } else {
    // Ordering:                x- x+ y- y+ z- z+ diagonal
    // Or in terms of Stencil7: w  e  s  n  b  t  p
    _stencilSize = 2*numDims+1;
    int offsets[7][numDims] = {{-1,0,0}, {1,0,0},
                               {0,-1,0}, {0,1,0},
                               {0,0,-1}, {0,0,1},
                               {0,0,0}};

    HYPRE_SStructStencilCreate(numDims, _stencilSize, &_stencil);
    for (int i = 0; i < _stencilSize; i++) {
      HYPRE_SStructStencilSetEntry(_stencil, i, offsets[i], 0);
    }
  }
  _exists[SStructStencil] = SStructCreated;

  //==================================================================
  // Setup connection graph
  //================================================================== 
  cout << _pg->myrank() << " Create the graph and stencil" << "\n";
  HYPRE_SStructGraphCreate(_pg->getComm(), _grid, &_graph);
  _exists[SStructGraph] = SStructCreated;
  
  // For ParCSR-requiring solvers like AMG
  if (_requiresPar) {
#if HAVE_HYPRE_1_9
    HYPRE_SStructGraphSetObjectType(_graph, HYPRE_PARCSR);
#else
    ostringstream msg;
    msg << "Hypre version does not support solvers that require "
        << "conversion from SStruct to ParCSR" << "\n";
    throw InternalError(msg.str(),__FILE__, __LINE__);
#endif 
  }

  //  Add stencil-based equations to the interior of the graph.
  for (int level = 0; level < numLevels; level++) {
    HYPRE_SStructGraphSetStencil(_graph, level, CC_VAR, _stencil);
  }

  // Add to graph the unstructured part of the stencil connecting the
  // coarse and fine levels at every C/F boundary   
  // Note: You need to do this "looking up" and "looking down'      
  for (int p = 0; p < _patches->size(); p++) {

    const Patch* patch = _patches->get(p);
    HyprePatch_CC hpatch(_patches->get(p),matl);
    int level = hpatch.getLevel();
    
    // Looking down
    if ((level > 0) && (patch->hasCoarseFineInterfaceFace())) {
      hpatch.makeGraphConnections(_graph,DoingFineToCoarse);
    } 
    // Looking up
    if (level < numLevels-1) {
      hpatch.makeGraphConnections(_graph,DoingCoarseToFine);
    }
  } 

  cout << Parallel::getMPIRank()<< " Doing Assemble graph \t\t\tPatches"<< *_patches << endl;  
  HYPRE_SStructGraphAssemble(_graph);
  _exists[SStructGraph] = SStructAssembled;

  //==================================================================
  // Set up matrix _HA
  //==================================================================
  HYPRE_SStructMatrixCreate(_pg->getComm(), _graph, &_HA);
  _exists[SStructA] = SStructCreated;
  // If specified by input parameter, declare the structured and
  // unstructured part of the matrix to be symmetric.
  
#if HAVE_HYPRE_1_9
  for (int level = 0; level < numLevels; level++) {
    HYPRE_SStructMatrixSetSymmetric(_HA, level,
                                    CC_VAR, CC_VAR,
                                    _params->symmetric);
  }
  HYPRE_SStructMatrixSetNSSymmetric(_HA, _params->symmetric);
#else
  cerr << "Warning: Hypre version does not correctly support "
       << "symmetric matrices; proceeding without doing anything "
       << "at this point." << "\n";
#endif

  // For solvers that require ParCSR format
  if (_requiresPar) {
    HYPRE_SStructMatrixSetObjectType(_HA, HYPRE_PARCSR);
  }
  HYPRE_SStructMatrixInitialize(_HA);
  _exists[SStructA] = SStructInitialized;

  //__________________________________
  // added the stencil entries to the interior cells
  for (int p = 0 ; p < _patches->size(); p++) {
    const Patch* patch = _patches->get(p);
    HyprePatch_CC hpatch(patch,matl); 
    hpatch.makeInteriorEquations(_HA, _A_dw, _A_label,
                                 _stencilSize, _params->symmetric);
  }
  
  for (int l = 0; l < numLevels; l++) {
    if (useBogusLevelData[l]) {
      HyprePatch_CC hpatch(l, matl);
      hpatch.makeInteriorEquations(_HA, _A_dw, _A_label, 
                                   _stencilSize, _params->symmetric);
    }
  }

  //__________________________________
  // added the unstructured entries at the C/F interfaces
  for (int p = 0; p < _patches->size(); p++) {
    const Patch* patch = _patches->get(p);
    HyprePatch_CC hpatch(_patches->get(p),matl); 
    int level = hpatch.getLevel();
    
    if ((level > 0) && (patch->hasCoarseFineInterfaceFace())) {
      hpatch.makeConnections(_HA, _A_dw, _A_label,
                             _stencilSize, DoingFineToCoarse);
    }
  } 
  HYPRE_SStructMatrixAssemble(_HA);
  _exists[SStructA] = SStructAssembled;


  //==================================================================
  //  Create the rhs
  //==================================================================
  cout << _pg->myrank() << " Doing setup RHS vector _HB" << "\n";
  HYPRE_SStructVectorCreate(_pg->getComm(), _grid, &_HB);
  _exists[SStructB] = SStructCreated;
  
  if (_requiresPar) {
    HYPRE_SStructVectorSetObjectType(_HB, HYPRE_PARCSR);
  }
  HYPRE_SStructVectorInitialize(_HB);
  _exists[SStructB] = SStructInitialized;

  // Set RHS vector entries
  for (int p = 0 ; p < _patches->size(); p++) {
    const Patch* patch = _patches->get(p);
    HyprePatch_CC hpatch(patch,matl); 
    hpatch.makeInteriorVector(_HB, _b_dw, _B_label);
  } 
  
  for (int l = 0; l < numLevels; l++) {
    if (useBogusLevelData[l]) {
      HyprePatch_CC hpatch(l, matl);
      hpatch.makeInteriorVectorZero(_HB, _b_dw, _B_label);
    }
  }

  HYPRE_SStructVectorAssemble(_HB);
  _exists[SStructB] = SStructAssembled;

  //==================================================================
  //  Create the solution
  //==================================================================
  cout << _pg->myrank() << " Doing setup solution vector _HX" << "\n";
  HYPRE_SStructVectorCreate(_pg->getComm(), _grid, &_HX);
  _exists[SStructX] = SStructCreated;
  
  if (_requiresPar) {
    HYPRE_SStructVectorSetObjectType(_HX, HYPRE_PARCSR);
  }
  HYPRE_SStructVectorInitialize(_HX);
  _exists[SStructX] = SStructInitialized;
  
  if (_guess_label) {
    for (int p = 0 ; p < _patches->size(); p++) {
      const Patch* patch = _patches->get(p);
      HyprePatch_CC hpatch(patch,matl); 
      hpatch.makeInteriorVector(_HX, _guess_dw, _guess_label);
    } 
    
    for (int l = 0; l < numLevels; l++) {
      if (useBogusLevelData[l]) {
        HyprePatch_CC hpatch(l, matl);
        hpatch.makeInteriorVectorZero(_HX, _guess_dw, _guess_label);
      }
    }
  } else {
#if 0
    // If guess is not provided by ICE, use zero as initial guess
    cout << _pg->myrank() << " Default initial guess: zero" << "\n";
    for (int p = 0 ; p < _patches->size(); p++) {
      // Read Uintah patch info into our data structure, set Uintah pointers
      const Patch* patch = _patches->get(p);
      HyprePatch_CC hpatch(patch,matl); // Read Uintah patch data
      hpatch.makeInteriorVectorZero(_HX, _guess_dw, _guess_label);
    } // end for p (patches)
    for (int l = 0; l < numLevels; l++) {
      if (useBogusLevelData[l]) {
        HyprePatch_CC hpatch(l, matl);
        hpatch.makeInteriorVectorZero(_HX, _guess_dw, _guess_label);
      }
    }
#endif
  }

  HYPRE_SStructVectorAssemble(_HX);
  _exists[SStructX] = SStructAssembled;

  // For solvers that require ParCSR format
  if (_requiresPar) {
    cout << _pg->myrank() << " Making ParCSR objects from SStruct objects" << "\n";
    HYPRE_SStructMatrixGetObject(_HA, (void **) &_HA_Par);
    HYPRE_SStructVectorGetObject(_HB, (void **) &_HB_Par);
    HYPRE_SStructVectorGetObject(_HX, (void **) &_HX_Par);
  }
  cout << Parallel::getMPIRank() << " HypreDriverSStruct::makeLinearSystem_CC() END" << "\n";
} // end HypreDriverSStruct::makeLinearSystem_CC()


//_____________________________________________________________________*/
void
HypreDriverSStruct::getSolution_CC(const int matl)
{
  for (int p = 0 ; p < _patches->size(); p++) {
    HyprePatch_CC hpatch(_patches->get(p),matl); // Read Uintah patch data
    hpatch.getSolution(_HX,_new_dw,_X_label,_modifies_x);
  } 
} 

//___________________________________________________________________
// Add this patch to the Hypre grid
//___________________________________________________________________
void
HypreDriverSStruct::HyprePatch_CC::addToGrid(HYPRE_SStructGrid& grid,
                                             HYPRE_SStructVariable* vars)

{
  cout << Parallel::getMPIRank() << " Adding patch " << (_patch?_patch->getID():-1)
       << " from "<< _low << " to " << _high
       << " Level " << _level << "\n";
  HYPRE_SStructGridSetExtents(grid, _level,
                              _low.get_pointer(),
                              _high.get_pointer());
  HYPRE_SStructGridSetVariables(grid, _level, CC_NUM_VARS, vars);
}
//___________________________________________________________________
// HypreDriverSStruct::HyprePatch_CC::makeGraphConnections~
// Add the connections at C/F interfaces of this patch to the HYPRE
// Graph.   You must do for the graph "looking up" from the coarse patch
// and looking down from the finePatch.
//___________________________________________________________________
void
HypreDriverSStruct::HyprePatch_CC::makeGraphConnections(HYPRE_SStructGraph& graph,
                                                   const CoarseFineViewpoint& viewpoint)

{
  int mpiRank = Parallel::getMPIRank();
  cout << mpiRank << " Doing makeGraphConnections \t\t\t\tL-"
                  << _level << " Patch " << _patch->getID()
                  << " viewpoint " << viewpoint << endl;
 
  //__________________________________
  // viewpoint LOGIC
  int f_level_ID, c_level_ID;
  Level::selectType finePatches;
  const Level* fineLevel;
  
  if(viewpoint == DoingFineToCoarse){
    const Patch* finePatch   = _patch;
    fineLevel                = finePatch->getLevel();
    const Level* coarseLevel = fineLevel->getCoarserLevel().get_rep();
    
    finePatches.push_back(finePatch);
    
    c_level_ID  = coarseLevel->getID();
    f_level_ID  = fineLevel->getID();  
  }
  if(viewpoint == DoingCoarseToFine){
    const Patch* coarsePatch = _patch;
    const Level* coarseLevel = coarsePatch->getLevel();
    fineLevel                = coarseLevel->getFinerLevel().get_rep();
    
    coarsePatch->getFineLevelPatches(finePatches);
    
    c_level_ID  = coarseLevel->getID();
    f_level_ID  = fineLevel->getID(); 
  }

  const IntVector& refRat = fineLevel->getRefinementRatio();
  
  //At the CFI compute the fine/coarse level indices and pass them to hypre

  for(int i = 0; i < finePatches.size(); i++){  
    const Patch* finePatch = finePatches[i];
   
    vector<Patch::FaceType>::const_iterator iter; 
    for (iter  = finePatch->getCoarseFineInterfaceFaces()->begin(); 
         iter != finePatch->getCoarseFineInterfaceFaces()->end(); ++iter) {

      Patch::FaceType face = *iter;                   
      IntVector offset = finePatch->faceDirection(face);
      CellIterator f_iter = finePatch->getFaceCellIterator(face,"alongInteriorFaceCells");

#if 1           // spew
      cout << mpiRank << "-----------------Face " << finePatch->getFaceName(face) 
           << " iter " << f_iter.begin() << " " << f_iter.end() 
           << " offset " << offset<<endl;
#endif

      for(; !f_iter.done(); f_iter++) {
        IntVector fineCell = *f_iter;                        
        IntVector coarseCell = (fineCell + offset) / refRat;

        if(viewpoint == DoingFineToCoarse){
        
          //cout <<mpiRank<<" looking Down: fineCell " << fineCell 
          //     << " -> coarseCell " << coarseCell;
        
          HYPRE_SStructGraphAddEntries(graph,
                                       f_level_ID, fineCell.get_pointer(),
                                       CC_VAR,
                                       c_level_ID, coarseCell.get_pointer(),
                                       CC_VAR);
          //cout << " done " << endl;

        }
        if(viewpoint == DoingCoarseToFine){
          //cout <<mpiRank<<" looking Up: fineCell " << fineCell 
          //     << " <- coarseCell " << coarseCell;
        
          HYPRE_SStructGraphAddEntries(graph,
                                       c_level_ID, coarseCell.get_pointer(),
                                       CC_VAR,
                                       f_level_ID, fineCell.get_pointer(),
                                       CC_VAR);
          //cout << " done " << endl;

        }
      }
    } // CFI
  }  // finePatches
} 
//___________________________________________________________________
// HypreDriverSStruct::HyprePatch_CC::makeInteriorEquations~
// Add the connections at C/F interfaces of this patch to the HYPRE
// Graph. If viewpoint == DoingFineToCoarse, we add the fine-to-coarse
// connections. If viewpoint == DoingCoarseToFine, we add the
// coarse-to-fine-connections that are read from the connection list
// prepared for this patch by ICE.
//___________________________________________________________________
void
HypreDriverSStruct::HyprePatch_CC::makeInteriorEquations(HYPRE_SStructMatrix& HA,
                                                         DataWarehouse* A_dw,
                                                         const VarLabel* A_label,
                                                         const int stencilSize,
                                                         const bool symmetric /* = false */)

{
  cout << Parallel::getMPIRank() << " doing makeInteriorEquations \t\t\t\tL-" 
             << _level<< " Patch " << (_patch?_patch->getID():-1) << endl;
             
  CCTypes::matrix_type A;
  if (_patch) {
    A_dw->get(A, A_label, _matl, _patch, Ghost::None, 0);
  }
  else {
    // should be a 1-cell object
    ASSERT(_low == _high);
    CCVariable<Stencil7>& Amod = A.castOffConst();
    Amod.rewindow(_low, _high +IntVector(1,1,1));
    Amod[_low].w = Amod[_low].e = Amod[_low].s = Amod[_low].n = Amod[_low].b = Amod[_low].t = 0;
    Amod[_low].p = 1;
  }
  
  if (symmetric) {
    double* values = new double[(_high.x()-_low.x()+1)*stencilSize];
    int stencil_indices[] = {0,1,2,3};
    for(int z = _low.z(); z <= _high.z(); z++) {
      for(int y = _low.y(); y <= _high.y(); y++) {
        const Stencil7* AA = &A[IntVector(_low.x(), y, z)];
        double* p = values;
        for (int x = _low.x(); x <= _high.x(); x++) {
          // Keep the ordering as in stencil offsets:
          // w s b p
          *p++ = AA->w;
          *p++ = AA->s;
          *p++ = AA->b;
          *p++ = AA->p;
          AA++;
        }
        IntVector chunkLow(_low.x(), y, z);
        IntVector chunkHigh(_high.x(), y, z);
        HYPRE_SStructMatrixSetBoxValues(HA, _level,
                                        chunkLow.get_pointer(),
                                        chunkHigh.get_pointer(), CC_VAR,
                                        stencilSize, stencil_indices, values);
      }
    }
    delete[] values;
  } else { // now symmetric = false
    double* values = new double[(_high.x()-_low.x()+1)*stencilSize];
    int stencil_indices[] = {0,1,2,3,4,5,6};
    for(int z = _low.z(); z <= _high.z(); z++) {
      for(int y = _low.y(); y <= _high.y(); y++) {
        const Stencil7* AA = &A[IntVector(_low.x(), y, z)];
        double* p = values;
        for (int x = _low.x(); x <= _high.x(); x++) {
          // Keep the ordering as in stencil offsets:
          // w e s n b t p
          *p++ = AA->w;
          *p++ = AA->e;
          *p++ = AA->s;
          *p++ = AA->n;
          *p++ = AA->b;
          *p++ = AA->t;
          *p++ = AA->p;
          AA++;
        }
        IntVector chunkLow(_low.x(), y, z);
        IntVector chunkHigh(_high.x(), y, z);
        HYPRE_SStructMatrixSetBoxValues(HA, _level,
                                        chunkLow.get_pointer(),
                                        chunkHigh.get_pointer(), CC_VAR,
                                        stencilSize, stencil_indices, values);
      }
    }
    delete[] values;
  } 
}

//___________________________________________________________________
//makeInteriorVector.
//___________________________________________________________________
void
HypreDriverSStruct::HyprePatch_CC::makeInteriorVector(HYPRE_SStructVector& HV,
                                                      DataWarehouse* V_dw,
                                                      const VarLabel* V_label)
{
  CCTypes::const_type V;
  V_dw->get(V, V_label, _matl, _patch, Ghost::None, 0);
  
  for(int z = _low.z(); z <= _high.z(); z++) {
    for(int y = _low.y(); y <= _high.y(); y++) {
      const double* values = &V[IntVector(_low.x(), y, z)];
      IntVector chunkLow(_low.x(), y, z);
      IntVector chunkHigh(_high.x(), y, z);
      HYPRE_SStructVectorSetBoxValues(HV, _level,
                                      chunkLow.get_pointer(),
                                      chunkHigh.get_pointer(), CC_VAR,
                                      const_cast<double*>(values));
    }
  }
} 

//___________________________________________________________________
// makeInteriorVectorZero
//___________________________________________________________________
void
HypreDriverSStruct::HyprePatch_CC::makeInteriorVectorZero(HYPRE_SStructVector& HV,
                                                          DataWarehouse* V_dw,
                                                          const VarLabel* V_label)
{
  // Make a vector of zeros
  CCVariable<double> V;
  V.rewindow(_low, _high+IntVector(1,1,1));
  V.initialize(0.0);
  
  for(int z = _low.z(); z <= _high.z(); z++) {
    for(int y = _low.y(); y <= _high.y(); y++) {
      const double* values = &V[IntVector(_low.x(), y, z)];
      IntVector chunkLow(_low.x(), y, z);
      IntVector chunkHigh(_high.x(), y, z);
      HYPRE_SStructVectorSetBoxValues(HV, _level,
                                      chunkLow.get_pointer(),
                                      chunkHigh.get_pointer(), CC_VAR,
                                      const_cast<double*>(values));
    }
  }
} 

//___________________________________________________________________
// HypreDriverSStruct::HyprePatch_CC::makeConnections~
// Add the connections at C/F interfaces of this patch.
//___________________________________________________________________
void
HypreDriverSStruct::HyprePatch_CC::makeConnections(HYPRE_SStructMatrix& HA,
                                                   DataWarehouse* A_dw,
                                                   const VarLabel* A_label,
                                                   const int stencilSize,
                                                   const CoarseFineViewpoint& viewpoint)
{
  // Important: cell iterators here MUST loop in the same order over
  // fine and coarse cells as in makeGraphConnections, otherwise
  // the entry counters (entryFine, entryCoarse) will point to the wrong
  // entries in the Hypre graph.
  const Patch* finePatch = _patch;
  const Level* fineLevel = finePatch->getLevel();
  const Level* coarseLevel = fineLevel->getCoarserLevel().get_rep();
  const IntVector& refRat = fineLevel->getRefinementRatio();
  int mpiRank = Parallel::getMPIRank();
  
  cout << mpiRank << " Doing makeConnections \t\t\t\tL-" << _level
           << " Patch " << finePatch->getID() << endl;
  
  const GridP grid = fineLevel->getGrid();
  const double ZERO = 0.0;
    
  cout.setf(ios::scientific,ios::floatfield);
  cout.precision(5);
  //__________________________________
  // Add fine-to-coarse entries to matrix  
  //
  // Unstructured entries:   stencilSize, stencilSize + 1, ... 
  // Structured entries:     0....stencilSize-1 
  //
  // Set the unstructured connection to  A[fineCell][face], because
  // it is an approximate flux across the fine face of the C/F boundary.
  CCTypes::matrix_type A;
  A_dw->get(A, A_label, _matl, finePatch, Ghost::None, 0);

  // allocate and initialize the stencil counter
  // finelevel  
  CCVariable<int> counter_fine;  
  A_dw->allocateTemporary(counter_fine, finePatch);
  counter_fine.initialize(stencilSize);

  // coarseLevel
  IntVector cl, ch, fl, fh;
  getCoarseLevelRange(finePatch, coarseLevel, cl, ch, fl, fh, 1); 
  CCVariable<int> counter_coarse;
  counter_coarse.allocate(cl, ch);
  counter_coarse.initialize(stencilSize );


  vector<Patch::FaceType>::const_iterator iter;
  for (iter  = finePatch->getCoarseFineInterfaceFaces()->begin(); 
       iter != finePatch->getCoarseFineInterfaceFaces()->end(); ++iter) {

    Patch::FaceType face = *iter;                  
    CellIterator f_iter = finePatch->getFaceCellIterator(face,"alongInteriorFaceCells");

    int opposite = face - int(patchFaceSide(face));    
    int stencilIndex_fine[1]   = {face};
    int stencilIndex_coarse[1] = {opposite};
    IntVector offset = finePatch->faceDirection(face);

#if 1
    cout << "-----------------Face " << finePatch->getFaceName(face) 
         << " iter " << f_iter.begin() << " " << f_iter.end() 
         << " offset " << offset << " opposite " << opposite << endl;
#endif

    for(; !f_iter.done(); f_iter++) {
      IntVector fineCell = *f_iter; 

      //_____________________________________________________________
      //  Update the entries on the fine level
      int graphIndex_fine[1] = {counter_fine[fineCell]};
      const double* graphValue = &A[fineCell][face];
      int f_level_ID = fineLevel->getID();

      // add the unstructured entry to the matrix
      HYPRE_SStructMatrixSetValues(HA, f_level_ID,
                                   fineCell.get_pointer(),
                                   CC_VAR, 1, graphIndex_fine,
                                   const_cast<double*>(graphValue));

      // Wipe out the original structured entry 
      const double* stencilValue = &ZERO;
       HYPRE_SStructMatrixSetValues(HA, f_level_ID,
                                   fineCell.get_pointer(),
                                   CC_VAR, 1, stencilIndex_fine,
                                   const_cast<double*>(stencilValue));

      counter_fine[fineCell]++;       

      //_____________________________________________________________
      // update the coarseLevel entries
      // For each fine cell at C/F interface add the 
      // unstructured connection between
      // the fine and coarse cells to the graph.    
      IntVector coarseCell = (fineCell + offset) / refRat;
      int graphIndex_coarse[1] = {counter_coarse[coarseCell]};
      int c_level_ID = coarseLevel->getID();

      HYPRE_SStructMatrixSetValues(HA, c_level_ID,
                                   coarseCell.get_pointer(),
                                   CC_VAR, 1, graphIndex_coarse,
                                   const_cast<double*>(graphValue));

      // Wipe out the original coarse-coarse structured connection
      if (counter_coarse[coarseCell] == stencilSize) {
        stencilValue = &ZERO;
        HYPRE_SStructMatrixSetValues(HA, c_level_ID,
                                     coarseCell.get_pointer(),
                                     CC_VAR, 1, stencilIndex_coarse, 
                                     const_cast<double*>(stencilValue));
      } 
      counter_coarse[coarseCell]++;
#if 0
      cout << " finePatch "<< fineCell
           << " f_index " << graphIndex_fine[0]
           << " s_index " << stencilIndex_fine[0]
           << " value " << graphValue[0] 
           << " \t| Coarse " << coarseCell
           << " c_index " << graphIndex_coarse[0]
           << " s_index " << stencilIndex_coarse[0]<<endl;  
#endif     
    }
  }
} 


//___________________________________________________________________
// getSolution(): move Hypre solution into Uintah datastructure
//___________________________________________________________________
void
HypreDriverSStruct::HyprePatch_CC::getSolution(HYPRE_SStructVector& HX,
                                               DataWarehouse* new_dw,
                                               const VarLabel* X_label,
                                               const bool modifies_x)
{
  typedef CCTypes::sol_type sol_type;
  sol_type Xnew;
  if (modifies_x) {
    new_dw->getModifiable(Xnew, X_label, _matl, _patch);
  } else {
    new_dw->allocateAndPut(Xnew, X_label, _matl, _patch);
  }

  for(int z = _low.z(); z <= _high.z(); z++) {
    for(int y = _low.y(); y <= _high.y(); y++) {
      const double* values = &Xnew[IntVector(_low.x(), y, z)];
      IntVector chunkLow(_low.x(), y, z);
      IntVector chunkHigh(_high.x(), y, z);
      HYPRE_SStructVectorGetBoxValues(HX, _level,
                                      chunkLow.get_pointer(),
                                      chunkHigh.get_pointer(),
                                      CC_VAR, const_cast<double*>(values));
    }
  }
}

//#####################################################################
// Utilities
//#####################################################################

void printLine(const string& s, const unsigned int len)
{
  for (unsigned int i = 0; i < len; i++) {
    cout << s;
  }
  cout << "\n";
}


namespace Uintah {

  std::ostream&
  operator << (std::ostream& os,
               const HypreDriverSStruct::CoarseFineViewpoint& v)
  {
    if      (v == HypreDriverSStruct::DoingCoarseToFine) os << "CoarseToFine";
    else if (v == HypreDriverSStruct::DoingFineToCoarse) os << "FineToCoarse";
    else os << "CoarseFineViewpoint WRONG!!!";
    return os;
  }

  std::ostream& operator<< (std::ostream& os,
                            const HypreDriverSStruct::HyprePatch& p)
    // Write our patch structure to the stream os.
  {
    os << *(p.getPatch());
    return os;
  }


} // end namespace Uintah

