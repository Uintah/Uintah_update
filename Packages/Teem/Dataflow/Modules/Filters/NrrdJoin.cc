//  The contents of this file are subject to the University of Utah Public
//  License (the "License"); you may not use this file except in compliance
//  with the License.
//  
//  Software distributed under the License is distributed on an "AS IS"
//  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
//  License for the specific language governing rights and limitations under
//  the License.
//  
//  The Original Source Code is SCIRun, released March 12, 2001.
//  
//  The Original Source Code was developed by the University of Utah.
//  Portions created by UNIVERSITY are Copyright (C) 2001, 1994
//  University of Utah. All Rights Reserved.
//  
//    File   : NrrdJoin.cc
//    Author : Martin Cole
//    Date   : Wed Jan 15 10:59:57 2003

#include <Dataflow/Network/Module.h>
#include <Core/Malloc/Allocator.h>
#include <Core/GuiInterface/GuiVar.h>
#include <Core/Containers/StringUtil.h>
#include <Teem/Dataflow/Ports/NrrdPort.h>

#include <iostream>
using std::endl;
#include <stdio.h>

namespace SCITeem {

class NrrdJoin : public Module {
public:
  NrrdJoin(SCIRun::GuiContext *ctx);
  virtual ~NrrdJoin();

  virtual void execute();

private:
  NrrdOPort* onrrd_;

  NrrdDataHandle    onrrd_handle_;  //! the cached output nrrd handle.
  vector<int>       in_generation_; //! all input generation nums.
  int               onrrd_type_;    //! target type for output nrrd.

  GuiString         join_axis_;
  GuiInt            incr_dim_;
};

} // End namespace SCITeem
using namespace SCITeem;
DECLARE_MAKER(NrrdJoin)

NrrdJoin::NrrdJoin(SCIRun::GuiContext *ctx) : 
  Module("NrrdJoin", ctx, Filter, "Filters", "Teem"), 
  onrrd_(0),
  onrrd_handle_(0),
  in_generation_(0),
  onrrd_type_(nrrdTypeLast),
  join_axis_(ctx->subVar("join-axis")),
  incr_dim_(ctx->subVar("incr-dim"))
{
}

NrrdJoin::~NrrdJoin() {
}

void 
NrrdJoin::execute()
{
  if (! onrrd_) {
    onrrd_ = (NrrdOPort *)get_oport("JoinedNrrd");
  }
  if (!onrrd_) {
    error("Unable to initialize oport 'Nrrds'.");
    return;
  }
  

  port_range_type range = get_iports("Nrrds");
  if (range.first == range.second) { return; }

  unsigned int i = 0;
  vector<NrrdDataHandle> nrrds;
  bool do_join = false;

  port_map_type::iterator pi = range.first;
  while (pi != range.second)
  {
    NrrdIPort *inrrd = (NrrdIPort *)get_iport(pi->second);
    if (!inrrd) {
      error("Unable to initialize iport '" + to_string(pi->second) + "'.");
      return;
    }

    NrrdDataHandle nrrd;

    if (inrrd->get(nrrd)) {
      cout << "found a nrrd in port: " << i << endl;
      cout << "its dim is: " << nrrd->nrrd->dim << endl;
      // check to see if we need to do the join or can output the cached onrrd.
      if (in_generation_.size() <= i) {
	// this is a new input, never been joined.
	do_join = true;
	in_generation_.push_back(nrrd->generation);
	onrrd_type_ = nrrdTypeLast;
      } else if (in_generation_[i] != nrrd->generation) {
	// different input than last execution
	do_join = true;
	in_generation_[i] = nrrd->generation;
	onrrd_type_ = nrrdTypeLast;
      }

      // the output nrrd must be of one type, so find the type that accomodates
      // all of the nrrds we have as input.
      if (onrrd_type_ == nrrdTypeLast) {
	// first time this value is set
	onrrd_type_ = nrrd->nrrd->type;
      }
      if ((onrrd_type_ != nrrd->nrrd->type) && 
	  (onrrd_type_ != nrrdTypeDouble)) {
	//! promote to the biggest type
	if (! nrrdTypeFixed[nrrd->nrrd->type]) {
	  //! just promote to double
	  onrrd_type_ = nrrdTypeDouble;
	} else if (nrrdTypeSize[nrrd->nrrd->type] > nrrdTypeSize[onrrd_type_]){
	  onrrd_type_ = nrrd->nrrd->type;
	}
      }
      

      nrrds.push_back(nrrd);
    }
    ++pi; ++i;
  }
  
  Nrrd* arr[nrrds.size()];
  if (do_join) {

    NrrdData *onrrd = new NrrdData(true);
    int i = 0;
    vector<NrrdDataHandle>::iterator iter = nrrds.begin();
    while(iter != nrrds.end()) {
      NrrdDataHandle nh = *iter;
      ++iter;

      NrrdData* cur_nrrd = nh.get_rep();
      // does it need conversion to the bigger type?
      if (cur_nrrd->nrrd->type != onrrd_type_) {
	Nrrd* new_nrrd = nrrdNew();
	if (nrrdConvert(new_nrrd, cur_nrrd->nrrd, onrrd_type_)) {
	  char *err = biffGetDone(NRRD);
	  error(string("Conversion Error: ") +  err);
	  free(err);
	  return;
	}
	arr[i] = new_nrrd;
      } else {
	arr[i] = cur_nrrd->nrrd;
      }
      if (i == 0) {
	onrrd->copy_sci_data(*cur_nrrd);
      }
      ++i;
    }
    
    int axis = 0;
    join_axis_.reset();
    incr_dim_.reset();
    cout << "axis " << join_axis_.get() << endl;
    cout << "incrdim " << incr_dim_.get() << endl;

    if (join_axis_.get() == "x") {
      axis = 1;
    } else if (join_axis_.get() == "y") {
      axis = 2;
    } else if (join_axis_.get() == "z") {
      axis = 3;
    } else if (join_axis_.get() == "sink") {
      axis = 0;
    }
    
    onrrd->nrrd = nrrdNew();
    if (nrrdJoin(onrrd->nrrd, arr, nrrds.size(), axis, incr_dim_.get())) {
      char *err = biffGetDone(NRRD);
      error(string("Join Error: ") +  err);
      free(err);
      return;
    }
    onrrd_handle_ = onrrd;
  }

  onrrd_->send(onrrd_handle_);
}

