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


/*
 *  SCIRunLoader.cc: An instance of the SCIRun CCA Component Loader
 *
 *  Written by:
 *   Keming Zhang & Kosta
 *   Department of Computer Science
 *   University of Utah
 *   April 2003
 *
 */

#include <include/sci_defs/config_defs.h> // For MPIPP_H on SGI
#include <mpi.h>
#include <SCIRun/SCIRunLoader.h>
#include <SCIRun/CCA/CCAComponentModel.h>
#include <SCIRun/CCA/CCAComponentDescription.h>
#include <SCIRun/CCA/CCAComponentInstance.h>
#include <SCIRun/SCIRunFramework.h>
#include <SCIRun/SCIRunErrorHandler.h>
#include <Core/Containers/StringUtil.h>
#include <Core/OS/Dir.h>
#include <Dataflow/XMLUtil/StrX.h>
#include <Dataflow/XMLUtil/XMLUtil.h>
#include <Core/Util/soloader.h>
#include <Core/CCA/PIDL/PIDL.h>
#include <SCIRun/resourceReference.h>
#include <Core/CCA/spec/cca_sidl.h>
#ifdef __sgi
#define IRIX
#pragma set woff 1375
#endif
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/sax/SAXException.hpp>
#include <xercesc/sax/SAXParseException.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/dom/DOMNamedNodeMap.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#ifdef __sgi
#pragma reset woff 1375
#endif
#include <iostream>
#include <sstream>

namespace SCIRun {

SCIRunLoader::SCIRunLoader(const std::string &loaderName,
                           const std::string & frameworkURL)
{
  /*  Object::pointer obj=PIDL::objectFrom(frameworkURL);
      if(obj.isNull()){
      cerr<<"Cannot get framework from url="<<frameworkURL<<std::endl;
      return;
      }
      sci::cca::AbstractFramework::pointer
      framework=pidl_cast<sci::cca::AbstractFramework::pointer>(obj);
      
      SSIDL::array1< std::string> URLs;
      URLs.push_back(this->getURL().getString());
      framework->registerLoader(loaderName, URLs);
  */
}

int SCIRunLoader::createPInstance(const std::string& componentName,
                                  const std::string& componentType, 
                                  const sci::cca::TypeMap::pointer& properties,
                                  SSIDL::array1<std::string> &componentURLs)
{
  //TODO: assume type is always good?
  std::cerr<<"SCIRunLoader::getRefCount()="<<getRefCount()<<std::endl;
  
  std::string lastname=componentType.substr(componentType.find('.')+1);  
  std::string so_name("lib/libCCA_Components_");
  so_name=so_name+lastname+".so";
  std::cerr<<"componentType="<<componentType<<" soname="<<so_name<<std::endl;
    
  LIBRARY_HANDLE handle = GetLibraryHandle(so_name.c_str());
  if(!handle){
    std::cerr << "Cannot load component " << componentType << std::endl;
    std::cerr << SOError() << std::endl;
    return 1;
  }
  std::string makername = "make_"+componentType;
  for(int i=0;i<(int)makername.size();i++)
    if(makername[i] == '.')
      makername[i]='_';
  
  void* maker_v = GetHandleSymbolAddress(handle, makername.c_str());
  if(!maker_v){
    std::cerr <<"Cannot load component " << componentType << std::endl;
    std::cerr << SOError() << std::endl;
    return 1;
  }
  sci::cca::Component::pointer (*maker)() = (sci::cca::Component::pointer (*)())(maker_v);
  sci::cca::Component::pointer component = (*maker)();
  //TODO: need keep a reference in the loader's creatation recored.
  component->addReference();

  MPI_Comm_rank(MPI_COMM_WORLD,&mpi_rank);
  MPI_Comm_size(MPI_COMM_WORLD,&mpi_size);

  std::cerr << "SCIRunLoader::createInstance..., rank/size="<<mpi_rank<<"/"<<mpi_size<<"\n";
  componentURLs.resize(1);
  componentURLs[0] = component->getURL().getString();
  //componentURLs.push_back(component->getURL().getString());
  std::cerr << "Done, rank/size="<<mpi_rank<<"/"<<mpi_size<<" and URL="<<component->getURL().getString()<<"\n";
  return 0;
}



int SCIRunLoader::createInstance(const std::string& componentName, const std::string& componentType, 
				 const sci::cca::TypeMap::pointer& properties,std::string &componentURL) {

  //TODO: assume type is always good?

  std::cerr<<"SCIRunLoader::getRefCount()="<<getRefCount()<<std::endl;

  
  std::string lastname=componentType.substr(componentType.find('.')+1);  
  std::string so_name("lib/libCCA_Components_");
  so_name=so_name+lastname+".so";
  std::cerr<<"componentType="<<componentType<<" soname="<<so_name<<std::endl;
    
  LIBRARY_HANDLE handle = GetLibraryHandle(so_name.c_str());
  if(!handle){
    std::cerr << "Cannot load component " << componentType << std::endl;
    std::cerr << SOError() << std::endl;
    return 1;
  }
  std::string makername = "make_"+componentType;
  for(int i=0;i<(int)makername.size();i++)
    if(makername[i] == '.')
      makername[i]='_';
  
  void* maker_v = GetHandleSymbolAddress(handle, makername.c_str());
  if(!maker_v){
    std::cerr <<"Cannot load component " << componentType << std::endl;
    std::cerr << SOError() << std::endl;
    return 1;
  }
  sci::cca::Component::pointer (*maker)() = (sci::cca::Component::pointer (*)())(maker_v);
  sci::cca::Component::pointer component = (*maker)();
  //TODO: need keep a reference in the loader's creatation recored.
  component->addReference();

  MPI_Comm_rank(MPI_COMM_WORLD,&mpi_rank);
  MPI_Comm_size(MPI_COMM_WORLD,&mpi_size);

  //componentURLs.resize(1);
  //componentURLs[0] = component->getURL().getString();
  //componentURLs.push_back(component->getURL().getString());
  componentURL=component->getURL().getString();
  std::cerr << "SCIRunLoader::createInstance Done, rank/size="<<mpi_rank<<"/"<<mpi_size<<"\n";
  return 0;
}


int SCIRunLoader::destroyInstance(const std::string& componentName, float time)
{
  std::cerr<<"destroyInstance not implemneted\n";
  return 0;
}

int SCIRunLoader::getAllComponentTypes(::SSIDL::array1< ::std::string>& componentTypes)
{
  std::cerr<<"listAllComponents() is called\n";
  buildComponentList();
  for(componentDB_type::iterator iter=components.begin();
      iter != components.end(); iter++){
    componentTypes.push_back(iter->second->getType());
  }
  return 0;
}

int SCIRunLoader::shutdown(float timeout)
{
  std::cerr<<"shutdown() is called: should unregisterLoader()\n";
  return 0;
}


SCIRunLoader::~SCIRunLoader()
{
  std::cerr << "SCIRun  Loader exits.\n";
  //abort();
}

/*
std::string CCAComponentModel::createComponent(const std::string& name,
						      const std::string& type)
						     
{
  
  sci::cca::Component::pointer component;
  componentDB_type::iterator iter = components.find(type);
  if(iter == components.end())
    return "";
    //ComponentDescription* cd = iter->second;
  
  std::string lastname=type.substr(type.find('.')+1);  
  std::string so_name("lib/libCCA_Components_");
  so_name=so_name+lastname+".so";
  //std::cerr<<"type="<<type<<" soname="<<so_name<<std::endl;
  
  LIBRARY_HANDLE handle = GetLibraryHandle(so_name.c_str());
  if(!handle){
    std::cerr << "Cannot load component " << type << std::endl;
    std::cerr << SOError() << std::endl;
    return "";
  }

  std::string makername = "make_"+type;
  for(int i=0;i<(int)makername.size();i++)
    if(makername[i] == '.')
      makername[i]='_';
  
  void* maker_v = GetHandleSymbolAddress(handle, makername.c_str());
  if(!maker_v){
    std::cerr << "Cannot load component " << type << std::endl;
    std::cerr << SOError() << std::endl;
    return "";
  }
  sci::cca::Component::pointer (*maker)() = (sci::cca::Component::pointer (*)())(maker_v);
  component = (*maker)();
  //need to make sure addReference() will not cause problem
  component->addReference();
  return component->getURL().getString();
}
*/


void SCIRunLoader::buildComponentList()
{
  // Initialize the XML4C system
  try {
    XMLPlatformUtils::Initialize();
  }catch (const XMLException& toCatch) {
    std::cerr << "Error during initialization! :\n"
	      << StrX(toCatch.getMessage()) << std::endl;
    return;
  }

  destroyComponentList();
  std::string component_path ="../src/CCA/Components/xml";
// "../src/CCA/Components/xml";
  while(component_path != ""){
    unsigned int firstColon = component_path.find(':');
    std::string dir;
    if(firstColon < component_path.size()){
      dir=component_path.substr(0, firstColon);
      component_path = component_path.substr(firstColon+1);
    } else {
      dir = component_path;
      component_path="";
    }
    Dir d(dir);
    std::cerr << "Looking at directory: " << dir << std::endl;
    std::vector<std::string> files;
    d.getFilenamesBySuffix(".cca", files);
    for(std::vector<std::string>::iterator iter = files.begin();
	iter != files.end(); iter++){
      std::string& file = *iter;
      readComponentDescription(dir+"/"+file);
    }
  }
}



void SCIRunLoader::readComponentDescription(const std::string& file)
{
  // Instantiate the DOM parser.
  XercesDOMParser parser;
  parser.setDoValidation(false);
  
  SCIRunErrorHandler handler;
  parser.setErrorHandler(&handler);
  
  try {
    parser.parse(file.c_str());
  }  catch (const XMLException& toCatch) {
    std::cerr << "Error during parsing: '" <<
      file << "'\nException message is:  " <<
      xmlto_string(toCatch.getMessage()) << std::endl;
    handler.foundError=true;
    return;
  }
  
  DOMDocument* doc = parser.getDocument();
  DOMNodeList* list = doc->getElementsByTagName(to_xml_ch_ptr("component"));
  int nlist = list->getLength();
  if(nlist == 0){
    std::cerr << "WARNING: file " << file << " has no components!\n";
  }
  for (int i=0;i<nlist;i++){
    DOMNode* d = list->item(i);
    //should use correct Loader pointer below.
    CCAComponentDescription* cd = new CCAComponentDescription(0); //TODO: assign null as CCA component model
    DOMNode* name = d->getAttributes()->getNamedItem(to_xml_ch_ptr("name"));
    if (name==0) {
      std::cout << "ERROR: Component has no name." << std::endl;
      cd->type = "unknown type";
    } else {
      cd->type = to_char_ptr(name->getNodeValue());
    }
  
    componentDB_type::iterator iter = components.find(cd->type);
    if(iter != components.end()){
      std::cerr << "WARNING: Component multiply defined: " << cd->type << std::endl;
    } else {
      std::cerr << "Added CCA component of type: " << cd->type << std::endl;
      components[cd->type]=cd;
    }
  }
}

void SCIRunLoader::destroyComponentList()
{
  for(componentDB_type::iterator iter=components.begin();
      iter != components.end(); iter++){
    delete iter->second;
  }
  components.clear();
}

} // end namespace SCIRun
