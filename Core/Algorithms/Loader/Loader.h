/* Loader.h
 *
 *  Written by:
 *   Yarden Livnat
 *   Department of Computer Science
 *   University of Utah
 *   March 2001
 *
 *  Copyright (C) 2001 SCI Institute
 *
 *  Manage dynamic properties of persistent objects.
 */

#ifndef Loader_h
#define Loader_h 

#include <map>

namespace SCIRun {


/*
 * Loadable
 */

class LoadableBase {
public:
  void *obj_;
  virtual ~LoadableBase() {}
};

template<class T>
class Loadable: public T, public LoadableBase {
public:
  Loadable( T *o) { obj_ = o; } 
};


/*
 * Loader
 */

class Loader 
{
public:
  Loader() {}
  Loader(const Loader &copy) {}
  virtual ~Loader() {}

  
  template<class T> void store( const string &, T *);
  template<class T> bool get( const string &, T *&);

private:
  typedef map<string, LoadableBase *> map_type;

  map_type objects_;
};


template<class T>
void 
Loader::store( const string &name,  T * obj )
{
  map_type::iterator loc = objects_.find(name);
  if (loc != objects_.end()) 
    delete loc->second;
    
  objects_[name] = new Loadable<T>( obj );
}

  
template<class T>
bool 
Loader::get(const string &name, T *&ref)
{
  map_type::iterator loc = objects_.find(name);
  if (loc != objects_.end()) {
    if ( dynamic_cast<T *>( loc->second ) ) {
      ref = static_cast<T *>(loc->second->obj_);
      return true;
    }
  }
  
  // either property not found, or it can not be cast to T
  return false;
}

} // namespace SCIRun

#endif // SCI_project_Loader_h
