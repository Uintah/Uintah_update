
/*
 *  GlobusError.h: Errors due to globus calls
 *
 *  Written by:
 *   Steven G. Parker
 *   Department of Computer Science
 *   University of Utah
 *   July 1999
 *
 *  Copyright (C) 1999 SCI Group
 */

#ifndef Core/CCA/Component_PIDL_GlobusError_h
#define Core/CCA/Component_PIDL_GlobusError_h

#include <Core/CCA/Component/PIDL/PIDLException.h>
#include <string>

namespace SCIRun {
/**************************************
 
CLASS
   GlobusError
   
KEYWORDS
   Exception, Error, globus, PIDL
   
DESCRIPTION
   Exception class for globus functions.  An unhandled negative return
   code from a globus function will get mapped to this exception.  The
   message is a description of the call, and the code is the result
   returned from globus.

****************************************/
	class GlobusError : public PIDLException {
	public:
	    //////////
	    // Construct the exception with the given reason and the
	    // return code from globus
	    GlobusError(const std::string& msg, int code);

	    //////////
	    // Copy ctor
	    GlobusError(const GlobusError&);

	    //////////
	    // Destructor
	    virtual ~GlobusError();

	    //////////
	    // An explanation message, containing the msg string and the
	    // return code passed into the constructor.
	    const char* message() const;

	    //////////
	    // The name of this class
	    const char* type() const;
	protected:
	private:
	    //////////
	    // The explanation string (usually the name of the offending
	    // call).
	    std::string d_msg;

	    //////////
	    // The globus error code.
	    int d_code;

	    GlobusError& operator=(const GlobusError&);
	};
} // End namespace SCIRun

#endif

