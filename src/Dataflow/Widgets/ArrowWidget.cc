//static char *id="@(#) $Id$";

/*
 *  ArrowWidget.cc
 *
 *  Written by:
 *   James Purciful
 *   Department of Computer Science
 *   University of Utah
 *   Aug. 1994
 *
 *  Copyright (C) 1994 SCI Group
 */


#include <PSECore/Widgets/ArrowWidget.h>
#include <SCICore/Geom/GeomCone.h>
#include <SCICore/Geom/GeomCylinder.h>
#include <SCICore/Geom/GeomSphere.h>
#include <SCICore/Geom/GeomGroup.h>
#include <SCICore/Geom/GeomPick.h>
#include <SCICore/Malloc/Allocator.h>
#include <PSECore/Dataflow/Module.h>

namespace PSECore {
namespace Widgets {

using SCICore::GeomSpace::GeomGroup;
using SCICore::GeomSpace::GeomSphere;
using SCICore::GeomSpace::GeomCylinder;
using SCICore::GeomSpace::GeomCone;
using SCICore::GeomSpace::GeomCappedCone;

using namespace PSECore::Constraints;

const Index NumCons = 0;
const Index NumVars = 1;
const Index NumGeoms = 3;
const Index NumPcks = 3;
const Index NumMatls = 3;
const Index NumMdes = 2;
const Index NumSwtchs = 3;
// const Index NumSchemes = 1;

enum { GeomPoint, GeomShaft, GeomHead };
enum { PointP, ShaftP, HeadP };

/***************************************************************************
 * The constructor initializes the widget's constraints, variables,
 *      geometry, picks, materials, modes, switches, and schemes.
 * Variables and constraints are initialized as a function of the
 *      widget_scale.
 * Much of the work is accomplished in the BaseWidget constructor which
 *      includes some consistency checking to ensure full initialization.
 */
ArrowWidget::ArrowWidget( Module* module, CrowdMonitor* lock, double widget_scale )
: BaseWidget(module, lock, "ArrowWidget", NumVars, NumCons, NumGeoms, NumPcks, NumMatls, NumMdes, NumSwtchs, widget_scale),
  direction(0, 0, 1)
{
   variables[PointVar] = scinew PointVariable("Point", solve, Scheme1, Point(0, 0, 0));

   geometries[GeomPoint] = scinew GeomSphere;
   picks[PointP] = scinew GeomPick(geometries[GeomPoint], module, this, PointP);
   picks[PointP]->set_highlight(DefaultHighlightMaterial);
   materials[PointMatl] = scinew GeomMaterial(picks[PointP], DefaultPointMaterial);
   CreateModeSwitch(0, materials[PointMatl]);
   geometries[GeomShaft] = scinew GeomCylinder;
   picks[ShaftP] = scinew GeomPick(geometries[GeomShaft], module, this, ShaftP);
   picks[ShaftP]->set_highlight(DefaultHighlightMaterial);
   materials[ShaftMatl] = scinew GeomMaterial(picks[ShaftP], DefaultEdgeMaterial);
   CreateModeSwitch(1, materials[ShaftMatl]);
   geometries[GeomHead] = scinew GeomCappedCone;
   picks[HeadP] = scinew GeomPick(geometries[GeomHead], module, this, HeadP);
   picks[HeadP]->set_highlight(DefaultHighlightMaterial);
   materials[HeadMatl] = scinew GeomMaterial(picks[HeadP], DefaultEdgeMaterial);
   CreateModeSwitch(2, materials[HeadMatl]);

   SetMode(Mode0, Switch0|Switch1|Switch2);
   SetMode(Mode1, Switch0);
   FinishWidget();
}


/***************************************************************************
 * The destructor frees the widget's allocated structures.
 * The BaseWidget's destructor frees the widget's constraints, variables,
 *      geometry, picks, materials, modes, switches, and schemes.
 * Therefore, most widgets' destructors will not need to do anything.
 */
ArrowWidget::~ArrowWidget()
{
}


/***************************************************************************
 * The widget's redraw method changes widget geometry to reflect the
 *      widget's variable values and its widget_scale.
 * Geometry should only be changed if the mode_switch that displays
 *      that geometry is active.
 * Redraw should also set the principal directions for all picks.
 * Redraw should never be called directly; the BaseWidget execute method
 *      calls redraw after establishing the appropriate locks.
 */
void
ArrowWidget::redraw()
{
   if (mode_switches[0]->get_state()) {
      Point center(variables[PointVar]->point());
      Vector direct(direction*widget_scale);
      ((GeomSphere*)geometries[GeomPoint])->move(center, widget_scale);

      if (direct.length2() > 1.e-6) {
	 ((GeomCylinder*)geometries[GeomShaft])->move(center,
						      center + direct * 3.0,
						      0.5*widget_scale);
	 ((GeomCappedCone*)geometries[GeomHead])->move(center + direct * 3.0,
						       center + direct * 5.0,
						       widget_scale,
						       0);
      }
   }

   if (mode_switches[1]->get_state()) {
       Point center(variables[PointVar]->point());
       ((GeomSphere*)geometries[GeomPoint])->move(center, widget_scale);
   }

   Vector v1, v2;
   if(direction.length2() > 1.e-6){
       direction.find_orthogonal(v1, v2);
       for (Index geom = 0; geom < NumPcks; geom++) {
	   picks[geom]->set_principal(direction, v1, v2);
       }
   }
}


/***************************************************************************
 * The widget's geom_moved method receives geometry move requests from
 *      the widget's picks.  The widget's variables must be altered to
 *      reflect these changes based upon which pick made the request.
 * No more than one variable should be Set since this triggers solution of
 *      the constraints--multiple Sets could lead to inconsistencies.
 *      The constraint system only requires that a variable be Set if the
 *      change would cause a constraint to be invalid.  For example, if
 *      all PointVariables are moved by the same delta, then no Set is
 *      required.
 * The last line of the widget's geom_moved method should call the
 *      BaseWidget execute method (which calls the redraw method).
 */
void
ArrowWidget::geom_moved( GeomPick*, int /* axis */, double /* dist */,
			 const Vector& delta, int pick, const BState& )
{
    switch(pick){
    case PointP: 
    case ShaftP: 
    case HeadP:
	MoveDelta(delta);
	break;
    }
    execute(0);
}


/***************************************************************************
 * This standard method simply moves all the widget's PointVariables by
 *      the same delta.
 * The last line of this method should call the BaseWidget execute method
 *      (which calls the redraw method).
 */
void
ArrowWidget::MoveDelta( const Vector& delta )
{
   variables[PointVar]->MoveDelta(delta);

   execute(1);
}


/***************************************************************************
 * This standard method returns a reference point for the widget.  This
 *      point should have some logical meaning such as the center of the
 *      widget's geometry.
 */
Point
ArrowWidget::ReferencePoint() const
{
   return variables[PointVar]->point();
}


void
ArrowWidget::SetPosition( const Point& p )
{
   variables[PointVar]->Move(p);

   execute(0);
}


Point
ArrowWidget::GetPosition() const
{
   return variables[PointVar]->point();
}


void
ArrowWidget::SetDirection( const Vector& v )
{
   direction = v;

   execute(0);
}


const Vector&
ArrowWidget::GetDirection() const
{
   return direction;
}


/***************************************************************************
 * This standard method returns a string describing the functionality of
 *      a widget's material property.  The string is used in the 
 *      BaseWidget UI.
 */
clString
ArrowWidget::GetMaterialName( const Index mindex ) const
{
   ASSERT(mindex<NumMaterials);
   
   switch(mindex){
   case 0:
      return "Point";
   case 1:
      return "Shaft";
   case 2:
      return "Head";
   default:
      return "UnknownMaterial";
   }
}


void
ArrowWidget::widget_tcl( TCLArgs& args )
{
   if (args[1] == "translate"){
      if (args.count() != 4) {
	 args.error("arrow widget needs axis translation");
	 return;
      }
      Real trans;
      if (!args[3].get_double(trans)) {
	 args.error("arrow widget can't parse translation `"+args[3]+"'");
	 return;
      }
      Point p(GetPosition());
      switch (args[2](0)) {
      case 'x':
	 p.x(trans);
	 break;
      case 'y':
	 p.y(trans);
	 break;
      case 'z':
	 p.z(trans);
	 break;
      default:
	 args.error("arrow widget unknown axis `"+args[2]+"'");
	 break;
      }
      SetPosition(p);
   }
}

} // End namespace Widgets
} // End namespace PSECore

//
// $Log$
// Revision 1.3  1999/09/02 04:44:58  dmw
// added a mode
//
// Revision 1.2  1999/08/17 06:38:27  sparker
// Merged in modifications from PSECore to make this the new "blessed"
// version of SCIRun/Uintah.
//
// Revision 1.1  1999/07/27 16:56:04  mcq
// Initial commit
//
// Revision 1.1.1.1  1999/04/24 23:12:29  dav
// Import sources
//
//
