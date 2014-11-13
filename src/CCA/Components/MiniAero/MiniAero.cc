/*
 * The MIT License
 *
 * Copyright (c) 1997-2014 The University of Utah
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <vector>
#include <list>

#include <CCA/Components/MiniAero/MiniAero.h>
#include <CCA/Components/MiniAero/BoundaryCond.h>
#include <CCA/Components/Examples/ExamplesLabel.h>
#include <CCA/Ports/Scheduler.h>

#include <Core/ProblemSpec/ProblemSpec.h>

#include <Core/Grid/Variables/SFCXVariable.h>
#include <Core/Grid/Variables/SFCYVariable.h>
#include <Core/Grid/Variables/SFCZVariable.h>
#include <Core/Grid/Variables/CellIterator.h>
#include <Core/Grid/SimulationState.h>
#include <Core/Grid/Task.h>
#include <Core/Grid/Level.h>
#include <Core/Grid/SimpleMaterial.h>
#include <Core/Grid/Variables/VarTypes.h>
#include <Core/Grid/Variables/Vector5.h>
#include <Core/Grid/DbgOutput.h>
#include <Core/Grid/Variables/Utils.h>

#include <Core/Math/Matrix3.h>

#include <Core/Parallel/ProcessorGroup.h>
#include <Core/Malloc/Allocator.h>
#include <Core/Util/DebugStream.h>

#include <Core/Exceptions/ProblemSetupException.h>
#include <Core/Exceptions/InvalidValue.h>
#include <Core/Exceptions/ParameterNotFound.h>

using namespace Uintah;

//__________________________________
//  To turn on internal debugging code
//  setenv SCI_DEBUG "MINIAERO:+"
//  MINIAERO:   output when tasks are scheduled and performed
static DebugStream dbg("MINIAERO", false);

//TO DO: Clean up Stencil 7
//TO DO: Add Riemann Solver to faceCenteredFlux
//TO DO: Add 2nd order option to faceCenteredFlux
//TO DO: Add tasks for computing Viscous Flux 
//______________________________________________________________________
//  Preliminary
MiniAero::MiniAero(const ProcessorGroup* myworld)
    : UintahParallelComponent(myworld)
{
  conserved_label    = VarLabel::create("conserved",   CCVariable<Vector5>::getTypeDescription());

  rho_CClabel        = VarLabel::create("density",     CCVariable<double>::getTypeDescription());
  vel_CClabel        = VarLabel::create("velocity",    CCVariable<Vector>::getTypeDescription());
  press_CClabel      = VarLabel::create("pressure",    CCVariable<double>::getTypeDescription());
  temp_CClabel       = VarLabel::create("temperature", CCVariable<double>::getTypeDescription());
  speedSound_CClabel = VarLabel::create("speedsound",  CCVariable<double>::getTypeDescription());
  viscosityLabel     = VarLabel::create("viscosity",   CCVariable<double>::getTypeDescription());
  machlabel          = VarLabel::create("mach",        CCVariable<double>::getTypeDescription());

  flux_mass_CClabel    = VarLabel::create("flux_mass",         CCVariable<Vector>::getTypeDescription());
  flux_mom_CClabel     = VarLabel::create("flux_mom",          CCVariable<Matrix3>::getTypeDescription());
  flux_energy_CClabel  = VarLabel::create("flux_energy",       CCVariable<Vector>::getTypeDescription());

  flux_mass_FCXlabel   = VarLabel::create("faceX_flux_mass",   SFCXVariable<Vector>::getTypeDescription());
  flux_mom_FCXlabel    = VarLabel::create("faceX_flux_mom",    SFCXVariable<Vector>::getTypeDescription());
  flux_energy_FCXlabel = VarLabel::create("faceX_flux_energy", SFCXVariable<Vector>::getTypeDescription());

  flux_mass_FCYlabel   = VarLabel::create("faceY_flux_mass",   SFCYVariable<Vector>::getTypeDescription());
  flux_mom_FCYlabel    = VarLabel::create("faceY_flux_mom",    SFCYVariable<Vector>::getTypeDescription());
  flux_energy_FCYlabel = VarLabel::create("faceY_flux_energy", SFCYVariable<Vector>::getTypeDescription());

  flux_mass_FCZlabel   = VarLabel::create("faceZ_flux_mass",   SFCZVariable<Vector>::getTypeDescription());
  flux_mom_FCZlabel    = VarLabel::create("faceZ_flux_mom",    SFCZVariable<Vector>::getTypeDescription());
  flux_energy_FCZlabel = VarLabel::create("faceZ_flux_energy", SFCZVariable<Vector>::getTypeDescription());

  residual_CClabel = VarLabel::create("residual", CCVariable<Vector5>::getTypeDescription());
}

MiniAero::~MiniAero()
{
  VarLabel::destroy(conserved_label);

  VarLabel::destroy(rho_CClabel);
  VarLabel::destroy(vel_CClabel);
  VarLabel::destroy(press_CClabel);
  VarLabel::destroy(temp_CClabel);
  VarLabel::destroy(speedSound_CClabel);
  VarLabel::destroy(viscosityLabel);
  VarLabel::destroy(machlabel);

  VarLabel::destroy(flux_mass_CClabel);
  VarLabel::destroy(flux_mom_CClabel);
  VarLabel::destroy(flux_energy_CClabel);

  VarLabel::destroy(flux_mass_FCXlabel);
  VarLabel::destroy(flux_mom_FCXlabel);
  VarLabel::destroy(flux_energy_FCXlabel);

  VarLabel::destroy(flux_mass_FCYlabel);
  VarLabel::destroy(flux_mom_FCYlabel);
  VarLabel::destroy(flux_energy_FCYlabel);

  VarLabel::destroy(flux_mass_FCZlabel);
  VarLabel::destroy(flux_mom_FCZlabel);
  VarLabel::destroy(flux_energy_FCZlabel);

  VarLabel::destroy(residual_CClabel);
}

//______________________________________________________________________
//
void MiniAero::problemSetup(const ProblemSpecP& params,
                            const ProblemSpecP& restart_prob_spec,
                            GridP& /*grid*/,
                            SimulationStateP& sharedState)
{
  sharedState_ = sharedState;
  mymat_ = scinew SimpleMaterial();
  sharedState_->registerSimpleMaterial(mymat_);

  //__________________________________
  // Thermodynamic Transport Properties
  ProblemSpecP ps = params->findBlock("MiniAero");
  ps->require("gamma",        d_gamma);
  ps->require("R",            d_R);
  ps->require("CFL",          d_CFL);
  ps->require("Is_visc_flow", d_viscousFlow);
   
  //Getting geometry objects
  getGeometryObjects(ps, d_geom_objs);
}

//______________________________________________________________________
// 
void MiniAero::scheduleInitialize(const LevelP& level,
                                  SchedulerP& sched)
{
  Task* t = scinew Task("MiniAero::initialize", this, &MiniAero::initialize);

  t->computes(conserved_label);

  t->computes(vel_CClabel);
  t->computes(rho_CClabel);
  t->computes(press_CClabel);
  t->computes(temp_CClabel);
  t->computes(speedSound_CClabel);
  t->computes(viscosityLabel);

  sched->addTask(t, level->eachPatch(), sharedState_->allMaterials());
}

//______________________________________________________________________
// 
void MiniAero::scheduleComputeStableTimestep(const LevelP& level,
                                             SchedulerP& sched)
{
  Task* task = scinew Task("MiniAero::computeStableTimestep", this, &MiniAero::computeStableTimestep);

  Ghost::GhostType  gn = Ghost::None;

  task->requires(Task::NewDW, vel_CClabel,        gn );
  task->requires(Task::NewDW, speedSound_CClabel, gn );

  task->computes(sharedState_->get_delt_label(), level.get_rep());

  sched->addTask(task, level->eachPatch(), sharedState_->allMaterials());
}

//______________________________________________________________________
//
void MiniAero::scheduleTimeAdvance(const LevelP& level,
                                   SchedulerP& sched)
{
  schedCellCenteredFlux(level, sched);
  schedFaceCenteredFlux(level, sched);
  schedUpdateResidual(level, sched);
  schedUpdateState(level, sched);
  schedPrimitives(level,sched);
}

void MiniAero::schedPrimitives(const LevelP& level,
                               SchedulerP& sched)
{
  Task* task = scinew Task("MiniAero::Primitives", this, 
                           &MiniAero::Primitives);

  task->requires(Task::NewDW, conserved_label,Ghost::None);

  task->computes(rho_CClabel);
  task->computes(vel_CClabel);
  task->computes(press_CClabel);
  task->computes(temp_CClabel);
  task->computes(speedSound_CClabel);
  task->computes(machlabel);

  sched->addTask(task,level->eachPatch(),sharedState_->allMaterials());
}

//______________________________________________________________________
//
void MiniAero::schedCellCenteredFlux(const LevelP& level,
                                     SchedulerP& sched)
{
  Task* task = scinew Task("MiniAero::cellCenteredFlux", this, 
                           &MiniAero::cellCenteredFlux);

  task->requires(Task::OldDW,rho_CClabel,Ghost::None);
  task->requires(Task::OldDW,vel_CClabel,Ghost::None);
  task->requires(Task::OldDW,press_CClabel,Ghost::None);

  task->computes(flux_mass_CClabel);
  task->computes(flux_mom_CClabel);
  task->computes(flux_energy_CClabel);

  sched->addTask(task,level->eachPatch(),sharedState_->allMaterials());
}

//______________________________________________________________________
//
void MiniAero::schedFaceCenteredFlux(const LevelP& level,
                                     SchedulerP& sched)
{
  Task* task = scinew Task("MiniAero::faceCenteredFlux", this, 
                           &MiniAero::faceCenteredFlux);

  task->requires(Task::NewDW,flux_mass_CClabel,Ghost::AroundCells, 1);
  task->requires(Task::NewDW,flux_mom_CClabel,Ghost::AroundCells, 1);
  task->requires(Task::NewDW,flux_energy_CClabel,Ghost::AroundCells, 1);

  task->computes(flux_mass_FCXlabel);
  task->computes(flux_mom_FCXlabel);
  task->computes(flux_energy_FCXlabel);
  task->computes(flux_mass_FCYlabel);
  task->computes(flux_mom_FCYlabel);
  task->computes(flux_energy_FCYlabel);
  task->computes(flux_mass_FCZlabel);
  task->computes(flux_mom_FCZlabel);
  task->computes(flux_energy_FCZlabel);

  sched->addTask(task,level->eachPatch(),sharedState_->allMaterials());
}

void MiniAero::schedUpdateResidual(const LevelP& level,
                                   SchedulerP& sched)
{
  Task* task = scinew Task("MiniAero::updateResidual", this, 
                           &MiniAero::updateResidual);

  task->requires(Task::NewDW,flux_mass_FCXlabel,Ghost::None);
  task->requires(Task::NewDW,flux_mom_FCXlabel,Ghost::None);
  task->requires(Task::NewDW,flux_energy_FCXlabel,Ghost::None);

  task->requires(Task::NewDW,flux_mass_FCYlabel,Ghost::None);
  task->requires(Task::NewDW,flux_mom_FCYlabel,Ghost::None);
  task->requires(Task::NewDW,flux_energy_FCYlabel,Ghost::None);

  task->requires(Task::NewDW,flux_mass_FCZlabel,Ghost::None);
  task->requires(Task::NewDW,flux_mom_FCZlabel,Ghost::None);
  task->requires(Task::NewDW,flux_energy_FCZlabel,Ghost::None);

  task->computes(residual_CClabel);

  sched->addTask(task,level->eachPatch(),sharedState_->allMaterials());
}

void MiniAero::schedUpdateState(const LevelP& level,
                                SchedulerP& sched)
{
  Task* task = scinew Task("MiniAero::updateState", this, 
                           &MiniAero::updateState);

  task->requires(Task::OldDW,sharedState_->get_delt_label());
  task->requires(Task::OldDW,conserved_label,Ghost::None);
  task->requires(Task::NewDW,residual_CClabel,Ghost::None);
  task->requires(Task::OldDW, sharedState_->get_delt_label());

  task->computes(conserved_label);

  sched->addTask(task,level->eachPatch(),sharedState_->allMaterials());
}

//______________________________________________________________________
//
void MiniAero::computeStableTimestep(const ProcessorGroup*,
                                     const PatchSubset* patches,
                                     const MaterialSubset*,
                                     DataWarehouse*,
                                     DataWarehouse* new_dw)
{
  const Level* level = getLevel(patches);
  for(int p=0;p<patches->size();p++){
    const Patch* patch = patches->get(p);
    printTask(patches, patch, dbg, "Doing MiniAero::computeStableTimestep" );

    Vector dx = patch->dCell();
    double delX = dx.x();
    double delY = dx.y();
    double delZ = dx.z();
    double delt = 1000.0;

    IntVector badCell(0,0,0);
    int indx = 0; 

    Ghost::GhostType  gn  = Ghost::None;
    constCCVariable<double> speedSound;
    constCCVariable<Vector> vel_CC;
    new_dw->get(speedSound, speedSound_CClabel, indx, patch, gn, 0 );
    new_dw->get(vel_CC,     vel_CClabel,        indx, patch, gn, 0 );

    for(CellIterator iter=patch->getCellIterator(); !iter.done(); iter++){
      IntVector c = *iter;
      double speed_Sound = speedSound[c];

      double A = d_CFL*delX/(speed_Sound + fabs(vel_CC[c].x()) ); 
      double B = d_CFL*delY/(speed_Sound + fabs(vel_CC[c].y()) ); 
      double C = d_CFL*delZ/(speed_Sound + fabs(vel_CC[c].z()) );

      delt = std::min(A, delt);
      delt = std::min(B, delt);
      delt = std::min(C, delt);

      if (A < 1e-20 || B < 1e-20 || C < 1e-20) {
        if (badCell == IntVector(0,0,0)) {
          badCell = c;
        }
        std::cerr << d_myworld->myrank() << " Bad cell " << c << " (" << patch->getID()
                  << "-" << level->getIndex() << "): " << vel_CC[c]<< std::endl;
      }

      //__________________________________
    }
    //__________________________________
    //  Bullet proofing
    if(delt < 1e-20) {
      std::ostringstream warn;
      const Level* level = getLevel(patches);
      warn << "ERROR MINIAERO:(L-"<< level->getIndex()
           << "):ComputeStableTimestep: delT < 1e-20 on cell " << badCell;
      throw InvalidValue(warn.str(), __FILE__, __LINE__);
    }

   new_dw->put(delt_vartype(delt), sharedState_->get_delt_label(), getLevel(patches));
  } //Patch loop
}

//______________________________________________________________________
//
void MiniAero::initialize(const ProcessorGroup*,
                          const PatchSubset* patches,
                          const MaterialSubset* matls,
                          DataWarehouse*,
                          DataWarehouse* new_dw)
{
  const Level* level = getLevel(patches);
  int L_indx = level->getIndex();

  for(int p=0;p<patches->size();p++){
    const Patch* patch = patches->get(p);

    printTask(patches, patch, dbg, "Doing Miniaero::initialize" );

    CCVariable<Vector5> conserved_CC;

    CCVariable<double>  rho_CC;
    CCVariable<double>  Temp_CC;
    CCVariable<double>  press_CC;
    CCVariable<Vector>  vel_CC;
    CCVariable<double>  speedSound;
    CCVariable<double>  viscosity;
    int indx = 0; 

    //__________________________________
    new_dw->allocateAndPut(conserved_CC,  conserved_label, indx, patch);

    new_dw->allocateAndPut(rho_CC,     rho_CClabel,        indx, patch);
    new_dw->allocateAndPut(Temp_CC,    temp_CClabel,       indx, patch);
    new_dw->allocateAndPut(press_CC,   press_CClabel,      indx, patch);
    new_dw->allocateAndPut(vel_CC,     vel_CClabel,        indx, patch);
    new_dw->allocateAndPut(speedSound, speedSound_CClabel, indx, patch);
    new_dw->allocateAndPut(viscosity,  viscosityLabel,     indx, patch);

    initializeCells(rho_CC, Temp_CC, press_CC, vel_CC,  patch, new_dw, d_geom_objs);

    MiniAeroNS::setBC( rho_CC,   "Density",     patch, indx );
    MiniAeroNS::setBC( Temp_CC,  "Temperature", patch, indx );
    MiniAeroNS::setBC( vel_CC,   "Velocity",    patch, indx );
    MiniAeroNS::setBC( press_CC, "Pressure",    patch, indx );

    //__________________________________
    //  compute the speed of sound
    // set sp_vol_CC
    for (CellIterator iter = patch->getExtraCellIterator();!iter.done();iter++){
      IntVector c = *iter;
     
      double velX  = vel_CC[c].x();    // for readability
      double velY  = vel_CC[c].y();
      double velZ  = vel_CC[c].z();
      double rho   = rho_CC[c];
      
      conserved_CC[c].rho  = rho;
      conserved_CC[c].momX = rho * velX;
      conserved_CC[c].momY = rho * velY; 
      conserved_CC[c].momZ = rho * velZ;
      
      conserved_CC[c].eng  =  0.5*rho*( velX*velX + velY*velY + velZ*velZ)
                           +  press_CC[c]/(d_gamma-1.);
                           
      viscosity[c]  = getViscosity(Temp_CC[c]);
      speedSound[c] = sqrt( d_gamma * d_R * Temp_CC[c]);
    }

    //____ B U L L E T   P R O O F I N G----
    IntVector neg_cell;
    std::ostringstream warn, base;
    base <<"ERROR MINIAERO:(L-"<<L_indx<<"):initialize, mat cell ";

    if( !areAllValuesPositive(rho_CC, neg_cell) ) {
      warn << base.str()<< neg_cell << " rho_CC is negative\n";
      throw ProblemSetupException(warn.str(), __FILE__, __LINE__ );
    }
    if( !areAllValuesPositive(Temp_CC, neg_cell) ) {
      warn << base.str()<< neg_cell << " Temp_CC is negative\n";
      throw ProblemSetupException(warn.str(), __FILE__, __LINE__ );
    }
  }  // patch loop
    
}

void MiniAero::getGeometryObjects(ProblemSpecP& ps,
                                  std::vector<GeometryObject*>& geom_objs)
{
  //__________________________________
  // Loop through all of the pieces in this geometry object
  int piece_num = 0;
  
  std::list<GeometryObject::DataItem> geom_obj_data;
  geom_obj_data.push_back(GeometryObject::DataItem("res",        GeometryObject::IntVector));
  geom_obj_data.push_back(GeometryObject::DataItem("temperature",GeometryObject::Double));
  geom_obj_data.push_back(GeometryObject::DataItem("pressure",   GeometryObject::Double));
  geom_obj_data.push_back(GeometryObject::DataItem("density",    GeometryObject::Double));
  geom_obj_data.push_back(GeometryObject::DataItem("velocity",   GeometryObject::Vector));
  
  for (ProblemSpecP geom_obj_ps = ps->findBlock("geom_object");
       geom_obj_ps != 0;
       geom_obj_ps = geom_obj_ps->findNextBlock("geom_object") ) {
       
    std::vector<GeometryPieceP> pieces;
    GeometryPieceFactory::create(geom_obj_ps, pieces);
    
    GeometryPieceP mainpiece;
    if(pieces.size() == 0){
      throw ParameterNotFound("No piece specified in geom_object", __FILE__, __LINE__);
    } else if(pieces.size() > 1){ 
      mainpiece = scinew UnionGeometryPiece(pieces);
    } else {
      mainpiece = pieces[0];
    } 
    
    piece_num++;
    geom_objs.push_back(scinew GeometryObject(mainpiece, geom_obj_ps, geom_obj_data));
  } 
}

/* ---------------------------------------------------------------------
 Purpose~ Initialize primitive variables
_____________________________________________________________________*/
void MiniAero::initializeCells(CCVariable<double>& rho_CC,
                               CCVariable<double>& temp_CC,
                               CCVariable<double>& press_CC,
                               CCVariable<Vector>& vel_CC,
                               const Patch* patch,
                               DataWarehouse* new_dw,
                               std::vector<GeometryObject*>& geom_objs)
{
  // Initialize velocity, density, temperature, and pressure to "EVIL_NUM".
  // If any of these variables stay at that value, the initialization
  // is not done correctly.
  vel_CC.initialize(Vector(d_EVIL_NUM,d_EVIL_NUM,d_EVIL_NUM));
  rho_CC.initialize(d_EVIL_NUM);
  temp_CC.initialize(d_EVIL_NUM);
  press_CC.initialize(d_EVIL_NUM);

  // Loop over geometry objects
  for(int obj=0; obj<(int)geom_objs.size(); obj++){
    GeometryPieceP piece = geom_objs[obj]->getPiece();

    IntVector ppc   = geom_objs[obj]->getInitialData_IntVector("res");
    Vector dxpp     = patch->dCell()/ppc;
    Vector dcorner  = dxpp*0.5;

    for(CellIterator iter = patch->getExtraCellIterator();!iter.done();iter++){
      IntVector c = *iter;
      Point lower = patch->nodePosition(c) + dcorner;

      bool inside = false;
      for(int ix=0; !inside && ix < ppc.x(); ix++){
        for(int iy=0; !inside && iy < ppc.y(); iy++){
          for(int iz=0; !inside && iz < ppc.z(); iz++){

            IntVector idx(ix, iy, iz);
            Point p = lower + dxpp*idx;
            if(piece->inside(p))
              inside = true;
          }
        }
      }
      //__________________________________
      // For single materials with more than one object
      if (inside) {
        vel_CC[c]     = geom_objs[obj]->getInitialData_Vector("velocity");
        rho_CC[c]     = geom_objs[obj]->getInitialData_double("density");
        temp_CC[c]    = geom_objs[obj]->getInitialData_double("temperature");
        press_CC[c]   = d_R*rho_CC[c]*temp_CC[c];
      }
    }  // Loop over domain
  }  // Loop over geom_objects
}

//______________________________________________________________________
//
void MiniAero::Primitives(const ProcessorGroup* /*pg*/,
                          const PatchSubset* patches,
                          const MaterialSubset* /*matls*/,
                          DataWarehouse* old_dw,
                          DataWarehouse* new_dw)
{
  for(int p=0;p<patches->size();p++){
    const Patch* patch = patches->get(p);

    Ghost::GhostType  gn  = Ghost::None;

    // Requires...
    constCCVariable<Vector5> conserved;
    new_dw->get( conserved,  conserved_label, 0, patch, gn, 0 );

    // Provides...
    CCVariable<double> rho_CC, pressure_CC, Temp_CC, speedSound, mach;
    new_dw->allocateAndPut( rho_CC,      rho_CClabel,        0, patch );
    new_dw->allocateAndPut( pressure_CC, press_CClabel,      0, patch );
    new_dw->allocateAndPut( Temp_CC,     temp_CClabel,       0, patch );
    new_dw->allocateAndPut( speedSound,  speedSound_CClabel, 0, patch );
    new_dw->allocateAndPut( mach,        machlabel,          0, patch );

    CCVariable<Vector> vel_CC;
    new_dw->allocateAndPut( vel_CC, vel_CClabel,   0,patch );

    //__________________________________
    // Backout primitive quantities from
    // the conserved ones.
    for(CellIterator iter = patch->getCellIterator(); !iter.done(); iter++) {
      IntVector c = *iter;
      rho_CC[c]  = conserved[c].rho;

      vel_CC[c].x( conserved[c].momX/rho_CC[c] );
      vel_CC[c].y( conserved[c].momY/rho_CC[c] );
      vel_CC[c].z( conserved[c].momZ/rho_CC[c] );
      
      Temp_CC[c] = (d_gamma-1.)/d_R*(conserved[c].eng/conserved[c].rho - 0.5*vel_CC[c].length2());
      
      pressure_CC[c] = rho_CC[c] * d_R * Temp_CC[c];
    }
  
    MiniAeroNS::setBC( rho_CC,      "Density",     patch, 0 );
    MiniAeroNS::setBC( Temp_CC,     "Temperature", patch, 0 );
    MiniAeroNS::setBC( vel_CC,      "Velocity",    patch, 0 );
    MiniAeroNS::setBC( pressure_CC, "Pressure",    patch, 0 );

    //__________________________________
    // Compute Speed of Sound and Mach
    // This must be done after BCs are set
    for(CellIterator iter = patch->getCellIterator(); !iter.done(); iter++) {
      IntVector c = *iter;
      speedSound[c] = sqrt(d_gamma*d_R*Temp_CC[c]);
      mach[c] = vel_CC[c].length()/speedSound[c]; 
    } 

  }//Patch loop
}


void MiniAero::cellCenteredFlux(const ProcessorGroup* /*pg*/,
                                const PatchSubset* patches,
                                const MaterialSubset* /*matls*/,
                                DataWarehouse* old_dw,
                                DataWarehouse* new_dw)
{
  for(int p=0;p<patches->size();p++){
    const Patch* patch = patches->get(p);
    Ghost::GhostType  gn  = Ghost::None;

    
    constCCVariable<double> rho_CC, pressure_CC;
    constCCVariable<Vector> vel_CC;
    CCVariable<Vector> flux_mass_CC;
    CCVariable<Matrix3> flux_mom_CC;
    CCVariable<Vector> flux_energy_CC;

    old_dw->get( rho_CC,  rho_CClabel, 0, patch, gn, 0 );
    old_dw->get( vel_CC,  vel_CClabel, 0, patch, gn, 0 );
    old_dw->get( pressure_CC,  press_CClabel, 0, patch, gn, 0 );

    new_dw->allocateAndPut( flux_mass_CC, flux_mass_CClabel,   0,patch );
    new_dw->allocateAndPut( flux_mom_CC, flux_mom_CClabel,   0,patch );
    new_dw->allocateAndPut( flux_energy_CC, flux_energy_CClabel,   0,patch );

    //__________________________________
    // Backout primitive quantities from
    // the conserved ones.
    for(CellIterator iter = patch->getExtraCellIterator(); !iter.done(); iter++) {
      IntVector c = *iter;
      for (int idim=0; idim < 3; ++idim) {
        flux_mass_CC[c][idim]            = rho_CC[c]*vel_CC[c][idim];
        double KE=0;
        for (int jdim=0; jdim < 3; ++jdim) {
          KE += 0.5*rho_CC[c]*vel_CC[c][jdim]*vel_CC[c][jdim];
          flux_mom_CC[c](idim,jdim)      = rho_CC[c]*vel_CC[c][idim]*vel_CC[c][jdim];
        }
        flux_mom_CC[c](idim, idim) += pressure_CC[c];
        flux_energy_CC[c][idim]     = KE + pressure_CC[c]*(d_gamma/(d_gamma-1));
      }
    }
  }
}

void MiniAero::faceCenteredFlux(const ProcessorGroup* /*pg*/,
                                const PatchSubset* patches,
                                const MaterialSubset* /*matls*/,
                                DataWarehouse* old_dw,
                                DataWarehouse* new_dw)
{
  for(int p=0;p<patches->size();p++){
    const Patch* patch = patches->get(p);
    Ghost::GhostType  gac  = Ghost::AroundCells;

    constCCVariable<Vector> flux_mass_CC;
    constCCVariable<Matrix3> flux_mom_CC;
    constCCVariable<Vector> flux_energy_CC;
    SFCXVariable<double> flux_mass_FCX;
    SFCXVariable<Vector> flux_mom_FCX;
    SFCXVariable<double> flux_energy_FCX;
    SFCYVariable<double> flux_mass_FCY;
    SFCYVariable<Vector> flux_mom_FCY;
    SFCYVariable<double> flux_energy_FCY;
    SFCZVariable<double> flux_mass_FCZ;
    SFCZVariable<Vector> flux_mom_FCZ;
    SFCZVariable<double> flux_energy_FCZ;

    new_dw->get( flux_mass_CC,  flux_mass_CClabel, 0, patch, gac, 1 );
    new_dw->get( flux_mom_CC,  flux_mom_CClabel, 0, patch, gac, 1 );
    new_dw->get( flux_energy_CC,  flux_energy_CClabel, 0, patch, gac, 1 );

    new_dw->allocateAndPut( flux_mass_FCX, flux_mass_FCXlabel,   0,patch );
    new_dw->allocateAndPut( flux_mom_FCX, flux_mom_FCXlabel,   0,patch );
    new_dw->allocateAndPut( flux_energy_FCX, flux_energy_FCXlabel,   0,patch );

    new_dw->allocateAndPut( flux_mass_FCY, flux_mass_FCYlabel,   0,patch );
    new_dw->allocateAndPut( flux_mom_FCY, flux_mom_FCYlabel,   0,patch );
    new_dw->allocateAndPut( flux_energy_FCY, flux_energy_FCYlabel,   0,patch );

    new_dw->allocateAndPut( flux_mass_FCZ, flux_mass_FCZlabel,   0,patch );
    new_dw->allocateAndPut( flux_mom_FCZ, flux_mom_FCZlabel,   0,patch );
    new_dw->allocateAndPut( flux_energy_FCZ, flux_energy_FCZlabel,   0,patch );

    //__________________________________
    //Compute Face Centered Fluxes from Cell Centered
    for(CellIterator iter = patch->getSFCXIterator(); !iter.done(); iter++) {
      IntVector c = *iter;
      IntVector offset(-1,0,0);
      flux_mass_FCX  [c]    = 0.5*(flux_mass_CC  [c][0]  +flux_mass_CC  [c+offset][0]);
      flux_mom_FCX   [c][0] = 0.5*(flux_mom_CC   [c](0,0)+flux_mom_CC   [c+offset](0,0));
      flux_mom_FCX   [c][1] = 0.5*(flux_mom_CC   [c](0,1)+flux_mom_CC   [c+offset](0,1));
      flux_mom_FCX   [c][2] = 0.5*(flux_mom_CC   [c](0,2)+flux_mom_CC   [c+offset](0,2));
      flux_energy_FCX[c]    = 0.5*(flux_energy_CC[c][0]  +flux_energy_CC[c+offset][0]);
    }
    for(CellIterator iter = patch->getSFCYIterator(); !iter.done(); iter++) {
      IntVector c = *iter;
      IntVector offset(0,-1,0);
      flux_mass_FCY  [c]    = 0.5*(flux_mass_CC  [c][1]  +flux_mass_CC  [c+offset][1]);
      flux_mom_FCY   [c][0] = 0.5*(flux_mom_CC   [c](1,0)+flux_mom_CC   [c+offset](1,0));
      flux_mom_FCY   [c][1] = 0.5*(flux_mom_CC   [c](1,1)+flux_mom_CC   [c+offset](1,1));
      flux_mom_FCY   [c][2] = 0.5*(flux_mom_CC   [c](1,2)+flux_mom_CC   [c+offset](1,2));
      flux_energy_FCY[c]    = 0.5*(flux_energy_CC[c][1]  +flux_energy_CC[c+offset][1]);
    }
    for(CellIterator iter = patch->getSFCZIterator(); !iter.done(); iter++) {
      IntVector c = *iter;
      IntVector offset(0,0,-1);
      flux_mass_FCZ  [c]    = 0.5*(flux_mass_CC  [c][2]  +flux_mass_CC  [c+offset][2]);
      flux_mom_FCZ   [c][0] = 0.5*(flux_mom_CC   [c](2,0)+flux_mom_CC   [c+offset](2,0));
      flux_mom_FCZ   [c][1] = 0.5*(flux_mom_CC   [c](2,1)+flux_mom_CC   [c+offset](2,1));
      flux_mom_FCZ   [c][2] = 0.5*(flux_mom_CC   [c](2,2)+flux_mom_CC   [c+offset](2,2));
      flux_energy_FCZ[c]    = 0.5*(flux_energy_CC[c][2]  +flux_energy_CC[c+offset][2]);
    }
  }
}
void MiniAero::updateResidual(const ProcessorGroup* /*pg*/,
                              const PatchSubset* patches,
                              const MaterialSubset* /*matls*/,
                              DataWarehouse* old_dw,
                              DataWarehouse* new_dw)
{
  Ghost::GhostType  gn  = Ghost::None;

  for(int p=0;p<patches->size();p++){
    const Patch* patch = patches->get(p);

    constSFCXVariable<double> flux_mass_FCX;
    constSFCXVariable<Vector> flux_mom_FCX;
    constSFCXVariable<double> flux_energy_FCX;

    constSFCYVariable<double> flux_mass_FCY;
    constSFCYVariable<Vector> flux_mom_FCY;
    constSFCYVariable<double> flux_energy_FCY;

    constSFCZVariable<double> flux_mass_FCZ;
    constSFCZVariable<Vector> flux_mom_FCZ;
    constSFCZVariable<double> flux_energy_FCZ;

    CCVariable<Vector5> residual_CC;

    new_dw->get( flux_mass_FCX,  flux_mass_FCXlabel, 0, patch, gn, 0);
    new_dw->get( flux_mom_FCX,  flux_mom_FCXlabel, 0, patch, gn, 0);
    new_dw->get( flux_energy_FCX,  flux_energy_FCXlabel, 0, patch, gn, 0);

    new_dw->get( flux_mass_FCY,  flux_mass_FCYlabel, 0, patch, gn, 0);
    new_dw->get( flux_mom_FCY,  flux_mom_FCYlabel, 0, patch, gn, 0);
    new_dw->get( flux_energy_FCY,  flux_energy_FCYlabel, 0, patch, gn, 0);

    new_dw->get( flux_mass_FCZ,  flux_mass_FCZlabel, 0, patch, gn, 0);
    new_dw->get( flux_mom_FCZ,  flux_mom_FCZlabel, 0, patch, gn, 0);
    new_dw->get( flux_energy_FCZ,  flux_energy_FCZlabel, 0, patch, gn, 0);

    new_dw->allocateAndPut( residual_CC, residual_CClabel,   0,patch );

    //__________________________________
    // Backout primitive quantities from
    // the conserved ones.

    const Vector& cellSize = patch->getLevel()->dCell();
    const double dx = cellSize[0];
    const double dy = cellSize[1];
    const double dz = cellSize[2];

    for(CellIterator iter = patch->getCellIterator(); !iter.done(); iter++) {
      IntVector c = *iter;
      IntVector XOffset(1,0,0);
      IntVector YOffset(0,1,0);
      IntVector ZOffset(0,0,1);

      residual_CC[c][0] = (flux_mass_FCX[c + XOffset]  - flux_mass_FCX[c])*dy*dz + 
        (flux_mass_FCY[c + YOffset]  - flux_mass_FCY[c])*dx*dz + 
        (flux_mass_FCZ[c + ZOffset]  - flux_mass_FCZ[c])*dy*dx;

      residual_CC[c][4] = (flux_energy_FCX[c + XOffset]  - flux_energy_FCX[c])*dy*dz + 
        (flux_energy_FCY[c + YOffset]  - flux_energy_FCY[c])*dx*dz + 
        (flux_energy_FCZ[c + ZOffset]  - flux_energy_FCZ[c])*dy*dx;

      for(int idim = 0; idim < 3; ++idim) {
        residual_CC[c][idim + 1] =  (flux_mom_FCX[c + XOffset][idim]  - flux_mom_FCX[c][idim])*dy*dz + 
        (flux_mom_FCY[c + YOffset][idim]  - flux_mom_FCY[c][idim])*dx*dz + 
        (flux_mom_FCZ[c + ZOffset][idim]  - flux_mom_FCZ[c][idim])*dy*dx;
      }
    }
  }
}
void MiniAero::updateState(const ProcessorGroup* /*pg*/,
                           const PatchSubset* patches,
                           const MaterialSubset* /*matls*/,
                           DataWarehouse* old_dw,
                           DataWarehouse* new_dw)
{
  Ghost::GhostType  gn  = Ghost::None;
  delt_vartype dt;
  old_dw->get(dt, sharedState_->get_delt_label());

  for(int p=0;p<patches->size();p++){
    const Patch* patch = patches->get(p);
    const Vector& cellSize = patch->getLevel()->dCell();
    const double cell_volume = cellSize[0]*cellSize[1]*cellSize[2];

    constCCVariable<Vector5> residual_CC;
    constCCVariable<Vector5> oldState_CC;
    CCVariable<Vector5> newState_CC;

    new_dw->get( residual_CC,  residual_CClabel, 0, patch, gn, 0);
    old_dw->get( oldState_CC,  conserved_label,  0, patch, gn, 0);

    new_dw->allocateAndPut( newState_CC, conserved_label,   0,patch );


    for(CellIterator iter = patch->getCellIterator(); !iter.done(); iter++) {
      IntVector c = *iter;
  
      for(unsigned k = 0; k < 5; k++) {
        newState_CC[c][k] = oldState_CC[c][k] - dt*residual_CC[c][k]/(cell_volume);
      }
    }
  }
}
