
/*
 *  LocatePoints.cc:  Rescale a surface
 *
 *  Written by:
 *   David Weinstein
 *   Department of Computer Science
 *   University of Utah
 *   February 1995
 *
 *  Copyright (C) 1994 SCI Group
 */

#include <Dataflow/Ports/MatrixPort.h>
#include <Dataflow/Ports/MeshPort.h>
#include <Dataflow/Ports/SurfacePort.h>
#include <Core/Datatypes/SparseRowMatrix.h>
#include <Core/Datatypes/TriSurface.h>
#include <Core/Malloc/Allocator.h>
#include <Core/GuiInterface/GuiVar.h>
#include <iostream>
using std::cerr;
#include <stdio.h>

namespace SCIRun {


class LocatePoints : public Module {
    MeshIPort* imesh;
    SurfaceIPort* isurf;
    MatrixOPort* omat;
    MatrixHandle matH;
    int meshGen;
    int surfGen;
public:
    LocatePoints(const clString& id);
    virtual ~LocatePoints();
    virtual void execute();
};

extern "C" Module* make_LocatePoints(const clString& id)
{
    return new LocatePoints(id);
}

LocatePoints::LocatePoints(const clString& id)
: Module("LocatePoints", id, Filter)
{
    imesh=scinew MeshIPort(this, "Mesh", MeshIPort::Atomic);
    add_iport(imesh);
    isurf=scinew SurfaceIPort(this, "Surface", SurfaceIPort::Atomic);
    add_iport(isurf);
    // Create the output port
    omat=scinew MatrixOPort(this, "Matrix", MatrixIPort::Atomic);
    add_oport(omat);
    meshGen=-1;
    surfGen=-1;
}

LocatePoints::~LocatePoints()
{
}

void sortpts(double *dist, int *idx) {
  double tmpd;
  int tmpi;
  int i,j;
  for (i=0; i<4; i++) {
    for (j=i+1; j<4; j++) {
      if (idx[j]<idx[i]) {
	tmpd=dist[i]; dist[i]=dist[j]; dist[j]=tmpd;
	tmpi=idx[i]; idx[i]=idx[j]; idx[j]=tmpi;
      }
    }
  }
}

void LocatePoints::execute()
{
    MeshHandle meshH;
    if(!imesh->get(meshH))
	return;

    SurfaceHandle surfH;
    if (!isurf->get(surfH))
	return;

    if (meshGen == meshH->generation && surfGen == surfH->generation && matH.get_rep()) {
	omat->send(matH);
	return;
    }

    meshGen = meshH->generation;
    surfGen = surfH->generation;

    TriSurface *ts=dynamic_cast<TriSurface*>(surfH.get_rep());
    if (!ts) {
      cerr << "Error - need a TriSurface.\n";
      return;
    }

    int *rows=new int[ts->points_.size()+1];
    int *cols=new int[ts->points_.size()*4];
    double *a=new double[ts->points_.size()*4];
    SparseRowMatrix *mm=scinew SparseRowMatrix(ts->points_.size(),
					       meshH->nodesize(),
					       rows, cols,
					       ts->points_.size()*4, a);
    matH=mm;
    int i,j;
    int ix;
    int nodes[4];
    double dist[4];
    double sum;
    for (i=0; i<ts->points_.size(); i++) {
      rows[i]=i*4;
      if (!meshH->locate(&ix, ts->points_[i], 1.e-4, 1.e-4)) {
	int foundIt=0;
	for (j=0; j<meshH->nodesize(); j++) {
	  if ((meshH->point(j) - ts->points_[i]).length2() < 0.001) {
	    ix=meshH->node(j).elems[0];
	    foundIt=1;
	    break;
	  }
	}
	if (!foundIt) {
	  cerr << "Error - couldn't find point "<<ts->points_[i]<<" in mesh.\n";
	  delete rows;
	  delete cols;
	  delete a;
	  return;
	}
      }
      Element *e=meshH->element(ix);
      for (sum=0,j=0; j<4; j++) {
	nodes[j]=e->n[j];
	dist[j]=(ts->points_[i] - meshH->point(nodes[j])).length();
	sum+=dist[j];
      }
      sortpts(dist, nodes);
      for (j=0; j<4; j++) {
	cols[i*4+j]=nodes[j];
	a[i*4+j]=dist[j]/sum;
      }
      if (i==ts->points_.size()-1 || i==0 || (!(i%(ts->points_.size()/10)))) {
	cerr << "i="<<i<<"\n";
	for (j=0; j<4; j++) {
	  cerr << "a["<<i*4+j<<"]="<<a[i*4+j]<<" ";
	}
	cerr << "\n";
      }
    }
    rows[i]=i*4;
    omat->send(matH);
}

} // End namespace SCIRun


