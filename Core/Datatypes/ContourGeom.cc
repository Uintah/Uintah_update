//  ContourGeom.cc - A group of Tets in 3 space
//  Written by:
//   Eric Kuehne
//   Department of Computer Science
//   University of Utah
//   April 2000
//  Copyright (C) 2000 SCI Institute

#include <Core/Datatypes/ContourGeom.h>
#include <Core/Persistent/PersistentSTL.h>

namespace SCIRun {

//////////
// PIO support
static Persistent* maker(){
  return new ContourGeom();
}

string ContourGeom::typeName(){
  static string typeName = "ContourGeom";
  return typeName;
}

PersistentTypeID ContourGeom::type_id(ContourGeom::typeName(),
				      UnstructuredGeom::typeName(),
				      maker);

#define CONTOURGEOM_VERSION 1
void ContourGeom::io(Piostream& stream)
{
  stream.begin_class(typeName().c_str(), CONTOURGEOM_VERSION);
  Pio(stream, d_edge);
  stream.end_class();
}

DebugStream ContourGeom::dbg("ContourGeom", true);

//////////
// Constructors/Destructor
ContourGeom::ContourGeom()
{
}

ContourGeom::ContourGeom(const vector<NodeSimp>& inodes,
			 const vector<EdgeSimp>& iedges)
  : PointCloudGeom(inodes)
{
  d_edge = iedges;
}

ContourGeom::~ContourGeom()
{
}

string
ContourGeom::getInfo()
{
  ostringstream retval;
  retval << "name = " << d_name << endl;
  return retval.str();
}

void
ContourGeom::setEdges(const vector<EdgeSimp>& iedges)
{
  d_edge.clear();
  d_edge = iedges;
}

} // End namespace SCIRun
