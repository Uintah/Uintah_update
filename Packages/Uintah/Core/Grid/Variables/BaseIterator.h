
#ifndef UINTAH_BaseIterator_H
#define UINTAH_BaseIterator_H

#include <ostream>
using namespace std;

#include <Core/Geometry/IntVector.h>

#include <Packages/Uintah/Core/Grid/uintahshare.h>
namespace Uintah {

  using SCIRun::IntVector;

  /**************************************

    CLASS
    BaseIterator

    Base class for all iterators

    GENERAL INFORMATION

    BaseIterator.h

    Justin Luitjens
    Department of Computer Science
    University of Utah

    Center for the Simulation of Accidental Fires and Explosions (C-SAFE)

    Copyright (C) 2000 SCI Group

    KEYWORDS
    BaseIterator

    DESCRIPTION
    Base class for all iterators.  In Uintah an iterator is both a container
    and an iterator.  In order to be considered valid the iterator must visit
    the container in a sorted order from low to high (according to Uintah's 
    defined order, which is Z,Y,X).  In addition, each cell 
    can only be in the iterator once.  These conditions are not strictly 
    enforced and thus it is up to the creater of the iterator to enforce 
    these conditions.

    WARNING

   ****************************************/

  class UINTAHSHARE BaseIterator : 
    public std::iterator<std::forward_iterator_tag, IntVector> 
  {
    friend class Iterator;
    public:

    virtual ~BaseIterator() {}

    /**
     * prefix operator to move the iterator forward
     */
    virtual BaseIterator& operator++()=0; 

    /**
     * postfix operator to move the iterator forward
     * does not return the iterator because of performance issues
     */
    virtual void operator++(int) = 0;

    /**
     * returns true if the iterator is done
     */    
    virtual bool done() const = 0;

    /**
     * returns the IntVector that the current iterator is pointing at
     */
    virtual IntVector operator*() const=0;


    /**
     * Return the first element of the iterator
     */
    virtual IntVector begin() const=0;

    /**
     * Return one past the last element of the iterator
     */
    virtual IntVector end() const=0;

    virtual ostream& limits(ostream&) const = 0;


    protected:
    /**
     * Prevent this class from being instantiated, use an inherited class instead
     */
    BaseIterator() {};

    /**
     * Returns a pointer to a deep copy of the virtual class
     * this should be used only by the Iterator class
     */
    virtual BaseIterator* clone() const = 0;
    

    private:
    

    /**
     * send iterator information to the ostream 
     */
    virtual ostream& put(ostream&) const = 0;

    /**
     * resets the iterator to the begining
     */
    virtual void reset() = 0;

  }; // end class BaseIterator


} // End namespace Uintah
  
#endif
