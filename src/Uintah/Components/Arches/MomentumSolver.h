//----- MomentumSolver.h -----------------------------------------------

#ifndef Uintah_Components_Arches_MomentumSolver_h
#define Uintah_Components_Arches_MomentumSolver_h

/**************************************
CLASS
   MomentumSolver
   
   Class MomentumSolver linearizes and solves momentum
   equation on a grid hierarchy


GENERAL INFORMATION
   MomentumSolver.h - declaration of the class
   
   Author: Rajesh Rawat (rawat@crsim.utah.edu)
   
   Creation Date:   Mar 1, 2000
   
   C-SAFE 
   
   Copyright U of U 2000

KEYWORDS


DESCRIPTION
   Class MomentumSolver linearizes and solves momentum
   equation on a grid hierarchy


WARNING
   none

************************************************************************/
#include <Uintah/Interface/SchedulerP.h>
#include <Uintah/Interface/ProblemSpecP.h>
#include <Uintah/Interface/DataWarehouseP.h>
#include <Uintah/Grid/LevelP.h>
#include <Uintah/Grid/Patch.h>
#include <Uintah/Grid/VarLabel.h>

namespace Uintah {
   class ProcessorGroup;
namespace ArchesSpace {

class TurbulenceModel;
class PhysicalConstants;
class Discretization;
class Source;
class BoundaryCondition;
class LinearSolver;

class MomentumSolver {

public:

      // GROUP: Constructors:
      ////////////////////////////////////////////////////////////////////////
      //
      // Construct an instance of the Momentum solver.
      //
      // PRECONDITIONS
      //
      // POSTCONDITIONS
      //   A linear level solver is partially constructed.  
      //
      MomentumSolver(TurbulenceModel* turb_model, 
		     BoundaryCondition* bndry_cond,
		     PhysicalConstants* physConst);

      // GROUP: Destructors:
      ////////////////////////////////////////////////////////////////////////
      //
      // Destructor
      //
      ~MomentumSolver();

      // GROUP: Problem Setup :
      ///////////////////////////////////////////////////////////////////////
      //
      // Set up the problem specification database
      //
      void problemSetup(const ProblemSpecP& params);

      // GROUP: Schedule Action :
      ///////////////////////////////////////////////////////////////////////
      //
      // Schedule Solve of the linearized momentum equation. 
      //
      void solve(const LevelP& level,
		 SchedulerP& sched,
		 DataWarehouseP& old_dw,
		 DataWarehouseP& new_dw, 
		 double time, double delta_t, int index);
   
      ///////////////////////////////////////////////////////////////////////
      //
      // Schedule the build of the linearized momentum matrix
      //
      void sched_buildLinearMatrix(const LevelP& level,
				   SchedulerP& sched,
				   DataWarehouseP& old_dw,
				   DataWarehouseP& new_dw,
				   double delta_t, int index);

protected: 

private:

      // GROUP: Constructors (private):
      ////////////////////////////////////////////////////////////////////////
      //
      // Default constructor.
      MomentumSolver();

      // GROUP: Action Methods (private):
      ///////////////////////////////////////////////////////////////////////
      //
      // Actually build the linearized momentum matrix
      //
      void buildLinearMatrix(const ProcessorGroup* pc,
			     const Patch* patch,
			     DataWarehouseP& old_dw,
			     DataWarehouseP& new_dw,
			     double delta_t, int index);

private:

      // computes coefficients
      Discretization* d_discretize;
      // computes sources
      Source* d_source;
      // linear solver
      LinearSolver* d_linearSolver;
      // turbulence model
      TurbulenceModel* d_turbModel;
      // boundary condition
      BoundaryCondition* d_boundaryCondition;
      // physical constants
      PhysicalConstants* d_physicalConsts;

      // const VarLabel* (required)
      const VarLabel* d_pressurePSLabel;
      const VarLabel* d_uVelocityCPBCLabel;
      const VarLabel* d_vVelocityCPBCLabel;
      const VarLabel* d_wVelocityCPBCLabel;
      const VarLabel* d_densityCPLabel;
      const VarLabel* d_viscosityCTSLabel;

      // const VarLabel* (computed)
      const VarLabel* d_uVelConvCoefMBLMLabel;
      const VarLabel* d_vVelConvCoefMBLMLabel;
      const VarLabel* d_wVelConvCoefMBLMLabel;
      const VarLabel* d_uVelCoefMBLMLabel;
      const VarLabel* d_vVelCoefMBLMLabel;
      const VarLabel* d_wVelCoefMBLMLabel;
      const VarLabel* d_uVelLinSrcMBLMLabel;
      const VarLabel* d_vVelLinSrcMBLMLabel;
      const VarLabel* d_wVelLinSrcMBLMLabel;
      const VarLabel* d_uVelNonLinSrcMBLMLabel;
      const VarLabel* d_vVelNonLinSrcMBLMLabel;
      const VarLabel* d_wVelNonLinSrcMBLMLabel;

      // DataWarehouse generation
      int d_generation;

}; // End class MomentumSolver

}  // End namespace ArchesSpace
}  // End namespace Uintah

#endif

//
// $Log$
// Revision 1.10  2000/07/03 05:30:15  bbanerje
// Minor changes for inlbcs dummy code to compile and work. densitySIVBC is no more.
//
// Revision 1.9  2000/06/22 23:06:34  bbanerje
// Changed velocity related variables to FCVariable type.
// ** NOTE ** We may need 3 types of FCVariables (one for each direction)
//
// Revision 1.8  2000/06/18 01:20:15  bbanerje
// Changed names of varlabels in source to reflect the sequence of tasks.
// Result : Seg Violation in addTask in MomentumSolver
//
// Revision 1.7  2000/06/17 07:06:24  sparker
// Changed ProcessorContext to ProcessorGroup
//
// Revision 1.6  2000/06/07 06:13:54  bbanerje
// Changed CCVariable<Vector> to CCVariable<double> for most cases.
// Some of these variables may not be 3D Vectors .. they may be Stencils
// or more than 3D arrays. Need help here.
//
// Revision 1.5  2000/06/04 22:40:13  bbanerje
// Added Cocoon stuff, changed task, require, compute, get, put arguments
// to reflect new declarations. Changed sub.mk to include all the new files.
//
//
