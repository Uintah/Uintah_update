

#ifndef OBJECT_H
#define OBJECT_H 1

#include <Packages/rtrt/Core/Array1.h>
#include <Packages/rtrt/Core/Color.h>
#include <iostream>

namespace SCIRun {
  class Point;
  class Vector;
}

namespace rtrt {

using SCIRun::Vector;
using SCIRun::Point;

struct DepthStats;

class  HitInfo;
class  Material;
class  Ray;
class  Light;
class  BBox;
class  PerProcessorContext;
class  UVMapping;

template<class T> class Array1;

class Object {
    Material* matl;
    UVMapping* uv;
public:
    Object(Material* matl, UVMapping* uv=0);
    inline Material* get_matl() const {
	return matl;
    }
    virtual Material* get_matl(HitInfo&) const {
	return matl;
    }
    inline void set_matl(Material* new_matl) {
	matl=new_matl;
    }
    inline UVMapping* get_uvmapping() {
	return uv;
    }
    inline void set_uvmapping(UVMapping* uv) {
	this->uv=uv;
    }
    virtual ~Object();
    virtual void intersect(const Ray& ray, HitInfo& hit, DepthStats* st,
			   PerProcessorContext*)=0;
    virtual void light_intersect(Light* light, const Ray& ray,
				 HitInfo& hit, double dist, Color& atten,
				 DepthStats* st, PerProcessorContext* ppc)=0;
    virtual void multi_light_intersect(Light* light, const Point& orig,
				       const Array1<Vector>& dirs,
				       const Array1<Color>& attens,
				       double dist,
				       DepthStats* st, PerProcessorContext* ppc);
    virtual Vector normal(const Point&, const HitInfo& hit)=0;
    virtual void animate(double t, bool& changed);
    virtual void preprocess(double maxradius, int& pp_offset, int& scratchsize);
    virtual void compute_bounds(BBox& bbox, double offset)=0;
    virtual void collect_prims(Array1<Object*>& prims);
    virtual void print(ostream& out);
};

} // end namespace rtrt

#endif
