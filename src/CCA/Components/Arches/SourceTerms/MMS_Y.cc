#include <CCA/Components/Arches/SourceTerms/MMS_Y.h>
#include <CCA/Components/Arches/ArchesLabel.h>
#include <Core/ProblemSpec/ProblemSpec.h>
#include <CCA/Ports/Scheduler.h>
#include <Core/Grid/SimulationState.h>
#include <Core/Grid/Variables/VarTypes.h>
#include <Core/Grid/Variables/CCVariable.h>
#include <Core/Exceptions/ProblemSetupException.h>
#include <Core/Parallel/Parallel.h>
#include <Core/Grid/Level.h>

//===========================================================================

using namespace std;
using namespace Uintah; 

/** @details
The MMS_Y class creates a source term Q for a manufactured solution testing only
the X convection term.  It is a source term for the PDE:

\f[
v \frac{ \partial \phi }{ \partial y }
= Q
\f]

Note that this PDE is steady, incompressible, and assumes constant Y velocity.

The unsteady term is zero because the assumed solution is not a function of time.

The X convection term is zero because the X velocity is zero.

These conditions (steady, incompressible, constant X velocity) are achieved 
by using a constant velocity and density field using the constant MMS case, 
whose input file is located in:

StandAlone/inputs/ARCHES/mms/constantMMS.ups

The assumed solution is:

\f[
\phi = \sin ( 2 \pi \frac{x}{L_x} ) \cos ( 2 \pi \frac{y}{L_y} )
\f]

This makes the source term Q equal to:

\f[
Q = - \frac{2 \pi}{L_y} v \sin ( 2 \pi \frac{x}{L_x} ) \sin ( 2 \pi \frac{y}{L_y} )
\f]

*/
MMS_Y::MMS_Y( std::string srcName, SimulationStateP& sharedState,
                            vector<std::string> reqLabelNames ) 
: SourceTermBase(srcName, sharedState, reqLabelNames)
{
  _src_label = VarLabel::create(srcName, CCVariable<double>::getTypeDescription()); 

  _source_type = CC_SRC; 

  MMS_Y::pi = 3.1415926535;
}

MMS_Y::~MMS_Y()
{}
//---------------------------------------------------------------------------
// Method: Problem Setup
//---------------------------------------------------------------------------
void 
MMS_Y::problemSetup(const ProblemSpecP& inputdb)
{
  ProblemSpecP db = inputdb;
  ProblemSpecP db_root = db->getRootNode();

  if( db_root->findBlock("CFD")->findBlock("ARCHES") ) {
    if( db_root->findBlock("CFD")->findBlock("ARCHES")->findBlock("MMS") ) {
      ProblemSpecP db_mms = db_root->findBlock("CFD")->findBlock("ARCHES")->findBlock("MMS");

      string which_mms;
      if( !db_mms->getAttribute( "whichMMS", which_mms ) ) {
        throw ProblemSetupException( "ERROR: Arches: MMS_Y: No Arches MMS type specified in input file.  To use MMS_Y, you must use a constant MMS.  See 'StandAlone/inputs/ARCHES/mms/constantMMS.ups' for an example input.", __FILE__, __LINE__);      
      } 

      if( which_mms != "constantMMS" ) {
        throw ProblemSetupException( "ERROR: Arches: MMS_Y: Incorrect Arches MMS type specified in input file.  To use MMS_Y, you must use a constant MMS.  See 'StandAlone/inputs/ARCHES/mms/constantMMS.ups' for an example input.", __FILE__, __LINE__); 
      }

      ProblemSpecP db_const_mms = db_mms->findBlock("constantMMS");
      db_const_mms->getWithDefault("cu",MMS_Y::d_uvel,0.0);
      db_const_mms->getWithDefault("cv",MMS_Y::d_vvel,0.0);
      db_const_mms->getWithDefault("cw",MMS_Y::d_wvel,0.0);

      if( d_uvel != 0.0 || d_wvel != 0.0 ) {
        throw ProblemSetupException( "ERROR: Arches: MMS_Y: This MMS source term is intended to verify Y convection only. X velocity and Z velocity must be set to 0. (Alternatively, use MMS_XYZ to verify all convective terms.)", __FILE__, __LINE__ );
      }

    } //end findblock MMS
  } //end findblock Arches
}
//---------------------------------------------------------------------------
// Method: Schedule the calculation of the source term 
//---------------------------------------------------------------------------
void 
MMS_Y::sched_computeSource( const LevelP& level, 
                            SchedulerP& sched, 
                            int timeSubStep )
{
  std::string taskname = "MMS_Y::computeSource";
  Task* tsk = scinew Task(taskname, this, &MMS_Y::computeSource, timeSubStep);

  if (timeSubStep == 0 && !_label_sched_init) {
    // Every source term needs to set this flag after the varLabel is computed. 
    // transportEqn.cleanUp should reinitialize this flag at the end of the time step. 
    _label_sched_init = true;
  }

  if( timeSubStep == 0 ) {
    tsk->computes(_src_label);
  } else {
    tsk->modifies(_src_label); 
  }

  grid = level->getGrid();

  sched->addTask(tsk, level->eachPatch(), _shared_state->allArchesMaterials()); 

}
//---------------------------------------------------------------------------
// Method: Actually compute the source term 
//---------------------------------------------------------------------------
void
MMS_Y::computeSource( const ProcessorGroup* pc, 
                      const PatchSubset* patches, 
                      const MaterialSubset* matls, 
                      DataWarehouse* old_dw, 
                      DataWarehouse* new_dw, 
                      int timeSubStep )
{
  Vector domain_size = Vector(0.0,0.0,0.0);
  grid->getLength(domain_size);

  //patch loop
  for (int p=0; p < patches->size(); p++){

    const Patch* patch = patches->get(p);
    int archIndex = 0;
    int matlIndex = _shared_state->getArchesMaterial(archIndex)->getDWIndex(); 
    Vector Dx = patch->dCell(); 

    CCVariable<double> MMS_Y_src; 

    if( timeSubStep == 0 ) {
      new_dw->allocateAndPut( MMS_Y_src, _src_label, matlIndex, patch );
      MMS_Y_src.initialize(0.0);
    } else {
      new_dw->getModifiable( MMS_Y_src, _src_label, matlIndex, patch ); 
    }

    for (CellIterator iter=patch->getCellIterator(); !iter.done(); iter++){
      IntVector c = *iter; 
      double x,y,z;
      x = c[0]*Dx.x() + Dx.x()/2.; 
      y = c[1]*Dx.y() + Dx.y()/2.; 
      z = c[2]*Dx.z() + Dx.z()/2.; 

      MMS_Y_src[c] = MMS_Y::evaluate_MMS_source( x, y, z, domain_size );
    }
  }
}
//---------------------------------------------------------------------------
// Method: Schedule dummy initialization
//---------------------------------------------------------------------------
void
MMS_Y::sched_dummyInit( const LevelP& level, SchedulerP& sched )
{
  string taskname = "MMS_Y::dummyInit"; 

  Task* tsk = scinew Task(taskname, this, &MMS_Y::dummyInit);

  tsk->computes(_src_label);

  for (std::vector<const VarLabel*>::iterator iter = _extra_local_labels.begin(); iter != _extra_local_labels.end(); iter++){
    tsk->computes(*iter); 
  }

  sched->addTask(tsk, level->eachPatch(), _shared_state->allArchesMaterials());

}
void 
MMS_Y::dummyInit( const ProcessorGroup* pc, 
                      const PatchSubset* patches, 
                      const MaterialSubset* matls, 
                      DataWarehouse* old_dw, 
                      DataWarehouse* new_dw )
{
  //patch loop
  for (int p=0; p < patches->size(); p++){

    const Patch* patch = patches->get(p);
    int archIndex = 0;
    int matlIndex = _shared_state->getArchesMaterial(archIndex)->getDWIndex(); 

    CCVariable<double> src;

    new_dw->allocateAndPut( src, _src_label, matlIndex, patch ); 

    src.initialize(0.0); 

    for (std::vector<const VarLabel*>::iterator iter = _extra_local_labels.begin(); iter != _extra_local_labels.end(); iter++){
      CCVariable<double> tempVar; 
      new_dw->allocateAndPut(tempVar, *iter, matlIndex, patch ); 
    }
  }
}

double MMS_Y::d_uvel;
double MMS_Y::d_vvel;
double MMS_Y::d_wvel;
double MMS_Y::pi;

