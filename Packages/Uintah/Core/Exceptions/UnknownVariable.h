

/*
 *  UnknownVariable.h: 
 *
 *  Written by:
 *   Steven G. Parker
 *   Department of Computer Science
 *   University of Utah
 *   April 2000
 *
 *  Copyright (C) 2000 SCI Group
 */

#ifndef Packages/Uintah_Exceptions_UnknownVariable_h
#define Packages/Uintah_Exceptions_UnknownVariable_h

#include <Core/Exceptions/Exception.h>
#include <string>

namespace Uintah {

   class UnknownVariable : public Exception {
   public:
      UnknownVariable(const std::string& varname, const Patch* patch,
		      int matlIndex, const std::string& extramsg = "");
      UnknownVariable(const std::string& varname,
		      const std::string& extramsg);
      UnknownVariable(const UnknownVariable&);
      virtual ~UnknownVariable();
      virtual const char* message() const;
      virtual const char* type() const;
   protected:
   private:
      std::string d_msg;
      UnknownVariable& operator=(const UnknownVariable&);
} // End namespace Uintah
   };

#endif


