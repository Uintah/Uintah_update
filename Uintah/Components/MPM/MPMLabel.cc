#include <Uintah/Components/MPM/MPMLabel.h>
#include <Uintah/Grid/ParticleVariable.h>
#include <Uintah/Grid/CCVariable.h>
#include <Uintah/Grid/NCVariable.h>
#include <Uintah/Grid/VarLabel.h>
#include <Uintah/Components/MPM/Util/Matrix3.h>
#include <Uintah/Grid/VarTypes.h>
#include <Uintah/Grid/PerPatch.h>

using namespace Uintah;
using namespace Uintah::MPM;

MPMLabel::MPMLabel()
{
  // Particle Variables
  pDeformationMeasureLabel = new VarLabel("p.deformationMeasure",
			ParticleVariable<Matrix3>::getTypeDescription());
  
  pStressLabel = new VarLabel( "p.stress",
			ParticleVariable<Matrix3>::getTypeDescription() );
  
  pVolumeLabel = new VarLabel( "p.volume",
			ParticleVariable<double>::getTypeDescription());
  
  pVolumeDeformedLabel = new VarLabel( "p.volumedeformed",
			ParticleVariable<double>::getTypeDescription());
  
  pMassLabel = new VarLabel( "p.mass",
			ParticleVariable<double>::getTypeDescription() );
  
  pVelocityLabel = new VarLabel( "p.velocity", 
			ParticleVariable<Vector>::getTypeDescription() );
  
  pExternalForceLabel = new VarLabel( "p.externalforce",
			ParticleVariable<Vector>::getTypeDescription() );
  
  pXLabel = new VarLabel( "p.x", ParticleVariable<Point>::getTypeDescription(),
			VarLabel::PositionVariable);
  
  pTemperatureLabel = new VarLabel( "p.temperature",
			ParticleVariable<double>::getTypeDescription() );
  
  pTemperatureGradientLabel = new VarLabel( "p.temperatureGradient",
			ParticleVariable<Vector>::getTypeDescription() );

  pTemperatureRateLabel  = new VarLabel( "p.temperatureRate",
			ParticleVariable<double>::getTypeDescription() );

  pExternalHeatRateLabel = new VarLabel( "p.externalHeatRate",
			ParticleVariable<double>::getTypeDescription() );
  
  //tan:
  //  pSurfaceNormalLabel is used to define the surface normal of a
  //  boundary particle.
  //  For the interior particle, the p.surfaceNormal vector is set to (0,0,0)
  //  in this way we can distinguish boundary particles to interior particles
  //
  pSurfLabel = new VarLabel( "p.surface",
			ParticleVariable<int>::getTypeDescription() );

  pSurfaceNormalLabel = new VarLabel( "p.surfaceNormal",
			ParticleVariable<Vector>::getTypeDescription() );

  pParticleIDLabel = new VarLabel("p.particleID",
			ParticleVariable<long>::getTypeDescription() );

  pIsIgnitedLabel  = new VarLabel( "p.isIgnited",
			ParticleVariable<int>::getTypeDescription() );
  
  pMassRateLabel  = new VarLabel( "p.massRate",
			ParticleVariable<double>::getTypeDescription() );
  
  // Particle Variables 
  pDeformationMeasureLabel_preReloc = new VarLabel("p.deformationMeasure+",
			ParticleVariable<Matrix3>::getTypeDescription());
  
  pStressLabel_preReloc = new VarLabel( "p.stress+",
			ParticleVariable<Matrix3>::getTypeDescription() );
  
  pVolumeLabel_preReloc = new VarLabel( "p.volume+",
			ParticleVariable<double>::getTypeDescription());
  
  pMassLabel_preReloc = new VarLabel( "p.mass+",
			ParticleVariable<double>::getTypeDescription() );
  
  pVelocityLabel_preReloc = new VarLabel( "p.velocity+", 
			ParticleVariable<Vector>::getTypeDescription() );
  
  pExternalForceLabel_preReloc = new VarLabel( "p.externalforce+",
			ParticleVariable<Vector>::getTypeDescription() );
  
  pXLabel_preReloc = new VarLabel( "p.x+", ParticleVariable<Point>::getTypeDescription(),
			VarLabel::PositionVariable);
  
  pTemperatureLabel_preReloc = new VarLabel( "p.temperature+",
			ParticleVariable<double>::getTypeDescription() );
  
  pTemperatureGradientLabel_preReloc = new VarLabel( "p.temperatureGradient+",
			ParticleVariable<Vector>::getTypeDescription() );

  pTemperatureRateLabel_preReloc  = new VarLabel( "p.temperatureRate+",
			ParticleVariable<double>::getTypeDescription() );

  pExternalHeatRateLabel_preReloc = new VarLabel( "p.externalHeatRate+",
			ParticleVariable<double>::getTypeDescription() );
  
  //tan:
  //  pSurfaceNormalLabel is used to define the surface normal of a
  //  boundary particle.
  //  For the interior particle, the p.surfaceNormal vector is set to (0,0,0)
  //  in this way we can distinguish boundary particles to interior particles
  //
  pSurfLabel_preReloc = new VarLabel( "p.surface+",
			ParticleVariable<int>::getTypeDescription() );

  pSurfaceNormalLabel_preReloc = new VarLabel( "p.surfaceNormal+",
			ParticleVariable<Vector>::getTypeDescription() );

  pParticleIDLabel_preReloc = new VarLabel("p.particleID+",
			ParticleVariable<long>::getTypeDescription() );

  pIsIgnitedLabel_preReloc  = new VarLabel( "p.isIgnited+",
			ParticleVariable<int>::getTypeDescription() );
  
  pMassRateLabel_preReloc  = new VarLabel( "p.massRate+",
			ParticleVariable<double>::getTypeDescription() );
  
  // Node Centered Variables
  
  gAccelerationLabel = new VarLabel( "g.acceleration",
			NCVariable<Vector>::getTypeDescription() );
  
  gMomExedAccelerationLabel = new VarLabel( "g.momexedacceleration",
			NCVariable<Vector>::getTypeDescription() );
  
  gMassLabel = new VarLabel( "g.mass",
			NCVariable<double>::getTypeDescription() );
  
  gVelocityLabel = new VarLabel( "g.velocity",
			NCVariable<Vector>::getTypeDescription() );
  
  gMomExedVelocityLabel = new VarLabel( "g.momexedvelocity",
			NCVariable<Vector>::getTypeDescription() );
  
  gExternalForceLabel = new VarLabel( "g.externalforce",
			NCVariable<Vector>::getTypeDescription() );
  
  gInternalForceLabel = new VarLabel( "g.internalforce",
			NCVariable<Vector>::getTypeDescription() );
  
  gVelocityStarLabel = new VarLabel( "g.velocity_star",
			NCVariable<Vector>::getTypeDescription() );
  
  gMomExedVelocityStarLabel = new VarLabel( "g.momexedvelocity_star",
			NCVariable<Vector>::getTypeDescription() );
  
  gSelfContactLabel = new VarLabel( "g.selfContact",
			NCVariable<bool>::getTypeDescription() );
  
  gTemperatureLabel = new VarLabel("g.temperature",
			NCVariable<double>::getTypeDescription());

  gTemperatureRateLabel = new VarLabel("g.temperatureRate",
			NCVariable<double>::getTypeDescription());

  gInternalHeatRateLabel = new VarLabel("g.internalHeatRate",
			NCVariable<double>::getTypeDescription());

  gExternalHeatRateLabel = new VarLabel("g.externalHeatRate",
			NCVariable<double>::getTypeDescription());

  // Cell centered variables

  cSelfContactLabel = new VarLabel( "c.selfContact",
			CCVariable<bool>::getTypeDescription() );
  
  cSurfaceNormalLabel = new VarLabel( "c.surfaceNormalLabel",
			CCVariable<Vector>::getTypeDescription() );
  
  cBurnedMassLabel = new VarLabel( "c.burnedMass",
			CCVariable<double>::getTypeDescription() );

  // Reduction variables

  delTLabel = new VarLabel( "delT", delt_vartype::getTypeDescription() );

  StrainEnergyLabel = new VarLabel( "StrainEnergy",
			sum_vartype::getTypeDescription() );

  KineticEnergyLabel = new VarLabel( "KineticEnergy",
			sum_vartype::getTypeDescription() );

  TotalMassLabel = new VarLabel( "TotalMass",
				 sum_vartype::getTypeDescription() );

  // PerPatch variables

  ppNAPIDLabel = new VarLabel("NAPID",PerPatch<long>::getTypeDescription() );

} 

const MPMLabel* MPMLabel::getLabels()
{
  static MPMLabel* instance=0;
  if(!instance)
    instance=new MPMLabel();
  return instance;
}

// $Log$
// Revision 1.14  2000/06/23 21:18:34  tan
// Added pExternalHeatRateLabel for heat conduction.
//
// Revision 1.13  2000/06/23 20:56:14  tan
// Fixed mistakes in label names of gInternalHeatRateLabel and
// gExternalHeatRateLabel.
//
// Revision 1.12  2000/06/23 20:02:26  tan
// Create pTemperatureLabel in the MPMLable constructor.
//
// Revision 1.11  2000/06/16 23:23:35  guilkey
// Got rid of pVolumeDeformedLabel_preReloc to fix some confusion
// the scheduler was having.
//
// Revision 1.10  2000/06/15 21:57:00  sparker
// Added multi-patch support (bugzilla #107)
// Changed interface to datawarehouse for particle data
// Particles now move from patch to patch
//
// Revision 1.9  2000/06/13 23:06:38  guilkey
// Added a CCVariable for the burned particle mass to go into.
//
// Revision 1.8  2000/06/08 16:56:51  guilkey
// Added tasks and VarLabels for HE burn model stuff.
//
// Revision 1.7  2000/06/03 05:25:43  sparker
// Added a new for pSurfLabel (was uninitialized)
// Uncommented pleaseSaveIntegrated
// Minor cleanups of reduction variable use
// Removed a few warnings
//
// Revision 1.6  2000/06/02 23:16:32  guilkey
// Added ParticleID labels.
//
// Revision 1.5  2000/05/31 22:15:38  guilkey
// Added VarLabels for some integrated quantities.
//
// Revision 1.4  2000/05/31 16:30:57  guilkey
// Tidied the file up, added cvs logging.
//
