
/*
 *  ScalarFieldWriter.cc: ScalarField Writer class
 *
 *  Written by:
 *   Steven G. Parker
 *   Department of Computer Science
 *   University of Utah
 *   July 1994
 *
 *  Copyright (C) 1994 SCI Group
 */

#include <Core/Persistent/Pstreams.h>
#include <Dataflow/Network/Module.h>
#include <Dataflow/Ports/ScalarFieldPort.h>
#include <Core/Datatypes/ScalarFieldRG.h>
#include <Core/Datatypes/ScalarField.h>
#include <Core/Malloc/Allocator.h>
#include <Core/TclInterface/TCLvar.h>
#include <iostream>
using std::cerr;

namespace SCIRun {


class MultiScalarFieldWriter : public Module {
    ScalarFieldIPort* inport;
    TCLstring filename;
    TCLstring filetype;
public:
    MultiScalarFieldWriter(const clString& id);
    virtual ~MultiScalarFieldWriter();
    virtual void execute();
};

extern "C" Module* make_MultiScalarFieldWriter(const clString& id) {
  return new MultiScalarFieldWriter(id);
}

MultiScalarFieldWriter::MultiScalarFieldWriter(const clString& id)
: Module("MultiScalarFieldWriter", id, Source), filename("filename", id, this),
  filetype("filetype", id, this)
{
    // Create the output data handle and port
    inport=scinew ScalarFieldIPort(this, "Input Data", ScalarFieldIPort::Atomic);
    add_iport(inport);
}

MultiScalarFieldWriter::~MultiScalarFieldWriter()
{
}

void MultiScalarFieldWriter::execute()
{

    ScalarFieldHandle handle;
    if(!inport->get(handle))
	return;
    clString fn(filename.get());
    if(fn == "")
	return;
    Piostream* stream;
    clString ft(filetype.get());

    char hun = '0',ten='0',one='1';

    ScalarFieldHandle  *temp_handle;
    ScalarFieldRG *RG = handle.get_rep()->getRG();

    if (!RG)
      return; // has to be a RG

    while (RG) {
      clString tmps = fn;
      temp_handle = scinew ScalarFieldHandle;
      *temp_handle = (ScalarField*)RG;
      
      tmps += hun;
      tmps += ten;
      tmps += one;

      cerr << "Trying "+tmps << "\n";

      if(ft=="Binary"){
	stream=scinew BinaryPiostream(tmps, Piostream::Write);
      } else {
	stream=scinew TextPiostream(tmps, Piostream::Write);
      }
      // Write the file
      Pio(*stream, *temp_handle);
      delete stream;
#ifdef NEEDAUGDATA
      RG = (ScalarFieldRG*)RG->next;
#endif
      one = one + 1;
      if (one > '9') {
	ten = ten + 1;
	if (ten > '9') {
	  hun = hun+1; // shouldn't go over...
	  ten = '0';
	}
	one = '0';
      }      
    }
}

} // End namespace SCIRun

