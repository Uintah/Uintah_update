#include <Uintah/Components/ICE/ICE.h>
#include <Uintah/Interface/CFDInterface.h>
#include <Uintah/Grid/VarLabel.h>
#include <Uintah/Grid/CCVariable.h>
#include <SCICore/Geometry/Vector.h>
#include <Uintah/Parallel/ProcessorGroup.h>
#include <Uintah/Grid/Array3Index.h>
#include <Uintah/Grid/Grid.h>
#include <Uintah/Grid/Level.h>
#include <Uintah/Grid/CCVariable.h>
#include <Uintah/Grid/NCVariable.h>
#include <Uintah/Grid/ParticleSet.h>
#include <Uintah/Grid/ParticleVariable.h>
#include <Uintah/Interface/ProblemSpec.h>
#include <Uintah/Grid/NodeIterator.h>
#include <Uintah/Grid/Patch.h>
#include <Uintah/Grid/PerPatch.h>
#include <Uintah/Grid/ReductionVariable.h>
#include <Uintah/Grid/SimulationState.h>
#include <Uintah/Grid/SoleVariable.h>
#include <Uintah/Grid/Task.h>
#include <Uintah/Interface/DataWarehouse.h>
#include <Uintah/Interface/Scheduler.h>
#include <Uintah/Exceptions/ParameterNotFound.h>
#include <Uintah/Parallel/ProcessorGroup.h>
#include <Uintah/Components/ICE/ICEMaterial.h>
#include <Uintah/Interface/ProblemSpecP.h>
#include <Uintah/Grid/VarTypes.h>
#include <SCICore/Datatypes/DenseMatrix.h>
#include <vector>
#include <Uintah/Grid/BoundCond.h>
#include <Uintah/Grid/PressureBoundCond.h>
#include <Uintah/Grid/VelocityBoundCond.h>
#include <Uintah/Grid/TemperatureBoundCond.h>
#include <Uintah/Grid/DensityBoundCond.h>
#include <iomanip>

using std::vector;
using std::max;
using SCICore::Geometry::Vector;

using namespace Uintah;
using namespace Uintah::ICESpace;
using SCICore::Datatypes::DenseMatrix;

static int iterNum = 0;

#undef john_debug
#define john_debug 1
//______________________________________________________________________
//                                   I   C   E
ICE::ICE(const ProcessorGroup* myworld) 
  : UintahParallelComponent(myworld)
{
  lb = scinew ICELabel();

  IFS_CCLabel = scinew VarLabel("IFS_CC",
                                CCVariable<fflux>::getTypeDescription());
  OFS_CCLabel = scinew VarLabel("OFS_CC",
                                CCVariable<fflux>::getTypeDescription());
  IFE_CCLabel = scinew VarLabel("IFE_CC",
                                CCVariable<eflux>::getTypeDescription());
  OFE_CCLabel = scinew VarLabel("OFE_CC",
                                CCVariable<eflux>::getTypeDescription());
  q_outLabel = scinew VarLabel("q_out",
                                CCVariable<fflux>::getTypeDescription());
  q_out_EFLabel = scinew VarLabel("q_out_EF",
                                CCVariable<eflux>::getTypeDescription());
  q_inLabel = scinew VarLabel("q_in",
                                CCVariable<fflux>::getTypeDescription());
  q_in_EFLabel = scinew VarLabel("q_in_EF",
                                CCVariable<eflux>::getTypeDescription());

}

ICE::~ICE()
{
  delete lb;
  delete IFS_CCLabel;
  delete OFS_CCLabel;
  delete IFE_CCLabel;
  delete OFE_CCLabel;
  delete q_outLabel;
  delete q_out_EFLabel;
  delete q_inLabel;
  delete q_in_EFLabel;

}
/* ---------------------------------------------------------------------
 Function~  ICE::problemSetup--
 Purpose~   Grap the data from the input file

Programmer         Date       Description                      
----------         ----       -----------                 
John Schmidt      10/04/00                              
_____________________________________________________________________*/
void ICE::problemSetup(const ProblemSpecP& prob_spec, GridP& grid,
		       SimulationStateP& sharedState)
{
  d_sharedState = sharedState;
  d_SMALL_NUM = 1.e-12;

  cerr << "In the preprocessor . . ." << endl;

  ProblemSpecP cfl_ps = prob_spec->findBlock("CFD");
  cfl_ps->require("cfl",d_CFL);
  cout << "cfl = " << d_CFL << endl;

  ProblemSpecP time_ps = prob_spec->findBlock("Time");
  time_ps->require("delt_init",d_initialDt);
  cout << "Initial dt = " << d_initialDt << endl;
    
  // Search for the MaterialProperties block and then get the MPM section
  ProblemSpecP mat_ps       =  prob_spec->findBlock("MaterialProperties");
  ProblemSpecP ice_mat_ps   = mat_ps->findBlock("ICE");  

  for (ProblemSpecP ps = ice_mat_ps->findBlock("material"); ps != 0;
       ps = ps->findNextBlock("material") ) {
    // Extract out the type of EOS and the 
    // associated parameters
     ICEMaterial *mat = scinew ICEMaterial(ps);
     sharedState->registerMaterial(mat);
     sharedState->registerICEMaterial(mat);
  }     

  // Pull out the exchange coefficients
  ProblemSpecP exch_ps = ice_mat_ps->findBlock("exchange_coefficients");

  exch_ps->require("momentum",d_K_mom);
  exch_ps->require("heat",d_K_heat);

  for (int i = 0; i<(int)d_K_mom.size(); i++)
    cout << "K_mom = " << d_K_mom[i] << endl;
  for (int i = 0; i<(int)d_K_heat.size(); i++)
    cout << "K_heat = " << d_K_heat[i] << endl;

  cout << "Number of ICE materials: " << d_sharedState->getNumICEMatls()<< endl;

  ProblemSpecP ic_ps = prob_spec->findBlock("InitialConditions");
  ProblemSpecP ice_ic_ps = ic_ps->findBlock("ICE");
  ice_ic_ps->require("pressure",d_pressure);   
}
//STOP_DOC
/* ---------------------------------------------------------------------
 Function~  ICE::scheduleInitialize--
 Purpose~   Schedule the initialization of the dw variables

Programmer         Date       Description                      
----------         ----       -----------                 
John Schmidt      10/04/00                              
_____________________________________________________________________*/ 
void ICE::scheduleInitialize(
    const LevelP&   level, 
    SchedulerP&     sched, 
    DataWarehouseP& dw)
{
  Level::const_patchIterator iter;

  for(iter=level->patchesBegin(); iter != level->patchesEnd(); iter++)
  {
    const Patch* patch=*iter;
    Task* t = scinew Task("ICE::actuallyInitialize", patch, dw, dw,this,
			    &ICE::actuallyInitialize);
    t->computes( dw,    d_sharedState->get_delt_label());
     
    for (int m = 0; m < d_sharedState->getNumICEMatls(); m++ ) 
    {
	ICEMaterial*  matl = d_sharedState->getICEMaterial(m);
	int dwindex = matl->getDWIndex();
       
	t->computes( dw, lb->temp_CCLabel,      dwindex, patch);
	t->computes( dw, lb->rho_micro_CCLabel, dwindex, patch);
	t->computes( dw, lb->rho_CCLabel,       dwindex, patch);
	t->computes( dw, lb->cv_CCLabel,        dwindex, patch);
	t->computes( dw, lb->viscosity_CCLabel, dwindex, patch);
	t->computes( dw, lb->vol_frac_CCLabel,  dwindex, patch);
	t->computes( dw, lb->uvel_CCLabel,      dwindex, patch);
	t->computes( dw, lb->vvel_CCLabel,      dwindex, patch);
	t->computes( dw, lb->wvel_CCLabel,      dwindex, patch);
	t->computes( dw, lb->uvel_FCLabel,      dwindex, patch);
	t->computes( dw, lb->vvel_FCLabel,      dwindex, patch);
	t->computes( dw, lb->wvel_FCLabel,      dwindex, patch);
    }

    t->computes(dw, lb->press_CCLabel,0, patch);

    sched->addTask(t);
  }
}
//STOP_DOC
/* ---------------------------------------------------------------------
 Function~  ICE::scheduleComputeStableTimestep--
 Purpose~  Schedule a task to compute the time step for ICE

Programmer         Date       Description                      
----------         ----       -----------                 
John Schmidt    10/04/00                              
_____________________________________________________________________*/
void ICE::scheduleComputeStableTimestep(
    const LevelP&   level,
    SchedulerP&     sched,
    DataWarehouseP& dw)
{
#if 0
  // Compute the stable timestep
  int numMatls = d_sharedState->getNumICEMatls();

  for (Level::const_patchIterator iter = level->patchesBegin();
       iter != level->patchesEnd(); iter++) 
  {
        const Patch* patch = *iter;
  
        Task* task = scinew Task("ICE::actuallyComputeStableTimestep",patch, dw,
			       dw,this, &ICE::actuallyComputeStableTimestep);

      for (int m = 0; m < numMatls; m++) 
      {
        ICEMaterial* matl = d_sharedState->getICEMaterial(m);
        int dwindex = matl->getDWIndex();
        task->requires(dw, lb->uvel_CCLabel,    dwindex,    patch,  Ghost::None);
        task->requires(dw, lb->vvel_CCLabel,    dwindex,    patch,  Ghost::None);
        task->requires(dw, lb->wvel_CCLabel,    dwindex,    patch,  Ghost::None);
      }
      sched->addTask(task);
  }
#endif
}
//STOP_DOC
/* ---------------------------------------------------------------------
 Function~  ICE::scheduleTimeAdvance--
 Purpose~   Schedule tasks for each of the major steps 

Programmer         Date       Description                      
----------         ----       -----------                 
John Schmidt      10/04/00                             
_____________________________________________________________________*/
void ICE::scheduleTimeAdvance(
    double t,   
    double dt,
    const LevelP&   level,
    SchedulerP&     sched,
    DataWarehouseP& old_dw,
    DataWarehouseP& new_dw)
{
  for(Level::const_patchIterator iter=level->patchesBegin();
       iter != level->patchesEnd(); iter++)
  {
    const Patch* patch=*iter;

    // Step 1a  computeSoundSpeed
    scheduleStep1a( patch,  sched,  old_dw, new_dw);
    
    // Step 1b calculate equlibration pressure
    scheduleStep1b( patch,  sched,  old_dw, new_dw);
    
    // Step 1c compute face centered velocities
    scheduleStep1c( patch,  sched,  old_dw, new_dw);
    
    // Step 1d computes momentum exchange on FC velocities
    scheduleStep1d( patch,  sched,  old_dw, new_dw);
    
    // Step 2 computes delPress and the new pressure
    scheduleStep2(  patch,  sched,  old_dw, new_dw);
    
    // Step 3 compute face centered pressure
    scheduleStep3(  patch,  sched,  old_dw, new_dw);
    
    // Step 4a compute sources of momentum
    scheduleStep4a( patch,  sched,  old_dw, new_dw);
    
    // Step 4b compute sources of energy
    scheduleStep4b( patch,  sched,  old_dw, new_dw);
    
    // Step 5a compute lagrangian quantities
    scheduleStep5a( patch,  sched,  old_dw, new_dw);
    
    // Step 5b cell centered momentum exchange
    scheduleStep5b( patch,  sched,  old_dw, new_dw);
    
    // Step 6and7 advect and advance in time
    scheduleStep6and7(patch,sched,  old_dw, new_dw);
  }
}
//STOP_DOC
/* ---------------------------------------------------------------------
 Function~  ICE::scheduleStep1a--
 Purpose~   Compute the speed of sound
 
Programmer         Date       Description                      
----------         ----       -----------                 
John Schmidt      10/04/00                             
_____________________________________________________________________*/
void ICE::scheduleStep1a(
    const Patch*    patch,
    SchedulerP&     sched,
    DataWarehouseP& old_dw,
    DataWarehouseP& new_dw)
{
  int numMatls = d_sharedState->getNumICEMatls();
  
  Task* task = scinew Task("ICE::step1a",patch, old_dw, new_dw,this,
			&ICE::actuallyStep1a);
  for (int m = 0; m < numMatls; m++) 
  {
    ICEMaterial* matl = d_sharedState->getICEMaterial(m);
    EquationOfState* eos = matl->getEOS();
    // Compute the speed of sound
    eos->addComputesAndRequiresSS(task,matl,patch,old_dw,new_dw);
  }
  sched->addTask(task);
}
//STOP_DOC
/* ---------------------------------------------------------------------
 Function~  ICE::scheduleStep1b--
 Purpose~   Compute the equilibration pressure
 
Programmer         Date       Description                      
----------         ----       -----------                 
John Schmidt      10/04/00                             
_____________________________________________________________________*/
void ICE::scheduleStep1b(
    const Patch*    patch,
    SchedulerP&     sched,
    DataWarehouseP& old_dw,
    DataWarehouseP& new_dw)
{
  Task* task = scinew Task("ICE::step1b",patch, old_dw, new_dw,this,
			   &ICE::actuallyStep1b);
  
  task->requires(old_dw,lb->press_CCLabel, 0,patch,Ghost::None);
  
  int numMatls=d_sharedState->getNumICEMatls();
  for (int m = 0; m < numMatls; m++) 
  {
    ICEMaterial*  matl = d_sharedState->getICEMaterial(m);
    int dwindex = matl->getDWIndex();
    EquationOfState* eos = matl->getEOS();
    // Compute the rho micro
    eos->addComputesAndRequiresRM(task,matl,patch,old_dw,new_dw);
    task->requires(old_dw,lb->vol_frac_CCLabel,  dwindex,patch,Ghost::None);
    task->requires(old_dw,lb->rho_CCLabel,       dwindex,patch,Ghost::None);
    task->requires(old_dw,lb->rho_micro_CCLabel, dwindex,patch,Ghost::None);
    task->requires(old_dw,lb->temp_CCLabel,      dwindex,patch,Ghost::None);
    task->requires(old_dw,lb->cv_CCLabel,        dwindex,patch,Ghost::None);
    task->requires(new_dw,lb->speedSound_CCLabel,dwindex,patch,Ghost::None);
    task->computes(new_dw,lb->vol_frac_CCLabel,          dwindex, patch);
    task->computes(new_dw,lb->speedSound_equiv_CCLabel,  dwindex, patch);
    task->computes(new_dw,lb->rho_micro_equil_CCLabel,   dwindex, patch);
  }

  task->computes(new_dw,lb->press_CCLabel,0, patch);
  sched->addTask(task);
}
//STOP_DOC
/* ---------------------------------------------------------------------
 Function~  ICE::scheduleStep1c--
 Purpose~   Compute the face-centered velocities

Version   Programmer         Date       Description                      
-------   ----------         ----       -----------                 
  1.0     John Schmidt      10/04/00                             
_____________________________________________________________________*/
void ICE::scheduleStep1c(
    const Patch*    patch,
    SchedulerP&     sched,
    DataWarehouseP& old_dw,
    DataWarehouseP& new_dw)
{
  Task* task = scinew Task("ICE::step1c",patch, old_dw, new_dw,this,
			   &ICE::actuallyStep1c);

  task->requires(new_dw,lb->press_CCLabel,0,patch,Ghost::None);

  int numMatls=d_sharedState->getNumICEMatls();
  for (int m = 0; m < numMatls; m++) 
  {
    ICEMaterial* matl = d_sharedState->getICEMaterial(m);
    int dwindex = matl->getDWIndex();
    task->requires(old_dw,lb->rho_CCLabel,   dwindex,patch,Ghost::None);
    task->requires(old_dw,lb->uvel_CCLabel,  dwindex,patch,Ghost::None);
    task->requires(old_dw,lb->vvel_CCLabel,  dwindex,patch,Ghost::None);
    task->requires(old_dw,lb->wvel_CCLabel,  dwindex,patch,Ghost::None);
    task->requires(new_dw,lb->rho_micro_equil_CCLabel,
		                               dwindex,patch,Ghost::None);
    
    task->computes(new_dw,lb->uvel_FCLabel,  dwindex, patch);
    task->computes(new_dw,lb->vvel_FCLabel,  dwindex, patch);
    task->computes(new_dw,lb->wvel_FCLabel,  dwindex, patch);
  }
  sched->addTask(task);
}
//STOP_DOC
/* ---------------------------------------------------------------------
 Function~  ICE::scheduleStep1d--
 Purpose~   Schedule compute the momentum exchange for the face centered 
            velocities

Programmer         Date       Description                      
----------         ----       -----------                 
John Schmidt      10/04/00                             
_____________________________________________________________________*/
void ICE::scheduleStep1d(
    const Patch*    patch,
    SchedulerP&     sched,
    DataWarehouseP& old_dw,
    DataWarehouseP& new_dw)
{
  Task* task = scinew Task("ICE::step1d",patch, old_dw, new_dw,this,
			   &ICE::actuallyStep1d);
  int numMatls=d_sharedState->getNumICEMatls();
  
  for (int m = 0; m < numMatls; m++) 
  {
    ICEMaterial* matl = d_sharedState->getICEMaterial(m);
    int dwindex = matl->getDWIndex();
    task->requires(new_dw,lb->rho_micro_equil_CCLabel,
		                                  dwindex,patch,Ghost::None);
    task->requires(new_dw,lb->vol_frac_CCLabel, dwindex, patch,Ghost::None);
    task->requires(old_dw,lb->uvel_FCLabel,     dwindex, patch,Ghost::None);
    task->requires(old_dw,lb->vvel_FCLabel,     dwindex, patch,Ghost::None);
    task->requires(old_dw,lb->wvel_FCLabel,     dwindex, patch,Ghost::None);
    
    task->computes(new_dw,lb->uvel_FCMELabel,   dwindex, patch);
    task->computes(new_dw,lb->vvel_FCMELabel,   dwindex, patch);
    task->computes(new_dw,lb->wvel_FCMELabel,   dwindex, patch);
  }
  sched->addTask(task);
}
//STOP_DOC
/* ---------------------------------------------------------------------
 Function~  ICE::scheduleStep2--
 Purpose~   Schedule compute delpress and new press_CC

Programmer         Date       Description                      
----------         ----       -----------                 
John Schmidt      10/04/00                             
_____________________________________________________________________*/
void ICE::scheduleStep2(
    const Patch*    patch,
    SchedulerP&     sched,
    DataWarehouseP& old_dw,
    DataWarehouseP& new_dw)
{
  Task* task = scinew Task("ICE::step2",patch, old_dw, new_dw,this,
			   &ICE::actuallyStep2);
  
  task->requires(new_dw,lb->press_CCLabel, 0,patch,Ghost::None);
  int numMatls=d_sharedState->getNumICEMatls();
  for (int m = 0; m < numMatls; m++) 
  {
    ICEMaterial* matl = d_sharedState->getICEMaterial(m);
    int dwindex = matl->getDWIndex();
    task->requires( new_dw, lb->vol_frac_CCLabel,  dwindex,patch,Ghost::None);
    task->requires( new_dw, lb->uvel_FCMELabel,    dwindex,patch,Ghost::None);
    task->requires( new_dw, lb->vvel_FCMELabel,    dwindex,patch,Ghost::None);
    task->requires( new_dw, lb->wvel_FCMELabel,    dwindex,patch,Ghost::None);
    task->requires( new_dw, lb->speedSound_equiv_CCLabel,
                                                   dwindex,patch,Ghost::None);
    task->requires( new_dw, lb->rho_micro_equil_CCLabel,
		                                     dwindex,patch,Ghost::None);
  }
  task->computes(   new_dw,lb->pressdP_CCLabel,     0,     patch);
  task->computes(   new_dw,lb->delPress_CCLabel,    0,     patch);
  
  sched->addTask(task);
}
//STOP_DOC
/* ---------------------------------------------------------------------
 Function~  ICE::scheduleStep3--
 Purpose~   Schedule compute face centered pressure press_FC

Programmer         Date       Description                      
----------         ----       -----------                 
John Schmidt      10/04/00                             
_____________________________________________________________________*/
void ICE::scheduleStep3(
    const Patch*    patch,
    SchedulerP&     sched,
    DataWarehouseP& old_dw,
    DataWarehouseP& new_dw)
{
  Task* task = scinew Task("ICE::step3",patch, old_dw, new_dw,this,
			   &ICE::actuallyStep3);
  
  task->requires(   new_dw,lb->pressdP_CCLabel, 0,      patch,  Ghost::None);
  int numMatls = d_sharedState->getNumICEMatls();
  for (int m = 0; m < numMatls; m++) 
  {
    ICEMaterial* matl = d_sharedState->getICEMaterial(m);
    int dwindex = matl->getDWIndex();
    task->requires( old_dw, lb->rho_CCLabel,    dwindex, patch, Ghost::None);
  }
  
  task->computes(   new_dw, lb->pressX_FCLabel, 0,      patch);
  task->computes(   new_dw, lb->pressY_FCLabel, 0,      patch);
  task->computes(   new_dw, lb->pressZ_FCLabel, 0,      patch);
  
  sched->addTask(task);
}
//STOP_DOC
/* ---------------------------------------------------------------------
 Function~  ICE::scheduleStep4a--
 Purpose~   Schedule compute sources and sinks of momentum

Programmer         Date       Description                      
----------         ----       -----------                 
John Schmidt      10/04/00                             
_____________________________________________________________________*/
void ICE::scheduleStep4a(
    const Patch*    patch,
    SchedulerP&     sched,
    DataWarehouseP& old_dw,
    DataWarehouseP& new_dw)
{
  Task* task = scinew Task("ICE::step4a",
            patch,      old_dw,         new_dw,     this,
	     &ICE::actuallyStep4a);

  task->requires(new_dw,    lb->pressX_FCLabel,     0,  patch,  Ghost::None);
  task->requires(new_dw,    lb->pressY_FCLabel,     0,  patch,  Ghost::None);
  task->requires(new_dw,    lb->pressZ_FCLabel,     0,  patch,  Ghost::None);
  int numMatls=d_sharedState->getNumICEMatls();
  
  for (int m = 0; m < numMatls; m++) 
  {
    ICEMaterial* matl = d_sharedState->getICEMaterial(m);
    int dwindex = matl->getDWIndex();
    task->requires(old_dw,  lb->rho_CCLabel,        dwindex,patch,Ghost::None);
    task->requires(old_dw,  lb->uvel_CCLabel,       dwindex,patch,Ghost::None);
    task->requires(old_dw,  lb->vvel_CCLabel,       dwindex,patch,Ghost::None);
    task->requires(old_dw,  lb->wvel_CCLabel,       dwindex,patch,Ghost::None);
    task->requires(old_dw,  lb->viscosity_CCLabel,  dwindex,patch,Ghost::None);
    task->requires(new_dw,  lb->vol_frac_CCLabel,   dwindex,patch,Ghost::None);
 
    task->computes(new_dw,  lb->xmom_source_CCLabel,dwindex,patch);
    task->computes(new_dw,  lb->ymom_source_CCLabel,dwindex,patch);
    task->computes(new_dw,  lb->zmom_source_CCLabel,dwindex,patch);
    task->computes(new_dw,  lb->tau_X_FCLabel,      dwindex,patch);
    task->computes(new_dw,  lb->tau_Y_FCLabel,      dwindex,patch);
    task->computes(new_dw,  lb->tau_Z_FCLabel,      dwindex,patch);
  }
  sched->addTask(task);
}
//STOP_DOC
/* ---------------------------------------------------------------------
 Function~  ICE::scheduleStep4b--
 Purpose~   Schedule compute sources and sinks of energy

Programmer         Date       Description                      
----------         ----       -----------                 
John Schmidt      10/04/00                             
_____________________________________________________________________*/
void ICE::scheduleStep4b(
    const Patch*    patch,
    SchedulerP&     sched,
    DataWarehouseP& old_dw,
    DataWarehouseP& new_dw)

{
  Task* task = scinew Task("ICE::step4b",patch, old_dw, new_dw,this,
			   &ICE::actuallyStep4b);
  
  task->requires(new_dw,    lb->press_CCLabel,    0, patch, Ghost::None);
  task->requires(new_dw,    lb->delPress_CCLabel, 0, patch, Ghost::None);
  int numMatls=d_sharedState->getNumICEMatls();
  
  for (int m = 0; m < numMatls; m++) 
  {
    ICEMaterial* matl = d_sharedState->getICEMaterial(m);
    int dwindex = matl->getDWIndex();
    
    task->requires( new_dw, lb->rho_micro_equil_CCLabel,    dwindex, patch,
                                                            Ghost::None);
    task->requires( new_dw, lb->speedSound_equiv_CCLabel,   dwindex, patch,
                                                            Ghost::None);
    task->requires( new_dw, lb->vol_frac_CCLabel,           dwindex, patch,
                                                            Ghost::None);
    
    task->computes (new_dw, lb->int_eng_source_CCLabel,     dwindex, patch);
  }
  sched->addTask(task);
}
//STOP_DOC
/* ---------------------------------------------------------------------
 Function~  ICE::scheduleStep5a--
 Purpose~   Schedule compute lagrangian mass momentum and internal energy
 
Programmer         Date       Description                      
----------         ----       -----------                 
John Schmidt      10/04/00                             
_____________________________________________________________________*/
void ICE::scheduleStep5a(
    const Patch*    patch,
    SchedulerP&     sched,
    DataWarehouseP& old_dw,
    DataWarehouseP& new_dw)
{
  Task* task = scinew Task("ICE::step5a",patch, old_dw, new_dw,this,
			       &ICE::actuallyStep5a);
  int numMatls=d_sharedState->getNumICEMatls();
  for (int m = 0; m < numMatls; m++) 
  {
    ICEMaterial* matl = d_sharedState->getICEMaterial(m);
    int dwindex = matl->getDWIndex();
    task->requires( old_dw, lb->rho_CCLabel,        dwindex,patch,Ghost::None);
    task->requires( old_dw, lb->uvel_CCLabel,       dwindex,patch,Ghost::None);
    task->requires( old_dw, lb->vvel_CCLabel,       dwindex,patch,Ghost::None);
    task->requires( old_dw, lb->wvel_CCLabel,       dwindex,patch,Ghost::None);
    task->requires( old_dw, lb->cv_CCLabel,         dwindex,patch,Ghost::None);
    task->requires( old_dw, lb->temp_CCLabel,       dwindex,patch,Ghost::None);
    task->requires( new_dw, lb->xmom_source_CCLabel,dwindex,patch,Ghost::None);
    task->requires( new_dw, lb->ymom_source_CCLabel,dwindex,patch,Ghost::None);
    task->requires( new_dw, lb->zmom_source_CCLabel,dwindex,patch,Ghost::None);
    task->requires( new_dw, lb->int_eng_source_CCLabel,
		                                      dwindex,patch,Ghost::None);
 
    task->computes( new_dw, lb->xmom_L_CCLabel,     dwindex,patch);
    task->computes( new_dw, lb->ymom_L_CCLabel,     dwindex,patch);
    task->computes( new_dw, lb->zmom_L_CCLabel,     dwindex,patch);
    task->computes( new_dw, lb->int_eng_L_CCLabel,  dwindex,patch);
    task->computes( new_dw, lb->mass_L_CCLabel,     dwindex,patch);
    task->computes( new_dw, lb->rho_L_CCLabel,      dwindex,patch);
  }
  sched->addTask(task);
}
//STOP_DOC
/* ---------------------------------------------------------------------
 Function~  ICE::scheduleStep5b--
 Purpose~   Schedule momentum and energy exchange on the lagrangian quantities

Programmer         Date       Description                      
----------         ----       -----------                 
John Schmidt      10/04/00                             
_____________________________________________________________________*/
void ICE::scheduleStep5b(
    const Patch*    patch,
    SchedulerP&     sched,
    DataWarehouseP& old_dw,
    DataWarehouseP& new_dw)
{
  Task* task = scinew Task("ICE::step5b",patch, old_dw, new_dw,this,
			   &ICE::actuallyStep5b);
  int numMatls=d_sharedState->getNumICEMatls();
  
  for (int m = 0; m < numMatls; m++) 
  {
    ICEMaterial* matl = d_sharedState->getICEMaterial(m);
    int dwindex = matl->getDWIndex();
    task->requires( old_dw, lb->rho_CCLabel,        dwindex,patch,Ghost::None);
    task->requires( new_dw, lb->xmom_L_CCLabel,     dwindex,patch,Ghost::None);
    task->requires( new_dw, lb->ymom_L_CCLabel,     dwindex,patch,Ghost::None);
    task->requires( new_dw, lb->zmom_L_CCLabel,     dwindex,patch,Ghost::None);
    task->requires( new_dw, lb->int_eng_L_CCLabel,  dwindex,patch,Ghost::None);
    task->requires( new_dw, lb->vol_frac_CCLabel,   dwindex,patch,Ghost::None);
    task->requires( old_dw, lb->cv_CCLabel,         dwindex,patch,Ghost::None);
    task->requires( new_dw, lb->rho_micro_equil_CCLabel,
		                                      dwindex,patch,Ghost::None);
    
    task->computes( new_dw, lb->xmom_L_ME_CCLabel,  dwindex,patch);
    task->computes( new_dw, lb->ymom_L_ME_CCLabel,  dwindex,patch);
    task->computes( new_dw, lb->zmom_L_ME_CCLabel,  dwindex,patch);
    task->computes( new_dw, lb->int_eng_L_ME_CCLabel,dwindex,patch);
  }
  sched->addTask(task);
}
//STOP_DOC
/* ---------------------------------------------------------------------
 Function~  ICE::scheduleStep6and7--
 Purpose~   Schedule advance and advect in time for mass, momentum
            and energy.  Note this function puts (*)vel_CC, rho_CC
            and Temp_CC into new dw, not flux variables

Programmer         Date       Description                      
----------         ----       -----------                 
John Schmidt      10/04/00                             
_____________________________________________________________________*/
void ICE::scheduleStep6and7(
    const Patch*    patch,
    SchedulerP&     sched,
    DataWarehouseP& old_dw,
    DataWarehouseP& new_dw)
{
  Task* task = scinew Task("ICE::step6and7",patch, old_dw, new_dw,this,
			   &ICE::actuallyStep6and7);
  int numMatls=d_sharedState->getNumICEMatls();
  for (int m = 0; m < numMatls; m++ ) 
  {
    ICEMaterial* matl = d_sharedState->getICEMaterial(m);
    int dwindex = matl->getDWIndex();
    task->requires(old_dw, lb->cv_CCLabel,dwindex,patch,Ghost::None,0);
    task->requires(old_dw, lb->rho_CCLabel,       dwindex,patch,Ghost::None);
    task->requires(old_dw, lb->uvel_CCLabel,      dwindex,patch,Ghost::None);
    task->requires(old_dw, lb->vvel_CCLabel,      dwindex,patch,Ghost::None);
    task->requires(old_dw, lb->wvel_CCLabel,      dwindex,patch,Ghost::None);
    task->requires(old_dw, lb->temp_CCLabel,      dwindex,patch,Ghost::None);
    task->requires(new_dw, lb->xmom_L_ME_CCLabel,
		                                   dwindex,patch,Ghost::None,0);
    task->requires(new_dw, lb->ymom_L_ME_CCLabel,
		                                   dwindex,patch,Ghost::None,0);
    task->requires(new_dw, lb->zmom_L_ME_CCLabel,
		                                   dwindex,patch,Ghost::None,0);
    task->requires(new_dw, lb->int_eng_L_ME_CCLabel,
		                                   dwindex,patch,Ghost::None,0);

    task->requires(new_dw, lb->speedSound_CCLabel,dwindex,patch,Ghost::None);
    task->computes(new_dw, lb->temp_CCLabel,      dwindex,patch);
    task->computes(new_dw, lb->rho_CCLabel,       dwindex,patch);
    task->computes(new_dw, lb->cv_CCLabel,        dwindex,patch);
    task->computes(new_dw, lb->uvel_CCLabel,      dwindex,patch);
    task->computes(new_dw, lb->vvel_CCLabel,      dwindex,patch);
    task->computes(new_dw, lb->wvel_CCLabel,      dwindex,patch);
  }
  task->computes(new_dw, d_sharedState->get_delt_label());
  sched->addTask(task);
}
//STOP_DOC
/* ---------------------------------------------------------------------
 Function~  ICE::actuallyComputeStableTimestep--
 Purpose~   Compute next time step based on speed of sound and 
            maximum velocity in the domain
            
             C U R R E N T L Y   T U R N E D   O F F

Programmer         Date       Description                      
----------         ----       -----------                 
John Schmidt      10/04/00                             
_____________________________________________________________________*/
void ICE::actuallyComputeStableTimestep(
    const ProcessorGroup*,
    const Patch*    patch,
    DataWarehouseP& old_dw,
    DataWarehouseP& new_dw)
{
  cout << "Doing actually Compute Stable Timestep " << endl;
#if 0
  Vector dx = patch->dCell();
  double delt_CFL = 100000,delt_stability = 1000000,fudge_factor = 1.;
  CCVariable<double> uvel,vvel,wvel,speedSound;
  double CFL,N_ITERATIONS_TO_STABILIZE = 2;
 
  ::iterNum++;
  if (iterNum < N_ITERATIONS_TO_STABILIZE) {
    CFL = d_CFL * (double)(::iterNum) *(1./(double)N_ITERATIONS_TO_STABILIZE);
  } else {
    CFL = d_CFL;
  }

  for (int m = 0; m < d_sharedState->getNumICEMatls(); m++) {
    ICEMaterial* ice_matl = d_sharedState->getICEMaterial(m);
    int dwindex = ice_matl->getDWIndex();

    new_dw->get(speedSound, lb->speedSound_equiv_CCLabel,
		dwindex,patch,Ghost::None, 0);
    new_dw->get(uvel, lb->uvel_CCLabel, dwindex,patch,Ghost::None, 0);
    new_dw->get(vvel, lb->vvel_CCLabel, dwindex,patch,Ghost::None, 0);
    new_dw->get(wvel, lb->wvel_CCLabel, dwindex,patch,Ghost::None, 0);
        
    for(CellIterator iter = patch->getCellIterator(); !iter.done(); iter++){   
      double A = fudge_factor*CFL*dx.x()/(speedSound[*iter] + 
					  fabs(uvel[*iter]) + d_SMALL_NUM);
      double B = fudge_factor*CFL*dx.y()/(speedSound[*iter] + 
					  fabs(vvel[*iter]) + d_SMALL_NUM);
      double C = fudge_factor*CFL*dx.z()/(speedSound[*iter] + 
					  fabs(wvel[*iter]) + d_SMALL_NUM);
      
      delt_CFL = std::min(A, delt_CFL);
      delt_CFL = std::min(B, delt_CFL);
      delt_CFL = std::min(C, delt_CFL);

      A = fudge_factor * 0.5 * (dx.x()*dx.x())/fabs(uvel[*iter]);
      B = fudge_factor * 0.5 * (dx.y()*dx.y())/fabs(vvel[*iter]);
      C = fudge_factor * 0.5 * (dx.z()*dx.z())/fabs(wvel[*iter]);

      delt_stability = std::min(A, delt_stability);
      delt_stability = std::min(B, delt_stability);
      delt_stability = std::min(C, delt_stability);
    }
  }
  
  double dT = std::min(delt_stability, delt_CFL);
  cout << "new dT = " << dT << endl;
  new_dw->put(delt_vartype(dT), lb->delTLabel);

#endif
}
//STOP_DOC
/* --------------------------------------------------------------------- 
 Function~  ICE::actuallyInitialize--
 Purpose~  Initialize the CC and FC variables and the pressure  
            
Programmer         Date       Description                      
----------         ----       -----------                 
John Schmidt      10/04/00                              
_____________________________________________________________________*/ 
void ICE::actuallyInitialize(
    const ProcessorGroup*,
    const Patch*    patch,
    DataWarehouseP& old_dw,
    DataWarehouseP& new_dw)
{
  //__________________________________
  //  Allocate local memory
  //  - put dt in new dw
  cout << "Doing actually Initialize" << endl;
  double dT = d_initialDt;
  CCVariable<double>    rho_micro,      Temp_CC,        cv,     
                        rho_CC,         press,          speedSound,
                        uvel_CC,        vvel_CC,        wvel_CC,
                        visc_CC,        vol_frac_CC;

  SFCXVariable<double>  uvel_FC;
  SFCYVariable<double>  vvel_FC;
  SFCZVariable<double>  wvel_FC;

  new_dw->put(delt_vartype(dT), lb->delTLabel);
  
  //__________________________________
  // press_CC
  // - allocate new dw memory
  // - initialize it
  // - set the BCs
  // - put it back to the new dw
  new_dw->allocate(press,lb->press_CCLabel, 0,patch);
#ifdef john_debug
  cout << "Initial pressure = " << d_pressure << endl;
#endif   
  press.initialize(d_pressure);

  setBC(press,"Pressure",patch);

  new_dw->put(press,    lb->press_CCLabel,  0,patch);
  
#ifdef john_debug
  cout << "Initial pressure = " << d_pressure << endl;
  for(CellIterator iter = patch->getCellIterator(); !iter.done(); iter++){
    cout << "press["<< *iter<< "]=" << press[*iter] << endl;
  } 
#endif
  //__________________________________
  //  Now operate on material dependent fields
  for (int m = 0; m < d_sharedState->getNumICEMatls(); m++ ) 
  {
    //__________________________________
    //  Allocate new DW memory
      ICEMaterial* ice_matl = d_sharedState->getICEMaterial(m);
      int dwindex = ice_matl->getDWIndex();
      new_dw->allocate(rho_micro,   lb->rho_micro_CCLabel,  dwindex,patch);
      new_dw->allocate(rho_CC,      lb->rho_CCLabel,        dwindex,patch);
      new_dw->allocate(Temp_CC,     lb->temp_CCLabel,       dwindex,patch);
      new_dw->allocate(cv,          lb->cv_CCLabel,         dwindex,patch);
      new_dw->allocate(speedSound,  lb->speedSound_CCLabel, dwindex,patch);
      new_dw->allocate(visc_CC,     lb->viscosity_CCLabel,  dwindex,patch);
      new_dw->allocate(vol_frac_CC, lb->vol_frac_CCLabel,   dwindex,patch);

      new_dw->allocate(uvel_CC,     lb->uvel_CCLabel,       dwindex,patch);
      new_dw->allocate(vvel_CC,     lb->vvel_CCLabel,       dwindex,patch);
      new_dw->allocate(wvel_CC,     lb->wvel_CCLabel,       dwindex,patch);

      new_dw->allocate(uvel_FC,     lb->uvel_FCLabel,       dwindex,patch);
      new_dw->allocate(vvel_FC,     lb->vvel_FCLabel,       dwindex,patch);
      new_dw->allocate(wvel_FC,     lb->wvel_FCLabel,       dwindex,patch);

      //__________________________________
      // Initialize CC variables 
      ice_matl->initializeCells(
            rho_micro,      rho_CC,     Temp_CC,    cv,     speedSound,     
            visc_CC,        vol_frac_CC,uvel_CC,    vvel_CC,wvel_CC,
            patch,          new_dw);
    //__________________________________
    //   Initialize all FC velocites
      uvel_FC.initialize(0.);
      vvel_FC.initialize(0.);
      wvel_FC.initialize(0.);

#ifdef john_debug
      cout << "Before doing the boundary conditions" << endl;
      for(CellIterator iter = patch->getExtraCellIterator(); !iter.done(); 
	  iter++){
	cout << "rho_CC["<< *iter<< "]=" << rho_CC[*iter] << endl;
#ifdef john_debug
	cout << "rho_micro["<< *iter<< "]=" << rho_micro[*iter] << endl;
	cout << "temp["<< *iter<< "]=" << Temp_CC[*iter] << endl;
	cout << "uvel_CC["<< *iter<< "]=" << uvel_CC[*iter] << endl;
	cout << "vvel_CC["<< *iter<< "]=" << vvel_CC[*iter] << endl;
	cout << "wvel_CC["<< *iter<< "]=" << wvel_CC[*iter] << endl;
#endif
	
      } 
#endif
    //__________________________________
    //   Set BCs for CC variables
      setBC(rho_CC, "Density",      patch);
      setBC(Temp_CC,"Temperature",  patch);
      setBC(uvel_CC,"Velocity","x", patch);
      setBC(vvel_CC,"Velocity","y", patch);
      setBC(wvel_CC,"Velocity","z", patch);

#ifdef john_debug
      cout << "After doing the boundary conditions" << endl;
      for(CellIterator iter = patch->getExtraCellIterator(); !iter.done(); 
	  iter++){
	cout << "rho_CC["<< *iter<< "]=" << rho_CC[*iter] << endl;
#ifdef john_debug
	cout << "rho_micro["<< *iter<< "]=" << rho_micro[*iter] << endl;
	cout << "temp["<< *iter<< "]=" << Temp_CC[*iter] << endl;
	cout << "uvel_CC["<< *iter<< "]=" << uvel_CC[*iter] << endl;
	cout << "vvel_CC["<< *iter<< "]=" << vvel_CC[*iter] << endl;
	cout << "wvel_CC["<< *iter<< "]=" << wvel_CC[*iter] << endl;
#endif
      } 
#endif
    //__________________________________
    //  Put modified data back in new DW
      new_dw->put(rho_micro,  lb->rho_micro_CCLabel, dwindex,patch);
      new_dw->put(rho_CC,     lb->rho_CCLabel,       dwindex,patch);
      new_dw->put(vol_frac_CC,lb->vol_frac_CCLabel,  dwindex,patch);
      new_dw->put(Temp_CC,    lb->temp_CCLabel,      dwindex,patch);
      new_dw->put(cv,         lb->cv_CCLabel,        dwindex,patch);
      new_dw->put(speedSound, lb->speedSound_CCLabel,dwindex,patch);
      new_dw->put(uvel_CC,    lb->uvel_CCLabel,      dwindex,patch);
      new_dw->put(vvel_CC,    lb->vvel_CCLabel,      dwindex,patch);
      new_dw->put(wvel_CC,    lb->wvel_CCLabel,      dwindex,patch);
      new_dw->put(uvel_FC,    lb->uvel_FCLabel,      dwindex,patch);
      new_dw->put(vvel_FC,    lb->vvel_FCLabel,      dwindex,patch);
      new_dw->put(wvel_FC,    lb->wvel_FCLabel,      dwindex,patch);
      new_dw->put(visc_CC,    lb->viscosity_CCLabel, dwindex,patch);
  }
}
//STOP_DOC
/* --------------------------------------------------------------------- 
 Function~  ICE::actuallyStep1a--
 Purpose~   Compute the speed of sound for each materials   

Programmer         Date       Description                      
----------         ----       -----------                 
John Schmidt      10/24/00                              
_____________________________________________________________________*/
void ICE::actuallyStep1a(const ProcessorGroup*,
		   const Patch* patch,
		   DataWarehouseP& old_dw,
		   DataWarehouseP& new_dw)
{
  cout << "Doing actually step1a -- speed_of_sound_MM" << endl;
  int numMatls = d_sharedState->getNumICEMatls();

  //__________________________________
  //   Loop over every mat and find speed of sound
  for (int m = 0; m < numMatls; m++) 
  {
    ICEMaterial* ice_matl   = d_sharedState->getICEMaterial(m);
    EquationOfState* eos    = ice_matl->getEOS();
    eos->computeSpeedSound( patch,  ice_matl,   old_dw, new_dw);
  }
}
//STOP_DOC
/* --------------------------------------------------------------------- 
 Function~  ICE::actuallyStep1b--
 Purpose~   Find the equilibration pressure  
 Reference: Flow of Interpenetrating Material Phases, J. Comp, Phys
               18, 440-464, 1975, see the equilibration section
                   
 Steps
 ----------------
    - Compute rho_micro_CC
    - Compute volfrac_CC

    _ WHILE LOOP(convergence, max_iterations)
        - compute the pressure and dp_drho from the EOS of each material.
        - Compute delta Pressure
        - Compute delta volume fraction and update the 
          volume fraction and the celldensity.
        - Test for convergence of delta pressure and delta volume fraction
    - END WHILE LOOP
    - After the solution has converged then set the real 
      array values = local temp values
 
Note:  The nomenclature follows the reference.             
Programmer         Date       Description                      
----------         ----       -----------                 
John Schmidt      10/24/00                              
_____________________________________________________________________*/
void ICE::actuallyStep1b(
    const ProcessorGroup*,
    const Patch*    patch,
    DataWarehouseP& old_dw,
    DataWarehouseP& new_dw)
{
    double  converg_coeff = 2;              // convergence criteria =
                                            // converg_coeff * DBL_EPSILON
    int numMatls = d_sharedState->getNumICEMatls();
    vector<CCVariable<double> > vol_frac(numMatls);
  cout << "Doing actually step1b -- calc_equilibration_pressure" << endl;

   //__________________________________
   // backout rho_micro_CC at the current
   // press and Temp.
  for(int m = 0; m < numMatls; m++)
  {
    ICEMaterial* ice_matl = d_sharedState->getICEMaterial( m );
    ice_matl->getEOS()->computeRhoMicro(patch,ice_matl,old_dw,new_dw);   
  }

  //_________________________________________________________
  // The volume fraction computed here won't necessarily = 1.0.
  // In fact it shouldn't except in completely static or 1 mat problems
  for (int m = 0; m < numMatls; m++) 
  {
      ICEMaterial* ice_matl = d_sharedState->getICEMaterial(m);
      int dwindex = ice_matl->getDWIndex();
      CCVariable<double> rho_micro_temp, rho_temp;   
       
      new_dw->allocate( vol_frac[m],lb->vol_frac_CCLabel, dwindex, patch);
      old_dw->get(      rho_temp,   lb->rho_CCLabel,      dwindex, patch,
                                                          Ghost::None, 0); 
      new_dw->get( rho_micro_temp,  lb->rho_micro_CCLabel,dwindex, patch,
                                                          Ghost::None, 0);
                                                             
      for(CellIterator iter = patch->getExtraCellIterator(); !iter.done(); 
      iter++)
      {
#ifdef john_debug
	cout << "rho_CC" << (*iter+IntVector(1,1,1)) << "=" << rho_temp[*iter] << endl;
#endif
	vol_frac[m][*iter] = rho_temp[*iter]/rho_micro_temp[*iter];
      }
   }

  //__________________________________
  //  - allocate local memory
  //  - pullout variables from old dw
  //  - allocate variables to new dw   
  vector<double> delVol_frac(numMatls),press_eos(numMatls);
  vector<double> dp_drho(numMatls),dp_de(numMatls);
  
  vector<CCVariable<double> > rho_micro(numMatls),rho_micro_equil(numMatls);
  vector<CCVariable<double> > rho(numMatls);
  vector<CCVariable<double> > cv(numMatls);
  vector<CCVariable<double> > Temp(numMatls);
  vector<CCVariable<double> > speedSound(numMatls),speedSound_old(numMatls);
  CCVariable<double> press,press_new;
  
  old_dw->get(press,         lb->press_CCLabel, 0,patch,Ghost::None, 0); 
  new_dw->allocate(press_new,lb->press_CCLabel, 0,patch);

  for (int m = 0; m < numMatls; m++) 
  {
    ICEMaterial* matl = d_sharedState->getICEMaterial(m);
    int dwindex = matl->getDWIndex();
    old_dw->get(    cv[m],              lb->cv_CCLabel,     
                    dwindex,            patch,          Ghost::None, 0);
             
    old_dw->get(    rho[m],             lb->rho_CCLabel,    
                    dwindex,            patch,          Ghost::None, 0);
             
    old_dw->get(    Temp[m],            lb->temp_CCLabel,   
                    dwindex,            patch,          Ghost::None, 0); 
            
    old_dw->get(    speedSound_old[m],  lb->speedSound_CCLabel, 
                    dwindex,            patch,          Ghost::None, 0); 
            

    new_dw->allocate(   speedSound[m],  lb->speedSound_equiv_CCLabel,   
                        dwindex,        patch);
            
    new_dw->allocate(   rho_micro_equil[m],lb->rho_micro_equil_CCLabel,    
                        dwindex,            patch);
            
    new_dw->get(    rho_micro[m],       lb->rho_micro_CCLabel,
                    dwindex,            patch,          Ghost::None, 0);
  }

  press_new = press;
  //__________________________________
  //  Loop over every cell     
  int count;
  for (CellIterator iter = patch->getExtraCellIterator();!iter.done();iter++)
  {
    double delPress = 0.;
    bool converged  = false;
    count           = 0;
    while ( count < MAX_ITER_EQUILIBRATION && converged == false) 
    {
     count ++;
     double A = 0.;
     double B = 0.;
     double C = 0.;
     
     for (int m = 0; m < numMatls; m++) 
       delVol_frac[m] = 0.;
     //__________________________________
     // evaluate presss_eos at cell i,j,k
     for (int m = 0; m < numMatls; m++) 
     {
	ICEMaterial* ice_matl = d_sharedState->getICEMaterial(m);
	double gamma = ice_matl->getGamma();
       
	ice_matl->getEOS()->computePressEOS(
                    rho_micro[m][*iter],        gamma,        cv[m][*iter], 
                    Temp[m][*iter],             press_eos[m], dp_drho[m],
                    dp_de[m]);
     }

     //__________________________________
     // - compute delPress
     // - update press_CC     
     vector<double> Q(numMatls),y(numMatls);     
     for (int m = 0; m < numMatls; m++) 
     {
	 Q[m] =  press_new[*iter] - press_eos[m];
	 y[m] =  rho[m][*iter]/(vol_frac[m][*iter]*vol_frac[m][*iter]) * 
                dp_drho[m];
	 A   +=  vol_frac[m][*iter];
	 B   +=  Q[m]/y[m];
	 C   +=  1./y[m];
     }
     double vol_frac_not_close_packed = 1.;
     delPress = (A - vol_frac_not_close_packed - B)/C;
     
     press_new[*iter] += delPress;
     
     //__________________________________
     // backout rho_micro_CC at this new pressure
     for (int m = 0; m < numMatls; m++)
     {
        ICEMaterial* ice_matl = d_sharedState->getICEMaterial(m);
        double gamma = ice_matl->getGamma();
       
        rho_micro[m][*iter] = ice_matl->getEOS()->computeRhoMicro(
                    press_new[*iter],       gamma,          cv[m][*iter],
                    Temp[m][*iter]);
     }
     //__________________________________
     // - compute the updated volume fractions
     //  There are two different way of doing it
     for (int m = 0; m < numMatls; m++) 
     {
       delVol_frac[m]       = -(Q[m] + delPress)/y[m];
       vol_frac[m][*iter]   += delVol_frac[m];
     }
     //__________________________________
     // Find the speed of sound at ijk
     // needed by eos and the the explicit
     // del pressure function
     for (int m = 0; m < numMatls; m++) 
     {
        ICEMaterial* ice_matl = d_sharedState->getICEMaterial(m);
        double gamma = ice_matl->getGamma();
        ice_matl -> getEOS() -> computePressEOS(
                    rho_micro[m][*iter],        gamma,          cv[m][*iter],
                    Temp[m][*iter],             press_eos[m],   dp_drho[m],
                    dp_de[m]);
         
        double temp = dp_drho[m] + dp_de[m] * 
                    (press_eos[m]/(rho_micro[m][*iter]*rho_micro[m][*iter]));
        speedSound[m][*iter] = sqrt(temp);
     }
     //__________________________________
     // - Test for convergence 
     //  If sum of vol_frac_CC ~= 1.0 then converged 
     double sum = 0.0;
     double convergence_crit = converg_coeff * DBL_EPSILON;

     for (int m = 0; m < numMatls; m++) 
     {
        sum += vol_frac[m][*iter];
     }
     if (sum < convergence_crit)
       converged = true;
     
    }  // end of converged
  }
  // Update the boundary conditions for the variables:
  // Pressure (press_new)

#ifdef john_debug
  for (CellIterator iter=patch->getExtraCellIterator(); !iter.done(); iter++) {
    cout << "press_new["<<*iter<<"]="<<press_new[*iter] << endl;
  }
#endif

/*__________________________________
*   THIS NEEDS TO BE FIXED 
*   WE NEED TO UPDATE BC_VALUES NOT PRESSURE
*   SINCE WE USE BC_VALUES LATER ON IN THE CODE
*___________________________________*/
  setBC(press_new,"Pressure",patch);
 
  //__________________________________
  // Hydrostatic pressure adjustment - subtract off the hydrostatic pressure
  
  Vector dx             = patch->dCell();
  Vector gravity        = d_sharedState->getGravity();
  IntVector highIndex   = patch->getCellHighIndex();
  IntVector lowIndex    = patch->getCellLowIndex();
  
  double width  = (highIndex.x() - lowIndex.x())*dx.x();
  double height = (highIndex.y() - lowIndex.y())*dx.y();
  double depth  = (highIndex.z() - lowIndex.z())*dx.z();
  
  if (gravity.x() != 0.) 
  {
    //__________________________________
    // x direction
    for (CellIterator iter = patch->getExtraCellIterator(); !iter.done(); 
        iter++)
    {
      IntVector curcell = *iter;
      double press_hydro = 0.;
      for (int m = 0; m < numMatls; m++) 
      {
        press_hydro += rho[m][*iter]* gravity.x()*
          ((double) (curcell-highIndex).x()*dx.x()- width);
      }
      press_new[*iter] -= press_hydro;
    }
  }
  if (gravity.y() != 0.) 
  {
    //__________________________________
    // y direction
    for (CellIterator iter = patch->getExtraCellIterator(); !iter.done(); 
        iter++)
    {
      IntVector curcell = *iter;
      double press_hydro = 0.;
      for (int m = 0; m < numMatls; m++) 
      {
        press_hydro += rho[m][*iter]* gravity.y()*
          ( (double) (curcell-highIndex).y()*dx.y()- height);
      }
      press_new[*iter] -= press_hydro;
    }
  }
  if (gravity.z() != 0.) 
  {
    //__________________________________
    // z direction
    for (CellIterator iter = patch->getExtraCellIterator(); !iter.done(); 
        iter++)
    {
      IntVector curcell = *iter;
      double press_hydro = 0.;
      for (int m = 0; m < numMatls; m++) 
      {
        press_hydro += rho[m][*iter]* gravity.z()*
          ((double) (curcell-highIndex).z()*dx.z()- depth);
      }
      press_new[*iter] -= press_hydro;
    }
  }  
  //__________________________________
  // Put data in the new dw
  for (int m = 0; m < numMatls; m++) 
  {
    ICEMaterial* ice_matl = d_sharedState->getICEMaterial(m);
    int dwindex = ice_matl->getDWIndex();
    new_dw->put( vol_frac[m],  lb->vol_frac_CCLabel,         dwindex,patch);
    new_dw->put( speedSound[m],lb->speedSound_equiv_CCLabel, dwindex,patch);
    new_dw->put( rho_micro[m], lb->rho_micro_equil_CCLabel,  dwindex,patch);
    
#ifdef john_debug
    for(CellIterator iter = patch->getCellIterator(); !iter.done(); iter++){
      cout << "rho_micro"<<*iter<<"="<<rho_micro[m][*iter] << endl;
    }
#endif
  }
  new_dw->put(press_new,lb->press_CCLabel,0,patch);
  
}
//STOP_DOC
/* ---------------------------------------------------------------------
 Function~  ICE::actuallyStep1c--
 Purpose~   compute the face centered velocities minus the exchange
            contribution.

Programmer         Date       Description                      
----------         ----       -----------                 
John Schmidt      10/04/00                             
_____________________________________________________________________*/
void ICE::actuallyStep1c(
    const ProcessorGroup*,
    const Patch*    patch,
    DataWarehouseP& old_dw,
    DataWarehouseP& new_dw)
{
  cout << "Doing actually step1c -- compute_face_centered_velocities" << endl;

  int numMatls = d_sharedState->getNumICEMatls();

  delt_vartype delT;
  old_dw->get(delT, d_sharedState->get_delt_label());
  Vector dx      = patch->dCell();
  Vector gravity = d_sharedState->getGravity();

  // Get required variables for this patch
  CCVariable<double> rho_CC, rho_micro_CC;
  CCVariable<double> uvel_CC, vvel_CC, wvel_CC;
  CCVariable<double> press_CC;
  new_dw->get(press_CC,lb->press_CCLabel, 0, patch, Ghost::None, 0);

  // Compute the face centered velocities
  for(int m = 0; m < numMatls; m++)
  {
      ICEMaterial* ice_matl = d_sharedState->getICEMaterial( m );
      int dwindex = ice_matl->getDWIndex();

      old_dw->get(rho_CC,  lb->rho_CCLabel,   dwindex, patch, Ghost::None, 0);
      new_dw->get(rho_micro_CC, lb->rho_micro_equil_CCLabel,
				                  dwindex, patch, Ghost::None, 0);
      old_dw->get(uvel_CC, lb->uvel_CCLabel,  dwindex, patch, Ghost::None, 0);
      old_dw->get(vvel_CC, lb->vvel_CCLabel,  dwindex, patch, Ghost::None, 0);
      old_dw->get(wvel_CC, lb->wvel_CCLabel,  dwindex, patch, Ghost::None, 0);

      // Create variables for the results
      SFCXVariable<double> uvel_FC;
      SFCYVariable<double> vvel_FC;
      SFCZVariable<double> wvel_FC;
      new_dw->allocate(uvel_FC, lb->uvel_FCLabel, dwindex, patch);
      new_dw->allocate(vvel_FC, lb->vvel_FCLabel, dwindex, patch);
      new_dw->allocate(wvel_FC, lb->wvel_FCLabel, dwindex, patch);

      // Initialize velocities
      uvel_FC.initialize(0.);
      vvel_FC.initialize(0.);
      wvel_FC.initialize(0.);
      
      double term1, term2, term3, press_coeff, rho_micro_FC, rho_FC;

      for(CellIterator iter = patch->getExtraCellIterator(); !iter.done(); 
	  iter++)
    {
	IntVector curcell = *iter;
	
       //__________________________________
       //    T O P   F A C E S 
       //   Extend the computations into the left
       //   and right ghost cells 
	if (curcell.y() < (patch->getCellHighIndex()).y()-1) 
       {
	    IntVector adjcell(curcell.x(),curcell.y()+1,curcell.z()); 

	    rho_micro_FC = rho_micro_CC[adjcell] + rho_micro_CC[curcell];
	    rho_FC       = rho_CC[adjcell]       + rho_CC[curcell];
	    //__________________________________
           // interpolation to the face
	    term1 = (rho_CC[adjcell] * vvel_CC[adjcell] +
		      rho_CC[curcell] * vvel_CC[curcell])/rho_FC;            
	    //__________________________________
           // pressure term
	    press_coeff = 2.0/(rho_micro_FC);
	    term2 =   delT * press_coeff *
	             (press_CC[adjcell] - press_CC[curcell])/dx.y();                
           //__________________________________
           // gravity term
	    term3 =  delT * gravity.y();
	    vvel_FC[curcell + IntVector(0,1,0)] = term1- term2 + term3;
           
  #ifdef john_debug
  	    cout << "rho_micro_CC adjacent = " << rho_micro_CC[adjcell] << 
	      " current = " << rho_micro_CC[curcell] << endl;
	    cout << "Top face rho_micro_FC = " << rho_micro_FC << endl;
	    cout << "Top face rho_FC = " << rho_FC << endl;
	    cout << "vvel_CC adjacent = " << vvel_CC[adjcell] << " current = " 
	         << vvel_CC[curcell] << endl;
  	    cout << "Top face term 1 = " << term1 << " term 2 = " << term2 << 
	      " term 3 = " << term3 << endl;
	    cout << "uvel="<< uvel_FC[curcell+IntVector(0,1,0)] << endl;
	    cout << "vvel="<< vvel_FC[curcell+IntVector(0,1,0)] << endl;
	    cout << "wvel="<< wvel_FC[curcell+IntVector(0,1,0)] << endl<<endl;
  #endif
	}

       //__________________________________
       //  R I G H T   F A C E 
       // Extend the computations to the 
       // top and bottom ghostcells 
	if (curcell.x() < (patch->getCellHighIndex()).x()-1) 
       {
	   IntVector adjcell(curcell.x()+1,curcell.y(),curcell.z()); 

	   rho_micro_FC = rho_micro_CC[adjcell] + rho_micro_CC[curcell];
	   rho_FC       = rho_CC[adjcell]       + rho_CC[curcell];
	   //__________________________________
          // interpolation to the face
	   term1 = (rho_CC[adjcell] * uvel_CC[adjcell] +
		    rho_CC[curcell] * uvel_CC[curcell])/rho_FC;
	   //__________________________________
          // pressure term
	   press_coeff = 2.0/(rho_micro_FC);

	   term2 =   delT * press_coeff *
	     (press_CC[adjcell] - press_CC[curcell])/dx.x();
          //__________________________________
          // gravity term
	   term3 =  delT * gravity.x();


	   // I don't know what this is going to look like yet
	   // but the equations are right I think.
	   uvel_FC[curcell + IntVector(1,0,0)] = term1- term2 + term3;
	   //	 vvel_FC[curcell + IntVector(1,0,0)] = 0.0;
	   //wvel_FC[curcell + IntVector(1,0,0)] = 0.0;


  #ifdef john_debug
	   cout << "Right face term 1 = " << term1 << " term 2 = " << term2 << 
	     " term 3 = " << term3 << endl;
	    cout << "uvel="<< uvel_FC[curcell+IntVector(1,0,0)] << endl;
	    cout << "vvel="<< vvel_FC[curcell+IntVector(1,0,0)] << endl;
	    cout << "wvel="<< wvel_FC[curcell+IntVector(1,0,0)] << endl<<endl;
  #endif
	}

    //__________________________________
    //  F R O N T   F A C E
    // Extend the computations to the front
    // and back ghostcells
	if (curcell.z() < (patch->getCellHighIndex()).z()-1) 
       {
	    IntVector adjcell(curcell.x(),curcell.y(),curcell.z()+1); 
	   
	    rho_micro_FC = rho_micro_CC[adjcell] + rho_micro_CC[curcell];
	    rho_FC       = rho_CC[adjcell]       + rho_CC[curcell];
           
	   //__________________________________
          // interpolation to the face
	    term1 = (rho_CC[adjcell] * wvel_CC[adjcell] +
		     rho_CC[curcell] * wvel_CC[curcell])/rho_FC;
                   
	   //__________________________________
          // pressure term
	    press_coeff = 2.0/(rho_micro_FC);

	    term2 =   delT * press_coeff *
	      (press_CC[adjcell] - press_CC[curcell])/dx.z();
             
          //__________________________________
          // gravity term
	    term3 =  delT * gravity.z();

	    // I don't know what this is going to look like yet
	    // but the equations are right I think.
	    //uvel_FC[curcell + IntVector(0,0,1)] = 0.0;
	    //vvel_FC[curcell + IntVector(0,0,1)] = 0.0;
	    wvel_FC[curcell + IntVector(0,0,1)] = term1- term2 + term3;
           
  #ifdef john_debug
  	    cout << "Front face term 1 = " << term1 << " term 2 = " << term2 << 
	      " term 3 = " << term3 << endl;
	    cout << "uvel="<< uvel_FC[curcell+IntVector(0,0,1)] << endl;
	    cout << "vvel="<< vvel_FC[curcell+IntVector(0,0,1)] << endl;
	    cout << "wvel="<< wvel_FC[curcell+IntVector(0,0,1)] << endl<<endl;
  #endif
	}
    }

#ifdef john_debug
      cout << "Before BC application" << endl << endl;
      for(CellIterator iter = patch->getExtraCellIterator(); !iter.done(); 
	  iter++) {
	cout << "left face velocity" << *iter << "=" <<uvel_FC[*iter] << endl;
	cout << "right face velocity" << *iter << "=" 
	     <<uvel_FC[*iter+IntVector(1,0,0)] << endl;
	cout << "bottom face velocity" << *iter << "=" <<vvel_FC[*iter] << endl;
	cout << "top face velocity" << *iter << "=" 
	     <<vvel_FC[*iter + IntVector(0,1,0)] << endl;
	cout << "back face velocity" << *iter << "=" <<wvel_FC[*iter] << endl;
	cout << "front face velocity" << *iter << "=" 
	     <<wvel_FC[*iter+IntVector(0,0,1)] << endl << endl;
      }
#endif     
      //__________________________________
      // Update any neumann boundary conditions    
      setBC(uvel_CC,"Velocity","x",patch);
      setBC(vvel_CC,"Velocity","y",patch);
      setBC(wvel_CC,"Velocity","z",patch);
      setBC(uvel_FC,"Velocity","x",patch);
      setBC(vvel_FC,"Velocity","y",patch);
      setBC(wvel_FC,"Velocity","z",patch);

#ifdef john_debug
      cout << "After BC in step 1c application" << endl << endl;
      for(CellIterator iter = patch->getExtraCellIterator(); !iter.done(); 
	  iter++) {
	cout << "left face velocity" << *iter << "=" <<uvel_FC[*iter] << endl;
	cout << "right face velocity" << *iter << "=" 
	     <<uvel_FC[*iter+IntVector(1,0,0)] << endl;
	cout << "bottom face velocity" << *iter << "=" <<vvel_FC[*iter] << endl;
	cout << "top face velocity" << *iter << "=" 
	     <<vvel_FC[*iter + IntVector(0,1,0)] << endl;
	cout << "back face velocity" << *iter << "=" <<wvel_FC[*iter] << endl;
	cout << "front face velocity" << *iter << "=" 
	     <<wvel_FC[*iter+IntVector(0,0,1)] << endl << endl;
      }
#endif       
      // Put the result in the datawarehouse
      new_dw->put(uvel_FC, lb->uvel_FCLabel, dwindex, patch);
      new_dw->put(vvel_FC, lb->vvel_FCLabel, dwindex, patch);
      new_dw->put(wvel_FC, lb->wvel_FCLabel, dwindex, patch);
  }
}
//STOP_DOC
/*---------------------------------------------------------------------
 Function~  Add_exchange_contribution_to_FC_vel--
 Purpose~
   This function adds the momentum exchange contribution to the 
   existing face-centered velocities

 Prerequisites:
            The face centered velocity for each material without
            the exchange must be solved prior to this routine.
            
                   (A)                              (X)
| (1+b12 + b13)     -b12          -b23          |   |del_FC[1]  |    
|                                               |   |           |    
| -b21              (1+b21 + b23) -b32          |   |del_FC[2]  |    
|                                               |   |           | 
| -b31              -b32          (1+b31 + b32) |   |del_FC[2]  |

                        =
                        
                        (B)
| b12( uvel_FC[2] - uvel_FC[1] ) + b13 ( uvel_FC[3] -uvel_FC[1])    | 
|                                                                   |
| b21( uvel_FC[1] - uvel_FC[2] ) + b23 ( uvel_FC[3] -uvel_FC[2])    | 
|                                                                   |
| b31( uvel_FC[1] - uvel_FC[3] ) + b32 ( uvel_FC[2] -uvel_FC[3])    |           

 Programmer         Date       Description
 ----------         ----       -----------
 John Schmidt    10/4/00
 
 Steps for each face:
    1) Comute the beta coefficients
    2) Form and A matrix and B vector
    3) Solve for del_FC[*]
    4) Add del_FC[*] to the appropriate velocity
 
 References: see "A Cell-Centered ICE method for multiphase flow simulations"
 by Kashiwa, above equation 4.13.
 ---------------------------------------------------------------------  */
void ICE::actuallyStep1d(
    const ProcessorGroup*,
    const Patch*    patch,
    DataWarehouseP& old_dw,
    DataWarehouseP& new_dw)
{
  cout << "Doing actually step1d -- Add_exchange_contribution_to_FC_vel" << endl;

  int numMatls = d_sharedState->getNumICEMatls();
  int itworked;
  delt_vartype delT;
  old_dw->get(delT, d_sharedState->get_delt_label());
  Vector dx = patch->dCell();
  Vector gravity = d_sharedState->getGravity();

  double temp;

  // Create variables for the required values
  vector<CCVariable<double> > rho_micro_CC(numMatls);
  vector<CCVariable<double> > vol_frac_CC(numMatls);
  vector<SFCXVariable<double> > uvel_FC(numMatls);
  vector<SFCYVariable<double> > vvel_FC(numMatls);
  vector<SFCZVariable<double> > wvel_FC(numMatls);

  // Create variables for the results
  vector<SFCXVariable<double> > uvel_FCME(numMatls);
  vector<SFCYVariable<double> > vvel_FCME(numMatls);
  vector<SFCZVariable<double> > wvel_FCME(numMatls);

  // Extract the momentum exchange coefficients
  vector<double> b(numMatls);
  DenseMatrix beta(numMatls,numMatls),a(numMatls,numMatls),K(numMatls,numMatls);
  for (int i = 0; i < numMatls; i++ ) 
  {
      K[numMatls-1-i][i] = d_K_mom[i];
  }

//__________________________________
//   For each material get data from new dw
//   and allocate memory for FC
//   velocities after exchange
  for(int m = 0; m < numMatls; m++)
  {
    ICEMaterial* matl = d_sharedState->getICEMaterial( m );
    int dwindex = matl->getDWIndex();
    new_dw->get(rho_micro_CC[m], lb->rho_micro_equil_CCLabel,
				    dwindex, patch, Ghost::None, 0);
    new_dw->get(vol_frac_CC[m],  lb->vol_frac_CCLabel,
				    dwindex, patch, Ghost::None, 0);
    new_dw->get(uvel_FC[m], lb->uvel_FCLabel, dwindex, patch, Ghost::None, 0);
    new_dw->get(vvel_FC[m], lb->vvel_FCLabel, dwindex, patch, Ghost::None, 0);
    new_dw->get(wvel_FC[m], lb->wvel_FCLabel, dwindex, patch, Ghost::None, 0);

    new_dw->allocate(uvel_FCME[m], lb->uvel_FCMELabel, dwindex, patch);
    new_dw->allocate(vvel_FCME[m], lb->vvel_FCMELabel, dwindex, patch);
    new_dw->allocate(wvel_FCME[m], lb->wvel_FCMELabel, dwindex, patch);
  }

#ifdef john_debug
  cout << "At the beginning of step1d" << endl << endl;
  for (int m = 0; m < numMatls; m++) {
    for(CellIterator iter = patch->getExtraCellIterator(); !iter.done(); 
	iter++) {
      cout << "left face velocity" << *iter << "=" <<uvel_FC[m][*iter] << endl;
      cout << "right face velocity" << *iter << "=" 
	   <<uvel_FC[m][*iter+IntVector(1,0,0)] << endl;
      cout << "bottom face velocity" << *iter << "=" <<vvel_FC[m][*iter] << endl;
      cout << "top face velocity" << *iter << "=" 
	   <<vvel_FC[m][*iter + IntVector(0,1,0)] << endl;
      cout << "back face velocity" << *iter << "=" <<wvel_FC[m][*iter] << endl;
      cout << "front face velocity" << *iter << "=" 
	   <<wvel_FC[m][*iter+IntVector(0,0,1)] << endl << endl;
    }
  }
#endif       
     
  for (int m = 0; m < numMatls; m++) 
  {
    uvel_FCME[m] = uvel_FC[m];
    vvel_FCME[m] = vvel_FC[m];
    wvel_FCME[m] = wvel_FC[m];
  }
  
  for(CellIterator iter = patch->getExtraCellIterator(); !iter.done();
      iter++){
    IntVector curcell = *iter;
    //    cout << "Working on cell " << curcell << endl;

    //__________________________________
    //  T  O  P -- B  E  T  A      
    //  Note this includes b[m][m]
    //  You need to make sure that mom_exch_coeff[m][m] = 0
    //   - form off diagonal terms of (a) 
    if (curcell.y() < (patch->getCellHighIndex()).y()-1) 
    {
        IntVector adjcell(curcell.x(),curcell.y()+1,curcell.z()); 

        for(int m = 0; m < numMatls; m++)
        {
	    for(int n = 0; n < numMatls; n++)
           {
	      temp = (vol_frac_CC[n][adjcell] + vol_frac_CC[n][curcell]) * 
                    K[n][m];

             beta[m][n] = delT * temp/
	        (rho_micro_CC[m][curcell] + rho_micro_CC[m][adjcell]);

	      a[m][n] = -beta[m][n];
	    }
        }
        //__________________________________
        //  F  O  R  M     M  A  T  R  I  X   (a)
        //  - Diagonal terms      
        for(int m = 0; m < numMatls; m++)
        {
	    a[m][m] = 1.;
	    for(int n = 0; n < numMatls; n++)
           {
	        a[m][m] +=  beta[m][n];
	    }
        }
        //__________________________________
        //    F  O  R  M     R  H  S  (b)     
        for(int m = 0; m < numMatls; m++)
        {
	    b[m] = 0.0;
	    for(int n = 0; n < numMatls; n++)
           {
	        b[m] += beta[m][n] * (vvel_FC[n][*iter] - vvel_FC[m][*iter]);
	    }
        }
        //__________________________________
        //      S  O  L  V  E  
        //   - backout velocities      
        itworked = a.solve(b);
        for(int m = 0; m < numMatls; m++)
        {
	   vvel_FCME[m][*iter] = vvel_FC[m][*iter] + b[m];
        }
    }
    //__________________________________
    //  R I G H T -- B  E  T  A      
    //  Note this includes b[m][m]
    //  You need to make sure that mom_exch_coeff[m][m] = 0
    //   - form off diagonal terms of (a)
    if (curcell.x() < (patch->getCellHighIndex()).x()-1) 
    {
        IntVector adjcell(curcell.x()+1,curcell.y(),curcell.z()); 
        for(int m = 0; m < numMatls; m++)
        {
	    for(int n = 0; n < numMatls; n++)
           {
	        temp = (vol_frac_CC[n][adjcell] + vol_frac_CC[n][curcell]) * 
                      K[n][m];

	        beta[m][n] = delT * temp/
	          (rho_micro_CC[m][curcell] + rho_micro_CC[m][adjcell]);

	        a[m][n] = -beta[m][n];
	    }
        }
        /*__________________________________
        *  F  O  R  M     M  A  T  R  I  X   (a)
        * - Diagonal terms
        *___________________________________*/
        for(int m = 0; m < numMatls; m++)
        {
	    a[m][m] = 1.;
	    for(int n = 0; n < numMatls; n++)
           {
	      a[m][m] +=  beta[m][n];
	    }
        }
        //__________________________________
        //    F  O  R  M     R  H  S  (b) 
        for(int m = 0; m < numMatls; m++)
        {
	    b[m] = 0.0;
	    for(int n = 0; n < numMatls; n++)
           {
	        b[m] += beta[m][n] * (uvel_FC[n][*iter] - uvel_FC[m][*iter]);
	    }
        }
        //__________________________________
        //      S  O  L  V  E
        //   - backout velocities
        itworked = a.solve(b);
        for(int m = 0; m < numMatls; m++)
        {
	  uvel_FCME[m][*iter] = uvel_FC[m][*iter] + b[m];
  #ifdef john_debug
	  cout << "uvel_FC = " << uvel_FC[m][*iter] << " b = " << b[m] <<
	    "uvel_FCME = " << uvel_FCME[m][*iter] << endl;
  #endif
        }
    }
    //__________________________________
    //  F R O N T -- B  E  T  A      
    //  Note this includes b[m][m]
    //  You need to make sure that mom_exch_coeff[m][m] = 0
    //   - form off diagonal terms of (a)
    if (curcell.z() < (patch->getCellHighIndex()).z()-1) 
    {
        IntVector adjcell(curcell.x(),curcell.y(),curcell.z()+1); 
        for(int m = 0; m < numMatls; m++)
        {
	    for(int n = 0; n < numMatls; n++)
           {
	      temp = (vol_frac_CC[n][adjcell] + vol_frac_CC[n][curcell]) *
                   K[n][m];

	      beta[m][n] = delT * temp/
	        (rho_micro_CC[m][curcell] + rho_micro_CC[m][adjcell]);

	      a[m][n] = -beta[m][n];
	    }
        }
        //__________________________________
        //  F  O  R  M     M  A  T  R  I  X   (a)
        // - Diagonal terms
        for(int m = 0; m < numMatls; m++)
        {
	    a[m][m] = 1.;
	    for(int n = 0; n < numMatls; n++)
           {
	      a[m][m] +=  beta[m][n];
	    }
        }
        //__________________________________
        //    F  O  R  M     R  H  S  (b)
        for(int m = 0; m < numMatls; m++)
        {
	    b[m] = 0.0;
	    for(int n = 0; n < numMatls; n++)
           {
	      b[m] += beta[m][n] * (wvel_FC[n][*iter] - wvel_FC[m][*iter]);
	    }
        }
        //__________________________________
        //      S  O  L  V  E
        //   - backout velocities
         itworked = a.solve(b);
  #ifdef john_debug
        for (int i = 0; i < (int)b.size(); i++) 
        {
	  cout << "Front faced b[" << i << "]=" << b[i] << endl;
        }      
  #endif

        for(int m = 0; m < numMatls; m++)
        {
	    wvel_FCME[m][*iter] = wvel_FC[m][*iter] + b[m];
        }
    }
  }


  // Apply grid boundary conditions to the velocity
  // before storing the data
#ifdef john_debug
  cout << "Before the BCs in step1d" << endl << endl;
  for (int m = 0; m < numMatls; m++) {
    for (CellIterator iter=patch->getExtraCellIterator(); !iter.done();
	 iter++) {
      cout << "left face velocity" << *iter << "=" <<uvel_FCME[m][*iter] << endl;
      cout << "right face velocity" << *iter << "=" 
	   <<uvel_FCME[m][*iter+IntVector(1,0,0)] << endl;
      cout << "bottom face velocity" << *iter << "=" <<vvel_FCME[m][*iter] << endl;
      cout << "top face velocity" << *iter << "=" 
	   <<vvel_FCME[m][*iter + IntVector(0,1,0)] << endl;
      cout << "back face velocity" << *iter << "=" <<wvel_FCME[m][*iter] << endl;
      cout << "front face velocity" << *iter << "=" 
	   <<wvel_FCME[m][*iter+IntVector(0,0,1)] << endl << endl;
    }
  }
#endif

  for (int m = 0; m < numMatls; m++) 
  {
    setBC(uvel_FCME[m],"Velocity","x",patch);
    setBC(vvel_FCME[m],"Velocity","y",patch);
    setBC(wvel_FCME[m],"Velocity","z",patch);
#ifdef john_debug
    cout << endl << endl << "Now doing the BC in step1d" << endl << endl;
    for (CellIterator iter=patch->getExtraCellIterator(); !iter.done();
	 iter++) {
      cout << "left face velocity" << *iter << "=" <<uvel_FCME[m][*iter] << endl;
      cout << "right face velocity" << *iter << "=" 
	   <<uvel_FCME[m][*iter+IntVector(1,0,0)] << endl;
      cout << "bottom face velocity" << *iter << "=" <<vvel_FCME[m][*iter] << endl;
      cout << "top face velocity" << *iter << "=" 
	   <<vvel_FCME[m][*iter + IntVector(0,1,0)] << endl;
      cout << "back face velocity" << *iter << "=" <<wvel_FCME[m][*iter] << endl;
      cout << "front face velocity" << *iter << "=" 
	   <<wvel_FCME[m][*iter+IntVector(0,0,1)] << endl << endl;
    }
#endif
  }
  // Put the result in the datawarehouse
  for(int m = 0; m < numMatls; m++)
  {
    ICEMaterial* matl = d_sharedState->getICEMaterial( m );
    int dwindex = matl->getDWIndex();
    new_dw->put(uvel_FCME[m], lb->uvel_FCMELabel, dwindex, patch);
    new_dw->put(vvel_FCME[m], lb->vvel_FCMELabel, dwindex, patch);
    new_dw->put(wvel_FCME[m], lb->wvel_FCMELabel, dwindex, patch);
  }
  
  //__________________________________
  //    QUITE wall and Fullwarn warnings
    itworked = itworked;
}
//STOP_DOC
/*---------------------------------------------------------------------
 Function~  explicit_delPress_MM--
 Purpose~
   This function calculates the change in pressure explicitly. 
 Note:  Units of delpress are [Pa]
 
 Programmer         Date       Description
 ----------         ----       -----------
 Jim Guilkey       10/4/00
       
 Reference:  Multimaterial Formalism eq. 1.5
 ---------------------------------------------------------------------  */
void ICE::actuallyStep2(
    const ProcessorGroup*,
    const Patch*    patch,
    DataWarehouseP& old_dw,
    DataWarehouseP& new_dw)
{
  cout << "Doing actually step2 -- divergence_of_face_centered_velocity_MM" << endl;
  int numMatls  = d_sharedState->getNumICEMatls();
  delt_vartype delT;
  old_dw->get(delT, d_sharedState->get_delt_label());
  Vector dx     = patch->dCell();

  double vol    = dx.x()*dx.y()*dx.z();
  double invvol = 1./vol;

//__________________________________
//   Allocate the temporary variables needed for advection
//   These arrays get re-used for each material
  CCVariable<double> q_CC,      q_advected;
  CCVariable<fflux> IFS,        OFS,                    
                    q_out,      q_in;
  CCVariable<eflux> IFE,        OFE,                    
                    q_out_EF,   q_in_EF;
                    
  CCVariable<double> pressure;
  CCVariable<double> delPress;
  CCVariable<double> pressdP;
  new_dw->get(pressure,        lb->press_CCLabel,    0, patch,Ghost::None, 0);
  new_dw->allocate(delPress,   lb->delPress_CCLabel, 0, patch);
  new_dw->allocate(pressdP,    lb->pressdP_CCLabel,  0, patch);

  new_dw->allocate(q_CC,       lb->q_CCLabel,       0, patch);
  new_dw->allocate(q_advected, lb->q_advectedLabel, 0, patch);
  new_dw->allocate(IFS,        IFS_CCLabel,         0, patch);  
  new_dw->allocate(OFS,        OFS_CCLabel,         0, patch);  
  new_dw->allocate(IFE,        IFE_CCLabel,         0, patch);  
  new_dw->allocate(OFE,        OFE_CCLabel,         0, patch);  
  new_dw->allocate(q_out,      q_outLabel,          0, patch);
  new_dw->allocate(q_out_EF,   q_out_EFLabel,       0, patch);  
  new_dw->allocate(q_in,       q_inLabel,           0, patch);
  new_dw->allocate(q_in_EF,    q_in_EFLabel,        0, patch);

  CCVariable<double> term1, term2, term3;
  new_dw->allocate(term1, lb->term3Label, 0, patch);
  new_dw->allocate(term2, lb->term3Label, 0, patch);
  new_dw->allocate(term3, lb->term3Label, 0, patch);

  term1.initialize(0.);
  term2.initialize(0.);
  term3.initialize(0.);
  
  //__________________________________
  // Loop over each mat. and advect vol_frac
  for(int m = 0; m < numMatls; m++)
  {
    ICEMaterial* matl = d_sharedState->getICEMaterial( m );
      int dwindex = matl->getDWIndex();
      // Get required variables for this patch
      SFCXVariable<double> uvel_FC;
      SFCYVariable<double> vvel_FC;
      SFCZVariable<double> wvel_FC;
      CCVariable<double>  vol_frac;
      CCVariable<double>  rho_micro_CC;
      CCVariable<double>  speedSound;
      new_dw->get(uvel_FC, lb->uvel_FCMELabel, dwindex,  patch,Ghost::None, 0);
      new_dw->get(vvel_FC, lb->vvel_FCMELabel, dwindex,  patch,Ghost::None, 0);
      new_dw->get(wvel_FC, lb->wvel_FCMELabel, dwindex,  patch,Ghost::None, 0);
      new_dw->get(vol_frac,lb->vol_frac_CCLabel,dwindex, patch,Ghost::None, 0);
      new_dw->get(rho_micro_CC, lb->rho_micro_equil_CCLabel,
						     dwindex,  patch,Ghost::None, 0);
      new_dw->get(speedSound,lb->speedSound_equiv_CCLabel,
					            dwindex,  patch,Ghost::None, 0);


      //__________________________________
      // Advection preprocessing
      // - divide vol_frac_cc/vol
      influxOutfluxVolume(  uvel_FC,    vvel_FC,    wvel_FC,
                            delT,       patch,
                            OFS,        OFE,        IFS,        IFE);

        for(CellIterator iter = patch->getExtraCellIterator(); !iter.done();
	    iter++)
        {
            q_CC[*iter] = vol_frac[*iter] * invvol;
#ifdef john_debug
	     cout << "q_CC"<<*iter<<"="<< q_CC[*iter] << endl;
#endif
        }
        //__________________________________
        //   First order advection of q_CC
        advectQFirst(       q_CC,       patch,
                            OFS,        OFE,        IFS,        IFE,
                            q_out,      q_out_EF,   q_in,   q_in_EF,
                            q_advected);

      for(CellIterator iter = patch->getCellIterator(); !iter.done(); 
	  iter++)
      {
          //__________________________________
          //   Contributions from reactions
          //   to be filled in Be very careful with units
	   term1[*iter] = 0.;
          
          //__________________________________
          //   Divergence of velocity * face area
          //   Be very careful with the units
          //   do the volume integral to check them
          //   See journal pg 171
          //   You need to divide by the cell volume
          //
          //  Note that sum(div (theta_k U^f_k) 
          //          =
          //  Advection(theta_k, U^f_k)
          //  This subtle point is discussed on pg
          //  190 of my Journal
	   term2[*iter] -= q_advected[*iter];
          
	   term3[*iter] += vol_frac[*iter] /(rho_micro_CC[*iter] *
			     speedSound[*iter]*speedSound[*iter]);
#ifdef john_debug                          
    cout << "term1 = " << term1[*iter] << " term2 = " << term2[*iter] << " term3 = " << term3[*iter] << endl;
#endif
      }
  }
  for(CellIterator iter = patch->getCellIterator(); !iter.done(); iter++)
  {
     delPress[*iter] = (delT * term1[*iter] - term2[*iter])/(term3[*iter]);
     pressdP[*iter]  = pressure[*iter] + delPress[*iter];
    
#ifdef john_debug
    cout << "delPress = " << delPress[*iter] << " pressdP = " 
	 << pressdP[*iter] << endl;
#endif
  }
  //__________________________________
  // Update the boundary conditions 
  // - put data in new dw 
  setBC(pressdP,"Pressure",patch);

  new_dw->put(delPress, lb->delPress_CCLabel, 0, patch);
  new_dw->put(pressdP,  lb->pressdP_CCLabel,  0, patch);
}
//STOP_DOC
/* ---------------------------------------------------------------------  
 Function~  ICE::actuallyStep3--
 Purpose~
    This function calculates the face centered pressure on each of the 
    cell faces for every cell in the computational domain and a single 
    layer of ghost cells.  This routine assume that there is a 
    single layer of ghostcells

Programmer         Date       Description
----------         ----       -----------
Jim Guilkey         10/04/00
Tod                 12/28/00    Changed the way press_FC is computed
  ---------------------------------------------------------------------  */
void ICE::actuallyStep3(
    const ProcessorGroup*,
    const Patch*    patch,
    DataWarehouseP& old_dw,
    DataWarehouseP& new_dw)
{
  cout << "Doing actually step3 -- press_face_MM" << endl;
  int numMatls = d_sharedState->getNumICEMatls();
  double sum_rho, sum_rho_adj, sum_all_rho;
  double A;                                      // Temp variable

  // Get required variables for this patch
  vector<CCVariable<double> > rho_CC(numMatls);
  CCVariable<double> press_CC;
  new_dw->get(press_CC,lb->pressdP_CCLabel, 0, patch, Ghost::None, 0);
  
  // Create variables for the results
  SFCXVariable<double> pressX_FC;
  SFCYVariable<double> pressY_FC;
  SFCZVariable<double> pressZ_FC;
  new_dw->allocate(pressX_FC,lb->pressX_FCLabel, 0, patch);
  new_dw->allocate(pressY_FC,lb->pressY_FCLabel, 0, patch);
  new_dw->allocate(pressZ_FC,lb->pressZ_FCLabel, 0, patch);

  // Compute the face centered velocities
  for(int m = 0; m < numMatls; m++)
  {
    ICEMaterial* matl = d_sharedState->getICEMaterial( m );
    int dwindex = matl->getDWIndex();
    old_dw->get(rho_CC[m], lb->rho_CCLabel, dwindex, patch, Ghost::None, 0);
  }

  for(CellIterator iter = patch->getExtraCellIterator();!iter.done();iter++){
    IntVector curcell = *iter;
#ifdef john_debug
    cout << "Cell = " << curcell << endl;
#endif
    //__________________________________
    //  T O P   F A C E
    if (curcell.y() < (patch->getCellHighIndex()).y()-1) 
    {
        IntVector adjcell(curcell.x(),curcell.y()+1,curcell.z());
        sum_rho         =0.0;
        sum_rho_adj     = 0.0;
        for(int m = 0; m < numMatls; m++)
        {
	    sum_rho      += (rho_CC[m][curcell] + d_SMALL_NUM);
	    sum_rho_adj  += (rho_CC[m][adjcell] + d_SMALL_NUM);
    #ifdef john_debug
	    cout << "rho_CC"<<curcell<<"=" << rho_CC[m][curcell] << " sum_rho=" << sum_rho << " sum_rho_adj=" << sum_rho_adj << endl;
    #endif
        }
        sum_all_rho  = sum_rho     +  sum_rho_adj;
  #ifdef john_debug
        cout << "sum_all_rho"<<curcell<<"=" << sum_all_rho << endl;
  #endif 
  
        A =  (press_CC[curcell]/sum_rho) + (press_CC[adjcell]/sum_rho_adj);
        pressY_FC[curcell+IntVector(0,1,0)]      =  A/((1/sum_rho) + (1.0/sum_rho_adj) );
    }
    //__________________________________
    //  R I G H T   F A C E
    if (curcell.x() < (patch->getCellHighIndex()).x()-1) 
    {
        IntVector adjcell(curcell.x()+1,curcell.y(),curcell.z());

        sum_rho=0.0;
        sum_rho_adj  = 0.0;
        /* THIS NEEDS TO BE CHANGED*/
        for(int m = 0; m < numMatls; m++)
        {
	    sum_rho      += (rho_CC[m][curcell] + d_SMALL_NUM);
	    sum_rho_adj  += (rho_CC[m][adjcell] + d_SMALL_NUM);
        }
        sum_all_rho  = sum_rho     +  sum_rho_adj;
        A =  (press_CC[curcell]/sum_rho) + (press_CC[adjcell]/sum_rho_adj);
        pressX_FC[curcell+IntVector(1,0,0)]      =  A/((1/sum_rho) + (1.0/sum_rho_adj) );
    }
    //__________________________________
    //     F R O N T   F A C E 
    if (curcell.z() < (patch->getCellHighIndex()).z()-1) 
    {
      IntVector adjcell(curcell.x(),curcell.y(),curcell.z()+1);
      
      sum_rho=0.0;
      sum_rho_adj  = 0.0;
      for(int m = 0; m < numMatls; m++)
      {
	sum_rho      += (rho_CC[m][curcell] + d_SMALL_NUM);
	sum_rho_adj  += (rho_CC[m][adjcell] + d_SMALL_NUM);
      }
      sum_all_rho  = sum_rho     +  sum_rho_adj;
#if 0
    /* 3D */
        A =  (press_CC[curcell]/sum_rho) + (press_CC[adjcell]/sum_rho_adj);
        pressZ_FC[curcell+IntVector(0,0,1)]      =  A/((1/sum_rho) + (1.0/sum_rho_adj) );
#endif
      pressZ_FC[curcell+IntVector(0,0,1)] = 0.;
    }
  }

#ifdef john_debug
  cout << "Before application of pressure BCS" << endl;
  for(CellIterator iter = patch->getExtraCellIterator(); !iter.done(); iter++){
    cout << "Cell = " << *iter << endl;
    cout << "top face pressure = " << pressY_FC[*iter+IntVector(0,1,0)] 
	     << endl;
    cout << "bottom face pressure = " << pressY_FC[*iter+IntVector(0,0,0)] 
	 << endl;
    cout << "left face pressure = " << pressX_FC[*iter+IntVector(0,0,0)] 
	 << endl;
    cout << "right face pressure = " << pressX_FC[*iter+IntVector(1,0,0)] 
	 << endl;
    cout << "front face pressure = " << pressZ_FC[*iter+IntVector(0,0,1)] 
	 << endl;
    cout << "back face pressure = " << pressZ_FC[*iter+IntVector(0,0,0)] 
	 << endl<<endl;
  }
#endif
    //__________________________________
    // Update the boundary conditions
    setBC(pressX_FC,"Pressure",patch);
    setBC(pressY_FC,"Pressure",patch);
    setBC(pressZ_FC,"Pressure",patch);

#ifdef john_debug
  cout << "After application of pressure BCS" << endl;
  for(CellIterator iter = patch->getExtraCellIterator(); !iter.done(); iter++){
    cout << "Cell = " << *iter << endl;
    cout << "top face pressure = " << pressY_FC[*iter+IntVector(0,1,0)] 
	     << endl;
    cout << "bottom face pressure = " << pressY_FC[*iter+IntVector(0,0,0)] 
	 << endl;
    cout << "left face pressure = " << pressX_FC[*iter+IntVector(0,0,0)] 
	 << endl;
    cout << "right face pressure = " << pressX_FC[*iter+IntVector(1,0,0)] 
	 << endl;
    cout << "front face pressure = " << pressZ_FC[*iter+IntVector(0,0,1)] 
	 << endl;
    cout << "back face pressure = " << pressZ_FC[*iter+IntVector(0,0,0)] 
	 << endl << endl;
  }
#endif
  //__________________________________
  //   Put press_FC in new dw
  new_dw->put(pressX_FC,lb->pressX_FCLabel, 0, patch);
  new_dw->put(pressY_FC,lb->pressY_FCLabel, 0, patch);
  new_dw->put(pressZ_FC,lb->pressZ_FCLabel, 0, patch);
}
//STOP_DOC
/* ---------------------------------------------------------------------
 Function~  accumulate_momentum_source_sinks--
 Purpose~   This function accumulates all of the sources/sinks of momentum
            which is added to the current value for the momentum to form
            the Lagrangian momentum
   
 Programmer         Date       Description                      
 ----------         ----       -----------                 
 Jim Guilkey       10/04/00                           
 ---------------------------------------------------------------------  */
void ICE::actuallyStep4a(
    const ProcessorGroup*,
    const Patch*    patch,
    DataWarehouseP& old_dw,
    DataWarehouseP& new_dw)
{
  cout << "Doing actually step4a -- accumulate_momentum_source_sinks_MM" << endl;

  int       numMatls;
  IntVector right, left, top, bottom, front, back;
  delt_vartype delT;
  
  Vector    dx, gravity;
  double    delX, delY, delZ;
  double    pressure_source, viscous_source, mass, vol;
  
  old_dw->get(delT, d_sharedState->get_delt_label());
  dx        = patch->dCell();
  gravity   = d_sharedState->getGravity();
  delX      = dx.x();
  delY      = dx.y();
  delZ      = dx.z();
  vol       = delX * delY * delZ;
  numMatls  = d_sharedState->getNumICEMatls();
  
  CCVariable<double>   rho_CC;
  CCVariable<double>   uvel_CC, vvel_CC, wvel_CC;
  CCVariable<double>   visc_CC;
  CCVariable<double>   vol_frac;
  SFCXVariable<double> pressX_FC;
  SFCYVariable<double> pressY_FC;
  SFCZVariable<double> pressZ_FC;

  CCVariable<double>   xmom_source, ymom_source, zmom_source;
  SFCXVariable<double> tau_X_FC;
  SFCYVariable<double> tau_Y_FC;
  SFCZVariable<double> tau_Z_FC;
  //__________________________________
  //   Get data from the data warehouse
  new_dw->get(pressX_FC,lb->pressX_FCLabel, 0, patch,Ghost::None, 0);
  new_dw->get(pressY_FC,lb->pressY_FCLabel, 0, patch,Ghost::None, 0);
  new_dw->get(pressZ_FC,lb->pressZ_FCLabel, 0, patch,Ghost::None, 0);

  for(int m = 0; m < numMatls; m++)
  {
    ICEMaterial* matl = d_sharedState->getICEMaterial( m );
    int dwindex = matl->getDWIndex();
    // Get required variables for this patch
    old_dw->get(rho_CC,  lb->rho_CCLabel,      dwindex,patch,Ghost::None, 0);  
    old_dw->get(uvel_CC, lb->uvel_CCLabel,     dwindex,patch,Ghost::None, 0);
    old_dw->get(vvel_CC, lb->vvel_CCLabel,     dwindex,patch,Ghost::None, 0);
    old_dw->get(wvel_CC, lb->wvel_CCLabel,     dwindex,patch,Ghost::None, 0);
    old_dw->get(visc_CC, lb->viscosity_CCLabel,dwindex,patch,Ghost::None, 0);
    new_dw->get(vol_frac,lb->vol_frac_CCLabel, dwindex,patch,Ghost::None, 0);

    // Create variables for the results
    new_dw->allocate(xmom_source, lb->xmom_source_CCLabel, dwindex, patch);
    new_dw->allocate(ymom_source, lb->ymom_source_CCLabel, dwindex, patch);
    new_dw->allocate(zmom_source, lb->zmom_source_CCLabel, dwindex, patch);
    new_dw->allocate(tau_X_FC,    lb->tau_X_FCLabel,       dwindex, patch);
    new_dw->allocate(tau_Y_FC,    lb->tau_Y_FCLabel,       dwindex, patch);
    new_dw->allocate(tau_Z_FC,    lb->tau_Z_FCLabel,       dwindex, patch);
 
    xmom_source.initialize(0.);
    ymom_source.initialize(0.);
    zmom_source.initialize(0.);

    for(CellIterator iter = patch->getCellIterator(); !iter.done(); iter++)
    {
       mass = rho_CC[*iter] * vol;
       right    = *iter + IntVector(1,0,0);
       left     = *iter + IntVector(0,0,0);
       top      = *iter + IntVector(1,0,0);
       bottom   = *iter + IntVector(0,0,0);
       front    = *iter + IntVector(1,0,0);
       back     = *iter + IntVector(0,0,0);
      //__________________________________
      //    X - M O M E N T U M 
       pressure_source = (pressX_FC[right] - pressX_FC[left]) * vol_frac[*iter];
       
#if 0
      // tau variables are really vector quantities and need to be
      // stored as SFCXVariable<Vector>.  But for now they are not
      // being used.
      viscous_source  = tau_X_FC[*iter+IntVector(1,0,0)] - 
	tau_X_FC[*iter+IntVector(0,0,0)] + 
	tau_X_FC[*iter+IntVector(0,1,0)]  - 
	tau_X_FC[*iter+IntVector(0,0,0)] + 
	tau_X_FC[*iter+IntVector(0,0,1)] - 
	tau_X_FC[*iter+IntVector(0,0,0)];
#endif
      xmom_source[*iter]  =   (-pressure_source * delY * delZ +
			       mass * gravity.x()) * delT;
      //__________________________________
      //    Y - M O M E N T U M
       pressure_source = (pressY_FC[top] - pressY_FC[bottom]) * vol_frac[*iter];
#if 0
      // tau variables are really vector quantities and need to be
      // stored as SFCXVariable<Vector>.  But for now they are not
      // being used.
      viscous_source  = tau_X_FC[*iter+IntVector(1,0,0)] - 
	tau_X_FC[*iter+IntVector(0,0,0)] + 
	tau_X_FC[*iter+IntVector(0,1,0)]  - 
	tau_X_FC[*iter+IntVector(0,0,0)] + 
	tau_X_FC[*iter+IntVector(0,0,1)] - 
	tau_X_FC[*iter+IntVector(0,0,0)];
#endif
      ymom_source[*iter]  =   (-pressure_source * delX * delZ +
			       mass * gravity.y()) * delT;
      //__________________________________
      //    Z - M O M E N T U M
      pressure_source = (pressZ_FC[front] - pressZ_FC[back]) * vol_frac[*iter];
#if 0
      // tau variables are really vector quantities and need to be
      // stored as SFCXVariable<Vector>.  But for now they are not
      // being used.
      viscous_source  = tau_X_FC[*iter+IntVector(1,0,0)] - 
	tau_X_FC[*iter+IntVector(0,0,0)] + 
	tau_X_FC[*iter+IntVector(0,1,0)]  - 
	tau_X_FC[*iter+IntVector(0,0,0)] + 
	tau_X_FC[*iter+IntVector(0,0,1)] - 
	tau_X_FC[*iter+IntVector(0,0,0)];
#endif
      zmom_source[*iter]  =   (-pressure_source * delX * delY +
			       mass * gravity.z()) * delT;
#ifdef john_debug
      cout << "xmom_source"<<*iter <<"="<<xmom_source[*iter] << " ymom_source=" << ymom_source[*iter] << " zmom_source="<< zmom_source[*iter] << endl;
#endif
    }
    //__________________________________
    //   Put momentum sources in new dw
    new_dw->put(xmom_source, lb->xmom_source_CCLabel, dwindex, patch);
    new_dw->put(ymom_source, lb->ymom_source_CCLabel, dwindex, patch);
    new_dw->put(zmom_source, lb->zmom_source_CCLabel, dwindex, patch);
  }
}
//STOP_DOC
/* --------------------------------------------------------------------- 
 Function~  ICE::actuallyStep4b--
 Purpose~   This function accumulates all of the sources/sinks of energy
            which is added to the current value for the energy to form
            the Lagrangian energy  
 Programmer         Date       Description                      
 ----------         ----       -----------                 
  Jim Guilkey       10/04/00

 Currently the kinetic energy isn't included.
 This is the routine where you would add additional sources/sinks of energy                 
 ---------------------------------------------------------------------  */
void ICE::actuallyStep4b(
    const ProcessorGroup*,
    const Patch*    patch,
    DataWarehouseP& old_dw,
    DataWarehouseP& new_dw)
{
  cout << "Doing actually step4b -- accumulate_energy_source_sinks" << endl;

  int numMatls = d_sharedState->getNumICEMatls();
  delt_vartype delT;
  old_dw->get(delT, d_sharedState->get_delt_label());
  Vector dx = patch->dCell();
  double A, B, vol=dx.x()*dx.y()*dx.z();
//__________________________________
//   - Allocate local arrays
//   - get data from dw
  CCVariable<double> rho_micro_CC;
  CCVariable<double> speedSound;
  CCVariable<double> vol_frac;
  CCVariable<double> press_CC;
  CCVariable<double> delPress;
  CCVariable<double> int_eng_source;

  new_dw->get(press_CC,lb->press_CCLabel,    0, patch,Ghost::None, 0);
  new_dw->get(delPress,lb->delPress_CCLabel, 0, patch,Ghost::None, 0);

  for(int m = 0; m < numMatls; m++)
 {
      //__________________________________
      //   - get data from new dw
      //   - allocate memory 
      ICEMaterial* matl = d_sharedState->getICEMaterial( m );
      int dwindex       = matl->getDWIndex();
      
      new_dw->get(  rho_micro_CC,   lb->rho_micro_equil_CCLabel,
		      dwindex,        patch,      Ghost::None, 0);
      
      new_dw->get(speedSound,       lb->speedSound_equiv_CCLabel,
		      dwindex,        patch,      Ghost::None, 0);
      
      new_dw->get(vol_frac,         lb->vol_frac_CCLabel,
                    dwindex,        patch,      Ghost::None, 0);

      new_dw->allocate(int_eng_source,          lb->int_eng_source_CCLabel,
                    dwindex,        patch);
      //__________________________________
      //   Compute int_eng_source in all cells
      for(CellIterator iter = patch->getCellIterator(); !iter.done(); iter++)
     {
	   A = vol * vol_frac[*iter] * press_CC[*iter];
          B = rho_micro_CC[*iter]   * speedSound[*iter] * speedSound[*iter];
          int_eng_source[*iter] = (A/B) * delPress[*iter];
  #ifdef john_debug
         cout << "A = " << A << " B = " << B << endl;
	  cout << "int_eng_source"<<*iter<<"="<<int_eng_source[*iter] << endl;
  #endif
      }
      //__________________________________
      //  - put int_eng_source in new dw
      new_dw->put(int_eng_source,lb->int_eng_source_CCLabel,dwindex,patch);
  }
}
//STOP_DOC
/* ---------------------------------------------------------------------
 Function~  ICE::actuallyStep5a--
 Purpose~
   This function calculates the The cell-centered, time n+1, 
   lagrangian mass momentum and energy
   
Programmer         Date       Description
----------         ----       -----------
 Jim Guilkey       10/04/00    
 ---------------------------------------------------------------------  */
void ICE::actuallyStep5a(
    const ProcessorGroup*,
    const Patch*    patch,
    DataWarehouseP& old_dw,
    DataWarehouseP& new_dw)
{
  cout << "Doing actually step5a -- calculate Lagrangian values for mass, momentum and energy" << endl;

  int numMatls = d_sharedState->getNumICEMatls();
  Vector    dx = patch->dCell();

  // Compute the Lagrangian quantities
  for(int m = 0; m < numMatls; m++)
  {
      ICEMaterial* matl = d_sharedState->getICEMaterial( m );
      int dwindex = matl->getDWIndex();
      //__________________________________
      // Get arrays from dw
      CCVariable<double> rho_CC;
      CCVariable<double> uvel_CC;
      CCVariable<double> vvel_CC;
      CCVariable<double> wvel_CC;
      CCVariable<double> cv_CC;
      CCVariable<double> temp_CC;
      CCVariable<double> xmom_source;
      CCVariable<double> ymom_source;
      CCVariable<double> zmom_source;
      CCVariable<double> int_eng_source;
      old_dw->get(rho_CC,  lb->rho_CCLabel,     dwindex,patch,Ghost::None, 0);
      old_dw->get(uvel_CC, lb->uvel_CCLabel,    dwindex,patch,Ghost::None, 0);
      old_dw->get(vvel_CC, lb->vvel_CCLabel,    dwindex,patch,Ghost::None, 0);
      old_dw->get(wvel_CC, lb->wvel_CCLabel,    dwindex,patch,Ghost::None, 0);
      old_dw->get(cv_CC,   lb->cv_CCLabel,      dwindex,patch,Ghost::None, 0);
      old_dw->get(temp_CC, lb->temp_CCLabel,    dwindex,patch,Ghost::None, 0);
      new_dw->get(xmom_source,    lb->xmom_source_CCLabel,
						      dwindex,patch,Ghost::None, 0);
      new_dw->get(ymom_source,    lb->ymom_source_CCLabel,
						      dwindex,patch,Ghost::None, 0);
      new_dw->get(zmom_source,    lb->zmom_source_CCLabel,
						      dwindex,patch,Ghost::None, 0);
      new_dw->get(int_eng_source, lb->int_eng_source_CCLabel,
						      dwindex,patch,Ghost::None, 0);
      //__________________________________
      // - create and allocate space in new dw
      CCVariable<double> xmom_L;
      CCVariable<double> ymom_L;
      CCVariable<double> zmom_L;
      CCVariable<double> int_eng_L;
      CCVariable<double> mass_L;
      CCVariable<double> rho_L;
      new_dw->allocate(xmom_L,    lb->xmom_L_CCLabel,    dwindex,patch);
      new_dw->allocate(ymom_L,    lb->ymom_L_CCLabel,    dwindex,patch);
      new_dw->allocate(zmom_L,    lb->zmom_L_CCLabel,    dwindex,patch);
      new_dw->allocate(int_eng_L, lb->int_eng_L_CCLabel, dwindex,patch);
      new_dw->allocate(mass_L,    lb->mass_L_CCLabel,    dwindex,patch);
      new_dw->allocate(rho_L,     lb->rho_L_CCLabel,     dwindex,patch);

      double vol = dx.x()*dx.y()*dx.z();
      for(CellIterator iter = patch->getExtraCellIterator(); !iter.done(); 
	    iter++)
     {
	  double mass = rho_CC[*iter] * vol;
	  mass_L[*iter] = mass; 
                // +  mass_source[*iter];
	  rho_L[*iter]  = mass_L[*iter]/vol;
         
	  xmom_L[*iter] = mass * uvel_CC[*iter]
		  //- uvel_CC[*iter] * mass_source[*iter]
		  + xmom_source[*iter];
         
	  ymom_L[*iter] = mass * vvel_CC[*iter]
		  //- vvel_CC[*iter] * mass_source[*iter]
		  + ymom_source[*iter];
         
	  zmom_L[*iter] = mass * wvel_CC[*iter]
		  //- wvel_CC[*iter] * mass_source[*iter]
		  + zmom_source[*iter];
         
	  int_eng_L[*iter] = mass * cv_CC[*iter] * temp_CC[*iter]
		  //-cv_CC[*iter] * temp_CC * mass_source[*iter]
		  + int_eng_source[*iter];
  #ifdef john_debug
	  cout << "mass_L"<<*iter<<"="<<mass_L[*iter] << endl;
	  cout << "rho_L"<<*iter<<"="<<rho_L[*iter] << endl;
	  cout << "xmom_L"<<*iter<<"="<<xmom_L[*iter] << endl;
	  cout << "ymom_L"<<*iter<<"="<<ymom_L[*iter] << endl;
	  cout << "zmom_L"<<*iter<<"="<<zmom_L[*iter] << endl;
	  cout << "int_eng_L"<<*iter<<"="<<int_eng_L[*iter] << endl << endl;
  #endif
      }
     //__________________________________
     //  Now put data in new dw
      new_dw->put(xmom_L,    lb->xmom_L_CCLabel,    dwindex,patch);
      new_dw->put(ymom_L,    lb->ymom_L_CCLabel,    dwindex,patch);
      new_dw->put(zmom_L,    lb->zmom_L_CCLabel,    dwindex,patch);
      new_dw->put(int_eng_L, lb->int_eng_L_CCLabel, dwindex,patch);
      new_dw->put(mass_L,    lb->mass_L_CCLabel,    dwindex,patch);
  }
}
//STOP_DOC                                                                 
/*---------------------------------------------------------------------
 Function~  add_exchange_contribution_to_data_L_CC--
 Purpose~
   This function adds the energy exchange contribution to the 
   existing cell-centered lagrangian temperature

 Prerequisites:
            The face centered velocity for each material without
            the exchange must be solved prior to this routine.
            
                   (A)                              (X)
| (1+b12 + b13)     -b12          -b23          |   |del_data_CC[1]  |    
|                                               |   |                |    
| -b21              (1+b21 + b23) -b32          |   |del_data_CC[2]  |    
|                                               |   |                | 
| -b31              -b32          (1+b31 + b32) |   |del_data_CC[2]  |

                        =
                        
                        (B)
| b12( data_CC[2] - data_CC[1] ) + b13 ( data_CC[3] -data_CC[1])    | 
|                                                                   |
| b21( data_CC[1] - data_CC[2] ) + b23 ( data_CC[3] -data_CC[2])    | 
|                                                                   |
| b31( data_CC[1] - data_CC[3] ) + b32 ( data_CC[2] -data_CC[3])    |           

Programmer         Date       Description
----------         ----       -----------
Jim Guilkey       10/04/00
 
 Steps for each face:
    1) Comute the beta coefficients
    2) Form and A matrix and B vector
    3) Solve for del_data_CC[*]
    4) Add del_data_CC[*] to the appropriate Lagrangian data
 
 References: see "A Cell-Centered ICE method for multiphase flow simulations"
 by Kashiwa, above equation 4.13.
 ---------------------------------------------------------------------  */
void ICE::actuallyStep5b(
    const ProcessorGroup*,
    const Patch*    patch,
    DataWarehouseP& old_dw,
    DataWarehouseP& new_dw)
{
  cout << "Doing actually step5b -- Heat and momentum exchange" << endl;

  int     numMatls  = d_sharedState->getNumICEMatls();
  double  temp;
  int     itworked;
  Vector dx         = patch->dCell();
  Vector gravity    = d_sharedState->getGravity();
  delt_vartype delT;
  old_dw->get(delT, d_sharedState->get_delt_label());
  //__________________________________
  //  - Create local variables 
  vector<CCVariable<double> > rho_CC(numMatls);
  vector<CCVariable<double> > xmom_L(numMatls);
  vector<CCVariable<double> > ymom_L(numMatls);
  vector<CCVariable<double> > zmom_L(numMatls);
  vector<CCVariable<double> > int_eng_L(numMatls);
  vector<CCVariable<double> > vol_frac_CC(numMatls);
  vector<CCVariable<double> > rho_micro_CC(numMatls);
  vector<CCVariable<double> > cv_CC(numMatls);
  // Create variables for the results
  vector<CCVariable<double> > xmom_L_ME(numMatls);
  vector<CCVariable<double> > ymom_L_ME(numMatls);
  vector<CCVariable<double> > zmom_L_ME(numMatls);
  vector<CCVariable<double> > int_eng_L_ME(numMatls);
    
  vector<double> b(numMatls);
  vector<double> mass(numMatls);
  DenseMatrix beta(numMatls,numMatls),acopy(numMatls,numMatls);
  DenseMatrix K(numMatls,numMatls),H(numMatls,numMatls),a(numMatls,numMatls);
  //__________________________________
  // - fill local vars with dw data
  // - allocate space for results in dw
  for(int m = 0; m < numMatls; m++)
  {
    ICEMaterial* matl = d_sharedState->getICEMaterial( m );
    int dwindex = matl->getDWIndex();
    old_dw->get( rho_CC[m],         lb->rho_CCLabel,
                                    dwindex,    patch, Ghost::None, 0);
    new_dw->get( xmom_L[m],         lb->xmom_L_CCLabel,
                                    dwindex,    patch, Ghost::None, 0);
    new_dw->get( ymom_L[m],         lb->ymom_L_CCLabel,
                                    dwindex,    patch, Ghost::None, 0);
    new_dw->get( zmom_L[m],         lb->zmom_L_CCLabel,
                                    dwindex,    patch, Ghost::None, 0);
    new_dw->get( int_eng_L[m],      lb->int_eng_L_CCLabel,
                                    dwindex,    patch, Ghost::None, 0);
    new_dw->get( vol_frac_CC[m],    lb->vol_frac_CCLabel,
                                    dwindex,    patch, Ghost::None, 0);
    new_dw->get( rho_micro_CC[m],   lb->rho_micro_equil_CCLabel,
                                    dwindex,    patch, Ghost::None, 0);
    old_dw->get( cv_CC[m],          lb->cv_CCLabel,
                                    dwindex,    patch, Ghost::None, 0);

    new_dw->allocate( xmom_L_ME[m],  lb->xmom_L_ME_CCLabel,    dwindex, patch);
    new_dw->allocate( ymom_L_ME[m],  lb->ymom_L_ME_CCLabel,    dwindex, patch);
    new_dw->allocate( zmom_L_ME[m],  lb->zmom_L_ME_CCLabel,    dwindex, patch);
    new_dw->allocate(int_eng_L_ME[m],lb->int_eng_L_ME_CCLabel, dwindex, patch);
  }
  //__________________________________
  // - pull out the exchange coefficients
  for (int i = 0; i < numMatls; i++ ) 
  {
      K[numMatls-1-i][i] = d_K_mom[i];
      H[numMatls-1-i][i] = d_K_heat[i];
  }
  //__________________________________
  // Set (*)mom_L_ME = (*)mom_L
  // if you have only 1 mat then there is no exchange
  for (int m = 0; m < numMatls; m++) 
  {
   xmom_L_ME[m] = xmom_L[m];
   ymom_L_ME[m] = ymom_L[m];
   zmom_L_ME[m] = zmom_L[m];
   int_eng_L_ME[m] = int_eng_L[m];
  }

  double vol = dx.x()*dx.y()*dx.z();
  for(CellIterator iter = patch->getExtraCellIterator(); !iter.done(); iter++)
  {
    //__________________________________
    //   Form BETA matrix (a), off diagonal terms
    for(int m = 0; m < numMatls; m++)
    {
        temp    = rho_micro_CC[m][*iter];
        mass[m] = rho_CC[m][*iter] * vol;
        
        for(int n = 0; n < numMatls; n++)
        {
            beta[m][n] = delT * vol_frac_CC[n][*iter] * K[n][m]/temp;
            a[m][n] = -beta[m][n];
        }
    }
    //__________________________________
    //   Form matrix (a) diagonal terms
    for(int m = 0; m < numMatls; m++)
    {
        a[m][m] = 1.;
        for(int n = 0; n < numMatls; n++)
        {
            a[m][m] +=  beta[m][n];
        }
    }
    //__________________________________
    //     X - M O M E N T U M
    // -  F O R M   R H S   (b)
    //   convert flux to primiative variable
    for(int m = 0; m < numMatls; m++)
    {
        b[m] = 0.0;
        for(int n = 0; n < numMatls; n++)
        {
            b[m] += beta[m][n] *
                    (xmom_L[n][*iter]/mass[n] - xmom_L[m][*iter]/mass[m]);
        }
    }
    //__________________________________
    //     S O L V E
    //  - push a copy of (a) into the solver
    //  - Add exchange contribution to orig value
    acopy = a;
    itworked = acopy.solve(b);
    for(int m = 0; m < numMatls; m++)
    {
        xmom_L_ME[m][*iter] = xmom_L[m][*iter] + b[m]*mass[m];
    }
    //__________________________________
    //     Y - M O M E N T U M
    // -  F O R M   R H S   (b)
    //   convert flux to primiative variable
    for(int m = 0; m < numMatls; m++)
    {
        b[m] = 0.0;
        for(int n = 0; n < numMatls; n++)
        {
            b[m] += beta[m][n] *
                    (ymom_L[n][*iter]/mass[n] - ymom_L[m][*iter]/mass[m]);
        }
    }
    //__________________________________
    //     S O L V E
    //  - push a copy of (a) into the solver
    //  - Add exchange contribution to orig value
    acopy    = a;
    itworked = acopy.solve(b);
    for(int m = 0; m < numMatls; m++)
    {
        ymom_L_ME[m][*iter] = ymom_L[m][*iter] + b[m]*mass[m];
    }
    //__________________________________
    //     Z - M O M E N T U M
    // -  F O R M   R H S   (b)
    //   convert flux to primiative variable
    for(int m = 0; m < numMatls; m++)
    {
        b[m] = 0.0;
        for(int n = 0; n < numMatls; n++)
        {
            b[m] += beta[m][n] *
                    (zmom_L[n][*iter]/mass[n] - zmom_L[m][*iter]/mass[m]);
        }
    }    
    //__________________________________
    //     S O L V E
    //  - push a copy of (a) into the solver
    //  - Add exchange contribution to orig value
    acopy    = a;
    itworked = acopy.solve(b);
    for(int m = 0; m < numMatls; m++)
    {
      zmom_L_ME[m][*iter] = zmom_L[m][*iter] + b[m]*mass[m];
    }
    //______________________________________________________________________
    //    E N E R G Y   E X C H A N G E
    //   Form BETA matrix (a) off diagonal terms
    for(int m = 0; m < numMatls; m++)
    {
        temp = cv_CC[m][*iter]*rho_micro_CC[m][*iter];
        for(int n = 0; n < numMatls; n++)
        {
            beta[m][n] = delT * vol_frac_CC[n][*iter] * H[n][m]/temp;
            a[m][n] = -beta[m][n];
        }
    }
    //__________________________________
    //   Form matrix (a) diagonal terms
    for(int m = 0; m < numMatls; m++)
    {
        a[m][m] = 1.;
        for(int n = 0; n < numMatls; n++)
        {
            a[m][m] +=  beta[m][n];
        }
    }
    //__________________________________
    // -  F O R M   R H S   (b), convert flux to primiative variable
    for(int m = 0; m < numMatls; m++)
    {
        b[m] = 0.0;
        for(int n = 0; n < numMatls; n++)
        {
            b[m] += beta[m][n] *
                    (int_eng_L[n][*iter]/(mass[n]*cv_CC[n][*iter]) -
                     int_eng_L[m][*iter]/(mass[m]*cv_CC[m][*iter]));
        }
    }
    //__________________________________
    //     S O L V E, Add exchange contribution to orig value
    itworked = a.solve(b);
    for(int m = 0; m < numMatls; m++)
    {
        int_eng_L_ME[m][*iter] =
                  int_eng_L[m][*iter] + b[m]*mass[m]*cv_CC[m][*iter];
    }
  }
  //__________________________________
  //  Update any neumann boundary conditions
  for (int m = 0; m < numMatls; m++) 
  {
      setBC(xmom_L_ME[m],"Velocity",patch);
      setBC(ymom_L_ME[m],"Velocity",patch);
      setBC(zmom_L_ME[m],"Velocity",patch);
  }
  //__________________________________
  //  Put data into new dw  
  for(int m = 0; m < numMatls; m++)
  {
     ICEMaterial* matl = d_sharedState->getICEMaterial( m );
     int dwindex = matl->getDWIndex();
     new_dw->put(xmom_L_ME[m],   lb->xmom_L_ME_CCLabel,   dwindex, patch);
     new_dw->put(ymom_L_ME[m],   lb->ymom_L_ME_CCLabel,   dwindex, patch);
     new_dw->put(zmom_L_ME[m],   lb->zmom_L_ME_CCLabel,   dwindex, patch);
     new_dw->put(int_eng_L_ME[m],lb->int_eng_L_ME_CCLabel,dwindex, patch);
  }
  //__________________________________
  //    KEEP WALL AND FULL WARN QUITE
    itworked = itworked;
}
//STOP_DOC
/* --------------------------------------------------------------------- 
 Function~  ICE::actuallyStep6and7--
 Purpose~
   This function calculates the The cell-centered, time n+1, mass, momentum
   and internal energy

Programmer         Date       Description
----------         ----       -----------
Jim Guilkey       10/04/00    

Need to include kinetic energy 
 ---------------------------------------------------------------------  */
void ICE::actuallyStep6and7(
    const ProcessorGroup*,
    const Patch* patch,
    DataWarehouseP& old_dw,
    DataWarehouseP& new_dw)
{

  cout << "Doing actually step6 and 7" << endl;
  delt_vartype delT;
  old_dw->get(delT, d_sharedState->get_delt_label());

  double delt_CFL = 100000,delt_stability = 1000000,fudge_factor = 1.;
  Vector dx = patch->dCell();
  double vol = dx.x()*dx.y()*dx.z(),mass;
  double invvol = 1.0/vol;

  double CFL,N_ITERATIONS_TO_STABILIZE = 2;
  iterNum++;
  if (iterNum < N_ITERATIONS_TO_STABILIZE) 
  {
    CFL = d_CFL * (double)iterNum *(1./(double)N_ITERATIONS_TO_STABILIZE);
  } else 
  {
    CFL = d_CFL;
  }
#ifdef john_debug
  cout << "CFL = " << CFL << endl;
#endif

  CCVariable<double> uvel_CC, vvel_CC, wvel_CC, rho_CC, visc_CC, cv_old, cv,temp;
  
  CCVariable<double> rho_CC_old, uvel_CC_old, vvel_CC_old, wvel_CC_old, 
                     temp_CC_old;

  CCVariable<double> xmom_L_ME, ymom_L_ME, zmom_L_ME, int_eng_L_ME, mass_L;
  CCVariable<double> speedSound;

  SFCXVariable<double> uvel_FC;
  SFCYVariable<double> vvel_FC;
  SFCZVariable<double> wvel_FC;
   //__________________________________
  // Allocate the temporary variables needed for advection
  // These arrays get re-used for each material, and for each
  // advected quantity
  CCVariable<double> q_CC, q_advected;
  CCVariable<fflux> IFS,OFS,q_out,q_in;
  CCVariable<eflux> IFE,OFE,q_out_EF,q_in_EF;

  new_dw->allocate(q_CC,       lb->q_CCLabel,       0, patch);
  new_dw->allocate(q_advected, lb->q_advectedLabel, 0, patch);
  new_dw->allocate(IFS,        IFS_CCLabel,         0, patch);
  new_dw->allocate(OFS,        OFS_CCLabel,         0, patch);
  new_dw->allocate(IFE,        IFE_CCLabel,         0, patch);
  new_dw->allocate(OFE,        OFE_CCLabel,         0, patch);
  new_dw->allocate(q_out,      q_outLabel,          0, patch);
  new_dw->allocate(q_out_EF,   q_out_EFLabel,       0, patch);
  new_dw->allocate(q_in,       q_inLabel,           0, patch);
  new_dw->allocate(q_in_EF,    q_in_EFLabel,        0, patch);

  for (int m = 0; m < d_sharedState->getNumICEMatls(); m++ ) 
  {
      //__________________________________
      //   - grab data from the new dw 
      //   - allocate memory in new dw
      ICEMaterial* ice_matl = d_sharedState->getICEMaterial(m);
      int dwindex = ice_matl->getDWIndex();

      old_dw->get(cv_old,     lb->cv_CCLabel,      dwindex,patch,Ghost::None,0);
  
/*`==========TESTING==========*/ 
    old_dw->get(rho_CC_old,lb->rho_CCLabel,   dwindex, patch,Ghost::None, 0);
    old_dw->get(cv_old,lb->cv_CCLabel,   dwindex, patch,Ghost::None, 0);
    old_dw->get(rho_CC_old,lb->rho_CCLabel,   dwindex, patch,Ghost::None, 0);
    old_dw->get(uvel_CC_old,lb->uvel_CCLabel,   dwindex, patch,Ghost::None, 0);
    old_dw->get(vvel_CC_old,lb->vvel_CCLabel,   dwindex, patch,Ghost::None, 0);
    old_dw->get(wvel_CC_old,lb->wvel_CCLabel,   dwindex, patch,Ghost::None, 0);
    old_dw->get(temp_CC_old,lb->temp_CCLabel,   dwindex, patch,Ghost::None, 0);

 /*==========TESTING==========`*/     
      new_dw->get(uvel_FC,   lb->uvel_FCMELabel,   dwindex,patch,Ghost::None,0);
      new_dw->get(vvel_FC,   lb->vvel_FCMELabel,   dwindex,patch,Ghost::None,0);
      new_dw->get(wvel_FC,   lb->wvel_FCMELabel,   dwindex,patch,Ghost::None,0);
      new_dw->get(xmom_L_ME, lb->xmom_L_ME_CCLabel,dwindex,patch,Ghost::None,0);
      new_dw->get(ymom_L_ME, lb->ymom_L_ME_CCLabel,dwindex,patch,Ghost::None,0);
      new_dw->get(zmom_L_ME, lb->zmom_L_ME_CCLabel,dwindex,patch,Ghost::None,0);
      new_dw->get(mass_L,    lb->mass_L_CCLabel,   dwindex,patch,Ghost::None,0);
      new_dw->get(int_eng_L_ME,lb->int_eng_L_ME_CCLabel,
                                                   dwindex,patch,Ghost::None,0);
      new_dw->get(speedSound, lb->speedSound_equiv_CCLabel,
                                                   dwindex,patch,Ghost::None,0);

      new_dw->allocate(rho_CC, lb->rho_CCLabel,        dwindex,patch);
      new_dw->allocate(temp,   lb->temp_CCLabel,       dwindex,patch);
      new_dw->allocate(cv,     lb->cv_CCLabel,         dwindex,patch);
      new_dw->allocate(uvel_CC,lb->uvel_CCLabel,       dwindex,patch);
      new_dw->allocate(vvel_CC,lb->vvel_CCLabel,       dwindex,patch);
      new_dw->allocate(wvel_CC,lb->wvel_CCLabel,       dwindex,patch);
      new_dw->allocate(visc_CC,lb->viscosity_CCLabel,  dwindex,patch);

/*`==========TESTING==========*/ 
    cv = cv_old;
    rho_CC = rho_CC_old;
     uvel_CC = uvel_CC_old;
     vvel_CC = vvel_CC_old;
     wvel_CC = wvel_CC_old;
     temp = temp_CC_old; 
 /*==========TESTING==========`*/
      //__________________________________
      //   Advection preprocessings
      influxOutfluxVolume(
                  uvel_FC,    vvel_FC,      wvel_FC,
                  delT,       patch,
                  OFS,        OFE,          IFS,        IFE);

      // outflowVolCentroid goes here if doing second order
      //outflowVolCentroid(uvel_FC,vvel_FC,wvel_FC,delT,dx,
      //           r_out_x, r_out_y, r_out_z,
      //           r_out_x_CF, r_out_y_CF, r_out_z_CF);

      //__________________________________
      // Advect mass and backout rho_CC
      for(CellIterator iter = patch->getExtraCellIterator(); !iter.done();iter++)
      {
          q_CC[*iter] = mass_L[*iter] * invvol;
      }

      advectQFirst( q_CC,       patch,
                    OFS,        OFE,        IFS,        IFE,
                    q_out,      q_out_EF,   q_in,       q_in_EF,   q_advected);

      for(CellIterator iter = patch->getCellIterator(); !iter.done(); iter++)
      {
  #ifdef john_debug
          cout << "mass_L"<<*iter<<"="<<mass_L[*iter] << " q_advected = " <<
          q_advected[*iter] << " invvol = " << invvol << endl;
  #endif
            rho_CC[*iter] = (mass_L[*iter] + q_advected[*iter]) * invvol;
  #ifdef john_debug
            cout << "rho_CC" << *iter << "=" << rho_CC[*iter] << endl;
  #endif
      }
      //__________________________________
      // Advect X momentum and backout uvel_CC
      for(CellIterator iter = patch->getExtraCellIterator(); !iter.done(); iter++)
      {
          q_CC[*iter] = xmom_L_ME[*iter] * invvol;
      }
      
      advectQFirst( q_CC,       patch,
                    OFS,        OFE,        IFS,        IFE,
                    q_out,      q_out_EF,   q_in,       q_in_EF,   q_advected);


      for(CellIterator iter = patch->getCellIterator(); !iter.done();  iter++)
      {
          mass = rho_CC[*iter] * vol;
          uvel_CC[*iter] = (xmom_L_ME[*iter] + q_advected[*iter])/mass;
      }
      //__________________________________
      // Advect Y momentum and backout vvel_CC
      for(CellIterator iter = patch->getExtraCellIterator(); !iter.done(); iter++)
      {
          q_CC[*iter] = ymom_L_ME[*iter] * invvol;
      }

       advectQFirst( q_CC,       patch,
                    OFS,        OFE,        IFS,        IFE,
                    q_out,      q_out_EF,   q_in,       q_in_EF,   q_advected);
                    
      for(CellIterator iter = patch->getCellIterator(); !iter.done(); iter++)
      {
        mass = rho_CC[*iter] * vol;
        vvel_CC[*iter] = (ymom_L_ME[*iter] + q_advected[*iter])/mass;
      }
      //__________________________________
      // Advect Z momentum and backout wvel_CC
      for(CellIterator iter = patch->getExtraCellIterator(); !iter.done(); iter++)
      {
        q_CC[*iter] = zmom_L_ME[*iter] * invvol;
      }

      advectQFirst( q_CC,       patch,
                    OFS,        OFE,        IFS,        IFE,
                    q_out,      q_out_EF,   q_in,       q_in_EF,   q_advected);
                    
      for(CellIterator iter = patch->getCellIterator(); !iter.done(); iter++)
      {
        mass = rho_CC[*iter] * vol;
        wvel_CC[*iter] = (zmom_L_ME[*iter] + q_advected[*iter])/mass;
      }
      //__________________________________
      // Advect internal energy and backout Temp_CC
      for(CellIterator iter = patch->getExtraCellIterator(); !iter.done(); iter++)
      {
        q_CC[*iter] = int_eng_L_ME[*iter] * invvol;
      }

      advectQFirst( q_CC,       patch,
                    OFS,        OFE,        IFS,        IFE,
                    q_out,      q_out_EF,   q_in,       q_in_EF,   q_advected);

      for(CellIterator iter = patch->getCellIterator(); !iter.done(); iter++)
      {
          mass = rho_CC[*iter] * vol;
          temp[*iter] = (int_eng_L_ME[*iter] + q_advected[*iter])/
                                                          (mass*cv[*iter]);
      }

  #ifdef john_debug
      cout << "Before applying bcs  6&7cd. . . " << endl << endl;
      for(CellIterator iter = patch->getExtraCellIterator(); !iter.done(); 
          iter++) {
        cout << "rho"<<*iter<<"="<<rho_CC[*iter]<< endl;
        cout << "temp"<<*iter<<"="<<temp[*iter]<< endl;
        cout << "uvel"<<*iter<<"="<<uvel_CC[*iter]<< endl;
        cout << "vvel"<<*iter<<"="<<vvel_CC[*iter]<< endl;
        cout << "wvel"<<*iter<<"="<<wvel_CC[*iter]<< endl;
      }
  #endif
      //__________________________________
      //   Update the boundary conditions
      setBC(rho_CC,   "Density",              patch);
      setBC(temp,     "Temperature",          patch);
      setBC(uvel_CC,  "Velocity",     "x",    patch);
      setBC(vvel_CC,  "Velocity",     "y",    patch);
      setBC(wvel_CC,  "Velocity",     "z",    patch);

  #ifdef john_debug
      cout << "After applying bcs . . . " << endl << endl;
      for(CellIterator iter = patch->getExtraCellIterator(); !iter.done(); 
          iter++) {
        cout << "rho"<<*iter<<"="<<rho_CC[*iter]<< endl;
        cout << "temp"<<*iter<<"="<<temp[*iter]<< endl;
        cout << "uvel"<<*iter<<"="<<uvel_CC[*iter]<< endl;
        cout << "vvel"<<*iter<<"="<<vvel_CC[*iter]<< endl;
        cout << "wvel"<<*iter<<"="<<wvel_CC[*iter]<< endl;
      }
  #endif
      //__________________________________
      //  Put updated arrays into dw
       new_dw->put(rho_CC, lb->rho_CCLabel,  dwindex,patch);
       new_dw->put(uvel_CC,lb->uvel_CCLabel, dwindex,patch);
       new_dw->put(vvel_CC,lb->vvel_CCLabel, dwindex,patch);
       new_dw->put(wvel_CC,lb->wvel_CCLabel, dwindex,patch);
       new_dw->put(temp,   lb->temp_CCLabel, dwindex,patch);

       // These are carried forward variables, they don't change
       new_dw->put(visc_CC,lb->viscosity_CCLabel,dwindex,patch);
       new_dw->put(cv,     lb->cv_CCLabel,       dwindex,patch);
  }
  //______________________________________________________________________
  // Compute new delt
    for (int m = 0; m < d_sharedState->getNumICEMatls(); m++ ) 
    {
   for(CellIterator iter = patch->getExtraCellIterator(); !iter.done(); iter++)
      {
         double A = fudge_factor*CFL*dx.x()/(speedSound[*iter] + 
                                fabs(uvel_CC[*iter])+d_SMALL_NUM);
         double B = fudge_factor*CFL*dx.y()/(speedSound[*iter] + 
                                fabs(vvel_CC[*iter])+d_SMALL_NUM);
         double C = fudge_factor*CFL*dx.z()/(speedSound[*iter] + 
                                fabs(wvel_CC[*iter])+d_SMALL_NUM);

         delt_CFL = std::min(A, delt_CFL);
         delt_CFL = std::min(B, delt_CFL);
         delt_CFL = std::min(C, delt_CFL);

         A = fudge_factor * 0.5 * (dx.x()*dx.x())/fabs(uvel_CC[*iter]);
         B = fudge_factor * 0.5 * (dx.y()*dx.y())/fabs(vvel_CC[*iter]);
         C = fudge_factor * 0.5 * (dx.z()*dx.z())/fabs(wvel_CC[*iter]);

         delt_stability = std::min(A, delt_stability);
         delt_stability = std::min(B, delt_stability);
         delt_stability = std::min(C, delt_stability);
       }
  }
  double dT = std::min(delt_stability, delt_CFL);
#ifdef john_debug
  cout << "new dT = " << dT << endl;
#endif
  new_dw->put(delt_vartype(dT), lb->delTLabel);

}
//STOP_DOC


void ICE::setBC(CCVariable<double>& variable, const string& kind, 
		const Patch* patch)
{
  //  cout << "Setting the " << kind << endl;
  Vector dx = patch->dCell();
  for(Patch::FaceType face = Patch::startFace;
      face <= Patch::endFace; face=Patch::nextFace(face)){
    vector<BoundCondBase* > bcs;
    bcs = patch->getBCValues(face);
#ifdef john_debug
    cout << bcs.size() << " of BCS for face " << face << endl;
    for (int i = 0; i<(int)bcs.size(); i++) {
      cout << "BC kind = " << bcs[i]->getType() << endl;
    }
#endif
    if (bcs.size() == 0) continue;
    
    BoundCondBase* bc_base = 0;
#ifdef john_debug
    cout << "bc_base = " << bc_base << endl;
#endif
    for (int i = 0; i<(int)bcs.size(); i++ ) {
      if (bcs[i]->getType() == kind) {
	bc_base = bcs[i];
	break;
      }
    }
#ifdef john_debug
    cout << "bc_base = " << bc_base << endl;
#endif
    if (bc_base == 0)
      continue;
    
    if (bc_base->getType() == "Pressure") {
      PressureBoundCond* bc = dynamic_cast<PressureBoundCond*>(bc_base);
      if (bc->getKind() == "Dirichlet") 
	variable.fillFace(face,bc->getValue());
      
      if (bc->getKind() == "Neumann") 
	variable.fillFaceFlux(face,bc->getValue(),dx);
	
    }
    if (bc_base->getType() == "Density") {
      DensityBoundCond* bc = dynamic_cast<DensityBoundCond*>(bc_base);
      if (bc->getKind() == "Dirichlet") 
	variable.fillFace(face,bc->getValue());
      
      if (bc->getKind() == "Neumann") 
	variable.fillFaceFlux(face,bc->getValue(),dx);
    }
    if (bc_base->getType() == "Temperature") {
      TemperatureBoundCond* bc = dynamic_cast<TemperatureBoundCond*>(bc_base);
      if (bc->getKind() == "Dirichlet") 
	variable.fillFace(face,bc->getValue());
      
      if (bc->getKind() == "Neumann") 
	variable.fillFaceFlux(face,bc->getValue(),dx);
	
    }

  }

}


void ICE::setBC(CCVariable<double>& variable, const  string& kind, 
		const string& comp, const Patch* patch) 
{
  Vector dx = patch->dCell();
  for(Patch::FaceType face = Patch::startFace;
      face <= Patch::endFace; face=Patch::nextFace(face)){
    vector<BoundCondBase* > bcs;
    bcs = patch->getBCValues(face);
    if (bcs.size() == 0) continue;
    
    BoundCondBase* bc_base = 0;
    for (int i = 0; i<(int)bcs.size(); i++ ) {
      if (bcs[i]->getType() == kind) {
	bc_base = bcs[i];
	break;
      }
    }

    if (bc_base == 0)
      continue;
    
    if (bc_base->getType() == "Velocity") {
      VelocityBoundCond* bc = dynamic_cast<VelocityBoundCond*>(bc_base);
      if (bc->getKind() == "Dirichlet") {
	if (comp == "x")
	  variable.fillFace(face,bc->getValue().x());
	if (comp == "y")
	  variable.fillFace(face,bc->getValue().y());
	if (comp == "z")
	  variable.fillFace(face,bc->getValue().z());
      }
      
      if (bc->getKind() == "Neumann") {
	if (comp == "x")
	  variable.fillFaceFlux(face,bc->getValue().x(),dx);
	if (comp == "y")
	  variable.fillFaceFlux(face,bc->getValue().y(),dx);
	if (comp == "z")
	  variable.fillFaceFlux(face,bc->getValue().z(),dx);
      }
    }
  }

}

void ICE::setBC(SFCXVariable<double>& variable, const string& kind, 
		const Patch* patch)
{

  Vector dx = patch->dCell();
  for(Patch::FaceType face = Patch::startFace;
      face <= Patch::endFace; face=Patch::nextFace(face)){
    vector<BoundCondBase* > bcs;
    bcs = patch->getBCValues(face);
    if (bcs.size() == 0) continue;
    
    BoundCondBase* bc_base = 0;
    for (int i = 0; i<(int)bcs.size(); i++ ) {
      if (bcs[i]->getType() == kind) {
	bc_base = bcs[i];
	break;
      }
    }

    if (bc_base == 0)
      continue;
    
    if (bc_base->getType() == "Pressure") {
      PressureBoundCond* bc = dynamic_cast<PressureBoundCond*>(bc_base);
      if (bc->getKind() == "Dirichlet") 
	variable.fillFace(face,bc->getValue());
      
      if (bc->getKind() == "Neumann") 
	variable.fillFaceFlux(face,bc->getValue(),dx);
    }
    if (bc_base->getType() == "Density") {
      DensityBoundCond* bc = dynamic_cast<DensityBoundCond*>(bc_base);
      if (bc->getKind() == "Dirichlet") 
	variable.fillFace(face,bc->getValue());
      
      if (bc->getKind() == "Neumann") 
	variable.fillFaceFlux(face,bc->getValue(),dx);
    }
    if (bc_base->getType() == "Temperature") {
      TemperatureBoundCond* bc = dynamic_cast<TemperatureBoundCond*>(bc_base);
      if (bc->getKind() == "Dirichlet") 
	variable.fillFace(face,bc->getValue());
      
      if (bc->getKind() == "Neumann") 
	variable.fillFaceFlux(face,bc->getValue(),dx);
    }
  }

}


void ICE::setBC(SFCXVariable<double>& variable, const  string& kind, 
		const string& comp, const Patch* patch) 
{
  Vector dx = patch->dCell();
  for(Patch::FaceType face = Patch::startFace;
      face <= Patch::endFace; face=Patch::nextFace(face)){
    vector<BoundCondBase* > bcs;
    bcs = patch->getBCValues(face);
    if (bcs.size() == 0) continue;
    
    BoundCondBase* bc_base = 0;
    for (int i = 0; i<(int)bcs.size(); i++ ) {
      if (bcs[i]->getType() == kind) {
	bc_base = bcs[i];
	break;
      }
    }
    
    if (bc_base == 0)
      continue;

    if (bc_base->getType() == "Velocity") {
      VelocityBoundCond* bc = dynamic_cast<VelocityBoundCond*>(bc_base);
      if (bc->getKind() == "Dirichlet") {
	if (comp == "x")
	  variable.fillFace(face,bc->getValue().x());
	if (comp == "y")
	  variable.fillFace(face,bc->getValue().y());
	if (comp == "z")
	  variable.fillFace(face,bc->getValue().z());
      }
      
      if (bc->getKind() == "Neumann") {
	if (comp == "x")
	  variable.fillFaceFlux(face,bc->getValue().x(),dx);
	if (comp == "y")
	  variable.fillFaceFlux(face,bc->getValue().y(),dx);
	if (comp == "z")
	  variable.fillFaceFlux(face,bc->getValue().z(),dx);
      }
    }
  }

}



void ICE::setBC(SFCYVariable<double>& variable, const string& kind, 
		const Patch* patch)
{

  Vector dx = patch->dCell();
  for(Patch::FaceType face = Patch::startFace;
      face <= Patch::endFace; face=Patch::nextFace(face)){
    vector<BoundCondBase* > bcs;
    bcs = patch->getBCValues(face);
    if (bcs.size() == 0) continue;
    
    BoundCondBase* bc_base = 0;
    for (int i = 0; i<(int)bcs.size(); i++ ) {
      if (bcs[i]->getType() == kind) {
	bc_base = bcs[i];
	break;
      }
    }

    if (bc_base == 0)
      continue;
    
    if (bc_base->getType() == "Pressure") {
      PressureBoundCond* bc = dynamic_cast<PressureBoundCond*>(bc_base);
      if (bc->getKind() == "Dirichlet") 
	variable.fillFace(face,bc->getValue());
      
      if (bc->getKind() == "Neumann") 
	variable.fillFaceFlux(face,bc->getValue(),dx);
    }
    if (bc_base->getType() == "Density") {
      DensityBoundCond* bc = dynamic_cast<DensityBoundCond*>(bc_base);
      if (bc->getKind() == "Dirichlet") 
	variable.fillFace(face,bc->getValue());
      
      if (bc->getKind() == "Neumann") 
	variable.fillFaceFlux(face,bc->getValue(),dx);
    }
    if (bc_base->getType() == "Temperature") {
      TemperatureBoundCond* bc = dynamic_cast<TemperatureBoundCond*>(bc_base);
      if (bc->getKind() == "Dirichlet") 
	variable.fillFace(face,bc->getValue());
      
      if (bc->getKind() == "Neumann") 
	variable.fillFaceFlux(face,bc->getValue(),dx);
    }
  }

}



void ICE::setBC(SFCYVariable<double>& variable, const  string& kind, 
		const string& comp, const Patch* patch) 
{
  Vector dx = patch->dCell();
  for(Patch::FaceType face = Patch::startFace;
      face <= Patch::endFace; face=Patch::nextFace(face)){
    vector<BoundCondBase* > bcs;
    bcs = patch->getBCValues(face);
    if (bcs.size() == 0) continue;
    
    BoundCondBase* bc_base = 0;
    for (int i = 0; i<(int)bcs.size(); i++ ) {
      if (bcs[i]->getType() == kind) {
	bc_base = bcs[i];
	break;
      }
    }
    
    if (bc_base == 0)
      continue;

    if (bc_base->getType() == "Velocity") {
      VelocityBoundCond* bc = dynamic_cast<VelocityBoundCond*>(bc_base);
      if (bc->getKind() == "Dirichlet") {
	if (comp == "x")
	  variable.fillFace(face,bc->getValue().x());
	if (comp == "y")
	  variable.fillFace(face,bc->getValue().y());
	if (comp == "z")
	  variable.fillFace(face,bc->getValue().z());
      }
      
      if (bc->getKind() == "Neumann") {
	if (comp == "x")
	  variable.fillFaceFlux(face,bc->getValue().x(),dx);
	if (comp == "y")
	  variable.fillFaceFlux(face,bc->getValue().y(),dx);
	if (comp == "z")
	  variable.fillFaceFlux(face,bc->getValue().z(),dx);
      }
    }
  }

}




void ICE::setBC(SFCZVariable<double>& variable, const string& kind, 
		const Patch* patch)
{

  Vector dx = patch->dCell();
  for(Patch::FaceType face = Patch::startFace;
      face <= Patch::endFace; face=Patch::nextFace(face)){
    vector<BoundCondBase* > bcs;
    bcs = patch->getBCValues(face);
    if (bcs.size() == 0) continue;
    
    BoundCondBase* bc_base = 0;
    for (int i = 0; i<(int)bcs.size(); i++ ) {
      if (bcs[i]->getType() == kind) {
	bc_base = bcs[i];
	break;
      }
    }
    if (bc_base == 0)
      continue;

    if (bc_base->getType() == "Pressure") {
      PressureBoundCond* bc = dynamic_cast<PressureBoundCond*>(bc_base);
      if (bc->getKind() == "Dirichlet") 
	variable.fillFace(face,bc->getValue());
      
      if (bc->getKind() == "Neumann") 
	variable.fillFaceFlux(face,bc->getValue(),dx);
    }
    if (bc_base->getType() == "Density") {
      DensityBoundCond* bc = dynamic_cast<DensityBoundCond*>(bc_base);
      if (bc->getKind() == "Dirichlet") 
	variable.fillFace(face,bc->getValue());
      
      if (bc->getKind() == "Neumann") 
	variable.fillFaceFlux(face,bc->getValue(),dx);
    }
    if (bc_base->getType() == "Temperature") {
      TemperatureBoundCond* bc = dynamic_cast<TemperatureBoundCond*>(bc_base);
      if (bc->getKind() == "Dirichlet") 
	variable.fillFace(face,bc->getValue());
      
      if (bc->getKind() == "Neumann") 
	variable.fillFaceFlux(face,bc->getValue(),dx);
    }
  }

}

void ICE::setBC(SFCZVariable<double>& variable, const  string& kind, 
		const string& comp, const Patch* patch) 
{
  Vector dx = patch->dCell();
  for(Patch::FaceType face = Patch::startFace;
      face <= Patch::endFace; face=Patch::nextFace(face)){
    vector<BoundCondBase* > bcs;
    bcs = patch->getBCValues(face);
    if (bcs.size() == 0) continue;
    
    BoundCondBase* bc_base = 0;
    for (int i = 0; i<(int)bcs.size(); i++ ) {
      if (bcs[i]->getType() == kind) {
	bc_base = bcs[i];
	break;
      }
    }

    if (bc_base == 0)
      continue;
    
    if (bc_base->getType() == "Velocity") {
      VelocityBoundCond* bc = dynamic_cast<VelocityBoundCond*>(bc_base);
      if (bc->getKind() == "Dirichlet") {
	if (comp == "x")
	  variable.fillFace(face,bc->getValue().x());
	if (comp == "y")
	  variable.fillFace(face,bc->getValue().y());
	if (comp == "z")
	  variable.fillFace(face,bc->getValue().z());
      }
      
      if (bc->getKind() == "Neumann") {
	if (comp == "x")
	  variable.fillFaceFlux(face,bc->getValue().x(),dx);
	if (comp == "y")
	  variable.fillFaceFlux(face,bc->getValue().y(),dx);
	if (comp == "z")
	  variable.fillFaceFlux(face,bc->getValue().z(),dx);
      }
    }
  }

}


/* ---------------------------------------------------------------------
 Function~  influx_outflux_volume--
 Purpose~   calculate the individual outfluxes and influxes for each cell.
            This includes the slabs and edge fluxes
 References:
    "Compatible Fluxes for van Leer Advection" W.B VanderHeyden and 
    B.A. Kashiwa, Journal of Computational Physics, 146, 1-28, (1998) 
            
 Steps for each cell:  
 1) calculate the volume for each outflux
 3) set the influx_volume for the appropriate cell = to the q_outflux of the 
    adjacent cell. 

Implementation notes:
    The outflux of volume is calculated in each cell in the computational domain
    + one layer of extra cells  surrounding the domain.The face-centered velocity 
    needs to be defined on all faces for these cells 
    
 Programmer         Date       Descriptionĝ
 ----------         ----       -----------
  Jim Guilkey       10/04/00

See schematic diagram at bottom of ice.cc for del* definitions
 ---------------------------------------------------------------------  */
void ICE::influxOutfluxVolume(
    const SFCXVariable<double>&     uvel_FC,
    const SFCYVariable<double>&     vvel_FC,
    const SFCZVariable<double>&     wvel_FC,
    const double&                   delT, 
    const Patch*                    patch,
    CCVariable<fflux>&              OFS, 
    CCVariable<eflux>&              OFE,
    CCVariable<fflux>&              IFS, 
    CCVariable<eflux>&              IFE)

{
  Vector dx = patch->dCell();
  double delY_top, delY_bottom,delX_right, delX_left, delZ_front, delZ_back;
  double delX_tmp, delY_tmp,   delZ_tmp, totalfluxin;
  double vol = dx.x()*dx.y()*dx.z();

#ifdef john_debug
  cout << "delT = " << delT << endl;
#endif
  //__________________________________
  // Compute outfluxes for all cells and 
  //  extra cells
  for(CellIterator iter = patch->getExtraCellIterator(); !iter.done(); iter++)
  {
      delY_top    = std::max(0.0, (vvel_FC[*iter+IntVector(0,1,0)] * delT));
      delY_bottom = std::max(0.0,-(vvel_FC[*iter+IntVector(0,0,0)] * delT));
      delX_right  = std::max(0.0, (uvel_FC[*iter+IntVector(1,0,0)] * delT));
      delX_left   = std::max(0.0,-(uvel_FC[*iter+IntVector(0,0,0)] * delT));
      delZ_front  = std::max(0.0, (wvel_FC[*iter+IntVector(0,0,1)] * delT));
      delZ_back   = std::max(0.0,-(wvel_FC[*iter+IntVector(0,0,0)] * delT));
  #ifdef john_debug
      cout << "delY_top = " << delY_top << " "
           << "delY_bottom = " << delY_bottom << " "
           << "delX_right = " << delX_right << " "
           << "delX_left = " << delX_left << " "
           << "delZ_front = " << delZ_front << " "
           << "delZ_back = " << delZ_back << " " << endl << endl;
  #endif

      delX_tmp    = dx.x() - delX_right - delX_left;
      delY_tmp    = dx.y() - delY_top   - delY_bottom;
      delZ_tmp    = dx.z() - delZ_front - delZ_back;

      //__________________________________
      //   SLAB outfluxes
      OFS[*iter].d_fflux[TOP]    = delY_top     * delX_tmp * dx.z();
      OFS[*iter].d_fflux[BOTTOM] = delY_bottom  * delX_tmp * dx.z();
      OFS[*iter].d_fflux[RIGHT]  = delX_right   * delY_tmp * dx.z();
      OFS[*iter].d_fflux[LEFT]   = delX_left    * delY_tmp * dx.z();
      OFS[*iter].d_fflux[FRONT]  = delZ_front   * delZ_tmp * dx.y();
      OFS[*iter].d_fflux[BACK]   = delZ_back    * delZ_tmp * dx.y();
      //__________________________________
      // Corners (these are actually edges in 3-d)
      OFE[*iter].d_eflux[TR] = delY_top      * delX_right * dx.z();
      OFE[*iter].d_eflux[TL] = delY_top      * delX_left  * dx.z();
      OFE[*iter].d_eflux[BR] = delY_bottom   * delX_right * dx.z();
      OFE[*iter].d_eflux[BL] = delY_bottom   * delX_left  * dx.z();
      //__________________________________
      // These need to be filled in for 3-d
      OFE[*iter].d_eflux[TF] = 0.0;
      OFE[*iter].d_eflux[Tb] = 0.0;
      OFE[*iter].d_eflux[BF] = 0.0;
      OFE[*iter].d_eflux[Bb] = 0.0;
      OFE[*iter].d_eflux[FR] = 0.0;
      OFE[*iter].d_eflux[FL] = 0.0;
      OFE[*iter].d_eflux[bR] = 0.0;
      OFE[*iter].d_eflux[bL] = 0.0;
  }
//__________________________________
//     INFLUX TERMS
//  only in computational domain
  for(CellIterator iter = patch->getCellIterator(); !iter.done(); iter++)
  {
      IntVector curcell = *iter,adjcell;
      //__________________________________
      //   INFLUX SLABS
      adjcell = IntVector(curcell.x(),curcell.y()+1,curcell.z());
      IFS[*iter].d_fflux[TOP]    = OFS[adjcell].d_fflux[BOTTOM];

      adjcell = IntVector(curcell.x(),curcell.y()-1,curcell.z());
      IFS[*iter].d_fflux[BOTTOM] = OFS[adjcell].d_fflux[TOP];

      adjcell = IntVector(curcell.x()+1,curcell.y(),curcell.z());
      IFS[*iter].d_fflux[RIGHT]  = OFS[adjcell].d_fflux[LEFT];

      adjcell = IntVector(curcell.x()-1,curcell.y(),curcell.z());
      IFS[*iter].d_fflux[LEFT]   = OFS[adjcell].d_fflux[RIGHT];

      adjcell = IntVector(curcell.x(),curcell.y(),curcell.z()-1);
      IFS[*iter].d_fflux[FRONT]  = OFS[adjcell].d_fflux[BACK];

      adjcell = IntVector(curcell.x(),curcell.y(),curcell.z()+1);
      IFS[*iter].d_fflux[BACK]   = OFS[adjcell].d_fflux[FRONT];

      //__________________________________
      //    INFLUX EDGES
      adjcell = IntVector(curcell.x()+1,curcell.y()+1,curcell.z());
      IFE[*iter].d_eflux[TR]    = OFE[adjcell].d_eflux[BL];

      adjcell = IntVector(curcell.x()+1,curcell.y()-1,curcell.z());
      IFE[*iter].d_eflux[BR]    = OFE[adjcell].d_eflux[TL];

      adjcell = IntVector(curcell.x()-1,curcell.y()+1,curcell.z());
      IFE[*iter].d_eflux[TL]    = OFE[adjcell].d_eflux[BR];

      adjcell = IntVector(curcell.x()-1,curcell.y()-1,curcell.z());
      IFE[*iter].d_eflux[BL]    = OFE[adjcell].d_eflux[TR];
      
      //__________________________________
      // NEED TO ADD CORNER FLUX TERMS
      totalfluxin = IFS[*iter].d_fflux[TOP]   + IFS[*iter].d_fflux[BOTTOM] +
        IFS[*iter].d_fflux[RIGHT] + IFS[*iter].d_fflux[LEFT]   +
        IFS[*iter].d_fflux[FRONT] + IFS[*iter].d_fflux[BACK]   +
        IFE[*iter].d_eflux[TR]    + IFE[*iter].d_eflux[BR]     +
        IFE[*iter].d_eflux[TL]    + IFE[*iter].d_eflux[BL];
  #ifdef john_debug
      cout << "totalfluxin = " << totalfluxin << endl;
      cout << "vol = " << vol << endl;
  #endif
       ASSERT(totalfluxin < vol);
  }
}
//STOP_DOC
/* ---------------------------------------------------------------------
 Function~  ICE::advectQFirst--ADVECTION:
 Purpose~   Calculate the advection of q_CC 
   
 References:
    "Compatible Fluxes for van Leer Advection" W.B VanderHeyden and 
    B.A. Kashiwa, Journal of Computational Physics, 146, 1-28, (1998) 
            
 Steps for each cell:      
- Compute q outflux and q influx for each cell.
- Finally sum the influx and outflux portions
       
 Programmer         Date       Description
 ----------         ----       -----------
  Jim Guilkey      10/04/00    

 advect_preprocessing MUST be done prior to this function
 ---------------------------------------------------------------------  */
void ICE::advectQFirst(
    const CCVariable<double>&   q_CC,
    const Patch*                patch,
    const CCVariable<fflux>&    OFS,
    const CCVariable<eflux>&    OFE,
    const CCVariable<fflux>&    IFS,
    const CCVariable<eflux>&    IFE,
    CCVariable<fflux>&          q_out,
    CCVariable<eflux>&          q_out_EF,
    CCVariable<fflux>&          q_in,
    CCVariable<eflux>&          q_in_EF,
    CCVariable<double>&         q_advected)

{
    double  sum_q_outflux,
            sum_q_outflux_EF,
            sum_q_influx,
            sum_q_influx_EF;
//__________________________________
// Determine the influx and outflux of q at each cell
  qOutfluxFirst(    q_CC,       patch,      q_out,      q_out_EF);
  
  qInflux(          q_out,      q_out_EF,   patch,      q_in,       q_in_EF);
  
//__________________________________
//  - Loop over each cell 
  for(CellIterator iter = patch->getCellIterator(); !iter.done(); iter++)
  {
    sum_q_outflux       = 0.0;
    sum_q_outflux_EF    = 0.0;
    sum_q_influx        = 0.0;
    sum_q_influx_EF     = 0.0;
    
    //__________________________________
    //  OUTFLUX: SLAB 
    for(int face = TOP; face <= BACK; face++ ) 
    {
       sum_q_outflux  += q_out[*iter].d_fflux[face] * OFS[*iter].d_fflux[face];
    }
    //__________________________________
    //  OUTFLUX: EDGE_FLUX
    for(int edge = TR; edge <= bL; edge++ ) 
    {
       sum_q_outflux_EF += q_out_EF[*iter].d_eflux[edge] * OFE[*iter].d_eflux[edge];
    }
                //__________________________________
                //  OUTFLUX: CORNER FLUX   not done  
    //__________________________________
    //  INFLUX: SLABS
    for(int face = TOP; face <= BACK; face++ ) 
    {
       sum_q_influx  += q_in[*iter].d_fflux[face] * IFS[*iter].d_fflux[face];
    }
    //__________________________________
    //  INFLUX: EDGES
    for(int edge = TR; edge <= bL; edge++ )
    {
       sum_q_influx_EF += q_in_EF[*iter].d_eflux[edge] * IFE[*iter].d_eflux[edge];
    }
                //__________________________________
                //   INFLUX: CORNER FLUX  not done 
    //__________________________________
    //  Calculate the advected q at t + delta t
    q_advected[*iter] = - sum_q_outflux - sum_q_outflux_EF
                        + sum_q_influx  + sum_q_influx_EF;
#ifdef john_debug
    cout << setprecision(16)<<"sum_q_outflux = " << sum_q_outflux << " sum_q_outflux_EF = " <<
      sum_q_outflux_EF << " sum_q_influx = " << sum_q_influx << 
      " sum_q_influx_EF = " << sum_q_influx_EF << " q_advect"<<*iter<<
      "=" << q_advected[*iter] << endl;
#endif
  }

}
//STOP_DOC
/*---------------------------------------------------------------------
 Function~  ICE::qOutfluxFirst-- 
 Purpose~  Calculate the quantity \langle q \rangle for each outflux, including
    the corner flux terms

 References:
    "Compatible Fluxes for van Leer Advection" W.B VanderHeyden and 
    B.A. Kashiwa, Journal of Computational Physics, 
    146, 1-28, (1998) 
            
 Steps for each cell:  
 --------------------        
    Calculate the quantity outflux of q for each of the outflowing volumes 
       
Programmer         Date       Description
----------         ----       -----------
Jim Guilkey        10/04/00 
 
 See schematic diagram at bottom of ice.cc
 FIRST ORDER ONLY AT THIS TIME 10/21/00
---------------------------------------------------------------------  */ 
void ICE::qOutfluxFirst(
    const CCVariable<double>&   q_CC,
    const Patch*                patch,
    CCVariable<fflux>&          q_out,
    CCVariable<eflux>&          q_out_EF)
{
    for(CellIterator iter = patch->getExtraCellIterator(); !iter.done(); iter++)
    {
        //__________________________________
        //  SLABS
        for(int face = TOP; face <= BACK; face++ ) 
        {
            q_out[*iter].d_fflux[face] = q_CC[*iter];
        }
        //__________________________________
        //  EDGE fluxes
        for(int edge = TR; edge <= bL; edge++ ) 
        {
           q_out_EF[*iter].d_eflux[edge] = q_CC[*iter];
        }
    }
}
//STOP_DOC
/*---------------------------------------------------------------------
 Function~  ICE::qInflux
 Purpose~
    Calculate the influx contribution \langle q \rangle for each slab and corner
    flux.   
 
 References:
    "Compatible Fluxes for van Leer Advection" W.B VanderHeyden 
    and B.A. Kashiwa, Journal of Computational Physics, 146, 1-28, (1998) 
              
Implementation Notes:
    The quantity q_outflux is needed from one layer of extra cells surrounding
    the computational domain.
    
  Programmer         Date       Description
  ----------         ----       -----------
Jim Guilkey         10/04/00
See schematic diagram at bottom of file ice.cc
---------------------------------------------------------------------  */
void ICE::qInflux(
    const CCVariable<fflux>&    q_out,
    const CCVariable<eflux>&    q_out_EF,
    const Patch*                patch,
    CCVariable<fflux>&          q_in,
    CCVariable<eflux>&          q_in_EF)

{
  for(CellIterator iter = patch->getCellIterator(); !iter.done(); iter++)
  {
    IntVector curcell = *iter,adjcell;
    //__________________________________
    //  q_influx SLABS
    adjcell = IntVector(curcell.x(),curcell.y()+1,curcell.z());
    q_in[*iter].d_fflux[TOP]    = q_out[adjcell].d_fflux[BOTTOM];
    adjcell = IntVector(curcell.x(),curcell.y()-1,curcell.z());
    q_in[*iter].d_fflux[BOTTOM] = q_out[adjcell].d_fflux[TOP];

    adjcell = IntVector(curcell.x()+1,curcell.y(),curcell.z());
    q_in[*iter].d_fflux[RIGHT]  = q_out[adjcell].d_fflux[LEFT];
    adjcell = IntVector(curcell.x()-1,curcell.y(),curcell.z());
    q_in[*iter].d_fflux[LEFT]   = q_out[adjcell].d_fflux[RIGHT];

    adjcell = IntVector(curcell.x(),curcell.y(),curcell.z()+1);
    q_in[*iter].d_fflux[FRONT]  = q_out[adjcell].d_fflux[BACK];
    adjcell = IntVector(curcell.x(),curcell.y(),curcell.z()-1);
    q_in[*iter].d_fflux[BACK]   = q_out[adjcell].d_fflux[FRONT];

    //__________________________________
    //  q_influx EDGES
    adjcell = IntVector(curcell.x()+1,curcell.y()+1,curcell.z());
    q_in_EF[*iter].d_eflux[TR]  = q_out_EF[adjcell].d_eflux[BL];

    adjcell = IntVector(curcell.x()+1,curcell.y()-1,curcell.z());
    q_in_EF[*iter].d_eflux[BR]  = q_out_EF[adjcell].d_eflux[TL];

    adjcell = IntVector(curcell.x()-1,curcell.y()+1,curcell.z());
    q_in_EF[*iter].d_eflux[TL]  = q_out_EF[adjcell].d_eflux[BR];

    adjcell = IntVector(curcell.x()-1,curcell.y()-1,curcell.z());
    q_in_EF[*iter].d_eflux[BL]  = q_out_EF[adjcell].d_eflux[TR];
  }

}
//STOP_DOC

#if 0
/*__________________________________
*   ONLY NEEDED BY SECOND ORDER ADVECTION
*___________________________________*/
void ICE::outflowVolCentroid(const SFCXVariable<double>& uvel_FC,
                             const SFCYVariable<double>& vvel_FC,
                             const SFCZVariable<double>& wvel_FC,
                             const double& delT, const Vector& dx,
                             CCVariable<fflux>& r_out_x,
                             CCVariable<fflux>& r_out_y,
                             CCVariable<fflux>& r_out_z,
                             CCVariable<eflux>& r_out_x_CF,
                             CCVariable<eflux>& r_out_y_CF,
                             CCVariable<eflux>& r_out_z_CF)

{

}

void ICE::qOutfluxSecond(CCVariable<fflux>& OFS,
                         CCVariable<fflux>& IFS,
                         CCVariable<fflux>& r_out_x,
                         CCVariable<fflux>& r_out_y,
                         CCVariable<fflux>& r_out_z,
                         CCVariable<eflux>& r_out_x_CF,
                         CCVariable<eflux>& r_out_y_CF,
                         CCVariable<eflux>& r_out_z_CF,
                         const Vector& dx)
{

}
#endif
/*______________________________________________________________________
*    Machinery to allow MPI to pass those structs of eflux and fflux around.
*_______________________________________________________________________*/
#ifdef __sgi
#define IRIX
#pragma set woff 1209
#endif

namespace Uintah {
   namespace ICESpace {


static MPI_Datatype makeMPI_fflux()
{
   ASSERTEQ(sizeof(ICE::fflux), sizeof(double)*6);
   MPI_Datatype mpitype;
   MPI_Type_vector(1, 6, 6, MPI_DOUBLE, &mpitype);
   MPI_Type_commit(&mpitype);
   return mpitype;
}

const TypeDescription* fun_getTypeDescription(ICE::fflux*)
{
   static TypeDescription* td = 0;
   if(!td){
      td = scinew TypeDescription(TypeDescription::Other,
                               "ICE::fflux", true, &makeMPI_fflux);
   }
   return td;
}

static MPI_Datatype makeMPI_eflux()
{
   ASSERTEQ(sizeof(ICE::eflux), sizeof(double)*12);
   MPI_Datatype mpitype;
   MPI_Type_vector(1, 12, 12, MPI_DOUBLE, &mpitype);
   MPI_Type_commit(&mpitype);
   return mpitype;
}

const TypeDescription* fun_getTypeDescription(ICE::eflux*)
{
   static TypeDescription* td = 0;
   if(!td){
      td = scinew TypeDescription(TypeDescription::Other,
                               "ICE::eflux", true, &makeMPI_eflux);
   }
   return td;
}


}
}

//______________________________________________________________________
//      Put this in seperate function  This is mainly for debugging
//      incompressible code
#if 0


    task->computes( new_dw, lb->div_velfc_CCLabel, dwindex,patch);

      double top, bottom, front, back, right, left;
      // Create variables for the divergence of the FC velocity
      CCVariable<double> div_velfc_CC;
      new_dw->allocate(div_velfc_CC, lb->div_velfc_CCLabel, dwindex, patch);

      for(CellIterator iter = patch->getCellIterator(); !iter.done(); iter++)
      {
#ifdef john_debug
	cout << "top face velocity = " << vvel_FC[*iter+IntVector(0,1,0)] 
	     << endl;
	cout << "bottom face velocity = " << vvel_FC[*iter+IntVector(0,0,0)] 
	     << endl;
	cout << "left face velocity = " << uvel_FC[*iter+IntVector(1,0,0)] 
	     << endl;
	cout << "right face velocity = " << uvel_FC[*iter+IntVector(0,0,0)] 
	     << endl;
	cout << "front face velocity = " << wvel_FC[*iter+IntVector(0,0,1)] 
	     << endl;
	cout << "back face velocity = " << wvel_FC[*iter+IntVector(0,0,0)] 
	     << endl;
#endif
	top      =  dx.x()*dx.z()* vvel_FC[*iter+IntVector(0,1,0)];
	bottom   = -dx.x()*dx.z()* vvel_FC[*iter+IntVector(0,0,0)];
	left     = -dx.y()*dx.z()* uvel_FC[*iter+IntVector(0,0,0)];
	right    =  dx.y()*dx.z()* uvel_FC[*iter+IntVector(1,0,0)];
	front    =  dx.x()*dx.y()* wvel_FC[*iter+IntVector(0,0,1)];
	back     = -dx.x()*dx.y()* wvel_FC[*iter+IntVector(0,0,0)];
#ifdef john_debug
	cout << "top = " << top << " bottom = " << bottom << " left = " 
	     << left << " right = " << right << " front = " << front 
	     << " back = " << back << " vol_frac = " << vol_frac[*iter] << 
	  endl;
#endif
	div_velfc_CC[*iter] = vol_frac[*iter]*
			     (top + bottom + left + right + front  + back );
      }
      
    new_dw->put(div_velfc_CC, lb->div_velfc_CCLabel, dwindex, patch);

#endif


/*______________________________________________________________________
          S H E M A T I C   D I A G R A M S

                                    q_outflux(TOP)

                                        |    (I/O)flux_EF(TOP_BK)
                                        |
  (I/O)flux_CF(TOP_L_BK)       _________|___________
                              /___/_____|_______/__/|   (I/O)flux_CF(TOP_R_BK)
                             /   /      |      /  | |
                            /   /       |     /  /| |
  (I/O)flux_EF(TOP_L)      /   /             /  / |/|
                          /___/_____________/__/ ------ (I/O)flux_EF(TOP_R)
                        _/__ /_____________/__/| /| | 
                        |   |             |  | |/ | |   (I/O)flux_EF(BCK_R)
                        | + |      +      | +| /  | |      
                        |---|----------------|/|  |/| 
                        |   |             |  | | /| /  (I/O)flux_CF(BOT_R_BK)
  (I/O)flux(LEFT_FR)    | + |     i,j,k   | +| |/ /          
                        |   |             |  |/| /   (I/O)flux_EF(BOT_R)
                        |---|----------------| |/
  (I/O)flux_CF(BOT_L_FR)| + |      +      | +|/    (I/O)flux_CF(BOT_R_FR)
                        ---------------------- 
                         (I/O)flux_EF(BOT_FR)       
                         
                                         
                         
                            (TOP)      
   ______________________              ______________________  _
   |   |             |  |              |   |             |  |  |  delY_top
   | + |      +      | +|              | + |      +      | +|  |
   |---|----------------|  --ytop      |---|----------------|  -
   |   |             |  |              |   |             |  |
   | + |     i,j,k   | +| (RIGHT)      | + |     i,j,k   | +|
   |   |             |  |              |   |             |  |
   |---|----------------|  --y0        |---|----------------|  -
   | + |      +      | +|              | + |      +      | +|  | delY_bottom
   ----------------------              ----------------------  -
       |             |                 |---|             |--|
       x0            xright              delX_left         delX_right
       
                            (BACK)
   ______________________              ______________________  _
   |   |             |  |              |   |             |  |  |  delZ_back
   | + |      +      | +|              | + |      +      | +|  |
   |---|----------------|  --z0        |---|----------------|  -
   |   |             |  |              |   |             |  |
   | + |     i,j,k   | +| (RIGHT)      | + |     i,j,k   | +|
   |   |             |  |              |   |             |  |
   |---|----------------|  --z_frt     |---|----------------|  -
   | + |      +      | +|              | + |      +      | +|  | delZ_front
   ----------------------              ----------------------  -
       |             |                 |---|             |--|
       x0            xright              delX_left         delX_right
                         
______________________________________________________________________*/                        

//
// $Log$
// Revision 1.69  2000/12/29 17:52:48  harman
// - removed div_vel_fc calculation from delpress calculation
// - changed how press_FC is being calculated
// - get press_CC from new_dw instead of old_dw in step 1c
// - changed convergence criteria in step1b (equilibration pressure)
//
// Revision 1.65  2000/12/20 00:30:56  jas
// Added john_debug to get rid of all debugging output.
//
// Revision 1.64  2000/12/18 23:25:55  jas
// 2d ice works for simple advection.
//
// Revision 1.63  2000/12/05 20:56:56  jas
// Fixes for step3.  Put in extraCellIterator and guards around adjacent cells.
//
// Revision 1.62  2000/12/05 20:45:49  jas
// Iterate over all the cells in influxOutfluxVolume.  Now step 2 is working.
//
// Revision 1.61  2000/12/05 15:45:29  jas
// Now using SFC{X,Y,Z} data types.  Fixed some small bugs and things appear
// to be working up to the middle of step 2.
//
// Revision 1.60  2000/11/28 03:50:28  jas
// Added {X,Y,Z}FCVariables.  Things still don't work yet!
//
// Revision 1.59  2000/11/22 01:28:05  guilkey
// Changed the way initial conditions are set.  GeometryObjects are created
// to fill the volume of the domain.  Each object has appropriate initial
// conditions associated with it.  ICEMaterial now has an initializeCells
// method, which for now just does what was previously done with the
// initial condition stuct d_ic.  This will be extended to allow regions of
// the domain to be initialized with different materials.  Sorry for the
// lame GeometryObject2, this could be changed to ICEGeometryObject or
// something.
//
// Revision 1.58  2000/11/21 21:52:27  jas
// Simplified scheduleTimeAdvance now is a bunch of functions.  More
// implementation of FC variables.
//
// Revision 1.57  2000/11/15 00:51:54  guilkey
// Changed code to take advantage of the ICEMaterial stuff I committed
// recently in preparation for coupling the two codes.
//
// Revision 1.56  2000/11/14 04:02:11  jas
// Added getExtraCellIterator and things now appear to be working up to
// face centered velocity calculations.
//
// Revision 1.55  2000/11/02 21:33:05  jas
// Added new bc implementation.  Things now work thru step 1b.  Neumann bcs
// are now set correctly.
//
// Revision 1.54  2000/10/31 04:16:17  jas
// Fixed some errors in speed of sound and equilibration pressure calculation.
// Added initial conditions.
//
// Revision 1.53  2000/10/27 23:41:01  jas
// Added more material constants and some debugging output.
//
// Revision 1.52  2000/10/26 23:22:09  jas
// BCs are now implemented.
//
// Revision 1.51  2000/10/26 00:52:54  guilkey
// Work on step4b
//
// Revision 1.50  2000/10/26 00:24:46  guilkey
// Made all pressures belong to material 0.  Implemented step4b.
//
// Revision 1.49  2000/10/25 23:12:17  guilkey
// Fixed step2, reorganized 6and7 just a little bit.
//
// Revision 1.48  2000/10/25 22:22:13  jas
// Change the fflux and eflux struct so that the data members begin with d_.
// This makes g++ happy.
//
// Revision 1.47  2000/10/25 21:15:31  guilkey
// Finished advection
//
// Revision 1.46  2000/10/24 23:07:21  guilkey
// Added code for steps6and7.
//
// Revision 1.45  2000/10/20 23:58:55  guilkey
// Added part of advection code.
//
// Revision 1.44  2000/10/19 02:44:52  guilkey
// Added code for step5b.
//
// Revision 1.43  2000/10/18 21:02:17  guilkey
// Added code for steps 4 and 5.
//
// Revision 1.42  2000/10/18 03:57:22  jas
// Don't print out bc values.
//
// Revision 1.41  2000/10/18 03:43:01  jas
// Implemented pressure boundary conditions during equilibration computation (1b).
//
// Revision 1.40  2000/10/17 23:05:15  guilkey
// Fixed some computes and requires.
//
// Revision 1.39  2000/10/17 20:26:20  jas
// Changed press to press_new.
//
// Revision 1.38  2000/10/17 18:35:20  guilkey
// Added some computes to actuallyInitialize.
//
// Revision 1.37  2000/10/17 04:33:35  jas
// Copied grid bcs into ice for initial testing.
//
// Revision 1.36  2000/10/17 04:13:25  jas
// Implement hydrostatic pressure adjustment as part of step 1b.  Still need
// to implement update bcs.
//
// Revision 1.35  2000/10/16 20:31:00  guilkey
// Step3 added
//
// Revision 1.34  2000/10/16 19:10:34  guilkey
// Combined step1e with step2 and eliminated step1e.
//
// Revision 1.33  2000/10/16 18:32:40  guilkey
// Implemented "step1e" of the ICE algorithm.
//
// Revision 1.32  2000/10/16 17:19:44  guilkey
// Code for ICE::step1d.  Only code for one of the faces is committed
// until things become more concrete.
//
// Revision 1.31  2000/10/14 02:49:46  jas
// Added implementation of compute equilibration pressure.  Still need to do
// the update of BCS and hydrostatic pressure.  Still some issues with
// computes and requires - will compile but won't run.
//
// Revision 1.30  2000/10/13 00:01:11  guilkey
// More work on ICE
//
// Revision 1.29  2000/10/11 00:15:50  jas
// Sketched out the compute equilibration pressure.
//
// Revision 1.28  2000/10/10 20:35:07  jas
// Move some stuff around.
//
// Revision 1.27  2000/10/09 22:37:01  jas
// Cleaned up labels and added more computes and requires for EOS.
//
// Revision 1.25  2000/10/05 04:26:48  guilkey
// Added code for part of the EOS evaluation.
//
// Revision 1.24  2000/10/05 00:16:33  jas
// Starting to work on the speed of sound stuff.
//
// Revision 1.23  2000/10/04 23:38:21  jas
// All of the steps are in place with just dummy functions.  delT is
// hardwired in for the moment so that we can actually do multiple
// time steps with empty functions.
//
// Revision 1.22  2000/10/04 20:15:27  jas
// Start to bring ICE into UCF.
//
