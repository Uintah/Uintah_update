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

//    File   : ChangeFieldDataAt.cc
//    Author : McKay Davis
//    Date   : July 2002


#include <Dataflow/Network/Module.h>
#include <Dataflow/Ports/FieldPort.h>
#include <Dataflow/Ports/MatrixPort.h>
#include <Dataflow/Modules/Fields/ChangeFieldDataAt.h>
#include <Core/Containers/StringUtil.h>
#include <map>
#include <iostream>

namespace SCIRun {


class PSECORESHARE ChangeFieldDataAt : public Module {
public:
  GuiString outputdataat_;    // the out data at
  GuiString inputdataat_;     // the in data at
  GuiString fldname_;         // the field name
  int              generation_;
  ChangeFieldDataAt(GuiContext* ctx);
  virtual ~ChangeFieldDataAt();
  virtual void execute();
  void update_input_attributes(FieldHandle);
};

  DECLARE_MAKER(ChangeFieldDataAt)

ChangeFieldDataAt::ChangeFieldDataAt(GuiContext* ctx)
  : Module("ChangeFieldDataAt", ctx, Filter, "FieldsData", "SCIRun"),
    outputdataat_(ctx->subVar("outputdataat")),
    inputdataat_(ctx->subVar("inputdataat", false)),
    fldname_(ctx->subVar("fldname", false)),
    generation_(-1)
{
}

ChangeFieldDataAt::~ChangeFieldDataAt()
{
  fldname_.set("---");
  inputdataat_.set("---");
}


void
ChangeFieldDataAt::update_input_attributes(FieldHandle f) 
{
  switch(f->data_at())
  {
  case Field::NODE:
    inputdataat_.set("Nodes");
    break;
  case Field::EDGE: 
    inputdataat_.set("Edges");
    break;
  case Field::FACE: 
    inputdataat_.set("Faces");
    break;
  case Field::CELL: 
    inputdataat_.set("Cells");
    break;
  case Field::NONE: 
    inputdataat_.set("None");
    break;
  default: ;
  }

  string fldname;
  if (f->get_property("name",fldname))
  {
    fldname_.set(fldname);
  }
  else
  {
    fldname_.set("--- No Name ---");
  }
}


void
ChangeFieldDataAt::execute()
{
  FieldIPort *iport = (FieldIPort*)get_iport("Input Field"); 
  if (!iport) {
    error("Unable to initialize iport 'Input Field'.");
    return;
  }
  
  // The input port (with data) is required.
  FieldHandle fh;
  if (!iport->get(fh) || !fh.get_rep())
  {
    fldname_.set("---");
    inputdataat_.set("---");
    return;
  }

  if (generation_ != fh.get_rep()->generation)
  {
    update_input_attributes(fh);
    generation_ = fh.get_rep()->generation;
  }

  // The output port is required.
  FieldOPort *ofport = (FieldOPort*)get_oport("Output Field");
  if (!ofport) {
    error("Unable to initialize oport 'Output Field'.");
    return;
  }

  // The output port is required.
  MatrixOPort *omport = (MatrixOPort*)get_oport("Interpolant");
  if (!omport) {
    error("Unable to initialize oport 'Interpolant'.");
    return;
  }

  Field::data_location dataat = fh->data_at();
  const string &d = outputdataat_.get();
  if (d == "Nodes")
  {
    dataat = Field::NODE;
  }
  else if (d == "Edges")
  {
    dataat = Field::EDGE;
  }
  else if (d == "Faces")
  {
    dataat = Field::FACE;
  }
  else if (d == "Cells")
  {
    dataat = Field::CELL;
  }
  else if (d == "None")
  {
    dataat = Field::NONE;
  }

  if (dataat == fh->data_at())
  {
    // No changes, just send the original through (it may be nothing!).
    remark("Passing field from input port to output port unchanged.");
    warning("Interpolant for that location combination is not yet supported.");
    ofport->send(fh);
    omport->send(0);
    return;
  }

  // Create a field identical to the input, except for the edits.
  const TypeDescription *fsrctd = fh->get_type_description();
  CompileInfoHandle ci =
    ChangeFieldDataAtAlgoCreate::get_compile_info(fsrctd);
  Handle<ChangeFieldDataAtAlgoCreate> algo;
  if (!DynamicCompilation::compile(ci, algo, this)) return;

  update_state(Executing);
  MatrixHandle interpolant(0);
  FieldHandle ef(algo->execute(this, fh, dataat, interpolant));

  if (interpolant.get_rep() == 0)
  {
    if (omport->nconnections() > 0)
    {
      error("Interpolant for that location combination is not supported.");
    }
    else
    {
      remark("Interpolant for that location combination is not supported.");
    }
  }

  ofport->send(ef);
  omport->send(interpolant);
}

    

CompileInfoHandle
ChangeFieldDataAtAlgoCreate::get_compile_info(const TypeDescription *field_td)
{
  // Use cc_to_h if this is in the .cc file, otherwise just __FILE__
  static const string include_path(TypeDescription::cc_to_h(__FILE__));
  static const string template_class("ChangeFieldDataAtAlgoCreateT");
  static const string base_class_name("ChangeFieldDataAtAlgoCreate");

  CompileInfo *rval = 
    scinew CompileInfo(template_class + "." +
		       field_td->get_filename() + ".",
		       base_class_name, 
		       template_class,
                       field_td->get_name());

  // Add in the include path to compile this obj
  rval->add_include(include_path);
  field_td->fill_compile_info(rval);
  return rval;
}

} // End namespace Moulding


