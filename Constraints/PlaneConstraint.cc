
/*
 *  Plane4Constraint.h
 *
 *  Written by:
 *   James Purciful
 *   Department of Computer Science
 *   University of Utah
 *   Aug. 1994
 *
 *  Copyright (C) 1994 SCI Group
 */


#include <Constraints/PlaneConstraint.h>
#include <Geometry/Plane.h>
#include <Geometry/Vector.h>
#include <Classlib/Debug.h>

static DebugSwitch pc_debug("Constraints", "Plane");

PlaneConstraint::PlaneConstraint( const clString& name,
				  const Index numSchemes,
				  PointVariable* p1, PointVariable* p2,
				  PointVariable* p3, PointVariable* p4)
:BaseConstraint(name, numSchemes, 4)
{
   vars[0] = p1;
   vars[1] = p2;
   vars[2] = p3;
   vars[3] = p4;

   whichMethod = 0;

   // Tell the variables about ourself.
   Register();
};

PlaneConstraint::~PlaneConstraint()
{
}


int
PlaneConstraint::Satisfy( const Index index, const Scheme scheme, const Real Epsilon,
			  BaseVariable*& var, VarCore& c )
{
   PointVariable& p1 = *vars[0];
   PointVariable& p2 = *vars[1];
   PointVariable& p3 = *vars[2];
   PointVariable& p4 = *vars[3];
   Vector vec1, vec2;

   if (pc_debug) {
      ChooseChange(index, scheme);
      print();
   }
   
   switch (ChooseChange(index, scheme)) {
   case 0:
      vec1 = ((Point)p2 - p3);
      vec2 = ((Point)p4 - p3);
      if (Cross(vec1, vec2).length2() < Epsilon) {
	 if (pc_debug) cerr << "No Plane." << endl;
      } else {
	 Plane plane(p2, p3, p4);
	 var = vars[0];
	 c = plane.project(p1);
	 return 1;
      }
      break;
   case 1:
      vec1 = ((Point)p1 - p3);
      vec2 = ((Point)p4 - p3);
      if (Cross(vec1, vec2).length2() < Epsilon) {
	 if (pc_debug) cerr << "No Plane." << endl;
      } else {
	 Plane plane(p1, p3, p4);
	 var = vars[1];
	 c = plane.project(p2);
	 return 1;
      }
      break;
   case 2:
      vec1 = ((Point)p1 - p2);
      vec2 = ((Point)p4 - p2);
      if (Cross(vec1, vec2).length2() < Epsilon) {
	 if (pc_debug) cerr << "No Plane." << endl;
      } else {
	 Plane plane(p1, p2, p4);
	 var = vars[2];
	 c = plane.project(p3);
	 return 1;
      }
      break;
   case 3:
      vec1 = ((Point)p1 - p2);
      vec2 = ((Point)p3 - p2);
      if (Cross(vec1, vec2).length2() < Epsilon) {
	 if (pc_debug) cerr << "No Plane." << endl;
      } else {
	 Plane plane(p1, p2, p3);
	 var = vars[3];
	 c = plane.project(p4);
	 return 1;
      }
      break;
   default:
      cerr << "Unknown variable in Plane Constraint!" << endl;
      break;
   }
   return 0;
}

