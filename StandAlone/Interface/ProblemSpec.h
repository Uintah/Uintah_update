#ifndef UINTAH_HOMEBREW_ProblemSpec_H
#define UINTAH_HOMEBREW_ProblemSpec_H

#include <Packages/Uintah/Grid/Handle.h>
#include <Packages/Uintah/Grid/RefCounted.h>
#include <Packages/Uintah/Interface/ProblemSpecP.h>

#include <string>
#include <map>

#ifdef __sgi
#define IRIX
#pragma set woff 1375
#endif
#include <util/PlatformUtils.hpp>
#include <parsers/DOMParser.hpp>
#include <dom/DOM_Node.hpp>
#include <dom/DOM_NamedNodeMap.hpp>
#ifdef __sgi
#pragma reset woff 1375
#endif

using namespace std;

namespace SCIRun {
  class IntVector;
  class Vector;
  class Point;
}

namespace Uintah {

class TypeDescription;

// This is the "base" problem spec.  There should be ways of breaking
// this up

/**************************************

CLASS
   ProblemSpec
   
   Short description...

GENERAL INFORMATION

   ProblemSpec.h

   Steven G. Parker
   Department of Computer Science
   University of Utah

   Center for the Simulation of Accidental Fires and Explosions (C-SAFE)
  
   Copyright (C) 2000 SCI Group

KEYWORDS
   Problem_Specification

DESCRIPTION
   Long description...
  
WARNING
  
****************************************/

   
   class ProblemSpec : public RefCounted {
   public:
      ProblemSpec(const DOM_Node& node, bool doWrite=true);
      virtual ~ProblemSpec();
      
      ProblemSpecP findBlock(const std::string& name) const;
      ProblemSpecP findBlock() const;
      ProblemSpecP findNextBlock(const std::string& name) const;
      ProblemSpecP findNextBlock() const;
      
      std::string getNodeName() const;
      
      void require(const std::string& name, double& value);
      void require(const std::string& name, int& value);
      void require(const std::string& name, bool& value);
      void require(const std::string& name, std::string& value);
      void require(const std::string& name, IntVector& value);
      void require(const std::string& name, Vector& value);
      void require(const std::string& name, Point& value);

   // Get any optional attributes associated with a tag

      void requireOptional(const std::string& name, std::string& value);
      ProblemSpecP getOptional(const std::string& name, std::string& value);

      
      ProblemSpecP get(const std::string& name, double& value);
      ProblemSpecP get(const std::string& name, int& value);
      ProblemSpecP get(const std::string& name, bool& value);
      ProblemSpecP get(const std::string& name, std::string& value);
      ProblemSpecP get(const std::string& name, 
		       IntVector& value);
      ProblemSpecP get(const std::string& name, 
		       Vector& value);
      ProblemSpecP get(const std::string& name, 
		       Point& value);

      void getAttributes(std::map<std::string,std::string>& value);

      static const TypeDescription* getTypeDescription();
      
      DOM_Node getNode() const {
	 return d_node;
      }

      void writeMessages(bool doWrite) {
        d_write = doWrite;
      }

      bool doWriteMessages() const
      { return d_write; }
   private:
      ProblemSpec(const ProblemSpec&);
      ProblemSpec& operator=(const ProblemSpec&);
      
      DOM_Node d_node;
      bool d_write;
   };
   
} // End namespace Uintah

#endif
