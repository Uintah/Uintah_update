#ifndef cVECTOR_H
#define cVECTOR_H 1

#include <iostream.h>
#include <fstream.h>
#include <math.h>
#include <Math/Complex.h>


#include <Datatypes/Datatype.h>
#include <Classlib/LockingHandle.h>

class cVector;
typedef LockingHandle<cVector> cVectorHandle;



class cVector :public Datatype{

  friend class cDMatrix;
  friend class cSMatrix;
  
private:
  Complex *a;
  int Size;
  
public:

// Persistent representation...
    virtual void io(Piostream&);
    static PersistentTypeID type_id;

  
  cVector (int N); //constructor
  cVector (const cVector &B); //copy constructor;
  cVector &operator=(const cVector &B); //assigment operator  
  ~cVector(); //Destructor;
  
  int size() {return Size;};
  double norm();
  Complex &operator()(int i);
  void conj();
  int load(char* filename);
  
  
  void set(const cVector& B);
  void add(const cVector& B);
  void subtr(const cVector& B);
  void mult(const Complex x); 
  
  cVector operator+(const cVector& B) const;
  cVector operator-(const cVector& B) const;
  
  friend Complex operator*(cVector& A, cVector& B);
  friend cVector  operator*(const cVector& B,Complex x);
  friend cVector  operator*(Complex x, const cVector &B);
  friend cVector  operator*(const cVector& B,double x);
  friend cVector  operator*(double x, const cVector &B);
  
  friend ostream &operator<< (ostream &output, cVector &B);
  
};

#endif






