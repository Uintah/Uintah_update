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

#include <Packages/ModelCreation/Core/Fields/MappingMatrixToField.h>

namespace ModelCreation {

using namespace SCIRun;

CompileInfoHandle MappingMatrixToFieldAlgo::get_compile_info(FieldHandle& field)
{
  const SCIRun::TypeDescription *ifieldtype = field->get_type_description();
  const SCIRun::TypeDescription *ofieldtype = field->get_type_description(0);
  
  std::string ifieldname = ifieldtype->get_name();
  std::string ofieldname = ofieldtype->get_name() + "<unsigned integer> ";
        
  // As I use my own Tensor and Vector algorithms they need to be
  // converted when reading the data, hence separate algorithms are
  // implemented for those cases
  
  std::string algo_name = "MappingMatrixToFieldAlgoT";
  std::string algo_base = "MappingMatrixToFieldAlgo";

  std::string include_path(SCIRun::TypeDescription::cc_to_h(__FILE__));

  SCIRun::CompileInfoHandle ci = 
    scinew SCIRun::CompileInfo(algo_name + "." +
                       ifieldtype->get_filename() + ".",
                       algo_base, 
                       algo_name, 
                       ifieldname + "," + ofieldname);

  ci->add_include(include_path);
  ci->add_namespace("ModelCreation");
  ifieldtype->fill_compile_info(ci.get_rep());
  return(ci);
}

} //end namespace
