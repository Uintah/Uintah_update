
#ifndef UINTAH_HOMEBREW_ParticleVariableBase_H
#define UINTAH_HOMEBREW_ParticleVariableBase_H

#include <Packages/Uintah/Core/Grid/ParticleSubset.h>
#include <Packages/Uintah/Core/Grid/Variable.h>

#include <mpi.h>

#include <vector>

namespace Uintah {

   class OutputContext;
   class ParticleSubset;
   class ParticleSet;
   class Patch;
   class ProcessorGroup;
   class TypeDescription;

/**************************************

CLASS
   ParticleVariableBase
   
   Short description...

GENERAL INFORMATION

   ParticleVariableBase.h

   Steven G. Parker
   Department of Computer Science
   University of Utah

   Center for the Simulation of Accidental Fires and Explosions (C-SAFE)
  
   Copyright (C) 2000 SCI Group

KEYWORDS
   ParticleVariableBase

DESCRIPTION
   Long description...
  
WARNING
  
****************************************/

   class ParticleVariableBase : public Variable {
   public:
      
      virtual ~ParticleVariableBase();
      virtual void copyPointer(const ParticleVariableBase&) = 0;
      
      //////////
      // Insert Documentation Here:
      virtual ParticleVariableBase* clone() const = 0;
      virtual ParticleVariableBase* cloneSubset(ParticleSubset*) const = 0;

      virtual void allocate(const Patch*) = 0; // will throw an InternalError
      virtual void allocate(ParticleSubset*) = 0;
      virtual void gather(ParticleSubset* dest,
			  std::vector<ParticleSubset*> subsets,
			  std::vector<ParticleVariableBase*> srcs,
			  particleIndex extra = 0) = 0;
      virtual void unpackMPI(void* buf, int bufsize, int* bufpos,
			     const ProcessorGroup* pg, int start, int n) = 0;
      virtual void packMPI(void* buf, int bufsize, int* bufpos,
			   const ProcessorGroup* pg, int start, int n) = 0;
      virtual void packsizeMPI(int* bufpos,
			       const ProcessorGroup* pg, int start, int n) = 0;
      virtual void emit(OutputContext&) = 0;

      //////////
      // Insert Documentation Here:
      ParticleSubset* getParticleSubset() const {
	 return d_pset;
      }

      //////////
      // Insert Documentation Here:
      ParticleSet* getParticleSet() const {
	 return d_pset->getParticleSet();
      }
      
      virtual void* getBasePointer() = 0;
      void getMPIBuffer(void*& buf, int& count, MPI_Datatype& datatype,
			bool& free_datatype, ParticleSubset* sendset);
      virtual const TypeDescription* virtualGetTypeDescription() const = 0;
   protected:
      ParticleVariableBase(const ParticleVariableBase&);
      ParticleVariableBase(ParticleSubset* pset);
      ParticleVariableBase& operator=(const ParticleVariableBase&);
      
      ParticleSubset*  d_pset;

   private:
   };

} // End namespace Uintah
   
#endif
