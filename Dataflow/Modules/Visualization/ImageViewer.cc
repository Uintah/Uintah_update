
/*
 *  ImageViewer.cc:  
 *
 *  Written by:
 *   ??
 *   Department of Computer Science
 *   University of Utah
 *   May 1995
 *
 *  Copyright (C) 1995 SCI Group
 */

#include <Core/Containers/Array1.h>
#include <Dataflow/Network/Module.h>
#include <Dataflow/Ports/GeometryPort.h>
#include <Dataflow/Ports/ScalarFieldPort.h>
#include <Core/Datatypes/ScalarFieldRGshort.h>
#include <Core/Geom/tGrid.h>
#include <Core/Geometry/Point.h>
#include <Core/Math/MinMax.h>
#include <Core/Malloc/Allocator.h>
#include <Core/TclInterface/TCLvar.h>
#include <iostream>
using std::cerr;

namespace SCIRun {


class ImageViewer : public Module {
  ScalarFieldIPort *inscalarfield;
  GeometryOPort* ogeom;

  int grid_id;

  ScalarFieldRGshort *ingrid; // this only works on regular grids for chaining

  int u_num, v_num;
  Point corner;
  Vector u, v;
  ScalarField* sfield;
  TexGeomGrid* grid;
public:
  ImageViewer(const clString& id);
  virtual ~ImageViewer();
  virtual void execute();
};

extern "C" Module* make_ImageViewer(const clString& id) {
  return new ImageViewer(id);
}

//static clString module_name("ImageViewer");

ImageViewer::ImageViewer(const clString& id)
: Module("ImageViewer", id, Filter)
{
  // Create the input ports
  // Need a scalar field and a colormap
  inscalarfield = scinew ScalarFieldIPort( this, "Scalar Field",
					  ScalarFieldIPort::Atomic);
  add_iport( inscalarfield);
  
  // Create the output port
  ogeom = scinew GeometryOPort(this, "Geometry", 
			       GeometryIPort::Atomic);
  add_oport(ogeom);
}

ImageViewer::~ImageViewer()
{
}

void ImageViewer::execute()
{
  int old_grid_id = grid_id;

  // get the scalar field and colormap...if you can
  ScalarFieldHandle sfieldh;
  if (!inscalarfield->get( sfieldh ))
    return;
  sfield=sfieldh.get_rep();

  if (!sfield->getRGBase())
    return;

  ingrid = sfield->getRGBase()->getRGShort();

  if (!ingrid)
    return;

  if (ingrid->grid.dim3() != 1) {
    error( "This module for 2d images only..");
    return;
  }
  
  u_num = ingrid->grid.dim1();
  v_num = ingrid->grid.dim2();
  
  corner = Point(0,0,0);
  u = Vector(u_num,0,0);
  v = Vector(0,v_num,0);

  cerr << u_num << " " << v_num << "\n";
  
  grid = scinew TexGeomGrid(v_num, u_num, corner, v, u,1);

  grid->set((unsigned short *) &ingrid->grid(0,0,0),4); // value doesn't matter...
  
  // delete the old grid/cutting plane
  if (old_grid_id != 0)
    ogeom->delObj( old_grid_id );
  
  grid_id = ogeom->addObj(grid, "Image Viewer");
}

} // End namespace SCIRun

