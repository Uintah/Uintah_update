#include <Packages/rtrt/Core/widget.h>
#include <Packages/rtrt/Core/shape.h>
#include <math.h>
#include <stdio.h>        
#include <stdlib.h>   

using namespace rtrt;

//                  [-------width--------]
//
//             ---> 0===================o0 <--(upperRightVertex[0],  ===
//             |     \                  /       upperRightVertex[1])  |
//(upperLeftVertex[0],\                /                              |
//  upperLeftVertex[1])\              /                               |
//                      \            /                                |
//             --------> \----------0 <------(midRightVertex[0],      |
//             |          \        /           midRightVertex[1])     |
// (midLeftVertex[0],      \      /                                   |
//   midLeftVertex[1])      \    /                                height
//                           \  /                                     |
//                            \/ <----(lowerVertex[0],               ===
//                                      lowerVertex[1])



// creation of new triangle widget
TriWidget::TriWidget( float x, float w, float h, float c[3], float a ) {
  switchFlag = 0;
  type = 0;
  drawFlag = 0;
  width = w;
  height = h;
  lowerVertex[0] = x;	       	      lowerVertex[1] = 55;
  midLeftVertex[0] = x-w/4;	      midLeftVertex[1] = (110+h)*0.5f;
  upperLeftVertex[0] = x-w/2;	      upperLeftVertex[1] = 55+h;
  upperRightVertex[0] = x+w/2;        upperRightVertex[1] = 55+h;
  midRightVertex[0] = x+w/4;	      midRightVertex[1] = (110+h)*0.5f;
  opac_x = x;
  opac_y = upperRightVertex[1];
  color[0] = c[0];
  color[1] = c[1];
  color[2] = c[2];
  alpha = a;
  translateStar = new GLStar( lowerVertex[0], lowerVertex[1], 8, 1, 0, 0 );
  lowerBoundStar = new GLStar( midRightVertex[0], midRightVertex[1], 8, 0, 1, 0 );
  widthStar = new GLStar( upperRightVertex[0], upperRightVertex[1], 8, 0, 0, 1 );
  shearBar = new GLBar( upperLeftVertex[0]+w/2, upperLeftVertex[1], 
			w, c[0], c[1], c[2] );
  barRounder = new GLStar( upperLeftVertex[0], upperLeftVertex[1], 
			   5.0, c[0], c[1], c[2] );
  opacityStar = new GLStar( opac_x, opac_y, 6.5, 1-c[0], 1-c[1], 1-c[2] );
  transText = new Texture<GLfloat>();
  transText->makeOneDimTextureImage();
} // TriWidget()



// replacement of another widget with a triangle widget, 
//  retaining some values such as position, opacity, and color
TriWidget::TriWidget( float x, float w, float h, float l, float c[3], float a,
		      float o_x, float o_y, Texture<GLfloat> *t, int sF ) {
  float fHeight = (l-55)/h;
  switchFlag = sF;
  drawFlag = 0;
  type = 0;
  width = w;
  height = h;
  lowerVertex[0] = x;		       	lowerVertex[1] = 55;
  midLeftVertex[0] = x-(w/2)*fHeight;   midLeftVertex[1] = l;
  upperLeftVertex[0] = x-w/2;		upperLeftVertex[1] = 55+h;
  upperRightVertex[0] = x+w/2;    	upperRightVertex[1] = 55+h;
  midRightVertex[0] = x+(w/2)*fHeight;	midRightVertex[1] = l;
  opac_x = o_x;
  opac_y = o_y;
  color[0] = c[0];
  color[1] = c[1];
  color[2] = c[2];
  alpha = a;
  translateStar = new GLStar( lowerVertex[0], lowerVertex[1], 8, 1, 0, 0 );
  lowerBoundStar = new GLStar( midRightVertex[0], midRightVertex[1], 8, 0, 1, 0 );
  widthStar = new GLStar( upperRightVertex[0], upperRightVertex[1], 8, 0, 0, 1 );
  shearBar = new GLBar( upperLeftVertex[0]+w/2, upperLeftVertex[1],
			w, c[0], c[1], c[2] );
  barRounder = new GLStar( upperLeftVertex[0], upperLeftVertex[1],
			   5.0, c[0], c[1], c[2] );
  opacityStar = new GLStar( opac_x, opac_y, 6.5, 1-c[0], 1-c[1], 1-c[2] );
  transText = t;
  transText->makeOneDimTextureImage();
} // TriWidget()



// used primarily to load widget information from saved UI state
TriWidget::TriWidget( float lV0, float mLV0, float mLV1, float mRV0, float mRV1,
		      float uLV0, float uLV1, float uRV0, float uRV1, float r,
		      float g, float b, float a, float o_x, float o_y, float t_r,
		      float t_g, float t_b, int t_x, int t_y, int sF ) {
  switchFlag = sF;
  lowerVertex[0] = lV0;        lowerVertex[1] = 55;
  midLeftVertex[0] = mLV0;     midLeftVertex[1] = mLV1;
  midRightVertex[0] = mRV0;    midRightVertex[1] = mRV1;
  upperLeftVertex[0] = uLV0;   upperLeftVertex[1] = uLV1;
  upperRightVertex[0] = uRV0;  upperRightVertex[1] = uRV1;
  width = uRV0-uLV0;
  height = uLV1-55;
  type = 0;
  drawFlag = 0;
  color[0] = r;
  color[1] = g;
  color[2] = b;
  alpha = a;
  opac_x = o_x;
  opac_y = o_y;
  translateStar = new GLStar( lowerVertex[0], lowerVertex[1], 8, 1, 0, 0 );
  lowerBoundStar = new GLStar( midRightVertex[0], midRightVertex[1], 8, 0, 1, 0 );
  widthStar = new GLStar( upperRightVertex[0], upperRightVertex[1], 8, 0, 0, 1 );
  shearBar = new GLBar( (upperRightVertex[0]+upperLeftVertex[0])/2,
			upperLeftVertex[1], width, r, g, b );
  barRounder = new GLStar( upperLeftVertex[0], upperLeftVertex[1], 5.0, r, g, b );
  opacityStar = new GLStar( opac_x, opac_y, 6.5, 1-r, 1-g, 1-b );
  transText = new Texture<GLfloat>();
  transText->current_color[0] = t_r;
  transText->current_color[1] = t_g;
  transText->current_color[2] = t_b;
  transText->colormap_x_offset = t_x;
  transText->colormap_y_offset = t_y;
  transText->makeOneDimTextureImage();
} // TriWidget()



// draws widget without its texture
void
TriWidget::draw( void ) {
  glBegin( GL_LINES );
  glColor3fv( color );
  glVertex2f( upperLeftVertex[0], upperLeftVertex[1] );  // left side
  glVertex2f( lowerVertex[0], lowerVertex[1] );

  glVertex2f( lowerVertex[0], lowerVertex[1] );          // right side
  glVertex2f( upperRightVertex[0], upperRightVertex[1] );

  glVertex2f( midRightVertex[0], midRightVertex[1] );    // lowerBound divider
  glVertex2f( midLeftVertex[0], midLeftVertex[1] );
  glEnd();

  shearBar->draw();
  barRounder->draw();
  translateStar->draw();
  widthStar->draw();
  lowerBoundStar->draw();
  opacityStar->draw();
} // draw()



// moves widget around the screen
void
TriWidget::translate( float dx, float dy ) {
  dy = 0; // prevents base from moving off window base
  // as long as translation keeps widget entirely inside its window
  if( upperLeftVertex[0]+dx > 5.0 && upperRightVertex[0]+dx < 495.0 &&
      lowerVertex[0]+dx > 5.0 && lowerVertex[0]+dx < 495.0 ) { 
    translateStar->translate( dx, 0 );
    lowerBoundStar->translate( dx, 0 );
    widthStar->translate( dx, 0 );
    shearBar->translate( dx, 0 );
    barRounder->translate( dx, 0 );
    opacityStar->translate( dx, 0 );
    opac_x += dx;
    lowerVertex[0] += dx;
    midLeftVertex[0] += dx;
    midRightVertex[0] += dx;
    upperLeftVertex[0] += dx;
    upperRightVertex[0] += dx;
  } // if
} // translate()



// adjusts the shear of the triangle widget by translating the uppermost part
//  and reconnecting it to the rest of the widget
void 
TriWidget::adjustShear( float dx, float dy ) { 
  // ratio of distances from the lowerBound and upperBound to the bottom tip
  float fractionalHeight = (midRightVertex[1]-lowerVertex[1])/
    (upperRightVertex[1]-lowerVertex[1]);

  // quicker computation if x and y translations keep widget fully inside window
  if( upperLeftVertex[0]+dx >= 5.0 && upperRightVertex[0]+dx <= 495.0  &&
      upperLeftVertex[1]+dy-3 > lowerVertex[1] && upperLeftVertex[1]+dy <= 295.0 ) {
    height += dy;
    widthStar->translate( dx, dy );
    barRounder->translate( dx, dy );
    shearBar->translate( dx, dy );
    lowerBoundStar->translate( dx*fractionalHeight, dy*fractionalHeight );
    opacityStar->translate( dx, dy );
    opac_x += dx;
    opac_y += dy;
    midLeftVertex[0] += dx*fractionalHeight;
    midLeftVertex[1] += dy*fractionalHeight;
    midRightVertex[0] += dx*fractionalHeight;
    midRightVertex[1] = midLeftVertex[1];
    upperLeftVertex[0] += dx;
    upperLeftVertex[1] += dy;
    upperRightVertex[0] += dx;
    upperRightVertex[1] = upperLeftVertex[1];
  } // if

  // if either x or y translation moves widget partially outside window, then slower
  // computation must be undertaken, inspecting the x and y dimensions independently
  else {
    if( upperLeftVertex[0]+dx >= 5.0 && upperRightVertex[0]+dx <= 495.0 ) {
      widthStar->translate( dx, 0 );
      barRounder->translate( dx, 0 );
      shearBar->translate( dx, 0 );
      lowerBoundStar->translate( dx*fractionalHeight, 0 );
      opacityStar->translate( dx, 0 );
      opac_x += dx;
      midLeftVertex[0] += dx*fractionalHeight;
      midRightVertex[0] += dx*fractionalHeight;
      upperLeftVertex[0] += dx;
      upperRightVertex[0] += dx;
    } // if()
    else if( upperLeftVertex[1]+dy-3 > lowerVertex[1] && 
	     upperLeftVertex[1]+dy <= 295.0 ) {
      height += dy;
      widthStar->translate( 0, dy );
      barRounder->translate( 0, dy );
      shearBar->translate( 0, dy );
      lowerBoundStar->translate( 0, dy*fractionalHeight );
      opacityStar->translate( 0, dy );
      opac_y += dy;
      midLeftVertex[1] += dy*fractionalHeight;
      midRightVertex[1] = midLeftVertex[1];
      upperLeftVertex[1] += dy;
      upperRightVertex[1] = upperLeftVertex[1];
    } // else if()
  } // else
} // adjustShear()



// adjusts this widget's shearBar's width
void 
TriWidget::adjustWidth( float dx, float dy ) {
  // if the adjustment doesn't cause part of the widget to fall outside its window
  if( upperLeftVertex[0]-dx+3 < upperRightVertex[0]+dx && 
      upperLeftVertex[0]-dx >= 5.0 && upperRightVertex[0]+dx <= 495.0 ) {
    // fraction of opacityStar's distance across the shearBar from left to right
    float frac_dist = ((opac_x-upperLeftVertex[0])/
		       (upperRightVertex[0]-upperLeftVertex[0]));
    // ratio of distances from the lowerBound and upperBound to the bottom tip
    float fractionalHeight = ((midRightVertex[1]-lowerVertex[1])/
			      (upperRightVertex[1]-lowerVertex[1]));
    width += 2*dx;
    shearBar->resize( dx, 0 );
    opac_x += 2*dx*frac_dist-dx;
    midLeftVertex[0] -= dx*fractionalHeight;
    midRightVertex[0] += dx*fractionalHeight;
    upperLeftVertex[0] -= dx;
    upperRightVertex[0] += dx;
    opacityStar->translate( 2*dx*frac_dist-dx, 0 );
    barRounder->translate( -dx, 0 );
    widthStar->translate( dx, 0 );
    lowerBoundStar->translate( dx*fractionalHeight, 0 );
  } // if()
} // adjustWidth()



// adjusts the lowerBoundStar's position along the right side of the widget
void 
TriWidget::adjustLowerBound( float dx, float dy ) {
  // slope of the right side of the widget
  float m = (upperRightVertex[1]-lowerVertex[1])/(upperRightVertex[0]-lowerVertex[0]);
  // ratio of distances from the lowerBound and upperBound to the bottom tip
  float fractionalHeight = (midRightVertex[1]-lowerVertex[1])/
    (upperRightVertex[1]-lowerVertex[1]);

  // following if statements attempt to manipulate lowerBoundStar more efficiently

  // if the mouse cursor is changing more in the x-direction...
  if( fabs(dx) > fabs(dy) && (midRightVertex[1]+dx*m) >= lowerVertex[1] &&
      (midRightVertex[1]+dx*m) <= upperRightVertex[1] ) {
    midRightVertex[0] += dx;
    midRightVertex[1] += dx*m;
    midLeftVertex[0] = midRightVertex[0]-(fractionalHeight*width);
    midLeftVertex[1] = midRightVertex[1];
    lowerBoundStar->translate( dx, dx*m );
  } // if
  // otherwise, it's moving more in the y-direction...
  else if( (midRightVertex[1]+dy) >= lowerVertex[1] &&
	   (midRightVertex[1]+dy) <= upperRightVertex[1] ) {
    midLeftVertex[1] += dy;
    midRightVertex[1] = midLeftVertex[1];
    midRightVertex[0] += dy/m;		
    midLeftVertex[0] = midRightVertex[0]-(fractionalHeight*width);
    lowerBoundStar->translate( dy/m, dy );
  } // else if
} // adjustLowerBound()



// adjusts the position of the opacityStar along this widget's shearBar
//  and the overall opacity of this widget's texture
void
TriWidget::adjustOpacity( float dx, float dy ) {
  // if the opacityStar's position adjustment will keep it on the shearBar
  if( opac_x+dx >= upperLeftVertex[0] && opac_x+dx <= upperRightVertex[0] ) {
    opac_x += dx;
    opacityStar->left += dx;
  } // if
} // adjustOpacity()



// controls in which way this widget is manipulated
void 
TriWidget::manipulate( float x, float dx, float y, float dy ) {
  // the following block of if statements allow for continuous manipulation
  //  without conducting parameter checks every time (quicker)
  if( drawFlag == 1)
    adjustOpacity( dx, dy );
  else if( drawFlag == 2 )
    adjustWidth( dx, dy );
  else if( drawFlag == 3 )
    adjustLowerBound( dx, dy );
  else if( drawFlag == 4 )
    adjustShear( dx, dy );
  else if( drawFlag == 5 )
    translate( dx, dy );

  // if drawFlag has not been set from main, then a parameter check must be
  //  conducted to determine in which way the user wishes to manipulate
  //  the widget (slower)
  else {
    // if mouse cursor near opacityStar
    if( x >= opac_x - 5 && x <= opac_x + 5 &&
	y >= opac_y - 5 && y <= opac_y + 5 ) {
      drawFlag = 1;
      adjustOpacity( dx, dy );
    } // if()
    // if mouse cursor near widthStar
    else if( x >= upperRightVertex[0] - 5 && x <= upperRightVertex[0] + 5 &&
	     y >= upperRightVertex[1] - 5 && y <= upperRightVertex[1] + 5 ) {
      drawFlag = 2;
      adjustWidth( dx, dy );
    } // if()
    // if mouse cursor near lowerBoundStar
    else if( x >= midRightVertex[0] - 5 && x <= midRightVertex[0] + 5 && 
	     y >= midRightVertex[1] - 5 && y <= midRightVertex[1] + 5 ) {
      drawFlag = 3;
      adjustLowerBound( dx, dy );
    } // if()
    // if mouse cursor on shearBar
    else if( x >= upperLeftVertex[0] - 5 && x <= upperRightVertex[0] + 5 && 
	     y >= upperRightVertex[1] - 5 && y <= upperRightVertex[1] + 5 ) {
      drawFlag = 4;
      adjustShear( dx, dy );
    } // if()
    // if mouse cursor near translateStar
    else if( x >= lowerVertex[0] - 5 && x <= lowerVertex[0] + 5 &&
	     y >= lowerVertex[1] - 5 && y <= lowerVertex[1] + 5 ) {
      drawFlag = 5;
      translate( dx, dy );
    } // if()
    // otherwise nothing pertinent was selected...
    else {
      drawFlag = 0;
      return;
    }
  } // else
} // manipulate()



// paints this widget's texture onto a background texture
void 
TriWidget::paintTransFunc( GLfloat texture_dest[textureHeight][textureWidth][4],
			   float master_alpha ) {
  float startyf = (midLeftVertex[1]-55.0f)*(float)textureHeight/240.0f;
  float endyf = (upperLeftVertex[1]-55.0f)*(float)textureHeight/240.0f;
  float height = endyf-startyf;
  float heightFactor = (float)textureHeight/height;
  int starty = (int)startyf;
  int endy = (int)endyf;
  float fractionalHeight = ((midLeftVertex[1]-lowerVertex[1])/
			    (upperLeftVertex[1]-lowerVertex[1]));
  // fractionalHeight iterative increment-step value
  float fhInterval = (1.0f-fractionalHeight)/height;
  float opacity_offset = 2.0f*((opac_x-upperLeftVertex[0])/
			       (upperRightVertex[0]-upperLeftVertex[0]))-1.0f;
  for( int y = starty; y < endy; y++ ) {
    int texture_y = (int)(((float)y-startyf)*heightFactor);
    int texture_x = texture_y*textureWidth/textureHeight;
    // higher precision values for intensity computation
    float startxf = (lowerVertex[0]-5-(lowerVertex[0]-upperLeftVertex[0])*
		     fractionalHeight)*textureWidth/490.0f;
    float endxf = (lowerVertex[0]-5+(upperRightVertex[0]-lowerVertex[0])*
		   fractionalHeight)*textureWidth/490.0f;
    float widthFactor = (float)textureWidth/(endxf-startxf);

    int startx = (int)startxf;
    int endx = (int)endxf;
    // paint one row of this widget's texture onto background texture
    if( !switchFlag )
      for( int x = startx; x < endx; x++ ) {
	int texture_x = (int)((x-startxf)*widthFactor);
	if( texture_x < 0 )
	  texture_x = 0;
	else if( texture_x >= textureWidth )
	  texture_x = textureWidth-1;
	blend( texture_dest[y][x], 
	       transText->current_color[0], 
	       transText->current_color[1], 
	       transText->current_color[2],
	       (transText->textArray[0][texture_x][3]+
		opacity_offset)*master_alpha );
      } // for()
    else
      for( int x = startx; x < endx; x++ )
	blend( texture_dest[y][x],
	       transText->current_color[0],
	       transText->current_color[1],
	       transText->current_color[2],
	       (transText->textArray[0][texture_y][3]+opacity_offset)*master_alpha );

    fractionalHeight += fhInterval;
  } // for
} // paintTransFunc()



// determines whether an (x,y) pair is inside this widget
bool
TriWidget::insideWidget( float x, float y ) {
  // ratio of distances of y-coordinate in question and upperBound from bottom tip
  float fractionalHeight = (y-lowerVertex[1])/(upperLeftVertex[1]-lowerVertex[1]);
  if( y > lowerVertex[1] && y < upperLeftVertex[1] && 
      x >= lowerVertex[0] - (lowerVertex[0]-upperLeftVertex[0])*fractionalHeight && 
      x <= lowerVertex[0] + (upperRightVertex[0]-lowerVertex[0])*fractionalHeight )
    return true;
  else
    return false;
} // insideWidget()



// allows another file to access many of this widget's parameters
void
TriWidget::returnParams( float *p[numWidgetParams] ) {
  p[0] = &upperLeftVertex[0];
  p[1] = &upperLeftVertex[1];
  p[2] = &width;
  p[3] = &midLeftVertex[1];
  p[4] = &color[0];
  p[5] = &color[1];
  p[6] = &color[2];
  p[7] = &alpha; 
  p[8] = &opac_x;
  p[9] = &opac_y;
} // returnParams()



// changes a widget's frame's color
void
TriWidget::changeColor( float r, float g, float b ) {
  shearBar->red = r;
  shearBar->green = g;
  shearBar->blue = b;
  barRounder->red = r;
  barRounder->green = g;
  barRounder->blue = b;
  color[0] = r;
  color[1] = g;
  color[2] = b;
}



// reflects this widget's texture across its diagonal
void
TriWidget::reflectTrans( void ) {
  switchFlag = !switchFlag;
}



// currently has no purpose
void
TriWidget::invertColor( void ) {
  return;
} // invertColor()






// replaces another widget with a rectangular widget
RectWidget::RectWidget( float x, float y, float w, float h, float c[3], float a, 
			int t, float ox, float oy, Texture<GLfloat> *text, int sF ) {
  switchFlag = sF;
  drawFlag = 0;
  width = w;
  height = h;
  upperLeftVertex[0] = x;	upperLeftVertex[1] = y;
  lowerRightVertex[0] = x+w;	lowerRightVertex[1] = y-h;
  color[0] = c[0];
  color[1] = c[1];
  color[2] = c[2];
  alpha = a;
  translateStar = new GLStar( upperLeftVertex[0], upperLeftVertex[1], 
			      5.0, c[0], c[1], c[2] );
  translateBar = new GLBar( upperLeftVertex[0]+width/2, upperLeftVertex[1],
			    width, color[0], color[1], color[2] );
  barRounder = new GLStar( lowerRightVertex[0], upperLeftVertex[1], 
			   5.0, c[0], c[1], c[2] );
  resizeStar = new GLStar( lowerRightVertex[0], lowerRightVertex[1],
			   8.0, c[0]+0.30, c[1], c[2] );
  opac_x = ox;
  opac_y = oy;
  opacityStar = new GLStar( opac_x, opac_y, 6.5, 1-color[0], 1-color[1], 1-color[2] );
  transText = text;
  focus_x = lowerRightVertex[0]-width/2;
  focus_y = lowerRightVertex[1]+height/2;
  focusStar = new GLStar( focus_x, focus_y, 8, 1-transText->current_color[0],
			  1-transText->current_color[1],
			  1-transText->current_color[2] );
  type = t;
  // determines which background texture to make from this widget's type
  switch( t ) {
  case 1:
    transText->makeEllipseTextureImage();
    break;
  case 2:
    transText->makeOneDimTextureImage();
    break;
  case 3:
    transText->makeDefaultTextureImage();
    break;
  } // switch()
} // RectWidget



// RectWidget construction used for restoring widget info from saved UI state
RectWidget::RectWidget( int t, float x, float y, float w, float h, float r, float g,
			float b, float a, float f_x, float f_y, float ox, float oy,
			float t_r, float t_g, float t_b, int t_x, int t_y, int sF ) {
  switchFlag = sF;
  drawFlag = 0;
  type = t;
  width = w;
  height = h;
  color[0] = r;
  color[1] = g;
  color[2] = b;
  alpha = a;
  focus_x = f_x;
  focus_y = f_y;
  opac_x = ox;
  opac_y = oy;
  upperLeftVertex[0] = x;
  upperLeftVertex[1] = y;
  lowerRightVertex[0] = x+w;
  lowerRightVertex[1] = y-h;
  translateStar = new GLStar( upperLeftVertex[0], upperLeftVertex[1], 5.0, r, g, b );
  translateBar = new GLBar( upperLeftVertex[0]+width/2, upperLeftVertex[1],
			    width, r, g, b );
  barRounder = new GLStar( lowerRightVertex[0], upperLeftVertex[1], 5.0, r, g, b );
  resizeStar = new GLStar( lowerRightVertex[0], lowerRightVertex[1],
			   8.0, r+0.30, g, b );
  opacityStar = new GLStar( opac_x, opac_y, 6.5, 1-r, 1-g, 1-b );
  focusStar = new GLStar( focus_x, focus_y, 8, 1-t_r, 1-t_g, 1-t_b );
  transText = new Texture<GLfloat>();
  transText->current_color[0] = t_r;
  transText->current_color[1] = t_g;
  transText->current_color[2] = t_b;
  transText->colormap_x_offset = t_x;
  transText->colormap_y_offset = t_y;
  // determines which background texture to make from this widget's type
  switch( t ) {
  case 1:
    transText->makeEllipseTextureImage();
    break;
  case 2:
    transText->makeOneDimTextureImage();
    break;
  case 3:
    transText->makeDefaultTextureImage();
    break;
  } // switch()
} // RectWidget()



// draws this widget without its texture
void 
RectWidget::draw( void ) {
  glBegin( GL_LINE_LOOP );
  glColor3fv( color );
  glVertex2f( upperLeftVertex[0], upperLeftVertex[1] );
  glVertex2f( lowerRightVertex[0], upperLeftVertex[1] );
  glVertex2f( lowerRightVertex[0], lowerRightVertex[1] );
  glVertex2f( upperLeftVertex[0], lowerRightVertex[1] );
  glEnd();
  translateStar->draw();
  translateBar->draw();
  barRounder->draw();
  resizeStar->draw();
  focusStar->draw();
  opacityStar->draw();
} // draw()



// moves this widget around the screen
void 
RectWidget::translate( float dx, float dy ) {
  // if x and y translations keep widget inside its window,
  //  then a faster computation can be undertaken
  if(upperLeftVertex[0]+dx >= 5.0 && lowerRightVertex[0]+dx <= 495.0 &&
     upperLeftVertex[1]+dy <= 295.0 &&lowerRightVertex[1]+dy >= 55.0 ) {
    translateStar->translate( dx, dy );
    barRounder->translate( dx, dy );
    resizeStar->translate( dx, dy );
    focusStar->translate( dx, dy );
    focus_x += dx;
    focus_y += dy;
    opac_x += dx;
    opac_y += dy;
    opacityStar->translate( dx, dy );
    translateBar->translate( dx, dy );
    upperLeftVertex[0] += dx;
    upperLeftVertex[1] += dy;
    lowerRightVertex[0] += dx;
    lowerRightVertex[1] += dy;
  } // if
  // otherwise each dimension must be inspected separately (slower)
  else {
    if( upperLeftVertex[0]+dx >= 5.0 && lowerRightVertex[0]+dx <= 495.0 ) {
      translateStar->translate( dx, 0 );
      barRounder->translate( dx, 0 );
      resizeStar->translate( dx, 0 );
      focusStar->translate( dx, 0 );
      focus_x += dx;
      opac_x += dx;
      opacityStar->translate( dx, 0 );
      translateBar->translate( dx, 0 );
      upperLeftVertex[0] += dx;
      lowerRightVertex[0] += dx;
    } // if
    else if( upperLeftVertex[1]+dy <= 295.0 && lowerRightVertex[1]+dy >= 55.0 ) {
      translateStar->translate( 0, dy );
      translateBar->translate( 0, dy );
      barRounder->translate( 0, dy );
      resizeStar->translate( 0, dy );
      focusStar->translate( 0, dy );
      focus_y += dy;
      opac_y += dy;
      opacityStar->translate( 0, dy );
      upperLeftVertex[1] += dy;
      lowerRightVertex[1] += dy;
    } // else if
  } // else
} // translate()



// resizes widget while restricting minimum width and height to small positive values
void 
RectWidget::resize( float dx, float dy ) {
  // fractional distance of focusStar across this widget's length from left to right
  float frac_dist = ((focus_x-upperLeftVertex[0])/
		     (lowerRightVertex[0]-upperLeftVertex[0]));
  // restricts width to positive values
  if( lowerRightVertex[0]+dx-3 > upperLeftVertex[0] && 
      lowerRightVertex[0]+dx <= 495.0 && upperLeftVertex[0] >= 5.0 ) {
    frac_dist = ((focus_x-upperLeftVertex[0])/
		 (lowerRightVertex[0]-upperLeftVertex[0]));
    focusStar->translate( dx*frac_dist, 0 );
    focus_x += dx*frac_dist;
    frac_dist = ((opac_x-upperLeftVertex[0])/
		 (lowerRightVertex[0]-upperLeftVertex[0]));
    opac_x += dx*frac_dist;
    opacityStar->translate( dx*frac_dist, 0 );
    width += dx;
    lowerRightVertex[0] += dx;
    resizeStar->translate( dx, 0 );
    translateBar->translate( dx/2, 0 );
    translateBar->resize( dx/2, 0.0f );
    barRounder->translate( dx, 0 );
  } // if
  // restricts height to positive values
  if( lowerRightVertex[1]+dy+3 < upperLeftVertex[1] && 
      lowerRightVertex[1]+dy >= 55.0 && upperLeftVertex[1]+dy <= 295.0) {
    frac_dist = 1-((focus_y-lowerRightVertex[1])/
		   (upperLeftVertex[1]-lowerRightVertex[1]));
    height -= dy;
    lowerRightVertex[1] += dy;
    resizeStar->top += dy;
    focusStar->top += dy*frac_dist;
    focus_y += dy*frac_dist;
  } // if
} // resize()



// moves the focusStar around inside the widget
void
RectWidget::adjustFocus( float dx, float dy ) {
  if( focus_x + dx >= upperLeftVertex[0] && focus_x + dx <= lowerRightVertex[0] ) {
    focus_x += dx;
    focusStar->translate( dx, 0 );
  } // if
  if( focus_y + dy >= lowerRightVertex[1] && focus_y + dy <= upperLeftVertex[1] ) {
    focus_y += dy;
    focusStar->translate( 0, dy );
  } // if
} // adjustFocus()



// moves opacityStar along translateBar and adjusts widget's texture's overall opacity
void
RectWidget::adjustOpacity( float dx, float dy ) {
  // if opacityStar remains inside translateBar
  if( opac_x+dx <= lowerRightVertex[0] && opac_x+dx >= upperLeftVertex[0] ) {
    opac_x += dx;
    opacityStar->translate( dx, 0 );
  } // if
} // adjustOpacity()



// controls which way this widget is manipulated
void 
RectWidget::manipulate( float x, float dx, float y, float dy ) {
  // the following block of if statements allow for continuous manipulation
  //  without conducting parameter checks every time (quicker)
  if( drawFlag == 1 )
    adjustOpacity( dx, dy );
  else if( drawFlag == 2 )
    adjustFocus( dx, dy );
  else if( drawFlag == 3 )
    resize( dx, dy );
  else if( drawFlag == 4 )
    translate( dx, dy );

  // if drawFlag has not been set from main, then a parameter check must be
  //  conducted to determine in which way the user wishes to manipulate
  //  the widget (slower)
  else {
    // if mouse cursor near opacityStar
    if( x >= opac_x - 5 && x <= opac_x + 5 &&
	y >= opac_y - 5 && y <= opac_y + 5 ) {
      drawFlag = 1;
      adjustOpacity( dx, dy );
    } // if
    // if mouse cursor near focusStar
    else if( x >= focus_x - 5 && x <= focus_x + 5 &&
	     y >= focus_y - 5 && y <= focus_y + 5 ) {
      drawFlag = 2;
      adjustFocus( dx, dy );
    } // else if
    // if mouse cursor near resizeStar
    else if( x >= lowerRightVertex[0] - 5 && x <= lowerRightVertex[0] + 5 &&
	     y >= lowerRightVertex[1] - 5 && y <= lowerRightVertex[1] + 5 ) {
      drawFlag = 3;
      resize( dx, dy );
    } // else if
    // if mouse cursor on translateBar
    else if( x >= upperLeftVertex[0] - 5 && x <= lowerRightVertex[0] + 5 &&
	     y >= upperLeftVertex[1] - 5 && y <= upperLeftVertex[1] + 5 ) {
      drawFlag = 4;
      translate( dx, dy );
    } // else if
    // otherwise nothing pertinent was selected
    else {
      drawFlag = 0;
      return;
    } // else
  } // else
} // manipulate()



// inverts focusStar's color to make it visible in front of widget's texture
void
RectWidget::invertColor( void ) {
  focusStar->invertColor( transText->current_color );
} // invertColor()



// reflects this widget's texture across its diagonal
void
RectWidget::reflectTrans( void ) {
  switchFlag = !switchFlag;
}



// paints this widget's texture onto a background texture
void
RectWidget::paintTransFunc( GLfloat texture_dest[textureHeight][textureWidth][4],
			    float master_alpha ) {
  float startxf = (upperLeftVertex[0]-5.0f)*(float)textureWidth/490.0f;
  float endxf = (lowerRightVertex[0]-5.0f)*(float)textureWidth/490.0f;
  int startx = (int)startxf;
  int endx = (int)endxf;
  float startyf = (float)textureHeight*(lowerRightVertex[1]-55.0f)/240.0f;
  float endyf = (float)textureHeight*(upperLeftVertex[1]-55.0f)/240.0f;
  int starty = (int)startyf;
  int endy = (int)endyf;
  float midx = (endxf+startxf)*0.5f;
  float midy = (endyf+startyf)*0.5f;
  float opacStar_alpha_off = 2.0f*((opac_x-upperLeftVertex[0])/
				(lowerRightVertex[0]-upperLeftVertex[0]))-1.0f; 
  float height = endyf-startyf;
  float width = endxf-startxf;
  float halfWidth = width*0.5f;
  float halfHeight = height*0.5f;
  float half_x = (focus_x-upperLeftVertex[0])/this->width*width+startxf;
  float half_y = ((focus_y-(upperLeftVertex[1]-this->height))/
		  this->height*height+startyf);
  float alpha_x_off = 2.0f*(focus_x-upperLeftVertex[0])/this->width-1.0f;
  float alpha_y_off = 2.0f*(focus_y-lowerRightVertex[1])/this->height-1.0f;

  switch( type ) {
    // elliptical texture
  case 1:
    for( int y = starty; y < endy; y++ ) {
      for( int x = startx; x < endx; x++ ) {
	float intensity = 1.0f - (2.0f*(x-half_x)*(x-half_x)/(halfWidth*halfWidth)+
				  2.0f*(y-half_y)*(y-half_y)/(halfHeight*halfHeight));
	if( intensity < 0.0f )
	  intensity = 0.0f;
	blend( texture_dest[y][x], 
	       transText->current_color[0], 
	       transText->current_color[1], 
	       transText->current_color[2],
	       (intensity+opacStar_alpha_off)*master_alpha );
      } // for()
    } // for()
    break;
  case 2:
    for( int y = starty; y < endy; y++ ) {
      if( !switchFlag )
	for( int x = startx; x < endx; x++ ) {
	  int texture_x = (int)(((float)x-startxf)/width*(float)textureWidth);
	  if( texture_x >= textureWidth )
	    texture_x = textureWidth-1;
	  else if( texture_x < 0 )
	    texture_x = 0;
	  blend( texture_dest[y][x],
		 transText->current_color[0],
		 transText->current_color[1],
		 transText->current_color[2],
		 (transText->textArray[0][texture_x][3]+opacStar_alpha_off+
		  alpha_x_off*((float)x-midx)/width)*master_alpha );
	} // for()
      else {
	float y_alpha = alpha_y_off*((float)y-midy)/height;
	for( int x = startx; x < endx; x++ ) {
	  int texture_x = (int)(((float)y-startyf)/height*(float)textureWidth);
	  if( texture_x >= textureWidth )
	    texture_x = textureWidth-1;
	  else if( texture_x < 0 )
	    texture_x = 0;
	  blend( texture_dest[y][x],
		 transText->current_color[0],
		 transText->current_color[1],
		 transText->current_color[2],
		 (transText->textArray[0][texture_x][3]+opacStar_alpha_off+
		  y_alpha)*master_alpha );
	  } // for()
      } // else
    } // for()
    break;
    // rainbow texture
  case 3:
    float x_alpha = alpha_x_off/width;
    for( int y = starty; y < endy; y++ ) {
      float y_alpha = alpha_y_off*((float)y-midy)/height;
      float init_intensity = 0.5f+opacStar_alpha_off+y_alpha;
      if( !switchFlag )
	for( int x = startx; x < endx; x++ ) {
	  int texture_x = (int)(((float)x-startxf)/width*textureWidth);
	  if( texture_x >= textureWidth )
	    texture_x = textureWidth-1;
	  else if( texture_x < 0 )
	    texture_x = 0;
	  blend( texture_dest[y][x],
		 transText->textArray[0][texture_x][0],
		 transText->textArray[0][texture_x][1],
		 transText->textArray[0][texture_x][2],
		 (init_intensity+x_alpha*((float)x-midx))*master_alpha );
	} // for()
      else {
	int texture_x = (int)(((float)y-startyf)/height*textureWidth);
	if( texture_x >= textureWidth )
	  texture_x = textureWidth-1;
	else if( texture_x < 0 )
	  texture_x = 0;
	for( int x = startx; x < endx; x++ )
	  blend( texture_dest[y][x],
		 transText->textArray[0][texture_x][0],
		 transText->textArray[0][texture_x][1],
		 transText->textArray[0][texture_x][2],
		 (init_intensity+x_alpha*((float)x-midx))*master_alpha );
      } // else
    } // for()
    break;
  } // switch()
} // paintTransFunc()



// determines whether an (x,y) pair is inside this widget
bool
RectWidget::insideWidget( float x, float y ) {
  if( x >= upperLeftVertex[0] && x <= lowerRightVertex[0] && 
      y >= lowerRightVertex[1] && y <= upperLeftVertex[1] )
    return true;
  else
    return false;
} // insideWidget()



// allows another file to acces many of this widget's parameters
void
RectWidget::returnParams( float *p[numWidgetParams] ) {
  p[0] = &upperLeftVertex[0];
  p[1] = &upperLeftVertex[1];
  p[2] = &width;
  p[3] = &height;
  p[4] = &color[0];
  p[5] = &color[1];
  p[6] = &color[2];
  p[7] = &alpha;
  p[8] = &opac_x;
  p[9] = &opac_y;
} // returnParams()



// changes this widget's frame's color
void
RectWidget::changeColor( float r, float g, float b ) {
  translateStar->red = r;
  translateStar->green = g;
  translateStar->blue = b;
  barRounder->red = r;
  barRounder->green = g;
  barRounder->blue = b;
  translateBar->red = r;
  translateBar->green = g;
  translateBar->blue = b;
  color[0] = r;
  color[1] = g;
  color[2] = b;
}



// blends the RGBA components of widget's texture with background texture
void
Widget::blend( GLfloat texture_dest[4], float r, float g, float b, float a ) {
  if( a < 0 )
    a = 0;
  else if( a > 1 )
    a = 1;
  texture_dest[0] = a*r + (1-a)*texture_dest[0];
  texture_dest[1] = a*g + (1-a)*texture_dest[1];
  texture_dest[2] = a*b + (1-a)*texture_dest[2];
  texture_dest[3] = a + (1-a)*texture_dest[3];
} // blend()
