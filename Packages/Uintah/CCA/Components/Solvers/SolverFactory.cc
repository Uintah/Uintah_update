#include <Packages/Uintah/CCA/Components/Solvers/SolverFactory.h>
#include <Packages/Uintah/CCA/Components/Solvers/CGSolver.h>
#include <Packages/Uintah/CCA/Components/Solvers/DirectSolve.h>
#include <sci_defs/hypre_defs.h>
#include <Packages/Uintah/CCA/Components/Solvers/HypreSolver.h>
#include <Packages/Uintah/CCA/Components/Solvers/AMRSolver.h>
#include <Packages/Uintah/Core/Parallel/ProcessorGroup.h>
#include <Packages/Uintah/Core/Exceptions/ProblemSetupException.h>


#include <iostream>
using std::cerr;
using std::endl;

using namespace Uintah;

SolverInterface* SolverFactory::create(ProblemSpecP& ps,
                                       const ProcessorGroup* world,
                                       string cmdline)
{
  string solver = "CGSolver";;
  if (cmdline == "") {
    ProblemSpecP sol_ps = ps->findBlock("Solver");
    if (sol_ps)
      sol_ps->get("type",solver);
  }
  else
    solver = cmdline;

  SolverInterface* solve = 0;
  if(solver == "CGSolver") {
    solve = new CGSolver(world);
  } else if (solver == "DirectSolve") {
    solve = new DirectSolve(world);
  } else if (solver == "HypreSolver" || solver == "hypre") {
#if HAVE_HYPRE
    solve = new HypreSolver2(world);
#else
    cerr << "Hypre solver not available, hypre not configured\n";
    exit(1);
#endif
  } else if (solver == "AMRSolver" || solver == "hypreamr") {
#if HAVE_HYPRE
    cerr << "Creating AMRSolver object" << "\n";
    solve = new AMRSolver(world);
#else
    cerr << "Hypre 1.9.0b solver not available, hypre not configured\n";
    exit(1);
#endif
  } else {
    ostringstream msg;
    msg << "Unknown solver " << solver
        << "Valid Solvers: CGSolver, DirectSolver, HypreSolver, AMGSolver";
    throw ProblemSetupException(msg.str(),__FILE__, __LINE__);
  }

  return solve;
}
