#ifndef FULLRES_H
#define FULLRES_H

#include <Core/Datatypes/GLVolRenState.h>

namespace SCIRun {

/**************************************

CLASS
   FullRes
   
   FullRes Class.

GENERAL INFORMATION

   FullRes.h

   Kurt Zimmerman
   Department of Computer Science
   University of Utah

   Center for the Simulation of Accidental Fires and Explosions (C-SAFE)
  
   Copyright (C) 2000 SCI Group

KEYWORDS
   FullRes

DESCRIPTION
   FullRes class.  Use subclasses to implement the drawing State
   for the VolumeRenderer.
  
WARNING
  
****************************************/

class FullRes : public GLVolRenState  {
public:

  // GROUP: Constructors:
  //////////
  // Constructor
  FullRes(const GLVolumeRenderer* glvr);
  // GROUP: Destructors
  //////////
  // Destructor
  ~FullRes(){}

  // GROUP: Operations
  //////////
  // draw the Volume
  virtual void draw();
  // draw Wireframe
  virtual void drawWireFrame();
  
protected:
  virtual void setAlpha(const Brick& brick);

};

} // End namespace SCIRun
#endif
