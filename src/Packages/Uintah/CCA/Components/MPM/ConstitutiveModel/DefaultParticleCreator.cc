#include "DefaultParticleCreator.h"
#include <Packages/Uintah/Core/Grid/ParticleSet.h>
#include <Packages/Uintah/Core/Grid/Patch.h>
#include <Packages/Uintah/CCA/Ports/DataWarehouse.h>
#include <Packages/Uintah/CCA/Components/MPM/MPMLabel.h>
#include <Packages/Uintah/Core/Grid/ParticleVariable.h>
#include <Packages/Uintah/Core/Grid/GeometryPiece.h>
#include <Packages/Uintah/CCA/Components/MPM/GeometrySpecification/GeometryObject.h>
#include <Packages/Uintah/Core/Grid/Box.h>
#include <Packages/Uintah/Core/Grid/CellIterator.h>
#include <Packages/Uintah/CCA/Components/MPM/ConstitutiveModel/MPMMaterial.h>

using namespace Uintah;


DefaultParticleCreator::DefaultParticleCreator(MPMMaterial* matl, MPMLabel* lb,
					       int n8or27) 
  : ParticleCreator(matl,lb,n8or27)
{
}

DefaultParticleCreator::~DefaultParticleCreator()
{
}

ParticleSubset* 
DefaultParticleCreator::createParticles(MPMMaterial* matl,
					particleIndex numParticles,
					CCVariable<short int>& cellNAPID,
					const Patch* patch,
					DataWarehouse* new_dw,
					MPMLabel* lb,
					vector<GeometryObject*>& d_geom_objs)
{

  ParticleSubset* subset = ParticleCreator::createParticles(matl,numParticles,
							    cellNAPID,patch,
							    new_dw,lb,
							    d_geom_objs);

  return subset;
  
}

particleIndex 
DefaultParticleCreator::countParticles(const Patch* patch,
				       vector<GeometryObject*>& d_geom_objs) const
{

  return ParticleCreator::countParticles(patch,d_geom_objs);
}

particleIndex 
DefaultParticleCreator::countParticles(GeometryObject* obj,
				       const Patch* patch) const
{

  return ParticleCreator::countParticles(obj,patch);
}


