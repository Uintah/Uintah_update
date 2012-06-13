#include "StrainTensorMagnitude.h"

#include <spatialops/OperatorDatabase.h>
#include <spatialops/structured/SpatialFieldStore.h>

StrainTensorMagnitude::
StrainTensorMagnitude( const Expr::Tag& vel1tag,
                       const Expr::Tag& vel2tag,
                       const Expr::Tag& vel3tag )
  : Expr::Expression<SVolField>(),
    vel1t_( vel1tag ),
    vel2t_( vel2tag ),
    vel3t_( vel3tag ),
    doX_  ( vel1t_ != Expr::Tag() ),
    doY_  ( vel2t_ != Expr::Tag() ),
    doZ_  ( vel3t_ != Expr::Tag() )
{}

//--------------------------------------------------------------------

StrainTensorMagnitude::
~StrainTensorMagnitude()
{}

//--------------------------------------------------------------------

void
StrainTensorMagnitude::
advertise_dependents( Expr::ExprDeps& exprDeps )
{
  if( doX_ ) exprDeps.requires_expression( vel1t_ );
  if( doY_ ) exprDeps.requires_expression( vel2t_ );
  if( doZ_ ) exprDeps.requires_expression( vel3t_ );
}

//--------------------------------------------------------------------

void
StrainTensorMagnitude::
bind_fields( const Expr::FieldManagerList& fml )
{
  const Expr::FieldManager<XVolField>& v1fm  = fml.field_manager<XVolField>();
  const Expr::FieldManager<YVolField>& v2fm  = fml.field_manager<YVolField>();
  const Expr::FieldManager<ZVolField>& v3fm  = fml.field_manager<ZVolField>();

  if ( doX_ ) vel1_ = &v1fm.field_ref( vel1t_ );
  if ( doY_ ) vel2_ = &v2fm.field_ref( vel2t_ );
  if ( doZ_ ) vel3_ = &v3fm.field_ref( vel3t_ );
}

//--------------------------------------------------------------------

void
StrainTensorMagnitude::
bind_operators( const SpatialOps::OperatorDatabase& opDB )
{
  dudxOp_ = opDB.retrieve_operator<dudxT>();
  dudyOp_ = opDB.retrieve_operator<dudyT>();
  dudzOp_ = opDB.retrieve_operator<dudzT>();
  
  dvdxOp_ = opDB.retrieve_operator<dvdxT>();
  dvdyOp_ = opDB.retrieve_operator<dvdyT>();
  dvdzOp_ = opDB.retrieve_operator<dvdzT>();  

  dwdxOp_ = opDB.retrieve_operator<dwdxT>();
  dwdyOp_ = opDB.retrieve_operator<dwdyT>();
	 dwdzOp_ = opDB.retrieve_operator<dwdzT>();
  
  xyInterpOp_ = opDB.retrieve_operator<XYInterpT>();
  yxInterpOp_ = opDB.retrieve_operator<YXInterpT>();

  xzInterpOp_ = opDB.retrieve_operator<XZInterpT>();
  zxInterpOp_ = opDB.retrieve_operator<ZXInterpT>();
  
  yzInterpOp_ = opDB.retrieve_operator<YZInterpT>();
  zyInterpOp_ = opDB.retrieve_operator<ZYInterpT>();  
}

//--------------------------------------------------------------------

void
StrainTensorMagnitude::
evaluate()
{
  using namespace SpatialOps;
  SVolField& StrTsrMag = this->value();
  StrTsrMag <<= 0.0;
  
  SpatialOps::SpatFldPtr<SVolField> tmp1 = SpatialOps::SpatialFieldStore::get<SVolField>( StrTsrMag );
  SpatialOps::SpatFldPtr<SVolField> tmp2 = SpatialOps::SpatialFieldStore::get<SVolField>( StrTsrMag );
  *tmp1 <<= 0.0;
  *tmp2 <<= 0.0;

  SpatialOps::SpatFldPtr<SpatialOps::structured::XSurfYField> xyfield = SpatialOps::SpatialFieldStore::get<SpatialOps::structured::XSurfYField>( StrTsrMag );
  SpatialOps::SpatFldPtr<SpatialOps::structured::YSurfXField> yxfield = SpatialOps::SpatialFieldStore::get<SpatialOps::structured::YSurfXField>( StrTsrMag );
  
  SpatialOps::SpatFldPtr<SpatialOps::structured::XSurfZField> xzfield = SpatialOps::SpatialFieldStore::get<SpatialOps::structured::XSurfZField>( StrTsrMag );
  SpatialOps::SpatFldPtr<SpatialOps::structured::ZSurfXField> zxfield = SpatialOps::SpatialFieldStore::get<SpatialOps::structured::ZSurfXField>( StrTsrMag );
  
  SpatialOps::SpatFldPtr<SpatialOps::structured::YSurfZField> yzfield = SpatialOps::SpatialFieldStore::get<SpatialOps::structured::YSurfZField>( StrTsrMag );
  SpatialOps::SpatFldPtr<SpatialOps::structured::ZSurfYField> zyfield = SpatialOps::SpatialFieldStore::get<SpatialOps::structured::ZSurfYField>( StrTsrMag );

  if ( doX_ ) {
    dudxOp_->apply_to_field( *vel1_, *tmp1 );      // S_11 = 0.5 * (du/dx + du/dx)
    StrTsrMag <<= StrTsrMag + *tmp1 * *tmp1;       // S_11 * S_11
  }
  
  if ( doY_ ) {
    dvdyOp_->apply_to_field( *vel2_, *tmp1 );        // S_22 = 0.5 * (dv/dy + dv/dy)
    StrTsrMag <<= StrTsrMag + *tmp1 * *tmp1;         // S_22 * S_22
  }
  
  if ( doZ_ ) {
    dwdzOp_->apply_to_field( *vel3_, *tmp1 );        // S_33 = 0.5 * (dw/dz + dw/dz)
    StrTsrMag <<= StrTsrMag + *tmp1 * *tmp1;         // S_33 * S_33
  }
  
  if ( doX_ && doY_ ) {
    dudyOp_->apply_to_field( *vel1_, *xyfield );    // du/dy 
    xyInterpOp_->apply_to_field( *xyfield, *tmp1);  // interpolate to scalar cells
    
    dvdxOp_->apply_to_field( *vel2_, *yxfield );    // dv/dx
    yxInterpOp_->apply_to_field( *yxfield, *tmp2);  // interpolate to scalar cells
    
    *tmp1 <<= *tmp1 + *tmp2;                         // S_12 = S_21 = 0.5 * (du/dy + dv/dx)
    StrTsrMag <<= StrTsrMag + 0.5 * *tmp1 * *tmp1;   // S_12 * S_12 + S_21 * S_21 = 2*S_12*S_12
  }
     
  if ( doX_ && doZ_ ) {
    dudzOp_->apply_to_field( *vel1_, *xzfield );    // du/dz
    xzInterpOp_->apply_to_field( *xzfield, *tmp1);
    
    dwdxOp_->apply_to_field( *vel3_, *zxfield );    // dw/dx
    zxInterpOp_->apply_to_field( *zxfield, *tmp2);  
    
    *tmp1 <<= *tmp1 + *tmp2;                         // 2*S_13 = 2*S_31 = du/dz + dw/dx
    StrTsrMag <<= StrTsrMag + *tmp1 * *tmp1 * 0.5;   // |S|^2 / 2 = S_ij * S_ij (we take account for S_ij and Sji at the same time)    
  }  
    
  if ( doY_ && doZ_ ) {
    dvdzOp_->apply_to_field( *vel2_, *yzfield );    // dv/dz
    yzInterpOp_->apply_to_field( *yzfield, *tmp1);
    
    dwdyOp_->apply_to_field( *vel3_, *zyfield );    // dw/dy
    zyInterpOp_->apply_to_field( *zyfield, *tmp2);  
    
    *tmp1 <<= *tmp1 + *tmp2;                         // 2*S_23 = 2*S_32 = dv/dz + dw/dy
    StrTsrMag <<= StrTsrMag + *tmp1 * *tmp1 * 0.5;   // |S|^2 / 2 = S_ij * S_ij (we take account for S_ij and Sji at the same time)    
  }
  
  //StrTsrMag <<= sqrt( StrTsrMag * 2.0 );                // |S|= (2 * S_ij * S_ij)^(1/2)
  
}

//--------------------------------------------------------------------

StrainTensorMagnitude::
Builder::Builder( const Expr::Tag& result,
                  const Expr::Tag& vel1tag,
                  const Expr::Tag& vel2tag,
                  const Expr::Tag& vel3tag )
  : ExpressionBuilder(result),
    v1t_( vel1tag ), 
    v2t_( vel2tag ), 
    v3t_( vel3tag )
{}

//--------------------------------------------------------------------

Expr::ExpressionBase*
StrainTensorMagnitude::Builder::build() const
{
  return new StrainTensorMagnitude( v1t_, v2t_, v3t_ );
}

//--------------------------------------------------------------------
//====================================================================
//--------------------------------------------------------------------

SquareStrainTensorMagnitude::
SquareStrainTensorMagnitude( const Expr::Tag& vel1tag,
                      const Expr::Tag& vel2tag,
                      const Expr::Tag& vel3tag )
: StrainTensorMagnitude( vel1tag, vel2tag, vel3tag )
{}

//--------------------------------------------------------------------

SquareStrainTensorMagnitude::
~SquareStrainTensorMagnitude()
{}

//--------------------------------------------------------------------

void
SquareStrainTensorMagnitude::
evaluate()
{
  using namespace SpatialOps;
  SVolField& StrTsrMag = this->value();
  StrTsrMag <<= 0.0;
  
  SpatialOps::SpatFldPtr<SpatialOps::structured::XSurfYField> xyfield = SpatialOps::SpatialFieldStore::get<SpatialOps::structured::XSurfYField>( StrTsrMag );
  SpatialOps::SpatFldPtr<SpatialOps::structured::YSurfXField> yxfield = SpatialOps::SpatialFieldStore::get<SpatialOps::structured::YSurfXField>( StrTsrMag );
  
  SpatialOps::SpatFldPtr<SpatialOps::structured::XSurfZField> xzfield = SpatialOps::SpatialFieldStore::get<SpatialOps::structured::XSurfZField>( StrTsrMag );  
  SpatialOps::SpatFldPtr<SpatialOps::structured::ZSurfXField> zxfield = SpatialOps::SpatialFieldStore::get<SpatialOps::structured::ZSurfXField>( StrTsrMag );  
  
  SpatialOps::SpatFldPtr<SpatialOps::structured::YSurfZField> yzfield = SpatialOps::SpatialFieldStore::get<SpatialOps::structured::YSurfZField>( StrTsrMag );
  SpatialOps::SpatFldPtr<SpatialOps::structured::ZSurfYField> zyfield = SpatialOps::SpatialFieldStore::get<SpatialOps::structured::ZSurfYField>( StrTsrMag );
  
  SpatialOps::SpatFldPtr<SVolField> g11 = SpatialOps::SpatialFieldStore::get<SVolField>( StrTsrMag );
  SpatialOps::SpatFldPtr<SVolField> g12 = SpatialOps::SpatialFieldStore::get<SVolField>( StrTsrMag );
  SpatialOps::SpatFldPtr<SVolField> g13 = SpatialOps::SpatialFieldStore::get<SVolField>( StrTsrMag );
  SpatialOps::SpatFldPtr<SVolField> g21 = SpatialOps::SpatialFieldStore::get<SVolField>( StrTsrMag );
  SpatialOps::SpatFldPtr<SVolField> g22 = SpatialOps::SpatialFieldStore::get<SVolField>( StrTsrMag );
  SpatialOps::SpatFldPtr<SVolField> g23 = SpatialOps::SpatialFieldStore::get<SVolField>( StrTsrMag );
  SpatialOps::SpatFldPtr<SVolField> g31 = SpatialOps::SpatialFieldStore::get<SVolField>( StrTsrMag );
  SpatialOps::SpatFldPtr<SVolField> g32 = SpatialOps::SpatialFieldStore::get<SVolField>( StrTsrMag );
  SpatialOps::SpatFldPtr<SVolField> g33 = SpatialOps::SpatialFieldStore::get<SVolField>( StrTsrMag );
  
  
  //  if (!doX_) {
  *g11 <<= 0.0;
  *g12 <<= 0.0;
  *g13 <<= 0.0;
  *g21 <<= 0.0;
  *g31 <<= 0.0;
  //  }
  
  //  if (!doY_) {
  *g21 <<= 0.0;
  *g22 <<= 0.0;
  *g23 <<= 0.0;
  *g12 <<= 0.0;
  *g32 <<= 0.0;
  //  }
  
  //  if (!doZ_) {
  *g31 <<= 0.0;
  *g32 <<= 0.0;
  *g33 <<= 0.0;
  *g23 <<= 0.0;
  *g13 <<= 0.0;
  //  }
  
  
  // du fields
  if (doX_) dudxOp_->apply_to_field( *vel1_, *g11 );
  if (doY_) dvdyOp_->apply_to_field( *vel2_, *g22 );
  if (doZ_) dwdzOp_->apply_to_field( *vel3_, *g33 );      
  
  if (doX_ && doY_) {
    dudyOp_->apply_to_field( *vel1_, *xyfield );   // du/dy 
    xyInterpOp_->apply_to_field( *xyfield, *g12);  // interpolate to scalar cells

    dvdxOp_->apply_to_field( *vel2_, *yxfield );   // dv/dx
    yxInterpOp_->apply_to_field( *yxfield, *g21);  // interpolate to scalar cells
  }
  
  if (doX_ && doZ_) {
    dudzOp_->apply_to_field( *vel1_, *xzfield );   // du/dz
    xzInterpOp_->apply_to_field( *xzfield, *g13);

    dwdxOp_->apply_to_field( *vel3_, *zxfield );   // dw/dx
    zxInterpOp_->apply_to_field( *zxfield, *g31);      
  }

  if (doY_ && doZ_) {    
    dvdzOp_->apply_to_field( *vel2_, *yzfield );   // dv/dz
    yzInterpOp_->apply_to_field( *yzfield, *g23);    
    
    dwdyOp_->apply_to_field( *vel3_, *zyfield );   // dw/dy
    zyInterpOp_->apply_to_field( *zyfield, *g32);      
  }
  
  SpatialOps::SpatFldPtr<SVolField> gd11 = SpatialOps::SpatialFieldStore::get<SVolField>( StrTsrMag );
  SpatialOps::SpatFldPtr<SVolField> gd12 = SpatialOps::SpatialFieldStore::get<SVolField>( StrTsrMag );
  SpatialOps::SpatFldPtr<SVolField> gd13 = SpatialOps::SpatialFieldStore::get<SVolField>( StrTsrMag );
  SpatialOps::SpatFldPtr<SVolField> gd21 = SpatialOps::SpatialFieldStore::get<SVolField>( StrTsrMag );
  SpatialOps::SpatFldPtr<SVolField> gd22 = SpatialOps::SpatialFieldStore::get<SVolField>( StrTsrMag );
  SpatialOps::SpatFldPtr<SVolField> gd23 = SpatialOps::SpatialFieldStore::get<SVolField>( StrTsrMag );
  SpatialOps::SpatFldPtr<SVolField> gd31 = SpatialOps::SpatialFieldStore::get<SVolField>( StrTsrMag );
  SpatialOps::SpatFldPtr<SVolField> gd32 = SpatialOps::SpatialFieldStore::get<SVolField>( StrTsrMag );
  SpatialOps::SpatFldPtr<SVolField> gd33 = SpatialOps::SpatialFieldStore::get<SVolField>( StrTsrMag );

  //  if (!doX_) {
  *gd11 <<= 0.0;
  *gd12 <<= 0.0;
  *gd13 <<= 0.0;
  *gd21 <<= 0.0;
  *gd31 <<= 0.0;
  //  }
  
  //  if (!doY_) {
  *gd21 <<= 0.0;
  *gd22 <<= 0.0;
  *gd23 <<= 0.0;
  *gd12 <<= 0.0;
  *gd32 <<= 0.0;
  //  }
  
  //  if (!doZ_) {
  *gd31 <<= 0.0;
  *gd32 <<= 0.0;
  *gd33 <<= 0.0;
  *gd23 <<= 0.0;
  *gd13 <<= 0.0;
  //  }
  
  *gd11 <<= *g11 * *g11 + *g12 * *g21 + *g13 * *g31;
  *gd12 <<= *g11 * *g12 + *g12 * *g22 + *g13 * *g32;  
  *gd13 <<= *g11 * *g13 + *g12 * *g23 + *g13 * *g33;

  *gd21 <<= *g21 * *g11 + *g22 * *g21 + *g23 * *g31;
  *gd22 <<= *g21 * *g12 + *g22 * *g22 + *g23 * *g32;  
  *gd23 <<= *g21 * *g13 + *g22 * *g23 + *g23 * *g33;

  *gd31 <<= *g31 * *g11 + *g32 * *g21 + *g33 * *g31;
  *gd32 <<= *g31 * *g12 + *g32 * *g22 + *g33 * *g32;  
  *gd33 <<= *g31 * *g13 + *g32 * *g23 + *g33 * *g33;      

  SpatialOps::SpatFldPtr<SVolField> tmp  = SpatialOps::SpatialFieldStore::get<SVolField>( StrTsrMag );
  SpatialOps::SpatFldPtr<SVolField> dilsq = SpatialOps::SpatialFieldStore::get<SVolField>( StrTsrMag );
  *tmp<<=0.0;
  *dilsq<<=0.0;
  
  *dilsq<<= (1.0/3.0)*(*gd11 + *gd22 + *gd33);
  
//  *tmp <<= *gd11 - *dilsq;                       // Sd_11
//  StrTsrMag <<= StrTsrMag + *tmp * *tmp;         // Sd_11*Sd_11
//
//  *tmp <<= 0.5*(*gd12 + *gd21);                  // Sd_12
//  StrTsrMag <<= StrTsrMag + 2.0 * *tmp * *tmp;   // + Sd_12*Sd_12 + Sd_21*Sd_21
//  
//  *tmp <<= 0.5*(*gd13 + *gd31);                  // Sd_13
//  StrTsrMag <<= StrTsrMag + 2.0 * *tmp * *tmp;   // + Sd_13*Sd_13 + Sd_31*Sd_31
//
////  *tmp <<= 0.5*(*gd21 + *gd12);            // Sd_21
////  StrTsrMag <<= StrTsrMag + *tmp * *tmp;   // + Sd_21*Sd_21
//
//  *tmp <<= *gd22  - *dilsq;                      // Sd_22
//  StrTsrMag <<= StrTsrMag + *tmp * *tmp;         // + Sd_22*Sd_22
//
//  *tmp <<= 0.5*(*gd23 + *gd32);                  // Sd_23
//  StrTsrMag <<= StrTsrMag + 2.0 * *tmp * *tmp;   // + Sd_23*Sd_23 + Sd_32*Sd_32
//
////  *tmp <<= 0.5*(*gd31 + *gd13);            // Sd_31
////  StrTsrMag <<= StrTsrMag + *tmp * *tmp;   // + Sd_31*Sd_31
//
////  *tmp <<= 0.5*(*gd32 + *gd23);            // Sd_32
////  StrTsrMag <<= StrTsrMag + *tmp * *tmp;   // + Sd_32*Sd_32
//
//  *tmp <<= *gd33 - *dilsq;                       // Sd_33
//  StrTsrMag <<= StrTsrMag + *tmp * *tmp;         // + Sd_33*Sd_33  
  
  StrTsrMag <<=  (0.5*(*gd11 + *gd11) - *dilsq) * (0.5*(*gd11 + *gd11) - *dilsq)  
                + 0.5 * (*gd12 + *gd21) * (*gd12 + *gd21)                                // Sd_12*Sd_12 + Sd_21*Sd_21 = 2.0 Sd_12*Sd_12
                + 0.5 * (*gd13 + *gd31) * (*gd13 + *gd31)                                // Sd_13*Sd_13 + Sd_31*Sd_31 = 2.0 Sd_13*Sd_13
                + (0.5*(*gd22 + *gd22) - *dilsq) * (0.5*(*gd22 + *gd22) - *dilsq)
                + 0.5 * (*gd23 + *gd32) * (*gd23 + *gd32)                                // Sd_23*Sd_23 + Sd_32*Sd_32 = 2.0 Sd_23*Sd_23
                + (0.5*(*gd33 + *gd33) - *dilsq) * (0.5*(*gd33 + *gd33) - *dilsq);
}

//--------------------------------------------------------------------

SquareStrainTensorMagnitude::
Builder::Builder( const Expr::Tag& result,
                 const Expr::Tag& vel1tag,
                 const Expr::Tag& vel2tag,
                 const Expr::Tag& vel3tag )
: ExpressionBuilder(result),
v1t_( vel1tag ), 
v2t_( vel2tag ), 
v3t_( vel3tag )
{}

//--------------------------------------------------------------------

Expr::ExpressionBase*
SquareStrainTensorMagnitude::Builder::build() const
{
  return new SquareStrainTensorMagnitude( v1t_, v2t_, v3t_ );
}

//--------------------------------------------------------------------