
/*
 *  Interval.cc: Specification of a range [x..y]
 *
 *  Written by:
 *   Steven G. Parker
 *   Department of Computer Science
 *   University of Utah
 *   May 1996
 *
 *  Copyright (C) 1996 SCI Group
 */

#include <Core/Datatypes/Interval.h>
#include <Core/Containers/String.h>
#include <Core/Malloc/Allocator.h>

namespace SCIRun {

static Persistent* maker()
{
    return scinew Interval(0, 1);
}

PersistentTypeID Interval::type_id("Interval", "Datatype", maker);

Interval::Interval(double low, double high)
: low(low), high(high)
{
}

Interval::Interval(const Interval& c)
: low(c.low), high(c.high)
{
}

Interval::~Interval()
{
}

Interval* Interval::clone() const
{
    return scinew Interval(*this);
}

#define INTERVAL_VERSION 1

void Interval::io(Piostream& stream)
{
    stream.begin_class("Interval", INTERVAL_VERSION);
    stream.io(low);
    stream.io(high);
    stream.end_class();
}

} // End namespace SCIRun

