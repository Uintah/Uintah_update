#ifndef UINTAH_HOMEBREW_OUTPUT_H
#define UINTAH_HOMEBREW_OUTPUT_H

#include <Packages/Uintah/Parallel/Packages/UintahParallelPort.h>
#include <Packages/Uintah/Grid/GridP.h>
#include <Packages/Uintah/Grid/LevelP.h>
#include <Packages/Uintah/Grid/SimulationStateP.h>
#include <Packages/Uintah/Interface/DataWarehouse.h>
#include <Packages/Uintah/Interface/ProblemSpecP.h>
#include <Packages/Uintah/Interface/SchedulerP.h>
#include <Core/OS/Dir.h>
#include <string>

namespace Uintah {

   class ProcessorGroup;
   class Patch;

/**************************************

CLASS
   Output
   
   Short description...

GENERAL INFORMATION

   Output.h

   Steven G. Parker
   Department of Computer Science
   University of Utah

   Center for the Simulation of Accidental Fires and Explosions (C-SAFE)
  
   Copyright (C) 2000 SCI Group

KEYWORDS
   Output

DESCRIPTION
   Long description...
  
WARNING
  
****************************************/

   class Output : public Packages/UintahParallelPort {
   public:
      Output();
      virtual ~Output();
      
      //////////
      // Insert Documentation Here:
      virtual void problemSetup(const ProblemSpecP& params) = 0;

      //////////
      // Insert Documentation Here:
      virtual void finalizeTimestep(double t, double delt, const LevelP&,
				    SchedulerP&,
				    DataWarehouseP&) = 0;

      //////////
      // Insert Documentation Here:
      virtual const std::string getOutputLocation() const = 0;

      //////////
      // Get the current time step
      virtual int getCurrentTimestep() = 0;

      //////////
      // Returns true if the last timestep was one
      // in which data was output.
      virtual bool wasOutputTimestep() = 0;
     
      //////////
      // Get the directory of the current time step for outputting info.
      virtual const std::string& getLastTimestepOutputLocation() const = 0;
   private:
      Output(const Output&);
      Output& operator=(const Output&);
   };
} // End namespace Uintah

#endif
