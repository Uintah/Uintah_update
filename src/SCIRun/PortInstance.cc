/*
  The contents of this file are subject to the University of Utah Public
  License (the "License"); you may not use this file except in compliance
  with the License.
  
  Software distributed under the License is distributed on an "AS IS"
  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
  License for the specific language governing rights and limitations under
  the License.
  
  The Original Source Code is SCIRun, released March 12, 2001.
  
  The Original Source Code was developed by the University of Utah.
  Portions created by UNIVERSITY are Copyright (C) 2001, 1994 
  University of Utah. All Rights Reserved.
*/

/*
 *  PortRecord.cc:
 *
 *  Written by:
 *   Steven G. Parker
 *   Department of Computer Science
 *   University of Utah
 *   October 2001
 *
 */

#include <SCIRun/PortInstance.h>
#include <iostream>
using namespace SCIRun;
using namespace std;

std::string PortInstance::getType()
{
  return "default type";
}

std::string PortInstance::getModel()
{
  return "default model";
}

bool PortInstance::available()
{
  cerr<<"default available() is called!";
  return true;
}

PortInstance* PortInstance::getPeer()
{
  cerr<<"default getPeer() is called!";
  return 0;
}

PortInstance::PortInstance()
{
}

PortInstance::~PortInstance()
{
}
