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



#include <Core/CCA/PIDL/NexusEpChannel.h>
#include <Core/CCA/PIDL/NexusHandlerThread.h>
#include <Core/CCA/PIDL/PIDL.h>
#include <Core/CCA/PIDL/Warehouse.h>
#include <Core/CCA/PIDL/ServerContext.h>
#include <Core/CCA/Exceptions/CommError.h>
#include <Core/CCA/PIDL/NexusEpMessage.h>
#include <Core/CCA/PIDL/NexusSpChannel.h>

#include <Core/Util/DebugStream.h>
#include <Core/Thread/Thread.h>

#include <iostream>
#include <sstream>
#include <globus_nexus.h>

using namespace SCIRun;
using namespace std;

/////////////
// Hostname of this computer
static char* hostname;

/////////////
// Port to listen to. Nexus assigns this.
static unsigned short port;

static DebugStream dbg( "NexusEpChannel", false );

static globus_nexus_handler_t emptytable[] = {{GLOBUS_NEXUS_HANDLER_TYPE_THREADED,0}};


void unknown_handler(globus_nexus_endpoint_t* endpoint,
		     globus_nexus_buffer_t* buffer,
		     int handler_id)
{
  void* v=globus_nexus_endpoint_get_user_pointer(endpoint);
  ServerContext* sc=static_cast< ServerContext*>(v); 
  NexusEpChannel *chan = dynamic_cast<NexusEpChannel*>(sc->chan);
  if (chan) {
    if (handler_id >= chan->table_size)
      throw CommError("Handler function does not exist", __FILE__, __LINE__, 1101);
    if(int _gerr=globus_nexus_buffer_save(buffer))
      throw CommError("buffer_save", __FILE__, __LINE__, _gerr);
    chan->msgbuffer = *buffer;
    Thread *t = new Thread(new NexusHandlerThread(chan, handler_id), "HANDLER_THREAD");
    t->detach(); 
  }
}


static int approval_fn(void*, char* urlstring, globus_nexus_startpoint_t* sp)
{
  try {
    Object* obj;
    URL url(urlstring);
    Warehouse* wh=PIDL::getWarehouse();
    obj = wh->lookupObject(url.getSpec());
    if(!obj){
      std::cerr << "Unable to find object: " << urlstring
		<< ", rejecting client (code=1002)\n";
      return 1002;
    }
    if(!obj->d_serverContext){
      std::cerr << "Object is corrupt: " << urlstring
		<< ", rejecting client (code=1003)\n";
      return 1003;
    }
    NexusEpChannel * chan = dynamic_cast<NexusEpChannel*>(obj->d_serverContext->chan);
    if (chan)
      return chan->approve(sp, obj);
    else
      throw CommError("You aren't using consistent communication libraries", __FILE__, __LINE__, 1007);
  } catch(const SCIRun::Exception& e) {
    std::cerr << "Caught exception (" << e.message() << "): " << urlstring
              << ", rejecting client (code=1005)\n";
    return 1005;
  } catch(...) {
    std::cerr << "Caught unknown exception: " << urlstring
              << ", rejecting client (code=1006)\n";
    return 1006;
  }
}


int NexusEpChannel::approve(globus_nexus_startpoint_t* sp, Object* obj)
{ 
  dbg << "NexusEpChannel::approve()\n";

  if(int gerr=globus_nexus_startpoint_bind(sp, &d_endpoint)){
    std::cerr << "Failed to bind startpoint: " 
	      << ", rejecting client (code=1004)\n";
    std::cerr << "Globus error code: " << gerr << '\n';
    return 1004;
  }

  /* Increment the reference count for this object. */
  obj->addReference();
  return GLOBUS_SUCCESS;
}

NexusEpChannel::NexusEpChannel() { 
  dbg << "NexusEpChannel::NexusEpChannel()\n";
}

NexusEpChannel::~NexusEpChannel() {
}

void NexusEpChannel::openConnection() {
  static bool once = false;
  if(!once){
    once=true;
    dbg << "NexusEpChannel::openConnection()\n";
  
    if(int gerr=globus_module_activate(GLOBUS_NEXUS_MODULE))
      throw CommError("Unable to initialize nexus", __FILE__, __LINE__, gerr);
    if(int gerr=globus_nexus_allow_attach(&port, &hostname, approval_fn, 0))
      throw CommError("globus_nexus_allow_attach failed", __FILE__, __LINE__, gerr);
    globus_nexus_enable_fault_tolerance(NULL, 0);
  }
}

void NexusEpChannel::closeConnection() {
  dbg << "NexusEpChannel::closeConnection()\n";

  if(int gerr=globus_nexus_endpoint_destroy(&d_endpoint))
    throw CommError("endpoint_destroy", __FILE__, __LINE__, gerr);
}

string NexusEpChannel::getUrl() {

  std::ostringstream o;
  if (hostname == NULL) return "";
  o << "x-nexus://" << hostname << ":" << port << "/";
  return o.str();
}

Message* NexusEpChannel::getMessage() {
  dbg << "NexusEpChannel::getMessage()\n";
  return (new NexusEpMessage(d_endpoint,msgbuffer));
}


void NexusEpChannel::activateConnection(void * obj) {
  dbg << "NexusEpChannel::activateConnection()\n";

  globus_nexus_endpointattr_t attr;
  if(int gerr=globus_nexus_endpointattr_init(&attr))
    throw CommError("endpointattr_init", __FILE__, __LINE__, gerr);
  if(int gerr=globus_nexus_endpointattr_set_handler_table(&attr,
							  emptytable, 0))
    throw CommError("endpointattr_set_handler_table", __FILE__, __LINE__, gerr);
  if(int gerr=globus_nexus_endpointattr_set_unknown_handler(&attr,
							    unknown_handler,
							    GLOBUS_NEXUS_HANDLER_TYPE_NON_THREADED))
    throw CommError("endpointattr_set_unknown_handler", __FILE__, __LINE__, gerr);
  if(int gerr=globus_nexus_endpoint_init(&d_endpoint, &attr))
    throw CommError("endpoint_init", __FILE__, __LINE__, gerr);
  globus_nexus_endpoint_set_user_pointer(&d_endpoint, obj);
  if(int gerr=globus_nexus_endpointattr_destroy(&attr))
    throw CommError("endpointattr_destroy", __FILE__, __LINE__, gerr);

}

void NexusEpChannel::allocateHandlerTable(int size) {
  dbg << "NexusEpChannel::allocateHandlerTable()\n";
  handler_table = new HPF[size];
  table_size = size;
}

void NexusEpChannel::registerHandler(int num, void* handle){
  handler_table[num-1] = (HPF) handle;
}

void NexusEpChannel::bind(SpChannel* spchan) {
  NexusSpChannel* nspchan = dynamic_cast<NexusSpChannel*>(spchan);
  if (nspchan == NULL) 
    throw CommError("Communication Library discrepancy detected\n", __FILE__, __LINE__, 1000);
  if(int gerr=globus_nexus_startpoint_bind(&(nspchan->d_sp), &d_endpoint))
    throw CommError("startpoint_bind", __FILE__, __LINE__, gerr);    
}
