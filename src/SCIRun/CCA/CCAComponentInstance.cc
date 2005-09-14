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
 *  CCAComponentInstance.cc:
 *
 *  Written by:
 *   Steven G. Parker
 *   Department of Computer Science
 *   University of Utah
 *   October 2001
 *
 */

#include <SCIRun/TypeMap.h>
#include <SCIRun/SCIRunFramework.h>
#include <SCIRun/CCA/CCAComponentInstance.h>
#include <SCIRun/CCA/CCAPortInstance.h>
#include <SCIRun/CCA/CCAException.h>
#include <Core/Thread/Mutex.h>
#include <iostream>

#include <Core/Util/NotFinished.h>

namespace SCIRun {

  CCAComponentInstance::CCAComponentInstance(sci::cca::SCIRunFramework::pointer framework,
					     const std::string &instanceName,
					     const std::string &className,
					     const sci::cca::TypeMap::pointer &properties,
					     const sci::cca::Component::pointer &component)
    : lock_ports("CCAComponentInstance::ports lock"),
      lock_preports("CCAComponentInstance::preports lock"),
      component(component), size(0), rank(0),
      className(className), properties(properties), framework(framework), instanceName(instanceName)
  {
    mutex = new Mutex("getPort mutex");
    
  }
  
  CCAComponentInstance::~CCAComponentInstance()
  {
    delete mutex;
  }
  
  /*
   * internal::cca::CCAComponentInstance interface
   */


  PortInstanceIterator::pointer CCAComponentInstance::getPorts()
  {
    return PortInstanceIterator::pointer(new Iterator(this));
  }


  /*
   * internal::cca::ComponentInstance interface
   */
  sci::cca::SCIRunFramework::pointer CCAComponentInstance::getFramework() 
  { return framework; }

  std::string  CCAComponentInstance::getClassName() 
  { return className; }

  sci::cca::TypeMap::pointer  CCAComponentInstance::getProperties() 
  { return properties; }

  void  CCAComponentInstance::setProperties(const sci::cca::TypeMap::pointer &properties) 
  { this->properties = properties; }

  /*
   * cca.ComponentID interface
   */

  std::string  CCAComponentInstance::getInstanceName() 
  { return instanceName; }

  std::string CCAComponentInstance::getSerialization()
  {
    return framework->getURL().getString()+"/"+instanceName;
  }

  /*
   * Services interface
   */
  
  sci::cca::Port::pointer CCAComponentInstance::getPort(const std::string& name)
  {
    mutex->lock();
    sci::cca::Port::pointer port=getPortNonblocking(name);
    mutex->unlock();
    return port;
  }
  
  sci::cca::Port::pointer
  CCAComponentInstance::getPortNonblocking(const std::string& name)
  {
    sci::cca::Port::pointer svc =
      framework->getFrameworkService(name, instanceName);
    if (!svc.isNull()) {
      return svc;
    }
    lock_ports.lock();
    std::map<std::string, CCAPortInstance::pointer>::iterator iter = ports.find(name);
    lock_ports.unlock(); 
    if (iter == ports.end()) {
      return sci::cca::Port::pointer(0);
    }
    CCAPortInstance::pointer pr = iter->second;
    if (pr->portUsage() != sci::cca::internal::cca::Uses) {
      throw sci::cca::CCAException::pointer(new CCAException("Cannot call getPort on a Provides port", sci::cca::BadPortType));
    }
    if (pr->numOfConnections() != 1) {
      return sci::cca::Port::pointer(0);
    }
    pr->incrementUseCount();
    CCAPortInstance::pointer pi=pidl_cast<CCAPortInstance::pointer>(pr->getPeer());
    return pi->getPort();
  }
  
  void CCAComponentInstance::releasePort(const std::string& name)
  {
    if (framework->releaseFrameworkService(name, instanceName)) {
      return;
    }
    
    lock_ports.lock();
    std::map<std::string, CCAPortInstance::pointer>::iterator iter = ports.find(name);
    lock_ports.unlock();
    if (iter == ports.end()) {
      throw sci::cca::CCAException::pointer(new CCAException("Released an unknown port: " + name, sci::cca::BadPortName));
    }
    
    CCAPortInstance::pointer pr = iter->second;
    if (pr->portUsage() == sci::cca::internal::cca::Provides) {
      throw sci::cca::CCAException::pointer(new CCAException("Cannot call releasePort on a Provides port", sci::cca::BadPortType));
    }
    if (!pr->decrementUseCount()) {
      // negative use count
      throw sci::cca::CCAException::pointer(new CCAException("Port released without correspond get"));
    }
  }
  
  sci::cca::TypeMap::pointer CCAComponentInstance::createTypeMap()
  {
    sci::cca::TypeMap::pointer tm(new TypeMap);
    // It is not clear why we need addReference here.
    // But removing it can cause random crash
    // when creating remote parallel components
    // TODO: possible memory leak?
    tm->addReference();
    return tm; 
  }
  

  void CCAComponentInstance::registerUsesPort(const std::string& portName,
					      const std::string& portType,
					      const sci::cca::TypeMap::pointer& properties)
  {
    SCIRun::Guard g1(&lock_ports);
    std::map<std::string, CCAPortInstance::pointer>::iterator iter = ports.find(portName);
    if (iter != ports.end()) {
      if (iter->second->portUsage() == sci::cca::internal::cca::Provides) {
	throw sci::cca::CCAException::pointer(new CCAException("name conflict between uses and provides ports for " + portName, sci::cca::BadPortName));
      } else {
	throw sci::cca::CCAException::pointer(new CCAException("registerUsesPort called twice for " + portName + " " + portType + " " + instanceName, sci::cca::PortAlreadyDefined));
      }
    }
    ports.insert(make_pair(portName, 
			   CCAPortInstance::pointer(new CCAPortInstance(portName, 
									portType, 
									properties, 
									sci::cca::internal::cca::Uses))));
  }

  void CCAComponentInstance::unregisterUsesPort(const std::string& portName)
  {
    SCIRun::Guard g1(&lock_ports);
    std::map<std::string, CCAPortInstance::pointer>::iterator iter = ports.find(portName);
    if (iter != ports.end()) {
      if (iter->second->portUsage() == sci::cca::internal::cca::Provides) {
	throw sci::cca::CCAException::pointer(new CCAException("name conflict between uses and provides ports for " + portName, sci::cca::BadPortName));
      } else {
	ports.erase(portName);
      }
    } else {
      throw sci::cca::CCAException::pointer(new CCAException("port name not found for " + portName, sci::cca::BadPortName));
    }
  }

  void CCAComponentInstance::addProvidesPort(const sci::cca::Port::pointer& port,
					     const std::string& portName,
					     const std::string& portType,
					     const sci::cca::TypeMap::pointer& properties)
  {
    lock_ports.lock();
    std::map<std::string, CCAPortInstance::pointer>::iterator iter = ports.find(portName);
    lock_ports.unlock();
    if (iter != ports.end()) {
      if (iter->second->portUsage() == sci::cca::internal::cca::Uses) {
        throw sci::cca::CCAException::pointer(new CCAException("name conflict between uses and provides ports for " + portName));
      } else {
        throw sci::cca::CCAException::pointer(new CCAException("addProvidesPort called twice for " + portName));
      }
    }
    if (!properties.isNull() && properties->getInt("size", 1) > 1) {
      //if port is collective.
      size = properties->getInt("size", 1);
      rank = properties->getInt("rank", 0);
    
      mutex->lock();
   
      lock_preports.lock(); 
      std::map<std::string, std::vector<Object::pointer> >::iterator iter = preports.find(portName);
      lock_preports.unlock();
      if (iter==preports.end()){
	if (port.isNull()) std::cerr<<"port is NULL\n";
      
	//new preport
	std::vector<Object::pointer> urls(size);
	lock_preports.lock();
	preports[portName]=urls;
	//      preports[portName][rank]=port->getURL();
	preports[portName][rank]=port;
	lock_preports.unlock();
	precnt[portName]=0;
	precond[portName]=new ConditionVariable("precond");
      }
      else {
	//existed preport  
	iter->second[rank]=port;
      }
      if (++precnt[portName]==size){
	//all member ports have arrived.
	Object::pointer obj=PIDL::objectFrom(preports[portName],1,0);
	sci::cca::Port::pointer cport=pidl_cast<sci::cca::Port::pointer>(obj);
	lock_ports.lock();
	ports.insert(make_pair(portName, 
			       CCAPortInstance::pointer(new CCAPortInstance(portName, 
									    portType,
									    properties, 
									    cport, 
									    sci::cca::internal::cca::Provides))));
	lock_ports.unlock();
	lock_preports.lock();
	preports.erase(portName);
	lock_preports.unlock();
	precond[portName]->conditionBroadcast();
	precnt[portName]--;
      } else {
	precond[portName]->wait(*mutex);
	if (--precnt[portName]==0){
	  precnt.erase(portName);
	  delete precond[portName];
	  precond.erase(portName);
	}
      }
      mutex->unlock();
      return;
    } else {
      lock_ports.lock();
      ports.insert(make_pair(portName,
			     CCAPortInstance::pointer(new CCAPortInstance(portName, 
									  portType, 
									  properties,
									  port, 
									  sci::cca::internal::cca::Provides))));
      lock_ports.unlock();
    }
  }

  // should throw CCAException of type 'PortNotDefined'
  void CCAComponentInstance::removeProvidesPort(const std::string& name)
  {
    if (size < 1) {
      std::cerr << "CCAComponentInstance::removeProvidesPort: name="
		<< name << std::endl;
      lock_ports.lock();
      std::map<std::string, CCAPortInstance::pointer>::iterator iter = ports.find(name);
      lock_ports.unlock();
      if (iter == ports.end()) { // port can't be found
	throw sci::cca::CCAException::pointer(new CCAException("Port " + name + " is not defined.", sci::cca::BadPortName));
      }

      // check if port is in use???
      //delete iter->second;
      iter->second = CCAPortInstance::pointer(0); // remove reference to it.
      lock_ports.lock();
      ports.erase(iter);
      lock_ports.unlock();
    } else { // don't handle parallel ports for now
      std::cerr << "CCAComponentInstance::removeProvidesPort is not implemented: name="
		<< name << std::endl;
    }
  }

  sci::cca::TypeMap::pointer
  CCAComponentInstance::getPortProperties(const std::string& portName)
  { 
    NOT_FINISHED("sci::cca::TypeMap::pointer CCAComponentInstance::getPortProperties(const std::string& portName)");
    return sci::cca::TypeMap::pointer(0);
  }

  sci::cca::ComponentID::pointer
  CCAComponentInstance::getComponentID()
  {
    return pointer(this);
#if 0
    sci::cca::ComponentID::pointer cid =
      framework->lookupComponentID(instanceName);
    if (! cid.isNull()) {
      return cid;
    }
    cid = sci::cca::ComponentID::pointer(
					 new ComponentID(framework, instanceName));
    //framework->compIDs.push_back(cid);
    return cid;
#endif
  }

  // TODO: implement registerForRelease
  void
  CCAComponentInstance::registerForRelease(const sci::cca::ComponentRelease::pointer & /*compRel*/)
  {
    NOT_FINISHED("void .sci.cca.Services.registerForRelease(in .sci.cca.ComponentRelease callBack)throws .sci.cca.CCAException");
  }


  sci::cca::internal::PortInstance::pointer
  CCAComponentInstance::getPortInstance(const std::string& portname)
  {
    SCIRun::Guard g1(&lock_ports);
    std::map<std::string, CCAPortInstance::pointer>::iterator iter = ports.find(portname);
    if (iter == ports.end())
      return sci::cca::internal::PortInstance::pointer(0);
    return iter->second;
  }

  /*
   * private functions
   */
  
  CCAComponentInstance::Iterator::Iterator(const CCAComponentInstance *comp)
    :/*iter(comp->ports.begin(),*/ comp(comp)
  {
    iter = comp->ports.begin();
  }
  
  CCAComponentInstance::Iterator::~Iterator()
  {
  }
  
  sci::cca::internal::PortInstance::pointer CCAComponentInstance::Iterator::get()
  {
    return iter->second;
  }
  
  bool CCAComponentInstance::Iterator::done()
  {
    return iter == comp->ports.end();
  }
  
  void CCAComponentInstance::Iterator::next()
  {
    ++iter;
  }
  
} // end namespace SCIRun
