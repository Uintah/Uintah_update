//static char *id="@(#) $Id$";

/*
 *  VectorFieldReader.cc: VectorField Reader class
 *
 *  Written by:
 *   Steven G. Parker
 *   Department of Computer Science
 *   University of Utah
 *   July 1994
 *
 *  Copyright (C) 1994 SCI Group
 */

#include <SCICore/Util/NotFinished.h>
#include <PSECore/Dataflow/Module.h>
#include <PSECore/CommonDatatypes/VectorFieldPort.h>
#include <SCICore/CoreDatatypes/VectorField.h>
#include <SCICore/Malloc/Allocator.h>
#include <SCICore/TclInterface/TCLTask.h>
#include <SCICore/TclInterface/TCLvar.h>

namespace PSECommon {
namespace Modules {

using namespace PSECore::Dataflow;
using namespace PSECore::CommonDatatypes;
using namespace SCICore::TclInterface;
using namespace SCICore::PersistentSpace;

class VectorFieldReader : public Module {
    VectorFieldOPort* outport;
    TCLstring filename;
    VectorFieldHandle handle;
    clString old_filename;
public:
    VectorFieldReader(const clString& id);
    virtual ~VectorFieldReader();
    virtual void execute();
};

Module* make_VectorFieldReader(const clString& id) {
  return new VectorFieldReader(id);
}

VectorFieldReader::VectorFieldReader(const clString& id)
: Module("VectorFieldReader", id, Source), filename("filename", id, this)
{
    // Create the output data handle and port
    outport=scinew VectorFieldOPort(this, "Output Data", VectorFieldIPort::Atomic);
    add_oport(outport);
}

VectorFieldReader::~VectorFieldReader()
{
}

#ifdef BROKEN
static void watcher(double pd, void* cbdata)
{
    VectorFieldReader* reader=(VectorFieldReader*)cbdata;
    if(TCLTask::try_lock()){
	// Try the malloc lock once before we call update_progress
	// If we can't get it, then back off, since our caller might
	// have it locked
	if(!Task::test_malloc_lock()){
	    TCLTask::unlock();
	    return;
	}
	reader->update_progress(pd);
	TCLTask::unlock();
    }
}
#endif

void VectorFieldReader::execute()
{
    using SCICore::Containers::Pio;

    clString fn(filename.get());
    if(!handle.get_rep() || fn != old_filename){
	old_filename=fn;
	Piostream* stream=auto_istream(fn);
	if(!stream){
	    error(clString("Error reading file: ")+filename.get());
	    return; // Can't open file...
	}
	// Read the file...
//	stream->watch_progress(watcher, (void*)this);
	Pio(*stream, handle);
	if(!handle.get_rep() || stream->error()){
	    error("Error reading VectorField from file");
	    delete stream;
	    return;
	}
	delete stream;
    }
    outport->send(handle);
}

} // End namespace Modules
} // End namespace PSECommon

//
// $Log$
// Revision 1.3  1999/08/18 20:19:52  sparker
// Eliminated copy constructor and clone in all modules
// Added a private copy ctor and a private clone method to Module so
//  that future modules will not compile until they remvoe the copy ctor
//  and clone method
// Added an ASSERTFAIL macro to eliminate the "controlling expression is
//  constant" warnings.
// Eliminated other miscellaneous warnings
//
// Revision 1.2  1999/08/17 06:37:36  sparker
// Merged in modifications from PSECore to make this the new "blessed"
// version of SCIRun/Uintah.
//
// Revision 1.1  1999/07/27 16:57:49  mcq
// Initial commit
//
// Revision 1.3  1999/07/07 21:10:27  dav
// added beginnings of support for g++ compilation
//
// Revision 1.2  1999/04/27 22:57:54  dav
// updates in Modules for CoreDatatypes
//
// Revision 1.1  1999/04/25 02:38:11  dav
// more things that should have been there but were not
//
//
