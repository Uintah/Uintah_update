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
 *  ComponentEventImpl.h: Implementation of the SCI CCA Extension
 *                    ComponentEvent interface for SCIRun
 *
 *  Written by:
 *   Ayla Khan
 *   Scientific Computing and Imaging Institute
 *   University of Utah
 *   October 2004
 *
 *  Copyright (C) 2004 SCI Institute
 *
 */

#ifndef SCIRun_ComponentEventImpl_h
#define SCIRun_ComponentEventImpl_h

#include <Core/CCA/spec/sci_sidl.h>

namespace SCIRun {

  namespace Distributed = sci::cca::distributed;

  template<class Base>
  class ComponentEventImpl : public Base 
  {
  public:
    ComponentEventImpl(sci::cca::ports::ComponentEventType type,
		       const sci::cca::ComponentID::pointer& id,
		       const sci::cca::TypeMap::pointer& properties);
    virtual ~ComponentEventImpl();
    
    /** ? */
    virtual sci::cca::ports::ComponentEventType getEventType();

    /** ? */
    virtual sci::cca::ComponentID::pointer getComponentID();
    
    /** ? */
    virtual sci::cca::TypeMap::pointer getComponentProperties();

  private:
    sci::cca::ports::ComponentEventType type;
    sci::cca::ComponentID::pointer id;
    sci::cca::TypeMap::pointer properties;
  };
  
} // namespace SCIRun

#include <SCIRun/Distributed/ComponentEventImpl.code>

#endif
