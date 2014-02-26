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

#ifndef UINTAH_MD_MATERIAL_H
#define UINTAH_MD_MATERIAL_H

#include <Core/Grid/Material.h>
#include <Core/ProblemSpec/ProblemSpecP.h>
#include <Core/Geometry/Vector.h>

namespace Uintah {

  using namespace SCIRun;

  class MDMaterial : public Material {

    public:

      MDMaterial();
      MDMaterial(ProblemSpecP&, SimulationStateP& sharedState);

      virtual ~MDMaterial();

      virtual ProblemSpecP outputProblemSpec(ProblemSpecP& ps) = 0;

      virtual void calculateForce(SCIRun::Vector& Force) const = 0;
      virtual void calculateEnergy(double& Energy) const = 0;
      virtual void calculateForceAndEnergy(SCIRun::Vector& Force, double& Energy) const = 0;
      virtual SCIRun::Vector getForce() const = 0;
      virtual double getEnergy() const = 0;

      virtual std::string getMaterialDescriptor() const = 0;

      virtual double getCharge() const = 0;
      virtual double getPolarizability() const = 0;

    private:
      // Prevent copying or assignment
      MDMaterial(const MDMaterial &material);
      MDMaterial& operator=(const MDMaterial &material);

  };

}  // End namespace Uintah

#endif // UINTAH_MD_MATERIAL_H
