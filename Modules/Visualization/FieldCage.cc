
/*
 *  FieldCage.cc:  IsoSurfaces a SFRG bitwise
 *
 *  Written by:
 *   David Weinstein
 *   Department of Computer Science
 *   University of Utah
 *   February 1995
 *
 *  Copyright (C) 1994 SCI Group
 */

#include <Classlib/NotFinished.h>
#include <Dataflow/Module.h>
#include <Datatypes/GeometryPort.h>
#include <Datatypes/ScalarFieldPort.h>
#include <Datatypes/VectorFieldPort.h>
#include <Geom/Geom.h>
#include <Geom/Line.h>
#include <Geom/Group.h>
#include <Geom/Material.h>
#include <Malloc/Allocator.h>
#include <TCL/TCLvar.h>
#include <iostream.h>

class FieldCage : public Module {
    ScalarFieldIPort* insfield;
    VectorFieldIPort* invfield;
    GeometryOPort* ogeom;
    MaterialHandle dk_red;
    MaterialHandle dk_green;
    MaterialHandle dk_blue;
    MaterialHandle lt_red;
    MaterialHandle lt_green;
    MaterialHandle lt_blue;
    MaterialHandle gray;
    TCLint numx;
    TCLint numy;
    TCLint numz;
public:
    FieldCage(const clString& id);
    FieldCage(const FieldCage&, int deep);
    virtual ~FieldCage();
    virtual Module* clone(int deep);
    virtual void execute();
    MaterialHandle matl;
};

extern "C" {
Module* make_FieldCage(const clString& id)
{
    return new FieldCage(id);
}
};

FieldCage::FieldCage(const clString& id)
: Module("FieldCage", id, Filter), numx("numx", id, this), numy("numy", id, this), numz("numz", id, this)
{
    // Create the input ports
    insfield=new ScalarFieldIPort(this, "Scalar Field", ScalarFieldIPort::Atomic);
    add_iport(insfield);
    invfield=new VectorFieldIPort(this, "Vector Field", VectorFieldIPort::Atomic);
    add_iport(invfield);
    // Create the output port
    ogeom=new GeometryOPort(this, "Geometry", GeometryIPort::Atomic);
    add_oport(ogeom);
    matl=scinew Material(Color(0,0,0), Color(.8,.8,.8),
			 Color(.7,.7,.7), 50);
   dk_red = scinew Material(Color(0,0,0), Color(.3,0,0),
			 Color(.5,.5,.5), 20);
   dk_green = scinew Material(Color(0,0,0), Color(0,.3,0),
			   Color(.5,.5,.5), 20);
   dk_blue = scinew Material(Color(0,0,0), Color(0,0,.3),
			  Color(.5,.5,.5), 20);
   lt_red = scinew Material(Color(0,0,0), Color(.8,0,0),
			 Color(.5,.5,.5), 20);
   lt_green = scinew Material(Color(0,0,0), Color(0,.8,0),
			   Color(.5,.5,.5), 20);
   lt_blue = scinew Material(Color(0,0,0), Color(0,0,.8),
			  Color(.5,.5,.5), 20);
    gray = scinew Material(Color(0,0,0), Color(.4,.4,.4),
			  Color(.5,.5,.5), 20);
}

FieldCage::FieldCage(const FieldCage& copy, int deep)
: Module(copy, deep), numx("numx", id, this), numy("numy", id, this), numz("numz", id, this)
{
    NOT_FINISHED("FieldCage::FieldCage");
}

FieldCage::~FieldCage()
{
}

Module* FieldCage::clone(int deep)
{
    return new FieldCage(*this, deep);
}

void FieldCage::execute()
{
    ogeom->delAll();

    ScalarFieldHandle sfield;
    Point min, max;
    bool haveit=false;
    if(insfield->get(sfield)){
	sfield->get_bounds(min, max);
	haveit=true;
    }
    VectorFieldHandle vfield;
    if(invfield->get(vfield)){
	vfield->get_bounds(min, max);
	haveit=true;
    }
    if(!haveit)
	return;
    GeomGroup* all=new GeomGroup();
    GeomLines* xmin=new GeomLines();
    GeomLines* ymin=new GeomLines();
    GeomLines* zmin=new GeomLines();
    GeomLines* xmax=new GeomLines();
    GeomLines* ymax=new GeomLines();
    GeomLines* zmax=new GeomLines();
    GeomLines* edges=new GeomLines();
    all->add(new GeomMaterial(xmin, dk_red));
    all->add(new GeomMaterial(ymin, dk_green));
    all->add(new GeomMaterial(zmin, dk_blue));
    all->add(new GeomMaterial(xmax, lt_red));
    all->add(new GeomMaterial(ymax, lt_green));
    all->add(new GeomMaterial(zmax, lt_blue));
    all->add(new GeomMaterial(edges, gray));

    edges->add(Point(min.x(), min.y(), min.z()), Point(min.x(), min.y(), max.z()));
    edges->add(Point(min.x(), min.y(), min.z()), Point(min.x(), max.y(), min.z()));
    edges->add(Point(min.x(), min.y(), min.z()), Point(max.x(), min.y(), min.z()));
    edges->add(Point(max.x(), min.y(), min.z()), Point(max.x(), max.y(), min.z()));
    edges->add(Point(max.x(), min.y(), min.z()), Point(max.x(), min.y(), max.z()));
    edges->add(Point(min.x(), max.y(), min.z()), Point(max.x(), max.y(), min.z()));
    edges->add(Point(min.x(), max.y(), min.z()), Point(min.x(), max.y(), max.z()));
    edges->add(Point(min.x(), min.y(), min.z()), Point(min.x(), min.y(), max.z()));
    edges->add(Point(min.x(), min.y(), max.z()), Point(max.x(), min.y(), max.z()));
    edges->add(Point(min.x(), min.y(), max.z()), Point(min.x(), max.y(), max.z()));
    edges->add(Point(max.x(), max.y(), min.z()), Point(max.x(), max.y(), max.z()));
    edges->add(Point(max.x(), min.y(), max.z()), Point(max.x(), max.y(), max.z()));
    edges->add(Point(min.x(), max.y(), max.z()), Point(max.x(), max.y(), max.z()));
    int i;
    int nx=numx.get();
    int ny=numy.get();
    int nz=numz.get();
    for(i=0;i<nx;i++){
	double x=Interpolate(min.x(), max.x(), double(i+1)/double(nx+1));
	ymin->add(Point(x, min.y(), min.z()), Point(x, min.y(), max.z()));
	ymax->add(Point(x, max.y(), min.z()), Point(x, max.y(), max.z()));
	zmin->add(Point(x, min.y(), min.z()), Point(x, max.y(), min.z()));
	zmax->add(Point(x, min.y(), max.z()), Point(x, max.y(), max.z()));
    }
    for(i=0;i<ny;i++){
	double y=Interpolate(min.y(), max.y(), double(i+1)/double(ny+1));
	xmin->add(Point(min.x(), y, min.z()), Point(min.x(), y, max.z()));
	xmax->add(Point(max.x(), y, min.z()), Point(max.x(), y, max.z()));
	zmin->add(Point(min.x(), y, min.z()), Point(max.x(), y, min.z()));
	zmax->add(Point(min.x(), y, max.z()), Point(max.x(), y, max.z()));
    }
    for(i=0;i<nz;i++){
	double z=Interpolate(min.z(), max.z(), double(i+1)/double(nz+1));
	xmin->add(Point(min.x(), min.y(), z), Point(min.x(), max.y(), z));
	xmax->add(Point(max.x(), min.y(), z), Point(max.x(), max.y(), z));
	ymin->add(Point(min.x(), min.y(), z), Point(max.x(), min.y(), z));
	ymax->add(Point(min.x(), max.y(), z), Point(max.x(), max.y(), z));
    }
    ogeom->delAll();
    ogeom->addObj(all, "Field Cage");
}
