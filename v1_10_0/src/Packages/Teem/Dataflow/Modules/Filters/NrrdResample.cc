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
 *  NrrdResample
 *
 *  Written by:
 *   David Weinstein
 *   Department of Computer Science
 *   University of Utah
 *   January 2000
 *
 *  Copyright (C) 2000 SCI Group
 */


#include <Dataflow/Network/Module.h>
#include <Core/Malloc/Allocator.h>
#include <Core/GuiInterface/GuiVar.h>
#include <Teem/Dataflow/Ports/NrrdPort.h>
#include <Core/Containers/StringUtil.h>

#include <iostream>
#include <stdio.h>

using std::endl;

namespace SCITeem {

class NrrdResample : public Module {
public:
  int getint(const char *str, int *n, int *none);
  int get_sizes(vector<string> &resampAxis, Nrrd *nrrd, 
		NrrdResampleInfo *info);
  NrrdResample(GuiContext *ctx);
  virtual ~NrrdResample();
  virtual void execute();

private:
  void load_gui();

  NrrdIPort          *inrrd_;
  NrrdOPort          *onrrd_;
  GuiString           filtertype_;
  GuiInt              dim_;
  vector<GuiString*>  resampAxes_;
  vector<string>      last_RA_;
  GuiDouble           sigma_;
  GuiDouble           extent_;
  string              last_filtertype_;
  int                 last_generation_;
  NrrdDataHandle      last_nrrdH_;

};

} // End namespace SCITeem
using namespace SCITeem;

DECLARE_MAKER(NrrdResample)

NrrdResample::NrrdResample(GuiContext *ctx) : 
  Module("NrrdResample", ctx, Filter, "Filters", "Teem"),
  filtertype_(ctx->subVar("filtertype")),
  dim_(ctx->subVar("dim")),
  sigma_(ctx->subVar("sigma")),
  extent_(ctx->subVar("extent")),
  last_filtertype_(""), 
  last_generation_(-1), 
  last_nrrdH_(0)
{
  // value will be overwritten at gui side initialization.
  dim_.set(0);
}

NrrdResample::~NrrdResample() 
{
}


void
NrrdResample::load_gui() {
  dim_.reset();
  if (dim_.get() == 0) { return; }

  last_RA_.resize(dim_.get(), "--");
  for (int a = 0; a < dim_.get(); a++) {
    ostringstream str;
    str << "resampAxis" << a;
    resampAxes_.push_back(new GuiString(ctx->subVar(str.str())));
  }
}

int 
NrrdResample::getint(const char *str, int *n, int *none) 
{
  if (!strlen(str)) return 1;
  if (str[0] == 'x') {
    if (strlen(str) > 1) {
      double ratio;
      if (sscanf(str+1, "%lf", &ratio) != 1) return 1;
      *n = (int)(*n * ratio);
    }
  } else if (str[0] == '=') {
    *none = 1;
  } else {
    if (sscanf(str, "%d", n) != 1) return 1;
  }
  if (*n < 2 && !none) {
    error("Invalid # of samples (" + to_string(*n) + ").");
    return 1;
  }
  return 0;
}


int 
NrrdResample::get_sizes(vector<string> &resampAxis, Nrrd *nrrd, NrrdResampleInfo *info)
{
  msgStream_ << "NrrdResample sizes: ";
  // set tuple axis info
  info->samples[0] = nrrd->axis[0].size;
  info->kernel[0] = 0;
  dim_.reset();
  for (int a = 1; a < dim_.get(); a++) {
    // only do work on geometric axes.
    info->samples[a] = nrrd->axis[a].size;
    const char *str = resampAxis[a].c_str();
    int none=0;
    if (getint(str, &(info->samples[a]), &none)) { 
      msgStream_ << "NrrdResample -- bad size.\n"; 
      return 0;
    }
    if (none) info->kernel[a] = 0;
    msgStream_ << info->samples[a];
    if (!info->kernel[a]) msgStream_ << "=";
    msgStream_ << " ";
  }
  msgStream_ << endl;
  return 1;
}

void 
NrrdResample::execute()
{
  NrrdDataHandle nrrdH;
  update_state(NeedData);
  inrrd_ = (NrrdIPort *)get_iport("Nrrd");
  onrrd_ = (NrrdOPort *)get_oport("Nrrd");
  if (!inrrd_) {
    error("Unable to initialize iport 'Nrrd'.");
    return;
  }
  if (!onrrd_) {
    error("Unable to initialize oport 'Nrrd'.");
    return;
  }
  if (!inrrd_->get(nrrdH))
    return;
  if (!nrrdH.get_rep()) {
    error("Empty input Nrrd.");
    return;
  }
  dim_.reset();

  if (last_generation_ != nrrdH->generation) {
    if (last_generation_ != -1) {
      last_RA_.clear();
      vector<GuiString*>::iterator iter = resampAxes_.begin();
      while(iter != resampAxes_.end()) {
	delete *iter;
	++iter;
      }
      resampAxes_.clear();
      gui->execute(id.c_str() + string(" clear_axes"));
    }
    
    last_generation_ = nrrdH->generation;
    dim_.set(nrrdH->nrrd->dim);
    dim_.reset();
    load_gui();
    gui->execute(id.c_str() + string(" init_axes"));
  }

  filtertype_.reset();
  for (int a = 0; a < dim_.get(); a++) {
    resampAxes_[a]->reset();
  }

  // See if gui values have changed from last execute,
  // and set up execution values. 
  bool changed = false;
  if (last_filtertype_ != filtertype_.get()) {
    changed = true;
    last_filtertype_ = filtertype_.get();
  }
  for (int a = 0; a < dim_.get(); a++) {
    if (last_RA_[a] != resampAxes_[a]->get()) {
      changed = true;
      last_RA_[a] = resampAxes_[a]->get();
    }
  }

  if (! changed) {
    onrrd_->send(last_nrrdH_);
    return;
  }
  
  NrrdResampleInfo *info = nrrdResampleInfoNew();

  Nrrd *nin = nrrdH->nrrd;
  msgStream_ << "Resampling with a " << last_filtertype_ << " filter." << endl;
  NrrdKernel *kern;
  double p[NRRD_KERNEL_PARMS_NUM];
  memset(p, 0, NRRD_KERNEL_PARMS_NUM * sizeof(double));
  p[0] = 1.0;
  if (last_filtertype_ == "box") {
    kern = nrrdKernelBox;
  } else if (last_filtertype_ == "tent") {
    kern = nrrdKernelTent;
  } else if (last_filtertype_ == "gaussian") { 
    kern = nrrdKernelGaussian; 
    p[1] = sigma_.get(); 
    p[2] = extent_.get(); 
  } else if (last_filtertype_ == "cubicCR") { 
    kern = nrrdKernelBCCubic; 
    p[1] = 0; 
    p[2] = 0.5; 
  } else if (last_filtertype_ == "cubicBS") { 
    kern = nrrdKernelBCCubic; 
    p[1] = 1; 
    p[2] = 0; 
  } else  { // default is quartic
    kern = nrrdKernelAQuartic; 
    p[1] = 0.0834; 
  }

  for (int a = 0; a < dim_.get(); a++) {
    info->kernel[a] = kern;
    memcpy(info->parm[a], p, NRRD_KERNEL_PARMS_NUM * sizeof(double));
    if (!(AIR_EXISTS(nin->axis[a].min) && AIR_EXISTS(nin->axis[a].max)))
      nrrdAxisMinMaxSet(nrrdH->nrrd, a, nrrdCenterNode);
    info->min[a] = nrrdH->nrrd->axis[a].min;
    info->max[a] = nrrdH->nrrd->axis[a].max;
  }    
  info->boundary = nrrdBoundaryBleed;
  info->type = nin->type;
  info->renormalize = AIR_TRUE;

  if (!get_sizes(last_RA_, nrrdH->nrrd, info)) return;

  last_generation_ = nrrdH->generation;

  NrrdData *nrrd = scinew NrrdData;
  if (nrrdSpatialResample(nrrd->nrrd=nrrdNew(), nin, info)) {
    char *err = biffGetDone(NRRD);
    error(string("Trouble resampling: ") +  err);
    msgStream_ << "  input Nrrd: nin->dim=" << nin->dim << "\n";
    free(err);
  }
  nrrdResampleInfoNix(info); 
  nrrd->copy_sci_data(*nrrdH.get_rep());
  last_nrrdH_ = nrrd;
  onrrd_->send(last_nrrdH_);
}

