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
 *  ManageFieldData: Store/retrieve values from an input matrix to/from 
 *            the data of a field
 *
 *  Written by:
 *   Michael Callahan
 *   Department of Computer Science
 *   University of Utah
 *   February 2001
 *
 *  Copyright (C) 2001 SCI Institute
 */

#include <Core/Persistent/Pstreams.h>
#include <Dataflow/Network/Module.h>
#include <Dataflow/Ports/MatrixPort.h>
#include <Dataflow/Ports/FieldPort.h>
#include <Dataflow/Modules/Fields/ManageFieldData.h>
#include <Core/GuiInterface/GuiVar.h>
#include <Core/Containers/Handle.h>
#include <iostream>
#include <stdio.h>

namespace SCIRun {

class ManageFieldData : public Module
{
public:
  ManageFieldData(GuiContext* ctx);
  virtual ~ManageFieldData();

  virtual void execute();
};


DECLARE_MAKER(ManageFieldData)
ManageFieldData::ManageFieldData(GuiContext* ctx)
  : Module("ManageFieldData", ctx, Filter, "Fields", "SCIRun")
{
}



ManageFieldData::~ManageFieldData()
{
}



void
ManageFieldData::execute()
{
  // Get input field.
  FieldIPort *ifp = (FieldIPort *)get_iport("Input Field");
  FieldHandle ifieldhandle;
  if (!ifp) {
    error("Unable to initialize iport 'Input Field'.");
    return;
  }
  if (!(ifp->get(ifieldhandle) && (ifieldhandle.get_rep())))
  {
    error( "No handle or representation." );
    return;
  }

  int svt_flag = 0;
  if (ifieldhandle->query_scalar_interface(this))
  {
    svt_flag = 0;
  }
  else if (ifieldhandle->query_vector_interface(this))
  {
    svt_flag = 1;
  }
  else if (ifieldhandle->query_tensor_interface(this))
  {
    svt_flag = 2;
  }
  
  // Compute output matrix.
  CompileInfoHandle ci_field =
    ManageFieldDataAlgoField::
    get_compile_info(ifieldhandle->get_type_description(),
		     ifieldhandle->data_at_type_description(),
		     svt_flag);
  Handle<ManageFieldDataAlgoField> algo_field;
  if (!module_dynamic_compile(ci_field, algo_field)) return;

  MatrixOPort *omp = (MatrixOPort *)get_oport("Output Matrix");
  if (!omp) {
    error("Unable to initialize oport 'Output Matrix'.");
  }
  else
  {
    omp->send(algo_field->execute(ifieldhandle));
  }

  // Compute output field.
  FieldHandle result_field;
  MatrixIPort *imatrix_port = (MatrixIPort *)get_iport("Input Matrix");
  MatrixHandle imatrixhandle;
  if (!imatrix_port) {
    error("Unable to initialize iport 'Input Matrix'.");
    return;
  }
  if (!(imatrix_port->get(imatrixhandle) && imatrixhandle.get_rep()))
  {
    remark("No input matrix connected, sending field as is.");
    result_field = ifieldhandle;
  }
  else
  {
    int matrix_svt_flag = svt_flag;
    if (imatrixhandle->nrows() == 1 || imatrixhandle->ncols() == 1)
    {
      matrix_svt_flag = 0;
    }
    else if (imatrixhandle->nrows() == 3 || imatrixhandle->ncols() == 3)
    {
      matrix_svt_flag = 1;
    }
    else if (imatrixhandle->nrows() == 9 || imatrixhandle->ncols() == 9)
    {
      matrix_svt_flag = 2;
    }

    CompileInfoHandle ci_mesh =
      ManageFieldDataAlgoMesh::
      get_compile_info(ifieldhandle->mesh()->get_type_description(),
		       ifieldhandle->get_type_description(),
		       matrix_svt_flag);
    Handle<ManageFieldDataAlgoMesh> algo_mesh;
    if (!module_dynamic_compile(ci_mesh, algo_mesh)) return;

    result_field =
      algo_mesh->execute(this, ifieldhandle->mesh(), imatrixhandle);
  }

  string units;
  if (imatrixhandle.get_rep() && imatrixhandle->get_property("units", units))
    result_field->set_property("units", units, false);
  
  FieldOPort *ofp = (FieldOPort *)get_oport("Output Field");
  if (!ofp) {
    error("Unable to initialize oport 'Output Field'.");
    return;
  }
  ofp->send(result_field);
}



CompileInfoHandle
ManageFieldDataAlgoField::get_compile_info(const TypeDescription *fsrc,
					   const TypeDescription *lsrc,
					   int svt_flag)
{
  // Use cc_to_h if this is in the .cc file, otherwise just __FILE__
  static const string include_path(TypeDescription::cc_to_h(__FILE__));
  static const string base_class_name("ManageFieldDataAlgoField");

  string extension;
  switch (svt_flag)
  {
  case 2:
    extension = "Tensor";
    break;

  case 1:
    extension = "Vector";
    break;

  default:
    extension = "Scalar";
    break;
  }

  CompileInfo *rval = 
    scinew CompileInfo(base_class_name + extension + "." +
		       fsrc->get_filename() + "." +
		       lsrc->get_filename() + ".",
                       base_class_name, 
                       base_class_name + extension, 
                       fsrc->get_name() + ", " + lsrc->get_name());

  // Add in the include path to compile this obj
  rval->add_include(include_path);
  fsrc->fill_compile_info(rval);
  return rval;
}



CompileInfoHandle
ManageFieldDataAlgoMesh::get_compile_info(const TypeDescription *msrc,
					  const TypeDescription *fsrc,
					  int svt_flag)
{
  // Use cc_to_h if this is in the .cc file, otherwise just __FILE__
  static const string include_path(TypeDescription::cc_to_h(__FILE__));
  static const string base_class_name("ManageFieldDataAlgoMesh");

  string extension;
  string extension2;
  switch (svt_flag)
  {
  case 2:
    extension = "Tensor";
    extension2 = "Tensor";
    break;

  case 1:
    extension = "Vector";
    extension2 = "Vector";
    break;

  default:
    extension = "Scalar";
    extension2 = "double";
    break;
  }

  string::size_type loc = fsrc->get_name().find_first_of("<");
  string fout = fsrc->get_name().substr(0, loc) + "<" + extension2 + "> ";

  CompileInfo *rval = 
    scinew CompileInfo(base_class_name + extension + "." +
		       msrc->get_filename() + "." +
		       to_filename(fout) + ".",
                       base_class_name, 
                       base_class_name + extension, 
                       msrc->get_name() + ", " + fout);

  // Add in the include path to compile this obj
  rval->add_include(include_path);
  fsrc->fill_compile_info(rval);
  return rval;
}



} // End namespace SCIRun

