//----- PicardNonlinearSolver.cc ----------------------------------------------

/* REFERENCED */
static char *id="@(#) $Id$";

#include <Uintah/Components/Arches/PicardNonlinearSolver.h>
#include <Uintah/Components/Arches/Arches.h>
#include <Uintah/Components/Arches/Properties.h>
#include <Uintah/Components/Arches/BoundaryCondition.h>
#include <Uintah/Components/Arches/TurbulenceModel.h>
#include <Uintah/Components/Arches/PhysicalConstants.h>
#include <Uintah/Components/Arches/PressureSolver.h>
#include <Uintah/Components/Arches/MomentumSolver.h>
#include <Uintah/Components/Arches/ScalarSolver.h>
#include <Uintah/Interface/ProblemSpec.h>
#include <Uintah/Interface/Scheduler.h>
#include <Uintah/Interface/DataWarehouse.h>
#include <Uintah/Grid/Level.h>
#include <Uintah/Grid/Task.h>
#include <Uintah/Grid/CCVariable.h>
#include <Uintah/Grid/SFCXVariable.h>
#include <Uintah/Grid/SFCYVariable.h>
#include <Uintah/Grid/SFCZVariable.h>
#include <Uintah/Exceptions/InvalidValue.h>
#include <SCICore/Util/NotFinished.h>
#include <math.h>

using namespace Uintah::ArchesSpace;

//****************************************************************************
// Default constructor for PicardNonlinearSolver
//****************************************************************************
PicardNonlinearSolver::
PicardNonlinearSolver(const ArchesLabel* label, 
		      Properties* props, 
		      BoundaryCondition* bc,
		      TurbulenceModel* turbModel,
		      PhysicalConstants* physConst): NonlinearSolver(),
		       d_lab(label), d_props(props), 
		       d_boundaryCondition(bc), d_turbModel(turbModel),
		       d_physicalConsts(physConst)
{
}

//****************************************************************************
// Destructor
//****************************************************************************
PicardNonlinearSolver::~PicardNonlinearSolver()
{
}

//****************************************************************************
// Problem Setup 
//****************************************************************************
void 
PicardNonlinearSolver::problemSetup(const ProblemSpecP& params)
{
  ProblemSpecP db = params->findBlock("PicardSolver");
  db->require("max_iter", d_nonlinear_its);
  
  // ** WARNING ** temporarily commented out
  // dw->put(nonlinear_its, "max_nonlinear_its");

  db->require("res_tol", d_resTol);
  int nDim = 3;
  //db->require("num_dim", nDim);
  bool calPress;
  db->require("cal_pressure", calPress);
  if (calPress) {
    d_pressSolver = scinew PressureSolver(d_lab,
					  d_turbModel, d_boundaryCondition,
					  d_physicalConsts);
    d_pressSolver->problemSetup(db);
  }
  bool calMom;
  db->require("cal_momentum", calMom);
  if (calMom) {
    d_momSolver = scinew MomentumSolver(d_lab, d_turbModel, d_boundaryCondition,
				     d_physicalConsts);
    d_momSolver->problemSetup(db);
  }
  bool calScalar;
  db->require("cal_mixturescalar", calScalar);
  if (calScalar) {
    d_scalarSolver = scinew ScalarSolver(d_lab, d_turbModel, d_boundaryCondition,
				      d_physicalConsts);
    d_scalarSolver->problemSetup(db);
  }
}

//****************************************************************************
// Schedule non linear solve and carry out some actual operations
//****************************************************************************
int PicardNonlinearSolver::nonlinearSolve(const LevelP& level,
					  SchedulerP& sched,
					  DataWarehouseP& old_dw,
					  DataWarehouseP& new_dw,
					  double time, double delta_t)
{
  //initializes and allocates vars for new_dw
  // set initial guess
  // require : old_dw -> pressureSPBC, [u,v,w]velocitySPBC, scalarSP, densityCP,
  //                     viscosityCTS
  // compute : new_dw -> pressureIN, [u,v,w]velocityIN, scalarIN, densityIN,
  //                     viscosityIN
  sched_setInitialGuess(level, sched, old_dw, new_dw);

  // Save the initial guess
  new_dw->pleaseSave(d_lab->d_pressureINLabel, 1);
  new_dw->pleaseSave(d_lab->d_uVelocityINLabel, 1);
  new_dw->pleaseSave(d_lab->d_vVelocityINLabel, 1);
  new_dw->pleaseSave(d_lab->d_wVelocityINLabel, 1);
  int nofScalars = d_props->getNumMixVars();
  new_dw->pleaseSave(d_lab->d_scalarINLabel, nofScalars);
  new_dw->pleaseSave(d_lab->d_densityINLabel, 1);
  new_dw->pleaseSave(d_lab->d_viscosityINLabel, 1);

  // Start the iterations
  int nlIterations = 0;
  double nlResidual = 2.0*d_resTol;;
  do{
    //correct inlet velocities to account for change in properties
    // require : densityIN, [u,v,w]VelocityIN (new_dw)
    // compute : [u,v,w]VelocitySIVBC
    d_boundaryCondition->sched_setInletVelocityBC(level, sched, old_dw, new_dw);

    // linearizes and solves pressure eqn
    // require : pressureIN, densityIN, viscosityIN,
    //           [u,v,w]VelocitySIVBC (new_dw)
    //           [u,v,w]VelocitySPBC, densityCP (old_dw)
    // compute : [u,v,w]VelConvCoefPBLM, [u,v,w]VelCoefPBLM, 
    //           [u,v,w]VelLinSrcPBLM, [u,v,w]VelNonLinSrcPBLM, (matrix_dw)
    //           presResidualPS, presCoefPBLM, presNonLinSrcPBLM,(matrix_dw)
    //           pressurePS (new_dw)
    d_pressSolver->solve(level, sched, old_dw, new_dw, time, delta_t);

    // Save the calculated pressure
    new_dw->pleaseSave(d_lab->d_pressurePSLabel, 1);

    // if external boundary then recompute velocities using new pressure
    // and puts them in nonlinear_dw
    // require : densityCP, pressurePS, [u,v,w]VelocitySIVBC
    // compute : [u,v,w]VelocityCPBC, pressureSPBC
    d_boundaryCondition->sched_recomputePressureBC(level, sched, old_dw,
						 new_dw);

    // Momentum solver
    // require : pressureSPBC, [u,v,w]VelocityCPBC, densityIN, 
    // viscosityIN (new_dw)
    //           [u,v,w]VelocitySPBC, densityCP (old_dw)
    // compute : [u,v,w]VelCoefPBLM, [u,v,w]VelConvCoefPBLM
    //           [u,v,w]VelLinSrcPBLM, [u,v,w]VelNonLinSrcPBLM
    //           [u,v,w]VelResidualMS, [u,v,w]VelCoefMS, 
    //           [u,v,w]VelNonLinSrcMS, [u,v,w]VelLinSrcMS,
    //           [u,v,w]VelocitySPBC
    for (int index = 1; index <= Arches::NDIM; ++index) {
      d_momSolver->solve(level, sched, old_dw, new_dw, time, delta_t, index);
    }
    
    // Save the calculated velocities
    new_dw->pleaseSave(d_lab->d_uVelocitySPBCLabel, 1);
    new_dw->pleaseSave(d_lab->d_vVelocitySPBCLabel, 1);
    new_dw->pleaseSave(d_lab->d_wVelocitySPBCLabel, 1);

    // equation for scalars
    // require : scalarIN, [u,v,w]VelocitySPBC, densityIN, viscosityIN (new_dw)
    //           scalarSP, densityCP (old_dw)
    // compute : scalarCoefSBLM, scalarLinSrcSBLM, scalarNonLinSrcSBLM
    //           scalResidualSS, scalCoefSS, scalNonLinSrcSS, scalarSS
    for (int index = 0;index < nofScalars; index ++) {
      // in this case we're only solving for one scalar...but
      // the same subroutine can be used to solve multiple scalars
      d_scalarSolver->solve(level, sched, old_dw, new_dw, time, delta_t, index);
    }

    // Save the scalars
    new_dw->pleaseSave(d_lab->d_scalarSPLabel, nofScalars);

    // update properties
    // require : densityIN
    // compute : densityCP
    d_props->sched_reComputeProps(level, sched, old_dw, new_dw);

    // Save the density
    new_dw->pleaseSave(d_lab->d_densityCPLabel, 1);

    // LES Turbulence model to compute turbulent viscosity
    // that accounts for sub-grid scale turbulence
    // require : densityCP, viscosityIN, [u,v,w]VelocitySPBC
    // compute : viscosityCTS
    d_turbModel->sched_reComputeTurbSubmodel(level, sched, old_dw, new_dw);

    // Save the viscosity
    new_dw->pleaseSave(d_lab->d_viscosityCTSLabel, 1);


    ++nlIterations;

    // residual represents the degrees of inaccuracies
    nlResidual = computeResidual(level, sched, old_dw, new_dw);

  }while((nlIterations < d_nonlinear_its)&&(nlResidual > d_resTol));

  return(0);
}

//****************************************************************************
// Schedule initialize 
//****************************************************************************
void 
PicardNonlinearSolver::sched_setInitialGuess(const LevelP& level,
					     SchedulerP& sched,
					     DataWarehouseP& old_dw,
					     DataWarehouseP& new_dw)
{
  for(Level::const_patchIterator iter=level->patchesBegin();
      iter != level->patchesEnd(); iter++){
    const Patch* patch=*iter;
    {
      //copies old db to new_db and then uses non-linear
      //solver to compute new values
      Task* tsk = scinew Task("PicardNonlinearSolver::initialGuess",patch,
			   old_dw, new_dw, this,
			   &PicardNonlinearSolver::setInitialGuess);
      int numGhostCells = 0;
      int matlIndex = 0;
      tsk->requires(old_dw, d_lab->d_cellTypeLabel, matlIndex, patch, 
		    Ghost::None, numGhostCells);
      tsk->requires(old_dw, d_lab->d_pressureSPBCLabel, matlIndex, patch, 
		    Ghost::None, numGhostCells);
      tsk->requires(old_dw, d_lab->d_uVelocitySPBCLabel, matlIndex, patch, 
		    Ghost::None, numGhostCells);
      tsk->requires(old_dw, d_lab->d_vVelocitySPBCLabel, matlIndex, patch, 
		    Ghost::None, numGhostCells);
      tsk->requires(old_dw, d_lab->d_wVelocitySPBCLabel, matlIndex, patch, 
		    Ghost::None, numGhostCells);

      int nofScalars = d_props->getNumMixVars();
      for (int ii = 0; ii < nofScalars; ii++) {
	tsk->requires(old_dw, d_lab->d_scalarSPLabel, ii, patch, 
		      Ghost::None, numGhostCells);
      }
      tsk->requires(old_dw, d_lab->d_densityCPLabel, matlIndex, patch, 
		    Ghost::None, numGhostCells);
      tsk->requires(old_dw, d_lab->d_viscosityCTSLabel, matlIndex, patch, 
		    Ghost::None, numGhostCells);
      tsk->computes(new_dw, d_lab->d_cellTypeLabel, matlIndex, patch);
      tsk->computes(new_dw, d_lab->d_pressureINLabel, matlIndex, patch);
      tsk->computes(new_dw, d_lab->d_uVelocityINLabel, matlIndex, patch);
      tsk->computes(new_dw, d_lab->d_vVelocityINLabel, matlIndex, patch);
      tsk->computes(new_dw, d_lab->d_wVelocityINLabel, matlIndex, patch);
      for (int ii = 0; ii < nofScalars; ii++) {
	tsk->computes(new_dw, d_lab->d_scalarINLabel, ii, patch);
      }
      tsk->computes(new_dw, d_lab->d_densityINLabel, matlIndex, patch);
      tsk->computes(new_dw, d_lab->d_viscosityINLabel, matlIndex, patch);

      sched->addTask(tsk);
    }
  }
}
//****************************************************************************
// Actual initialize 
//****************************************************************************
void 
PicardNonlinearSolver::setInitialGuess(const ProcessorGroup* ,
				       const Patch* patch,
				       DataWarehouseP& old_dw,
				       DataWarehouseP& new_dw)
{
  // Get the pressure, velocity, scalars, density and viscosity from the
  // old datawarehouse
  int matlIndex = 0;
  int nofGhostCells = 0;
  CCVariable<int> cellType;
  old_dw->get(cellType, d_lab->d_cellTypeLabel, matlIndex, patch,
	      Ghost::None, nofGhostCells);
  CCVariable<double> pressure;
  old_dw->get(pressure, d_lab->d_pressureSPBCLabel, matlIndex, patch, 
	      Ghost::None, nofGhostCells);

  SFCXVariable<double> uVelocity;
  old_dw->get(uVelocity, d_lab->d_uVelocitySPBCLabel, matlIndex, patch, 
	      Ghost::None, nofGhostCells);
  SFCYVariable<double> vVelocity;
  old_dw->get(vVelocity, d_lab->d_vVelocitySPBCLabel, matlIndex, patch, 
	      Ghost::None, nofGhostCells);
  SFCZVariable<double> wVelocity;
  old_dw->get(wVelocity, d_lab->d_wVelocitySPBCLabel, matlIndex, patch, 
	      Ghost::None, nofGhostCells);

  int nofScalars = d_props->getNumMixVars();
  vector<CCVariable<double> > scalar(nofScalars);
  for (int ii = 0; ii < nofScalars; ii++) {
    old_dw->get(scalar[ii], d_lab->d_scalarSPLabel, ii, patch, 
		Ghost::None, nofGhostCells);
  }

  CCVariable<double> density;
  old_dw->get(density, d_lab->d_densityCPLabel, matlIndex, patch, 
	      Ghost::None, nofGhostCells);

  CCVariable<double> viscosity;
  old_dw->get(viscosity, d_lab->d_viscosityCTSLabel, matlIndex, patch, 
	      Ghost::None, nofGhostCells);


  // Create vars for new_dw
  CCVariable<int> cellType_new;
  new_dw->allocate(cellType_new, d_lab->d_cellTypeLabel, matlIndex, patch);
  cellType_new = cellType;
    // Get the PerPatch CellInformation data
  PerPatch<CellInformation*> cellInfoP;
  cellInfoP.setData(scinew CellInformation(patch));
  new_dw->put(cellInfoP, d_lab->d_cellInfoLabel, matlIndex, patch);

  CCVariable<double> pressure_new;
  new_dw->allocate(pressure_new, d_lab->d_pressureINLabel, matlIndex, patch);
  pressure_new = pressure; // copy old into new

  SFCXVariable<double> uVelocity_new;
  new_dw->allocate(uVelocity_new, d_lab->d_uVelocityINLabel, matlIndex, patch);
  uVelocity_new = uVelocity; // copy old into new
  SFCYVariable<double> vVelocity_new;
  new_dw->allocate(vVelocity_new, d_lab->d_vVelocityINLabel, matlIndex, patch);
  vVelocity_new = vVelocity; // copy old into new
  SFCZVariable<double> wVelocity_new;
  new_dw->allocate(wVelocity_new, d_lab->d_wVelocityINLabel, matlIndex, patch);
  wVelocity_new = wVelocity; // copy old into new

  vector<CCVariable<double> > scalar_new(nofScalars);
  for (int ii = 0; ii < nofScalars; ii++) {
    new_dw->allocate(scalar_new[ii], d_lab->d_scalarINLabel, ii, patch);
    scalar_new[ii] = scalar[ii]; // copy old into new
  }

  CCVariable<double> density_new;
  new_dw->allocate(density_new, d_lab->d_densityINLabel, matlIndex, patch);
  density_new = density; // copy old into new

  CCVariable<double> viscosity_new;
  new_dw->allocate(viscosity_new, d_lab->d_viscosityINLabel, matlIndex, patch);
  viscosity_new = viscosity; // copy old into new

  // Copy the variables into the new datawarehouse
  new_dw->put(cellType_new, d_lab->d_cellTypeLabel, matlIndex, patch);
  new_dw->put(pressure_new, d_lab->d_pressureINLabel, matlIndex, patch);
  new_dw->put(uVelocity_new, d_lab->d_uVelocityINLabel, matlIndex, patch);
  new_dw->put(vVelocity_new, d_lab->d_vVelocityINLabel, matlIndex, patch);
  new_dw->put(wVelocity_new, d_lab->d_wVelocityINLabel, matlIndex, patch);
  for (int ii = 0; ii < nofScalars; ii++) {
    new_dw->put(scalar_new[ii], d_lab->d_scalarINLabel, ii, patch);
  }
  new_dw->put(density_new, d_lab->d_densityINLabel, matlIndex, patch);
  new_dw->put(viscosity_new, d_lab->d_viscosityINLabel, matlIndex, patch);
}

//****************************************************************************
// compute the residual
//****************************************************************************
double 
PicardNonlinearSolver::computeResidual(const LevelP& level,
				       SchedulerP& sched,
				       DataWarehouseP& old_dw,
				       DataWarehouseP& new_dw)
{
  double nlresidual = 0.0;
#if 0
  SoleVariable<double> residual;
  SoleVariable<double> omg;
  // not sure of the syntax...this operation is supposed to get 
  // L1norm of the residual over the whole level
  new_dw->get(residual,"pressResidual");
  new_dw->get(omg,"pressomg");
  nlresidual = MACHINEPRECISSION + log(residual/omg);
  for (int index = 1; index <= Arches::NDIM; ++index) {
    new_dw->get(residual,"velocityResidual", index);
    new_dw->get(omg,"velocityomg", index);
    nlresidual = max(nlresidual, MACHINEPRECISSION+log(residual/omg));
  }
  //for multiple scalars iterate
  for (int index = 1;index <= d_props->getNumMixVars(); index ++) {
    new_dw->get(residual,"scalarResidual", index);
    new_dw->get(omg,"scalaromg", index);
    nlresidual = max(nlresidual, MACHINEPRECISSION+log(residual/omg));
  }
#endif
  return nlresidual;
}


//
// $Log$
// Revision 1.42  2000/08/16 19:36:40  bbanerje
// Changed second argument in pleaseSave from matlIndex to numMaterials.
//
// Revision 1.41  2000/08/15 05:10:15  bbanerje
// Added pleaseSave after each solve.
//
// Revision 1.40  2000/08/10 21:29:09  rawat
// fixed a bug in cellinformation
//
// Revision 1.39  2000/08/08 23:34:18  rawat
// fixed some bugs in profv.F and Properties.cc
//
// Revision 1.38  2000/08/01 06:18:37  bbanerje
// Made ScalarSolver similar to PressureSolver and MomentumSolver.
//
// Revision 1.37  2000/07/28 02:31:00  rawat
// moved all the labels in ArchesLabel. fixed some bugs and added matrix_dw to store matrix
// coeffecients
//
// Revision 1.36  2000/07/13 06:32:10  bbanerje
// Labels are once more consistent for one iteration.
//
// Revision 1.35  2000/07/11 15:46:27  rawat
// added setInitialGuess in PicardNonlinearSolver and also added uVelSrc
//
// Revision 1.34  2000/07/08 08:03:34  bbanerje
// Readjusted the labels upto uvelcoef, removed bugs in CellInformation,
// made needed changes to uvelcoef.  Changed from StencilMatrix::AE etc
// to Arches::AE .. doesn't like enums in templates apparently.
//
// Revision 1.33  2000/07/03 05:30:15  bbanerje
// Minor changes for inlbcs dummy code to compile and work. densitySIVBC is no more.
//
// Revision 1.32  2000/07/02 05:47:30  bbanerje
// Uncommented all PerPatch and CellInformation stuff.
// Updated array sizes in inlbcs.F
//
// Revision 1.31  2000/06/29 22:56:43  bbanerje
// Changed FCVars to SFC[X,Y,Z]Vars, and added the neceesary getIndex calls.
//
// Revision 1.30  2000/06/22 23:06:35  bbanerje
// Changed velocity related variables to FCVariable type.
// ** NOTE ** We may need 3 types of FCVariables (one for each direction)
//
// Revision 1.29  2000/06/21 07:51:00  bbanerje
// Corrected new_dw, old_dw problems, commented out intermediate dw (for now)
// and made the stuff go through schedule_time_advance.
//
// Revision 1.28  2000/06/21 06:49:21  bbanerje
// Straightened out some of the problems in data location .. still lots to go.
//
// Revision 1.27  2000/06/18 01:20:15  bbanerje
// Changed names of varlabels in source to reflect the sequence of tasks.
// Result : Seg Violation in addTask in MomentumSolver
//
// Revision 1.26  2000/06/17 07:06:24  sparker
// Changed ProcessorContext to ProcessorGroup
//
// Revision 1.25  2000/06/16 07:06:16  bbanerje
// Added init of props, pressure bcs and turbulence model in Arches.cc
// Changed duplicate task names (setProfile) in BoundaryCondition.cc
// Commented out nolinear_dw creation in PicardNonlinearSolver.cc
//
// Revision 1.24  2000/06/16 04:25:40  bbanerje
// Uncommented BoundaryCondition related stuff.
//
// Revision 1.23  2000/06/14 20:40:49  rawat
// modified boundarycondition for physical boundaries and
// added CellInformation class
//
// Revision 1.22  2000/06/13 06:02:31  bbanerje
// Added some more StencilMatrices and vector<CCVariable> types.
//
// Revision 1.21  2000/06/07 06:13:55  bbanerje
// Changed CCVariable<Vector> to CCVariable<double> for most cases.
// Some of these variables may not be 3D Vectors .. they may be Stencils
// or more than 3D arrays. Need help here.
//
// Revision 1.20  2000/06/04 23:57:46  bbanerje
// Updated Arches to do ScheduleTimeAdvance.
//
// Revision 1.19  2000/06/04 22:40:14  bbanerje
// Added Cocoon stuff, changed task, require, compute, get, put arguments
// to reflect new declarations. Changed sub.mk to include all the new files.
//
//
