
/*
 *  BaseConstraint.h
 *
 *  Written by:
 *   James Purciful
 *   Department of Computer Science
 *   University of Utah
 *   Aug. 1994
 *
 *  Copyright (C) 1994 SCI Group
 */


#ifndef SCI_project_Base_Constraint_h
#define SCI_project_Base_Constraint_h 1

#include <Dataflow/share/share.h>
#include <Dataflow/Constraints/BaseVariable.h>
#include <Core/Containers/Array1.h>
#include <Core/Containers/Array2.h>
#include <Core/Containers/String.h>

namespace SCIRun {


class PSECORESHARE BaseConstraint {
   friend class BaseVariable;
   friend class ConstraintSolver;
public:
   BaseConstraint( const clString& name, const Index numSchemes,
		   const Index VariableCount );
   virtual ~BaseConstraint();

   // Use this to define the priorities of this constraint in relation
   // to each of its variables.
   // (This is a cheating way for varargs...)
   void Priorities( const VPriority p1 = P_Default,
		    const VPriority p2 = P_Default,
		    const VPriority p3 = P_Default,
		    const VPriority p4 = P_Default,
		    const VPriority p5 = P_Default,
		    const VPriority p6 = P_Default,
		    const VPriority p7 = P_Default );
   // Use this to define the variable to change to fulfill the constraint
   // given the variable that requested re-satisfication.
   // (This is a cheating way for varargs...)
   void VarChoices( const Scheme scheme,
		    const Index i1 = 0,
		    const Index i2 = 0,
		    const Index i3 = 0,
		    const Index i4 = 0,
		    const Index i5 = 0,
		    const Index i6 = 0,
		    const Index i7 = 0 );

   void print( std::ostream& os );
   void printc( std::ostream& os, const Scheme scheme );

protected:
   clString name;
   Index nschemes;
   
   Index varCount;
   Array1<BaseVariable*> vars;
   Array1<Index> var_indexs; // The var's index for this constraint.
   Array2<Index> var_choices;
   Index whichMethod, callingMethod;

   void Register();
   inline Index ChooseChange( const Index index, const Scheme scheme );
   virtual int Satisfy( const Index index, const Scheme scheme, const Real Epsilon,
			BaseVariable*& var, VarCore& c );
};
inline PSECORESHARE std::ostream& operator<<( std::ostream& os, BaseConstraint& v );


/***************************************************************************
 * The BaseConstraint ChooseChange method is used to select which variable
 *      should be altered to maintain the constraint.
 */
inline Index
BaseConstraint::ChooseChange( const Index index, const Scheme scheme )
{
   return whichMethod = var_choices(scheme, callingMethod = index);
}

inline std::ostream&
operator<<( std::ostream& os, BaseConstraint& c )
{
   c.print(os);
   return os;
}

} // End namespace SCIRun


#endif
