//  
//  For more information, please see: http://software.sci.utah.edu
//  
//  The MIT License
//  
//  Copyright (c) 2004 Scientific Computing and Imaging Institute,
//  University of Utah.
//  
//  License for the specific language governing rights and limitations under
//  Permission is hereby granted, free of charge, to any person obtaining a
//  copy of this software and associated documentation files (the "Software"),
//  to deal in the Software without restriction, including without limitation
//  the rights to use, copy, modify, merge, publish, distribute, sublicense,
//  and/or sell copies of the Software, and to permit persons to whom the
//  Software is furnished to do so, subject to the following conditions:
//  
//  The above copyright notice and this permission notice shall be included
//  in all copies or substantial portions of the Software.
//  
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
//  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
//  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//  DEALINGS IN THE SOFTWARE.
//  
//    File   : TextureBuilderAlgo.h
//    Author : Milan Ikits
//    Date   : Thu Jul 15 00:47:30 2004

#ifndef Volume_TextureBuilderAlgo_h
#define Volume_TextureBuilderAlgo_h

#include <Core/Datatypes/Datatype.h>
#include <Core/Malloc/Allocator.h>
#include <Core/Util/DynamicLoader.h>
#include <Core/Volume/Texture.h>
#include <Core/Volume/TextureBrick.h>

#include <iostream>
using std::cerr;
using std::endl;

using std::string;
using std::vector;


namespace SCIRun {

class TextureBuilderAlgoBase : public SCIRun::DynamicAlgoBase
{
public:
  TextureBuilderAlgoBase();
  virtual ~TextureBuilderAlgoBase();
  virtual void build(TextureHandle texture,
                     FieldHandle vfield, double vmin, double vmax,
                     FieldHandle gfield, double gmin, double gmax,
                     int card_mem) = 0;
  //! support the dynamically compiled algorithm concept
  static const string& get_h_file_path();
  static CompileInfoHandle get_compile_info(const TypeDescription* td);

protected:
  virtual void build_bricks(vector<TextureBrick*>& bricks,
			    int nx, int ny, int nz,
                            int nc, int* nb,
			    const BBox& bbox, int brick_mem) = 0;
  virtual void fill_brick(TextureBrick* brick,
                          FieldHandle vfield, double vmin, double vmax,
                          FieldHandle gfield, double gmin, double gmax) = 0;
};

template <class FieldType>
class TextureBuilderAlgo : public TextureBuilderAlgoBase
{
public:
  typedef typename FieldType::value_type value_type;

  TextureBuilderAlgo() {}
  virtual ~TextureBuilderAlgo() {}
  
  virtual void build(TextureHandle texture,
                     FieldHandle vfield, double vmin, double vmax,
                     FieldHandle gfield, double gmin, double gmax,
                     int card_mem);
  
protected:
  virtual void build_bricks(vector<TextureBrick*>& bricks,
			    int nx, int ny, int nz,
                            int nc, int* nb,
			    const BBox& bbox, int brick_mem);
  virtual void fill_brick(TextureBrick* brick,
                          FieldHandle vfield, double vmin, double vmax,
                          FieldHandle gfield, double gmin, double gmax);
}; 

template<typename FieldType>
void
TextureBuilderAlgo<FieldType>::build(TextureHandle texture,
                                     FieldHandle vfield,
				     double vmin, double vmax,
                                     FieldHandle gfield,
				     double gmin, double gmax,
                                     int card_mem)
{
  LatVolMeshHandle mesh = (LatVolMesh*)(vfield->mesh().get_rep());
  int nx = mesh->get_ni();
  int ny = mesh->get_nj();
  int nz = mesh->get_nk();
  if(vfield->basis_order() == 0) {
    --nx; --ny; --nz;
  }
  int nc = gfield.get_rep() ? 2 : 1;
  int nb[2];
  nb[0] = gfield.get_rep() ? 4 : 1;
  nb[1] = gfield.get_rep() ? 1 : 0;
  Transform tform;
  mesh->transform(tform);

  texture->lock_bricks();
  vector<TextureBrick*>& bricks = texture->bricks();
  // TODO:  BBox is only the transform if we're axis aligned.
  // Fix to use the mesh transform.
  const BBox &bbox = mesh->get_bounding_box();
  if (nx != texture->nx() || ny != texture->ny() || nz != texture->nz()
      || nc != texture->nc() || card_mem != texture->card_mem() ||
      bbox.min() != texture->bbox().min() ||
      bbox.max() != texture->bbox().max())
  {
    build_bricks(bricks, nx, ny, nz, nc, nb, bbox, card_mem);
    texture->set_size(nx, ny, nz, nc, nb);
    texture->set_card_mem(card_mem);
  }
  texture->set_bbox(bbox);
  texture->set_minmax(vmin, vmax, gmin, gmax);
  texture->set_transform(tform);
  for(unsigned int i=0; i<bricks.size(); i++) {
    fill_brick(bricks[i], vfield, vmin, vmax, gfield, gmin, gmax);
    bricks[i]->set_dirty(true);
  }
  texture->unlock_bricks();
}

template<typename FieldType>
void
TextureBuilderAlgo<FieldType>::build_bricks(vector<TextureBrick*>& bricks,
					    int nx, int ny, int nz,
					    int nc, int* nb,
                                            const BBox& bbox, int card_mem)
{
  int brick_mem = card_mem*1024*1024/2;
  const int data_size[3] = { nx, ny, nz };
  int brick_size[3];
  int brick_offset[3];
  int brick_pad[3];
  int num_brick[3];
  int brick_nb;
  // figure out largest possible brick size
  int size[3];
  size[0] = data_size[0]; size[1] = data_size[1]; size[2] = data_size[2];
  // initial brick size
  brick_size[0] = NextPowerOf2(data_size[0]);
  brick_size[1] = NextPowerOf2(data_size[1]);
  brick_size[2] = NextPowerOf2(data_size[2]);
  // number of bytes per brick
  brick_nb = brick_size[0]*brick_size[1]*brick_size[2]*nb[0];
  // find brick size
  if(brick_nb > brick_mem)
  {
    // subdivide until fits
    while(brick_nb > brick_mem)
    {
      // number of bricks along the axes
      // the divisions have to satisfy the equation:
      // data_size <= (brick_size-1)*num_brick + 1
      // we assume that brick_size > 1
      num_brick[0] = (int)ceil((double)(data_size[0]-1)/(brick_size[0]-1));
      num_brick[1] = (int)ceil((double)(data_size[1]-1)/(brick_size[1]-1));
      num_brick[2] = (int)ceil((double)(data_size[2]-1)/(brick_size[2]-1));
      // size of leftover volumes
      int sp[3];
      sp[0] = data_size[0] - (num_brick[0]-1)*(brick_size[0]-1);
      sp[1] = data_size[1] - (num_brick[1]-1)*(brick_size[1]-1);
      sp[2] = data_size[2] - (num_brick[2]-1)*(brick_size[2]-1);
      // size of padding
      brick_pad[0] = NextPowerOf2(sp[0]) - sp[0];
      brick_pad[1] = NextPowerOf2(sp[1]) - sp[1];
      brick_pad[2] = NextPowerOf2(sp[2]) - sp[2];
      // sort padding
      int idx[3];
      SortIndex(brick_pad, idx);
      // split largest one
      size[idx[2]] = (int)ceil(size[idx[2]]/2.0);
      brick_size[idx[2]] = NextPowerOf2(size[idx[2]]);
      brick_nb = brick_size[0]*brick_size[1]*brick_size[2]*nb[0];
    }
  }
  // number of bricks along the axes
  // the divisions have to satisfy the equation:
  // data_size <= (brick_size-1)*num_brick + 1
  // we assume that brick_size > 1
  num_brick[0] = (int)ceil((double)(data_size[0]-1)/(brick_size[0]-1));
  num_brick[1] = (int)ceil((double)(data_size[1]-1)/(brick_size[1]-1));
  num_brick[2] = (int)ceil((double)(data_size[2]-1)/(brick_size[2]-1));
//   cerr << "Number of bricks: " << num_brick[0] << " x " << num_brick[1]
//        << " x " << num_brick[2] << endl;
  // padded sizes of last bricks
  brick_pad[0] = NextPowerOf2(data_size[0] - (num_brick[0]-1)*(brick_size[0]-1));
  brick_pad[1] = NextPowerOf2(data_size[1] - (num_brick[1]-1)*(brick_size[1]-1));
  brick_pad[2] = NextPowerOf2(data_size[2] - (num_brick[2]-1)*(brick_size[2]-1));
  // delete previous bricks (if any)
  for(unsigned int i=0; i<bricks.size(); i++) delete bricks[i];
  bricks.resize(0);
  bricks.reserve(num_brick[0]*num_brick[1]*num_brick[2]);
  // create bricks
  // data bbox
  double data_bbmin[3], data_bbmax[3];
  data_bbmin[0] = bbox.min().x();
  data_bbmin[1] = bbox.min().y();
  data_bbmin[2] = bbox.min().z();
  data_bbmax[0] = bbox.max().x();
  data_bbmax[1] = bbox.max().y();
  data_bbmax[2] = bbox.max().z();
  // bbox and tbox parameters
  double bmin[3], bmax[3], tmin[3], tmax[3];
  brick_offset[2] = 0;
  bmin[2] = data_bbmin[2];
  bmax[2] = num_brick[2] > 1 ?
    (double)(brick_size[2] - 0.5)/(double)data_size[2] *
    (data_bbmax[2] - data_bbmin[2]) + data_bbmin[2] : data_bbmax[2];
  tmin[2] = 0.0;
  tmax[2] = num_brick[2] > 1 ? (double)(brick_size[2] - 0.5)/(double)brick_size[2] :
    (double)data_size[2]/(double)brick_size[2];
  for (int k=0; k<num_brick[2]; k++)
  {
    brick_offset[1] = 0;
    bmin[1] = data_bbmin[1];
    bmax[1] = num_brick[1] > 1 ?
      (double)(brick_size[1] - 0.5)/(double)data_size[1] *
      (data_bbmax[1] - data_bbmin[1]) + data_bbmin[1] : data_bbmax[1];
    tmin[1] = 0.0;
    tmax[1] = num_brick[1] > 1 ?
      (double)(brick_size[1] - 0.5)/(double)brick_size[1] :
      (double)data_size[1]/(double)brick_size[1];
    for (int j=0; j<num_brick[1]; j++)
    {
      brick_offset[0] = 0;
      bmin[0] = data_bbmin[0];
      bmax[0] = num_brick[0] > 1 ?
        (double)(brick_size[0] - 0.5)/(double)data_size[0] *
        (data_bbmax[0] - data_bbmin[0]) + data_bbmin[0] : data_bbmax[0];
      tmin[0] = 0.0;
      tmax[0] = num_brick[0] > 1 ? (double)(brick_size[0] - 0.5)/(double)brick_size[0] :
        (double)data_size[0]/(double)brick_size[0];
      for (int i=0; i<num_brick[0]; i++)
      {
        TextureBrick* b = scinew TextureBrickT<unsigned char>(
          i < num_brick[0]-1 ? brick_size[0] : brick_pad[0],
          j < num_brick[1]-1 ? brick_size[1] : brick_pad[1],
          k < num_brick[2]-1 ? brick_size[2] : brick_pad[2],
          nc, nb,
          brick_offset[0], brick_offset[1], brick_offset[2],
          i < num_brick[0]-1 ? brick_size[0] : data_size[0] - (num_brick[0]-1)*(brick_size[0]-1),
          j < num_brick[1]-1 ? brick_size[1] : data_size[1] - (num_brick[1]-1)*(brick_size[1]-1),
          k < num_brick[2]-1 ? brick_size[2] : data_size[2] - (num_brick[2]-1)*(brick_size[2]-1),
          BBox(Point(bmin[0], bmin[1], bmin[2]),
               Point(bmax[0], bmax[1], bmax[2])),
          BBox(Point(tmin[0], tmin[1], tmin[2]),
               Point(tmax[0], tmax[1], tmax[2])),
          true);
//         cerr << "Adding brick: " << b->nx() << " " << b->ny() << " " << b->nz() << " | "
//              << b->ox() << " " << b->oy() << " " << b->oz() << " -> "
//              << b->mx() << " " << b->my() << " " << b->mz() << endl;
        bricks.push_back(b);

        // update x parameters                     
        brick_offset[0] += brick_size[0]-1;
        bmin[0] = bmax[0];
        bmax[0] += i < num_brick[0]-2 ?
          (double)(brick_size[0]-1)/(double)data_size[0]*(data_bbmax[0] - data_bbmin[0]) :
          (double)(data_size[0]-brick_offset[0]-0.5)/(double)data_size[0]*(data_bbmax[0]-data_bbmin[0]);
        tmin[0] = i < num_brick[0]-2 ? 0.5/(double)brick_size[0] : 0.5/(double)brick_pad[0];
        tmax[0] = i < num_brick[0]-2 ? (double)(brick_size[0]-0.5)/(double)brick_size[0] : (data_size[0] - brick_offset[0])/(double)brick_pad[0];
      }
      // update y parameters
      brick_offset[1] += brick_size[1]-1;
      bmin[1] = bmax[1];
      bmax[1] += j < num_brick[1]-2 ?
        (double)(brick_size[1]-1)/(double)data_size[1] * (data_bbmax[1]-data_bbmin[1]) :
        (double)(data_size[1]-brick_offset[1]-0.5)/(double)data_size[1] * (data_bbmax[1]-data_bbmin[1]);
      tmin[1] = j < num_brick[1]-2 ? 0.5/(double)brick_size[1] : 0.5/(double)brick_pad[1];
      tmax[1] = j < num_brick[1]-2 ? (double)(brick_size[1]-0.5)/(double)brick_size[1] : (data_size[1]-brick_offset[1])/(double)brick_pad[1];
    } // j
    // update z parameters
    brick_offset[2] += brick_size[2]-1;
    bmin[2] = bmax[2];
    bmax[2] += k < num_brick[2]-2 ?
      (double)(brick_size[2]-1)/(double)data_size[2] * (data_bbmax[2]-data_bbmin[2]) :
      (double)(data_size[2]-brick_offset[2]-0.5)/(double)data_size[2] * (data_bbmax[2]-data_bbmin[2]);
    tmin[2] = k < num_brick[2]-2 ? 0.5/(double)brick_size[2] : 0.5/(double)brick_pad[2];
    tmax[2] = k < num_brick[2]-2 ? (double)(brick_size[2]-0.5)/(double)brick_size[2] : (data_size[2] - brick_offset[2])/(double)brick_pad[2];
  } // k
}

template <class FieldType>
void 
TextureBuilderAlgo<FieldType>::fill_brick(TextureBrick* brick,
                                          FieldHandle vfield,
					  double vmin, double vmax,
                                          FieldHandle gfield,
					  double gmin, double gmax)
{
  LatVolField<value_type>* vfld = 
    dynamic_cast<LatVolField<value_type>*>(vfield.get_rep());
  LatVolField<Vector>* gfld = 
    dynamic_cast<LatVolField<Vector>*>(gfield.get_rep());
  int nc = brick->nc();
  TextureBrickT<unsigned char>* br =
    dynamic_cast<TextureBrickT<unsigned char>*>(brick);

  int nx = brick->nx();
  int ny = brick->ny();
  int nz = brick->nz();
  int x0 = brick->ox();
  int y0 = brick->oy();
  int z0 = brick->oz();
  int x1 = x0+brick->mx();
  int y1 = y0+brick->my();
  int z1 = z0+brick->mz();

  int i, j, k, ii, jj, kk;
    
  if (br && vfld && ((gfld && nc == 2) || nc == 1))
  {
    typename FieldType::mesh_type* mesh = vfld->get_typed_mesh().get_rep();

    if (!gfld) { // fill only values
      unsigned char* tex = br->data(0);
      if(vfield->basis_order() == 0) {
        typename FieldType::mesh_type::RangeCellIter iter(mesh, x0, y0, z0,
                                                          x1, y1, z1);
        for(k=0, kk=z0; kk<z1; kk++, k++) {
          for(j=0, jj=y0; jj<y1; jj++, j++) {
            for(i=0, ii=x0; ii<x1; ii++, i++) {
              double v = vfld->fdata()[*iter];
              tex[k*ny*nx+j*nx+i] =
                (unsigned char)(Clamp((v - vmin)/(vmax-vmin), 0.0, 1.0)*255.0);
              ++iter;
            }
            if(nx != brick->mx()) {
              tex[k*ny*nx+j*nx+i] = 0;
            }
          }
          if(ny != brick->my()) {
            for(i=0; i<Min(nx, brick->mx()+1); i++) {
              tex[k*ny*nx+j*nx+i] = 0;
            }
          }
        }
        if(nz != brick->mz()) {
          for(j=0; j<Min(ny, brick->my()+1); j++) {
            for(i=0; i<Min(nx, brick->mx()+1); i++) {
              tex[k*ny*nx+j*nx+i] = 0;
            }
          }
        }
      } else {
        typename FieldType::mesh_type::RangeNodeIter iter(mesh, x0, y0, z0,
                                                          x1, y1, z1);
        for(k=0, kk=z0; kk<z1; kk++, k++) {
          for(j=0, jj=y0; jj<y1; jj++, j++) {
            for(i=0, ii=x0; ii<x1; ii++, i++) {
              double v = vfld->fdata()[*iter];
              tex[k*ny*nx+j*nx+i] =
                (unsigned char)(Clamp((v - vmin)/(vmax-vmin), 0.0, 1.0)*255.0);
              ++iter;
            }
            if(nx != brick->mx()) {
              tex[k*ny*nx+j*nx+i] = 0;
            }
          }
          if(ny != brick->my()) {
            for(i=0; i<Min(nx, brick->mx()+1); i++) {
              tex[k*ny*nx+j*nx+i] = 0;
            }
          }
        }
        if(nz != brick->mz()) {
          for(j=0; j<Min(ny, brick->my()+1); j++) {
            for(i=0; i<Min(nx, brick->mx()+1); i++) {
              tex[k*ny*nx+j*nx+i] = 0;
            }
          }
        }
      }
    } else { // fill values + gradient
      unsigned char* tex0 = br->data(0);
      unsigned char* tex1 = br->data(1);
      
      if(vfield->basis_order() == 0) {
        typename FieldType::mesh_type::RangeCellIter iter(mesh, x0, y0, z0,
                                                          x1, y1, z1);
        for(k=0, kk=z0; kk<z1; kk++, k++) {
          for(j=0, jj=y0; jj<y1; jj++, j++) {
            for(i=0, ii=x0; ii<x1; ii++, i++) {
              double v = vfld->fdata()[*iter];
              int idx = k*ny*nx+j*nx+i;
              tex0[idx*4+3] =
                (unsigned char)(Clamp((v - vmin)/(vmax-vmin), 0.0, 1.0)*255.0);
              Vector g = gfld->fdata()[*iter];
              double gn = g.length();
              if(gn > std::numeric_limits<float>::epsilon())
                g.normalize();
              else
                g = Vector(0.0, 0.0, 0.0);
              tex0[idx*4+0] = (unsigned char)((g.x()*0.5 + 0.5)*255);
              tex0[idx*4+1] = (unsigned char)((g.y()*0.5 + 0.5)*255);
              tex0[idx*4+2] = (unsigned char)((g.z()*0.5 + 0.5)*255);
              tex1[idx] = (unsigned char)((gn-gmin)/(gmax-gmin))*255;
              ++iter;
            }
            if(nx != brick->mx()) {
              int idx = k*ny*nx+j*nx+i;
              tex0[idx*4+0] = 0;
              tex0[idx*4+1] = 0;
              tex0[idx*4+2] = 0;
              tex0[idx*4+3] = 0;
              tex1[idx] = 0;
            }
          }
          if(ny != brick->my()) {
            for(i=0; i<Min(nx, brick->mx()+1); i++) {
              int idx = k*ny*nx+j*nx+i;
              tex0[idx*4+0] = 0;
              tex0[idx*4+1] = 0;
              tex0[idx*4+2] = 0;
              tex0[idx*4+3] = 0;
              tex1[idx] = 0;
            }
          }
        }
        if(nz != brick->mz()) {
          for(j=0; j<Min(ny, brick->my()+1); j++) {
            for(i=0; i<Min(nx, brick->mx()+1); i++) {
              int idx = k*ny*nx+j*nx+i;
              tex0[idx*4+0] = 0;
              tex0[idx*4+1] = 0;
              tex0[idx*4+2] = 0;
              tex0[idx*4+3] = 0;
              tex1[idx] = 0;
            }
          }
        }
      } else {
        typename FieldType::mesh_type::RangeNodeIter iter(mesh, x0, y0, z0,
                                                          x1, y1, z1);
        for(k=0, kk=z0; kk<z1; kk++, k++) {
          for(j=0, jj=y0; jj<y1; jj++, j++) {
            for(i=0, ii=x0; ii<x1; ii++, i++) {
              double v = vfld->fdata()[*iter];
              int idx = k*ny*nx+j*nx+i;
              tex0[idx*4+3] =
                (unsigned char)(Clamp((v - vmin)/(vmax-vmin), 0.0, 1.0)*255.0);
              Vector g = gfld->fdata()[*iter];
              double gn = g.length();
              if(gn > std::numeric_limits<float>::epsilon())
                g.normalize();
              else
                g = Vector(0.0, 0.0, 0.0);
              tex0[idx*4+0] = (unsigned char)((g.x()*0.5 + 0.5)*255);
              tex0[idx*4+1] = (unsigned char)((g.y()*0.5 + 0.5)*255);
              tex0[idx*4+2] = (unsigned char)((g.z()*0.5 + 0.5)*255);
              tex1[idx] = (unsigned char)((gn-gmin)/(gmax-gmin))*255;
              ++iter;
            }
            if(nx != brick->mx()) {
              int idx = k*ny*nx+j*nx+i;
              tex0[idx*4+0] = 0;
              tex0[idx*4+1] = 0;
              tex0[idx*4+2] = 0;
              tex0[idx*4+3] = 0;
              tex1[idx] = 0;
            }
          }
          if(ny != brick->my()) {
            for(i=0; i<Min(nx, brick->mx()+1); i++) {
              int idx = k*ny*nx+j*nx+i;
              tex0[idx*4+0] = 0;
              tex0[idx*4+1] = 0;
              tex0[idx*4+2] = 0;
              tex0[idx*4+3] = 0;
              tex1[idx] = 0;
            }
          }
        }
        if(nz != brick->mz()) {
          for(j=0; j<Min(ny, brick->my()+1); j++) {
            for(i=0; i<Min(nx, brick->mx()+1); i++) {
              int idx = k*ny*nx+j*nx+i;
              tex0[idx*4+0] = 0;
              tex0[idx*4+1] = 0;
              tex0[idx*4+2] = 0;
              tex0[idx*4+3] = 0;
              tex1[idx] = 0;
            }
          }
        }
      }
    }    
  }
  else
  {
    cerr<<"Not a Lattice type---should not be here\n";
  }
}


} // namespace SCIRun

#endif // Volume_TextureBuilderAlgo_h
