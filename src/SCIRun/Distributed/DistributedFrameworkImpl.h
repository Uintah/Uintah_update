/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2005 Scientific Computing and Imaging Institute,
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
 *  DistributedFrameworkImpl.h: 
 *
 *  Written by:
 *   Yarden Livnat
 *   SCI Institute
 *   University of Utah
 *   August 2005
 *
 */

#ifndef SCIRun_Framework_DistributedFrameworkImpl_h
#define SCIRun_Framework_DistributedFrameworkImpl_h


#include <Core/CCA/spec/sci_sidl.h>
#include <string>


namespace SCIRun {

  /**
   * \class DistributedFrameworkImpl
   * 
   * \brief An implementation of a DistributedFrameworkImpl 
   */
  
  namespace Distributed = sci::cca::distributed;

  template<class Base>
  class DistributedFrameworkImpl : public Base
  {
  public:
    typedef Distributed::DistributedFramework::pointer pointer;

    DistributedFrameworkImpl( pointer parent = 0);
    virtual ~DistributedFrameworkImpl();
    
    virtual Distributed::FrameworkID::pointer getFrameworkID();
    virtual bool isRoot() { return !parent.isNull(); }
    
    virtual pointer getParent() { return parent; }
    virtual SSIDL::array1<pointer> getChildren() { return public_children; }
    
  private:
    pointer parent;
    SSIDL::array1<pointer> public_children;
    SSIDL::array1<pointer> private_children;
  };
  
} // end namespace SCIRun

//#include <SCIRun/Distributed/DistributedFrameworkImpl.code>

#endif
