/*
 *  Magnitude.cc:  Unfinished modules
 *
 *  Written by:
 *   Steven G. Parker
 *   Department of Computer Science
 *   University of Utah
 *   March 1994
 *
 *  Copyright (C) 1994 SCI Group
 */

#include <Classlib/NotFinished.h>
#include <Dataflow/Module.h>
#include <Datatypes/ScalarFieldPort.h>
#include <Datatypes/SurfacePort.h>
#include <Datatypes/ScalarFieldRG.h>
#include <Datatypes/VectorFieldPort.h>
#include <Datatypes/VectorFieldOcean.h>
#include <Geometry/Point.h>
#include <TCL/TCLvar.h>
#include <Math/Expon.h>
#include <Multitask/Task.h>
#include <values.h>

class Magnitude : public Module {
    VectorFieldIPort* infield;
    ScalarFieldOPort* outfield;
public:
    void parallel(int proc);
    VectorFieldOcean* vfield;
    ScalarFieldRG* sfield;
    int np;
    Mutex minmax;
    Magnitude(const clString& id);
    Magnitude(const Magnitude&, int deep);
    virtual ~Magnitude();
    virtual Module* clone(int deep);
    virtual void execute();
};

extern "C" {
Module* make_Magnitude(const clString& id)
{
    return new Magnitude(id);
}
};

Magnitude::Magnitude(const clString& id)
: Module("Magnitude", id, Filter)
{
  infield=new VectorFieldIPort(this, "Vector", VectorFieldIPort::Atomic);
  add_iport(infield);

  // Create the output port
  outfield=new ScalarFieldOPort(this, "Magnitude", ScalarFieldIPort::Atomic);
  add_oport(outfield);
}

Magnitude::Magnitude(const Magnitude& copy, int deep)
: Module(copy, deep)
{
}

Magnitude::~Magnitude()
{
}

Module* Magnitude::clone(int deep)
{
    return new Magnitude(*this, deep);
}

static void do_parallel(void* obj, int proc)
{
  Magnitude* module=(Magnitude*)obj;
  module->parallel(proc);
}

  

void Magnitude::parallel(int proc)
{
  int nx=vfield->nx;
  int ny=vfield->ny;
  int nz=vfield->nz;
  int sz=proc*nz/np;
  int ez=(proc+1)*nz/np;
  double min=MAXDOUBLE;
  double max=-MAXDOUBLE;
  float* p1=vfield->data+nx*ny*sz;
  float* p2=p1+nx*ny*nz;
  for(int k=sz;k<ez;k++){
    if(proc == 0)
      update_progress(k, ez);
    for(int j=0;j<ny;j++){
      for(int i=0;i<nx;i++){
	float u=*p1++;
	float v=*p2++;
	double mag=Sqrt(u*u+v*v);
	sfield->grid(i,j,k)=mag;
	min=Min(min, mag);
	max=Max(max, mag);
      }
    }
  }
  minmax.lock();
  double mn, mx;
  sfield->get_minmax(mn, mx);
  min=Min(mn, min);
  max=Max(mx, max);
  sfield->set_minmax(min, max);
  cerr << proc << ": min=" << min << ", max=" << max << endl;
  minmax.unlock();
}

void Magnitude::execute()
{
  VectorFieldHandle iff;
  if(!infield->get(iff))
    return;
  vfield=iff->getOcean();
  if(!vfield){
    error("Magnitude can't deal with this field");
    return;
  }
  sfield=new ScalarFieldRG();
  sfield->resize(vfield->nx, vfield->ny, vfield->nz);
  Point min, max;
  vfield->get_bounds(min, max);
  sfield->set_bounds(min, max);
  cerr << "setting zgrid, depthval=" << vfield->depthval.size() << endl;
  sfield->zgrid=vfield->depthval;
  cerr << "zgrid=" << sfield->zgrid.size() << endl;
  cerr << "At magnitude: this=" << sfield << ", sizes: " << sfield->xgrid.size() << ", " << sfield->ygrid.size() << ", " << sfield->zgrid.size() << endl;
  sfield->set_minmax(MAXDOUBLE, -MAXDOUBLE);
  np=Task::nprocessors();
  Task::multiprocess(np, do_parallel, this);
  outfield->send(sfield);
}
