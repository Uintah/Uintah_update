/*
 *  MaskedTetVol.h
 *
 *  Written by:
 *   Martin Cole
 *   School of Computing
 *   University of Utah
 *
 *  Copyright (C) 2001 SCI Institute
 */

#ifndef Datatypes_MaskedTetVol_h
#define Datatypes_MaskedTetVol_h

#include <Core/Datatypes/Field.h>
#include <Core/Datatypes/TetVol.h>
#include <Core/Datatypes/GenericField.h>
#include <Core/Containers/LockingHandle.h>
#include <Core/Persistent/PersistentSTL.h>
#include <vector>

namespace SCIRun {

template <class T> 
class MaskedTetVol : public TetVol<T> {
private:
  vector<char> mask_;  // since Pio isn't implemented for bool's
public:
  vector<char>& mask() { return mask_; }

  MaskedTetVol() :
    TetVol<T>() {};
  MaskedTetVol(Field::data_location data_at) : 
    TetVol<T>(data_at) {};
  MaskedTetVol(TetVolMeshHandle mesh, Field::data_location data_at) : 
    TetVol<T>(mesh, data_at) 
  {
    resize_fdata();
  };

  virtual ~MaskedTetVol() {};

  bool value(T &val, typename TetVolMesh::node_index i) const
  { if (!mask_[i]) return false; val = fdata_[i]; return true; }
  bool value(T &val, typename TetVolMesh::edge_index i) const
  { if (!mask_[i]) return false; val = fdata_[i]; return true; }
  bool value(T &val, typename TetVolMesh::face_index i) const
  { if (!mask_[i]) return false; val = fdata_[i]; return true; }
  bool value(T &val, typename TetVolMesh::cell_index i) const
  { if (!mask_[i]) return false; val = fdata_[i]; return true; }

  void    io(Piostream &stream);

  void resize_fdata() {
    if (data_at() == NODE)
      mask_.resize(get_typed_mesh()->nodes_size());
    else if (data_at() == EDGE)
      mask_.resize(get_typed_mesh()->edges_size());
    else if (data_at() == FACE)
      ASSERTFAIL("tetvol doesn't support data at faces (yet)")
    else
      ASSERTFAIL("data at unrecognized location")
    TetVol<T>::resize_fdata();
  }

  void initialize_mask(int) {}

  static  PersistentTypeID type_id;
  static const string type_name(int n = -1);
  virtual const string get_type_name(int n = -1) const { return type_name(n); }

private:
  static Persistent *maker();
};

// Pio defs.
const int MASKED_TET_VOL_VERSION = 1;

template <class T>
Persistent*
MaskedTetVol<T>::maker()
{
  return scinew MaskedTetVol<T>;
}

template <class T>
PersistentTypeID 
MaskedTetVol<T>::type_id(type_name(), 
			 GenericField<TetVolMesh, vector<T> >::type_name(),
			 maker);


template <class T>
void 
MaskedTetVol<T>::io(Piostream& stream)
{
  stream.begin_class(type_name().c_str(), MASKED_TET_VOL_VERSION);
  GenericField<TetVolMesh, vector<T> >::io(stream);
  Pio(stream, mask_);
  stream.end_class();
}

template <class T> 
const string 
MaskedTetVol<T>::type_name(int n)
{
  ASSERT((n >= -1) && n <= 1);
  if (n == -1)
  {
    static const string name = type_name(0) + FTNS + type_name(1) + FTNE;
    return name;

  }
  else if (n == 0)
  {
    return "MaskedTetVol";
  }
  else
  {
    return find_type_name((T *)0);
  }
}

} // end namespace SCIRun

#endif // Datatypes_MaskedTetVol_h
