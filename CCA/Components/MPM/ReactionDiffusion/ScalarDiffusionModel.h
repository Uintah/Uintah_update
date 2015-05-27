/*
 * The MIT License
 *
 * Copyright (c) 1997-2014 The University of Utah
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef UINTAH_RF_SCALARDIFFUSIONMODEL_H
#define UINTAH_RF_SCALARDIFFUSIONMODEL_H

#include <Core/Grid/Variables/ComputeSet.h>
#include <Core/Grid/SimulationStateP.h>
#include <Core/ProblemSpec/ProblemSpecP.h>
#include <Core/Grid/Variables/VarLabel.h>

#include <string>

namespace Uintah {

  class Task;
  class MPMFlags;
  class MPMLabel;
  class MPMMaterial;
  class ReactionDiffusionLabel;
  class DataWarehouse;
  class ProcessorGroup;

  
  class ScalarDiffusionModel {
  public:
    
    ScalarDiffusionModel(ProblemSpecP& ps, SimulationStateP& sS, MPMFlags* Mflag,
                         std::string diff_type);
    virtual ~ScalarDiffusionModel();

    virtual std::string getDiffusionType();

    virtual void setIncludeHydroStress(bool value);

    virtual void addInitialComputesAndRequires(Task* task, const MPMMaterial* matl,
                                               const PatchSet* patches) const;

    virtual void initializeSDMData(const Patch* patch, const MPMMaterial* matl,
                                   DataWarehouse* new_dw);

    virtual void addParticleState(std::vector<const VarLabel*>& from,
                                  std::vector<const VarLabel*>& to);

    virtual void scheduleInterpolateParticlesToGrid(Task* task,
		                                    const MPMMaterial* matl,
                                                    const PatchSet* patch) const;

    virtual void interpolateParticlesToGrid(const Patch* patch, const MPMMaterial* matl,
                                            DataWarehouse* old_dw, DataWarehouse* new_dw);

    virtual void scheduleComputeFlux(Task* task, const MPMMaterial* matl, 
		                                      const PatchSet* patch) const;

    virtual void computeFlux(const Patch* patch, const MPMMaterial* matl,
                                  DataWarehouse* old_dw, DataWarehouse* new_dw);

    virtual void scheduleComputeDivergence(Task* task, const MPMMaterial* matl, 
		                                      const PatchSet* patch) const;

    virtual void computeDivergence(const Patch* patch, const MPMMaterial* matl,
                                  DataWarehouse* old_dw, DataWarehouse* new_dw);

    virtual void scheduleInterpolateToParticlesAndUpdate(Task* task, const MPMMaterial* matl, 
		                                                     const PatchSet* patch) const;

    virtual void interpolateToParticlesAndUpdate(const Patch* patch, const MPMMaterial* matl,
                                                 DataWarehouse* old_dw, DataWarehouse* new_dw);

#if 0
    virtual void scheduleFinalParticleUpdate(Task* task, const MPMMaterial* matl, 
		                                         const PatchSet* patch) const;

    virtual void finalParticleUpdate(const Patch* patch, const MPMMaterial* matl,
                                     DataWarehouse* old_dw, DataWarehouse* new_dw);
#endif

  protected:
    MPMLabel* d_lb;
    MPMFlags* d_Mflag;
    SimulationStateP d_sharedState;
    ReactionDiffusionLabel* d_rdlb;

    int NGP, NGN;
    bool do_explicit;
    std::string diffusion_type;
    bool include_hydrostress;

    ScalarDiffusionModel(const ScalarDiffusionModel&);
    ScalarDiffusionModel& operator=(const ScalarDiffusionModel&);
    
    double diffusivity;
    double max_concentration;
  };
  
} // end namespace Uintah
#endif
