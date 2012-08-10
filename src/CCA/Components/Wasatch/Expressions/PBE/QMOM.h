/*
 * Copyright (c) 2012 The University of Utah
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef QMOM_Expr_h
#define QMOM_Expr_h
#include <spatialops/structured/FVStaggeredFieldTypes.h>
#include <spatialops/structured/FVStaggeredOperatorTypes.h>

#include <expression/Expression.h>
#include <cmath>

//#define WASATCH_QMOM_DIAGNOSTICS

// declare lapack eigenvalue solver
extern "C"{
  void dsyev_( char* jobz, char* uplo, int* n, double* a, int* lda,
        double* w, double* work, int* lwork, int* info );
}


/**
 *  \class QMOM
 *  \author Tony Saad
 *  \todo add documentation
 *  \todo add till support
 */
template<typename FieldT>
class QMOM : public Expr::Expression<FieldT>
{
  typedef std::vector<const FieldT*> FieldTVec;
  FieldTVec knownMoments_;

  const FieldT* superSaturation_;
  const Expr::TagList knownMomentsTagList_;
  const Expr::Tag superSaturationTag_;

  const int nMoments_;

  double **pmatrix_;
  std::vector<double> a_, b_, alpha_, jMatrix_, eigenValues_, weights_, work_;

  QMOM( const Expr::TagList knownMomentsTagList,
       const Expr::Tag superSaturationTag );

public:
  class Builder : public Expr::ExpressionBuilder
  {
  public:
    Builder( const Expr::TagList& result,
             const Expr::TagList& knownMomentsTagList,
             const Expr::Tag& superSaturationTag)
    : ExpressionBuilder(result),
      knownMomentsTagList_( knownMomentsTagList ),
      supersaturationt_ ( superSaturationTag  )
    {}
    ~Builder(){}
    Expr::ExpressionBase* build() const
    {
      return new QMOM<FieldT>( knownMomentsTagList_, supersaturationt_ );
    }

  private:
    const Expr::TagList knownMomentsTagList_;
    const Expr::Tag     supersaturationt_;
  };

  ~QMOM();

  void advertise_dependents( Expr::ExprDeps& exprDeps );
  void bind_fields( const Expr::FieldManagerList& fml );
  void bind_operators( const SpatialOps::OperatorDatabase& opDB );
  void evaluate();

};


// ###################################################################
//
//                          Implementation
//
// ###################################################################


template<typename FieldT>
QMOM<FieldT>::
QMOM( const Expr::TagList knownMomentsTaglist,
      const Expr::Tag     superSaturationTag )
  : Expr::Expression<FieldT>(),
    knownMomentsTagList_( knownMomentsTaglist ),
    superSaturationTag_ ( superSaturationTag  ),
    nMoments_( knownMomentsTaglist.size() )
{
  pmatrix_ = new double *[nMoments_];
  for (int i = 0; i<nMoments_; i++)
    pmatrix_[i] = new double [nMoments_+1];

  const int abSize = nMoments_/2;
  alpha_.resize(nMoments_);
  a_.resize( abSize );
  b_.resize( abSize );
  jMatrix_.resize( abSize*abSize );
  eigenValues_.resize( abSize );
  weights_.resize(abSize);
}

//--------------------------------------------------------------------
template<typename FieldT>
QMOM<FieldT>::
~QMOM()
{
  for (int i = 0; i<nMoments_; i++) delete[] pmatrix_[i];
  delete[] pmatrix_;
}

//--------------------------------------------------------------------
template< typename FieldT >
void
QMOM<FieldT>::
advertise_dependents( Expr::ExprDeps& exprDeps )
{
  exprDeps.requires_expression( knownMomentsTagList_ );
  if ( superSaturationTag_ != Expr::Tag () ) {
    exprDeps.requires_expression( superSaturationTag_ );
  }
}
//--------------------------------------------------------------------

template< typename FieldT >
void
QMOM<FieldT>::
bind_fields( const Expr::FieldManagerList& fml )
{
  const typename Expr::FieldMgrSelector<FieldT>::type& fm = fml.field_manager<FieldT>();
  knownMoments_.clear();
  for( Expr::TagList::const_iterator iMomTag=knownMomentsTagList_.begin();
       iMomTag!=knownMomentsTagList_.end();
       ++iMomTag ){
    knownMoments_.push_back( &fm.field_ref(*iMomTag) );
  }
  if ( superSaturationTag_ != Expr::Tag () ) {
    superSaturation_ = &fm.field_ref( superSaturationTag_ );
  }
}

//--------------------------------------------------------------------

template< typename FieldT >
void
QMOM<FieldT>::
bind_operators( const SpatialOps::OperatorDatabase& opDB )
{}

//--------------------------------------------------------------------

template< typename FieldT >
void
QMOM<FieldT>::
evaluate()
{
  using namespace SpatialOps;
  using namespace SCIRun;
  // the results vector is organized as follows:
  // (w0, a0, w1, a1, et...)
  typedef std::vector<FieldT*> ResultsVec;
  ResultsVec& results = this->get_value_vec();
  //

  // initialize the p matrix
  for (int i=0; i<nMoments_; ++i) {
    for (int j=0; j<nMoments_+1; ++j) {
      pmatrix_[i][j]=0.0;
    }
  }
  pmatrix_[0][0] = 1.0;

  //
  const int abSize = nMoments_/2;
  alpha_.resize(nMoments_);
  a_.resize( abSize );
  b_.resize( abSize );
  jMatrix_.resize( abSize*abSize );
  eigenValues_.resize( abSize );
  weights_.resize(abSize);
  
  // loop over interior points in the patch. To do this, we grab a sample
  // iterator from any of the exisiting fields, for example, m0.
  const FieldT* sampleField = knownMoments_[0];
  typename FieldT::const_interior_iterator sampleIterator = sampleField->interior_begin();

  // grab an iterator for the supersaturation ratio field, set to m_0 if no
  // no supersaturation tag is provided.
  if ( superSaturationTag_ == Expr::Tag() ) {
    superSaturation_ = knownMoments_[0];
  }
  typename FieldT::const_interior_iterator supersatIter = superSaturation_->interior_begin();

  double m0;
  //
  // create a vector of iterators for the known moments and for the results
  //
  std::vector<typename FieldT::const_interior_iterator> knownMomentsIterators;
  std::vector<typename FieldT::interior_iterator> resultsIterators;
  for (int i=0; i<nMoments_; ++i) {
    typename FieldT::const_interior_iterator thisIterator = knownMoments_[i]->interior_begin();
    knownMomentsIterators.push_back(thisIterator);

    typename FieldT::interior_iterator thisResultsIterator = results[i]->interior_begin();
    resultsIterators.push_back(thisResultsIterator);
  }

  const double epsilon = 1.0e-10;
  // now loop over the interior points, construct the matrix, and solve for the weights & abscissae
  while (sampleIterator!=sampleField->interior_end()) {

    // check if we are in a region where supersaturation is zero
    if ( (superSaturationTag_ != Expr::Tag() && *supersatIter < 1e-10) || (superSaturationTag_ != Expr::Tag() && *knownMomentsIterators[0] == 0) ) {
      // in case the supersaturation or m_0 is zero, set the weights to zero and abscissae to 1
      // helps with numerical stabilization of problem
      for (int i=0; i<abSize; ++i) {
        int matLoc = 2*i;
        *resultsIterators[matLoc] = 0.0;     // weight
        *resultsIterators[matLoc + 1] = 1.0; // abscissa
      }
      //increment iterators
      ++sampleIterator;
      for (int i=0; i<nMoments_; ++i) {
        knownMomentsIterators[i] += 1;
        resultsIterators[i] += 1;
      }
      ++supersatIter;
      continue;
    }

    // for every point, calculate the quadrature weights and abscissae
    // start by putting together the p matrix. this is documented in the wasatch
    // pdf documentation.
    for (int iRow=0; iRow<=nMoments_-2; iRow += 2) {
      // get the the iRow moment for this point
      pmatrix_[iRow][1] = *knownMomentsIterators[iRow];
      // get the the (iRow+1) moment for all points
      pmatrix_[iRow+1][1] = -*knownMomentsIterators[iRow+1];
    }
    // keep filling the p matrix. this gets easier now
    for (int jCol=2; jCol<=nMoments_; ++jCol) {
      for (int iRow=0; iRow <= nMoments_ - jCol; ++iRow) {
        pmatrix_[iRow][jCol] = pmatrix_[0][jCol-1]*pmatrix_[iRow+1][jCol-2]
                             - pmatrix_[0][jCol-2]*pmatrix_[iRow+1][jCol-1];
      }
    }

#ifdef WASATCH_QMOM_DIAGNOSTICS
    for (int iRow=0; iRow<nMoments_; iRow++) {
      for (int jCol=0; jCol<nMoments_+1; jCol++) {
        printf("p[%i][%i] = %f \t \t",iRow,jCol,pmatrix_[iRow][jCol]);
      }
      printf("\n");
    }
#endif

    //
    //
    alpha_[0]=0.0;
    for (int jCol=1; jCol<nMoments_; ++jCol)
      alpha_[jCol] = pmatrix_[0][jCol+1]/(pmatrix_[0][jCol]*pmatrix_[0][jCol-1]);

#ifdef WASATCH_QMOM_DIAGNOSTICS
    for (int iRow=0; iRow<nMoments_; iRow++) {
        printf("alpha[%i] = %f \t \t",iRow,alpha_[iRow]);
    }
    printf("\n");
#endif

    //_________________________
    // construct a and b arrays
    for (int jCol=0; jCol < abSize-1; ++jCol) {
      const int twojcol = 2*jCol;
      a_[jCol] = alpha_[twojcol+1] + alpha_[twojcol];
      const double rhsB = alpha_[twojcol+1]*alpha_[twojcol+2];
      //
      if (rhsB < 0) {
        std::ostringstream errorMsg;
        errorMsg << endl
                 << "ERROR: Negative number detected in constructing the b auxiliary matrix while processing the QMOM expression." << std::endl
                 << "Value: b["<<jCol<<"] = "<<rhsB << std::endl;
        std::cout << superSaturationTag_ << std::endl;
        for (int i = 0; i<nMoments_; i++) {
          std::cout << "Value: M["<<i<<"] = "<<*knownMomentsIterators[i] << std::endl;
        }
        throw std::runtime_error( errorMsg.str() );
      }
      b_[jCol] = -std::sqrt(rhsB);
    }
    // fill in the last entry for a
    a_[abSize-1] = alpha_[2*(abSize-1) + 1] + alpha_[2*(abSize-1)];

#ifdef WASATCH_QMOM_DIAGNOSTICS
    for (int iRow=0; iRow<abSize; iRow++) {
      printf("a[%i] = %f \t \t",iRow,a_[iRow]);
    }
    printf("\n");
    for (int iRow=0; iRow<abSize; iRow++) {
      printf("b[%i] = %f \t \t",iRow,b_[iRow]);
    }
    printf("\n");
#endif

    //need to initialize JMAtrix to zero at each point
    for (int i = 0; i<abSize*abSize; i++) {
      jMatrix_[i] = 0.0;
    }
    
    //___________________
    // construct J matrix
    for (int iRow=0; iRow<abSize - 1; ++iRow) {
      jMatrix_[iRow+iRow*abSize] = a_[iRow];
      jMatrix_[iRow+(iRow+1)*abSize] = b_[iRow];
      jMatrix_[iRow + 1 + iRow*abSize] = b_[iRow];
    }
    jMatrix_[abSize*abSize-1] = a_[abSize - 1];

#ifdef WASATCH_QMOM_DIAGNOSTICS
    for (int iRow=0; iRow<abSize*abSize; iRow++) {
      printf("J[%i] = %f \t \t",iRow,jMatrix_[iRow]);
    }
    printf("\n");
#endif

    //__________
    // Eigenvalue solve
    /* Query and allocate the optimal workspace */
    int n = abSize, lda = abSize, info, lwork;
    double wkopt;
    lwork = -1;
    char jobz='V';
    char matType = 'U';
    dsyev_( &jobz, &matType, &n, &jMatrix_[0], &lda, &eigenValues_[0], &wkopt, &lwork, &info );
    lwork = (int)wkopt;
    work_.resize(lwork);
    // Solve eigenproblem. eigenvectors are stored in the jMatrix, columnwise
    dsyev_( &jobz, &matType, &n, &jMatrix_[0], &lda, &eigenValues_[0], &work_[0], &lwork, &info );

#ifdef WASATCH_QMOM_DIAGNOSTICS
    for (int iRow=0; iRow<abSize; iRow++) {
      printf("Eigen[%i] = %.12f \t \t",iRow,eigenValues_[iRow]);
    }
    printf("\n");
#endif

    //__________
    // save the weights and abscissae
    m0 = *knownMomentsIterators[0];
    for (int i=0; i<abSize; ++i) {
      const int matLoc = i*abSize;
      weights_[i] = jMatrix_[matLoc]*jMatrix_[matLoc]*m0;
    }
    for (int i=0; i<abSize; ++i) {
      const int matLoc = 2*i;
      *resultsIterators[matLoc] = weights_[i];
      *resultsIterators[matLoc + 1] = eigenValues_[i];
    }

#ifdef WASATCH_QMOM_DIAGNOSTICS
    for (int i=0; i<abSize; i++) {
      int matLoc = 2*i;
      printf("w[%i] = %.12f ",i,*resultsIterators[matLoc]);
      printf("a[%i] = %.12f ",i,*resultsIterators[matLoc+1]);
    }
    printf("\n");
    printf("__________________________________________________");
    printf("\n");
#endif

    //__________
    // increment iterators
    ++sampleIterator;
    for (int i=0; i<nMoments_; ++i) {
      knownMomentsIterators[i] += 1;
      resultsIterators[i] += 1;
    }
  }
}

#endif // QMOM_Expr_h
