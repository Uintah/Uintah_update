/*

The MIT License

Copyright (c) 1997-2009 Center for the Simulation of Accidental Fires and 
Explosions (CSAFE), and  Scientific Computing and Imaging Institute (SCI), 
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


#include <Uintah/Core/Grid/Variables/GridVariableBase.h>
#include <Uintah/Core/Disclosure/TypeDescription.h>
#include <Uintah/Core/Parallel/BufferInfo.h>

#include <Core/Geometry/IntVector.h>
#include <Core/Exceptions/InternalError.h>
#include <Core/Thread/Mutex.h>

using namespace Uintah;
using namespace SCIRun;

#undef UINTAHSHARE
#if defined(_WIN32) && !defined(BUILD_UINTAH_STATIC)
#  define UINTAHSHARE __declspec(dllimport)
#else
#  define UINTAHSHARE
#endif

extern UINTAHSHARE Mutex MPITypeLock;

void GridVariableBase::getMPIBuffer(BufferInfo& buffer,
	  			    const IntVector& low, const IntVector& high)
{
  const TypeDescription* td = virtualGetTypeDescription()->getSubType();
  MPI_Datatype basetype=td->getMPIType();
  IntVector l, h, s, strides, dataLow;
  getSizes(l, h, dataLow, s, strides);

  IntVector off = low - dataLow;
  char* startbuf = (char*)getBasePointer();
  startbuf += strides.x()*off.x()+strides.y()*off.y()+strides.z()*off.z();
  IntVector d = high-low;
  MPI_Datatype type1d;
 MPITypeLock.lock();
  MPI_Type_hvector(d.x(), 1, strides.x(), basetype, &type1d);

  MPI_Datatype type2d;
  MPI_Type_hvector(d.y(), 1, strides.y(), type1d, &type2d);
  MPI_Type_free(&type1d);
  MPI_Datatype type3d;
  MPI_Type_hvector(d.z(), 1, strides.z(), type2d, &type3d);
  MPI_Type_free(&type2d);
  MPI_Type_commit(&type3d);
 MPITypeLock.unlock();  
  buffer.add(startbuf, 1, type3d, true);
}
