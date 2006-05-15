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



#ifndef CCA_Components_GUIBuilder_ComponentSkeletonWriter_h
#define CCA_Components_GUIBuilder_ComponentSkeletonWriter_h

#include <vector>
#include <string>
#include <fstream>

namespace GUIBuilder {

class PortDescriptor {
public:
  PortDescriptor(const std::string& cName, const std::string& type, const std::string& uName)
    : cName(cName), type(type) , uName(uName) {}
  const std::string& GetClassName() const { return cName; }
  const std::string& GetType() const { return type; }
  const std::string& GetUniqueName() const { return uName; }

private:
  std::string cName; // name of the concrete port class to be provided or used by the component
                     // (inherits from SIDL port type)
  std::string type; // SIDL port type
  std::string uName; // port name (must be unique in component class)
};

class ComponentSkeletonWriter {
public:
  ComponentSkeletonWriter(const std::string &cname, const std::vector<PortDescriptor*> pp, const std::vector<PortDescriptor*> up);

  void ComponentClassDefinitionCode();
  void ComponentSourceFileCode();
  void ComponentMakefileCode();
  //void PortClassDefinitionCode();
  void GenerateCode();

private:
  void writeLicense(std::ofstream& fileStream);
  void writeMakefileLicense(std::ofstream& fileStream);

  // generate header file
  void writeHeaderInit();
  void writeComponentDefinitionCode();
  void writePortClassDefinitionCode();

  // generate implementation
  void writeLibraryHandle();
  void writeSourceInit();
  void writeSourceClassImpl();
  void writeSourceFileHeaderCode();
  void writeConstructorandDestructorCode();
  void writeSetServicesCode();
  void writeGoAndUIFunctionsCode();


  // frequently used string tokens
  const static std::string SP;
  const static std::string QT;
  const static std::string DIR_SEP;
  const static std::string OPEN_C_COMMENT;
  const static std::string CLOSE_C_COMMENT;
  const static std::string UNIX_SHELL_COMMENT;
  const static std::string NEWLINE;

  const static std::string DEFAULT_NAMESPACE;
  const static std::string DEFAULT_SIDL_NAMESPACE;
  const static std::string DEFAULT_PORT_NAMESPACE;
  const static std::string DEFAULT_SIDL_PORT_NAMESPACE;
  const std::string SERVICES_POINTER;
  const std::string TYPEMAP_POINTER;
  const std::string LICENCE;

  // Component name
  std::string compName;

  // List of Ports added
  std::vector<PortDescriptor*> providesPortsList;
  std::vector<PortDescriptor*> usesPortsList;

  // File Handles
  std::ofstream componentSourceFile;
  std::ofstream componentHeaderFile;
  std::ofstream componentMakefile;
};

}

#endif
