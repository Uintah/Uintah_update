
/*
 *  AddWells2.cc:  
 *
 *  Written by:
 *   David Weinstein
 *   Department of Computer Science
 *   University of Utah
 *   Mar. 1995
 *
 *  Copyright (C) 1995 SCI Group
 */

#include <Dataflow/Network/Module.h>
#include <Dataflow/Ports/GeometryPort.h>
#include <Core/Geom/GeomCylinder.h>
#include <Core/Geom/GeomGroup.h>
#include <Core/Geom/GeomLine.h>
#include <Core/Geom/Material.h>
#include <Core/Malloc/Allocator.h>
#include <Core/TclInterface/TCLvar.h>
#include <iostream>
using std::cerr;
#include <fstream>
using std::ifstream;

namespace SCIRun {


class AddWells2 : public Module {
    TCLstring filename;
    TCLdouble radius;
    GeometryOPort* ogeom;
    int ndeep;
    double* dp;
    double* ms;
    double toms(double depth);
public:
    AddWells2(const clString& id);
    virtual ~AddWells2();
    virtual void execute();
};

extern "C" Module* make_AddWells2(const clString& id) {
 return new AddWells2(id);
}

//static clString module_name("AddWells2");

AddWells2::AddWells2(const clString& id)
: Module("AddWells2", id, Source),
  filename("filename", id, this), radius("radius", id, this)
{
   // Create the output port
   ogeom = scinew GeometryOPort(this, "Geometry", GeometryIPort::Atomic);
   add_oport(ogeom);
}

AddWells2::~AddWells2()
{
}

void AddWells2::execute()
{
    clString fn(filename.get());
    ifstream in(fn());
    if(!in){
	cerr << "Didn't open file: " << fn << '\n';
	return;
    }
    double rad=radius.get();
    GeomGroup* all=new GeomGroup;
    GeomGroup* h[10];
    for(int i=0;i<9;i++){
	h[i]=new GeomGroup();
	HSVColor color(i*360/9., 0.7, 1.0);
	MaterialHandle matl=new Material(color*.2, color*.8, Color(.6,.6,.6),20);
	all->add(new GeomMaterial(h[i], matl));
    }
    char line[1000];
    in.getline(line, 1000);
    while(in){
	double x, y;
	double dummy;
	in >> dummy >> x >> y >> dummy >> dummy >> dummy;
	double z1;
	in >> dummy >> dummy >> z1;
	if(!in)break;
	for(int i=0;i<9;i++){
	    double z2;
	    in >> dummy >> dummy >> z2;
	    if(!in)break;
	    if(z2 > 50000)
		continue;
	    GeomCylinder* cyl=new GeomCylinder(Point(x,y,z1),
					       Point(x,y,z2),
					       rad, 12, 2);
	    h[i]->add(cyl);
	    z1=z2;
	}
	if(!in)break;
    }
    ogeom->delAll();
    ogeom->addObj(all, "Wells");
    ogeom->flushViews();
}

} // End namespace SCIRun

