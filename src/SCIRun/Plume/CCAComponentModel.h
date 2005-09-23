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
 *  CCAComponentModel.h: 
 *
 *  Written by:
 *   Steven G. Parker
 *   Department of Computer Science
 *   University of Utah
 *   October 2001
 *
 */

#ifndef SCIRun_CCA_CCAComponentModel_h
#define SCIRun_CCA_CCAComponentModel_h

#include <Core/Thread/Mutex.h>
#include <SCIRun/Distributed/ComponentInfo.h>
#include <Core/CCA/spec/sci_sidl.h>
#include <vector>
#include <string>
#include <map>

namespace SCIRun {

  namespace Distributed = sci::cca::distributed;

  class DitributedFramework;
  class ComponentDescription;
  class ComponentInfo;
  
  /**
   * \class CCAComponentModel
   *
   */
  class CCAComponentModel 
  {
  public:
    CCAComponentModel(DistributedFramework *framework);
    virtual ~CCAComponentModel();
    

    virtual ComponentInfo *createComponnet(const std::string& name,
					   const std::string& type,
					   const sci::cca::TypeMap::pointer& properties);
    
    /** Deallocates the component instance \em ci.  Returns \code true on success and
	\code false on failure. */
    virtual void destroyComponent(const Distributed::ComponentInfo::pointer &info);
    
    /** Get/set the directory path to component DLLs.  By default,
     * the sidlDLLPath is initialized to the environment variable
     * SIDL_DLL_PATH. */
    std::string getSidlDLLPath() const { return sidlDLLPath; }
    void setSidlDLLPath( const std::string& s) { sidlDLLPath = s; }
    
    static const std::string DEFAULT_PATH;
    
private:
    DistributedFramework *framework;
    typedef std::map<std::string, CCAComponentDescription*> componentDB_type;
    componentDB_type components;
    SCIRun::Mutex components_lock;   
   
    std::string sidlDLLPath;

    void readComponentDescription(const std::string& file);

    CCAComponentModel(const CCAComponentModel&);
    CCAComponentModel& operator=(const CCAComponentModel&);
  };

} // end namespace SCIRun

#endif
