
/*
 *  Parallel1: Automatically instantiate several threads, with 1 argument
 *
 *  Written by:
 *   Author: Steve Parker
 *   Department of Computer Science
 *   University of Utah
 *   Date: June 2000
 *
 *  Copyright (C) 2000 SCI Group
 */

#ifndef Core_Thread_Parallel1_h
#define Core_Thread_Parallel1_h

#include <Core/Thread/ParallelBase.h>
#include <Core/Thread/Semaphore.h>

namespace SCIRun {
/**************************************

 CLASS
 Parallel1

 KEYWORDS
 Thread

 DESCRIPTION
 Helper class to make instantiating threads to perform a parallel
 task easier.
   
****************************************/
template<class T, class Arg1> class Parallel1  : public ParallelBase {
public:
  //////////
  // Create a Parallel1 object, using the specified member
  // function instead of <i>Parallel1</i>.  This will
  // typically be used like:
  // <b><pre>Thread::Parallel1(Parallel1&lt;MyClass&gt;(this, &amp;MyClass::mymemberfn), nthreads);</pre></b>
  Parallel1(T* obj, void (T::*pmf)(int, Arg1), Arg1 a1);
	    
  //////////
  // Destroy the Parallel1 object - the threads will remain alive.
  virtual ~Parallel1();
  T* d_obj;
  void (T::*d_pmf)(int, Arg1);
  Arg1 a1;
protected:
  virtual void run(int proc);
private:
  // Cannot copy them
  Parallel1(const Parallel1&);
  Parallel1<T, Arg1>& operator=(const Parallel1<T, Arg1>&);
};

template<class T, class Arg1>
void
Parallel1<T, Arg1>::run(int proc)
{
    // Copy out do make sure that the call is atomic
    T* obj=d_obj;
    void (T::*pmf)(int, Arg1) = d_pmf;
    if(d_wait)
	d_wait->up();
    (obj->*pmf)(proc, a1);
    // Cannot do anything here, since the object may be deleted by the
    // time we return
}

template<class T, class Arg1>
Parallel1<T, Arg1>::Parallel1(T* obj,
			      void (T::*pmf)(int, Arg1),
			      Arg1 a1)
  : d_obj(obj), d_pmf(pmf), a1(a1)
{
  d_wait=0; // This may be set by Thread::parallel
} // End namespace SCIRun

template<class T, class Arg1>
Parallel1<T, Arg1>::~Parallel1()
{
}

} // End namespace SCIRun

#endif


