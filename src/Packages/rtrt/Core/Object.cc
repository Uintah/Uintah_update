
#include <Packages/rtrt/Core/Object.h>
#include <Packages/rtrt/Core/Array1.h>
#include <Packages/rtrt/Core/Ray.h>
#include <Packages/rtrt/Core/UVPlane.h>
#include <iostream>
#include <Packages/rtrt/Core/HitInfo.h>
#include <Packages/rtrt/Core/Material.h>

using namespace rtrt;
using namespace SCIRun;

static UVPlane default_mapping(Point(0,0,0), Vector(1,0,0), Vector(0,1,0));
// initialize the static member type_id
PersistentTypeID Object::type_id("Object", "Persistent", 0);

Object::Object(Material* matl, UVMapping* uv)
  : matl(matl), uv(uv)
{
  if(!uv)
    this->uv=&default_mapping;
}

Object::~Object()
{
}

void Object::animate(double, bool&)
{
}

void Object::collect_prims(Array1<Object*>& prims)
{
  prims.add(this);
}

void Object::preprocess(double, int&, int& /*scratchsize*/)
{
  //  scratchsize=Max(scratchsize, matl->get_scratchsize());
}

void Object::print(ostream& out)
{
  out << "Unknown object: " << this << '\n';
}

void Object::light_intersect(const Ray& ray, HitInfo& hit, Color&,
			     DepthStats* st, PerProcessorContext* ppc)
{
  intersect(ray, hit, st, ppc);
}

void Object::softshadow_intersect(Light*, const Ray& ray,
				  HitInfo& hit, double, Color& atten,
				  DepthStats* st, PerProcessorContext* ppc)
{
  light_intersect(ray, hit, atten, st, ppc);
}

void Object::multi_light_intersect(Light*, const Point& orig,
				   const Array1<Vector>& dirs,
				   const Array1<Color>& attens,
				   double dist,
				   DepthStats* st, PerProcessorContext*)
{
  for(int i=0;i<dirs.size();i++){
    if(attens[i].luminance() != 0){
      Color atten;
      Ray ray(orig, dirs[i]);
      HitInfo hit;
      intersect(ray, hit, st, 0);
      if(hit.was_hit && hit.min_t < dist)
	atten = Color(0,0,0);
      else
	atten=Color(1,1,1);
      attens[i]=atten;
    }
  }
}

const int OBJECT_VERSION = 1;
void 
Object::io(SCIRun::Piostream &str)
{
  str.begin_class("Object", OBJECT_VERSION);
  SCIRun::Pio(str, matl);
  SCIRun::Pio(str, uv);
  str.end_class();
}

namespace SCIRun {
void Pio(SCIRun::Piostream& stream, rtrt::Object*& obj)
{
  SCIRun::Persistent* pobj=obj;
  stream.io(pobj, rtrt::Object::type_id);
  if(stream.reading()) {
    obj=dynamic_cast<rtrt::Object*>(pobj);
    ASSERT(obj != 0)
  }
}
} // end namespace SCIRun
