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
 *  ConnectionEventService.cc: Baseementation of CCA ConnectionEventService for SCIRun
 *
 *  Written by:
 *   Steven G. Parker
 *   Department of Computer Science
 *   University of Utah
 *   October 2001
 *
 */

#include <SCIRun/Core/CoreFramework.h>
#include <SCIRun/Core/ConnectionEventService.h>
#include <SCIRun/Core/ConnectionEventServiceBase.code>

#include <iostream>

namespace SCIRun {

  ConnectionEventService::ConnectionEventService(const CoreFramework::internalPointer &framework)
    : ConnectionEventServiceBase<CorePorts::ConnectionEventService>(framework)
  {}

  ConnectionEventService::~ConnectionEventService()
  {
    std::cerr << "EventService destroyed..." << std::endl;
  }

  ConnectionEventService::pointer ConnectionEventService::create(const CoreFramework::internalPointer &framework)
  {
    return pointer(new ConnectionEventService(framework));
  }

  void ConnectionEventService::emitConnectionEvent(const Ports::ConnectionEvent::pointer& event)
  {
    // iterate through listeners and call connectionActivity
    // should the event type to be emitted be ALL?
    if (event->getEventType() == Ports::ALL) {
      return;
    }

    {
      SCIRun::Guard lock(&listeners_lock);

      for (std::vector<Listener*>::iterator iter=listeners.begin();
	   iter != listeners.end(); iter++) {
        if ((*iter)->type == Ports::ALL ||
	    (*iter)->type == event->getEventType()) {
	  (*iter)->listener->connectionActivity(event);
        }
      }
    }
  }
  
} // end namespace SCIRun
