#ifndef UINTAH_HOMEBREW_PARTICLEVARIABLE_H
#define UINTAH_HOMEBREW_PARTICLEVARIABLE_H

#include <Core/Util/FancyAssert.h>
#include <Core/Exceptions/ErrnoException.h>
#include <Core/Exceptions/InternalError.h>
#include <Core/Util/Assert.h>
#include <Core/Malloc/Allocator.h>
#include <Dataflow/XMLUtil/XMLUtil.h>
#include <Packages/Uintah/Core/Grid/ParticleVariableBase.h>
#include <Packages/Uintah/CCA/Ports/InputContext.h>
#include <Packages/Uintah/CCA/Ports/OutputContext.h>
#include <Packages/Uintah/Core/Exceptions/TypeMismatchException.h>
#include <Packages/Uintah/Core/Grid/ParticleData.h>
#include <Packages/Uintah/Core/Grid/ParticleSubset.h>
#include <Packages/Uintah/Core/Disclosure/TypeDescription.h>
#include <Packages/Uintah/Core/Disclosure/TypeUtils.h>
#include <Packages/Uintah/Core/Parallel/ProcessorGroup.h>

#include <unistd.h>
#include <errno.h>

namespace Uintah {

using namespace SCIRun;

class TypeDescription;

/**************************************

CLASS
ParticleVariable
  
Short description...

GENERAL INFORMATION

ParticleVariable.h

Steven G. Parker
Department of Computer Science
University of Utah

Center for the Simulation of Accidental Fires and Explosions (C-SAFE)

Copyright (C) 2000 SCI Group

KEYWORDS
Particle_Variable

DESCRIPTION
Long description...
  
WARNING
  
****************************************/

template<class T>
class ParticleVariable : public ParticleVariableBase {
public:
  ParticleVariable();
  virtual ~ParticleVariable();
  ParticleVariable(ParticleSubset* pset);
  ParticleVariable(ParticleData<T>*, ParticleSubset* pset);
  ParticleVariable(const ParticleVariable<T>&);
      
    //////////
    // Insert Documentation Here:
  static const TypeDescription* getTypeDescription();

  //////////
  // Insert Documentation Here:
  void resync() {
    d_pdata->resize(getParticleSet()->numParticles());
  }
      
  //////////
  // Insert Documentation Here:
  virtual ParticleVariableBase* clone() const;
  virtual ParticleVariableBase* cloneSubset(ParticleSubset*) const;
      
  //////////
  // Insert Documentation Here:
  inline T& operator[](particleIndex idx) {
    ASSERTRANGE(idx, 0, (particleIndex)d_pdata->data.size());
    return d_pdata->data[idx];
  }
      
  //////////
  // Insert Documentation Here:
  inline const T& operator[](particleIndex idx) const {
    ASSERTRANGE(idx, 0, (particleIndex)d_pdata->data.size());
    return d_pdata->data[idx];
  }
      
  virtual void copyPointer(const ParticleVariable<T>&);
  virtual void copyPointer(const ParticleVariableBase&);
  virtual void allocate(ParticleSubset*);
  virtual void allocate(const Patch*)
  { throw InternalError("Should not call ParticleVariable<T>::allocate(const Patch*), use allocate(ParticleSubset*) instead."); }
  
  virtual void gather(ParticleSubset* dest,
		      std::vector<ParticleSubset*> subsets,
		      std::vector<ParticleVariableBase*> srcs,
		      particleIndex extra = 0);
  virtual void unpackMPI(void* buf, int bufsize, int* bufpos,
			 const ProcessorGroup* pg, ParticleSubset* pset);
  virtual void packMPI(void* buf, int bufsize, int* bufpos,
		       const ProcessorGroup* pg, ParticleSubset* pset);
  virtual void packsizeMPI(int* bufpos,
			   const ProcessorGroup* pg,
			   ParticleSubset* pset);
  virtual void emitNormal(ostream& out, DOM_Element varnode);
  virtual void emitRLE(ostream& out, DOM_Element varnode);
  
  virtual void readNormal(istream& in);
  virtual void readRLE(istream& in);
  
  virtual void* getBasePointer() const;
  virtual const TypeDescription* virtualGetTypeDescription() const;
  virtual RefCounted* getRefCounted() {
    return d_pdata;
  }
  virtual void getSizeInfo(string& elems, unsigned long& totsize,
			   void*& ptr) const {
    ostringstream str;
    str << getParticleSet()->numParticles();
    elems=str.str();
    totsize = getParticleSet()->numParticles()*sizeof(T);
    ptr = getBasePointer();
  }
protected:
  ParticleVariable<T>& operator=(const ParticleVariable<T>&);

private:
    //////////
    // Insert Documentation Here:
  ParticleData<T>* d_pdata;
      
  static TypeDescription::Register registerMe;
  static Variable* maker();
};

  template<class T>
  TypeDescription::Register ParticleVariable<T>::registerMe(getTypeDescription());

  template<class T>
  const TypeDescription*
  ParticleVariable<T>::getTypeDescription()
  {
    static TypeDescription* td;
    if(!td){
      td = scinew TypeDescription(TypeDescription::ParticleVariable,
				  "ParticleVariable", &maker,
				  fun_getTypeDescription((T*)0));
    }
    return td;
  }
   
  template<class T>
  Variable*
  ParticleVariable<T>::maker()
  {
    return scinew ParticleVariable<T>();
  }
   
  template<class T>
  ParticleVariable<T>::ParticleVariable()
    : ParticleVariableBase(0), d_pdata(0)
  {
  }
   
  template<class T>
  ParticleVariable<T>::~ParticleVariable()
  {
    if(d_pdata && d_pdata->removeReference())
      delete d_pdata;
  }
   
  template<class T>
  ParticleVariable<T>::ParticleVariable(ParticleSubset* pset)
    : ParticleVariableBase(pset)
  {
    d_pdata=scinew ParticleData<T>(pset->getParticleSet()->numParticles());
    d_pdata->addReference();
  }
   
  template<class T>
  void ParticleVariable<T>::allocate(ParticleSubset* pset)
  {
    if(d_pdata && d_pdata->removeReference())
      delete d_pdata;
    if(d_pset && d_pset->removeReference())
      delete d_pset;

    d_pset=pset;
    d_pset->addReference();
    d_pdata=scinew ParticleData<T>(pset->getParticleSet()->numParticles());
    d_pdata->addReference();
  }
   
  template<class T>
  ParticleVariableBase*
  ParticleVariable<T>::clone() const
  {
    return scinew ParticleVariable<T>(*this);
  }
   
  template<class T>
  ParticleVariableBase*
  ParticleVariable<T>::cloneSubset(ParticleSubset* pset) const
  {
    return scinew ParticleVariable<T>(d_pdata, pset);
  }

  template<class T>
  ParticleVariable<T>::ParticleVariable(ParticleData<T>* pdata,
					ParticleSubset* pset)
    : ParticleVariableBase(pset), d_pdata(pdata)
  {
    if(d_pdata)
      d_pdata->addReference();
  }
   
  template<class T>
  ParticleVariable<T>::ParticleVariable(const ParticleVariable<T>& copy)
    : ParticleVariableBase(copy), d_pdata(copy.d_pdata)
  {
    if(d_pdata)
      d_pdata->addReference();
  }
   
  template<class T>
  void
  ParticleVariable<T>::copyPointer(const ParticleVariable<T>& copy)
  {
    if(this != &copy){
      ParticleVariableBase::operator=(copy);
      if(d_pdata && d_pdata->removeReference())
	delete d_pdata;
      d_pdata = copy.d_pdata;
      if(d_pdata)
	d_pdata->addReference();
    }
  }
   
  template<class T>
  void
  ParticleVariable<T>::copyPointer(const ParticleVariableBase& copy)
  {
    const ParticleVariable<T>* c = dynamic_cast<const ParticleVariable<T>* >(&copy);
    if(!c)
      throw TypeMismatchException("Type mismatch in particle variable");
    copyPointer(*c);
  }
  
  template<class T>
  void
  ParticleVariable<T>::gather(ParticleSubset* pset,
			      std::vector<ParticleSubset*> subsets,
			      std::vector<ParticleVariableBase*> srcs,
			      particleIndex extra)
  {
    if(d_pdata && d_pdata->removeReference())
      delete d_pdata;
    if(d_pset && d_pset->removeReference())
      delete d_pset;
    d_pset = pset;
    pset->addReference();
    d_pdata=scinew ParticleData<T>(pset->getParticleSet()->numParticles());
    d_pdata->addReference();
    ASSERTEQ(subsets.size(), srcs.size());
    ParticleSubset::iterator dstiter = pset->begin();
    for(int i=0;i<(int)subsets.size();i++){
      ParticleVariable<T>* srcptr = dynamic_cast<ParticleVariable<T>*>(srcs[i]);
      if(!srcptr)
	throw TypeMismatchException("Type mismatch in ParticleVariable::gather");
      ParticleVariable<T>& src = *srcptr;
      ParticleSubset* subset = subsets[i];
      for(ParticleSubset::iterator srciter = subset->begin();
	  srciter != subset->end(); srciter++){
	(*this)[*dstiter] = src[*srciter];
	dstiter++;
      }
    }
    ASSERT(dstiter+extra == pset->end());
  }
  
  template<class T>
  void*
  ParticleVariable<T>::getBasePointer() const
  {
    return &d_pdata->data[0];
  }
  
  template<class T>
  const TypeDescription*
  ParticleVariable<T>::virtualGetTypeDescription() const
  {
    return getTypeDescription();
  }
  
  template<class T>
  void
  ParticleVariable<T>::unpackMPI(void* buf, int bufsize, int* bufpos,
				 const ProcessorGroup* pg,
				 ParticleSubset* pset)
  {
    // This should be fixed for variable sized types!
    const TypeDescription* td = getTypeDescription()->getSubType();
    if(td->isFlat()){
      for(ParticleSubset::iterator iter = pset->begin();
	  iter != pset->end(); iter++){
	MPI_Unpack(buf, bufsize, bufpos,
		   &d_pdata->data[*iter], 1, td->getMPIType(),
		   pg->getComm());
      }
    } else {
      throw InternalError("packMPI not finished\n");
    }
  }
  
  template<class T>
  void
  ParticleVariable<T>::packMPI(void* buf, int bufsize, int* bufpos,
			       const ProcessorGroup* pg,
			       ParticleSubset* pset)
  {
    // This should be fixed for variable sized types!
    const TypeDescription* td = getTypeDescription()->getSubType();
    if(td->isFlat()){
      for(ParticleSubset::iterator iter = pset->begin();
	  iter != pset->end(); iter++){
	MPI_Pack(&d_pdata->data[*iter], 1, td->getMPIType(),
		 buf, bufsize, bufpos, pg->getComm());
      }
    } else {
      throw InternalError("packMPI not finished\n");
    }
  }

  template<class T>
  void
  ParticleVariable<T>::packsizeMPI(int* bufpos,
				   const ProcessorGroup* pg,
				   ParticleSubset* pset)
  {
    // This should be fixed for variable sized types!
    const TypeDescription* td = getTypeDescription()->getSubType();
    int n = pset->numParticles();
    if(td->isFlat()){
      int size;
      MPI_Pack_size(n, td->getMPIType(), pg->getComm(), &size);
      (*bufpos)+= size;
    } else {
      throw InternalError("packsizeMPI not finished\n");
    }
  }

  template<class T>
  void
  ParticleVariable<T>::emitNormal(ostream& out, DOM_Element varnode)
  {
    const TypeDescription* td = fun_getTypeDescription((T*)0);

    if (findNode("numParticles", varnode) == 0)
      appendElement(varnode, "numParticles", d_pset->numParticles());
      
    if(!td->isFlat()){
      throw InternalError("Cannot yet write non-flat objects!\n");
    }
    else {
      // This could be optimized...
      ParticleSubset::iterator iter = d_pset->begin();
      while(iter != d_pset->end()){
	particleIndex start = *iter;
	iter++;
	particleIndex end = start+1;
	while(iter != d_pset->end() && *iter == end) {
	  end++;
	  iter++;
	}
	ssize_t size = (ssize_t)(sizeof(T)*(end-start));
	out.write((char*)&(*this)[start], size);
      }
    }
  }

  template<class T>
  void
  ParticleVariable<T>::emitRLE(ostream& out, DOM_Element varnode)
  {
    const TypeDescription* td = fun_getTypeDescription((T*)0);
    if (findNode("numParticles", varnode) == 0)
      appendElement(varnode, "numParticles", d_pset->numParticles());
    if(!td->isFlat()){
      throw InternalError("Cannot yet write non-flat objects!\n");
    }
    else {
      // emit in runlength encoded format
      RunLengthEncoder<T> rle;
      ParticleSubset::iterator iter = d_pset->begin();
      for ( ; iter != d_pset->end(); iter++)
	rle.addItem((*this)[*iter]);
      rle.write(out);
    }
  }
  
  template<class T>
  void
  ParticleVariable<T>::readNormal(istream& in)
  {
    const TypeDescription* td = fun_getTypeDescription((T*)0);
    if(!td->isFlat()) {
      throw InternalError("Cannot yet read non-flat objects!\n");
    }
    else {
      // This could be optimized...
      ParticleSubset::iterator iter = d_pset->begin();
      while(iter != d_pset->end()){
	particleIndex start = *iter;
	iter++;
	particleIndex end = start+1;
	while(iter != d_pset->end() && *iter == end) {
	  end++;
	  iter++;
	}
	ssize_t size = (ssize_t)(sizeof(T)*(end-start));
	in.read((char*)&(*this)[start], size);
      }
    }
  }

  template<class T>
  void
  ParticleVariable<T>::readRLE(istream& in)
  {
    const TypeDescription* td = fun_getTypeDescription((T*)0);
    if(!td->isFlat()) {
      throw InternalError("Cannot yet read non-flat objects!\n");
    }
    else {
      RunLengthEncoder<T> rle;
      rle.read(in);
      ParticleSubset::iterator iter = d_pset->begin();
      RunLengthEncoder<T>::iterator rle_iter = rle.begin();
      for ( ; iter != d_pset->end() && rle_iter != rle.end();
	    iter++, rle_iter++)
	(*this)[*iter] = *rle_iter;

      if ((rle_iter != rle.end()) || (iter != d_pset->end()))
	throw InternalError("ParticleVariable::read RLE data is not consistent with the particle subset size");
    }
  }
} // End namespace Uintah

#endif
