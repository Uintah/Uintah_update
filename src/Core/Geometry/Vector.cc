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
 *  Vector.cc: ?
 *
 *  Written by:
 *   Author?
 *   Department of Computer Science
 *   University of Utah
 *   Date?
 *
 *  Copyright (C) 199? SCI Group
 */

#include <Core/Disclosure/TypeDescription.h>
#include <Core/Geometry/Vector.h>
#include <Core/Geometry/Point.h>
#include <Core/Util/Assert.h>
#include <Core/Persistent/Persistent.h>
#include <Core/Math/Expon.h>
#include <Core/Math/MiscMath.h>
#include <iostream>
#include <stdio.h>

using std::istream;
using std::ostream;

namespace SCIRun {

string
Vector::get_string() const
{
    char buf[100];
    sprintf(buf, "[%g, %g, %g]", _x, _y, _z);
    return buf;
}

void
Vector::find_orthogonal(Vector& v1, Vector& v2) const
{
    Vector v0(Cross(*this, Vector(1,0,0)));
    if(v0.length2() == 0){
	v0=Cross(*this, Vector(0,1,0));
    }
    v1=Cross(*this, v0);
    v1.normalize();
    v2=Cross(*this, v1);
    v2.normalize();
}

Vector
Vector::normal() const
{
   Vector v(*this);
   v.normalize();
   return v;			// 
}

ostream& operator<<( ostream& os, const Vector& v )
{
   os << v.get_string();
   return os;
}

istream& operator>>( istream& is, Vector& v)
{
  double x, y, z;
  char st;
  is >> st >> x >> st >> y >> st >> z >> st;
  v=Vector(x,y,z);
  return is;
}

int
Vector::operator== ( const Vector& v ) const
{
    return v._x == _x && v._y == _y && v._z == _z;
}

int Vector::operator!=(const Vector& v) const
{
    return v._x != _x || v._y != _y || v._z != _z;
}

void
Pio(Piostream& stream, Vector& p)
{

    stream.begin_cheap_delim();
    Pio(stream, p._x);
    Pio(stream, p._y);
    Pio(stream, p._z);
    stream.end_cheap_delim();
}

void Vector::rotz90(const int c)
{
    // Rotate by c*90 degrees counter clockwise
    switch(c%4){
    case 0:
	// 0 degrees, do nothing
	break;
    case 1:
	// 90 degrees
	{
	    double newx=-_y;
	    _y=_x;
	    _x=newx;
	}
	break;
    case 2:
	// 180 degrees
	_x=-_x;
	_y=-_y;
	break;
    case 3:
	// 270 degrees
	{
	    double newy=-_x;
	    _x=_y;
	    _y=newy;
	}
	break;
    }
}

const TypeDescription* get_type_description(Vector*)
{
  static TypeDescription* td = 0;
  if(!td){
    td = scinew TypeDescription("Vector", Vector::get_h_file_path(), "SCIRun");
  }
  return td;
}

} // End namespace SCIRun


