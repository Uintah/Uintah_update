#include <Core/Datatypes/GLVolumeRenderer.h>
#include <GL/gl.h>

namespace SCIRun {



GLAttenuate::GLAttenuate(const GLVolumeRenderer* glvr) :
  GLTexRenState( glvr )
{
}

void GLAttenuate::preDraw()
{
  glEnable(GL_BLEND);
  glBlendEquation(GL_FUNC_ADD_EXT);
  glBlendFunc(GL_CONSTANT_ALPHA_EXT, GL_ONE);
  glBlendColor(1.f, 1.f, 1.f, 1.f/volren->slices);
}

void GLAttenuate::postDraw()
{
  glDisable(GL_BLEND);
}

} // End namespace SCIRun
