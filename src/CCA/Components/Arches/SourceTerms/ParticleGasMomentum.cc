#include <CCA/Components/Arches/SourceTerms/ParticleGasMomentum.h>
#include <CCA/Components/Arches/TransportEqns/DQMOMEqnFactory.h>
#include <CCA/Components/Arches/CoalModels/CoalModelFactory.h>
#include <CCA/Components/Arches/CoalModels/ModelBase.h>
#include <CCA/Components/Arches/CoalModels/ParticleVelocity.h>
#include <Core/ProblemSpec/ProblemSpec.h>
#include <CCA/Ports/Scheduler.h>
#include <Core/Grid/SimulationState.h>
#include <Core/Grid/Variables/VarTypes.h>
#include <Core/Grid/Variables/CCVariable.h>

//===========================================================================

using namespace std;
using namespace Uintah; 

//---------------------------------------------------------------------------
// Builder:
ParticleGasMomentumBuilder::ParticleGasMomentumBuilder(std::string srcName, 
                                         vector<std::string> reqLabelNames, 
                                         SimulationStateP& sharedState)
: SourceTermBuilder(srcName, reqLabelNames, sharedState)
{}

ParticleGasMomentumBuilder::~ParticleGasMomentumBuilder(){}

SourceTermBase*
ParticleGasMomentumBuilder::build(){
  return scinew ParticleGasMomentum( d_srcName, d_sharedState, d_requiredLabels );
}
// End Builder
//---------------------------------------------------------------------------

ParticleGasMomentum::ParticleGasMomentum( std::string srcName, 
                                          SimulationStateP& sharedState,
                                          vector<std::string> reqLabelNames ) 
: SourceTermBase(srcName, sharedState, reqLabelNames)
{
  d_srcLabel = VarLabel::create(srcName, CCVariable<Vector>::getTypeDescription()); 
}

ParticleGasMomentum::~ParticleGasMomentum()
{}
//---------------------------------------------------------------------------
// Method: Problem Setup
//---------------------------------------------------------------------------
void 
ParticleGasMomentum::problemSetup(const ProblemSpecP& inputdb)
{
  //ProblemSpecP db = inputdb; 
}
//---------------------------------------------------------------------------
// Method: Schedule the calculation of the source term 
//---------------------------------------------------------------------------
void 
ParticleGasMomentum::sched_computeSource( const LevelP& level, SchedulerP& sched, int timeSubStep )
{ 
  std::string taskname = "ParticleGasMomentum::computeSource";
  Task* tsk = scinew Task(taskname, this, &ParticleGasMomentum::computeSource, timeSubStep);

  if (timeSubStep == 0 && !d_labelSchedInit) {
    // Every source term needs to set this flag after the varLabel is computed. 
    // transportEqn.cleanUp should reinitialize this flag at the end of the time step. 
    d_labelSchedInit = true;
  }

  if( timeSubStep == 0 ) {
    tsk->computes(d_srcLabel);
  } else {
    tsk->modifies(d_srcLabel); 
  }

  CoalModelFactory& coal_model_factory = CoalModelFactory::self(); 

  // only require weight eqn and particle velocity labels 
  // if there is actually a particle velocity model
  if( coal_model_factory.useParticleVelocityModel() ) {
 
    DQMOMEqnFactory& dqmom_factory  = DQMOMEqnFactory::self(); 
    for (int iqn = 0; iqn < dqmom_factory.get_quad_nodes(); iqn++){
      ParticleVelocity* vel_model = coal_model_factory.getParticleVelocityModel( iqn );
      tsk->requires( Task::NewDW, vel_model->getGasSourceLabel(), Ghost::None, 0 );
    }
  }
  
  sched->addTask(tsk, level->eachPatch(), d_sharedState->allArchesMaterials());

}
//---------------------------------------------------------------------------
// Method: Actually compute the source term 
//---------------------------------------------------------------------------
void
ParticleGasMomentum::computeSource( const ProcessorGroup* pc, 
                                    const PatchSubset* patches, 
                                    const MaterialSubset* matls, 
                                    DataWarehouse* old_dw, 
                                    DataWarehouse* new_dw, 
                                    int timeSubStep )
{
  //patch loop
  for (int p=0; p < patches->size(); p++){

    //Ghost::GhostType  gaf = Ghost::AroundFaces;
    //Ghost::GhostType  gac = Ghost::AroundCells;
    Ghost::GhostType  gn  = Ghost::None;

    const Patch* patch = patches->get(p);
    int archIndex = 0;
    int matlIndex = d_sharedState->getArchesMaterial(archIndex)->getDWIndex(); 
    
    CCVariable<Vector> dragSrc; 
    if( timeSubStep == 0 ) {
      new_dw->allocateAndPut( dragSrc, d_srcLabel, matlIndex, patch );
    } else {
      new_dw->getModifiable( dragSrc, d_srcLabel, matlIndex, patch ); 
    }
    dragSrc.initialize(Vector(0.,0.,0.));


    CoalModelFactory& coal_model_factory = CoalModelFactory::self(); 

    if( coal_model_factory.useParticleVelocityModel() ) {

      DQMOMEqnFactory& dqmom_factory  = DQMOMEqnFactory::self(); 
      int numEnvironments = dqmom_factory.get_quad_nodes();
 
      // vector holding drag model constCCVariables
      vector< constCCVariable<Vector>* > dragCCVars(numEnvironments);

      // populate vector holding drag model constCCVariables
      for( int iqn=0; iqn < numEnvironments; ++iqn ) {
        dragCCVars[iqn] = scinew constCCVariable<Vector>;
        ParticleVelocity* vel_model = coal_model_factory.getParticleVelocityModel( iqn );
        new_dw->get( *(dragCCVars[iqn]), vel_model->getGasSourceLabel(), matlIndex, patch, gn, 0);
      }

      // Now add the source terms for each quadrature node together
      for (CellIterator iter=patch->getCellIterator(); !iter.done(); iter++){
        IntVector c = *iter; 

        Vector running_sum = Vector(0.0, 0.0, 0.0);
        for (vector< constCCVariable<Vector>* >::iterator iDrag = dragCCVars.begin();
             iDrag != dragCCVars.end(); ++iDrag ) {
          running_sum += (**iDrag)[c];
        }

        dragSrc[c] = running_sum;

        /*
        //cmr
        if( c == IntVector(1,2,3) ) {
          cout << "The drag source term is " << dragSrc[c] << endl;
        }
        */

      }

      // now delete CCVariables created on the stack with the "scinew" operator
      for( vector< constCCVariable<Vector>* >::iterator ii = dragCCVars.begin(); ii != dragCCVars.end(); ++ii ) {
        delete *ii;
      }

    }


  }
}
//---------------------------------------------------------------------------
// Method: Schedule dummy initialization
//---------------------------------------------------------------------------
void
ParticleGasMomentum::sched_dummyInit( const LevelP& level, SchedulerP& sched )
{
  string taskname = "ParticleGasMomentum::dummyInit"; 

  Task* tsk = scinew Task(taskname, this, &ParticleGasMomentum::dummyInit);

  tsk->computes(d_srcLabel);

  for (std::vector<const VarLabel*>::iterator iter = d_extraLocalLabels.begin(); iter != d_extraLocalLabels.end(); iter++){
    tsk->computes(*iter); 
  }

  sched->addTask(tsk, level->eachPatch(), d_sharedState->allArchesMaterials());

}
void 
ParticleGasMomentum::dummyInit( const ProcessorGroup* pc, 
                            const PatchSubset* patches, 
                            const MaterialSubset* matls, 
                            DataWarehouse* old_dw, 
                            DataWarehouse* new_dw )
{
  //patch loop
  for (int p=0; p < patches->size(); p++){

    const Patch* patch = patches->get(p);
    int archIndex = 0;
    int matlIndex = d_sharedState->getArchesMaterial(archIndex)->getDWIndex(); 

    CCVariable<double> src;

    new_dw->allocateAndPut( src, d_srcLabel, matlIndex, patch ); 

    src.initialize(0.0); 

    for (std::vector<const VarLabel*>::iterator iter = d_extraLocalLabels.begin(); iter != d_extraLocalLabels.end(); iter++){
      CCVariable<double> tempVar; 
      new_dw->allocateAndPut(tempVar, *iter, matlIndex, patch ); 
    }
  }
}




