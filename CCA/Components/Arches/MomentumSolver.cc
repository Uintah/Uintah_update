//----- MomentumSolver.cc ----------------------------------------------

#include <TauProfilerForSCIRun.h>
#include <Packages/Uintah/CCA/Components/Arches/Arches.h>
#include <Packages/Uintah/CCA/Components/Arches/ArchesLabel.h>
#include <Packages/Uintah/CCA/Components/Arches/ArchesMaterial.h>
#include <Packages/Uintah/CCA/Components/Arches/BoundaryCondition.h>
#include <Packages/Uintah/CCA/Components/Arches/CellInformationP.h>
#include <Packages/Uintah/CCA/Components/Arches/debug.h>
#include <Packages/Uintah/CCA/Components/Arches/MomentumSolver.h>
#include <Packages/Uintah/CCA/Components/Arches/PhysicalConstants.h>
#include <Packages/Uintah/CCA/Components/Arches/RBGSSolver.h>
#include <Packages/Uintah/CCA/Components/Arches/Source.h>
#include <Packages/Uintah/CCA/Components/Arches/TurbulenceModel.h>
#include <Packages/Uintah/CCA/Components/Arches/ScaleSimilarityModel.h>
#include <Packages/Uintah/CCA/Components/Arches/TimeIntegratorLabel.h>
#include <Packages/Uintah/CCA/Components/MPMArches/MPMArchesLabel.h>
#include <Packages/Uintah/CCA/Ports/DataWarehouse.h>
#include <Packages/Uintah/CCA/Ports/Scheduler.h>
#include <Packages/Uintah/Core/Exceptions/InvalidValue.h>
#include <Packages/Uintah/Core/Grid/CCVariable.h>
#include <Packages/Uintah/Core/Grid/CellIterator.h>
#include <Packages/Uintah/Core/Grid/Level.h>
#include <Packages/Uintah/Core/Grid/PerPatch.h>
#include <Packages/Uintah/Core/Grid/SFCXVariable.h>
#include <Packages/Uintah/Core/Grid/SFCYVariable.h>
#include <Packages/Uintah/Core/Grid/SFCZVariable.h>
#include <Packages/Uintah/Core/Grid/SimulationState.h>
#include <Packages/Uintah/Core/Grid/Task.h>
#include <Packages/Uintah/Core/Grid/VarTypes.h>
#include <Packages/Uintah/Core/ProblemSpec/ProblemSpec.h>
#include <Core/Math/MiscMath.h>


using namespace Uintah;
using namespace std;
#include <Packages/Uintah/CCA/Components/Arches/fortran/computeVel_fort.h>

//****************************************************************************
// Default constructor for MomentumSolver
//****************************************************************************
MomentumSolver::
MomentumSolver(const ArchesLabel* label, const MPMArchesLabel* MAlb,
	       TurbulenceModel* turb_model,
	       BoundaryCondition* bndry_cond,
	       PhysicalConstants* physConst) : 
                                   d_lab(label), d_MAlab(MAlb),
                                   d_turbModel(turb_model), 
                                   d_boundaryCondition(bndry_cond),
				   d_physicalConsts(physConst)
{
  d_discretize = 0;
  d_source = 0;
  d_linearSolver = 0;
}

//****************************************************************************
// Destructor
//****************************************************************************
MomentumSolver::~MomentumSolver()
{
  delete d_discretize;
  delete d_source;
  delete d_linearSolver;
}

//****************************************************************************
// Problem Setup 
//****************************************************************************
void 
MomentumSolver::problemSetup(const ProblemSpecP& params)
{
  ProblemSpecP db = params->findBlock("MomentumSolver");
  string finite_diff;
  db->require("finite_difference", finite_diff);
  if (finite_diff == "second") 
    d_discretize = scinew Discretization();
  else {
    throw InvalidValue("Finite Differencing scheme "
		       "not supported: " + finite_diff);
  }
  db->getWithDefault("central",d_central,false);
//  if (db->findBlock("central"))
//    db->require("central",d_central);
//  else
//    d_central = false;
  // make source and boundary_condition objects
  db->getWithDefault("pressure_correction",d_pressure_correction,false);

  d_source = scinew Source(d_turbModel, d_physicalConsts);

  string linear_sol;
  db->require("linear_solver", linear_sol);
  if (linear_sol == "linegs")

    d_linearSolver = scinew RBGSSolver();

  else {

    throw InvalidValue("linear solver option"
		       " not supported" + linear_sol);

  }

  d_linearSolver->problemSetup(db);

}

//****************************************************************************
// Schedule linear momentum solve
//****************************************************************************
void 
MomentumSolver::solve(SchedulerP& sched,
		      const PatchSet* patches,
		      const MaterialSet* matls,
		      const TimeIntegratorLabel* timelabels,
		      int index)
{
  //computes stencil coefficients and source terms
  // require : pressureCPBC, [u,v,w]VelocityCPBC, densityIN, viscosityIN (new_dw)
  //           [u,v,w]SPBC, densityCP (old_dw)
  // compute : [u,v,w]VelCoefPBLM, [u,v,w]VelConvCoefPBLM
  //           [u,v,w]VelLinSrcPBLM, [u,v,w]VelNonLinSrcPBLM

  sched_buildLinearMatrix(sched, patches, matls, timelabels, index);
    

}

//****************************************************************************
// Schedule the build of the linear matrix
//****************************************************************************
void 
MomentumSolver::sched_buildLinearMatrix(SchedulerP& sched,
					const PatchSet* patches,
					const MaterialSet* matls,
		    			const TimeIntegratorLabel* timelabels,
					int index)
{
  string taskname =  "MomentumSolver::BuildCoeff" +
		     timelabels->integrator_step_name;
  Task* tsk = scinew Task(taskname,
			  this, &MomentumSolver::buildLinearMatrix,
			  timelabels, index);

  tsk->requires(Task::OldDW, d_lab->d_sharedState->get_delt_label());

  tsk->requires(Task::NewDW, d_lab->d_cellTypeLabel, 
		  Ghost::AroundCells, Arches::ONEGHOSTCELL);
  
  tsk->requires(Task::NewDW, timelabels->density_out,
		Ghost::AroundCells, Arches::ONEGHOSTCELL);
  tsk->requires(Task::NewDW, timelabels->pressure_out,
		Ghost::AroundCells, Arches::ONEGHOSTCELL);

  if (d_MAlab) {
    tsk->requires(Task::NewDW, d_lab->d_mmgasVolFracLabel,
		  Ghost::AroundCells, Arches::ONEGHOSTCELL);
  }

  switch (index) {

  case Arches::XDIR:

    tsk->requires(Task::NewDW, timelabels->uvelhat_out,
		  Ghost::AroundFaces, Arches::ONEGHOSTCELL);
    tsk->computes(timelabels->uvelocity_out);

    tsk->computes(timelabels->maxabsu_out);

    break;

  case Arches::YDIR:

    // use new uvelocity for v coef calculation
    
    tsk->requires(Task::NewDW, timelabels->vvelhat_out, 
		  Ghost::AroundFaces, Arches::ONEGHOSTCELL);
    tsk->computes(timelabels->vvelocity_out);

    tsk->computes(timelabels->maxabsv_out);

    break;

  case Arches::ZDIR:

    // use new uvelocity for v coef calculation

    tsk->requires(Task::NewDW, timelabels->wvelhat_out, 
		  Ghost::AroundFaces, Arches::ONEGHOSTCELL);
    tsk->computes(timelabels->wvelocity_out);

    tsk->computes(timelabels->maxabsw_out);

    break;

  default:

    throw InvalidValue("Invalid index in MomentumSolver");
    
  }
	
  sched->addTask(tsk, patches, matls);


}

//****************************************************************************
// Actual build of the linear matrix
//****************************************************************************
void 
MomentumSolver::buildLinearMatrix(const ProcessorGroup* pc,
				  const PatchSubset* patches,
				  const MaterialSubset* /*matls*/,
				  DataWarehouse* old_dw,
				  DataWarehouse* new_dw,
		    		  const TimeIntegratorLabel* timelabels,
				  int index)
{
  delt_vartype delT;
  old_dw->get(delT, d_lab->d_sharedState->get_delt_label() );
  double delta_t = delT;
  delta_t *= timelabels->time_multiplier;

  for (int p = 0; p < patches->size(); p++) {

    const Patch* patch = patches->get(p);
    int archIndex = 0; // only one arches material
    int matlIndex = d_lab->d_sharedState->getArchesMaterial(archIndex)->getDWIndex(); 
    ArchesVariables velocityVars;
    ArchesConstVariables constVelocityVars;

    // Get the required data
    new_dw->get(constVelocityVars.cellType, d_lab->d_cellTypeLabel,
		matlIndex, patch, Ghost::AroundCells, Arches::ONEGHOSTCELL);

    new_dw->get(constVelocityVars.pressure, timelabels->pressure_out, 
		matlIndex, patch, Ghost::AroundCells, Arches::ONEGHOSTCELL);
    new_dw->get(constVelocityVars.density, timelabels->density_out, 
		matlIndex, patch, Ghost::AroundCells, Arches::ONEGHOSTCELL);

    // Get the PerPatch CellInformation data
    PerPatch<CellInformationP> cellInfoP;

    if (new_dw->exists(d_lab->d_cellInfoLabel, matlIndex, patch)) 
      new_dw->get(cellInfoP, d_lab->d_cellInfoLabel, matlIndex, patch);
    else {
      cellInfoP.setData(scinew CellInformation(patch));
      new_dw->put(cellInfoP, d_lab->d_cellInfoLabel, matlIndex, patch);
    }

    CellInformation* cellinfo = cellInfoP.get().get_rep();

    if (d_MAlab) {
      new_dw->get(constVelocityVars.voidFraction, d_lab->d_mmgasVolFracLabel,
		      matlIndex, patch, 
		      Ghost::AroundCells, Arches::ONEGHOSTCELL);
    }
    
    switch (index) {

    case Arches::XDIR:

      new_dw->allocateAndPut(velocityVars.uVelRhoHat, timelabels->uvelocity_out,
		             matlIndex, patch,
			     Ghost::AroundFaces, Arches::ONEGHOSTCELL);
      new_dw->copyOut(velocityVars.uVelRhoHat, timelabels->uvelhat_out, 
		  matlIndex, patch, Ghost::AroundFaces, Arches::ONEGHOSTCELL);

      break;

    case Arches::YDIR:

      new_dw->allocateAndPut(velocityVars.vVelRhoHat, timelabels->vvelocity_out,
		             matlIndex, patch,
			     Ghost::AroundFaces, Arches::ONEGHOSTCELL);
      new_dw->copyOut(velocityVars.vVelRhoHat, timelabels->vvelhat_out, 
		  matlIndex, patch, Ghost::AroundFaces, Arches::ONEGHOSTCELL);

      break;

    case Arches::ZDIR:

      new_dw->allocateAndPut(velocityVars.wVelRhoHat, timelabels->wvelocity_out,
		             matlIndex, patch,
			     Ghost::AroundFaces, Arches::ONEGHOSTCELL);
      new_dw->copyOut(velocityVars.wVelRhoHat, timelabels->wvelhat_out, 
		  matlIndex, patch, Ghost::AroundFaces, Arches::ONEGHOSTCELL);

      break;

    default:

      throw InvalidValue("Invalid index in MomentumSolver");

    }

#ifdef ARCHES_MOM_DEBUG
    cerr << "momentum solver: pressure=\n";
    velocityVars.pressure.print(cerr);
    if (patch->containsCell(IntVector(2,3,3))) {
      cerr << "[2,3,3] press[2,3,3]" << velocityVars.pressure[IntVector(2,3,3)] 
	   << " " << velocityVars.pressure[IntVector(1,3,3)] << endl;
    }
#endif
    
    // Actual compute operations

    if (d_MAlab) {
      
      d_boundaryCondition->calculateVelocityPred_mm(pc, patch, 
						    delta_t, index, cellinfo,
						    &velocityVars,
						    &constVelocityVars);

    }
    else {
    
      d_source->calculateVelocityPred(pc, patch, 
				      delta_t, index,
				      cellinfo, &velocityVars,
				      &constVelocityVars);

      if (d_boundaryCondition->getIntrusionBC())
	d_boundaryCondition->calculateIntrusionVel(pc, patch,
						   index, cellinfo,
						   &velocityVars,
						   &constVelocityVars);
    }
    d_boundaryCondition->velocityPressureBC(pc, patch, index, cellinfo,
					    &velocityVars, &constVelocityVars);
    d_boundaryCondition->addPresGradVelocityOutletBC(pc, patch, index, cellinfo,
						     delta_t, &velocityVars,
						     &constVelocityVars);

    double maxAbsU = 0.0;
    double maxAbsV = 0.0;
    double maxAbsW = 0.0;
    double temp_absU, temp_absV, temp_absW;
    IntVector ixLow;
    IntVector ixHigh;
    
    switch (index) {
    case Arches::XDIR:

      ixLow = patch->getSFCXFORTLowIndex();
      ixHigh = patch->getSFCXFORTHighIndex();
    
      for (int colZ = ixLow.z(); colZ <= ixHigh.z(); colZ ++) {
        for (int colY = ixLow.y(); colY <= ixHigh.y(); colY ++) {
          for (int colX = ixLow.x(); colX <= ixHigh.x(); colX ++) {

              IntVector currCell(colX, colY, colZ);

	      temp_absU = Abs(velocityVars.uVelRhoHat[currCell]);
	      if (temp_absU > maxAbsU) maxAbsU = temp_absU;
          }
        }
      }
      new_dw->put(max_vartype(maxAbsU), timelabels->maxabsu_out); 

      break;
    case Arches::YDIR:

      ixLow = patch->getSFCYFORTLowIndex();
      ixHigh = patch->getSFCYFORTHighIndex();
    
      for (int colZ = ixLow.z(); colZ <= ixHigh.z(); colZ ++) {
        for (int colY = ixLow.y(); colY <= ixHigh.y(); colY ++) {
          for (int colX = ixLow.x(); colX <= ixHigh.x(); colX ++) {

              IntVector currCell(colX, colY, colZ);

	      temp_absV = Abs(velocityVars.vVelRhoHat[currCell]);
	      if (temp_absV > maxAbsV) maxAbsV = temp_absV;
          }
        }
      }
      new_dw->put(max_vartype(maxAbsV), timelabels->maxabsv_out); 

      break;
    case Arches::ZDIR:

      ixLow = patch->getSFCZFORTLowIndex();
      ixHigh = patch->getSFCZFORTHighIndex();
    
      for (int colZ = ixLow.z(); colZ <= ixHigh.z(); colZ ++) {
        for (int colY = ixLow.y(); colY <= ixHigh.y(); colY ++) {
          for (int colX = ixLow.x(); colX <= ixHigh.x(); colX ++) {

              IntVector currCell(colX, colY, colZ);

	      temp_absW = Abs(velocityVars.wVelRhoHat[currCell]);
	      if (temp_absW > maxAbsW) maxAbsW = temp_absW;
          }
        }
      }
      new_dw->put(max_vartype(maxAbsW), timelabels->maxabsw_out); 

      break;
    default:
      throw InvalidValue("Invalid index in max abs velocity calculation");
    }
  }
}

//****************************************************************************
// Schedule calculation of hat velocities
//****************************************************************************
void MomentumSolver::solveVelHat(const LevelP& level,
				 SchedulerP& sched,
		    		 const TimeIntegratorLabel* timelabels)
{
  const PatchSet* patches = level->eachPatch();
  const MaterialSet* matls = d_lab->d_sharedState->allArchesMaterials();

#ifdef filter_convection_terms
  sched_computeNonlinearTerms(sched, patches, matls, d_lab, timelabels);
#endif

  sched_buildLinearMatrixVelHat(sched, patches, matls, timelabels);

}



// ****************************************************************************
// Schedule build of linear matrix
// ****************************************************************************
void 
MomentumSolver::sched_buildLinearMatrixVelHat(SchedulerP& sched,
					      const PatchSet* patches,
					      const MaterialSet* matls,
					  const TimeIntegratorLabel* timelabels)
{
  string taskname =  "MomentumSolver::BuildCoeffVelHat" +
		     timelabels->integrator_step_name;
  Task* tsk = scinew Task(taskname, 
			  this, &MomentumSolver::buildLinearMatrixVelHat,
			  timelabels);


  tsk->requires(Task::OldDW, d_lab->d_sharedState->get_delt_label());
    
  // Requires
  // from old_dw for time integration
  // get old_dw from getTop function

  tsk->requires(Task::NewDW, d_lab->d_cellTypeLabel,
		Ghost::AroundCells, Arches::TWOGHOSTCELLS);

  tsk->requires(Task::NewDW, timelabels->density_in,
		Ghost::AroundCells, Arches::TWOGHOSTCELLS);
  tsk->requires(Task::NewDW, timelabels->old_density_in,
		Ghost::AroundCells, Arches::ONEGHOSTCELL);
  tsk->requires(Task::NewDW, timelabels->density_out, 
		Ghost::AroundCells, Arches::ONEGHOSTCELL);
  tsk->requires(Task::NewDW, d_lab->d_denRefArrayLabel,
		Ghost::AroundCells, Arches::ONEGHOSTCELL);
  tsk->requires(Task::NewDW, timelabels->viscosity_in,
		Ghost::AroundCells, Arches::ONEGHOSTCELL);
  tsk->requires(Task::NewDW, timelabels->uvelocity_in,
		Ghost::AroundFaces, Arches::TWOGHOSTCELLS);
  tsk->requires(Task::NewDW, timelabels->vvelocity_in,
		Ghost::AroundFaces, Arches::TWOGHOSTCELLS);
  tsk->requires(Task::NewDW, timelabels->wvelocity_in,
		Ghost::AroundFaces, Arches::TWOGHOSTCELLS);
  tsk->requires(Task::NewDW, timelabels->old_uvelocity_in,
		Ghost::None, Arches::ZEROGHOSTCELLS);
  tsk->requires(Task::NewDW, timelabels->old_vvelocity_in,
		Ghost::None, Arches::ZEROGHOSTCELLS);
  tsk->requires(Task::NewDW, timelabels->old_wvelocity_in,
		Ghost::None, Arches::ZEROGHOSTCELLS);

  if (d_pressure_correction)
  if (timelabels->integrator_last_step)
    tsk->requires(Task::NewDW, timelabels->pressure_guess, 
		  Ghost::AroundCells, Arches::ONEGHOSTCELL);
  else
    tsk->requires(Task::OldDW, timelabels->pressure_guess, 
		  Ghost::AroundCells, Arches::ONEGHOSTCELL);

  if (timelabels->integrator_step_number == TimeIntegratorStepNumber::First) {
    tsk->requires(Task::OldDW, timelabels->maxabsu_in);
    tsk->requires(Task::OldDW, timelabels->maxabsv_in);
    tsk->requires(Task::OldDW, timelabels->maxabsw_in);
  }
  else {
    tsk->requires(Task::NewDW, timelabels->maxabsu_in);
    tsk->requires(Task::NewDW, timelabels->maxabsv_in);
    tsk->requires(Task::NewDW, timelabels->maxabsw_in);
  }
  // required for computing div constraint
#ifdef divergenceconstraint
  tsk->requires(Task::NewDW, timelabels->scalar_in, 
		Ghost::AroundCells, Arches::ONEGHOSTCELL);
  tsk->requires(Task::NewDW, d_lab->d_drhodfCPLabel, 
		Ghost::None, Arches::ZEROGHOSTCELLS);
  tsk->requires(Task::NewDW, d_lab->d_scalDiffCoefLabel, 
		d_lab->d_stencilMatl, Task::OutOfDomain,
		Ghost::None, Arches::ZEROGHOSTCELLS);
  tsk->requires(Task::NewDW, d_lab->d_scalDiffCoefSrcLabel, 
		Ghost::None, Arches::ZEROGHOSTCELLS);
#endif
#ifdef filter_convection_terms
  tsk->requires(Task::NewDW, d_lab->d_filteredRhoUjULabel,
		d_lab->d_scalarFluxMatl, Task::OutOfDomain,
		Ghost::AroundFaces, Arches::TWOGHOSTCELLS);
  tsk->requires(Task::NewDW, d_lab->d_filteredRhoUjVLabel,
		d_lab->d_scalarFluxMatl, Task::OutOfDomain,
		Ghost::AroundFaces, Arches::TWOGHOSTCELLS);
  tsk->requires(Task::NewDW, d_lab->d_filteredRhoUjWLabel,
		d_lab->d_scalarFluxMatl, Task::OutOfDomain,
		Ghost::AroundFaces, Arches::TWOGHOSTCELLS);
#endif

  if (dynamic_cast<const ScaleSimilarityModel*>(d_turbModel)) 
    if (timelabels->integrator_step_number == TimeIntegratorStepNumber::First)
      tsk->requires(Task::OldDW, d_lab->d_stressTensorCompLabel,
		    d_lab->d_stressTensorMatl,Task::OutOfDomain,
		    Ghost::AroundCells, Arches::ONEGHOSTCELL);
    else
      tsk->requires(Task::NewDW, d_lab->d_stressTensorCompLabel,
		    d_lab->d_stressTensorMatl,Task::OutOfDomain,
		    Ghost::AroundCells, Arches::ONEGHOSTCELL);


    // for multi-material
    // requires su_drag[x,y,z], sp_drag[x,y,z] for arches

  if (d_MAlab) {
    tsk->requires(Task::NewDW, d_MAlab->d_uVel_mmLinSrcLabel,
		  Ghost::None, Arches::ZEROGHOSTCELLS);
    tsk->requires(Task::NewDW, d_MAlab->d_uVel_mmNonlinSrcLabel,
		  Ghost::None, Arches::ZEROGHOSTCELLS);
    tsk->requires(Task::NewDW, d_MAlab->d_vVel_mmLinSrcLabel, 
		  Ghost::None, Arches::ZEROGHOSTCELLS);
    tsk->requires(Task::NewDW, d_MAlab->d_vVel_mmNonlinSrcLabel,
		  Ghost::None, Arches::ZEROGHOSTCELLS);
    tsk->requires(Task::NewDW, d_MAlab->d_wVel_mmLinSrcLabel,
		  Ghost::None, Arches::ZEROGHOSTCELLS);
    tsk->requires(Task::NewDW, d_MAlab->d_wVel_mmNonlinSrcLabel,
		  Ghost::None, Arches::ZEROGHOSTCELLS);
  }

  tsk->computes(timelabels->uvelhat_out);
  tsk->computes(timelabels->vvelhat_out);
  tsk->computes(timelabels->wvelhat_out);
    
#ifdef divergenceconstraint
  if (timelabels->integrator_step_number == TimeIntegratorStepNumber::First)
    tsk->computes(d_lab->d_divConstraintLabel);
  else
    tsk->modifies(d_lab->d_divConstraintLabel);
#endif

  sched->addTask(tsk, patches, matls);
}




// ***********************************************************************
// Actual build of linear matrices for momentum components
// ***********************************************************************

void 
MomentumSolver::buildLinearMatrixVelHat(const ProcessorGroup* pc,
					const PatchSubset* patches,
					const MaterialSubset* /*matls*/,
					DataWarehouse* old_dw,
					DataWarehouse* new_dw,
					const TimeIntegratorLabel* timelabels)
{
  TAU_PROFILE_TIMER(input, "Input", "[MomSolver::buildMVelHatPred::input]" , TAU_USER);
  TAU_PROFILE_TIMER(inputcell, "Inputcell", "[MomSolver::buildMVelHatPred::inputcell]" , TAU_USER);
  TAU_PROFILE_TIMER(compute, "Compute", "[MomSolver::buildMVelHatPred::compute]" , TAU_USER);
  delt_vartype delT;
  old_dw->get(delT, d_lab->d_sharedState->get_delt_label() );
  double delta_t = delT;
  delta_t *= timelabels->time_multiplier;

  double maxAbsU;
  double maxAbsV;
  double maxAbsW;
  max_vartype mxAbsU;
  max_vartype mxAbsV;
  max_vartype mxAbsW;
  if (timelabels->integrator_step_number == TimeIntegratorStepNumber::First) {
    old_dw->get(mxAbsU, timelabels->maxabsu_in);
    old_dw->get(mxAbsV, timelabels->maxabsv_in);
    old_dw->get(mxAbsW, timelabels->maxabsw_in);
  }
  else {
    new_dw->get(mxAbsU, timelabels->maxabsu_in);
    new_dw->get(mxAbsV, timelabels->maxabsv_in);
    new_dw->get(mxAbsW, timelabels->maxabsw_in);
  }
  maxAbsU = mxAbsU;
  maxAbsV = mxAbsW;
  maxAbsW = mxAbsW;

  for (int p = 0; p < patches->size(); p++) {
  TAU_PROFILE_START(input);

    const Patch* patch = patches->get(p);
    int archIndex = 0; // only one arches material
    int matlIndex = d_lab->d_sharedState->getArchesMaterial(archIndex)->getDWIndex(); 
    ArchesVariables velocityVars;
    ArchesConstVariables constVelocityVars;

    new_dw->get(constVelocityVars.cellType, d_lab->d_cellTypeLabel, 
		matlIndex, patch, Ghost::AroundCells, Arches::TWOGHOSTCELLS);

    new_dw->get(constVelocityVars.density, timelabels->density_in, 
		matlIndex, patch, Ghost::AroundCells, Arches::TWOGHOSTCELLS);
    new_dw->get(constVelocityVars.old_density, timelabels->old_density_in, 
		matlIndex, patch, Ghost::AroundCells, Arches::ONEGHOSTCELL);
    new_dw->get(constVelocityVars.new_density, timelabels->density_out, 
		matlIndex, patch, Ghost::AroundCells, Arches::ONEGHOSTCELL);
    new_dw->get(constVelocityVars.denRefArray, d_lab->d_denRefArrayLabel,
    		matlIndex, patch, Ghost::AroundCells, Arches::ONEGHOSTCELL);
    new_dw->get(constVelocityVars.viscosity, timelabels->viscosity_in, 
		matlIndex, patch, Ghost::AroundCells, Arches::ONEGHOSTCELL);
    new_dw->get(constVelocityVars.uVelocity, timelabels->uvelocity_in, 
		matlIndex, patch, Ghost::AroundFaces, Arches::TWOGHOSTCELLS);
    new_dw->get(constVelocityVars.vVelocity, timelabels->vvelocity_in, 
		matlIndex, patch, Ghost::AroundFaces, Arches::TWOGHOSTCELLS);
    new_dw->get(constVelocityVars.wVelocity, timelabels->wvelocity_in, 
		matlIndex, patch, Ghost::AroundFaces, Arches::TWOGHOSTCELLS);
    new_dw->get(constVelocityVars.old_uVelocity, timelabels->old_uvelocity_in, 
		matlIndex, patch, Ghost::None, Arches::ZEROGHOSTCELLS);
    new_dw->get(constVelocityVars.old_vVelocity, timelabels->old_vvelocity_in, 
		matlIndex, patch, Ghost::None, Arches::ZEROGHOSTCELLS);
    new_dw->get(constVelocityVars.old_wVelocity, timelabels->old_wvelocity_in, 
		matlIndex, patch, Ghost::None, Arches::ZEROGHOSTCELLS);

    if (d_pressure_correction)
    if (timelabels->integrator_last_step)
      new_dw->get(constVelocityVars.pressure, timelabels->pressure_guess, 
		  matlIndex, patch, Ghost::AroundCells, Arches::ONEGHOSTCELL);
    else
      old_dw->get(constVelocityVars.pressure, timelabels->pressure_guess, 
		  matlIndex, patch, Ghost::AroundCells, Arches::ONEGHOSTCELL);

#ifdef divergenceconstraint
    new_dw->get(constVelocityVars.scalar, timelabels->scalar_in,
		    matlIndex, patch, Ghost::AroundCells, Arches::ONEGHOSTCELL);
    new_dw->get(constVelocityVars.drhodf, d_lab->d_drhodfCPLabel,
		    matlIndex, patch, Ghost::None, Arches::ZEROGHOSTCELLS);
    for (int ii = 0; ii < d_lab->d_stencilMatl->size(); ii++)
      new_dw->get(constVelocityVars.scalarDiffusionCoeff[ii],
		  d_lab->d_scalDiffCoefLabel,
		  ii, patch, Ghost::None, Arches::ZEROGHOSTCELLS);
    new_dw->get(constVelocityVars.scalarDiffNonlinearSrc, 
	        d_lab->d_scalDiffCoefSrcLabel, matlIndex, patch,
		Ghost::None, Arches::ZEROGHOSTCELLS);
    if (timelabels->integrator_step_number == TimeIntegratorStepNumber::First)
      new_dw->allocateAndPut(velocityVars.divergence,
			     d_lab->d_divConstraintLabel, matlIndex, patch);
    else
      new_dw->getModifiable(velocityVars.divergence,
			    d_lab->d_divConstraintLabel, matlIndex, patch);
    velocityVars.divergence.initialize(0.0);
#endif

  TAU_PROFILE_STOP(input);
  TAU_PROFILE_START(inputcell);

    PerPatch<CellInformationP> cellInfoP;
    if (new_dw->exists(d_lab->d_cellInfoLabel, matlIndex, patch)) 
      new_dw->get(cellInfoP, d_lab->d_cellInfoLabel, matlIndex, patch);
    else {
      cellInfoP.setData(scinew CellInformation(patch));
      new_dw->put(cellInfoP, d_lab->d_cellInfoLabel, matlIndex, patch);
    }
    CellInformation* cellinfo = cellInfoP.get().get_rep();

  TAU_PROFILE_STOP(inputcell);
  TAU_PROFILE_START(input);

    for(int index = 1; index <= Arches::NDIM; ++index) {

      // get multimaterial momentum source terms
      // get velocities for MPMArches with extra ghost cells

      if (d_MAlab) {
	switch (index) {
	
	case Arches::XDIR:

	  new_dw->get(constVelocityVars.mmuVelSu, d_MAlab->d_uVel_mmNonlinSrcLabel,
		      matlIndex, patch,
		      Ghost::None, Arches::ZEROGHOSTCELLS);
	  new_dw->get(constVelocityVars.mmuVelSp, d_MAlab->d_uVel_mmLinSrcLabel,
		      matlIndex, patch,
		      Ghost::None, Arches::ZEROGHOSTCELLS);
	  break;

	case Arches::YDIR:

	  new_dw->get(constVelocityVars.mmvVelSu, d_MAlab->d_vVel_mmNonlinSrcLabel,
		      matlIndex, patch,
		      Ghost::None, Arches::ZEROGHOSTCELLS);
	  new_dw->get(constVelocityVars.mmvVelSp, d_MAlab->d_vVel_mmLinSrcLabel,
		      matlIndex, patch,
		      Ghost::None, Arches::ZEROGHOSTCELLS);
	  break;
	case Arches::ZDIR:

	  new_dw->get(constVelocityVars.mmwVelSu, d_MAlab->d_wVel_mmNonlinSrcLabel,
		      matlIndex, patch,
		      Ghost::None, Arches::ZEROGHOSTCELLS);
	  new_dw->get(constVelocityVars.mmwVelSp, d_MAlab->d_wVel_mmLinSrcLabel,
		      matlIndex, patch,
		      Ghost::None, Arches::ZEROGHOSTCELLS);
	  break;
	}
      }
      
      for (int ii = 0; ii < d_lab->d_stencilMatl->size(); ii++) {

	switch(index) {

	case Arches::XDIR:

	  new_dw->allocateTemporary(velocityVars.uVelocityCoeff[ii],  patch);
	  new_dw->allocateTemporary(velocityVars.uVelocityConvectCoeff[ii],  patch);
	  break;
	case Arches::YDIR:
	  new_dw->allocateTemporary(velocityVars.vVelocityCoeff[ii],  patch);
	  new_dw->allocateTemporary(velocityVars.vVelocityConvectCoeff[ii],  patch);
	  break;
	case Arches::ZDIR:
	  new_dw->allocateTemporary(velocityVars.wVelocityCoeff[ii],  patch);
	  new_dw->allocateTemporary(velocityVars.wVelocityConvectCoeff[ii],  patch);
	  break;
	default:
	  throw InvalidValue("invalid index for velocity in MomentumSolver");
	}
      }
  TAU_PROFILE_STOP(input);
  TAU_PROFILE_START(compute);

      // Calculate Velocity Coeffs :
      //  inputs : [u,v,w]VelocitySIVBC, densityIN, viscosityIN
      //  outputs: [u,v,w]VelCoefPBLM, [u,v,w]VelConvCoefPBLM 

      d_discretize->calculateVelocityCoeff(pc, patch, 
					   delta_t, index, d_central, 
					   cellinfo, &velocityVars,
					   &constVelocityVars);

      // Calculate Velocity source
      //  inputs : [u,v,w]VelocitySIVBC, densityIN, viscosityIN
      //  outputs: [u,v,w]VelLinSrcPBLM, [u,v,w]VelNonLinSrcPBLM
      // get data
      // allocate
      
      switch(index) {

      case Arches::XDIR:

	new_dw->allocateTemporary(velocityVars.uVelLinearSrc,  patch);
	new_dw->allocateTemporary(velocityVars.uVelNonlinearSrc,  patch);
	new_dw->allocateAndPut(velocityVars.uVelRhoHat, timelabels->uvelhat_out,
			       matlIndex, patch,
			       Ghost::AroundFaces, Arches::ONEGHOSTCELL);
	velocityVars.uVelRhoHat.copy(constVelocityVars.uVelocity,
				     velocityVars.uVelRhoHat.getLowIndex(),
				     velocityVars.uVelRhoHat.getHighIndex());

	break;

      case Arches::YDIR:

	new_dw->allocateTemporary(velocityVars.vVelLinearSrc,  patch);
	new_dw->allocateTemporary(velocityVars.vVelNonlinearSrc,  patch);
	new_dw->allocateAndPut(velocityVars.vVelRhoHat, timelabels->vvelhat_out,
			       matlIndex, patch,
			       Ghost::AroundFaces, Arches::ONEGHOSTCELL);
	velocityVars.vVelRhoHat.copy(constVelocityVars.vVelocity,
				     velocityVars.vVelRhoHat.getLowIndex(),
				     velocityVars.vVelRhoHat.getHighIndex());

	break;

      case Arches::ZDIR:

	new_dw->allocateTemporary(velocityVars.wVelLinearSrc,  patch);
	new_dw->allocateTemporary(velocityVars.wVelNonlinearSrc,  patch);
	new_dw->allocateAndPut(velocityVars.wVelRhoHat, timelabels->wvelhat_out,
			       matlIndex, patch,
			       Ghost::AroundFaces, Arches::ONEGHOSTCELL);
	velocityVars.wVelRhoHat.copy(constVelocityVars.wVelocity,
				     velocityVars.wVelRhoHat.getLowIndex(),
				     velocityVars.wVelRhoHat.getHighIndex());

	break;

      default:
	throw InvalidValue("Invalid index in MomentumSolver for calcVelSrc");

      }

      d_source->calculateVelocitySource(pc, patch, 
					delta_t, index,
					cellinfo, &velocityVars,
					&constVelocityVars);

#ifdef filter_convection_terms
    for (int ii = 0; ii < d_lab->d_scalarFluxMatl->size(); ii++) {
      switch (index) {
	  case Arches::XDIR:
	    new_dw->allocateTemporary(velocityVars.filteredRhoUjU[ii],patch,
			    Ghost::AroundFaces, Arches::TWOGHOSTCELLS);

	    new_dw->copyOut(velocityVars.filteredRhoUjU[ii], 
			    d_lab->d_filteredRhoUjULabel, ii, patch,
			    Ghost::AroundFaces, Arches::TWOGHOSTCELLS);
	break;

	case Arches::YDIR:
	    new_dw->allocateTemporary(velocityVars.filteredRhoUjV[ii],patch,
			    Ghost::AroundFaces, Arches::TWOGHOSTCELLS);

	    new_dw->copyOut(velocityVars.filteredRhoUjV[ii], 
			    d_lab->d_filteredRhoUjVLabel, ii, patch,
			    Ghost::AroundFaces, Arches::TWOGHOSTCELLS);
	break;

	case Arches::ZDIR:
	    new_dw->allocateTemporary(velocityVars.filteredRhoUjW[ii],patch,
			    Ghost::AroundFaces, Arches::TWOGHOSTCELLS);

	    new_dw->copyOut(velocityVars.filteredRhoUjW[ii], 
			    d_lab->d_filteredRhoUjWLabel, ii, patch,
			    Ghost::AroundFaces, Arches::TWOGHOSTCELLS);
	break;
	default:
	  throw InvalidValue("Invalid index in MomentumSolver::BuildVelCoeff");
      }
    }

    filterNonlinearTerms(pc, patch, index, cellinfo, &velocityVars);

    IntVector indexLow;
    IntVector indexHigh;
    double areaew, areans, areatb;
	
    switch (index) {
	case Arches::XDIR:
	  indexLow = patch->getSFCXFORTLowIndex();
	  indexHigh = patch->getSFCXFORTHighIndex();

	  for (int colZ = indexLow.z(); colZ <= indexHigh.z(); colZ ++) {
	    for (int colY = indexLow.y(); colY <= indexHigh.y(); colY ++) {
	      for (int colX = indexLow.x(); colX <= indexHigh.x(); colX ++) {
          	IntVector currCell(colX, colY, colZ);
          	IntVector xplusCell(colX+1, colY, colZ);
          	IntVector yplusCell(colX, colY+1, colZ);
        	IntVector zplusCell(colX, colY, colZ+1);
	  	areaew = cellinfo->sns[colY] * cellinfo->stb[colZ];
	  	areans = cellinfo->sewu[colX] * cellinfo->stb[colZ];
	  	areatb = cellinfo->sewu[colX] * cellinfo->sns[colY];

		velocityVars.uVelNonlinearSrc[currCell] -=
		((velocityVars.filteredRhoUjU[0])[xplusCell]-
		 (velocityVars.filteredRhoUjU[0])[currCell]) * areaew +
		((velocityVars.filteredRhoUjU[1])[yplusCell]-
		 (velocityVars.filteredRhoUjU[1])[currCell]) *areans +
		((velocityVars.filteredRhoUjU[2])[zplusCell]-
		 (velocityVars.filteredRhoUjU[2])[currCell]) *areatb;
	      }
	    }
	  }
	break;

	case Arches::YDIR:
	  indexLow = patch->getSFCYFORTLowIndex();
	  indexHigh = patch->getSFCYFORTHighIndex();

	  for (int colZ = indexLow.z(); colZ <= indexHigh.z(); colZ ++) {
	    for (int colY = indexLow.y(); colY <= indexHigh.y(); colY ++) {
	      for (int colX = indexLow.x(); colX <= indexHigh.x(); colX ++) {
          	IntVector currCell(colX, colY, colZ);
          	IntVector xplusCell(colX+1, colY, colZ);
          	IntVector yplusCell(colX, colY+1, colZ);
        	IntVector zplusCell(colX, colY, colZ+1);
	  	areaew = cellinfo->snsv[colY] * cellinfo->stb[colZ];
	  	areans = cellinfo->sew[colX] * cellinfo->stb[colZ];
	  	areatb = cellinfo->sew[colX] * cellinfo->snsv[colY];

		velocityVars.vVelNonlinearSrc[currCell] -=
		((velocityVars.filteredRhoUjV[0])[xplusCell]-
		 (velocityVars.filteredRhoUjV[0])[currCell]) * areaew +
		((velocityVars.filteredRhoUjV[1])[yplusCell]-
		 (velocityVars.filteredRhoUjV[1])[currCell]) *areans +
		((velocityVars.filteredRhoUjV[2])[zplusCell]-
		 (velocityVars.filteredRhoUjV[2])[currCell]) * areatb;
	      }
	    }
	  }
	break;

	case Arches::ZDIR:
	  indexLow = patch->getSFCZFORTLowIndex();
	  indexHigh = patch->getSFCZFORTHighIndex();

	  for (int colZ = indexLow.z(); colZ <= indexHigh.z(); colZ ++) {
	    for (int colY = indexLow.y(); colY <= indexHigh.y(); colY ++) {
	      for (int colX = indexLow.x(); colX <= indexHigh.x(); colX ++) {
          	IntVector currCell(colX, colY, colZ);
          	IntVector xplusCell(colX+1, colY, colZ);
          	IntVector yplusCell(colX, colY+1, colZ);
        	IntVector zplusCell(colX, colY, colZ+1);
	  	areaew = cellinfo->sns[colY] * cellinfo->stbw[colZ];
	  	areans = cellinfo->sew[colX] * cellinfo->stbw[colZ];
	  	areatb = cellinfo->sew[colX] * cellinfo->sns[colY];

		velocityVars.wVelNonlinearSrc[currCell] -=
		((velocityVars.filteredRhoUjW[0])[xplusCell]-
		 (velocityVars.filteredRhoUjW[0])[currCell]) * areaew +
		((velocityVars.filteredRhoUjW[1])[yplusCell]-
		 (velocityVars.filteredRhoUjW[1])[currCell]) *areans +
		((velocityVars.filteredRhoUjW[2])[zplusCell]-
		 (velocityVars.filteredRhoUjW[2])[currCell]) *areatb;
	      }
	    }
	  }
	break;
	default:
	  throw InvalidValue("Invalid index in MomentumSolver::BuildVelCoeff");
    }
#endif

      // for scalesimilarity model add stress tensor to the source of velocity eqn.
      if (dynamic_cast<const ScaleSimilarityModel*>(d_turbModel)) {
	StencilMatrix<constCCVariable<double> > stressTensor; //9 point tensor
        if (timelabels->integrator_step_number == 
			TimeIntegratorStepNumber::First)
	  for (int ii = 0; ii < d_lab->d_stressTensorMatl->size(); ii++) {
	    old_dw->get(stressTensor[ii], 
			d_lab->d_stressTensorCompLabel, ii, patch,
			Ghost::AroundCells, Arches::ONEGHOSTCELL);
	  }
	else
	  for (int ii = 0; ii < d_lab->d_stressTensorMatl->size(); ii++) {
	    new_dw->get(stressTensor[ii], 
			d_lab->d_stressTensorCompLabel, ii, patch,
			Ghost::AroundCells, Arches::ONEGHOSTCELL);
	  }

	IntVector indexLow = patch->getCellFORTLowIndex();
	IntVector indexHigh = patch->getCellFORTHighIndex();
	
	// set density for the whole domain


	      // Store current cell
	double sue, suw, sun, sus, sut, sub;
	switch (index) {
	case Arches::XDIR:
	  for (int colZ = indexLow.z(); colZ <= indexHigh.z(); colZ ++) {
	    for (int colY = indexLow.y(); colY <= indexHigh.y(); colY ++) {
	      for (int colX = indexLow.x(); colX <= indexHigh.x(); colX ++) {
		IntVector currCell(colX, colY, colZ);
		IntVector prevXCell(colX-1, colY, colZ);
		IntVector prevYCell(colX, colY-1, colZ);
		IntVector prevZCell(colX, colY, colZ-1);

		sue = cellinfo->sns[colY]*cellinfo->stb[colZ]*
		             (stressTensor[0])[currCell];
		suw = cellinfo->sns[colY]*cellinfo->stb[colZ]*
		             (stressTensor[0])[prevXCell];
		sun = 0.25*cellinfo->sew[colX]*cellinfo->stb[colZ]*
		             ((stressTensor[1])[currCell]+
			      (stressTensor[1])[prevXCell]+
			      (stressTensor[1])[IntVector(colX,colY+1,colZ)]+
			      (stressTensor[1])[IntVector(colX-1,colY+1,colZ)]);
		sus =  0.25*cellinfo->sew[colX]*cellinfo->stb[colZ]*
		             ((stressTensor[1])[currCell]+
			      (stressTensor[1])[prevXCell]+
			      (stressTensor[1])[IntVector(colX,colY-1,colZ)]+
			      (stressTensor[1])[IntVector(colX-1,colY-1,colZ)]);
		sut = 0.25*cellinfo->sns[colY]*cellinfo->sew[colX]*
		             ((stressTensor[2])[currCell]+
			      (stressTensor[2])[prevXCell]+
			      (stressTensor[2])[IntVector(colX,colY,colZ+1)]+
			      (stressTensor[2])[IntVector(colX-1,colY,colZ+1)]);
		sub =  0.25*cellinfo->sns[colY]*cellinfo->sew[colX]*
		             ((stressTensor[2])[currCell]+
			      (stressTensor[2])[prevXCell]+
			      (stressTensor[2])[IntVector(colX,colY,colZ-1)]+
			      (stressTensor[2])[IntVector(colX-1,colY,colZ-1)]);
		velocityVars.uVelNonlinearSrc[currCell] += suw-sue+sus-sun+sub-sut;
	      }
	    }
	  }
	  break;
	case Arches::YDIR:
	  for (int colZ = indexLow.z(); colZ <= indexHigh.z(); colZ ++) {
	    for (int colY = indexLow.y(); colY <= indexHigh.y(); colY ++) {
	      for (int colX = indexLow.x(); colX <= indexHigh.x(); colX ++) {
		IntVector currCell(colX, colY, colZ);
		IntVector prevXCell(colX-1, colY, colZ);
		IntVector prevYCell(colX, colY-1, colZ);
		IntVector prevZCell(colX, colY, colZ-1);

		sue = 0.25*cellinfo->sns[colY]*cellinfo->stb[colZ]*
		  ((stressTensor[3])[currCell]+
		   (stressTensor[3])[prevYCell]+
		   (stressTensor[3])[IntVector(colX+1,colY,colZ)]+
		   (stressTensor[3])[IntVector(colX+1,colY-1,colZ)]);
		suw =  0.25*cellinfo->sns[colY]*cellinfo->stb[colZ]*
		  ((stressTensor[3])[currCell]+
		   (stressTensor[3])[prevYCell]+
		   (stressTensor[3])[IntVector(colX-1,colY,colZ)]+
		   (stressTensor[3])[IntVector(colX-1,colY-1,colZ)]);
		sun = cellinfo->sew[colX]*cellinfo->stb[colZ]*
		  (stressTensor[4])[currCell];
		sus = cellinfo->sew[colX]*cellinfo->stb[colZ]*
		  (stressTensor[4])[prevYCell];
		sut = 0.25*cellinfo->sns[colY]*cellinfo->sew[colX]*
		  ((stressTensor[5])[currCell]+
		   (stressTensor[5])[prevYCell]+
		   (stressTensor[5])[IntVector(colX,colY,colZ+1)]+
		   (stressTensor[5])[IntVector(colX,colY-1,colZ+1)]);
		sub =  0.25*cellinfo->sns[colY]*cellinfo->sew[colX]*
		  ((stressTensor[5])[currCell]+
		   (stressTensor[5])[prevYCell]+
		   (stressTensor[5])[IntVector(colX,colY,colZ-1)]+
		   (stressTensor[5])[IntVector(colX,colY-1,colZ-1)]);
		velocityVars.vVelNonlinearSrc[currCell] += suw-sue+sus-sun+sub-sut;
	      }
	    }
	  }
	  break;
	case Arches::ZDIR:
	  for (int colZ = indexLow.z(); colZ <= indexHigh.z(); colZ ++) {
	    for (int colY = indexLow.y(); colY <= indexHigh.y(); colY ++) {
	      for (int colX = indexLow.x(); colX <= indexHigh.x(); colX ++) {
		IntVector currCell(colX, colY, colZ);
		IntVector prevXCell(colX-1, colY, colZ);
		IntVector prevYCell(colX, colY-1, colZ);
		IntVector prevZCell(colX, colY, colZ-1);

		sue = 0.25*cellinfo->sns[colY]*cellinfo->stb[colZ]*
		             ((stressTensor[6])[currCell]+
			      (stressTensor[6])[prevZCell]+
			      (stressTensor[6])[IntVector(colX+1,colY,colZ)]+
			      (stressTensor[6])[IntVector(colX+1,colY,colZ-1)]);
		suw =  0.25*cellinfo->sns[colY]*cellinfo->stb[colZ]*
		             ((stressTensor[6])[currCell]+
			      (stressTensor[6])[prevZCell]+
			      (stressTensor[6])[IntVector(colX-1,colY,colZ)]+
			      (stressTensor[6])[IntVector(colX-1,colY,colZ-1)]);
		sun = 0.25*cellinfo->sew[colX]*cellinfo->stb[colZ]*
		             ((stressTensor[7])[currCell]+
			      (stressTensor[7])[prevZCell]+
			      (stressTensor[7])[IntVector(colX,colY+1,colZ)]+
			      (stressTensor[7])[IntVector(colX,colY+1,colZ-1)]);
		sus =  0.25*cellinfo->sew[colX]*cellinfo->stb[colZ]*
		             ((stressTensor[7])[currCell]+
			      (stressTensor[7])[prevZCell]+
			      (stressTensor[7])[IntVector(colX,colY-1,colZ)]+
			      (stressTensor[7])[IntVector(colX,colY-1,colZ-1)]);
		sut = cellinfo->sew[colX]*cellinfo->sns[colY]*
		             (stressTensor[8])[currCell];
		sub = cellinfo->sew[colX]*cellinfo->sns[colY]*
		             (stressTensor[8])[prevZCell];
		velocityVars.wVelNonlinearSrc[currCell] += suw-sue+sus-sun+sub-sut;
	      }
	    }
	  }
	  break;
	default:
	  throw InvalidValue("Invalid index in MomentumSolver::BuildVelCoeffPred");
	}
      }
	
      // add multimaterial momentum source term

      if (d_MAlab)
	d_source->computemmMomentumSource(pc, patch, index, cellinfo,
					  &velocityVars, &constVelocityVars);

      // Calculate the Velocity BCS
      //  inputs : densityCP, [u,v,w]VelocitySIVBC, [u,v,w]VelCoefPBLM
      //           [u,v,w]VelLinSrcPBLM, [u,v,w]VelNonLinSrcPBLM
      //  outputs: [u,v,w]VelCoefPBLM, [u,v,w]VelLinSrcPBLM, 
      //           [u,v,w]VelNonLinSrcPBLM
      
      d_boundaryCondition->velocityBC(pc, patch, 
				    index,
				    cellinfo, &velocityVars,
				    &constVelocityVars);

      if (d_boundaryCondition->getIntrusionBC()) {
	d_boundaryCondition->intrusionMomExchangeBC(pc, patch, index,
						    cellinfo, &velocityVars,
						    &constVelocityVars);
	d_boundaryCondition->intrusionVelocityBC(pc, patch, index, 
						 cellinfo, &velocityVars,
						 &constVelocityVars);

      }
    // apply multimaterial velocity bc
    // treats multimaterial wall as intrusion

      if (d_MAlab)
	d_boundaryCondition->mmvelocityBC(pc, patch, index, cellinfo,
					  &velocityVars, &constVelocityVars);
    
    // Modify Velocity Mass Source
    //  inputs : [u,v,w]VelocitySIVBC, [u,v,w]VelCoefPBLM, 
    //           [u,v,w]VelConvCoefPBLM, [u,v,w]VelLinSrcPBLM, 
    //           [u,v,w]VelNonLinSrcPBLM
    //  outputs: [u,v,w]VelLinSrcPBLM, [u,v,w]VelNonLinSrcPBLM
      d_source->modifyVelMassSource(pc, patch, delta_t, index,
				    &velocityVars, &constVelocityVars);

      // Calculate Velocity diagonal
      //  inputs : [u,v,w]VelCoefPBLM, [u,v,w]VelLinSrcPBLM
      //  outputs: [u,v,w]VelCoefPBLM

      d_discretize->calculateVelDiagonal(pc, patch,
					 index,
					 &velocityVars);

      if (d_MAlab) {
	d_boundaryCondition->calculateVelRhoHat_mm(pc, patch, index, delta_t,
						   cellinfo, &velocityVars,
						   &constVelocityVars);
      }
      else {
	d_discretize->calculateVelRhoHat(pc, patch, index, delta_t,
					 cellinfo, &velocityVars,
					 &constVelocityVars);
      }

  if (d_pressure_correction) {
  int ioff, joff, koff;
  IntVector idxLoU;
  IntVector idxHiU;
  switch(index) {
  case Arches::XDIR:
    idxLoU = patch->getSFCXFORTLowIndex();
    idxHiU = patch->getSFCXFORTHighIndex();
    ioff = 1; joff = 0; koff = 0;
    fort_computevel(idxLoU, idxHiU, velocityVars.uVelRhoHat, 
		    constVelocityVars.pressure,
		    constVelocityVars.new_density, delta_t,
		    ioff, joff, koff, cellinfo->dxpw);
    break;
  case Arches::YDIR:
    idxLoU = patch->getSFCYFORTLowIndex();
    idxHiU = patch->getSFCYFORTHighIndex();
    ioff = 0; joff = 1; koff = 0;
    fort_computevel(idxLoU, idxHiU, velocityVars.vVelRhoHat,
		    constVelocityVars.pressure,
		    constVelocityVars.new_density, delta_t,
		    ioff, joff, koff, cellinfo->dyps);

    break;
  case Arches::ZDIR:
    idxLoU = patch->getSFCZFORTLowIndex();
    idxHiU = patch->getSFCZFORTHighIndex();
    ioff = 0; joff = 0; koff = 1;
    fort_computevel(idxLoU, idxHiU, velocityVars.wVelRhoHat,
		    constVelocityVars.pressure,
		    constVelocityVars.new_density, delta_t,
		    ioff, joff, koff, cellinfo->dzpb);
    break;
  default:
    throw InvalidValue("Invalid index in MomentumSolver::addPressGrad");
  }
  }

#ifdef ARCHES_PRES_DEBUG
    std::cerr << "Done building matrix for press coeff" << endl;
#endif
    
    }
//    d_boundaryCondition->newrecomputePressureBC(pc, patch, cellinfo,
//						&velocityVars);
    d_boundaryCondition->velRhoHatInletBC(pc, patch, cellinfo,
					  &velocityVars, &constVelocityVars);
    d_boundaryCondition->velRhoHatPressureBC(pc, patch, cellinfo,
					     &velocityVars, &constVelocityVars);
    d_boundaryCondition->velRhoHatOutletBC(pc, patch, cellinfo, delta_t,
					   &velocityVars, &constVelocityVars,
					   maxAbsU, maxAbsV, maxAbsW);
  if (d_pressure_correction) {
  IntVector idxLo = patch->getCellFORTLowIndex();
  IntVector idxHi = patch->getCellFORTHighIndex();

  int out_celltypeval = d_boundaryCondition->outletCellType();

  bool xminus = patch->getBCType(Patch::xminus) != Patch::Neighbor;
  bool xplus =  patch->getBCType(Patch::xplus) != Patch::Neighbor;
  bool yminus = patch->getBCType(Patch::yminus) != Patch::Neighbor;
  bool yplus =  patch->getBCType(Patch::yplus) != Patch::Neighbor;
  bool zminus = patch->getBCType(Patch::zminus) != Patch::Neighbor;
  bool zplus =  patch->getBCType(Patch::zplus) != Patch::Neighbor;

  if (xminus) {
    int colX = idxLo.x();
    for (int colZ = idxLo.z(); colZ <= idxHi.z(); colZ ++) {
      for (int colY = idxLo.y(); colY <= idxHi.y(); colY ++) {
        IntVector currCell(colX, colY, colZ);
        IntVector xminusCell(colX-1, colY, colZ);
        if (constVelocityVars.cellType[xminusCell] == out_celltypeval) {
           double avdenlow = 0.5 * (constVelocityVars.new_density[currCell] +
			            constVelocityVars.new_density[xminusCell]);

           velocityVars.uVelRhoHat[currCell] -= 2.0*delta_t*
		   		constVelocityVars.pressure[currCell]/
				(cellinfo->sew[colX] * avdenlow);

           velocityVars.uVelRhoHat[xminusCell] = velocityVars.uVelRhoHat[currCell];

        }
      }
    }
  }
  if (xplus) {
    int colX = idxHi.x();
    for (int colZ = idxLo.z(); colZ <= idxHi.z(); colZ ++) {
      for (int colY = idxLo.y(); colY <= idxHi.y(); colY ++) {
        IntVector currCell(colX, colY, colZ);
        IntVector xplusCell(colX+1, colY, colZ);
        IntVector xplusplusCell(colX+2, colY, colZ);
        if (constVelocityVars.cellType[xplusCell] == out_celltypeval) {
           double avden = 0.5 * (constVelocityVars.new_density[xplusCell] +
			         constVelocityVars.new_density[currCell]);

           velocityVars.uVelRhoHat[xplusCell] += 2.0*delta_t*
				constVelocityVars.pressure[currCell]/
				(cellinfo->sew[colX] * avden);

           velocityVars.uVelRhoHat[xplusplusCell] = velocityVars.uVelRhoHat[xplusCell];
        }
      }
    }
  }
  if (yminus) {
    int colY = idxLo.y();
    for (int colZ = idxLo.z(); colZ <= idxHi.z(); colZ ++) {
      for (int colX = idxLo.x(); colX <= idxHi.x(); colX ++) {
        IntVector currCell(colX, colY, colZ);
	IntVector yminusCell(colX, colY-1, colZ);
        if (constVelocityVars.cellType[yminusCell] == out_celltypeval) {
           double avdenlow = 0.5 * (constVelocityVars.new_density[currCell] +
			            constVelocityVars.new_density[yminusCell]);

           velocityVars.vVelRhoHat[currCell] -= 2.0*delta_t*
		   		constVelocityVars.pressure[currCell]/
				(cellinfo->sns[colY] * avdenlow);

           velocityVars.vVelRhoHat[yminusCell] = velocityVars.vVelRhoHat[currCell];

        }
      }
    }
  }
  if (yplus) {
    int colY = idxHi.y();
    for (int colZ = idxLo.z(); colZ <= idxHi.z(); colZ ++) {
      for (int colX = idxLo.x(); colX <= idxHi.x(); colX ++) {
        IntVector currCell(colX, colY, colZ);
        IntVector yplusCell(colX, colY+1, colZ);
        IntVector yplusplusCell(colX, colY+2, colZ);
        if (constVelocityVars.cellType[yplusCell] == out_celltypeval) {
           double avden = 0.5 * (constVelocityVars.new_density[yplusCell] +
			         constVelocityVars.new_density[currCell]);

           velocityVars.vVelRhoHat[yplusCell] += 2.0*delta_t*
		   		constVelocityVars.pressure[currCell]/
				(cellinfo->sns[colY] * avden);

           velocityVars.vVelRhoHat[yplusplusCell] = velocityVars.vVelRhoHat[yplusCell];

        }
      }
    }
  }
  if (zminus) {
    int colZ = idxLo.z();
    for (int colY = idxLo.y(); colY <= idxHi.y(); colY ++) {
      for (int colX = idxLo.x(); colX <= idxHi.x(); colX ++) {
        IntVector currCell(colX, colY, colZ);
	IntVector zminusCell(colX, colY, colZ-1);
        if (constVelocityVars.cellType[zminusCell] == out_celltypeval) {
           double avdenlow = 0.5 * (constVelocityVars.new_density[currCell] +
			            constVelocityVars.new_density[zminusCell]);

           velocityVars.wVelRhoHat[currCell] -= 2.0*delta_t*
		   		constVelocityVars.pressure[currCell]/
				(cellinfo->stb[colZ] * avdenlow);

           velocityVars.wVelRhoHat[zminusCell] = velocityVars.wVelRhoHat[currCell];

        }
      }
    }
  }
  if (zplus) {
    int colZ = idxHi.z();
    for (int colY = idxLo.y(); colY <= idxHi.y(); colY ++) {
      for (int colX = idxLo.x(); colX <= idxHi.x(); colX ++) {
        IntVector currCell(colX, colY, colZ);
        IntVector zplusCell(colX, colY, colZ+1);
        IntVector zplusplusCell(colX, colY, colZ+2);
        if (constVelocityVars.cellType[zplusCell] == out_celltypeval) {
           double avden = 0.5 * (constVelocityVars.new_density[zplusCell] +
			         constVelocityVars.new_density[currCell]);

           velocityVars.wVelRhoHat[zplusCell] += 2.0*delta_t*
		   		constVelocityVars.pressure[currCell]/
				(cellinfo->stb[colZ] * avden);

           velocityVars.wVelRhoHat[zplusplusCell] = velocityVars.wVelRhoHat[zplusCell];

        }
      }
    }
  }
  }

#ifdef divergenceconstraint    
    // compute divergence constraint to use in pressure equation
    d_discretize->computeDivergence(pc, patch,&velocityVars,&constVelocityVars);
#endif

#ifdef ARCHES_PRES_DEBUG
    std::cerr << "Done building matrix for vel coeff for pressure" << endl;
#endif
    
  TAU_PROFILE_STOP(compute);
  }
}

//****************************************************************************
// Schedule the averaging of hat velocities for Runge-Kutta step
//****************************************************************************
void 
MomentumSolver::sched_averageRKHatVelocities(SchedulerP& sched,
					 const PatchSet* patches,
				 	 const MaterialSet* matls,
				         const TimeIntegratorLabel* timelabels)
{
  string taskname =  "MomentumSolver::averageRKHatVelocities" +
		     timelabels->integrator_step_name;
  Task* tsk = scinew Task(taskname, this,
			  &MomentumSolver::averageRKHatVelocities,
			  timelabels);

  tsk->requires(Task::NewDW, d_lab->d_uVelocityOUTBCLabel,
                Ghost::None, Arches::ZEROGHOSTCELLS);
  tsk->requires(Task::NewDW, d_lab->d_vVelocityOUTBCLabel,
                Ghost::None, Arches::ZEROGHOSTCELLS);
  tsk->requires(Task::NewDW, d_lab->d_wVelocityOUTBCLabel,
                Ghost::None, Arches::ZEROGHOSTCELLS);
  tsk->requires(Task::NewDW, d_lab->d_densityINLabel,
		Ghost::AroundCells, Arches::ONEGHOSTCELL);
  tsk->requires(Task::NewDW, d_lab->d_cellTypeLabel,
		Ghost::None, Arches::ZEROGHOSTCELLS);

  tsk->requires(Task::NewDW, timelabels->rho2_density,
		Ghost::AroundCells, Arches::ONEGHOSTCELL);
  tsk->requires(Task::NewDW, timelabels->density_out,
		Ghost::AroundCells, Arches::ONEGHOSTCELL);

  tsk->modifies(timelabels->uvelhat_out);
  tsk->modifies(timelabels->vvelhat_out);
  tsk->modifies(timelabels->wvelhat_out);

  sched->addTask(tsk, patches, matls);
}
//****************************************************************************
// Actually average the Runge-Kutta hat velocities here
//****************************************************************************
void 
MomentumSolver::averageRKHatVelocities(const ProcessorGroup*,
			   	       const PatchSubset* patches,
			   	       const MaterialSubset*,
			   	       DataWarehouse*,
			   	       DataWarehouse* new_dw,
			   	       const TimeIntegratorLabel* timelabels)
{
  for (int p = 0; p < patches->size(); p++) {

    const Patch* patch = patches->get(p);
    int archIndex = 0; // only one arches material
    int matlIndex = d_lab->d_sharedState->
		     getArchesMaterial(archIndex)->getDWIndex(); 

    constCCVariable<double> old_density;
    constCCVariable<double> rho2_density;
    constCCVariable<double> new_density;
    constCCVariable<int> cellType;
    constSFCXVariable<double> old_uvel;
    constSFCYVariable<double> old_vvel;
    constSFCZVariable<double> old_wvel;
    SFCXVariable<double> new_uvel;
    SFCYVariable<double> new_vvel;
    SFCZVariable<double> new_wvel;

    new_dw->get(old_density, d_lab->d_densityINLabel, 
		matlIndex, patch, Ghost::AroundCells, Arches::ONEGHOSTCELL);
    new_dw->get(cellType, d_lab->d_cellTypeLabel,
		matlIndex, patch, Ghost::None, Arches::ZEROGHOSTCELLS);
    new_dw->get(old_uvel, d_lab->d_uVelocityOUTBCLabel, 
		matlIndex, patch, Ghost::None, Arches::ZEROGHOSTCELLS);
    new_dw->get(old_vvel, d_lab->d_vVelocityOUTBCLabel, 
		matlIndex, patch, Ghost::None, Arches::ZEROGHOSTCELLS);
    new_dw->get(old_wvel, d_lab->d_wVelocityOUTBCLabel, 
		matlIndex, patch, Ghost::None, Arches::ZEROGHOSTCELLS);

    new_dw->get(rho2_density, timelabels->rho2_density, 
		matlIndex, patch, Ghost::AroundCells, Arches::ONEGHOSTCELL);
    new_dw->get(new_density, timelabels->density_out, 
		matlIndex, patch, Ghost::AroundCells, Arches::ONEGHOSTCELL);

    new_dw->getModifiable(new_uvel, timelabels->uvelhat_out, 
			  matlIndex, patch);
    new_dw->getModifiable(new_vvel, timelabels->vvelhat_out, 
			  matlIndex, patch);
    new_dw->getModifiable(new_wvel, timelabels->wvelhat_out, 
			  matlIndex, patch);

    double factor_old, factor_new, factor_divide;
    factor_old = timelabels->factor_old;
    factor_new = timelabels->factor_new;
    factor_divide = timelabels->factor_divide;

    IntVector indexLow, indexHigh;
    indexLow = patch->getSFCXFORTLowIndex();
    indexHigh = patch->getSFCXFORTHighIndex();

    for (int colZ = indexLow.z(); colZ <= indexHigh.z(); colZ ++) {
      for (int colY = indexLow.y(); colY <= indexHigh.y(); colY ++) {
	for (int colX = indexLow.x(); colX <= indexHigh.x(); colX ++) {
	  IntVector currCell(colX, colY, colZ);
	  IntVector xminusCell(colX-1, colY, colZ);
          
	    new_uvel[currCell] = (factor_old*old_uvel[currCell]*
		(old_density[currCell]+old_density[xminusCell]) +
		factor_new*new_uvel[currCell]*
		(rho2_density[currCell]+rho2_density[xminusCell]))/
		(factor_divide*(new_density[currCell]+new_density[xminusCell]));

	}
      }
    }
    indexLow = patch->getSFCYFORTLowIndex();
    indexHigh = patch->getSFCYFORTHighIndex();

    for (int colZ = indexLow.z(); colZ <= indexHigh.z(); colZ ++) {
      for (int colY = indexLow.y(); colY <= indexHigh.y(); colY ++) {
	for (int colX = indexLow.x(); colX <= indexHigh.x(); colX ++) {
	  IntVector currCell(colX, colY, colZ);
	  IntVector yminusCell(colX, colY-1, colZ);
          
	    new_vvel[currCell] = (factor_old*old_vvel[currCell]*
		(old_density[currCell]+old_density[yminusCell]) +
		factor_new*new_vvel[currCell]*
		(rho2_density[currCell]+rho2_density[yminusCell]))/
		(factor_divide*(new_density[currCell]+new_density[yminusCell]));

	}
      }
    }
    indexLow = patch->getSFCZFORTLowIndex();
    indexHigh = patch->getSFCZFORTHighIndex();

    for (int colZ = indexLow.z(); colZ <= indexHigh.z(); colZ ++) {
      for (int colY = indexLow.y(); colY <= indexHigh.y(); colY ++) {
	for (int colX = indexLow.x(); colX <= indexHigh.x(); colX ++) {
	  IntVector currCell(colX, colY, colZ);
	  IntVector zminusCell(colX, colY, colZ-1);
          
	    new_wvel[currCell] = (factor_old*old_wvel[currCell]*
		(old_density[currCell]+old_density[zminusCell]) +
		factor_new*new_wvel[currCell]*
		(rho2_density[currCell]+rho2_density[zminusCell]))/
		(factor_divide*(new_density[currCell]+new_density[zminusCell]));

	}
      }
    }

  int out_celltypeval = d_boundaryCondition->outletCellType();
  int press_celltypeval = d_boundaryCondition->pressureCellType();
  IntVector idxLo = patch->getCellFORTLowIndex();
  IntVector idxHi = patch->getCellFORTHighIndex();
  bool xminus = patch->getBCType(Patch::xminus) != Patch::Neighbor;
  bool xplus =  patch->getBCType(Patch::xplus) != Patch::Neighbor;
  bool yminus = patch->getBCType(Patch::yminus) != Patch::Neighbor;
  bool yplus =  patch->getBCType(Patch::yplus) != Patch::Neighbor;
  bool zminus = patch->getBCType(Patch::zminus) != Patch::Neighbor;
  bool zplus =  patch->getBCType(Patch::zplus) != Patch::Neighbor;

  if (xminus) {
    int colX = idxLo.x();
    for (int colZ = idxLo.z(); colZ <= idxHi.z(); colZ ++) {
      for (int colY = idxLo.y(); colY <= idxHi.y(); colY ++) {
        IntVector currCell(colX, colY, colZ);
        IntVector xminusCell(colX-1, colY, colZ);
        IntVector xminusyminusCell(colX-1, colY-1, colZ);
        IntVector xminuszminusCell(colX-1, colY, colZ-1);

	   new_uvel[currCell] = (factor_old*old_uvel[currCell]*
		(old_density[currCell]+old_density[xminusCell]) +
		factor_new*new_uvel[currCell]*
		(rho2_density[currCell]+rho2_density[xminusCell]))/
		(factor_divide*(new_density[currCell]+new_density[xminusCell]));

 	if ((cellType[xminusCell] == out_celltypeval)||
	    (cellType[xminusCell] == press_celltypeval)) {

           new_uvel[xminusCell] = new_uvel[currCell];

        if (!(yminus && (colY == idxLo.y()))) {
	    new_vvel[xminusCell] = (factor_old*old_vvel[xminusCell]*
		(old_density[xminusCell]+old_density[xminusyminusCell]) +
		factor_new*new_vvel[xminusCell]*
		(rho2_density[xminusCell]+rho2_density[xminusyminusCell]))/
		(factor_divide*
		(new_density[xminusCell]+new_density[xminusyminusCell]));
	}
        if (!(zminus && (colZ == idxLo.z()))) {
	    new_wvel[xminusCell] = (factor_old*old_wvel[xminusCell]*
		(old_density[xminusCell]+old_density[xminuszminusCell]) +
		factor_new*new_wvel[xminusCell]*
		(rho2_density[xminusCell]+rho2_density[xminuszminusCell]))/
		(factor_divide*
		(new_density[xminusCell]+new_density[xminuszminusCell]));
	}
	}
      }
    }
  }
  if (xplus) {
    int colX = idxHi.x();
    for (int colZ = idxLo.z(); colZ <= idxHi.z(); colZ ++) {
      for (int colY = idxLo.y(); colY <= idxHi.y(); colY ++) {
        IntVector currCell(colX, colY, colZ);
        IntVector xplusCell(colX+1, colY, colZ);
        IntVector xplusyminusCell(colX+1, colY-1, colZ);
        IntVector xpluszminusCell(colX+1, colY, colZ-1);
        IntVector xplusplusCell(colX+2, colY, colZ);

	   new_uvel[xplusCell] = (factor_old*old_uvel[xplusCell]*
		(old_density[xplusCell]+old_density[currCell]) +
		factor_new*new_uvel[xplusCell]*
		(rho2_density[xplusCell]+rho2_density[currCell]))/
		(factor_divide*(new_density[xplusCell]+new_density[currCell]));

 	if ((cellType[xplusCell] == out_celltypeval)||
	    (cellType[xplusCell] == press_celltypeval)) {

           new_uvel[xplusplusCell] = new_uvel[xplusCell];

        if (!(yminus && (colY == idxLo.y()))) {
	    new_vvel[xplusCell] = (factor_old*old_vvel[xplusCell]*
		(old_density[xplusCell]+old_density[xplusyminusCell]) +
		factor_new*new_vvel[xplusCell]*
		(rho2_density[xplusCell]+rho2_density[xplusyminusCell]))/
		(factor_divide*
		(new_density[xplusCell]+new_density[xplusyminusCell]));
	}

        if (!(zminus && (colZ == idxLo.z()))) {
	    new_wvel[xplusCell] = (factor_old*old_wvel[xplusCell]*
		(old_density[xplusCell]+old_density[xpluszminusCell]) +
		factor_new*new_wvel[xplusCell]*
		(rho2_density[xplusCell]+rho2_density[xpluszminusCell]))/
		(factor_divide*
		(new_density[xplusCell]+new_density[xpluszminusCell]));
	}
	}
      }
    }
  }
  if (yminus) {
    int colY = idxLo.y();
    for (int colZ = idxLo.z(); colZ <= idxHi.z(); colZ ++) {
      for (int colX = idxLo.x(); colX <= idxHi.x(); colX ++) {
        IntVector currCell(colX, colY, colZ);
	IntVector yminusCell(colX, colY-1, colZ);
        IntVector yminusxminusCell(colX-1, colY-1, colZ);
        IntVector yminuszminusCell(colX, colY-1, colZ-1);

	   new_vvel[currCell] = (factor_old*old_vvel[currCell]*
		(old_density[currCell]+old_density[yminusCell]) +
		factor_new*new_vvel[currCell]*
		(rho2_density[currCell]+rho2_density[yminusCell]))/
		(factor_divide*(new_density[currCell]+new_density[yminusCell]));

 	if ((cellType[yminusCell] == out_celltypeval)||
	    (cellType[yminusCell] == press_celltypeval)) {

           new_vvel[yminusCell] = new_vvel[currCell];

        if (!(xminus && (colX == idxLo.x()))) {
	    new_uvel[yminusCell] = (factor_old*old_uvel[yminusCell]*
		(old_density[yminusCell]+old_density[yminusxminusCell]) +
		factor_new*new_uvel[yminusCell]*
		(rho2_density[yminusCell]+rho2_density[yminusxminusCell]))/
		(factor_divide*
		(new_density[yminusCell]+new_density[yminusxminusCell]));
	}
        if (!(zminus && (colZ == idxLo.z()))) {
	    new_wvel[yminusCell] = (factor_old*old_wvel[yminusCell]*
		(old_density[yminusCell]+old_density[yminuszminusCell]) +
		factor_new*new_wvel[yminusCell]*
		(rho2_density[yminusCell]+rho2_density[yminuszminusCell]))/
		(factor_divide*
		(new_density[yminusCell]+new_density[yminuszminusCell]));
	}
	}
      }
    }
  }
  if (yplus) {
    int colY = idxHi.y();
    for (int colZ = idxLo.z(); colZ <= idxHi.z(); colZ ++) {
      for (int colX = idxLo.x(); colX <= idxHi.x(); colX ++) {
        IntVector currCell(colX, colY, colZ);
        IntVector yplusCell(colX, colY+1, colZ);
        IntVector yplusxminusCell(colX-1, colY+1, colZ);
        IntVector ypluszminusCell(colX, colY+1, colZ-1);
        IntVector yplusplusCell(colX, colY+2, colZ);

	   new_vvel[yplusCell] = (factor_old*old_vvel[yplusCell]*
		(old_density[yplusCell]+old_density[currCell]) +
		factor_new*new_vvel[yplusCell]*
		(rho2_density[yplusCell]+rho2_density[currCell]))/
		(factor_divide*(new_density[yplusCell]+new_density[currCell]));

 	if ((cellType[yplusCell] == out_celltypeval)||
	    (cellType[yplusCell] == press_celltypeval)) {

           new_vvel[yplusplusCell] = new_vvel[yplusCell];

        if (!(xminus && (colX == idxLo.x()))) {
	    new_uvel[yplusCell] = (factor_old*old_uvel[yplusCell]*
		(old_density[yplusCell]+old_density[yplusxminusCell]) +
		factor_new*new_uvel[yplusCell]*
		(rho2_density[yplusCell]+rho2_density[yplusxminusCell]))/
		(factor_divide*
		(new_density[yplusCell]+new_density[yplusxminusCell]));
	}

        if (!(zminus && (colZ == idxLo.z()))) {
	    new_wvel[yplusCell] = (factor_old*old_wvel[yplusCell]*
		(old_density[yplusCell]+old_density[ypluszminusCell]) +
		factor_new*new_wvel[yplusCell]*
		(rho2_density[yplusCell]+rho2_density[ypluszminusCell]))/
		(factor_divide*
		(new_density[yplusCell]+new_density[ypluszminusCell]));
	}
	}
      }
    }
  }
  if (zminus) {
    int colZ = idxLo.z();
    for (int colY = idxLo.y(); colY <= idxHi.y(); colY ++) {
      for (int colX = idxLo.x(); colX <= idxHi.x(); colX ++) {
        IntVector currCell(colX, colY, colZ);
	IntVector zminusCell(colX, colY, colZ-1);
        IntVector zminusxminusCell(colX-1, colY, colZ-1);
        IntVector zminusyminusCell(colX, colY-1, colZ-1);

	   new_wvel[currCell] = (factor_old*old_wvel[currCell]*
		(old_density[currCell]+old_density[zminusCell]) +
		factor_new*new_wvel[currCell]*
		(rho2_density[currCell]+rho2_density[zminusCell]))/
		(factor_divide*
		(new_density[currCell]+new_density[zminusCell]));

 	if ((cellType[zminusCell] == out_celltypeval)||
	    (cellType[zminusCell] == press_celltypeval)) {

           new_wvel[zminusCell] = new_wvel[currCell];

        if (!(xminus && (colX == idxLo.x()))) {
	    new_uvel[zminusCell] = (factor_old*old_uvel[zminusCell]*
		(old_density[zminusCell]+old_density[zminusxminusCell]) +
		factor_new*new_uvel[zminusCell]*
		(rho2_density[zminusCell]+rho2_density[zminusxminusCell]))/
		(factor_divide*
		(new_density[zminusCell]+new_density[zminusxminusCell]));
	}
        if (!(yminus && (colY == idxLo.y()))) {
	    new_vvel[zminusCell] = (factor_old*old_vvel[zminusCell]*
		(old_density[zminusCell]+old_density[zminusyminusCell]) +
		factor_new*new_vvel[zminusCell]*
		(rho2_density[zminusCell]+rho2_density[zminusyminusCell]))/
		(factor_divide*
		(new_density[zminusCell]+new_density[zminusyminusCell]));
	}
	}
      }
    }
  }
  if (zplus) {
    int colZ = idxHi.z();
    for (int colY = idxLo.y(); colY <= idxHi.y(); colY ++) {
      for (int colX = idxLo.x(); colX <= idxHi.x(); colX ++) {
        IntVector currCell(colX, colY, colZ);
        IntVector zplusCell(colX, colY, colZ+1);
        IntVector zplusxminusCell(colX-1, colY, colZ+1);
        IntVector zplusyminusCell(colX, colY-1, colZ+1);
        IntVector zplusplusCell(colX, colY, colZ+2);

	   new_wvel[zplusCell] = (factor_old*old_wvel[zplusCell]*
		(old_density[zplusCell]+old_density[currCell]) +
		factor_new*new_wvel[zplusCell]*
		(rho2_density[zplusCell]+rho2_density[currCell]))/
		(factor_divide*(new_density[zplusCell]+new_density[currCell]));

 	if ((cellType[zplusCell] == out_celltypeval)||
	    (cellType[zplusCell] == press_celltypeval)) {

           new_wvel[zplusplusCell] = new_wvel[zplusCell];

        if (!(xminus && (colX == idxLo.x()))) {
	    new_uvel[zplusCell] = (factor_old*old_uvel[zplusCell]*
		(old_density[zplusCell]+old_density[zplusxminusCell]) +
		factor_new*new_uvel[zplusCell]*
		(rho2_density[zplusCell]+rho2_density[zplusxminusCell]))/
		(factor_divide*
		(new_density[zplusCell]+new_density[zplusxminusCell]));
	}

        if (!(yminus && (colY == idxLo.y()))) {
	    new_vvel[zplusCell] = (factor_old*old_vvel[zplusCell]*
		(old_density[zplusCell]+old_density[zplusyminusCell]) +
		factor_new*new_vvel[zplusCell]*
		(rho2_density[zplusCell]+rho2_density[zplusyminusCell]))/
		(factor_divide*
		(new_density[zplusCell]+new_density[zplusyminusCell]));
	}
	}
      }
    }
  }
  }
}
//****************************************************************************
//  Schedule computing of nonlinear terms
//****************************************************************************
void 
MomentumSolver::sched_computeNonlinearTerms(SchedulerP& sched, 
					    const PatchSet* patches,
					    const MaterialSet* matls,
					    const ArchesLabel* d_lab,
		    		          const TimeIntegratorLabel* timelabels)
{
  string taskname =  "MomentumSolver::computeNonlinearTerms" +
		     timelabels->integrator_step_name;
  Task* tsk = scinew Task(taskname,
			  this,
			  &MomentumSolver::computeNonlinearTerms,
			  d_lab, timelabels);

  tsk->requires(Task::NewDW, d_lab->d_cellTypeLabel,
		Ghost::AroundCells, Arches::ONEGHOSTCELL);

  tsk->requires(Task::NewDW, timelabels->density_in,
		Ghost::AroundCells, Arches::TWOGHOSTCELLS);
  tsk->requires(Task::NewDW, timelabels->uvelocity_in,
		Ghost::AroundFaces, Arches::ONEGHOSTCELL);
  tsk->requires(Task::NewDW, timelabels->vvelocity_in, 
		Ghost::AroundFaces, Arches::ONEGHOSTCELL);
  tsk->requires(Task::NewDW, timelabels->wvelocity_in, 
		Ghost::AroundFaces, Arches::ONEGHOSTCELL);

  // Computes
  if (timelabels->integrator_step_number == TimeIntegratorStepNumber::First) {
    tsk->computes(d_lab->d_filteredRhoUjULabel, d_lab->d_scalarFluxMatl,
		  Task::OutOfDomain);
    tsk->computes(d_lab->d_filteredRhoUjVLabel, d_lab->d_scalarFluxMatl,
		  Task::OutOfDomain);
    tsk->computes(d_lab->d_filteredRhoUjWLabel, d_lab->d_scalarFluxMatl,
		  Task::OutOfDomain);
  }
  else {
    tsk->modifies(d_lab->d_filteredRhoUjULabel, d_lab->d_scalarFluxMatl,
		  Task::OutOfDomain);
    tsk->modifies(d_lab->d_filteredRhoUjVLabel, d_lab->d_scalarFluxMatl,
		  Task::OutOfDomain);
    tsk->modifies(d_lab->d_filteredRhoUjWLabel, d_lab->d_scalarFluxMatl,
		  Task::OutOfDomain);
  }

  sched->addTask(tsk, patches, matls);
}
//****************************************************************************
//  Computation of and setting boundary conditions for nonlinear terms
//****************************************************************************
void 
MomentumSolver::computeNonlinearTerms(const ProcessorGroup* pc,
					const PatchSubset* patches,
					const MaterialSubset*,
					DataWarehouse*,
					DataWarehouse* new_dw,
					const ArchesLabel* d_lab,
		    		        const TimeIntegratorLabel* timelabels)
{
  for (int p = 0; p < patches->size(); p++) {
    const Patch* patch = patches->get(p);
    int archIndex = 0; // only one arches material
    int matlIndex = d_lab->d_sharedState->getArchesMaterial(archIndex)->getDWIndex(); 
    ArchesVariables velocityVars;
    ArchesConstVariables constVelocityVars;
    int index;

    bool xminus = patch->getBCType(Patch::xminus) != Patch::Neighbor;
    bool xplus =  patch->getBCType(Patch::xplus) != Patch::Neighbor;
    bool yminus = patch->getBCType(Patch::yminus) != Patch::Neighbor;
    bool yplus =  patch->getBCType(Patch::yplus) != Patch::Neighbor;
    bool zminus = patch->getBCType(Patch::zminus) != Patch::Neighbor;
    bool zplus =  patch->getBCType(Patch::zplus) != Patch::Neighbor;

    constSFCXVariable<double> uVelocity;
    constSFCYVariable<double> vVelocity;
    constSFCZVariable<double> wVelocity;
    constCCVariable<double> density;

    new_dw->get(constVelocityVars.cellType, d_lab->d_cellTypeLabel, 
		matlIndex, patch, Ghost::AroundCells, Arches::ONEGHOSTCELL);

    new_dw->get(density, timelabels->density_in,
		matlIndex, patch,
		Ghost::AroundCells, Arches::TWOGHOSTCELLS);
    new_dw->get(uVelocity, timelabels->uvelocity_in,
		matlIndex, patch,
		Ghost::AroundFaces, Arches::ONEGHOSTCELL);
    new_dw->get(vVelocity, timelabels->vvelocity_in, 
		matlIndex, patch,
		Ghost::AroundFaces, Arches::ONEGHOSTCELL);
    new_dw->get(wVelocity, timelabels->wvelocity_in, 
		matlIndex, patch,
		Ghost::AroundFaces, Arches::ONEGHOSTCELL);

    for (int ii = 0; ii < d_lab->d_scalarFluxMatl->size(); ii++) {
      if (timelabels->integrator_step_number ==
			TimeIntegratorStepNumber::First) {
        new_dw->allocateAndPut(velocityVars.filteredRhoUjU[ii], 
			       d_lab->d_filteredRhoUjULabel, ii, patch);
        new_dw->allocateAndPut(velocityVars.filteredRhoUjV[ii], 
			       d_lab->d_filteredRhoUjVLabel, ii, patch);
        new_dw->allocateAndPut(velocityVars.filteredRhoUjW[ii], 
			       d_lab->d_filteredRhoUjWLabel, ii, patch);
      }
      else {
        new_dw->getModifiable(velocityVars.filteredRhoUjU[ii], 
			       d_lab->d_filteredRhoUjULabel, ii, patch);
        new_dw->getModifiable(velocityVars.filteredRhoUjV[ii], 
			       d_lab->d_filteredRhoUjVLabel, ii, patch);
        new_dw->getModifiable(velocityVars.filteredRhoUjW[ii], 
			       d_lab->d_filteredRhoUjWLabel, ii, patch);
      }
      velocityVars.filteredRhoUjU[ii].initialize(0.0);
      velocityVars.filteredRhoUjV[ii].initialize(0.0);
      velocityVars.filteredRhoUjW[ii].initialize(0.0);
    }

    IntVector idxLo;
    IntVector idxHi;
    IntVector idxULo;
    IntVector idxUHi;
    IntVector idxVLo;
    IntVector idxVHi;
    IntVector idxWLo;
    IntVector idxWHi;
    

    idxLo = patch->getSFCXLowIndex();
    idxHi = patch->getSFCXHighIndex();

    if (xminus) idxLo = idxLo + IntVector(2,0,0);
    if (yminus) idxLo = idxLo + IntVector(0,1,0);
    if (zminus) idxLo = idxLo + IntVector(0,0,1);
    if (xplus) idxHi = idxHi - IntVector(1,0,0);

// sizes for computed rhoUU
    idxULo = idxLo;
    idxUHi = idxHi;
    if (yplus) idxUHi = idxUHi - IntVector(0,1,0);
    if (zplus) idxUHi = idxUHi - IntVector(0,0,1);
// sizes for computed rhoVU
    idxVLo = idxLo;
    idxVHi = idxHi;
    if (xplus) idxVHi = idxVHi - IntVector(1,0,0);
    if (zplus) idxVHi = idxVHi - IntVector(0,0,1);
// sizes for computed rhoWU
    idxWLo = idxLo;
    idxWHi = idxHi;
    if (xplus) idxWHi = idxWHi - IntVector(1,0,0);
    if (yplus) idxWHi = idxWHi - IntVector(0,1,0);

    for (int k = idxULo.z(); k < idxUHi.z(); ++k) {
      for (int j = idxULo.y(); j < idxUHi.y(); ++j) {
	for (int i = idxULo.x(); i < idxUHi.x(); ++i) {
	  IntVector idx(i,j,k);
	  IntVector idxminusu(i-1,j,k);
	  IntVector idxminusuminusu(i-2,j,k);
	  (velocityVars.filteredRhoUjU[0])[idx] = 0.125 * 
	   (((density[idx]+density[idxminusu])*uVelocity[idx]+
	    (density[idxminusu]+density[idxminusuminusu])*uVelocity[idxminusu])*
	    (uVelocity[idx]+uVelocity[idxminusu]));
	}
      }
    }
    for (int k = idxVLo.z(); k < idxVHi.z(); ++k) {
      for (int j = idxVLo.y(); j < idxVHi.y(); ++j) {
	for (int i = idxVLo.x(); i < idxVHi.x(); ++i) {
	  IntVector idx(i,j,k);
	  IntVector idxminusu(i-1,j,k);
	  IntVector idxminusv(i,j-1,k);
	  IntVector idxminusuminusv(i-1,j-1,k);
	  (velocityVars.filteredRhoUjU[1])[idx] = 0.125 * 
	   (((density[idx]+density[idxminusv])*vVelocity[idx]+
	    (density[idxminusu]+density[idxminusuminusv])*vVelocity[idxminusu])*
	    (uVelocity[idx]+uVelocity[idxminusv]));
//	if (i == 1) cerr << idx << " " << (velocityVars.filteredRhoUjU[1])[idx] << " " << density[idx] << " " << density[idxminusv] << " " << density[idxminusuminusv] << " " << uVelocity[idx] << " " << uVelocity[idxminusv] << " " << vVelocity[idx] << " " << vVelocity[idxminusu] << endl;
	}
      }
    }
    for (int k = idxWLo.z(); k < idxWHi.z(); ++k) {
      for (int j = idxWLo.y(); j < idxWHi.y(); ++j) {
	for (int i = idxWLo.x(); i < idxWHi.x(); ++i) {
	  IntVector idx(i,j,k);
	  IntVector idxminusu(i-1,j,k);
	  IntVector idxminusw(i,j,k-1);
	  IntVector idxminusuminusw(i-1,j,k-1);
	  (velocityVars.filteredRhoUjU[2])[idx] = 0.125 * 
	   (((density[idx]+density[idxminusw])*wVelocity[idx]+
	    (density[idxminusu]+density[idxminusuminusw])*wVelocity[idxminusu])*
	    (uVelocity[idx]+uVelocity[idxminusw]));
	}
      }
    }

    index = Arches::XDIR;
    d_boundaryCondition->setFluxBC(pc, patch, index, &velocityVars);


    
    idxLo = patch->getSFCYLowIndex();
    idxHi = patch->getSFCYHighIndex();

    if (xminus) idxLo = idxLo + IntVector(1,0,0);
    if (yminus) idxLo = idxLo + IntVector(0,2,0);
    if (zminus) idxLo = idxLo + IntVector(0,0,1);
    if (yplus) idxHi = idxHi - IntVector(0,1,0);

// sizes for computed rhoUV
    idxULo = idxLo;
    idxUHi = idxHi;
    if (yplus) idxUHi = idxUHi - IntVector(0,1,0);
    if (zplus) idxUHi = idxUHi - IntVector(0,0,1);
// sizes for computed rhoVV
    idxVLo = idxLo;
    idxVHi = idxHi;
    if (xplus) idxVHi = idxVHi - IntVector(1,0,0);
    if (zplus) idxVHi = idxVHi - IntVector(0,0,1);
// sizes for computed rhoWV
    idxWLo = idxLo;
    idxWHi = idxHi;
    if (xplus) idxWHi = idxWHi - IntVector(1,0,0);
    if (yplus) idxWHi = idxWHi - IntVector(0,1,0);

    for (int k = idxULo.z(); k < idxUHi.z(); ++k) {
      for (int j = idxULo.y(); j < idxUHi.y(); ++j) {
	for (int i = idxULo.x(); i < idxUHi.x(); ++i) {
	  IntVector idx(i,j,k);
	  IntVector idxminusu(i-1,j,k);
	  IntVector idxminusv(i,j-1,k);
	  IntVector idxminusvminusu(i-1,j-1,k);
	  (velocityVars.filteredRhoUjV[0])[idx] = 0.125 * 
	   (((density[idx]+density[idxminusu])*uVelocity[idx]+
	    (density[idxminusv]+density[idxminusvminusu])*uVelocity[idxminusv])*
	    (vVelocity[idx]+vVelocity[idxminusu]));
	}
      }
    }
    for (int k = idxVLo.z(); k < idxVHi.z(); ++k) {
      for (int j = idxVLo.y(); j < idxVHi.y(); ++j) {
	for (int i = idxVLo.x(); i < idxVHi.x(); ++i) {
	  IntVector idx(i,j,k);
	  IntVector idxminusv(i,j-1,k);
	  IntVector idxminusvminusv(i,j-2,k);
	  (velocityVars.filteredRhoUjV[1])[idx] = 0.125 * 
	   (((density[idx]+density[idxminusv])*vVelocity[idx]+
	    (density[idxminusv]+density[idxminusvminusv])*vVelocity[idxminusv])*
	    (vVelocity[idx]+vVelocity[idxminusv]));
	}
      }
    }
    for (int k = idxWLo.z(); k < idxWHi.z(); ++k) {
      for (int j = idxWLo.y(); j < idxWHi.y(); ++j) {
	for (int i = idxWLo.x(); i < idxWHi.x(); ++i) {
	  IntVector idx(i,j,k);
	  IntVector idxminusv(i,j-1,k);
	  IntVector idxminusw(i,j,k-1);
	  IntVector idxminusvminusw(i,j-1,k-1);
	  (velocityVars.filteredRhoUjV[2])[idx] = 0.125 * 
	   (((density[idx]+density[idxminusw])*wVelocity[idx]+
	    (density[idxminusv]+density[idxminusvminusw])*wVelocity[idxminusv])*
	    (vVelocity[idx]+vVelocity[idxminusw]));
	}
      }
    }

    index = Arches::YDIR;
    d_boundaryCondition->setFluxBC(pc, patch, index, &velocityVars);


    
    idxLo = patch->getSFCZLowIndex();
    idxHi = patch->getSFCZHighIndex();

    if (xminus) idxLo = idxLo + IntVector(1,0,0);
    if (yminus) idxLo = idxLo + IntVector(0,1,0);
    if (zminus) idxLo = idxLo + IntVector(0,0,2);
    if (zplus) idxHi = idxHi - IntVector(0,0,1);

// sizes for computed rhoUW
    idxULo = idxLo;
    idxUHi = idxHi;
    if (yplus) idxUHi = idxUHi - IntVector(0,1,0);
    if (zplus) idxUHi = idxUHi - IntVector(0,0,1);
// sizes for computed rhoVW
    idxVLo = idxLo;
    idxVHi = idxHi;
    if (xplus) idxVHi = idxVHi - IntVector(1,0,0);
    if (zplus) idxVHi = idxVHi - IntVector(0,0,1);
// sizes for computed rhoWW
    idxWLo = idxLo;
    idxWHi = idxHi;
    if (xplus) idxWHi = idxWHi - IntVector(1,0,0);
    if (yplus) idxWHi = idxWHi - IntVector(0,1,0);

    for (int k = idxULo.z(); k < idxUHi.z(); ++k) {
      for (int j = idxULo.y(); j < idxUHi.y(); ++j) {
	for (int i = idxULo.x(); i < idxUHi.x(); ++i) {
	  IntVector idx(i,j,k);
	  IntVector idxminusu(i-1,j,k);
	  IntVector idxminusw(i,j,k-1);
	  IntVector idxminuswminusu(i-1,j,k-1);
	  (velocityVars.filteredRhoUjW[0])[idx] = 0.125 * 
	   (((density[idx]+density[idxminusu])*uVelocity[idx]+
	    (density[idxminusw]+density[idxminuswminusu])*uVelocity[idxminusw])*
	    (wVelocity[idx]+wVelocity[idxminusu]));
	}
      }
    }
    for (int k = idxVLo.z(); k < idxVHi.z(); ++k) {
      for (int j = idxVLo.y(); j < idxVHi.y(); ++j) {
	for (int i = idxVLo.x(); i < idxVHi.x(); ++i) {
	  IntVector idx(i,j,k);
	  IntVector idxminusv(i,j-1,k);
	  IntVector idxminusw(i,j,k-1);
	  IntVector idxminuswminusv(i,j-1,k-1);
	  (velocityVars.filteredRhoUjW[1])[idx] = 0.125 * 
	   (((density[idx]+density[idxminusv])*vVelocity[idx]+
	    (density[idxminusw]+density[idxminuswminusv])*vVelocity[idxminusw])*
	    (wVelocity[idx]+wVelocity[idxminusv]));
	}
      }
    }
    for (int k = idxWLo.z(); k < idxWHi.z(); ++k) {
      for (int j = idxWLo.y(); j < idxWHi.y(); ++j) {
	for (int i = idxWLo.x(); i < idxWHi.x(); ++i) {
	  IntVector idx(i,j,k);
	  IntVector idxminusw(i,j,k-1);
	  IntVector idxminuswminusw(i,j,k-2);
	  (velocityVars.filteredRhoUjW[2])[idx] = 0.125 * 
	   (((density[idx]+density[idxminusw])*wVelocity[idx]+
	    (density[idxminusw]+density[idxminuswminusw])*wVelocity[idxminusw])*
	    (wVelocity[idx]+wVelocity[idxminusw]));
	}
      }
    }

    index = Arches::ZDIR;
    d_boundaryCondition->setFluxBC(pc, patch, index, &velocityVars);

  }
}
//****************************************************************************
//  Filtering of nonlinear terms
//****************************************************************************
void 
MomentumSolver::filterNonlinearTerms(const ProcessorGroup*,
					const Patch* patch,
					int index,
					CellInformation* cellinfo,
					ArchesVariables* vars)

{

  bool xminus = patch->getBCType(Patch::xminus) != Patch::Neighbor;
  bool xplus =  patch->getBCType(Patch::xplus) != Patch::Neighbor;
  bool yminus = patch->getBCType(Patch::yminus) != Patch::Neighbor;
  bool yplus =  patch->getBCType(Patch::yplus) != Patch::Neighbor;
  bool zminus = patch->getBCType(Patch::zminus) != Patch::Neighbor;
  bool zplus =  patch->getBCType(Patch::zplus) != Patch::Neighbor;

  Array3<double> rhoUU;
  Array3<double> rhoVU;
  Array3<double> rhoWU;
  Array3<double> rhoUV;
  Array3<double> rhoVV;
  Array3<double> rhoWV;
  Array3<double> rhoUW;
  Array3<double> rhoVW;
  Array3<double> rhoWW;

  IntVector idxLo;
  IntVector idxHi;
  IntVector idxUnfilteredLo;
  IntVector idxUnfilteredHi;
  IntVector idxFilteredLo;
  IntVector idxFilteredHi;
  IntVector idxFilterComputedLo;
  IntVector idxFilterComputedHi;
  IntVector idxUFilterComputedLo;
  IntVector idxUFilterComputedHi;
  IntVector idxVFilterComputedLo;
  IntVector idxVFilterComputedHi;
  IntVector idxWFilterComputedLo;
  IntVector idxWFilterComputedHi;

  switch (index) {
    case Arches::XDIR:
    
    idxLo = patch->getSFCXLowIndex();
    idxHi = patch->getSFCXHighIndex();

// sizes for unfiltered rhoUU, rhoVU, rhoWU
    idxUnfilteredLo = idxLo;
    idxUnfilteredHi = idxHi;
    if (xminus) idxUnfilteredLo = idxUnfilteredLo + IntVector(2,0,0);
    if (yminus) idxUnfilteredLo = idxUnfilteredLo + IntVector(0,1,0);
    if (zminus) idxUnfilteredLo = idxUnfilteredLo + IntVector(0,0,1);
    if (xplus) idxUnfilteredHi = idxUnfilteredHi - IntVector(1,0,0);
    if (!(xminus)) idxUnfilteredLo = idxUnfilteredLo - IntVector(1,0,0);
    if (!(yminus)) idxUnfilteredLo = idxUnfilteredLo - IntVector(0,1,0);
    if (!(zminus)) idxUnfilteredLo = idxUnfilteredLo - IntVector(0,0,1);
    if (!(xplus)) idxUnfilteredHi = idxUnfilteredHi + IntVector(2,0,0);
    if (!(yplus)) idxUnfilteredHi = idxUnfilteredHi + IntVector(0,2,0);
    if (!(zplus)) idxUnfilteredHi = idxUnfilteredHi + IntVector(0,0,2);

// sizes for filtered rhoUU, rhoVU, rhoWU
    idxFilteredLo = idxUnfilteredLo;
    idxFilteredHi = idxUnfilteredHi;
    if (!(xminus)) idxFilteredLo = idxFilteredLo + IntVector(1,0,0);
    if (!(yminus)) idxFilteredLo = idxFilteredLo + IntVector(0,1,0);
    if (!(zminus)) idxFilteredLo = idxFilteredLo + IntVector(0,0,1);
    if (!(xplus)) idxFilteredHi = idxFilteredHi - IntVector(1,0,0);
    if (!(yplus)) idxFilteredHi = idxFilteredHi - IntVector(0,1,0);
    if (!(zplus)) idxFilteredHi = idxFilteredHi - IntVector(0,0,1);

    idxFilterComputedLo = idxFilteredLo;
    idxFilterComputedHi = idxFilteredHi;
    if (xminus) idxFilterComputedLo= idxFilterComputedLo + IntVector(1,0,0);
    if (yminus) idxFilterComputedLo= idxFilterComputedLo + IntVector(0,1,0);
    if (zminus) idxFilterComputedLo= idxFilterComputedLo + IntVector(0,0,1);
    if (xplus) idxFilterComputedHi = idxFilterComputedHi - IntVector(1,0,0);
    if (yplus) idxFilterComputedHi = idxFilterComputedHi - IntVector(0,1,0);
    if (zplus) idxFilterComputedHi = idxFilterComputedHi - IntVector(0,0,1);

// sizes for rhoUU, where filter is actually applied
    idxUFilterComputedLo = idxFilterComputedLo;
    idxUFilterComputedHi = idxFilterComputedHi;
    if (yplus) idxUFilterComputedHi = idxUFilterComputedHi - IntVector(0,1,0);
    if (zplus) idxUFilterComputedHi = idxUFilterComputedHi - IntVector(0,0,1);
// sizes for rhoVU where filter is actually applied
    idxVFilterComputedLo = idxFilterComputedLo;
    idxVFilterComputedHi = idxFilterComputedHi;
    if (xplus) idxVFilterComputedHi = idxVFilterComputedHi - IntVector(1,0,0);
    if (zplus) idxVFilterComputedHi = idxVFilterComputedHi - IntVector(0,0,1);
// sizes for rhoWU where filter is actually applied
    idxWFilterComputedLo = idxFilterComputedLo;
    idxWFilterComputedHi = idxFilterComputedHi;
    if (xplus) idxWFilterComputedHi = idxWFilterComputedHi - IntVector(1,0,0);
    if (yplus) idxWFilterComputedHi = idxWFilterComputedHi - IntVector(0,1,0);

    rhoUU.resize(idxUnfilteredLo,idxUnfilteredHi);
    rhoVU.resize(idxUnfilteredLo,idxUnfilteredHi);
    rhoWU.resize(idxUnfilteredLo,idxUnfilteredHi);
    rhoUU.initialize(0.0);
    rhoVU.initialize(0.0);
    rhoWU.initialize(0.0);

  //  vars->filteredRhoUjU[0].print(cerr);
  //  vars->filteredRhoUjU[1].print(cerr);
  //  vars->filteredRhoUjU[2].print(cerr);
    for (int k = idxUnfilteredLo.z(); k < idxUnfilteredHi.z(); ++k) {
      for (int j = idxUnfilteredLo.y(); j < idxUnfilteredHi.y(); ++j) {
	for (int i = idxUnfilteredLo.x(); i < idxUnfilteredHi.x(); ++i) {
	  IntVector idx(i,j,k);
	  rhoUU[idx] = (vars->filteredRhoUjU[0])[idx];
	  rhoVU[idx] = (vars->filteredRhoUjU[1])[idx];
	  rhoWU[idx] = (vars->filteredRhoUjU[2])[idx];
	}
      }
    }

    for (int k = idxUFilterComputedLo.z(); k < idxUFilterComputedHi.z(); ++k) {
      for (int j = idxUFilterComputedLo.y(); j < idxUFilterComputedHi.y(); ++j) {
	for (int i = idxUFilterComputedLo.x(); i < idxUFilterComputedHi.x(); ++i){
	  IntVector currCell(i,j,k);
	  double cube_delta = (2.0*cellinfo->sewu[i])*
			      (2.0*cellinfo->sns[j])*
                 	      (2.0*cellinfo->stb[k]);
	  double invDelta = 1.0/cube_delta;
          (vars->filteredRhoUjU[0])[currCell] = 0.0;
	  
	  for (int kk = -1; kk <= 1; kk ++) {
	    for (int jj = -1; jj <= 1; jj ++) {
	      for (int ii = -1; ii <= 1; ii ++) {
		IntVector filterCell = IntVector(i+ii,j+jj,k+kk);
		double vol = cellinfo->sewu[i] * cellinfo->sns[j] *
			     cellinfo->stb[k] *
		             (1.0-0.5*abs(ii))*
		             (1.0-0.5*abs(jj))*(1.0-0.5*abs(kk));
		(vars->filteredRhoUjU[0])[currCell] += rhoUU[filterCell]*vol;  
	      }
	    }
	  }
	  (vars->filteredRhoUjU[0])[currCell] *= invDelta;
	}
      }
    }
    for (int k = idxVFilterComputedLo.z(); k < idxVFilterComputedHi.z(); ++k) {
      for (int j = idxVFilterComputedLo.y(); j < idxVFilterComputedHi.y(); ++j) {
	for (int i = idxVFilterComputedLo.x(); i < idxVFilterComputedHi.x(); ++i){
	  IntVector currCell(i,j,k);
	  double cube_delta = (2.0*cellinfo->sewu[i])*
			      (2.0*cellinfo->sns[j])*
                 	      (2.0*cellinfo->stb[k]);
	  double invDelta = 1.0/cube_delta;
          (vars->filteredRhoUjU[1])[currCell] = 0.0;
	  
	  for (int kk = -1; kk <= 1; kk ++) {
	    for (int jj = -1; jj <= 1; jj ++) {
	      for (int ii = -1; ii <= 1; ii ++) {
		IntVector filterCell = IntVector(i+ii,j+jj,k+kk);
		double vol = cellinfo->sewu[i] * cellinfo->sns[j] *
			     cellinfo->stb[k] *
		             (1.0-0.5*abs(ii))*
		             (1.0-0.5*abs(jj))*(1.0-0.5*abs(kk));
		(vars->filteredRhoUjU[1])[currCell] += rhoVU[filterCell]*vol;  
	      }
	    }
	  }
	  (vars->filteredRhoUjU[1])[currCell] *= invDelta;
	}
      }
    }
    for (int k = idxWFilterComputedLo.z(); k < idxWFilterComputedHi.z(); ++k) {
      for (int j = idxWFilterComputedLo.y(); j < idxWFilterComputedHi.y(); ++j) {
	for (int i = idxWFilterComputedLo.x(); i < idxWFilterComputedHi.x(); ++i){
	  IntVector currCell(i,j,k);
	  double cube_delta = (2.0*cellinfo->sewu[i])*
			      (2.0*cellinfo->sns[j])*
                 	      (2.0*cellinfo->stb[k]);
	  double invDelta = 1.0/cube_delta;
          (vars->filteredRhoUjU[2])[currCell] = 0.0;
	  
	  for (int kk = -1; kk <= 1; kk ++) {
	    for (int jj = -1; jj <= 1; jj ++) {
	      for (int ii = -1; ii <= 1; ii ++) {
		IntVector filterCell = IntVector(i+ii,j+jj,k+kk);
		double vol = cellinfo->sewu[i] * cellinfo->sns[j] *
			     cellinfo->stb[k] *
		             (1.0-0.5*abs(ii))*
		             (1.0-0.5*abs(jj))*(1.0-0.5*abs(kk));
		(vars->filteredRhoUjU[2])[currCell] += rhoWU[filterCell]*vol;  
	      }
	    }
	  }
	  (vars->filteredRhoUjU[2])[currCell] *= invDelta;
	}
      }
    }
    //vars->filteredRhoUjU[0].print(cerr);
    //vars->filteredRhoUjU[1].print(cerr);
    //vars->filteredRhoUjU[2].print(cerr);

    break;

    case Arches::YDIR:
    
    idxLo = patch->getSFCYLowIndex();
    idxHi = patch->getSFCYHighIndex();

// sizes for unfiltered rhoUV, rhoVV, rhoWV
    idxUnfilteredLo = idxLo;
    idxUnfilteredHi = idxHi;
    if (xminus) idxUnfilteredLo = idxUnfilteredLo + IntVector(1,0,0);
    if (yminus) idxUnfilteredLo = idxUnfilteredLo + IntVector(0,2,0);
    if (zminus) idxUnfilteredLo = idxUnfilteredLo + IntVector(0,0,1);
    if (yplus) idxUnfilteredHi = idxUnfilteredHi - IntVector(0,1,0);
    if (!(xminus)) idxUnfilteredLo = idxUnfilteredLo - IntVector(1,0,0);
    if (!(yminus)) idxUnfilteredLo = idxUnfilteredLo - IntVector(0,1,0);
    if (!(zminus)) idxUnfilteredLo = idxUnfilteredLo - IntVector(0,0,1);
    if (!(xplus)) idxUnfilteredHi = idxUnfilteredHi + IntVector(2,0,0);
    if (!(yplus)) idxUnfilteredHi = idxUnfilteredHi + IntVector(0,2,0);
    if (!(zplus)) idxUnfilteredHi = idxUnfilteredHi + IntVector(0,0,2);

// sizes for filtered rhoUV, rhoVV, rhoWV
    idxFilteredLo = idxUnfilteredLo;
    idxFilteredHi = idxUnfilteredHi;
    if (!(xminus)) idxFilteredLo = idxFilteredLo + IntVector(1,0,0);
    if (!(yminus)) idxFilteredLo = idxFilteredLo + IntVector(0,1,0);
    if (!(zminus)) idxFilteredLo = idxFilteredLo + IntVector(0,0,1);
    if (!(xplus)) idxFilteredHi = idxFilteredHi - IntVector(1,0,0);
    if (!(yplus)) idxFilteredHi = idxFilteredHi - IntVector(0,1,0);
    if (!(zplus)) idxFilteredHi = idxFilteredHi - IntVector(0,0,1);

    idxFilterComputedLo = idxFilteredLo;
    idxFilterComputedHi = idxFilteredHi;
    if (xminus) idxFilterComputedLo= idxFilterComputedLo + IntVector(1,0,0);
    if (yminus) idxFilterComputedLo= idxFilterComputedLo + IntVector(0,1,0);
    if (zminus) idxFilterComputedLo= idxFilterComputedLo + IntVector(0,0,1);
    if (xplus) idxFilterComputedHi = idxFilterComputedHi - IntVector(1,0,0);
    if (yplus) idxFilterComputedHi = idxFilterComputedHi - IntVector(0,1,0);
    if (zplus) idxFilterComputedHi = idxFilterComputedHi - IntVector(0,0,1);

// sizes for rhoUV, where filter is actually applied
    idxUFilterComputedLo = idxFilterComputedLo;
    idxUFilterComputedHi = idxFilterComputedHi;
    if (yplus) idxUFilterComputedHi = idxUFilterComputedHi - IntVector(0,1,0);
    if (zplus) idxUFilterComputedHi = idxUFilterComputedHi - IntVector(0,0,1);
// sizes for rhoVV, where filter is actually applied
    idxVFilterComputedLo = idxFilterComputedLo;
    idxVFilterComputedHi = idxFilterComputedHi;
    if (xplus) idxVFilterComputedHi = idxVFilterComputedHi - IntVector(1,0,0);
    if (zplus) idxVFilterComputedHi = idxVFilterComputedHi - IntVector(0,0,1);
// sizes for rhoWV where filter is actually applied
    idxWFilterComputedLo = idxFilterComputedLo;
    idxWFilterComputedHi = idxFilterComputedHi;
    if (xplus) idxWFilterComputedHi = idxWFilterComputedHi - IntVector(1,0,0);
    if (yplus) idxWFilterComputedHi = idxWFilterComputedHi - IntVector(0,1,0);

    rhoUV.resize(idxUnfilteredLo,idxUnfilteredHi);
    rhoVV.resize(idxUnfilteredLo,idxUnfilteredHi);
    rhoWV.resize(idxUnfilteredLo,idxUnfilteredHi);
    rhoUV.initialize(0.0);
    rhoVV.initialize(0.0);
    rhoWV.initialize(0.0);

    for (int k = idxUnfilteredLo.z(); k < idxUnfilteredHi.z(); ++k) {
      for (int j = idxUnfilteredLo.y(); j < idxUnfilteredHi.y(); ++j) {
	for (int i = idxUnfilteredLo.x(); i < idxUnfilteredHi.x(); ++i) {
	  IntVector idx(i,j,k);
          rhoUV[idx] = (vars->filteredRhoUjV[0])[idx];
          rhoVV[idx] = (vars->filteredRhoUjV[1])[idx];
          rhoWV[idx] = (vars->filteredRhoUjV[2])[idx];
	}
      }
    }

    for (int k = idxUFilterComputedLo.z(); k < idxUFilterComputedHi.z(); ++k) {
      for (int j = idxUFilterComputedLo.y(); j < idxUFilterComputedHi.y(); ++j) {
	for (int i = idxUFilterComputedLo.x(); i < idxUFilterComputedHi.x(); ++i){
	  IntVector currCell(i,j,k);
	  double cube_delta = (2.0*cellinfo->sew[i])*
			      (2.0*cellinfo->snsv[j])*
                 	      (2.0*cellinfo->stb[k]);
	  double invDelta = 1.0/cube_delta;
          (vars->filteredRhoUjV[0])[currCell] = 0.0;
	  
	  for (int kk = -1; kk <= 1; kk ++) {
	    for (int jj = -1; jj <= 1; jj ++) {
	      for (int ii = -1; ii <= 1; ii ++) {
		IntVector filterCell = IntVector(i+ii,j+jj,k+kk);
		double vol = cellinfo->sew[i] * cellinfo->snsv[j] *
			     cellinfo->stb[k] *
		             (1.0-0.5*abs(ii))*
		             (1.0-0.5*abs(jj))*(1.0-0.5*abs(kk));
		(vars->filteredRhoUjV[0])[currCell] += rhoUV[filterCell]*vol;  
	      }
	    }
	  }
	  (vars->filteredRhoUjV[0])[currCell] *= invDelta;
	}
      }
    }
    for (int k = idxVFilterComputedLo.z(); k < idxVFilterComputedHi.z(); ++k) {
      for (int j = idxVFilterComputedLo.y(); j < idxVFilterComputedHi.y(); ++j) {
	for (int i = idxVFilterComputedLo.x(); i < idxVFilterComputedHi.x(); ++i){
	  IntVector currCell(i,j,k);
	  double cube_delta = (2.0*cellinfo->sew[i])*
			      (2.0*cellinfo->snsv[j])*
                 	      (2.0*cellinfo->stb[k]);
	  double invDelta = 1.0/cube_delta;
          (vars->filteredRhoUjV[1])[currCell] = 0.0;
	  
	  for (int kk = -1; kk <= 1; kk ++) {
	    for (int jj = -1; jj <= 1; jj ++) {
	      for (int ii = -1; ii <= 1; ii ++) {
		IntVector filterCell = IntVector(i+ii,j+jj,k+kk);
		double vol = cellinfo->sew[i] * cellinfo->snsv[j] *
			     cellinfo->stb[k] *
		             (1.0-0.5*abs(ii))*
		             (1.0-0.5*abs(jj))*(1.0-0.5*abs(kk));
		(vars->filteredRhoUjV[1])[currCell] += rhoVV[filterCell]*vol;  
	      }
	    }
	  }
	  (vars->filteredRhoUjV[1])[currCell] *= invDelta;
	}
      }
    }
    for (int k = idxWFilterComputedLo.z(); k < idxWFilterComputedHi.z(); ++k) {
      for (int j = idxWFilterComputedLo.y(); j < idxWFilterComputedHi.y(); ++j) {
	for (int i = idxWFilterComputedLo.x(); i < idxWFilterComputedHi.x(); ++i){
	  IntVector currCell(i,j,k);
	  double cube_delta = (2.0*cellinfo->sew[i])*
			      (2.0*cellinfo->snsv[j])*
                 	      (2.0*cellinfo->stb[k]);
	  double invDelta = 1.0/cube_delta;
          (vars->filteredRhoUjV[2])[currCell] = 0.0;
	  
	  for (int kk = -1; kk <= 1; kk ++) {
	    for (int jj = -1; jj <= 1; jj ++) {
	      for (int ii = -1; ii <= 1; ii ++) {
		IntVector filterCell = IntVector(i+ii,j+jj,k+kk);
		double vol = cellinfo->sew[i] * cellinfo->snsv[j] *
			     cellinfo->stb[k] *
		             (1.0-0.5*abs(ii))*
		             (1.0-0.5*abs(jj))*(1.0-0.5*abs(kk));
		(vars->filteredRhoUjV[2])[currCell] += rhoWV[filterCell]*vol;  
	      }
	    }
	  }
	  (vars->filteredRhoUjV[2])[currCell] *= invDelta;
	}
      }
    }

    break;

    case Arches::ZDIR:
    
    idxLo = patch->getSFCZLowIndex();
    idxHi = patch->getSFCZHighIndex();

// sizes for unfiltered rhoUW, rhoVW, rhoWW
    idxUnfilteredLo = idxLo;
    idxUnfilteredHi = idxHi;
    if (xminus) idxUnfilteredLo = idxUnfilteredLo + IntVector(1,0,0);
    if (yminus) idxUnfilteredLo = idxUnfilteredLo + IntVector(0,1,0);
    if (zminus) idxUnfilteredLo = idxUnfilteredLo + IntVector(0,0,2);
    if (zplus) idxUnfilteredHi = idxUnfilteredHi - IntVector(0,0,1);
    if (!(xminus)) idxUnfilteredLo = idxUnfilteredLo - IntVector(1,0,0);
    if (!(yminus)) idxUnfilteredLo = idxUnfilteredLo - IntVector(0,1,0);
    if (!(zminus)) idxUnfilteredLo = idxUnfilteredLo - IntVector(0,0,1);
    if (!(xplus)) idxUnfilteredHi = idxUnfilteredHi + IntVector(2,0,0);
    if (!(yplus)) idxUnfilteredHi = idxUnfilteredHi + IntVector(0,2,0);
    if (!(zplus)) idxUnfilteredHi = idxUnfilteredHi + IntVector(0,0,2);

// sizes for filtered rhoUW, rhoVW, rhoWW
    idxFilteredLo = idxUnfilteredLo;
    idxFilteredHi = idxUnfilteredHi;
    if (!(xminus)) idxFilteredLo = idxFilteredLo + IntVector(1,0,0);
    if (!(yminus)) idxFilteredLo = idxFilteredLo + IntVector(0,1,0);
    if (!(zminus)) idxFilteredLo = idxFilteredLo + IntVector(0,0,1);
    if (!(xplus)) idxFilteredHi = idxFilteredHi - IntVector(1,0,0);
    if (!(yplus)) idxFilteredHi = idxFilteredHi - IntVector(0,1,0);
    if (!(zplus)) idxFilteredHi = idxFilteredHi - IntVector(0,0,1);

    idxFilterComputedLo = idxFilteredLo;
    idxFilterComputedHi = idxFilteredHi;
    if (xminus) idxFilterComputedLo= idxFilterComputedLo + IntVector(1,0,0);
    if (yminus) idxFilterComputedLo= idxFilterComputedLo + IntVector(0,1,0);
    if (zminus) idxFilterComputedLo= idxFilterComputedLo + IntVector(0,0,1);
    if (xplus) idxFilterComputedHi = idxFilterComputedHi - IntVector(1,0,0);
    if (yplus) idxFilterComputedHi = idxFilterComputedHi - IntVector(0,1,0);
    if (zplus) idxFilterComputedHi = idxFilterComputedHi - IntVector(0,0,1);

// sizes for rhoUW, where filter is actually applied
    idxUFilterComputedLo = idxFilterComputedLo;
    idxUFilterComputedHi = idxFilterComputedHi;
    if (yplus) idxUFilterComputedHi = idxUFilterComputedHi - IntVector(0,1,0);
    if (zplus) idxUFilterComputedHi = idxUFilterComputedHi - IntVector(0,0,1);
// sizes for rhoVW, where filter is actually applied
    idxVFilterComputedLo = idxFilterComputedLo;
    idxVFilterComputedHi = idxFilterComputedHi;
    if (xplus) idxVFilterComputedHi = idxVFilterComputedHi - IntVector(1,0,0);
    if (zplus) idxVFilterComputedHi = idxVFilterComputedHi - IntVector(0,0,1);
// sizes for rhoWW, where filter is actually applied
    idxWFilterComputedLo = idxFilterComputedLo;
    idxWFilterComputedHi = idxFilterComputedHi;
    if (xplus) idxWFilterComputedHi = idxWFilterComputedHi - IntVector(1,0,0);
    if (yplus) idxWFilterComputedHi = idxWFilterComputedHi - IntVector(0,1,0);

    rhoUW.resize(idxUnfilteredLo,idxUnfilteredHi);
    rhoVW.resize(idxUnfilteredLo,idxUnfilteredHi);
    rhoWW.resize(idxUnfilteredLo,idxUnfilteredHi);
    rhoUW.initialize(0.0);
    rhoVW.initialize(0.0);
    rhoWW.initialize(0.0);

    for (int k = idxUnfilteredLo.z(); k < idxUnfilteredHi.z(); ++k) {
      for (int j = idxUnfilteredLo.y(); j < idxUnfilteredHi.y(); ++j) {
	for (int i = idxUnfilteredLo.x(); i < idxUnfilteredHi.x(); ++i) {
	  IntVector idx(i,j,k);
          rhoUW[idx] = (vars->filteredRhoUjW[0])[idx];
          rhoVW[idx] = (vars->filteredRhoUjW[1])[idx];
          rhoWW[idx] = (vars->filteredRhoUjW[2])[idx];
	}
      }
    }

    for (int k = idxUFilterComputedLo.z(); k < idxUFilterComputedHi.z(); ++k) {
      for (int j = idxUFilterComputedLo.y(); j < idxUFilterComputedHi.y(); ++j) {
	for (int i = idxUFilterComputedLo.x(); i < idxUFilterComputedHi.x(); ++i){
	  IntVector currCell(i,j,k);
	  double cube_delta = (2.0*cellinfo->sew[i])*
			      (2.0*cellinfo->sns[j])*
                 	      (2.0*cellinfo->stbw[k]);
	  double invDelta = 1.0/cube_delta;
          (vars->filteredRhoUjW[0])[currCell] = 0.0;
	  
	  for (int kk = -1; kk <= 1; kk ++) {
	    for (int jj = -1; jj <= 1; jj ++) {
	      for (int ii = -1; ii <= 1; ii ++) {
		IntVector filterCell = IntVector(i+ii,j+jj,k+kk);
		double vol = cellinfo->sew[i] * cellinfo->sns[j] *
			     cellinfo->stbw[k] *
		             (1.0-0.5*abs(ii))*
		             (1.0-0.5*abs(jj))*(1.0-0.5*abs(kk));
		(vars->filteredRhoUjW[0])[currCell] += rhoUW[filterCell]*vol;  
	      }
	    }
	  }
	  (vars->filteredRhoUjW[0])[currCell] *= invDelta;
	}
      }
    }
    for (int k = idxVFilterComputedLo.z(); k < idxVFilterComputedHi.z(); ++k) {
      for (int j = idxVFilterComputedLo.y(); j < idxVFilterComputedHi.y(); ++j) {
	for (int i = idxVFilterComputedLo.x(); i < idxVFilterComputedHi.x(); ++i){
	  IntVector currCell(i,j,k);
	  double cube_delta = (2.0*cellinfo->sew[i])*
			      (2.0*cellinfo->sns[j])*
                 	      (2.0*cellinfo->stbw[k]);
	  double invDelta = 1.0/cube_delta;
          (vars->filteredRhoUjW[1])[currCell] = 0.0;
	  
	  for (int kk = -1; kk <= 1; kk ++) {
	    for (int jj = -1; jj <= 1; jj ++) {
	      for (int ii = -1; ii <= 1; ii ++) {
		IntVector filterCell = IntVector(i+ii,j+jj,k+kk);
		double vol = cellinfo->sew[i] * cellinfo->sns[j] *
			     cellinfo->stbw[k] *
		             (1.0-0.5*abs(ii))*
		             (1.0-0.5*abs(jj))*(1.0-0.5*abs(kk));
		(vars->filteredRhoUjW[1])[currCell] += rhoVW[filterCell]*vol;  
	      }
	    }
	  }
	  (vars->filteredRhoUjW[1])[currCell] *= invDelta;
	}
      }
    }
    for (int k = idxWFilterComputedLo.z(); k < idxWFilterComputedHi.z(); ++k) {
      for (int j = idxWFilterComputedLo.y(); j < idxWFilterComputedHi.y(); ++j) {
	for (int i = idxWFilterComputedLo.x(); i < idxWFilterComputedHi.x(); ++i){
	  IntVector currCell(i,j,k);
	  double cube_delta = (2.0*cellinfo->sew[i])*
			      (2.0*cellinfo->sns[j])*
                 	      (2.0*cellinfo->stbw[k]);
	  double invDelta = 1.0/cube_delta;
          (vars->filteredRhoUjW[2])[currCell] = 0.0;
	  
	  for (int kk = -1; kk <= 1; kk ++) {
	    for (int jj = -1; jj <= 1; jj ++) {
	      for (int ii = -1; ii <= 1; ii ++) {
		IntVector filterCell = IntVector(i+ii,j+jj,k+kk);
		double vol = cellinfo->sew[i] * cellinfo->sns[j] *
			     cellinfo->stbw[k] *
		             (1.0-0.5*abs(ii))*
		             (1.0-0.5*abs(jj))*(1.0-0.5*abs(kk));
		(vars->filteredRhoUjW[2])[currCell] += rhoWW[filterCell]*vol;  
	      }
	    }
	  }
	  (vars->filteredRhoUjW[2])[currCell] *= invDelta;
	}
      }
    }

    break;

    default:
	  throw InvalidValue("Invalid index in MomentumSolver::filterNterms");
  }
}
