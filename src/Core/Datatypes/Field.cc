/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2004 Scientific Computing and Imaging Institute,
   University of Utah.

   License for the specific language governing rights and limitations under
   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
*/



#include <Core/Datatypes/Field.h>
#include <Core/Datatypes/FieldInterfaceAux.h>
#include <Core/Util/ProgressReporter.h>

namespace SCIRun{

// initialize the static member type_id
PersistentTypeID Field::type_id("Field", "PropertyManager", 0);


Field::Field(int order) :
  order_(order)
{
}

Field::~Field()
{
}

const int FIELD_VERSION = 2;

void 
Field::io(Piostream& stream){
  int version = stream.begin_class("Field", FIELD_VERSION);
  if (version < FIELD_VERSION) {

    // The following was FIELD_VERSION 1 data_at ordering
    //     enum data_location{
    //       NODE,
    //       EDGE,
    //       FACE,
    //       CELL,
    //       NONE
    //     };

    unsigned int tmp;
    Pio(stream, tmp);
    if (tmp == 1) {
      // data_at_ was NODE
      order_ = 1;
    } else {
      // data_at_ was somewhere else
      order_ = 0;
    }
  } else {
    Pio(stream, order_);
  }
  
  PropertyManager::io(stream);
  stream.end_class();
}

const string
Field::get_type_name(int n) const
{
  return get_type_description(n)->get_name();
}


ScalarFieldInterfaceHandle
Field::query_scalar_interface(ProgressReporter *reporter)
{
  const TypeDescription *ftd = get_type_description();
  const TypeDescription *ltd = order_type_description();
  CompileInfoHandle ci = ScalarFieldInterfaceMaker::get_compile_info(ftd, ltd);
  LockingHandle<ScalarFieldInterfaceMaker> algo(0);
  
  ProgressReporter my_reporter;
  if (!reporter) reporter = &my_reporter;
  if ( DynamicCompilation::compile( ci, algo, true, reporter ) )
    return algo->make(this);
  else
    return 0;
}


VectorFieldInterfaceHandle
Field::query_vector_interface(ProgressReporter *reporter)
{
  const TypeDescription *ftd = get_type_description();
  const TypeDescription *ltd = order_type_description();
  CompileInfoHandle ci = VectorFieldInterfaceMaker::get_compile_info(ftd, ltd);
  LockingHandle<VectorFieldInterfaceMaker> algo(0);
  
  ProgressReporter my_reporter;
  if (!reporter) reporter = &my_reporter;
  if ( DynamicCompilation::compile( ci, algo, true, reporter ) )
    return algo->make(this);
  else
    return 0;
}


TensorFieldInterfaceHandle
Field::query_tensor_interface(ProgressReporter *reporter)
{
  const TypeDescription *ftd = get_type_description();
  const TypeDescription *ltd = order_type_description();
  CompileInfoHandle ci = TensorFieldInterfaceMaker::get_compile_info(ftd, ltd);
  LockingHandle<TensorFieldInterfaceMaker> algo(0);
  
  ProgressReporter my_reporter;
  if (!reporter) reporter = &my_reporter;
  if ( DynamicCompilation::compile( ci, algo, true, reporter ) )
    return algo->make(this);
  else
    return 0;
}

}
