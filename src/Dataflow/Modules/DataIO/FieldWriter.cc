/*
 *  FieldWriter.cc: Write Field datatype persistent objects
 *
 *  Written by:
 *   Alexei Samsonov
 *   Department of Computer Science
 *   University of Utah
 *   December 2000
 *
 *  Copyright (C) 2000 SCI Group
 */

#include <Dataflow/Network/Module.h>
#include <Core/Datatypes/Field.h>
#include <Dataflow/Ports/FieldPort.h>
#include <Core/Persistent/Pstreams.h>
#include <Core/Malloc/Allocator.h>
#include <Core/GuiInterface/GuiVar.h>

namespace SCIRun {

class FieldWriter : public Module {
  
  // GROUP: private data
  //////////
  // 
  FieldIPort*     d_iport;
  GuiString       d_filename;
  GuiString       d_filetype;

public:
  // GROUP: Constructor/Destructor
  //////////
  //
  FieldWriter(const clString& id);
  virtual ~FieldWriter();

  // GROUP: public member functions
  //////////
  //
  virtual void execute();
};

//////////
// Module maker function
extern "C" Module* make_FieldWriter(const clString& id) {
  return new FieldWriter(id);
}

//////////
// Constructor/Desctructor
FieldWriter::FieldWriter(const clString& id): 
  Module("FieldWriter", id, Source), 
  d_filename("d_filename", id, this),
  d_filetype("d_filetype", id, this)
{
  d_iport=scinew FieldIPort(this, "Input Data", FieldIPort::Atomic);
  add_iport(d_iport);
}

FieldWriter::~FieldWriter()
{
}

//////////
//
void FieldWriter::execute()
{
  FieldHandle handle;
  
  if(!d_iport->get(handle))
    return;
  
  clString fn(d_filename.get());
  
  if(fn == "")
    return;
  
  Piostream* stream;
  clString ft(d_filetype.get());
  
  if(ft=="Binary"){
    stream=scinew BinaryPiostream(fn, Piostream::Write);
  } else {
    stream=scinew TextPiostream(fn, Piostream::Write);
  }
  
  Pio(*stream, handle);
  delete stream;
}

} // End namespace SCIRun

