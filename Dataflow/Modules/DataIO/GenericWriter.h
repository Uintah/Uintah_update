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
 *  ColorMapWriter.cc: Save persistent representation of a colormap to a file
 *
 *  Written by:
 *   Steven G. Parker
 *   Department of Computer Science
 *   University of Utah
 *   July 1994
 *
 *  Copyright (C) 1994 SCI Group
 */


/*
 * Limitations:
 *   Uses .tcl file with "filename" and "filetype"
 *   Input port must be of type SimpleIPort
 */

#include <Core/GuiInterface/GuiVar.h>
#include <Core/Malloc/Allocator.h>
#include <Dataflow/Network/Module.h>

namespace SCIRun {


template <class HType>
class GenericWriter : public Module {
public:
  GuiString filename_;
  GuiString filetype_;
  GenericWriter(const string &name, const string& id,
		const string &category, const string &package);
  virtual ~GenericWriter();

  virtual void execute();
};


template <class HType>
GenericWriter<HType>::GenericWriter(const string &name, const string& id,
				    const string &cat, const string &pack)
  : Module(name, id, Sink, cat, pack),
    filename_("filename", id, this),
    filetype_("filetype", id, this)
{
}


template <class HType>
GenericWriter<HType>::~GenericWriter()
{
}

template <class HType>
void
GenericWriter<HType>::execute()
{
  SimpleIPort<HType> *inport = (SimpleIPort<HType> *)get_iport(0);
  if (!inport) {
    postMessage("Unable to initialize "+name+"'s iport\n");
    return;
  }

  // Read data from the input port
  HType handle;
  if (!inport->get(handle) || !handle.get_rep())
  {
    remark("No data on input port.");
    return;
  }

  // If no name is provided, return.
  const string fn(filename_.get());
  if (fn == "")
  {
    warning("No filename specified.");
    return;
  }
   
  // Open up the output stream
  Piostream* stream;
  string ft(filetype_.get());
  if (ft == "Binary")
  {
    stream = scinew BinaryPiostream(fn, Piostream::Write);
  }
  else
  {
    stream = scinew TextPiostream(fn, Piostream::Write);
  }

  if (stream->error()) {
    error("Could not open file for writing" + fn);
  } else {
    // Write the file
    Pio(*stream, handle);
    delete stream;
  } 
}

} // End namespace SCIRun
