
/*
 *  ProxyBase.h: Base class for all PIDL proxies
 *
 *  Written by:
 *   Steven G. Parker
 *   Department of Computer Science
 *   University of Utah
 *   July 1999
 *
 *  Copyright (C) 1999 SCI Group
 */

#include <Core/CCA/Component/PIDL/ProxyBase.h>
#include <Core/CCA/Component/PIDL/GlobusError.h>
#include <Core/CCA/Component/PIDL/TypeInfo.h>
#include <globus_nexus.h>

using Component::PIDL::ProxyBase;
using Component::PIDL::Reference;

ProxyBase::ProxyBase(const Reference& ref)
    : d_ref(ref)
{
}

ProxyBase::~ProxyBase()
{
    int size=0;
    globus_nexus_buffer_t buffer;
    if(int gerr=globus_nexus_buffer_init(&buffer, size, 0))
	throw GlobusError("buffer_init", gerr);

    // Send the message
    Reference ref;
    _proxyGetReference(ref, false);
    int handler=TypeInfo::vtable_deleteReference_handler;
    if(int gerr=globus_nexus_send_rsr(&buffer, &ref.d_sp,
				      handler, GLOBUS_TRUE, GLOBUS_FALSE))
	throw GlobusError("send_rsr", gerr);
    // No reply is sent for this

    if(d_ref.d_vtable_base != TypeInfo::vtable_invalid){
	if(int gerr=globus_nexus_startpoint_destroy_and_notify(&d_ref.d_sp)){
	    throw GlobusError("nexus_startpoint_destroy_and_notify", gerr);
	}
    }
}

void ProxyBase::_proxyGetReference(Reference& ref, bool copy) const
{
    ref=d_ref;
    if(copy){
	if(int gerr=globus_nexus_startpoint_copy(&ref.d_sp, const_cast<globus_nexus_startpoint_t*>(&d_ref.d_sp)))
	    throw GlobusError("startpoint_copy", gerr);
    }
}

