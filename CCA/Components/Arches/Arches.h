//----- Arches.h -----------------------------------------------

#ifndef Uintah_Component_Arches_Arches_h
#define Uintah_Component_Arches_Arches_h

/**************************************

CLASS
   Arches
   
   Short description...

GENERAL INFORMATION

   Arches.h

   Author: Rajesh Rawat (rawat@crsim.utah.edu)
   Department of Chemical Engineering
   University of Utah

   Center for the Simulation of Accidental Fires and Explosions (C-SAFE)
  
   Copyright (C) 2000 University of Utah

KEYWORDS
   Arches

DESCRIPTION
   Long description...
  
WARNING
  
****************************************/

#include <Packages/Uintah/Core/Parallel/UintahParallelComponent.h>
#include <Packages/Uintah/CCA/Ports/SimulationInterface.h>
#include <Packages/Uintah/Core/Grid/SimulationStateP.h>
#include <Packages/Uintah/Core/ProblemSpec/ProblemSpecP.h>
#include <Packages/Uintah/Core/ProblemSpec/Handle.h>
#include <Packages/Uintah/Core/Grid/GridP.h>
#include <Packages/Uintah/Core/Grid/LevelP.h>
#include <Packages/Uintah/Core/Grid/ComputeSet.h>

// Choices for time advance methods


//#define correctorstep

//#define Runge_Kutta_2nd
#ifdef Runge_Kutta_2nd
  #define correctorstep
#endif 

//#define Runge_Kutta_3d  //Does NOT work
#ifdef Runge_Kutta_3d
  #define correctorstep
#endif 

//#define Runge_Kutta_3d_ssp
#ifdef Runge_Kutta_3d_ssp
  #define Runge_Kutta_3d
  #define correctorstep
#endif 

// Divergence constraint instead of drhodt in pressure equation
//#define divergenceconstraint

// Exact Initialization for first time step in
// MPMArches problem, to eliminate problem of
// sudden appearance of mass in second step
//
// #define ExactMPMArchesInitialize

// Choices of scheme for convection of scalar

//#define Scalar_ENO

//#define Scalar_WENO
#ifdef Scalar_WENO
  #define Scalar_ENO
#endif

// Choices of scheme for convection of velocity
// So far only central differencing is implemented for filtered convection terms

//#define filter_convection_terms

#ifdef HAVE_PETSC
  #define PetscFilter
#ifdef PetscFilter
#define FILTER_DRHODT
#endif
#endif
namespace Uintah {

  class VarLabel;
  class PhysicalConstants;
  class NonlinearSolver;
  class Properties;
  class TurbulenceModel;
  class BoundaryCondition;
  class MPMArchesLabel;
  class ArchesLabel;
#ifdef PetscFilter
  class Filter;
#endif

class Arches : public UintahParallelComponent, public SimulationInterface {

public:

      // GROUP: Static Variables:
      ////////////////////////////////////////////////////////////////////////
      // Number of dimensions in the problem
      static const int NDIM;

      // GROUP: Constants:
      ////////////////////////////////////////////////////////////////////////
      enum d_eqnType { PRESSURE, MOMENTUM, SCALAR };
      enum d_dirName { NODIR, XDIR, YDIR, ZDIR };
      enum d_stencilName { AP, AE, AW, AN, AS, AT, AB };
      enum d_numGhostCells {ZEROGHOSTCELLS , ONEGHOSTCELL, TWOGHOSTCELLS,
			    THREEGHOSTCELLS, FOURGHOSTCELLS, FIVEGHOSTCELLS };
      enum d_Runge_Kutta_current_step {FIRST, SECOND, THIRD};

      // GROUP: Constructors:
      ////////////////////////////////////////////////////////////////////////
      // Arches constructor
      Arches(const ProcessorGroup* myworld);

      // GROUP: Destructors:
      ////////////////////////////////////////////////////////////////////////
      // Virtual destructor 
      virtual ~Arches();

      // GROUP: Problem Setup :
      ///////////////////////////////////////////////////////////////////////
      // Set up the problem specification database
      virtual void problemSetup(const ProblemSpecP& params, 
				GridP& grid,
				SimulationStateP&);

      // GROUP: Schedule Action :
      ///////////////////////////////////////////////////////////////////////
      // Schedule initialization
      virtual void scheduleInitialize(const LevelP& level,
				      SchedulerP&);
	 
      ///////////////////////////////////////////////////////////////////////
      // Schedule parameter initialization
      virtual void sched_paramInit(const LevelP& level,
				   SchedulerP&);
      
      ///////////////////////////////////////////////////////////////////////
      // Schedule Compute if Stable time step
      virtual void scheduleComputeStableTimestep(const LevelP& level,
						 SchedulerP&);

      ///////////////////////////////////////////////////////////////////////
      // Schedule time advance
      virtual void scheduleTimeAdvance(const LevelP& level, 
				       SchedulerP&);


      // for multimaterial
      void setMPMArchesLabel(const MPMArchesLabel* MAlb)
	{
	  d_MAlab = MAlb;
	}

      const ArchesLabel* getArchesLabel()
	{
	  return d_lab;
	}
      NonlinearSolver* getNonlinearSolver()
	{
	  return d_nlSolver;
	}

      BoundaryCondition* getBoundaryCondition()
	{
	  return d_boundaryCondition;
	}

      TurbulenceModel* getTurbulenceModel()
	{
	  return d_turbModel;
	}


protected:

private:

      // GROUP: Constructors (Private):
      ////////////////////////////////////////////////////////////////////////
      // Default Arches constructor
      Arches();

      ////////////////////////////////////////////////////////////////////////
      // Arches copy constructor
      Arches(const Arches&);

      // GROUP: Overloaded Operators (Private):
      ////////////////////////////////////////////////////////////////////////
      // Arches assignment constructor
      Arches& operator=(const Arches&);

      // GROUP: Action Methods (Private):
      ////////////////////////////////////////////////////////////////////////
      // Arches assignment constructor
      void paramInit(const ProcessorGroup*,
		     const PatchSubset* patches,
		     const MaterialSubset*,
		     DataWarehouse* ,
		     DataWarehouse* new_dw );

      void computeStableTimeStep(const ProcessorGroup* ,
				 const PatchSubset* patches,
				 const MaterialSubset* matls,
				 DataWarehouse* ,
				 DataWarehouse* new_dw);

private:

      double d_deltaT;
      int d_nofScalars;
      int d_nofScalarStats;
      bool d_variableTimeStep;
      bool d_reactingFlow;
      bool d_calcReactingScalar;
      bool d_calcEnthalpy;
      PhysicalConstants* d_physicalConsts;
      NonlinearSolver* d_nlSolver;
      // properties...solves density, temperature and species concentrations
      Properties* d_props;
      // Turbulence Model
      TurbulenceModel* d_turbModel;
      // Boundary conditions
      BoundaryCondition* d_boundaryCondition;
      SimulationStateP d_sharedState;
      // Variable labels that are used by the simulation controller
      ArchesLabel* d_lab;
      // for multimaterial
      const MPMArchesLabel* d_MAlab;
#ifdef PetscFilter
      Filter* d_filter;
#endif

      bool nofTimeSteps;
#ifdef multimaterialform
      MultiMaterialInterface* d_mmInterface;
#endif

}; // end class Arches

} // End namespace Uintah

#endif

