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
 *  SCIRunComponentModel.cc:
 *
 *  Written by:
 *   Steven G. Parker
 *   Department of Computer Science
 *   University of Utah
 *   October 2001
 *
 */

#include <SCIRun/Dataflow/SCIRunComponentModel.h>
#include <Core/Containers/StringUtil.h>
#include <Core/OS/Dir.h>
#include <Core/Util/soloader.h>
#include <Dataflow/Network/Module.h>
#include <Dataflow/Network/Network.h>
#include <Dataflow/Network/NetworkEditor.h>
#include <Dataflow/Network/PackageDB.h>
#include <Dataflow/Network/Scheduler.h>
#include <Core/GuiInterface/TCLTask.h>
#include <Core/GuiInterface/TCLInterface.h>
#include <Dataflow/XMLUtil/StrX.h>
#include <Dataflow/XMLUtil/XMLUtil.h>
#include <SCIRun/Dataflow/SCIRunComponentDescription.h>
#include <SCIRun/Dataflow/SCIRunComponentInstance.h>
#include <SCIRun/SCIRunErrorHandler.h>
#include <iostream>
#include <sci_defs.h>
using namespace std;
using namespace SCIRun;


GuiInterface*
SCIRunComponentModel::gui(0);

Network*
SCIRunComponentModel::net(0);

static bool split_name(const string& type, string& package,
		       string& category, string& module)
{
  unsigned int dot = type.find('.');
  if(dot >= type.size())
    return false;
  package = type.substr(0, dot);
  string rest = type.substr(dot+1);
  dot = rest.find('.');
  if(dot >= rest.size())
    return false;
  category = rest.substr(0, dot);
  module = rest.substr(dot+1);
  if(module.size()<1)
    return false;
  return true;
}

SCIRunComponentModel::SCIRunComponentModel(SCIRunFramework* framework)
  : ComponentModel("scirun"), framework(framework)
{
  packageDB = new PackageDB(0);
  // load the packages
  packageDB->loadPackage(false);
}

SCIRunComponentModel::~SCIRunComponentModel()
{
}

bool SCIRunComponentModel::haveComponent(const std::string& type)
{
  string package, category, module;
  if(!split_name(type, package, category, module))
    return false;
  return packageDB->haveModule(package, category, module);
}

ComponentInstance*
SCIRunComponentModel::createInstance(const std::string& name,
				     const std::string& type)
{
  string package, category, module;
  if(!split_name(type, package, category, module))
    return 0;
  if(!gui) {
    initGuiInterface();
  }

  Module* m = net->add_module2(package, category, module);
  SCIRunComponentInstance* ci = new SCIRunComponentInstance(framework, name,
							    type, m);
  return ci;
}

void SCIRunComponentModel::initGuiInterface() {
  int argc=1;
  char* argv[2];
  argv[0]="sr";
  argv[1]=0;
  TCLTask* tcl_task = new TCLTask(argc, argv);
  Thread* t=new Thread(tcl_task, "TCL main event loop");
  t->detach();
  tcl_task->mainloop_waitstart();
  
  // Create user interface link
  gui = new TCLInterface();
  
  // Set up the TCL environment to find core components
  const string DataflowTCLpath = SCIRUN_SRCDIR+string("/Dataflow/GUI");
  const string CoreTCLpath = SCIRUN_SRCDIR+string("/Core/GUI");
  gui->execute("global CoreTCL SCIRUN_SRCDIR SCIRUN_OBJDIR scirun2");
  gui->execute("set CoreTCL "+CoreTCLpath);
  gui->execute("set SCIRUN_SRCDIR "SCIRUN_SRCDIR);
  gui->execute("set SCIRUN_OBJDIR "SCIRUN_OBJDIR);
  gui->execute("set scirun2 1");
  gui->execute("lappend auto_path "+CoreTCLpath);
  gui->execute("lappend auto_path "+DataflowTCLpath);
  gui->execute("lappend auto_path "ITCL_WIDGETS);
  gui->source_once(DataflowTCLpath+string("/NetworkEditor.tcl"));

  
  tcl_task->release_mainloop();
  packageDB->setGui(gui);
  
  net = new Network();
  Scheduler* sched_task=new Scheduler(net);
  new NetworkEditor(net, gui);
  gui->execute("wm withdraw .");

  // Activate the scheduler.  Arguments and return
  // values are meaningless
  Thread* t2=new Thread(sched_task, "Scheduler");
  t2->setDaemon(true);
  t2->detach();
}

bool SCIRunComponentModel::destroyInstance(ComponentInstance * ic)
{
  cerr<<"Warning:I don't know how to destroy a SCIRun component instance\n";
  return true; 
}

string SCIRunComponentModel::getName() const
{
  return "Dataflow";
}

void SCIRunComponentModel::listAllComponentTypes(vector<ComponentDescription*>& list,
						 bool /*listInternal*/)
{
  vector<string> packages = packageDB->packageNames();
  typedef vector<string>::iterator striter;
  for(striter iter = packages.begin(); iter != packages.end(); ++iter){
    string package = *iter;
    vector<string> categories = packageDB->categoryNames(package);
    for(striter iter = categories.begin(); iter != categories.end(); ++iter){
      string category = *iter;
      vector<string> modules = packageDB->moduleNames(package, category);
      for(striter iter = modules.begin(); iter != modules.end(); ++iter){
	string module = *iter;
	list.push_back(new SCIRunComponentDescription(this, package,
						      category, module));
      }
    }
  }
}
