
/*
 *  DistanceConstraint.h
 *
 *  Written by:
 *   James Purciful
 *   Department of Computer Science
 *   University of Utah
 *   Aug. 1994
 *
 *  Copyright (C) 1994 SCI Group
 */


#include <Constraints/DistanceConstraint.h>
#include <Geometry/Vector.h>
#include <Classlib/Debug.h>

static DebugSwitch dc_debug("Constraints", "Distance");

DistanceConstraint::DistanceConstraint( const clString& name,
					const Index numSchemes,
					PointVariable* p1, PointVariable* p2,
					RealVariable* dist )
:BaseConstraint(name, numSchemes, 3),
 guess(1, 0, 0), minimum(0.0)
{
   vars[0] = p1;
   vars[1] = p2;
   vars[2] = dist;
   whichMethod = 0;

   // Tell the variables about ourself.
   Register();
}

DistanceConstraint::~DistanceConstraint()
{
}


int
DistanceConstraint::Satisfy( const Index index, const Scheme scheme, const Real Epsilon,
			     BaseVariable*& var, VarCore& c )
{
   PointVariable& p1 = *vars[0];
   PointVariable& p2 = *vars[1];
   RealVariable& dist = *vars[2];
   Vector v;
   Real t;

   if (dc_debug) {
      ChooseChange(index, scheme);
      printc(cout, scheme);
   }
   
   switch (ChooseChange(index, scheme)) {
   case 0:
      v = ((Point)p1 - p2);
      if (v.length2() < Epsilon)
	 v = guess;
      else
	 v.normalize();
      if (dist < minimum) {
	 t = minimum;
      } else
	 t = dist;
      var = vars[0];
      c = (Point)p2 + (v * t);
      return 1;
   case 1:
      v = ((Point)p2 - p1);
      if (v.length2() < Epsilon)
	 v = guess;
      else
	 v.normalize();
      if (dist < minimum) {
	 t = minimum;
      } else
	 t = dist;
      var = vars[1];
      c = (Point)p1 + (v * t);
      return 1;
   case 2:
      t = ((Point)p2 - p1).length();
      if (t < minimum) {
	 t = minimum;
      }
      var = vars[2];
      c = t;
      return 1;
   default:
      cerr << "Unknown variable in Distance Constraint!" << endl;
      break;
   }
   return 0;
}


void
DistanceConstraint::SetDefault( const Vector& v )
{
   guess = v;
}


void
DistanceConstraint::SetMinimum( const Real min )
{
   ASSERT(min>0.0);

   minimum = min;
}


