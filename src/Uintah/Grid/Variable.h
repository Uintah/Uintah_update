#ifndef UINTAH_HOMEBREW_Variable_H
#define UINTAH_HOMEBREW_Variable_H

namespace Uintah {
   class TypeDescription;

   /**************************************
     
     CLASS
       Variable
      
       Short Description...
      
     GENERAL INFORMATION
      
       Variable.h
      
       Steven G. Parker
       Department of Computer Science
       University of Utah
      
       Center for the Simulation of Accidental Fires and Explosions (C-SAFE)
      
       Copyright (C) 2000 SCI Group
      
     KEYWORDS
       Variable
      
     DESCRIPTION
       Long description...
      
     WARNING
      
     ****************************************/
    
   class Variable {
   public:
      virtual const TypeDescription* virtualGetTypeDescription() const = 0;

   protected:
      Variable();
      virtual ~Variable();
   private:
      Variable(const Variable&);
      Variable& operator=(const Variable&);
      
   };
} // end namespace Uintah

//
// $Log$
// Revision 1.1  2000/07/27 22:39:51  sparker
// Implemented MPIScheduler
// Added associated support
//
//

#endif

