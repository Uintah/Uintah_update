/*
  The contents of this file are subject to the University of Utah Public
  License (the "License"); you may not use this file except in compliance
  with the License.

  Software distributed under the License is distributed on an "AS IS"
  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
  License for the specific language governing rights and limitations under
  the License.

  The Original Source Code is SCIRun, released March 12, 2001.

  The Original Source Code was developed by the University of Utah.
  Portions created by UNIVERSITY are Copyright (C) 2001, 1994
  University of Utah. All Rights Reserved.
*/

/*
 *  ScanlineMesh.cc: Templated Mesh defined on a 3D Regular Grid
 *
 *  Written by:
 *   Michael Callahan &&
 *   Chris Moulding
 *   Department of Computer Science
 *   University of Utah
 *   January 2001
 *
 *  Copyright (C) 2001 SCI Group
 *
 */

#include <Core/Datatypes/ScanlineMesh.h>
#include <Core/Datatypes/FieldAlgo.h>
#include <iostream>


namespace SCIRun {

using namespace std;


PersistentTypeID ScanlineMesh::type_id("ScanlineMesh", "MeshBase", maker);


BBox
ScanlineMesh::get_bounding_box() const
{
  BBox result;
  result.extend(min_);
  result.extend(max_);
  return result;
}

void
ScanlineMesh::get_nodes(Node::array_type &array, Edge::index_type idx) const
{
  array.resize(2);
  array[0] = Node::index_type(idx);
  array[1] = Node::index_type(idx + 1);
}

//! return all cell_indecies that overlap the BBox in arr.
void
ScanlineMesh::get_edges(Edge::array_type &/* arr */, const BBox &/*bbox*/) const
{
  // TODO: implement this
}


void
ScanlineMesh::get_center(Point &result, Node::index_type idx) const
{
  const double sx = (max_.x() - min_.x()) / (length_ - 1);

  result.x(idx * sx + min_.x());
  result.y(0);
  result.z(0);
}


void
ScanlineMesh::get_center(Point &result, Edge::index_type idx) const
{
  const double sx = (max_.x() - min_.x()) / (length_ - 1);

  result.x((idx + 0.5) * sx + min_.x());
  result.y(0);
  result.z(0);
}

// TODO: verify
bool
ScanlineMesh::locate(Edge::index_type &cell, const Point &p) const
{
  double i = (p.x() - min_.x()) / (max_.x() - min_.x()) * (length_ - 1) + 0.5;

  cell = (unsigned int)i;

  if (cell >= (length_ - 1))
  {
    return false;
  }
  else
  {
    return true;
  }
}


// TODO: verify
bool
ScanlineMesh::locate(Node::index_type &node, const Point &p) const
{
  Node::array_type nodes;     // storage for node_indeces
  Cell::index_type cell;
  double max;
  int loop;

  // locate the cell enclosing the point (including weights)
  if (!locate(cell,p)) return false;
  weight_array w;
  calc_weights(this, cell, p, w);

  // get the node_indeces in this cell
  get_nodes(nodes,cell);

  // find, and return, the "heaviest" node
  max = w[0];
  loop=1;
  while (loop<8) {
    if (w[loop]>max) {
      max=w[loop];
      node=nodes[loop];
    }
  }
  return true;
}


#define LATVOLMESH_VERSION 1

void
ScanlineMesh::io(Piostream& stream)
{
  stream.begin_class(type_name(-1), LATVOLMESH_VERSION);

  MeshBase::io(stream);

  // IO data members, in order
  Pio(stream, length_);
  Pio(stream, min_);
  Pio(stream, max_);

  stream.end_class();
}

const string
ScanlineMesh::type_name(int n)
{
  ASSERT(n >= -1 && n <= 0);
  static const string name = "ScanlineMesh";
  return name;
}


template<>
ScanlineMesh::Node::iterator
ScanlineMesh::tbegin(ScanlineMesh::Node::iterator *) const
{
  return Node::iterator(offset_);
}

template<>
ScanlineMesh::Node::iterator
ScanlineMesh::tend(ScanlineMesh::Node::iterator *) const
{
  return Node::iterator(offset_ + length_);
}

template<>
ScanlineMesh::Node::size_type
ScanlineMesh::tsize(ScanlineMesh::Node::size_type *) const
{
  return Node::size_type(length_);
}

template<>
ScanlineMesh::Edge::iterator
ScanlineMesh::tbegin(ScanlineMesh::Edge::iterator *) const
{
  return Edge::iterator(offset_);
}

template<>
ScanlineMesh::Edge::iterator
ScanlineMesh::tend(ScanlineMesh::Edge::iterator *) const
{
  return Edge::iterator(offset_+length_-1);
}

template<>
ScanlineMesh::Edge::size_type
ScanlineMesh::tsize(ScanlineMesh::Edge::size_type *) const
{
  return Edge::size_type(length_ - 1);
}

template<>
ScanlineMesh::Face::iterator
ScanlineMesh::tbegin(ScanlineMesh::Face::iterator *) const
{
  return Face::iterator(0);
}

template<>
ScanlineMesh::Face::iterator
ScanlineMesh::tend(ScanlineMesh::Face::iterator *) const
{
  return Face::iterator(0);
}

template<>
ScanlineMesh::Face::size_type
ScanlineMesh::tsize(ScanlineMesh::Face::size_type *) const
{
  return Face::size_type(0);
}

template<>
ScanlineMesh::Cell::iterator
ScanlineMesh::tbegin(ScanlineMesh::Cell::iterator *) const
{
  return Cell::iterator(0);
}

template<>
ScanlineMesh::Cell::iterator
ScanlineMesh::tend(ScanlineMesh::Cell::iterator *) const
{
  return Cell::iterator(0);
}

template<>
ScanlineMesh::Cell::size_type
ScanlineMesh::tsize(ScanlineMesh::Cell::size_type *) const
{
  return Cell::size_type(0);
}


ScanlineMesh::Node::iterator ScanlineMesh::node_begin() const
{ return tbegin((Node::iterator *)0); }
ScanlineMesh::Edge::iterator ScanlineMesh::edge_begin() const
{ return tbegin((Edge::iterator *)0); }
ScanlineMesh::Face::iterator ScanlineMesh::face_begin() const
{ return tbegin((Face::iterator *)0); }
ScanlineMesh::Cell::iterator ScanlineMesh::cell_begin() const
{ return tbegin((Cell::iterator *)0); }

ScanlineMesh::Node::iterator ScanlineMesh::node_end() const
{ return tend((Node::iterator *)0); }
ScanlineMesh::Edge::iterator ScanlineMesh::edge_end() const
{ return tend((Edge::iterator *)0); }
ScanlineMesh::Face::iterator ScanlineMesh::face_end() const
{ return tend((Face::iterator *)0); }
ScanlineMesh::Cell::iterator ScanlineMesh::cell_end() const
{ return tend((Cell::iterator *)0); }

ScanlineMesh::Node::size_type ScanlineMesh::nodes_size() const
{ return tsize((Node::size_type *)0); }
ScanlineMesh::Edge::size_type ScanlineMesh::edges_size() const
{ return tsize((Edge::size_type *)0); }
ScanlineMesh::Face::size_type ScanlineMesh::faces_size() const
{ return tsize((Face::size_type *)0); }
ScanlineMesh::Cell::size_type ScanlineMesh::cells_size() const
{ return tsize((Cell::size_type *)0); }

} // namespace SCIRun
