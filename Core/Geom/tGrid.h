
/*
 *  tGrid.h: Grid object
 *
 *  Written by:
 *   Steven G. Parker
 *   Department of Computer Science
 *   University of Utah
 *   May 1995
 *
 *  Copyright (C) 1995 SCI Group
 */

#ifndef SCI_Geom_tGrid_h
#define SCI_Geom_tGrid_h 1

#include <Core/Geom/GeomObj.h>
#include <Core/Geom/Material.h>
#include <Core/Geometry/Point.h>
#include <Core/Geometry/Vector.h>
#include <Core/Containers/Array2.h>

namespace SCIRun {

class SCICORESHARE TexGeomGrid : public GeomObj {
    int tmap_size;
    int tmap_dlist;
    Point corner;
    Vector u, v, w;
    void adjust();

    unsigned short* tmapdata; // texture map
    int MemDim;
    int dimU,dimV;

    int num_chan;
    int convolve;
    int conv_dim;

    int kernal_change;

    float conv_data[25];
public:
    TexGeomGrid(int, int, const Point&, const Vector&, const Vector&,
		int chanels=3);
    TexGeomGrid(const TexGeomGrid&);
    virtual ~TexGeomGrid();

    virtual GeomObj* clone();

    void set(unsigned short* data,int datadim);

    void do_convolve(int dim, float* data);
  
#ifdef SCI_OPENGL
    virtual void draw(DrawInfoOpenGL*, Material*, double time);
#endif
    virtual void get_bounds(BBox&);

    virtual void io(Piostream&);
    static PersistentTypeID type_id;
    virtual bool saveobj(std::ostream&, const clString& format, GeomSave*);
};

} // End namespace SCIRun


#endif /* SCI_Geom_Grid_h */
