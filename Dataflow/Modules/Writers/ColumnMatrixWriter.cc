//static char *id="@(#) $Id$";

/*
 *  ColumnMatrixWriter.cc: ColumnMatrix Writer class
 *
 *  Written by:
 *   Steven G. Parker
 *   Department of Computer Science
 *   University of Utah
 *   July 1994
 *
 *  Copyright (C) 1994 SCI Group
 */

#include <SCICore/Persistent/Pstreams.h>
#include <PSECore/Dataflow/Module.h>
#include <PSECore/Datatypes/ColumnMatrixPort.h>
#include <SCICore/Datatypes/ColumnMatrix.h>
#include <SCICore/Malloc/Allocator.h>
#include <SCICore/TclInterface/TCLvar.h>

namespace PSECommon {
namespace Modules {

using namespace PSECore::Dataflow;
using namespace PSECore::Datatypes;
using namespace SCICore::TclInterface;
using namespace SCICore::PersistentSpace;

class ColumnMatrixWriter : public Module {
    ColumnMatrixIPort* inport;
    TCLstring filename;
    TCLstring filetype;
public:
    ColumnMatrixWriter(const clString& id);
    virtual ~ColumnMatrixWriter();
    virtual void execute();
};

Module* make_ColumnMatrixWriter(const clString& id) {
  return new ColumnMatrixWriter(id);
}

ColumnMatrixWriter::ColumnMatrixWriter(const clString& id)
: Module("ColumnMatrixWriter", id, Source), filename("filename", id, this),
  filetype("filetype", id, this)
{
    // Create the output data handle and port
    inport=scinew ColumnMatrixIPort(this, "Input Data", ColumnMatrixIPort::Atomic);
    add_iport(inport);
}

ColumnMatrixWriter::~ColumnMatrixWriter()
{
}

void ColumnMatrixWriter::execute()
{
    using SCICore::Containers::Pio;

    ColumnMatrixHandle handle;
    if(!inport->get(handle))
	return;
    clString fn(filename.get());
    if(fn == "")
	return;
    Piostream* stream;
    clString ft(filetype.get());
    if(ft=="Binary"){
	stream=scinew BinaryPiostream(fn, Piostream::Write);
    } else {
	stream=scinew TextPiostream(fn, Piostream::Write);
    }
    // Write the file
    Pio(*stream, handle);
    delete stream;
}

} // End namespace Modules
} // End namespace PSECommon

//
// $Log$
// Revision 1.6  1999/10/07 02:07:11  sparker
// use standard iostreams and complex type
//
// Revision 1.5  1999/08/25 03:48:14  sparker
// Changed SCICore/CoreDatatypes to SCICore/Datatypes
// Changed PSECore/CommonDatatypes to PSECore/Datatypes
// Other Misc. directory tree updates
//
// Revision 1.4  1999/08/19 23:18:00  sparker
// Removed a bunch of #include <SCICore/Util/NotFinished.h> statements
// from files that did not need them.
//
// Revision 1.3  1999/08/18 20:20:14  sparker
// Eliminated copy constructor and clone in all modules
// Added a private copy ctor and a private clone method to Module so
//  that future modules will not compile until they remvoe the copy ctor
//  and clone method
// Added an ASSERTFAIL macro to eliminate the "controlling expression is
//  constant" warnings.
// Eliminated other miscellaneous warnings
//
// Revision 1.2  1999/08/17 06:37:56  sparker
// Merged in modifications from PSECore to make this the new "blessed"
// version of SCIRun/Uintah.
//
// Revision 1.1  1999/07/27 16:58:19  mcq
// Initial commit
//
// Revision 1.3  1999/07/07 21:10:31  dav
// added beginnings of support for g++ compilation
//
// Revision 1.2  1999/04/27 22:58:04  dav
// updates in Modules for Datatypes
//
// Revision 1.1  1999/04/25 03:25:34  dav
// adding these files in too... should have been there already... oh well, sigh
//
//
