//  SField.h - Scalar Field
//
//  Written by:
//   Eric Kuehne
//   Department of Computer Science
//   University of Utah
//   April 2000
//
//  Copyright (C) 2000 SCI Institute


#ifndef SCI_project_SField_h
#define SCI_project_SField_h 1


#include <SCICore/Datatypes/Datatype.h>
#include <SCICore/Containers/LockingHandle.h>
#include <SCICore/Datatypes/Field.h>
#include <SCICore/Datatypes/FieldInterface.h>

namespace SCICore{
namespace Datatypes{

using SCICore::Containers::LockingHandle;
using SCICore::PersistentSpace::Piostream;
using SCICore::PersistentSpace::PersistentTypeID;

class SField;
typedef LockingHandle<SField> SFieldHandle;


class SCICORESHARE SField:public Field{
public:
  //////////
  // Constructor
  SField();

  //////////
  // Destructor
  ~SField();

  //////////
  // Used by SFieldHandle to obtain a SField*
  inline SField* get_base() {return this;};


  //////////
  // Persistent representation...
  virtual void io(Piostream&);
  static PersistentTypeID type_id;
private:  
};

} // end SCICore
} // end Datatypes

#endif
