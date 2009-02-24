/*

The MIT License

Copyright (c) 1997-2009 Center for the Simulation of Accidental Fires and 
Explosions (CSAFE), and  Scientific Computing and Imaging Institute (SCI), 
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


//  Kayenta.h 
//  class ConstitutiveModel ConstitutiveModel data type -- 3D - 
//  holds ConstitutiveModel
//    This is for calling the Kayenta model
//    Features:
//      Usage:



#ifndef __KAYENTA_H__
#define __KAYENTA_H__

#include <cmath>
#include "ConstitutiveModel.h"	
#include <Uintah/Core/Math/Matrix3.h>
#include <vector>
#include <Uintah/Core/Grid/Variables/VarLabel.h>

namespace Uintah {
  class Kayenta : public ConstitutiveModel {
  public:
    double UI[90];
    vector<const VarLabel*> ISVLabels;
    vector<const VarLabel*> ISVLabels_preReloc;
    int d_NINSV;

  private:
    // Prevent copying of this class
    // copy constructor
    Kayenta& operator=(const Kayenta &cm);

    void getInputParameters(ProblemSpecP& ps);

    void initializeLocalMPMLabels();

  public:
    // constructors
    Kayenta(ProblemSpecP& ps, MPMFlags* flag);
    Kayenta(const Kayenta* cm);
       
    // destructor
    virtual ~Kayenta();

    virtual void outputProblemSpec(ProblemSpecP& ps,bool output_cm_tag = true);

    // clone
    Kayenta* clone();

    // compute stable timestep for this patch
    virtual void computeStableTimestep(const Patch* patch,
				       const MPMMaterial* matl,
				       DataWarehouse* new_dw);

    // compute stress at each particle in the patch
    virtual void computeStressTensor(const PatchSubset* patches,
				     const MPMMaterial* matl,
				     DataWarehouse* old_dw,
				     DataWarehouse* new_dw);


    // carry forward CM data for RigidMPM
    virtual void carryForward(const PatchSubset* patches,
                              const MPMMaterial* matl,
                              DataWarehouse* old_dw,
                              DataWarehouse* new_dw);

    // initialize  each particle's constitutive model data
    virtual void initializeCMData(const Patch* patch,
				  const MPMMaterial* matl,
				  DataWarehouse* new_dw);

    virtual void allocateCMDataAddRequires(Task* task, const MPMMaterial* matl,
					   const PatchSet* patch, 
					   MPMLabel* lb) const;


    virtual void allocateCMDataAdd(DataWarehouse* new_dw,
				   ParticleSubset* subset,
				   map<const VarLabel*,
                                   ParticleVariableBase*>* newState,
				   ParticleSubset* delset,
				   DataWarehouse* old_dw);

    virtual void addInitialComputesAndRequires(Task* task,
                                               const MPMMaterial* matl,
                                               const PatchSet* patches) const;

    virtual void addComputesAndRequires(Task* task,
					const MPMMaterial* matl,
					const PatchSet* patches) const;

    virtual void addComputesAndRequires(Task* task,
					const MPMMaterial* matl,
					const PatchSet* patches,
					const bool recursion) const;

    virtual double computeRhoMicroCM(double pressure,
				     const double p_ref,
				     const MPMMaterial* matl);

    virtual void computePressEOSCM(double rho_m, double& press_eos,
				   double p_ref,
				   double& dp_drho, double& ss_new,
				   const MPMMaterial* matl);

    virtual double getCompressibility();


    virtual void addParticleState(std::vector<const VarLabel*>& from,
				  std::vector<const VarLabel*>& to);
  };

} // End namespace Uintah

#endif  // __KAYENTA_H__ 
