/*
  For more information, please see: http://software.sci.utah.edu

  The MIT License

  Copyright (c) 2004 Scientific Computing and Imaging Institute,
  University of Utah.

  License for the specific language governing rights and limitations under
  Permission is hereby granted, free of charge, to any person obtaining a
  copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation
  the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the
  Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
  DEALINGS IN THE SOFTWARE.
*/


/*
 *  MeshWindow.cc:
 *
 *  Written by:
 *   Keming Zhang
 *   Department of Computer Science
 *   University of Utah
 *   May 2002
 *
 */

#include <math.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>

#include <CCA/Components/Tri/MeshWindow.h>
#include <CCA/Components/Tri/Delaunay.h>
#include <Core/CCA/datawrapper/vector2d.h>
#include <Core/CCA/spec/cca_sidl.h>

namespace Tri {

MeshWindow::MeshWindow(wxWindow *parent, Delaunay *mesh)
  : wxFrame(parent, wxID_ANY, wxT("Tri::MeshWindow")), mesh(mesh), border(20)
{
  //   w=h=500;
  //   setGeometry(QRect(200,200,w,h));
}

//define a smaller window height
int MeshWindow::height()
{
  int w, h;
  GetSize(&w, &h);
  return h - border * 2;
}

//define a smaller window width
int MeshWindow::width()
{
  int w, h;
  GetSize(&w, &h);
  return w - border * 2;
}


//plot the triangles and/or cirlcles
void MeshWindow::OnPaint(wxPaintEvent& event)
{
//   QFrame::paintEvent(e);
//   QPainter p(this);

//   int r=5;
//   std::vector<vector2d> pts=mesh->getNodes();
//   std::vector<Triangle> tri=mesh->getTriangles();
//   std::vector<Circle> circles=mesh->getCircles();
//   std::vector<Boundary> boundaries=mesh->getBoundaries();

//   if(pts.empty()) return;

//   p.setBrush(NoBrush);
//   for(unsigned i=0;i<circles.size();i++){
//     //check if the triangle does not contain the bounding vertices
//     //if not, plot it.
//     if(true){
//       //(tri[i].index[0]>=4 && tri[i].index[1]>=4 && tri[i].index[0]>=4){

//       //int x=meshToWindowX(circles[i].center.x);
//       //int y=meshToWindowY(circles[i].center.y);
//       //int rx=int(circles[i].radius*width()/mesh->width());
//       //int ry=int(circles[i].radius*height()/mesh->height());


//       int x1=meshToWindowX(pts[tri[i].index[0]].x);
//       int y1=meshToWindowY(pts[tri[i].index[0]].y);
//       int x2=meshToWindowX(pts[tri[i].index[1]].x);
//       int y2=meshToWindowY(pts[tri[i].index[1]].y);
//       int x3=meshToWindowX(pts[tri[i].index[2]].x);
//       int y3=meshToWindowY(pts[tri[i].index[2]].y);
//       p.setPen(red);

//       p.drawLine(x1, y1, x2, y2);
//       p.drawLine(x2, y2, x3, y3);
//       p.drawLine(x3, y3, x1, y1);
//     }
//   }

//   p.setBrush(black);
//   for(unsigned i=0;i<pts.size();i++){
//     int x=meshToWindowX(pts[i].x);
//     int y=meshToWindowY(pts[i].y);
//     p.drawEllipse(x-r, y-r, r+r, r+r);
//     char s[5];
//     sprintf(s,"%d",i-4);
//     p.drawText(x+r,y+r,s);
//   }

//   //plot boundaries
//   p.setBrush(green);
//   for(unsigned b=0; b<boundaries.size(); b++){
//     for(unsigned i=0;i<boundaries[b].index.size();i++){
//       int x=meshToWindowX(pts[boundaries[b].index[i]].x);
//       int y=meshToWindowY(pts[boundaries[b].index[i]].y);
//       p.drawEllipse(x-r, y-r, r+r, r+r);

//     }
//   }
}


//left button to add one node
#if 0
// void MeshWindow::mousePressEvent( QMouseEvent * e)
// {
//   /*  QPoint p=e->pos();
//       if(e->button()==LeftButton){
//       //vector2d v=vector2d(windowToMeshX(p.x()), windowToMeshY(p.y()));
//       //mesh->addNode(v);
//       mesh->triangulation();
//       update();
//       }
//       else if(e->button()==RightButton){

//       }
//   */
// }
#endif

int MeshWindow::meshToWindowX(double x)
{
  return border + int( (x - mesh->minX()) * width() / mesh->width());
}

int MeshWindow::meshToWindowY(double y)
{
  return border + height() - int( (y - mesh->minY()) * height() / mesh->height());
}

double MeshWindow::windowToMeshX(int x)
{
  return mesh->minX() + mesh->width() * (x - border) / width();
}

double MeshWindow::windowToMeshY(int y)
{
  return mesh->minY() + mesh->height() * (height() - (y - border)) / height();
}

}
