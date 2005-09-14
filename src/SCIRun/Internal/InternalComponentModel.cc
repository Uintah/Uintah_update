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
 *  InternalComponentModel.cc:
 *
 *  Written by:
 *   Steven G. Parker
 *   Department of Computer Science
 *   University of Utah
 *   October 2001
 *
 */

#include <SCIRun/Internal/FrameworkInternalException.h>
#include <SCIRun/Internal/InternalComponentModel.h>
#include <SCIRun/Internal/InternalFrameworkServiceDescription.h>
#include <SCIRun/Internal/BuilderService.h>
#include <SCIRun/Internal/ComponentEventService.h>
#include <SCIRun/Internal/ConnectionEventService.h>
#include <SCIRun/Internal/ComponentRegistry.h>
#include <SCIRun/Internal/FrameworkProperties.h>
#include <SCIRun/Internal/FrameworkProxyService.h>
#include <SCIRun/SCIRunFramework.h>

#ifdef BUILD_DATAFLOW
 #include <SCIRun/Dataflow/DataflowScheduler.h>
#endif
#include <iostream>

#ifndef DEBUG
 #define DEBUG 0
#endif

namespace SCIRun {

InternalComponentModel::InternalComponentModel(sci::cca::SCIRunFramework::pointer framework)
  : ComponentModel("internal"), framework(framework),
    lock_frameworkServices("InternalComponentModel::frameworkServices lock")
{
    addService(new InternalFrameworkServiceDescription(this, "cca.BuilderService", &BuilderService::create));
    addService(new InternalFrameworkServiceDescription(this, "cca.ComponentRepository", &ComponentRegistry::create));
    addService(new InternalFrameworkServiceDescription(this, "cca.ComponentEventService", &ComponentEventService::create));
    addService(new InternalFrameworkServiceDescription(this, "cca.ConnectionEventService", &ConnectionEventService::create));
#ifdef BUILD_DATAFLOW
    addService(new InternalFrameworkServiceDescription(this, "cca.DataflowScheduler", &DataflowScheduler::create));
#endif
    addService(new InternalFrameworkServiceDescription(this, "cca.FrameworkProperties", &FrameworkProperties::create));
    addService(new InternalFrameworkServiceDescription(this, "cca.FrameworkProxyService", &FrameworkProxyService::create));
}

InternalComponentModel::~InternalComponentModel()
{
  SCIRun::Guard g1(&lock_frameworkServices);
  for (FrameworkServicesMap::iterator iter=frameworkServices.begin(); iter != frameworkServices.end(); iter++) {
    delete iter->second;
  }
}

void InternalComponentModel::addService(InternalFrameworkServiceDescription* desc)
{
  SCIRun::Guard g1(&lock_frameworkServices);
  if (frameworkServices.find(desc->getType()) != frameworkServices.end()) 
    throw FrameworkInternalException("add duplicate service ["+desc->getType()+"]");
  frameworkServices[desc->getType()] = desc;
}

sci::cca::Port::pointer
InternalComponentModel::getFrameworkService(const std::string& type,
					    const std::string& componentName)
{
  InternalFrameworkServiceInstance::pointer service;
  sci::cca::Port::pointer port(0);

  lock_frameworkServices.lock();

    FrameworkServicesMap::const_iterator fwkServiceDesc = frameworkServices.find(type);
    
  lock_frameworkServices.unlock();

  if ( fwkServiceDesc != frameworkServices.end() )
    service = fwkServiceDesc->second->get(framework);

  if ( !service.isNull() ) {
    service->incrementUseCount();
    port = service->getService(type);
    port->addReference();
  }

  return port;
}

bool
InternalComponentModel::releaseFrameworkService(const std::string& type,
                                                const std::string& componentName)
{
  lock_frameworkServices.lock();
  FrameworkServicesMap::iterator iter = frameworkServices.find(type);
  lock_frameworkServices.unlock();
  if (iter == frameworkServices.end()) { 
    return false; 
  }
  
  iter->second->release(framework);

  return true;
}

bool InternalComponentModel::haveComponent(const std::string& /*name*/)
{
#if DEBUG
  std::cerr << "Error: InternalComponentModel does not implement haveComponent"
        << std::endl;
#endif
    return false;
}

void InternalComponentModel::destroyComponentList()
{
#if DEBUG
  std::cerr << "Error: InternalComponentModel does not implement destroyComponentList"
            << std::endl;
#endif
}

void InternalComponentModel::buildComponentList()
{
#if DEBUG
  std::cerr << "Error: InternalComponentModel does not implement buildComponentList"
            << std::endl;
#endif
}

ComponentInstance::pointer InternalComponentModel::createInstance(const std::string&,
                                                          const std::string&)
{
    return ComponentInstance::pointer(0);
}

bool InternalComponentModel::destroyInstance(const ComponentInstance::pointer &ic)
{
#if DEBUG
    std::cerr << "Warning: I don't know how to destroy a internal component instance!" << std::endl;
#endif
    return true;
}

std::string InternalComponentModel::getName() const
{
    return "Internal";
}

void
InternalComponentModel::listAllComponentTypes(
   std::vector<ComponentDescription*>& list, bool listInternal)
{
  SCIRun::Guard g1(&lock_frameworkServices);
  if (listInternal) {
    for (FrameworkServicesMap::iterator iter = frameworkServices.begin(); iter != frameworkServices.end(); iter++) {
      list.push_back(iter->second);
    }
  }
}

} // end namespace SCIRun
