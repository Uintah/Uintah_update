
// $Id$

#include <Uintah/Components/Schedulers/TaskGraph.h>
#include <Uintah/Interface/DataWarehouse.h>
#include <Uintah/Exceptions/TypeMismatchException.h>
#include <Uintah/Grid/Patch.h>
#include <Uintah/Grid/Task.h>
#include <Uintah/Grid/TypeDescription.h>

#include <SCICore/Exceptions/InternalError.h>
#include <SCICore/Malloc/Allocator.h>
#include <SCICore/Util/DebugStream.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <strstream>
#include <unistd.h>

using namespace Uintah;

using SCICore::Exceptions::InternalError;
using namespace std;
static SCICore::Util::DebugStream dbg("MPIScheduler", false);

TaskGraph::TaskGraph()
{
}

TaskGraph::~TaskGraph()
{
   vector<Task*>::iterator iter;

   for( iter=d_tasks.begin(); iter != d_tasks.end(); iter++ )
      delete *iter;
}

void
TaskGraph::initialize()
{
   vector<Task*>::iterator iter;

   for( iter=d_tasks.begin(); iter != d_tasks.end(); iter++ )
      delete *iter;

   d_tasks.clear();
   d_allcomps.clear();
}

void
TaskGraph::setupTaskConnections()
{
   vector<Task*>::iterator iter;
   // Initialize variables on the tasks
   for( iter=d_tasks.begin(); iter != d_tasks.end(); iter++ ) {
      Task* task = *iter;
      task->visited=false;
      task->sorted=false;
   }

   // Look for all of the reduction variables - we must treat those
   // special.  Create a fake task that performs the reduction
   // While we are at it, ensure that we aren't producing anything
   // into a frozen data warehouse
   map<const VarLabel*, Task*, VarLabel::Compare> reductionTasks;
   for( iter=d_tasks.begin(); iter != d_tasks.end(); iter++ ) {
      Task* task = *iter;
      const vector<Task::Dependency*>& comps = task->getComputes();
      for(vector<Task::Dependency*>::const_iterator iter = comps.begin();
	  iter != comps.end(); iter++){
	 Task::Dependency* dep = *iter;
	 if(dep->d_dw->isFinalized()){
	    throw InternalError("Variable produced in old datawarehouse: "+dep->d_var->getName());
	 } else if(dep->d_var->typeDescription()->isReductionVariable()){
	    // Look up this variable in the reductionTasks map
	    const VarLabel* var = dep->d_var;
	    map<const VarLabel*, Task*, VarLabel::Compare>::iterator it=reductionTasks.find(var);
	    if(it == reductionTasks.end()){
	       reductionTasks[var]=new Task(var->getName()+" reduction");
	       it = reductionTasks.find(var);
	       it->second->computes(dep->d_dw, var, -1, 0);
	    }
	    it->second->requires(dep->d_dw, var, -1, task->getPatch(),
				 Ghost::None);
	 }
      }
   }

   // Add the new reduction tasks to the list of tasks
   for(map<const VarLabel*, Task*, VarLabel::Compare>::iterator it = reductionTasks.begin();
       it != reductionTasks.end(); it++){
      addTask(it->second);
   }

   // Connect the tasks together using the computes/requires info
   // Also do a type check
   for( iter=d_tasks.begin(); iter != d_tasks.end(); iter++ ) {
      Task* task = *iter;
      const vector<Task::Dependency*>& reqs = task->getRequires();
      for(vector<Task::Dependency*>::const_iterator iter = reqs.begin();
	  iter != reqs.end(); iter++){
	 Task::Dependency* dep = *iter;
	 if(dep->d_dw->isFinalized()){
	    if(!dep->d_dw->exists(dep->d_var, dep->d_patch))
	       throw InternalError("Variable required from old datawarehouse, but it does not exist: "+dep->d_var->getName());
	 } else {
	    TaskProduct p(dep->d_patch, dep->d_matlIndex, dep->d_var);
	    map<TaskProduct, Task*>::iterator aciter = d_allcomps.find(p);
	    if(aciter == d_allcomps.end())
	       throw InternalError("Scheduler could not find production for variable: "+dep->d_var->getName()+", required for task: "+task->getName());
	    if(dep->d_var->typeDescription() != aciter->first.getLabel()->typeDescription())
	       throw TypeMismatchException("Type mismatch for variable: "+dep->d_var->getName());
	 }
      }
   }
}

void
TaskGraph::processTask(Task* task, vector<Task*>& sortedTasks) const
{
   dbg << "Looking at task: " << task->getName();
   if(task->getPatch())
      dbg << " on patch " << task->getPatch()->getID();
   dbg << '\n';

   if(task->visited){
      ostrstream error;
      error << "Cycle detected in task graph: already did\n\t"
            << task->getName();
      if(task->getPatch())
	 error << " on patch " << task->getPatch()->getID();
      error << "\n";
      throw InternalError(error.str());
   }

   task->visited=true;
   const vector<Task::Dependency*>& reqs = task->getRequires();
   for(vector<Task::Dependency*>::const_iterator iter = reqs.begin();
       iter != reqs.end(); iter++){
      Task::Dependency* dep = *iter;
      if(!dep->d_dw->isFinalized()){
	 TaskProduct p(dep->d_patch, dep->d_matlIndex, dep->d_var);
	 map<TaskProduct, Task*>::const_iterator aciter = d_allcomps.find(p);
	 if(!aciter->second->sorted){
	    if(aciter->second->visited){
	       ostrstream error;
	       error << "Cycle detected in task graph: trying to do\n\t"
		     << task->getName();
	       if(task->getPatch())
		  error << " on patch " << task->getPatch()->getID();
	       error << "\nbut already did:\n\t"
		     << aciter->second->getName();
	       if(aciter->second->getPatch())
		  error << " on patch " << aciter->second->getPatch()->getID();
	       error << ",\nwhile looking for variable: \n\t" 
		     << dep->d_var->getName() << ", material " 
		     << dep->d_matlIndex;
	       if(dep->d_patch)
		  error << ", patch " << dep->d_patch->getID();
	       error << "\n";
	       throw InternalError(error.str());
	    }
	    processTask(aciter->second, sortedTasks);
	 }
      }
   }
   // All prerequisites are done - add this task to the list
   sortedTasks.push_back(task);
   task->sorted=true;
   dbg << "Added task: " << task->getName();
   if(task->getPatch())
      dbg << " on patch " << task->getPatch()->getID();
}

void
TaskGraph::topologicalSort(vector<Task*>& sortedTasks)
{
   setupTaskConnections();
   dumpDependencies();

   vector<Task*>::iterator iter;
   for( iter=d_tasks.begin(); iter != d_tasks.end(); iter++ ) {
      Task* task = *iter;
      if(!task->sorted){
	 processTask(task, sortedTasks);
      }
   }
}

void
TaskGraph::addTask(Task* task)
{
   d_tasks.push_back(task);
 
   const vector<Task::Dependency*>& comps = task->getComputes();
   for(vector<Task::Dependency*>::const_iterator iter = comps.begin();
       iter != comps.end(); iter++){
      Task::Dependency* dep = *iter;
      TaskProduct p(dep->d_patch, dep->d_matlIndex, dep->d_var);
      map<TaskProduct,Task*>::iterator aciter = d_allcomps.find(p);
      if(aciter != d_allcomps.end()) 
	 throw InternalError("Two tasks compute the same result: "+dep->d_var->getName()+" (tasks: "+task->getName()+" and "+aciter->second->getName()+")");
      d_allcomps[p] = task;
   }
}

bool
TaskGraph::allDependenciesCompleted(Task*) const
{
    //cerr << "TaskGraph::allDependenciesCompleted broken!\n";
    return true;
}

void
TaskGraph::dumpDependencies()
{
    static int call_nr = 0;
    
    // the first call is just some initialization tasks. All subsequent calls
    // will have the same dependency graph, modulo an output task. The first
    // non-initialization call will have the output task, so we'll just output
    // that one.
    if (call_nr++ != 1)
    	return;
	
    ofstream depfile("dependencies");
    if (!depfile) {
	cerr << "TaskGraph::dumpDependencies: unable to open output file!\n";
	return;	// dependency dump failure shouldn't be fatal to anything else
    }

    vector<Task*>::const_iterator iter;
    for (iter = d_tasks.begin(); iter != d_tasks.end(); iter++) {
    	const Task* task = *iter;

	const vector<Task::Dependency*>& deps = task->getRequires();
	vector<Task::Dependency*>::const_iterator dep_iter;
	for (dep_iter = deps.begin(); dep_iter != deps.end(); dep_iter++) {
	    const Task::Dependency* dep = *dep_iter;

	    if (!dep->d_dw->isFinalized()) {

		TaskProduct p(dep->d_patch, dep->d_matlIndex, dep->d_var);
		map<TaskProduct, Task*>::const_iterator deptask =
	    	    d_allcomps.find(p);

		const Task* task1 = task;
		const Task* task2 = deptask->second;

		depfile << "\"" << task1->getName();
		if(task1->getPatch())
		   depfile << "\\nPatch" << task1->getPatch()->getID();
		depfile << "\" \""  << task2->getName();
		if(task2->getPatch())
		   depfile << "\\nPatch" << task2->getPatch()->getID();
		depfile << "\"" << endl;
	    }
	}
    }

    depfile.close();
}

int TaskGraph::getNumTasks() const
{
   return (int)d_tasks.size();
}

Task* TaskGraph::getTask(int idx)
{
   return d_tasks[idx];
}

//
// $Log$
// Revision 1.1  2000/06/17 07:04:55  sparker
// Implemented initial load balancer modules
// Use ProcessorGroup
// Implemented TaskGraph - to contain the common scheduling stuff
//
//
