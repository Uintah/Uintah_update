#include <Packages/Uintah/CCA/Components/MPMICE/MPMICELabel.h>
#include <Packages/Uintah/Core/Math/Matrix3.h>
#include <Packages/Uintah/Core/Grid/ParticleVariable.h>
#include <Packages/Uintah/Core/Grid/CCVariable.h>
#include <Packages/Uintah/Core/Grid/NCVariable.h>
#include <Packages/Uintah/Core/Grid/SFCXVariable.h>
#include <Packages/Uintah/Core/Grid/SFCYVariable.h>
#include <Packages/Uintah/Core/Grid/SFCZVariable.h>
#include <Packages/Uintah/Core/Grid/VarLabel.h>
#include <Packages/Uintah/Core/Grid/VarTypes.h>
#include <Packages/Uintah/Core/Grid/PerPatch.h>
#include <Core/Malloc/Allocator.h>

using namespace SCIRun;
using namespace Uintah;

MPMICELabel::MPMICELabel()
{
  // Cell centered variables
  cMassLabel    = VarLabel::create( "c.mass",
			CCVariable<double>::getTypeDescription() );
  cVolumeLabel  = VarLabel::create( "c.volume",
			CCVariable<double>::getTypeDescription() );
  vel_CCLabel   = VarLabel::create( "vel_CC",
			CCVariable<Vector>::getTypeDescription() );
  velstar_CCLabel  = VarLabel::create( "velstar_CC",
			CCVariable<Vector>::getTypeDescription() );
  dvdt_CCLabel     = VarLabel::create( "dvdt_CC",
			CCVariable<Vector>::getTypeDescription() );
  dTdt_CCLabel     = VarLabel::create( "dTdt_CC",
			CCVariable<double>::getTypeDescription() );
  temp_CCLabel     = VarLabel::create("temp_CC",
			CCVariable<double>::getTypeDescription() );
  temp_CC_scratchLabel = VarLabel::create("temp_CC_scratch",
			CCVariable<double>::getTypeDescription() );
  press_NCLabel      = VarLabel::create("pressureNC",
			NCVariable<double>::getTypeDescription());

  velInc_CCLabel     = VarLabel::create("velIncCC",
			CCVariable<Vector>::getTypeDescription());
  velInc_NCLabel     = VarLabel::create("velIncNC",
			NCVariable<Vector>::getTypeDescription());

  burnedMassCCLabel   = VarLabel::create("burnedMass",
			CCVariable<double>::getTypeDescription());  
  releasedHeatCCLabel = VarLabel::create("releasedHeat",
			CCVariable<double>::getTypeDescription());
  sumBurnedMassLabel  = VarLabel::create("sumBurnedMass",
			CCVariable<double>::getTypeDescription());  
  sumReleasedHeatLabel = VarLabel::create("sumReleasedHeat",
			CCVariable<double>::getTypeDescription());
  sumCreatedVolLabel   = VarLabel::create("sumCreateVol",
                        CCVariable<double>::getTypeDescription());


} 

MPMICELabel::~MPMICELabel()
{
  delete cMassLabel;
  delete cVolumeLabel;
  delete vel_CCLabel;
  delete velstar_CCLabel;
  delete dvdt_CCLabel;
  delete dTdt_CCLabel;
  delete temp_CCLabel;
  delete temp_CC_scratchLabel;
  delete press_NCLabel;
  delete velInc_CCLabel;
  delete velInc_NCLabel;
  delete burnedMassCCLabel;
  delete releasedHeatCCLabel;
  delete sumBurnedMassLabel;
  delete sumReleasedHeatLabel;
  delete sumCreatedVolLabel;
}
