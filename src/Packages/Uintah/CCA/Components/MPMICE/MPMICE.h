#ifndef UINTAH_HOMEBREW_MPMICE_H
#define UINTAH_HOMEBREW_MPMICE_H

#include <Packages/Uintah/Core/Parallel/UintahParallelComponent.h>
#include <Packages/Uintah/Core/ProblemSpec/ProblemSpecP.h>
#include <Packages/Uintah/Core/Grid/GridP.h>
#include <Packages/Uintah/Core/Grid/LevelP.h>
#include <Packages/Uintah/Core/Grid/Variables/NCVariable.h>
#include <Packages/Uintah/CCA/Ports/DataWarehouseP.h>
#include <Packages/Uintah/CCA/Ports/SimulationInterface.h>
#include <Packages/Uintah/CCA/Components/ICE/ICEMaterial.h>
#include <Packages/Uintah/CCA/Components/MPM/ConstitutiveModel/MPMMaterial.h>
#include <Packages/Uintah/CCA/Components/MPM/Contact/Contact.h>
#include <Packages/Uintah/CCA/Components/MPM/SerialMPM.h>
#include <Packages/Uintah/CCA/Components/MPM/RigidMPM.h>
#include <Packages/Uintah/CCA/Components/MPM/PhysicalBC/MPMPhysicalBC.h>
#include <Packages/Uintah/CCA/Ports/SwitchingCriteria.h>
#include <Core/Geometry/Vector.h>

#include <Packages/Uintah/CCA/Components/MPMICE/share.h>
namespace Uintah {
  class ICE;
  class ICELabel;
  class MPMLabel;
  class MPMICELabel;
  class Output;

using namespace SCIRun;

/**************************************

CLASS
   MPMICE
   
   Short description...

GENERAL INFORMATION

   MPMICE.h

   Steven G. Parker
   Department of Computer Science
   University of Utah

   Center for the Simulation of Accidental Fires and Explosions (C-SAFE)
  
   Copyright (C) 2000 SCI Group

KEYWORDS
   MPMICE

DESCRIPTION
   Long description...
  
WARNING
  
****************************************/

enum MPMType {STAND_MPMICE = 0, RIGID_MPMICE, SHELL_MPMICE, FRACTURE_MPMICE};

class SCISHARE MPMICE : public SimulationInterface, public UintahParallelComponent {

public:
//  MPMICE(const ProcessorGroup* myworld, const bool doAMR);
  MPMICE(const ProcessorGroup* myworld, MPMType type, const bool doAMR = false);
  virtual ~MPMICE();
  
  virtual bool restartableTimesteps();

  virtual double recomputeTimestep(double current_dt); 
          
  virtual void problemSetup(const ProblemSpecP& params, 
                            const ProblemSpecP& materials_ps, 
                            GridP& grid, SimulationStateP&);

  virtual void outputProblemSpec(ProblemSpecP& ps);
         
  virtual void scheduleInitialize(const LevelP& level,
                                  SchedulerP&);

  virtual void scheduleInitializeAddedMaterial(const LevelP& level,
                                               SchedulerP&);

  virtual void restartInitialize();

  virtual void scheduleComputeStableTimestep(const LevelP& level,
                                             SchedulerP&);
  
  // scheduleTimeAdvance version called by the AMR simulation controller.
  virtual void scheduleTimeAdvance( const LevelP& level, 
				        SchedulerP&);
 
  virtual void scheduleFinalizeTimestep(const LevelP& level, 
                                        SchedulerP&);
                            
  void scheduleInterpolateNCToCC_0(SchedulerP&, 
                                  const PatchSet*,
                                  const MaterialSubset*,
                                  const MaterialSet*);

  void scheduleCoarsenCC_0(SchedulerP&, 
                           const PatchSet*,
                           const MaterialSet*);

  void scheduleCoarsenNCMass(SchedulerP&,
                             const PatchSet*,
                             const MaterialSet*);

  void scheduleComputeLagrangianValuesMPM(SchedulerP&, 
                                          const PatchSet*,
                                          const MaterialSubset*,
                                          const MaterialSet*);

  void scheduleCoarsenLagrangianValuesMPM(SchedulerP&, 
                                          const PatchSet*,
                                          const MaterialSet*);

  void scheduleInterpolateCCToNC(SchedulerP&, const PatchSet*,
                                 const MaterialSet*);

  void scheduleComputeCCVelAndTempRates(SchedulerP&, const PatchSet*,
                                        const MaterialSet*);

  void scheduleRefineCC(SchedulerP&, const PatchSet*,
                        const MaterialSet*);

  void scheduleComputeNonEquilibrationPressure(SchedulerP&, 
                                               const PatchSet*,
                                               const MaterialSubset*,
                                               const MaterialSubset*,
                                               const MaterialSubset*,
                                               const MaterialSet*);

  void scheduleComputePressure(SchedulerP&, 
                               const PatchSet*,
                               const MaterialSubset*,
                               const MaterialSubset*,
                               const MaterialSubset*,
                               const MaterialSet*);


  void scheduleInterpolatePressCCToPressNC(SchedulerP&, 
                                          const PatchSet*,
                                          const MaterialSubset*,
                                          const MaterialSet*);

  void scheduleRefinePressCC(SchedulerP&, 
                                      const PatchSet*,
                                      const MaterialSubset*,
                                      const MaterialSet*);

  void scheduleInterpolatePAndGradP(SchedulerP&, 
                                    const PatchSet*,
                                    const MaterialSubset*,
                                    const MaterialSubset*,
                                    const MaterialSubset*,
                                    const MaterialSet*);
            
  void scheduleComputeInternalForce(SchedulerP&, const PatchSet*,
				    const MaterialSet*);

  void computeInternalForce(const ProcessorGroup*,
			    const PatchSubset* patches,
			    const MaterialSubset* matls,
			    DataWarehouse* old_dw,
			    DataWarehouse* new_dw);

//______________________________________________________________________
//       A C T U A L   S T E P S : 
  void actuallyInitialize(const ProcessorGroup*,
                          const PatchSubset* patch,
                          const MaterialSubset* matls,
                          DataWarehouse*,
                          DataWarehouse* new_dw);
                         

  void actuallyInitializeAddedMPMMaterial(const ProcessorGroup*,
                                          const PatchSubset* patch,
                                          const MaterialSubset* matls,
                                          DataWarehouse*,
                                          DataWarehouse* new_dw);
                         
                                                    
  void interpolateNCToCC_0(const ProcessorGroup*,
                           const PatchSubset* patch,
                           const MaterialSubset* matls,
                           DataWarehouse* old_dw,
                           DataWarehouse* new_dw);
  
  void computeLagrangianValuesMPM(const ProcessorGroup*,
                                  const PatchSubset* patch,
                                  const MaterialSubset* matls,
                                  DataWarehouse* old_dw,
                                  DataWarehouse* new_dw);

  void computeEquilibrationPressure(const ProcessorGroup*,
                                    const PatchSubset* patch,
                                    const MaterialSubset* matls,
                                    DataWarehouse*, 
                                    DataWarehouse*,
                                    const MaterialSubset* press_matl);


  void interpolateCCToNC(const ProcessorGroup*,
                         const PatchSubset* patch,
                         const MaterialSubset* matls,
                         DataWarehouse* old_dw,
                         DataWarehouse* new_dw);

  void computeCCVelAndTempRates(const ProcessorGroup*,
                                const PatchSubset* patch,
                                const MaterialSubset* matls,
                                DataWarehouse* old_dw,
                                DataWarehouse* new_dw);

  void interpolateCCToNCRefined(const ProcessorGroup*,
                                const PatchSubset* patch,
                                const MaterialSubset* matls,
                                DataWarehouse* old_dw,
                                DataWarehouse* new_dw);

  void interpolatePressCCToPressNC(const ProcessorGroup*,
                                   const PatchSubset* patch,
                                   const MaterialSubset* matls,
                                   DataWarehouse* old_dw,
                                   DataWarehouse* new_dw);

  void interpolatePAndGradP(const ProcessorGroup*,
                            const PatchSubset* patch,
                            const MaterialSubset* matls,
                            DataWarehouse* old_dw,
                            DataWarehouse* new_dw);
                            
  void HEChemistry(const ProcessorGroup*,
                     const PatchSubset* patch,
                     const MaterialSubset* matls,
                     DataWarehouse* old_dw,
                     DataWarehouse* new_dw);
  
  void printData( int indx,
                  const Patch* patch, 
                  int   include_EC,
                  const string&    message1,        
                  const string&    message2,  
                  const NCVariable<double>& q_NC);
                  
  void printNCVector(int indx,
                      const Patch* patch, int include_EC,
                     const string&    message1,        
                     const string&    message2, 
                     int     component,
                     const NCVariable<Vector>& q_NC);
                     
  void binaryPressureSearch(StaticArray<constCCVariable<double> >& Temp, 
                            StaticArray<CCVariable<double> >& rho_micro, 
                            StaticArray<CCVariable<double> >& vol_frac, 
                            StaticArray<CCVariable<double> >& rho_CC_new,
                            StaticArray<CCVariable<double> >& speedSound_new,
                            StaticArray<double> & dp_drho, 
                            StaticArray<double> & dp_de, 
                            StaticArray<double> & press_eos,
                            constCCVariable<double> & press,
                            CCVariable<double> & press_new, 
                            double press_ref,
                            StaticArray<constCCVariable<double> > & cv,
                            StaticArray<constCCVariable<double> > & gamma,
                            double convergence_crit,
                            int numALLMatls,
                            int & count,
                            double & sum,
                            IntVector c );                   
//__________________________________
//    R A T E   F O R M                   
  void computeRateFormPressure(const ProcessorGroup*,
                               const PatchSubset* patch,
                               const MaterialSubset* matls,
                               DataWarehouse*, 
                               DataWarehouse*); 

  // MATERIAL ADDITION
  virtual bool needRecompile(double time, double dt, const GridP& grid);

  virtual void addMaterial(const ProblemSpecP& params,
                           GridP& grid,
                           SimulationStateP&);

  virtual void scheduleSwitchTest(const LevelP& level, SchedulerP& sched);

  //__________________________________
  //   AMR
  virtual void scheduleRefineInterface(const LevelP& fineLevel,
                                       SchedulerP& scheduler,
                                       bool needOld, bool needNew);
  
  virtual void scheduleRefine (const PatchSet* patches, 
                               SchedulerP& sched); 
    
  virtual void scheduleCoarsen(const LevelP& coarseLevel, 
                               SchedulerP& sched);

  void refine(const ProcessorGroup*,
              const PatchSubset* patches,
              const MaterialSubset* matls,
              DataWarehouse*,
              DataWarehouse* new_dw);

  virtual void scheduleInitialErrorEstimate(const LevelP& coarseLevel,
                                            SchedulerP& sched);
                                               
  virtual void scheduleErrorEstimate(const LevelP& coarseLevel,
                                     SchedulerP& sched);

  void scheduleRefineVariableCC(SchedulerP& sched,
                                const PatchSet* patches,
                                const MaterialSet* matls,
                                const VarLabel* variable);

  template<typename T>
    void scheduleCoarsenVariableCC(SchedulerP& sched,
                                   const PatchSet* patches,
                                   const MaterialSet* matls,
                                   const VarLabel* variable,
                                   T defaultValue, 
                                   bool modifies,
                                   const string& coarsenMethod);

  template<typename T>
    void scheduleCoarsenVariableNC(SchedulerP& sched,
                                   const PatchSet* patches,
                                   const MaterialSet* matls,
                                   const VarLabel* variable,
                                   T defaultValue,
                                   bool modifies,
                                   string coarsenMethod);

  template<typename T>
    void refineVariableCC(const ProcessorGroup*,
                          const PatchSubset* patch,
                          const MaterialSubset* matls,
                          DataWarehouse* old_dw,
                          DataWarehouse* new_dw,
                          const VarLabel* variable);

  template<typename T>                                 
    void coarsenDriver_std(IntVector cl, 
                           IntVector ch,
                           IntVector refinementRatio,
                           double ratio,
                           const Level* coarseLevel,
                           constCCVariable<T>& fine_q_CC,
                           CCVariable<T>& coarse_q_CC );
  template<typename T>
    void coarsenDriver_stdNC(IntVector cl,
                             IntVector ch,
                             IntVector refinementRatio,
                             double ratio,
                             const Level* coarseLevel,
                             constNCVariable<T>& fine_q_NC,
                             NCVariable<T>& coarse_q_NC );

  template<typename T>
    void coarsenDriver_massWeighted(IntVector cl, 
                                    IntVector ch,
                                    IntVector refinementRatio,
                                    const Level* coarseLevel,
                                    constCCVariable<double>& cMass,
                                    constCCVariable<T>& fine_q_CC,
                                    CCVariable<T>& coarse_q_CC );

  template<typename T>
    void coarsenVariableCC(const ProcessorGroup*,
                           const PatchSubset* patch,
                           const MaterialSubset* matls,
                           DataWarehouse* old_dw,
                           DataWarehouse* new_dw,
                           const VarLabel* variable,
                           T defaultValue, 
                           bool modifies,
                           string coarsenMethod);

  template<typename T>
    void coarsenVariableNC(const ProcessorGroup*,
                           const PatchSubset* patch,
                           const MaterialSubset* matls,
                           DataWarehouse* old_dw,
                           DataWarehouse* new_dw,
                           const VarLabel* variable,
                           T defaultValue,
                           bool modifies,
                           string coarsenMethod);

    void refineCoarseFineInterface(const ProcessorGroup*,
                                   const PatchSubset* patches,
                                   const MaterialSubset*,
                                   DataWarehouse* fine_old_dw,
                                   DataWarehouse* fine_new_dw);

private:
                        
  void printSchedule(const PatchSet* patches,
                     const string& where);
                     
  void printSchedule(const LevelP& level,
                     const string& where); 
                     
  void printTask(const PatchSubset* patches,
                 const Patch* patch,
                 const string& where);                         
     
  enum bctype { NONE=0,
                FIXED,
                SYMMETRY,
                NEIGHBOR };

protected:
  MPMICE(const MPMICE&);
  MPMICE& operator=(const MPMICE&);
  SimulationStateP d_sharedState; 
  Output* dataArchiver;
  MPMLabel* Mlb;
  ICELabel* Ilb;
  MPMICELabel* MIlb;

  bool             d_rigidMPM;
  SerialMPM*       d_mpm;
  ICE*             d_ice;
  int              d_8or27;
  int              NGN;
  bool             d_recompile;
  bool             d_doAMR;
  bool             d_testForNegTemps_mpm;
  bool             do_mlmpmice;

  int              pbx_matl_num;
  MaterialSubset*  pbx_matl;

  SwitchingCriteria* d_switchCriteria;

  vector<MPMPhysicalBC*> d_physicalBCs;
  double d_SMALL_NUM;
  double d_TINY_RHO;
  
  // Debugging switches
  bool switchDebug_InterpolateNCToCC_0;
  bool switchDebug_InterpolateCCToNC;
  bool switchDebug_InterpolatePAndGradP;
};

} // End namespace Uintah
      
#endif
