/*
 *  Graph.cc:
 *
 *  Written by:
 *   Yarden Livnat
 *   July 20001
 *
 */



#include <iostream>
using std::cin;
using std::endl;
#include <sstream>
using std::ostringstream;

#include <tcl.h>
#include <tk.h>

#include <Core/2d/Graph.h>
#include <Core/Malloc/Allocator.h>
#include <Core/GuiInterface/TclObj.h>
#include <Core/GuiInterface/TCLTask.h>
#include <Core/GuiInterface/TCL.h>

namespace SCIRun {


ObjInfo::ObjInfo( const string &name, Drawable *d)  
{
  name_ = name;
  obj_ = d;
  mapped_ = false;
}

void 
ObjInfo::set_window( const string &window)
{
  TclObj *t = dynamic_cast<TclObj *>(obj_);
  if ( t )
    t->set_window( window );
}

void 
ObjInfo::set_id( const string &id)
{
  TclObj *t = dynamic_cast<TclObj *>(obj_);
  if ( t )
    t->set_id(id);
}


Graph::Graph( const string &id )
  : TclObj( "Graph" ), Drawable( id ), gl_window("gl-window", id, this )
{
  obj_ = 0;

  set_id( id );

  ogl_ = scinew OpenGLWindow;
  ogl_->set_id( id + "-gl" );
}

void
Graph::set_window( const string &window )
{
  TclObj::set_window( window );
  reset_vars();
  ogl_->set_window( gl_window.get(), id());
  
  if ( obj_ ) {
    obj_->obj_->set_opengl( ogl_ );
    TclObj *to = dynamic_cast<TclObj *>(obj_->obj_);
    if ( to ) {
      to->set_window( window );
      if ( ogl_ ) ogl_->command( string(" setobj ") + to->id() );
    }
  }
}

void
Graph::add( const string &name, Drawable *d )
{
  d->set_parent( this );

  if ( obj_ ) delete obj_;

  obj_ = scinew ObjInfo (name, d );

  obj_->set_id( id() + "-obj" );
  if ( ogl_->initialized() &&  window() != "" ) {
    obj_->obj_->set_opengl( ogl_ );
    TclObj *to = dynamic_cast<TclObj *>(obj_->obj_);
    if ( to ) {
      to->set_window( window() );
      command( string(" setobj ") + to->id() );
    }
  }
}


void
Graph::need_redraw() 
{
  update();
}

void
Graph::update()
{
  if ( !ogl_->initialized() || !obj_) {
    return; 
  }
  
  ogl_->pre();
  ogl_->clear();
  obj_->draw();
  ogl_->post();
}

void
Graph::tcl_command(TCLArgs& args, void* userdata)
{
  if ( args[1] == "redraw" ) {
    if ( ogl_->initialized() ) 
      update();
  }
}


#define GRAPH_VERSION 1

void
Graph::io(Piostream& stream)
{
  stream.begin_class( "Graph", GRAPH_VERSION);


  stream.end_class();
}

} // End namespace SCIRun


