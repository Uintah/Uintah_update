
#ifndef TEXTUREDTRI2_H
#define TEXTUREDTRI2_H 1

#include <Packages/rtrt/Core/TexturedTri.h>

namespace rtrt {

class TexturedTri2 : public TexturedTri
{

 protected:

  Transform tex_trans_;

 public:

  TexturedTri2(Material *m, const Point &p0, const Point &p1, const Point &p2)
    : TexturedTri(m,p0,p1,p2) { tex_trans_.load_identity(); }
  virtual ~TexturedTri2() {}

  const translate_tex(const Vector &v) { tex_trans_.pre_translate(v); }

  const scale_tex(double x, double y, double z) 
    { tex_trans_.pre_scale( Vector(x,y,z) ); }
    
  void uv(UV& uv, const Point&, const HitInfo& hit)
  {
    Point tp = t1+((ntu*((double*)hit.scratchpad)[1])+
                   (ntv*((double*)hit.scratchpad)[0]));

    Point xtp = tex_trans_.project(tp);
    
    uv.set(xtp.x(),xtp.y());
  } 
};

}

#endif
