/*
  The contents of this file are subject to the University of Utah Public
  License (the "License"); you may not use this file except in compliance
  with the License.
  
  Software distributed under the License is distributed on an "AS IS"
  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
  License for the specific language governing rights and limitations under
  the License.
  
  The Original Source Code is SCIRun, released March 12, 2001.
  
  The Original Source Code was developed by the University of Utah.
  Portions created by UNIVERSITY are Copyright (C) 2001, 1994 
  University of Utah. All Rights Reserved.
*/

/*
 *  ImageToField.cc:
 *
 *  Written by:
 *   darbyb
 *   TODAY'S DATE HERE
 *
 */

#include <Dataflow/Network/Module.h>
#include <Core/Malloc/Allocator.h>

#include <Packages/Insight/Dataflow/Ports/ITKDatatypePort.h>
#include <Dataflow/Ports/FieldPort.h>
#include <Packages/Insight/Core/Datatypes/ITKImageField.h>
#include <Packages/Insight/Core/Datatypes/ITKLatVolField.h>

#include <Core/Datatypes/ImageField.h>
#include <Core/Datatypes/LatVolField.h>

#include <Core/Datatypes/ImageMesh.h>

#include <Packages/Insight/share/share.h>

#include "itkRGBPixel.h"

namespace Insight {

using namespace SCIRun;

class InsightSHARE ImageToField : public Module {  
public:
  ITKDatatypeIPort* inport1_;
  ITKDatatypeHandle inhandle1_;

  FieldOPort* ofield_;
  FieldHandle ofield_handle_;

  GuiInt gui_copy_;

public:
  ImageToField(GuiContext*);

  virtual ~ImageToField();

  virtual void execute();

  virtual void tcl_command(GuiArgs&, void*);

  // Run function will dynamically cast data to determine which
  // instantiation we are working with. The last template type
  // refers to the last template type of the filter intstantiation.
  template< class InputImageType > 
  bool run( itk::Object* );

private:
  template<class InputImageType>
  FieldHandle create_image_field(ITKDatatypeHandle &im);

  template<class InputImageType>
  FieldHandle create_latvol_field(ITKDatatypeHandle &im);
  
};


DECLARE_MAKER(ImageToField)
ImageToField::ImageToField(GuiContext* ctx)
  : Module("ImageToField", ctx, Source, "Converters", "Insight"),
    gui_copy_(ctx->subVar("copy"))
{
}

ImageToField::~ImageToField(){
}

template<class InputImageType>
FieldHandle ImageToField::create_image_field(ITKDatatypeHandle &img) {
  InputImageType *n = dynamic_cast< InputImageType * >( img.get_rep()->data_.GetPointer() );

  typedef ITKImageField<typename InputImageType::PixelType> ITKImageFieldType;
  typedef ImageField<typename InputImageType::PixelType> ImageFieldType;

  double origin_x = n->GetOrigin()[0];
  double origin_y = n->GetOrigin()[1];

  // get number of data points
  unsigned int size_x = (n->GetLargestPossibleRegion()).GetSize()[0];
  unsigned int size_y = (n->GetLargestPossibleRegion()).GetSize()[1];

  // get spacing between data points
  double space_x = n->GetSpacing()[0];
  double  space_y = n->GetSpacing()[1];

  // the origin specified by the itk image should remain the same
  // so we must make the min and max points accordingly
  double spread_x = (space_x * size_x)/2;
  double spread_y = (space_y * size_y)/2;
  
  Point min(origin_x - spread_x, origin_y - spread_y, 0.0);
  Point max(origin_x + spread_x, origin_y + spread_y, 0.0);

  ImageMesh* m = new ImageMesh(size_x, size_y, min, max);

  ImageMeshHandle mh(m);

  FieldHandle fh;
  int mn_idx, mx_idx;
  
  if(gui_copy_.get() == 0) {
    // simply point to the itk image
    fh = new ITKImageFieldType(mh, Field::NODE, n); 
  }
  else if(gui_copy_.get() == 1) {
    // copy the data into a SCIRun ImageField
    fh = new ImageFieldType(mh, Field::NODE);
    ImageMesh::Node::iterator iter, end;
    mh->begin(iter);
    mh->end(end);
    
    // fill data
    typename InputImageType::IndexType pixelIndex;
    typedef typename ImageFieldType::value_type val_t;
    val_t tmp;
    ImageFieldType* fld = (ImageFieldType* )fh.get_rep();
    
    for(int row=0; row < size_y; row++) {
      for(int col=0; col < size_x; col++) {
	if(iter == end) {
	  return fh;
	}
	pixelIndex[0] = col;
	pixelIndex[1] = row;
	
	tmp = n->GetPixel(pixelIndex);
	fld->set_value(tmp, *iter);
	++iter;
      }
    }
  }
  else {
    ASSERTFAIL("ImageToField Error");
  }
  return fh;
}

template<class InputImageType>
FieldHandle ImageToField::create_latvol_field(ITKDatatypeHandle &img) {
  
  typedef ITKLatVolField<typename InputImageType::PixelType> ITKLatVolFieldType;
  typedef LatVolField<typename InputImageType::PixelType> LatVolFieldType;

  InputImageType *n = dynamic_cast< InputImageType * >( img.get_rep()->data_.GetPointer() );

  // get number of data points
  unsigned int size_x = (n->GetRequestedRegion()).GetSize()[0];
  unsigned int size_y = (n->GetRequestedRegion()).GetSize()[1];
  unsigned int size_z = (n->GetRequestedRegion()).GetSize()[2];

  // get spacing between data points
  float space_x = n->GetSpacing()[0];
  float space_y = n->GetSpacing()[1];
  float space_z = n->GetSpacing()[2];

  // get origin in physical space
  double origin_x = n->GetOrigin()[0];
  double origin_y = n->GetOrigin()[1];
  double origin_z = n->GetOrigin()[2];

  // the origin specified by the itk image should remain the same
  // so we must make the min and max points accordingly

  double spread_x = (space_x * size_x)/2;
  double spread_y = (space_y * size_y)/2;
  double spread_z = (space_z * size_z)/2;
  
  Point min(origin_x - spread_x, origin_y - spread_y, origin_z - spread_z);
  Point max(origin_x + spread_x, origin_y + spread_y, origin_z + spread_z);
  
  LatVolMesh* m = new LatVolMesh(size_x, size_y, size_z, min, max);

  LatVolMeshHandle mh(m);

  FieldHandle fh;
  int mn_idx, mx_idx;
  
  if(gui_copy_.get() == 0) {
    // simply referenc itk image
    fh = new ITKLatVolFieldType(mh, Field::NODE, n); 
  }
  else if(gui_copy_.get() == 1) {
    // copy the data into a SCIRun LatVolField
    fh = new LatVolFieldType(mh, Field::NODE); 
    LatVolMesh::Node::iterator iter, end;
    mh->begin(iter);
    mh->end(end);
    
    // fill data
    typename InputImageType::IndexType pixelIndex;
    typedef typename LatVolFieldType::value_type val_t;
    val_t tmp;
    LatVolFieldType* fld = (LatVolFieldType* )fh.get_rep();
    
    for(int z=0; z < size_z; z++) {
      for(int row=0; row < size_y; row++) {
	for(int col=0; col < size_x; col++) {
	  if(iter == end) {
	    return fh;
	  }
	  pixelIndex[0] = col;
	  pixelIndex[1] = row;
	  pixelIndex[2] = z;
	  
	  tmp = n->GetPixel(pixelIndex);
	  fld->set_value(tmp, *iter);
	  ++iter;
	}
      }
    }
  }
  else {
    ASSERTFAIL("ImageToField Error");
  }
  return fh;
}

template<class InputImageType >
bool ImageToField::run( itk::Object* obj1) 
{
   InputImageType* n = dynamic_cast< InputImageType * >(obj1);
  if( !n ) {
    return false;
  }

  bool dim_based_convert = true;
  
  // do a standard dimension based convert
  if(dim_based_convert) {
    int dim = n->GetImageDimension();
    
    switch(dim) {
      
    case 2:
      ofield_handle_ = create_image_field<InputImageType>(inhandle1_);
      break;
      
    case 3:
      ofield_handle_ = create_latvol_field<InputImageType>(inhandle1_);
      break;
    default:
      error("Cannot convert data that is not 2D or 3D to a SCIRun Field.");
      return false;
    }
  }
  return true;
}

void ImageToField::execute(){
  inport1_ = (ITKDatatypeIPort *)get_iport("InputImage");
  ofield_ = (FieldOPort *)get_oport("OutputImage");

  if (!inport1_) {
    error("Unable to initialize iport 'InputImage'.");
    return;
  }
  if (!ofield_) {
    error("Unable to initialize oport 'OutputImage'.");
    return;
  }

  if(!inport1_->get(inhandle1_))
    return;

  // get input
  itk::Object *n = inhandle1_.get_rep()->data_.GetPointer();

  // can we operate on it?
  if(0) { }
  else if(run< itk::Image<float, 2> >(n)) { }
  else if(run< itk::Image<float, 3> >(n)) { }
  else if(run< itk::Image<double, 3> >(n)) { }
  else if(run< itk::Image<unsigned char, 2> >(n)) { }
  else if(run< itk::Image<unsigned short, 2> >(n)) { }
  //else if(run< itk::Image<itk::RGBPixel<unsigned char>, 2> >(n)) { }
  else {
    // error
    error("Incorrect input type");
    return;
  }
  ofield_->send(ofield_handle_);
}

void ImageToField::tcl_command(GuiArgs& args, void* userdata)
{
  Module::tcl_command(args, userdata);
}



} // End namespace Insight


