#include <testprograms/Component/framework/cca_sidl.h>
#include <testprograms/Component/framework/Registry.h>
#include <testprograms/Component/framework/ConnectionServicesImpl.h>
#include <testprograms/Component/framework/FrameworkImpl.h>

#include <iostream>

using std::cerr;

namespace sci_cca {

typedef Registry::component_iterator component_iterator;

ConnectionServicesImpl::ConnectionServicesImpl() 
{
}


ConnectionServicesImpl::~ConnectionServicesImpl()
{
}


void 
ConnectionServicesImpl::init( const Framework &f ) 
{ 
  framework_ = f; 
  
  registry_ = dynamic_cast<FrameworkImpl *>(f.getPointer())->registry_;
}

bool
ConnectionServicesImpl::connect( const ComponentID &uses, 
				 const string &use_port, 
				 const ComponentID &provider, 
				 const string &provide_port)
{
  // lock registry
  registry_->connections_.writeLock();

  // get provide port record
  ProvidePortRecord *provide = registry_->getProvideRecord( provider, 
							    provide_port );
  if ( !provide ) {
    // error: could not find provider's port
    return false;
  }

  if ( provide->connection_ ) {
    // error: provide port in use
    return false;
  }

  // get use port record
  UsePortRecord *use = registry_->getUseRecord( uses, use_port );

  if ( !use ) {
    // error: could not find use's port
    return false;
  }

  if ( use->connection_ ) {
    // error: uses port in use
    return false;
  }

  // connect
  ConnectionRecord *record = new ConnectionRecord;
  record->use_ = use;
  record->provide_ = provide;

  provide->connection_ = record;
  use->connection_ = record;

  // unlock registry
  registry_->connections_.writeUnlock();

  // notify who ever wanted to 

  // done
  return true;
}
  

bool 
ConnectionServicesImpl::disconnect( const ComponentID &, const string &, 
				    const ComponentID &, const string &)
{
  return false;
}

bool 
ConnectionServicesImpl::exportAs( const ComponentID &, const string &, 
				  const string &)
{
  return false;
}

bool
ConnectionServicesImpl::provideTo( const ComponentID &, const string&, 
				   const string &)
{
  return false;
}

} // namespace sci_cca
