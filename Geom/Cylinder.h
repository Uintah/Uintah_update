
/*
 *  Cylinder.h: Cylinder Object
 *
 *  Written by:
 *   Steven G. Parker & David Weinstein
 *   Department of Computer Science
 *   University of Utah
 *   April 1994
 *
 *  Copyright (C) 1994 SCI Group
 */

#ifndef SCI_Geom_Cylinder_h
#define SCI_Geom_Cylinder_h 1

#include <Geom/Geom.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>

class GeomCylinder : public GeomObj {
    Vector v1;
    Vector v2;
public:
    Point bottom;
    Point top;
    Vector axis;
    double rad;
    int nu;
    int nv;
    void adjust();
    void move(const Point&, const Point&, double, int nu=20, int nv=4);

    GeomCylinder();
    GeomCylinder(const Point&, const Point&, double, int nu=20, int nv=4);
    GeomCylinder(const GeomCylinder&);
    virtual ~GeomCylinder();

    virtual GeomObj* clone();
    virtual void get_bounds(BBox&);

    virtual void objdraw(DrawInfoOpenGL*);
    virtual void make_prims(Array1<GeomObj*>& free,
			    Array1<GeomObj*>& dontfree);
};

#endif /* SCI_Geom_Cylinder_h */
