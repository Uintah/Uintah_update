
#include "ConstitutiveModelFactory.h"
#include "CompNeoHook.h"
#include <Uintah/Grid/Region.h>
#include <Uintah/Interface/DataWarehouse.h>
#include <Uintah/Grid/NCVariable.h>
#include <Uintah/Grid/ParticleSet.h>
#include <Uintah/Grid/ParticleVariable.h>
#include <Uintah/Grid/ReductionVariable.h>
#include <Uintah/Grid/Task.h>
#include <Uintah/Grid/VarLabel.h>
#include <SCICore/Math/MinMax.h>
#include <Uintah/Components/MPM/Util/Matrix3.h>
#include <Uintah/Components/MPM/ConstitutiveModel/MPMMaterial.h>
#include <Uintah/Grid/VarTypes.h>
#include <Uintah/Components/MPM/MPMLabel.h>
#include <fstream>
#include <iostream>

using std::cerr;
using namespace Uintah::MPM;
using SCICore::Math::Min;
using SCICore::Math::Max;
using SCICore::Geometry::Vector;

CompNeoHook::CompNeoHook(ProblemSpecP& ps)
{
  ps->require("bulk_modulus",d_initialData.Bulk);
  ps->require("shear_modulus",d_initialData.Shear);

  p_cmdata_label = new VarLabel("p.cmdata",
                                ParticleVariable<CMData>::getTypeDescription());

  bElBarLabel = new VarLabel("p.bElBar",
                ParticleVariable<Point>::getTypeDescription(),
                                VarLabel::PositionVariable);

 
}

CompNeoHook::~CompNeoHook()
{
  // Destructor
 
}

void CompNeoHook::initializeCMData(const Region* region,
                                        const MPMMaterial* matl,
                                        DataWarehouseP& new_dw)
{
   // Put stuff in here to initialize each particle's
   // constitutive model parameters and deformationMeasure
   Matrix3 Identity, zero(0.);
   Identity.Identity();

   const MPMLabel* lb = MPMLabel::getLabels();

   ParticleVariable<CMData> cmdata;
   new_dw->allocate(cmdata, p_cmdata_label, matl->getDWIndex(), region);
   ParticleVariable<Matrix3> deformationGradient;
   new_dw->allocate(deformationGradient,
                lb->pDeformationMeasureLabel, matl->getDWIndex(), region);
   ParticleVariable<Matrix3> pstress;
   new_dw->allocate(pstress, lb->pStressLabel, matl->getDWIndex(), region);
   ParticleVariable<Matrix3> bElBar;
   new_dw->allocate(bElBar,  bElBarLabel, matl->getDWIndex(), region);

   ParticleSubset* pset = cmdata.getParticleSubset();
   for(ParticleSubset::iterator iter = pset->begin();
          iter != pset->end(); iter++) {
            cmdata[*iter] = d_initialData;
          deformationGradient[*iter] = Identity;
          bElBar[*iter] = Identity;
          pstress[*iter] = zero;
   }
   new_dw->put(cmdata, p_cmdata_label, matl->getDWIndex(), region);
   new_dw->put(deformationGradient, lb->pDeformationMeasureLabel,
                                 matl->getDWIndex(), region);
   new_dw->put(pstress, lb->pStressLabel, matl->getDWIndex(), region);
   new_dw->put(bElBar, bElBarLabel, matl->getDWIndex(), region);

   computeStableTimestep(region, matl, new_dw);

}

void CompNeoHook::computeStableTimestep(const Region* region,
                                             const MPMMaterial* matl,
                                             DataWarehouseP& new_dw)
{
   // This is only called for the initial timestep - all other timesteps
   // are computed as a side-effect of computeStressTensor
  Vector dx = region->dCell();
  int matlindex = matl->getDWIndex();

  const MPMLabel* lb = MPMLabel::getLabels();
  // Retrieve the array of constitutive parameters
  ParticleVariable<CMData> cmdata;
  new_dw->get(cmdata, p_cmdata_label, matlindex, region, Ghost::None, 0);
  ParticleVariable<double> pmass;
  new_dw->get(pmass, lb->pMassLabel, matlindex, region, Ghost::None, 0);
  ParticleVariable<double> pvolume;
  new_dw->get(pvolume, lb->pVolumeLabel, matlindex, region, Ghost::None, 0);

  ParticleSubset* pset = pmass.getParticleSubset();
  ASSERT(pset == pvolume.getParticleSubset());

  double c_dil = 0.0,c_rot = 0.0;
  for(ParticleSubset::iterator iter = pset->begin();
      iter != pset->end(); iter++){
     particleIndex idx = *iter;

     // Compute wave speed at each particle, store the maximum
     double mu = cmdata[idx].Shear;
     double lambda = cmdata[idx].Bulk -.6666666667*cmdata[idx].Shear;
     c_dil = Max(c_dil,(lambda + 2.*mu)*pvolume[idx]/pmass[idx]);
     c_rot = Max(c_rot, mu*pvolume[idx]/pmass[idx]);
    }
    double WaveSpeed = sqrt(Max(c_rot,c_dil));
    // Fudge factor of .8 added, just in case
    double delt_new = .8*(Min(dx.x(), dx.y(), dx.z())/WaveSpeed);
    new_dw->put(delt_vartype(delt_new), lb->deltLabel);

}

void CompNeoHook::computeStressTensor(const Region* region,
                                          const MPMMaterial* matl,
                                          DataWarehouseP& old_dw,
                                          DataWarehouseP& new_dw)
{
  Matrix3 velGrad,Shear,fbar,deformationGradientInc;
  double J,p;
  double c_dil, c_rot,WaveSpeed;
  double onethird = (1.0/3.0);
  Matrix3 Identity;

  Identity.Identity();

  Vector dx = region->dCell();
  double oodx[3] = {1./dx.x(), 1./dx.y(), 1./dx.z()};

  int matlindex = matl->getDWIndex();
  const MPMLabel* lb = MPMLabel::getLabels();
  // Create array for the particle position
  ParticleVariable<Point> px;
  old_dw->get(px, lb->pXLabel, matlindex, region, Ghost::None, 0);
  // Create array for the particle deformation
  ParticleVariable<Matrix3> deformationGradient;
  old_dw->get(deformationGradient, lb->pDeformationMeasureLabel,
              matlindex, region, Ghost::None, 0);
  ParticleVariable<Matrix3> bElBar;
  old_dw->get(bElBar, bElBarLabel, matlindex, region, Ghost::None, 0);

  // Create array for the particle stress
  ParticleVariable<Matrix3> pstress;
  new_dw->allocate(pstress, lb->pStressLabel, matlindex, region);

  // Retrieve the array of constitutive parameters
  ParticleVariable<CMData> cmdata;
  old_dw->get(cmdata, p_cmdata_label, matlindex, region, Ghost::None, 0);
  ParticleVariable<double> pmass;
  old_dw->get(pmass, lb->pMassLabel, matlindex, region, Ghost::None, 0);
  ParticleVariable<double> pvolume;
  old_dw->get(pvolume, lb->pVolumeLabel, matlindex, region, Ghost::None, 0);

  NCVariable<Vector> gvelocity;

  new_dw->get(gvelocity, lb->gMomExedVelocityLabel, matlindex,region,
              Ghost::None, 0);
  delt_vartype delt;
  old_dw->get(delt, lb->deltLabel);

  ParticleSubset* pset = px.getParticleSubset();
  ASSERT(pset == pstress.getParticleSubset());
  ASSERT(pset == deformationGradient.getParticleSubset());
  ASSERT(pset == pmass.getParticleSubset());
  ASSERT(pset == pvolume.getParticleSubset());

  for(ParticleSubset::iterator iter = pset->begin();
     iter != pset->end(); iter++){
     particleIndex idx = *iter;

     velGrad.set(0.0);
     // Get the node indices that surround the cell
     IntVector ni[8];
     Vector d_S[8];
     if(!region->findCellAndShapeDerivatives(px[idx], ni, d_S))
         continue;

      for(int k = 0; k < 8; k++) {
          Vector& gvel = gvelocity[ni[k]];
          for (int j = 0; j<3; j++){
            for (int i = 0; i<3; i++) {
                velGrad(i+1,j+1)+=gvel(i) * d_S[k](j) * oodx[j];
            }
          }
      }

    // Calculate the stress Tensor (symmetric 3 x 3 Matrix) given the
    // time step and the velocity gradient and the material constants
    double shear = cmdata[idx].Shear;
    double bulk  = cmdata[idx].Bulk;

    // Compute the deformation gradient increment using the time_step
    // velocity gradient
    // F_n^np1 = dudx * dt + Identity
    deformationGradientInc = velGrad * delt + Identity;

    // Update the deformation gradient tensor to its time n+1 value.
    deformationGradient[idx] = deformationGradientInc *
                             deformationGradient[idx];

    // get the volume preserving part of the deformation gradient increment
    fbar = deformationGradientInc *
                        pow(deformationGradientInc.Determinant(),-onethird);

    bElBar[idx] = fbar*bElBar[idx]*fbar.Transpose();

    // Shear is equal to the shear modulus times dev(bElBar)
    Shear = (bElBar[idx] - Identity*onethird*bElBar[idx].Trace())*shear;

    // get the volumetric part of the deformation
    J = deformationGradient[idx].Determinant();

    // get the hydrostatic part of the stress
    p = 0.5*bulk*(J - 1.0/J);

    // compute the total stress (volumetric + deviatoric)
    pstress[idx] = Identity*J*p + Shear;

    // Compute wave speed at each particle, store the maximum
    double mu = cmdata[idx].Shear;
    double lambda = cmdata[idx].Bulk -.6666666667*cmdata[idx].Shear;

    c_dil = Max(c_dil,(lambda + 2.*mu)*pvolume[idx]/pmass[idx]);
    c_rot = Max(c_rot, mu*pvolume[idx]/pmass[idx]);
  }

  WaveSpeed = sqrt(Max(c_rot,c_dil));
  // Fudge factor of .8 added, just in case
  double delt_new = .8*Min(dx.x(), dx.y(), dx.z())/WaveSpeed;
  new_dw->put(delt_vartype(delt_new), lb->deltLabel);
  new_dw->put(pstress, lb->pStressLabel, matlindex, region);
  new_dw->put(deformationGradient, lb->pDeformationMeasureLabel,
                matlindex, region);
  new_dw->put(bElBar, bElBarLabel, matlindex, region);

  // This is just carried forward with the updated alpha
  new_dw->put(cmdata, p_cmdata_label, matlindex, region);
  // Volume is currently being carried forward, will be updated
  new_dw->put(pvolume,lb->pVolumeLabel, matlindex,region);

}

double CompNeoHook::computeStrainEnergy(const Region* region,
                                        const MPMMaterial* matl,
                                        DataWarehouseP& new_dw)
{
  double U,W,J,se=0;
  int matlindex = matl->getDWIndex();
  const MPMLabel* lb = MPMLabel::getLabels();
  // Create array for the particle deformation
  ParticleVariable<Matrix3> deformationGradient;
  new_dw->get(deformationGradient, lb->pDeformationMeasureLabel,
              matlindex, region, Ghost::None, 0);

  // Get the elastic part of the shear strain
  ParticleVariable<Matrix3> bElBar;
  new_dw->get(bElBar, bElBarLabel, matlindex, region, Ghost::None, 0);
  // Retrieve the array of constitutive parameters
  ParticleVariable<CMData> cmdata;
  new_dw->get(cmdata, p_cmdata_label, matlindex, region, Ghost::None, 0);
  ParticleVariable<double> pvolume;
  new_dw->get(pvolume, lb->pVolumeLabel, matlindex, region, Ghost::None, 0);

  ParticleSubset* pset = deformationGradient.getParticleSubset();
  ASSERT(pset == pvolume.getParticleSubset());

  for(ParticleSubset::iterator iter = pset->begin();
     iter != pset->end(); iter++){
     particleIndex idx = *iter;

     double shear = cmdata[idx].Shear;
     double bulk  = cmdata[idx].Bulk;

     J = deformationGradient[idx].Determinant();

     U = .5*bulk*(.5*(pow(J,2.0) - 1.0) - log(J));
     W = .5*shear*(bElBar[idx].Trace() - 3.0);

     se += (U + W)*pvolume[idx];
  }

  return se;
}

void CompNeoHook::addComputesAndRequires(Task* task,
					 const MPMMaterial* matl,
					 const Region* region,
					 DataWarehouseP& old_dw,
					 DataWarehouseP& new_dw) const
{
  const  MPMLabel* lb = MPMLabel::getLabels();
   task->requires(old_dw, lb->pXLabel, matl->getDWIndex(), region,
                  Ghost::None);
   task->requires(old_dw, lb->pDeformationMeasureLabel, matl->getDWIndex(), region,
                  Ghost::None);
   task->requires(old_dw, p_cmdata_label, matl->getDWIndex(),  region,
                  Ghost::None);
   task->requires(old_dw, lb->pMassLabel, matl->getDWIndex(),  region,
                  Ghost::None);
   task->requires(old_dw, lb->pVolumeLabel, matl->getDWIndex(),  region,
                  Ghost::None);
   task->requires(new_dw, lb->gMomExedVelocityLabel, matl->getDWIndex(), region,
                  Ghost::AroundCells, 1);
   task->requires(old_dw, bElBarLabel, matl->getDWIndex(), region,
                  Ghost::None);
   task->requires(old_dw, lb->deltLabel);

   task->computes(new_dw, lb->deltLabel);
   task->computes(new_dw, lb->pStressLabel, matl->getDWIndex(),  region);
   task->computes(new_dw, lb->pDeformationMeasureLabel, matl->getDWIndex(), region);
   task->computes(new_dw, bElBarLabel, matl->getDWIndex(),  region);
   task->computes(new_dw, p_cmdata_label, matl->getDWIndex(),  region);
   task->computes(new_dw, lb->pVolumeLabel, matl->getDWIndex(), region);

}
#ifdef __sgi
#define IRIX
#pragma set woff 1209
#endif

namespace Uintah {
   namespace MPM {
const TypeDescription* fun_getTypeDescription(CompNeoHook::CMData*)
{
   static TypeDescription* td = 0;
   if(!td){
      ASSERTEQ(sizeof(CompNeoHook::CMData), sizeof(double)*2);
      td = new TypeDescription(TypeDescription::Other, "CompNeoHook::CMData", true);
   }
   return td;
}
   }
}

// $Log$
// Revision 1.10  2000/05/26 21:37:33  jas
// Labels are now created and accessed using Singleton class MPMLabel.
//
// Revision 1.9  2000/05/26 18:15:11  guilkey
// Brought the CompNeoHook constitutive model up to functionality
// with the UCF.  Also, cleaned up all of the working models to
// rid them of the SAMRAI crap.
//
// Revision 1.8  2000/05/11 20:10:13  dav
// adding MPI stuff.  The biggest change is that old_dws cannot be const and so a large number of declarations had to change.
//
// Revision 1.7  2000/05/07 06:02:03  sparker
// Added beginnings of multiple patch support and real dependencies
//  for the scheduler
//
// Revision 1.6  2000/04/26 06:48:14  sparker
// Streamlined namespaces
//
// Revision 1.5  2000/04/25 18:42:33  jas
// Revised the factory method and constructor to take a ProblemSpec argument
// to create a new constitutive model.
//
// Revision 1.4  2000/04/19 21:15:54  jas
// Changed BoundedArray to vector<double>.  More stuff to compile.  Critical
// functions that need access to data warehouse still have WONT_COMPILE_YET
// around the methods.
//
// Revision 1.3  2000/04/14 17:34:42  jas
// Added ProblemSpecP capabilities.
//
// Revision 1.2  2000/03/20 17:17:07  sparker
// Made it compile.  There are now several #idef WONT_COMPILE_YET statements.
//
// Revision 1.1  2000/03/14 22:11:47  jas
// Initial creation of the constitutive model directory with the legacy
// constitutive model classes.
//
// Revision 1.1  2000/02/24 06:11:53  sparker
// Imported homebrew code
//
// Revision 1.1  2000/01/24 22:48:48  sparker
// Stuff may actually work someday...
//
// Revision 1.8  1999/12/17 22:05:22  guilkey
// Changed all constitutive models to take in velocityGradient and dt as
// arguments.  This allowed getting rid of velocityGradient as stored data
// in the constitutive model.  Also, in all hyperelastic models,
// deformationGradientInc was also removed from the private data.
//
// Revision 1.7  1999/11/17 22:26:35  guilkey
// Added guts to computeStrainEnergy functions for CompNeoHook CompNeoHookPlas
// and CompMooneyRivlin.  Also, made the computeStrainEnergy function non consted
// for all models.
//
// Revision 1.6  1999/11/17 20:08:46  guilkey
// Added a computeStrainEnergy function to each constitutive model
// so that we can have a valid strain energy calculation for functions
// other than the Elastic Model.  This is called from printParticleData.
// Currently, only the ElasticConstitutiveModel version gives the right
// answer, but that was true before as well.  The others will be filled in.
//
// Revision 1.5  1999/10/14 16:13:04  guilkey
// Added bElBar to the packStream function
//
// Revision 1.4  1999/09/22 22:49:02  guilkey
// Added data to the pack/unpackStream functions to get the proper data into the
// ghost cells.
//
// Revision 1.3  1999/09/10 19:08:37  guilkey
// Added bElBar to the copy constructor
//
// Revision 1.2  1999/06/18 05:44:51  cgl
// - Major work on the make environment for smpm.  See doc/smpm.make
// - fixed getSize(), (un)packStream() for all constitutive models
//   and Particle so that size reported and packed amount are the same.
// - Added infomation to Particle.packStream().
// - fixed internal force summation equation to keep objects from exploding.
// - speed up interpolateParticlesToPatchData()
// - Changed lists of Particles to lists of Particle*s.
// - Added a command line option for smpm `-c npatch'.  Valid values are 1 2 4
//
// Revision 1.1  1999/06/14 06:23:38  cgl
// - src/mpm/Makefile modified to work for IRIX64 or Linux
// - src/grid/Grid.cc added length to character array, since it
// 	was only 4 long, but was being sprintf'd with a 4 character
// 	number, leaving no room for the terminating 0.
// - added smpm directory. to house the samrai version of mpm.
//
// Revision 1.3  1999/05/31 19:36:12  cgl
// Work in stand-alone version of MPM:
//
// - Added materials_dat.cc in src/constitutive_model to generate the
//   materials.dat file for preMPM.
// - Eliminated references to ConstitutiveModel in Grid.cc and GeometryObject.cc
//   Now only Particle and Material know about ConstitutiveModel.
// - Added reads/writes of Particle start and restart information as member
//   functions of Particle
// - "part.pos" now has identicle format to the restart files.
//   mpm.cc modified to take advantage of this.
//
// Revision 1.2  1999/05/30 02:10:47  cgl
// The stand-alone version of ConstitutiveModel and derived classes
// are now more isolated from the rest of the code.  A new class
// ConstitutiveModelFactory has been added to handle all of the
// switching on model type.  Between the ConstitutiveModelFactory
// class functions and a couple of new virtual functions in the
// ConstitutiveModel class, new models can be added without any
// source modifications to any classes outside of the constitutive_model
// directory.  See csafe/Uintah/src/CD/src/constitutive_model/HOWTOADDANEWMODEL
// for updated details on how to add a new model.
//
// --cgl
//
// Revision 1.1  1999/04/10 00:12:07  guilkey
// Compressible Neo-Hookean hyperelastic constitutive model
//
