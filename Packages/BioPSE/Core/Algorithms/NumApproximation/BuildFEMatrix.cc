/*
  The contents of this file are subject to the University of Utah Public
  License (the "License"); you may not use this file except in compliance
  with the License.
  
  Software distributed under the License is distributed on an "AS IS"
  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
  License for the specific language governing rights and limitations under
  the License.
  
  The Original Source Code is SCIRun, released March 12, 2001.
  
  The Original Source Code was developed by the University of Utah.
  Portions created by UNIVERSITY are Copyright (C) 2001, 1994 
  University of Utah. All Rights Reserved.
*/

/*
 *  BuildFEMatrix.cc:  Build finite element matrix
 *
 *  Written by:
 *   Alexei Samsonov
 *   Department of Computer Science
 *   University of Utah
 *   March 2001   
 *  Copyright (C) 2001 SCI Group
 */

#include <Packages/BioPSE/Core/Algorithms/NumApproximation/BuildFEMatrix.h>
#include <iostream>

namespace BioPSE {

using namespace SCIRun;

//! Constructor
// -- it's private, no occasional object creation
BuildFEMatrix::BuildFEMatrix(TetVolIntHandle hField,
			     DirichletBC&  dirBC,
			     Array1<Tensor>& tens,
			     MatrixHandle& hA, 
			     MatrixHandle& hRhs,
			     int np):
  // ---------------------------------------------
  hField_(hField),
  dirBC_(dirBC),
  hA_(hA),
  hRhs_(hRhs),
  np_(np),
  barrier_("BuildFEMatrix barrier"),
  colIdx_(np+1),
  tens_(tens)
  //pModule(pM),
  //msgStream_(*(pM->msgStream))
{
  hMesh_=hField->get_typed_mesh();
  int nNodes = hMesh_->nodes_size();
  rows_ = scinew int[nNodes+1];
}
BuildFEMatrix::~BuildFEMatrix(){}

bool BuildFEMatrix::build_FEMatrix(TetVolIntHandle hField,
				   DirichletBC& dirBC,
				   Array1<Tensor>& tens,
				   MatrixHandle& hA, 
				   MatrixHandle& hRhs)
  //------------------------------------------------
{
  int np=Thread::numProcessors();
  
  if (np>10) {
    np=5;
  }
  
  hA = 0;
  hRhs = 0;

  BuildFEMatrixHandle hMaker = new BuildFEMatrix(hField, dirBC, tens, hA, hRhs, np);
  
  Thread::parallel(Parallel<BuildFEMatrix>(hMaker.get_rep(), &BuildFEMatrix::parallel),
		   np, true);
  
  // -- refer to the object one more time not to make it die before
  hMaker = 0;
  
  if (hA.get_rep() && hRhs.get_rep())
    return true;
  else
    return false;
}

// -- callback routine to execute in parallel
void BuildFEMatrix::parallel(int proc)
{
  //! dividing nodes among processors
  int nNodes     = hMesh_->nodes_size();
  int start_node = nNodes * proc/np_;
  int end_node   = nNodes * (proc+1)/np_;
  int ndof       = end_node - start_node;
  
  int r = start_node;
  int i;

  //----------------------------------------------------------------------
  //! Creating sparse matrix structure
  Array1<int> mycols(0, 15*ndof);

  for(i=start_node;i<end_node;i++){
    rows_[r++]=mycols.size();
    TetVolMesh::node_array neib_nodes;

    hMesh_->compute_node_neighbors();
    hMesh_->get_neighbors(neib_nodes, i);

    for (int jj=0; jj<neib_nodes.size(); jj++)
      mycols.add(neib_nodes[jj]);
  }
  
  colIdx_[proc]=mycols.size();
  
  //! check point
  barrier_.wait(np_);
  
  int st=0;
  if (proc == 0){
    
    //pModule->update_progress(2,6);
    
    for(i=0;i<np_;i++){
      int ns=st+colIdx_[i];
      colIdx_[i]=st;
      st=ns;
    }
    
    colIdx_[np_]=st;
    //msgStream_ << "st=" << st << endl;
    allCols_=scinew int[st];
  }

  //! check point
  barrier_.wait(np_);
  
  //! updating global column by each of the processors
  int s=colIdx_[proc];  
  int n=mycols.size();
  
  for(i=0;i<n;i++){
    allCols_[i+s]=mycols[i];
  }
  
  for(i=start_node;i<end_node;i++){
    rows_[i]+=s;
  }

  //! check point
  barrier_.wait(np_);
  
  //! the main thread makes the matrix and rhs...
  if(proc == 0){
    rows_[nNodes]=st;
    //pModule->update_progress(3,6);
    
    pA_ = scinew SparseRowMatrix(nNodes, nNodes, rows_, allCols_, st);
    pRhs_ = scinew ColumnMatrix(nNodes);
    hA_ = pA_;
    hRhs_ = pRhs_;
  }

  //! check point
  barrier_.wait(np_);
  
  
  //! zeroing in parallel
  double* a = pA_->a;
  for(i=start_node; i<end_node; i++){
    (*pRhs_)[i]=0;
  }
  
  int ns=colIdx_[proc];
  int ne=colIdx_[proc+1];

  for(i=ns;i<ne;i++){
    a[i]=0;
  }

  //----------------------------------------------------------
  //! Filling the matrix

  //! Dividing cells among processors
  //int nCells=hMesh_->cells_size();
  //TetVolMesh::cell_index cind1 = nCells * proc/np_;
  //TetVolMesh::cell_index cind2 = nCells * (proc+1)/np_;
  
  TetVolMesh::cell_iterator ii;
  
  double lcl_matrix[4][4];
 
  for (ii=hMesh_->cell_begin(); ii!=hMesh_->cell_end(); ++ii){
    TetVolMesh::node_array  cell_nodes;
    hMesh_->get_nodes(cell_nodes, *ii); 
    
    //! calculate local matrix if at least one node in the cell belongs to the process nodes range
    if (  (cell_nodes[0] >= start_node && cell_nodes[0] < end_node)
       || (cell_nodes[1] >= start_node && cell_nodes[1] < end_node)
       || (cell_nodes[2] >= start_node && cell_nodes[2] < end_node)
       || (cell_nodes[3] >= start_node && cell_nodes[3] < end_node)){
      
      build_local_matrix(lcl_matrix, *ii);
      add_lcl_gbl(lcl_matrix, *ii, start_node, end_node);
    }
  }
  
  barrier_.wait(np_);

  //! adjusting matrix for Dirichlet BC on first processor
  // --  no parralelization here, no many Dirichlet nodes
  Array1<int> idcNz;
  Array1<double> valNz;

  TetVolMesh::node_iterator niter;
  TetVolMesh::node_array nind;
  vector<double> dbc;

  if (proc==0){
    for(int idx = 0; idx<dirBC_.size(); ++idx){
      int ni = dirBC_[i].first;
      int val = dirBC_[i].second;

      // -- getting column indices of non-zero elements for the current row
      pA_->getRowNonzeros(ni, idcNz, valNz);
      
      // -- updating rhs
      for (int i=0; i<idcNz.size(); ++i){
	int j = idcNz[i];
	(*pRhs_)[j] +=-val*valNz[i]; 
      };
    }

    //! zeroing matrix row and column corresponding to the dirichlet nodes
    for(int idx = 0; idx<dirBC_.size(); ++idx){
      int ni = dirBC_[i].first;
      int val = dirBC_[i].second;

      pA_->getRowNonzeros(ni, idcNz, valNz);
      for (int i=0; i<idcNz.size(); ++i){
	int j = idcNz[i];
	pA_->put(ni, j, 0);
	pA_->put(j, ni, 0); 
      }
      //! updating dirichlet node and corresponding entry in rhs
      pA_->put(ni, ni, 1);
      (*pRhs_)[ni] = val;
    }
  }
}

void BuildFEMatrix::build_local_matrix(double lcl_a[4][4], TetVolMesh::cell_index c_ind)
{
  Vector grad1, grad2, grad3, grad4;
  double vol = hMesh_->get_gradient_basis(c_ind, grad1, grad2, grad3, grad4);
  int  ind = hField_->value(c_ind);

  double (&el_cond)[3][3] = tens_[ind].mat_;
  
  if(vol < 1.e-10){
    for(int i = 0; i<4; i++)
      for(int j = 0; j<4; j++)
	lcl_a[i][j]=0;
    return;
  }
  
  double el_coefs[4][3];
  
  // -- this 4x3 array holds the 3 gradients to be used 
  // as coefficients for each of the four nodes of the 
  // element
  
  el_coefs[0][0]=grad1.x();
  el_coefs[0][1]=grad1.y();
  el_coefs[0][2]=grad1.z();
  
  el_coefs[1][0]=grad2.x();
  el_coefs[1][1]=grad2.y();
  el_coefs[1][2]=grad2.z();
  
  el_coefs[2][0]=grad3.x();
  el_coefs[2][1]=grad3.y();
  el_coefs[2][2]=grad3.z();
  
  el_coefs[3][0]=grad4.x();
  el_coefs[3][1]=grad4.y();
  el_coefs[3][2]=grad4.z();
  
  // build the local matrix
  for(int i=0; i< 4; i++) {
    for(int j=0; j< 4; j++) {
      lcl_a[i][j] = 0.0;
      for (int k=0; k< 3; k++){
	for (int l=0; l<3; l++){
	  lcl_a[i][j] += 
	    el_cond[k][l]*el_coefs[i][k]*el_coefs[j][l];
	}
      }
      lcl_a[i][j] *= vol;
    }
  }
}

void BuildFEMatrix::add_lcl_gbl(double lcl_a[4][4], TetVolMesh::cell_index c_ind, int s, int e)
{
  TetVolMesh::node_array cell_nodes;

  hMesh_->get_nodes(cell_nodes, c_ind); 
  for (int i=0; i<4; i++) {
    int ii = cell_nodes[i];
    if (ii>=s && ii<e)          //! the row to update belongs to the process, proceed...
      for (int j=0; j<4; j++) {      
	int jj = cell_nodes[j];
	pA_->get(ii, jj) += lcl_a[i][j];
      }
  }
}

void BuildFEMatrix::io(Piostream&){
  
}

} // end namespace BioPSE
