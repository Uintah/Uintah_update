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
 *  ImageMesh.h: Templated Mesh defined on a 2D Regular Grid
 *
 *  Written by:
 *   Michael Callahan
 *   Department of Computer Science
 *   University of Utah
 *   January 2001
 *
 *  Copyright (C) 2001 SCI Group
 *
 */

#ifndef SCI_project_ImageMesh_h
#define SCI_project_ImageMesh_h 1

#include <Core/Geometry/Point.h>
#include <Core/Containers/LockingHandle.h>
#include <Core/Datatypes/MeshBase.h>
#include <Core/share/share.h>
#include <string>
#include <iostream>

namespace SCIRun {

using std::string;

class SCICORESHARE ImageMesh : public MeshBase
{
public:


  struct UnfinishedIndex
  {
  public:
    UnfinishedIndex() : i_(0) {}
    UnfinishedIndex(unsigned i) : i_(i) {}

    operator unsigned() const { return i_; }

    unsigned i_;
  };

  struct EdgeIndex : public UnfinishedIndex
  {
    EdgeIndex() : UnfinishedIndex() {}
    EdgeIndex(unsigned i) : UnfinishedIndex(i) {}
  };

  struct CellIndex : public UnfinishedIndex
  {
    CellIndex() : UnfinishedIndex() {}
    CellIndex(unsigned i) : UnfinishedIndex(i) {}
  };

  struct UnfinishedIter : public UnfinishedIndex
  {
    UnfinishedIter(const ImageMesh *m, unsigned i)
      : UnfinishedIndex(i), mesh_(m) {}

    const UnfinishedIndex &operator *() { return *this; }

    bool operator ==(const UnfinishedIter &a) const
    {
      return i_ == a.i_ && mesh_ == a.mesh_;
    }

    bool operator !=(const UnfinishedIter &a) const
    {
      return !(*this == a);
    }

    const ImageMesh *mesh_;
  };

  struct EdgeIter : public UnfinishedIter
  {
    EdgeIter(const ImageMesh *m, unsigned i)
      : UnfinishedIter(m, i) {}

    const EdgeIndex &operator *() const { return (const EdgeIndex&)(*this); }

    EdgeIter &operator++() { return *this; }

  private:

    EdgeIter operator++(int)
    {
      EdgeIter result(*this);
      operator++();
      return result;
    }
  };

  struct CellIter : public UnfinishedIter
  {
    CellIter(const ImageMesh *m, unsigned i)
      : UnfinishedIter(m, i) {}

    const CellIndex &operator *() const { return (const CellIndex&)(*this); }

    CellIter &operator++() { return *this; }

  private:

    CellIter operator++(int)
    {
      CellIter result(*this);
      operator++();
      return result;
    }
  };


  struct ImageIndex
  {
  public:
    ImageIndex() : i_(0), j_(0) {}
    ImageIndex(unsigned i, unsigned j) : i_(i), j_(j) {}

    operator unsigned() const { return i_*j_; }

    unsigned i_, j_;
  };

  struct FaceIndex : public ImageIndex
  {
    FaceIndex() : ImageIndex() {}
    FaceIndex(unsigned i, unsigned j) : ImageIndex(i, j) {}
  };

  struct NodeIndex : public ImageIndex
  {
    NodeIndex() : ImageIndex() {}
    NodeIndex(unsigned i, unsigned j) : ImageIndex(i, j) {}
  };

  struct ImageIter : public ImageIndex
  {
    ImageIter(const ImageMesh *m, unsigned i, unsigned j)
      : ImageIndex(i, j), mesh_(m) {}

    const ImageIndex &operator *() { return *this; }

    bool operator ==(const ImageIter &a) const
    {
      return i_ == a.i_ && j_ == a.j_ && mesh_ == a.mesh_;
    }

    bool operator !=(const ImageIter &a) const
    {
      return !(*this == a);
    }

    const ImageMesh *mesh_;
  };


  struct NodeIter : public ImageIter
  {
    NodeIter(const ImageMesh *m, unsigned i, unsigned j)
      : ImageIter(m, i, j) {}

    const NodeIndex &operator *() const { return (const NodeIndex&)(*this); }

    NodeIter &operator++()
    {
      i_++;
      if (i_ >= mesh_->min_x_ + mesh_->nx_) {
	i_ = mesh_->min_x_;
	j_++;
      }
      return *this;
    }

  private:

    NodeIter operator++(int)
    {
      NodeIter result(*this);
      operator++();
      return result;
    }
  };


  struct FaceIter : public ImageIter
  {
    FaceIter(const ImageMesh *m, unsigned i, unsigned j)
      : ImageIter(m, i, j) {}

    const FaceIndex &operator *() const { return (const FaceIndex&)(*this); }

    FaceIter &operator++()
    {
      i_++;
      if (i_ >= mesh_->min_x_+mesh_->nx_-1) {
	i_ = mesh_->min_x_;
	j_++;
      }
      return *this;
    }

  private:

    FaceIter operator++(int)
    {
      FaceIter result(*this);
      operator++();
      return result;
    }
  };

  //typedef ImageIndex      under_type;

  //! Index and Iterator types required for Mesh Concept.
  struct Node {
    typedef NodeIndex          index_type;
    typedef NodeIter           iterator;
    typedef NodeIndex          size_type;
    typedef vector<index_type> array_type;
  };			
			
  struct Edge {		
    typedef EdgeIndex          index_type;
    typedef EdgeIter           iterator;
    typedef EdgeIndex          size_type;
    typedef vector<index_type> array_type;
  };			
			
  struct Face {		
    typedef FaceIndex          index_type;
    typedef FaceIter           iterator;
    typedef FaceIndex          size_type;
    typedef vector<index_type> array_type;
  };			
			
  struct Cell {		
    typedef CellIndex          index_type;
    typedef CellIter           iterator;
    typedef CellIndex          size_type;
    typedef vector<index_type> array_type;
  };

  typedef Face Elem;

  typedef vector<double>      weight_array;

  friend class NodeIter;
  friend class CellIter;
  friend class EdgeIter;
  friend class FaceIter;

  ImageMesh()
    : min_x_(0), min_y_(0),
      nx_(1), ny_(1) {}
  ImageMesh(unsigned x, unsigned y, const Point &min, const Point &max);
  ImageMesh(ImageMesh* mh, unsigned int mx, unsigned int my,
	    unsigned int x, unsigned int y)
    : min_x_(mx), min_y_(my), nx_(x), ny_(y), transform_(mh->transform_) {}
  ImageMesh(const ImageMesh &copy)
    : min_x_(copy.min_x_), min_y_(copy.min_y_),
      nx_(copy.get_nx()), ny_(copy.get_ny()), transform_(copy.transform_) {}
  virtual ImageMesh *clone() { return new ImageMesh(*this); }
  virtual ~ImageMesh() {}

  template <class I> I tbegin(I*) const;
  template <class I> I tend(I*) const;
  template <class S> S tsize(S*) const;

  Node::iterator  node_begin() const;
  Node::iterator  node_end() const;
  Node::size_type nodes_size() const;

  Edge::iterator  edge_begin() const;
  Edge::iterator  edge_end() const;
  Edge::size_type edges_size() const;

  Face::iterator  face_begin() const;
  Face::iterator  face_end() const;
  Face::size_type faces_size() const;

  Cell::iterator  cell_begin() const;
  Cell::iterator  cell_end() const;
  Cell::size_type cells_size() const;

  //! get the mesh statistics
  unsigned get_nx() const { return nx_; }
  unsigned get_ny() const { return ny_; }
  Vector diagonal() const { return get_bounding_box().diagonal(); }
  virtual BBox get_bounding_box() const;
  virtual void transform(Transform &t);

  //! set the mesh statistics
  void set_min_x(unsigned x) {min_x_ = x; }
  void set_min_y(unsigned y) {min_y_ = y; }
  void set_nx(unsigned x) { nx_ = x; }
  void set_ny(unsigned y) { ny_ = y; }

  //! get the child elements of the given index
  void get_nodes(Node::array_type &, Edge::index_type) const {}
  void get_nodes(Node::array_type &, Face::index_type) const;
  void get_nodes(Node::array_type &, Cell::index_type) const {}
  void get_edges(Edge::array_type &, Face::index_type) const {}
  void get_edges(Edge::array_type &, Cell::index_type) const {}
  void get_faces(Face::array_type &, Cell::index_type) const {}

  //! get the parent element(s) of the given index
  unsigned get_edges(Edge::array_type &, Node::index_type) const { return 0; }
  unsigned get_faces(Face::array_type &, Node::index_type) const { return 0; }
  unsigned get_faces(Face::array_type &, Edge::index_type) const { return 0; }
  unsigned get_cells(Cell::array_type &, Node::index_type) const { return 0; }
  unsigned get_cells(Cell::array_type &, Edge::index_type) const { return 0; }
  unsigned get_cells(Cell::array_type &, Face::index_type) const { return 0; }

  //! return all face_indecies that overlap the BBox in arr.
  void get_faces(Face::array_type &arr, const BBox &box);

  //! similar to get_faces() with Face::index_type argument, but
  //  returns the "other" face if it exists, not all that exist
  bool get_neighbor(Face::index_type & /*neighbor*/, Face::index_type /*from*/,
		    Edge::index_type /*idx*/) const {
    ASSERTFAIL("ImageMesh::get_neighbor not implemented.");
  }

  //! get the center point (in object space) of an element
  void get_center(Point &, Node::index_type) const;
  void get_center(Point &, Edge::index_type) const {}
  void get_center(Point &, Face::index_type) const;
  void get_center(Point &, Cell::index_type) const {}

  bool locate(Node::index_type &, const Point &);
  bool locate(Edge::index_type &, const Point &) const { return false; }
  bool locate(Face::index_type &, const Point &);
  bool locate(Cell::index_type &, const Point &) const { return false; }

  void get_weights(const Point &p, Node::array_type &l, vector<double> &w);
  void get_weights(const Point &, Edge::array_type &, vector<double> &) {ASSERTFAIL("ImageMesh::get_weights for edges isn't supported");}
  void get_weights(const Point &p, Face::array_type &l, vector<double> &w);
  void get_weights(const Point &, Cell::array_type &, vector<double> &) {ASSERTFAIL("ImageMesh::get_weights for cells isn't supported");}

  void get_point(Point &p, Node::index_type i) const
  { get_center(p, i); }
  void get_normal(Vector &/* result */, Node::index_type /* index */) const
  { ASSERTFAIL("not implemented") }


  virtual void io(Piostream&);
  static PersistentTypeID type_id;
  static  const string type_name(int n = -1);
  virtual const string get_type_name(int n = -1) const { return type_name(n); }

private:

  //! the min_Node::index_type ( incase this is a subLattice )
  unsigned min_x_, min_y_;
  //! the Node::index_type space extents of a ImageMesh
  //! (min=min_Node::index_type, max=min+extents-1)
  unsigned nx_, ny_;

  //! the object space extents of a ImageMesh
  Transform transform_;

  // returns a ImageMesh
  static Persistent *maker() { return new ImageMesh(); }
};

typedef LockingHandle<ImageMesh> ImageMeshHandle;

template <> ImageMesh::Node::size_type ImageMesh::tsize(ImageMesh::Node::size_type *) const;
template <> ImageMesh::Edge::size_type ImageMesh::tsize(ImageMesh::Edge::size_type *) const;
template <> ImageMesh::Face::size_type ImageMesh::tsize(ImageMesh::Face::size_type *) const;
template <> ImageMesh::Cell::size_type ImageMesh::tsize(ImageMesh::Cell::size_type *) const;
				
template <> ImageMesh::Node::iterator ImageMesh::tbegin(ImageMesh::Node::iterator *) const;
template <> ImageMesh::Edge::iterator ImageMesh::tbegin(ImageMesh::Edge::iterator *) const;
template <> ImageMesh::Face::iterator ImageMesh::tbegin(ImageMesh::Face::iterator *) const;
template <> ImageMesh::Cell::iterator ImageMesh::tbegin(ImageMesh::Cell::iterator *) const;
				
template <> ImageMesh::Node::iterator ImageMesh::tend(ImageMesh::Node::iterator *) const;
template <> ImageMesh::Edge::iterator ImageMesh::tend(ImageMesh::Edge::iterator *) const;
template <> ImageMesh::Face::iterator ImageMesh::tend(ImageMesh::Face::iterator *) const;
template <> ImageMesh::Cell::iterator ImageMesh::tend(ImageMesh::Cell::iterator *) const;

} // namespace SCIRun

#endif // SCI_project_ImageMesh_h
