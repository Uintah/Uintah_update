/* REFERENCED */
static char *id="@(#) $Id$";

#include <Uintah/Grid/Task.h>
#include <Uintah/Grid/Material.h>
#include <Uintah/Grid/Region.h>
#include <Uintah/Grid/TypeDescription.h>
#include <Uintah/Interface/DataWarehouse.h>
#include <SCICore/Exceptions/InternalError.h>
#include <iostream>

using namespace Uintah;
using SCICore::Exceptions::InternalError;

Task::ActionBase::~ActionBase()
{
}

Task::~Task()
{
  vector<Dependency*>::iterator iter;

  for( iter=d_reqs.begin(); iter != d_reqs.end(); iter++ )
    { delete *iter; }
  for( iter=d_comps.begin(); iter != d_comps.end(); iter++)
    { delete *iter; }
  delete d_action;
}

void
Task::usesMPI(bool state)
{
  d_usesMPI = state;
}

void
Task::usesThreads(bool state)
{
  d_usesThreads = state;
}

void
Task::subregionCapable(bool state)
{
  d_subregionCapable = state;
}

void
Task::requires(const DataWarehouseP& ds, const VarLabel* var)
{
  d_reqs.push_back(new Dependency(this, ds, var, -1, 0));
}

void
Task::requires(const DataWarehouseP& ds, const VarLabel* var, int matlIndex,
	       const Region* region, GhostType gtype, int numGhostCells)
{
   const TypeDescription* td = var->typeDescription();
   int l,h;
   switch(gtype){
   case Task::None:
      if(numGhostCells != 0)
	 throw InternalError("Ghost cells specified with task type none!\n");
      l=h=0;
      break;
   case Task::AroundNodes:
      if(numGhostCells == 0)
	 throw InternalError("No ghost cells specified with Task::AroundNodes");
      switch(td->getBasis()){
      case TypeDescription::Node:
	 // All 27 neighbors
	 l=-1;
	 h=1;
	 break;
      case TypeDescription::Cell:
	 // Lower neighbors
	 l=-1;
	 h=0;
         break;
      default:
	 throw InternalError("Illegal Basis type");
      }
      break;
   case Task::AroundCells:
      if(numGhostCells == 0)
	 throw InternalError("No ghost cells specified with Task::AroundCells");
      switch(td->getBasis()){
      case TypeDescription::Node:
	 // Upper neighbors
	 l=0;
	 h=1;
         break;
      case TypeDescription::Cell:
	 // All 27 neighbors
	 l=-1;
	 h=1;
	 break;
      default:
	 throw InternalError("Illegal Basis type");
      }
      break;
   default:
      throw InternalError("Illegal ghost type");
   }
   for(int ix=l;ix<=h;ix++){
      for(int iy=l;iy<=h;iy++){
	 for(int iz=l;iz<=h;iz++){
	    const Region* neighbor = region->getNeighbor(IntVector(ix,iy,iz));
	    if(neighbor)
	       d_reqs.push_back(new Dependency(this, ds, var, matlIndex,
					       neighbor));
	 }
      }
   }
}

void
Task::computes(const DataWarehouseP& ds, const VarLabel* var)
{
  d_comps.push_back(new Dependency(this, ds, var, -1, 0));
}

void
Task::computes(const DataWarehouseP& ds, const VarLabel* var, int matlIndex,
	       const Region*)
{
  d_comps.push_back(new Dependency(this, ds, var, matlIndex, d_region));
}

void
Task::doit(const ProcessorContext* pc)
{
  if( d_completed )
      throw InternalError("Task performed, but already completed");
  d_action->doit(pc, d_region, d_fromDW, d_toDW);
  d_completed=true;
}

Task::Dependency::Dependency(Task* task, const DataWarehouseP& dw,
			     const VarLabel* var, int matlIndex,
			     const Region* region)
    : d_task(task),
      d_dw(dw),
      d_var(var),
      d_matlIndex(matlIndex),
      d_region(region)
{
}

const vector<Task::Dependency*>&
Task::getComputes() const
{
  return d_comps;
}

const vector<Task::Dependency*>&
Task::getRequires() const
{
  return d_reqs;
}

//
// $Log$
// Revision 1.9  2000/05/07 06:02:13  sparker
// Added beginnings of multiple patch support and real dependencies
//  for the scheduler
//
// Revision 1.8  2000/05/05 06:42:45  dav
// Added some _hopefully_ good code mods as I work to get the MPI stuff to work.
//
// Revision 1.7  2000/04/26 06:49:00  sparker
// Streamlined namespaces
//
// Revision 1.6  2000/04/20 18:56:31  sparker
// Updates to MPM
//
// Revision 1.5  2000/04/11 07:10:50  sparker
// Completing initialization and problem setup
// Finishing Exception modifications
//
// Revision 1.4  2000/03/17 09:29:59  sparker
// New makefile scheme: sub.mk instead of Makefile.in
// Use XML-based files for module repository
// Plus many other changes to make these two things work
//
// Revision 1.3  2000/03/16 22:08:01  dav
// Added the beginnings of cocoon docs.  Added namespaces.  Did a few other coding standards updates too
//
//
