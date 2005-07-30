/*--------------------------------------------------------------------------
 * File: HypreSolverAMR.h
 *
 * Header file for the interface to Hyupre's semi-structured matrix interface
 * and corresponding solvers.
 * class HypreSolverAMG is a wrapper, receiving Uintah data for the elliptic
 * pressure equation in implicit ICE AMR mode, calling a Hypre solver,
 * and returning the pressure results into Uintah. HypreSolverAMG schedules
 * a task in sus called scheduleSolve() that carries out these operations.
 * It is based on the one-level solver class HypreSolver2.
 *--------------------------------------------------------------------------*/
#ifndef Packages_Uintah_CCA_Components_Solvers_HypreSolverAMR_h
#define Packages_Uintah_CCA_Components_Solvers_HypreSolverAMR_h

#include <Packages/Uintah/CCA/Ports/SolverInterface.h>
#include <Packages/Uintah/Core/Parallel/UintahParallelComponent.h>

namespace Uintah {
  class HypreSolverAMG : public SolverInterface, public UintahParallelComponent { 
  public:
    HypreSolverAMG(const ProcessorGroup* myworld);
    virtual ~HypreSolverAMG();

    /* Load solver parameters from input struct */
    virtual SolverParameters* readParameters(ProblemSpecP& params,
                                             const std::string& name);

    /* Main task that solves the pressure equation and returns
       cell-centered pressure. In the future we can also add here
       solutions of other variable types, like node-centered. */
    virtual void scheduleSolve(const LevelP& level, SchedulerP& sched,
                               const MaterialSet* matls,
                               const VarLabel* A,    
                               Task::WhichDW which_A_dw,  
                               const VarLabel* x,
                               bool modifies_x,
                               const VarLabel* b,    
                               Task::WhichDW which_b_dw,  
                               const VarLabel* guess,
                               Task::WhichDW guess_dw,
                               const SolverParameters* params);
  private:
  };
}

#endif // Packages_Uintah_CCA_Components_Solvers_HypreSolverAMR_h

