/*______________________________________________________________________
*   To shutup the warning messages from -fullwarn (Steve Parker)
*_______________________________________________________________________*/
#if defined(__sgi) && !defined(__GNUC__) && (_MIPS_SIM != _MIPS_SIM_ABI32)
#pragma set woff 1174
#pragma set woff 3201
#endif

/*  
======================================================================*/
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h> 
#include "functionDeclare.h"
#define NR_END 1
#define FREE_ARG char*

void nrerror(char error_text[])
/* Numerical Recipes standard error handler */
{
	fprintf(stderr,"Numerical Recipes run-time error...\n");
	fprintf(stderr,"%s\n",error_text);
	fprintf(stderr,"...now exiting to system...\n");
	exit(1);
}

float *vector_nr(long nl, long nh)
/* allocate a float vector_nr with subscript range v[nl..nh] */
{
	float *v;

	v=(float *)malloc((size_t) ((nh-nl+1+NR_END)*sizeof(float)));
	if (!v) nrerror("allocation failure in vector_nr()");
	return v-nl+NR_END;
}

int *ivector_nr(long nl, long nh)
/* allocate an int vector with subscript range v[nl..nh] */
{
	int *v;

	v=(int *)malloc((size_t) ((nh-nl+1+NR_END)*sizeof(int)));
	if (!v) nrerror("allocation failure in ivector_nr()");
	return v-nl+NR_END;
}

unsigned char *cvector_nr(long nl, long nh)
/* allocate an unsigned char vector with subscript range v[nl..nh] */
{
	unsigned char *v;

	v=(unsigned char *)malloc((size_t) ((nh-nl+1+NR_END)*sizeof(unsigned char)));
	if (!v) nrerror("allocation failure in cvector_nr()");
	return v-nl+NR_END;
}

unsigned long *lvector_nr(long nl, long nh)
/* allocate an unsigned long vector with subscript range v[nl..nh] */
{
	unsigned long *v;

	v=(unsigned long *)malloc((size_t) ((nh-nl+1+NR_END)*sizeof(long)));
	if (!v) nrerror("allocation failure in lvector_nr()");
	return v-nl+NR_END;
}

double *dvector_nr(long nl, long nh)
/* allocate a double vector with subscript range v[nl..nh] */
{
	double *v;

	v=(double *)malloc((size_t) ((nh-nl+1+NR_END)*sizeof(double)));
	if (!v) nrerror("allocation failure in dvector_nr()");
	return v-nl+NR_END;
}

float **matrix(long n1dl, long n1dh, long n2dl, long n2dh)
/* allocate a float matrix with subscript range m[n1dl..n1dh][n2dl..n2dh] */
{
	long i, nrow=n1dh-n1dl+1,ncol=n2dh-n2dl+1;
	float **m;

	/* allocate pointers to rows */
	m=(float **) malloc((size_t)((nrow+NR_END)*sizeof(float*)));
	if (!m) nrerror("allocation failure 1 in matrix()");
	m += NR_END;
	m -= n1dl;

	/* allocate rows and set pointers to them */
	m[n1dl]=(float *) malloc((size_t)((nrow*ncol+NR_END)*sizeof(float)));
	if (!m[n1dl]) nrerror("allocation failure 2 in matrix()");
	m[n1dl] += NR_END;
	m[n1dl] -= n2dl;

	for(i=n1dl+1;i<=n1dh;i++) m[i]=m[i-1]+ncol;

	/* return pointer to array of pointers to rows */
	return m;
}

double **dmatrix(long n1dl, long n1dh, long n2dl, long n2dh)
/* allocate a double matrix with subscript range m[n1dl..n1dh][n2dl..n2dh] */
{
	long i, nrow=n1dh-n1dl+1,ncol=n2dh-n2dl+1;
	double **m;

	/* allocate pointers to rows */
	m=(double **) malloc((size_t)((nrow+NR_END)*sizeof(double*)));
	if (!m) nrerror("allocation failure 1 in matrix()");
	m += NR_END;
	m -= n1dl;

	/* allocate rows and set pointers to them */
	m[n1dl]=(double *) malloc((size_t)((nrow*ncol+NR_END)*sizeof(double)));
	if (!m[n1dl]) nrerror("allocation failure 2 in matrix()");
	m[n1dl] += NR_END;
	m[n1dl] -= n2dl;

	for(i=n1dl+1;i<=n1dh;i++) m[i]=m[i-1]+ncol;

	/* return pointer to array of pointers to rows */
	return m;
}

int **imatrix(long n1dl, long n1dh, long n2dl, long n2dh)
/* allocate a int matrix with subscript range m[n1dl..n1dh][n2dl..n2dh] */
{
	long i, nrow=n1dh-n1dl+1,ncol=n2dh-n2dl+1;
	int **m;

	/* allocate pointers to rows */
	m=(int **) malloc((size_t)((nrow+NR_END)*sizeof(int*)));
	if (!m) nrerror("allocation failure 1 in matrix()");
	m += NR_END;
	m -= n1dl;


	/* allocate rows and set pointers to them */
	m[n1dl]=(int *) malloc((size_t)((nrow*ncol+NR_END)*sizeof(int)));
	if (!m[n1dl]) nrerror("allocation failure 2 in matrix()");
	m[n1dl] += NR_END;
	m[n1dl] -= n2dl;

	for(i=n1dl+1;i<=n1dh;i++) m[i]=m[i-1]+ncol;

	/* return pointer to array of pointers to rows */
	return m;
}

float **submatrix(float **a, long oldrl, long oldrh, long oldcl, long oldch,
	long newrl, long newcl)
/* point a submatrix [newrl..][newcl..] to a[oldrl..oldrh][oldcl..oldch] */
{
	long i,j,nrow=oldrh-oldrl+1,ncol=oldcl-newcl;
	float **m;

	/* allocate array of pointers to rows */
	m=(float **) malloc((size_t) ((nrow+NR_END)*sizeof(float*)));
	if (!m) nrerror("allocation failure in submatrix()");
	m += NR_END;
	m -= newrl;

	/* set pointers to rows */
	for(i=oldrl,j=newrl;i<=oldrh;i++,j++) m[j]=a[i]+ncol;

	/* return pointer to array of pointers to rows */
	return m;
}

float **convert_matrix(float *a, long n1dl, long n1dh, long n2dl, long n2dh)
/* allocate a float matrix m[n1dl..n1dh][n2dl..n2dh] that points to the matrix
declared in the standard C manner as a[nrow][ncol], where nrow=n1dh-n1dl+1
and ncol=n2dh-n2dl+1. The routine should be called with the address
&a[0][0] as the first argument. */
{
	long i,j,nrow=n1dh-n1dl+1,ncol=n2dh-n2dl+1;
	float **m;

	/* allocate pointers to rows */
	m=(float **) malloc((size_t) ((nrow+NR_END)*sizeof(float*)));
	if (!m) nrerror("allocation failure in convert_matrix()");
	m += NR_END;
	m -= n1dl;

	/* set pointers to rows */
	m[n1dl]=a-n2dl;
	for(i=1,j=n1dl+1;i<nrow;i++,j++) m[j]=m[j-1]+ncol;
	/* return pointer to array of pointers to rows */
	return m;
}
float ***f3tensor(long n1dl, long n1dh, long n2dl, long n2dh, long n3dl, long n3dh)
/* allocate a float 3tensor with range t[n1dl..n1dh][n2dl..n2dh][n3dl..n3dh] */
{
	long i,j,nrow=n1dh-n1dl+1,ncol=n2dh-n2dl+1,ndep=n3dh-n3dl+1;
	float ***t;

	/* allocate pointers to pointers to rows */
	t=(float ***) malloc((size_t)((nrow+NR_END)*sizeof(float**)));
	if (!t) nrerror("allocation failure 1 in f3tensor()");
	t += NR_END;
	t -= n1dl;

	/* allocate pointers to rows and set pointers to them */
	t[n1dl]=(float **) malloc((size_t)((nrow*ncol+NR_END)*sizeof(float*)));
	if (!t[n1dl]) nrerror("allocation failure 2 in f3tensor()");
	t[n1dl] += NR_END;
	t[n1dl] -= n2dl;

	/* allocate rows and set pointers to them */
	t[n1dl][n2dl]=(float *) malloc((size_t)((nrow*ncol*ndep+NR_END)*sizeof(float)));
	if (!t[n1dl][n2dl]) nrerror("allocation failure 3 in f3tensor()");
	t[n1dl][n2dl] += NR_END;
	t[n1dl][n2dl] -= n3dl;

	for(j=n2dl+1;j<=n2dh;j++) t[n1dl][j]=t[n1dl][j-1]+ndep;
	for(i=n1dl+1;i<=n1dh;i++) {
		t[i]=t[i-1]+ncol;
		t[i][n2dl]=t[i-1][n2dl]+ncol*ndep;
		for(j=n2dl+1;j<=n2dh;j++) t[i][j]=t[i][j-1]+ndep;
	}

	/* return pointer to array of pointers to rows */
	return t;
}
/*______________________________________________________________________
*  Modified by Todd Harman 2.26.99
*-----------------------------------------------------------------------*/

double ***darray_3d(    long n1dl,      long n1dh, 
                        long n2dl,      long n2dh, 
                        long n3dl,      long n3dh)
/* allocate a double 3darray with range t[n1dl..n1dh][n2dl..n2dh][n3dl..n3dh] */
{
	long     i,j,
                nrow=n1dh-n1dl+1,
                ncol=n2dh-n2dl+1,
                ndep=n3dh-n3dl+1;
	double   ***t;

	/* allocate pointers to pointers to rows */
	t=(double ***) malloc((size_t)((nrow+NR_END)*sizeof(double**)));
	if (!t) nrerror("allocation failure 1 in darray_3d()");
	t += NR_END;
	t -= n1dl;

	/* allocate pointers to rows and set pointers to them */
	t[n1dl]=(double **) malloc((size_t)((nrow*ncol+NR_END)*sizeof(double*)));
	if (!t[n1dl]) nrerror("allocation failure 2 in darray_3d()");
	t[n1dl] += NR_END;
	t[n1dl] -= n2dl;

	/* allocate rows and set pointers to them */
	t[n1dl][n2dl]=(double *) malloc((size_t)((nrow*ncol*ndep+NR_END)*sizeof(double)));
	if (!t[n1dl][n2dl]) nrerror("allocation failure 3 in darray_3d()");
	t[n1dl][n2dl] += NR_END;
	t[n1dl][n2dl] -= n3dl;

/* 	for(j=n2dl+1;j<=n2dh;j++) t[n1dl][j]=t[n1dl][j-1]+ndep;
	for(i=n1dl+1;i<=n1dh;i++) {
		t[i]=t[i-1]+ncol;
		t[i][n2dl]=t[i-1][n2dl]+ncol*ndep;
		for(j=n2dl+1;j<=n2dh;j++) t[i][j]=t[i][j-1]+ndep;
	} */
/* New Stuff: */

    	for( i=1; i<nrow; i++ )
        {
	    t[i+n1dl] = t[i-1+n1dl] + ncol;
        }

	for( j=1; j<nrow*ncol; j++ )
	  {
	    t[n1dl][j+n2dl] = t[n1dl][j-1+n2dl] + ndep;
	  }
	/* return pointer to array of pointers to rows */
	return t;
}

/*__________________________________
*   integer 3D array
*___________________________________*/

int ***iarray_3d(long n1dl, long n1dh, long n2dl, long n2dh, long n3dl, long n3dh)
/* allocate a int 3darray with range t[n1dl..n1dh][n2dl..n2dh][n3dl..n3dh] */
{
	long i,j,k; 
       long nrow=n1dh-n1dl+1,ncol=n2dh-n2dl+1,ndep=n3dh-n3dl+1;
	int ***t;

	/* allocate pointers to pointers to rows */
	t=(int ***) malloc((size_t)((nrow+NR_END)*sizeof(int**)));
	if (!t) nrerror("allocation failure 1 in darray_3d()");
	t += NR_END;
	t -= n1dl;

	/* allocate pointers to rows and set pointers to them */
	t[n1dl]=(int **) malloc((size_t)((nrow*ncol+NR_END)*sizeof(int*)));
	if (!t[n1dl]) nrerror("allocation failure 2 in darray_3d()");
	t[n1dl] += NR_END;
	t[n1dl] -= n2dl;

	/* allocate rows and set pointers to them */
	t[n1dl][n2dl]=(int *) malloc((size_t)((nrow*ncol*ndep+NR_END)*sizeof(int)));
	if (!t[n1dl][n2dl]) nrerror("allocation failure 3 in darray_3d()");
	t[n1dl][n2dl] += NR_END;
	t[n1dl][n2dl] -= n3dl;

	for(j=n2dl+1;j<=n2dh;j++) t[n1dl][j]=t[n1dl][j-1]+ndep;
	for(i=n1dl+1;i<=n1dh;i++) {
		t[i]=t[i-1]+ncol;
		t[i][n2dl]=t[i-1][n2dl]+ncol*ndep;
		for(j=n2dl+1;j<=n2dh;j++) t[i][j]=t[i][j-1]+ndep;
	}
       
       for(j=n2dl;j<=n2dh;j++)
       { 
	     for(i=n1dl;i<=n1dh;i++) 
            {
		for(k=n3dl;k<=n3dh;k++) 
              {
                t[i][j][k] = 0;
              }
            }
        }
	/* return pointer to array of pointers to rows */
	return t;
}
/*__________________________________
*   double 4D array
*___________________________________*/
double ****darray_4d(   long n1dl,   long n1dh, 
		          long n2dl,   long n2dh,
		          long n3dl,   long n3dh,
		          long n4dl,  long n4dh)
/* allocate a double 4darray with range
t[n1dl..n1dh][n2dl..n2dh][n3dl..n3dh][n4dl..n4dh] */
{
	double ****t;
	long       i,j,k,
	           nrow=n1dh-n1dl+1,
	           ncol=n2dh-n2dl+1,
	           ndep=n3dh-n3dl+1,
	           n4d=n4dh-n4dl+1;

	/* allocate pointers to pointers to rows */
	t=(double ****) malloc((size_t)((nrow+NR_END)*sizeof(double***)));
	if (!t) nrerror("allocation failure 1 in darray_4d()");
	t += NR_END;
	t -= n1dl;

	/* allocate pointers to rows and set pointers to them */
	t[n1dl]=(double ***)malloc((size_t)((nrow*ncol+NR_END)*sizeof(double**)));
	if (!t[n1dl]) nrerror("allocation failure 2 in darray_4d()");
	t[n1dl] += NR_END;
	t[n1dl] -= n2dl;

	/* allocate rows and set pointers to them */
	t[n1dl][n2dl]=(double **)malloc((size_t)((nrow*ncol*ndep+NR_END)*sizeof(double*)));
	if (!t[n1dl][n2dl]) nrerror("allocation failure 3 in darray_4d()");
	t[n1dl][n2dl] += NR_END;
	t[n1dl][n2dl] -= n3dl;

	/* allocate the 4th dimension and set pointers to them */
	t[n1dl][n2dl][n3dl]=(double *)malloc((size_t)((nrow*ncol*ndep*n4d+NR_END)*sizeof(double)));
	if (!t[n1dl][n2dl][n3dl]) nrerror("allocation failure 4 in darray_4d()");
	t[n1dl][n2dl][n3dl] += NR_END;
	t[n1dl][n2dl][n3dl] -= n4dl;

/* New Stuff: */

    	for( i=1; i<nrow; i++ )
	  {
	    t[i+n1dl] = t[i-1+n1dl] + ncol;
	  }

	for( j=1; j<nrow*ncol; j++ )
	  {
	    t[n1dl][j+n2dl] = t[n1dl][j-1+n2dl] + ndep;
	  }

	for( k=1; k<nrow*ncol*ndep; k++ )
	  {
	    t[n1dl][n2dl][k+n3dl] = t[n1dl][n2dl][k-1+n3dl] + n4d;
	  }

	/* return pointer to array of pointers to rows */
	return t;
}
/*__________________________________
*   double 5D array
*___________________________________*/
double *****darray_5d(  long n1dl,   long n1dh, 
		          long n2dl,   long n2dh,
		          long n3dl,   long n3dh,
		          long n4dl,  long n4dh,
                        long n5dl,  long n5dh)
/* allocate a double 4darray with range
t[n1dl..n1dh][n2dl..n2dh][n3dl..n3dh][n4dl..n4dh][n5dl..n5dh] */
{
	double *****t;
	long       i,j,k,l,m,n,
	           nrow=n1dh-n1dl+1,
	           ncol=n2dh-n2dl+1,
	           ndep=n3dh-n3dl+1,
	           n4d=n4dh-n4dl+1,
                  n5d=n5dh-n5dl+1;

	/* allocate pointers to pointers to rows */
	t=(double *****) malloc((size_t)((nrow+NR_END)*sizeof(double****)));
	if (!t) nrerror("allocation failure 1 in darray_5d()");
	t += NR_END;
	t -= n1dl;
       
	t[n1dl]=(double ****) malloc((size_t)((nrow*ncol+NR_END)*sizeof(double***)));
	if (!t[n1dl]) nrerror("allocation failure 2 in darray_5d()");
	t[n1dl] += NR_END;
	t[n1dl] -= n2dl;

	/* allocate pointers to rows and set pointers to them */
	t[n1dl][n2dl]=(double ***)malloc((size_t)((nrow*ncol*ndep+NR_END)*sizeof(double**)));
	if (!t[n1dl][n2dl]) nrerror("allocation failure 3 in darray_5d()");
	t[n1dl][n2dl] += NR_END;
	t[n1dl][n2dl] -= n3dl;

	/* allocate rows and set pointers to them */
	t[n1dl][n2dl][n3dl]=(double **)malloc((size_t)((nrow*ncol*ndep*n4d+NR_END)*sizeof(double*)));
	if (!t[n1dl][n2dl][n3dl]) nrerror("allocation failure 4 in darray_5d()");
	t[n1dl][n2dl][n3dl] += NR_END;
	t[n1dl][n2dl][n3dl] -= n4dl;

	/* allocate the 4th dimension and set pointers to them */
	t[n1dl][n2dl][n3dl][n4dl]=(double *)malloc((size_t)((nrow*ncol*ndep*n4d*n5d+NR_END)*sizeof(double)));
	if (!t[n1dl][n2dl][n3dl][n4dl]) nrerror("allocation failure 5 in darray_5d()");
	t[n1dl][n2dl][n3dl][n4dl] += NR_END;
	t[n1dl][n2dl][n3dl][n4dl] -= n5dl;

/* New Stuff: */

    	for( i=1; i<nrow; i++ )
	  {
	    t[i+n1dl] = t[i-1+n1dl] + ncol;
	  }

	for( j=1; j<nrow*ncol; j++ )
	  {
	    t[n1dl][j+n2dl] = t[n1dl][j-1+n2dl] + ndep;
	  }

	for( k=1; k<nrow*ncol*ndep; k++ )
	  {
	    t[n1dl][n2dl][k+n3dl] = t[n1dl][n2dl][k-1+n3dl] + n4d;
	  }
       for( m=1; m<nrow*ncol*ndep*n4d; m++ )
	  {
	    t[n1dl][n2dl][n3dl][m+n4dl] = t[n1dl][n2dl][n3dl][m-1 +n4dl] + n5d;
	  }

	/* return pointer to array of pointers to rows */
       
	return t;
}

/*__________________________________
*   double 5D array
*___________________________________*/
double ******darray_6d(  long n1dl,   long n1dh, 
		          long n2dl,   long n2dh,
		          long n3dl,   long n3dh,
		          long n4dl,  long n4dh,
                        long n5dl,  long n5dh,
                        long n6dl,  long n6dh)
/* allocate a double 6 darray with range
t[n1dl..n1dh][n2dl..n2dh][n3dl..n3dh][n4dl..n4dh][n5dl..n5dh][n6dl..n6dh] */
{
	double ******t;
	long       i,j,k,l,m,n,
	           nrow=n1dh-n1dl+1,
	           ncol=n2dh-n2dl+1,
	           ndep=n3dh-n3dl+1,
	           n4d=n4dh-n4dl+1,
                  n5d=n5dh-n5dl+1,
                  n6d=n6dh-n6dl+1;

	/* allocate pointers to pointers to rows */
	t=(double ******) malloc((size_t)((nrow+NR_END)*sizeof(double *****)));
	if (!t) nrerror("allocation failure 1 in darray_6d()");
	t += NR_END;
	t -= n1dl;
       
	t[n1dl]=(double *****) malloc((size_t)((nrow*ncol+NR_END)*sizeof(double ****)));
	if (!t[n1dl]) nrerror("allocation failure 2 in darray_6d()");
	t[n1dl] += NR_END;
	t[n1dl] -= n2dl;

	/* allocate pointers to rows and set pointers to them */
	t[n1dl][n2dl]=(double ****)malloc((size_t)((nrow*ncol*ndep+NR_END)*sizeof(double ***)));
	if (!t[n1dl][n2dl]) nrerror("allocation failure 3 in darray_6d()");
	t[n1dl][n2dl] += NR_END;
	t[n1dl][n2dl] -= n3dl;

	/* allocate rows and set pointers to them */
	t[n1dl][n2dl][n3dl]=(double ***)malloc((size_t)((nrow*ncol*ndep*n4d+NR_END)*sizeof(double **)));
	if (!t[n1dl][n2dl][n3dl]) nrerror("allocation failure 4 in darray_6d()");
	t[n1dl][n2dl][n3dl] += NR_END;
	t[n1dl][n2dl][n3dl] -= n4dl;

	/* allocate the 4th dimension and set pointers to them */
	t[n1dl][n2dl][n3dl][n4dl]=(double **)malloc((size_t)((nrow*ncol*ndep*n4d*n5d+NR_END)*sizeof(double *)));
	if (!t[n1dl][n2dl][n3dl][n4dl]) nrerror("allocation failure 5 in darray_6d()");
	t[n1dl][n2dl][n3dl][n4dl] += NR_END;
	t[n1dl][n2dl][n3dl][n4dl] -= n5dl;
       
	/* allocate the 5dth dimension and set pointers to them */
	t[n1dl][n2dl][n3dl][n4dl][n5dl]=(double *)malloc((size_t)((nrow*ncol*ndep*n4d*n5d*n6d+NR_END)*sizeof(double)));
	if (!t[n1dl][n2dl][n3dl][n4dl][n5dl]) nrerror("allocation failure 6 in darray_6d()");
	t[n1dl][n2dl][n3dl][n4dl][n5dl] += NR_END;
	t[n1dl][n2dl][n3dl][n4dl][n5dl] -= n6dl;

/* Dav St. Germain: */

    	for( i=1; i<nrow; i++ )
	  {
	    t[i+n1dl] = t[i-1+n1dl] + ncol;
	  }

	for( j=1; j<nrow*ncol; j++ )
	  {
	    t[n1dl][j+n2dl] = t[n1dl][j-1+n2dl] + ndep;
	  }

	for( k=1; k<nrow*ncol*ndep; k++ )
	  {
	    t[n1dl][n2dl][k+n3dl] = t[n1dl][n2dl][k-1+n3dl] + n4d;
	  }
       for( m=1; m<nrow*ncol*ndep*n4d; m++ )
	  {
	    t[n1dl][n2dl][n3dl][m+n4dl] = t[n1dl][n2dl][n3dl][m-1 +n4dl] + n5d;
	  }
       for( n=1; n<nrow*ncol*ndep*n4d*n5d; n++ )
	  {
	    t[n1dl][n2dl][n3dl][n4dl][n+n5dl] = t[n1dl][n2dl][n3dl][n4dl][n-1 +n5dl] + n6d;
	  }

	/* return pointer to array of pointers to rows */
	return t;
}
/*__________________________________
* Converts 2D double array into a vector_nr
*___________________________________*/
float *convert_darray_2d_to_vector(double **t,      int n1dl,   int n1dh, 
                                    int n2dl,       int n2dh,   int *max_len)
{
    float *vect;
    int i, j, counter;
    *max_len    = (int)(n1dh- n1dl+ 1)*(n2dh - n2dl + 1);
    
    assert(*max_len >= 0);
    vect        = vector_nr(1,*max_len);
/*__________________________________
*
*___________________________________*/
    counter = 0;
    for(j = n2dl; j <= n2dh; j++)
    {
        for(i = n1dl; i <= n1dh; i++)
        {
            counter = counter + 1;
            vect[counter] = (float)t[i][j];
        }
    }
    
    return vect;
}
/*__________________________________
* Converts 3D double array into a vector_nr
*___________________________________*/
float *convert_darray_3d_to_vector( double ***t,    int n1dl,  int n1dh, 
                                    int n2dl,       int n2dh,  int n3dl,  int n3dh,
                                    int *max_len)
{
    float *vect;
    int i, j, k, counter;
    *max_len    = (int)(n1dh- n1dl+ 1)*(n2dh - n2dl + 1)* (n3dh - n3dl + 1);
    
    assert(*max_len >= 0);
    
   vect         = vector_nr(1,*max_len);
/*__________________________________
*
*___________________________________*/
    counter = 0;
    for(k = n3dl; k <= n3dh; k++)
    {
        for(j = n2dl; j <= n2dh; j++)
        {
            for(i = n1dl; i <= n1dh; i++)
            {
                counter = counter + 1;
                vect[counter] = (float)t[i][j][k];
                
            }
        }
    }
    return vect;
}

/*__________________________________
* Converts 3D int array into a vector_nr
*___________________________________*/
float *convert_iarray_3d_to_vector( int ***t,  int n1dl,  int n1dh, 
                                    int n2dl,  int n2dh,  int n3dl,  int n3dh,
                                    int *max_len)
{
    float *vect;
    int i, j, k, counter;
    *max_len     = (int)(n1dh- n1dl+ 1)*(n2dh - n2dl + 1)* (n3dh - n3dl + 1);
    
    assert(*max_len >= 0);
    vect = vector_nr(1,*max_len);
/*__________________________________
*
*___________________________________*/
    counter = 0;
    for(k = n3dl; k <= n3dh; k++)
    {
        for(j = n2dl; j <= n2dh; j++)
        {
            for(i = n1dl; i <= n1dh; i++)
            {
                counter = counter +1;
                vect[counter] = (float)t[i][j][k];
                
            }
        }
    }
    return vect;
}

/*__________________________________
* Converts 4D double array into a vector_nr
*___________________________________*/
float *convert_darray_4d_to_vector(
                                    double ****t,     int n1dl,  int n1dh, 
                                    int n2dl,         int n2dh,   int n3dl,  int n3dh,
                                    int n4l,          int n4h,
                                    int *max_len)
{
    float *vect;
    int i, j, k, m,counter;
    *max_len     = (int)(n1dh- n1dl+ 1)*(n2dh - n2dl + 1)* (n3dh - n3dl + 1)*
                    (n4h - n4l +1);
                    
   assert(*max_len > 0);
   vect = vector_nr(1,*max_len);
/*__________________________________
*
*___________________________________*/
    counter = 0;
    for(k = n3dl; k <= n3dh; k++)
    {
        for(j = n2dl; j <= n2dh; j++)
        {
            for(i = n1dl; i <= n1dh; i++)
            {
                for(m = n4l; m <= n4h; m++)
                {
                     counter = counter +1;
                    vect[counter] = (float)t[m][i][j][k];
                    
                }
            }
        }
    }
    return vect;
}
/*__________________________________
* Converts 5D array into a vector_nr
*___________________________________*/
#include <assert.h>
float *convert_darray_5d_to_vector( double *****t,  int n1dl,   int n1dh, 
                                    int n2dl,       int n2dh,   int n3dl,   int n3dh,
                                    int n4l,        int n4h,    int n5l,    int n5h,
                                    int *max_len,   int ptrflag)
{
    float   *vect;
    int     i, j, k, m, n,counter;
    assert (ptrflag == 0 || ptrflag == 1);
/*__________________________________
* Find the max len. of the vector
* and allocate memory for it
*___________________________________*/
    *max_len     = (int)(n1dh - n1dl + 1) *(n2dh - n2dl + 1)* (n3dh - n3dl + 1)*
                   (n4h - n4l + 1) *(n5h - n5l + 1);
                   
    assert(*max_len >= 0); 
   vect = vector_nr(1,*max_len);
/*__________________________________
*
*___________________________________*/
    counter = 0;
    for(k = n3dl; k <= n3dh; k++)
    {
        for(j = n2dl; j <= n2dh; j++)
        {
            for(i = n1dl; i <= n1dh; i++)
            {
                for(m = n4l; m <= n4h; m++)
                {
                    for(n = n5l; n <= n5h; n++)
                    {
                    counter = counter +1;
                    if (ptrflag == 0 )
                        vect[counter] = (float)t[i][j][k][m][n];
                
                    if (ptrflag == 1)
                        vect[counter] = (float)*t[m][i][j][k];                            
                    
                    
                    }
                }
            }
        }
    }
    return vect;
}
/*__________________________________
* Converts 6D array into a vector_nr
* Note this assumes that the array 
* is *array[i][j][k][f][m]
*___________________________________*/
#include <assert.h>
#include <stdio.h>
float *convert_darray_6d_to_vector( double ******t,  
                                    int n1dl,       int n1dh, 
                                    int n2dl,       int n2dh,   int n3dl,   int n3dh,
                                    int n4l,        int n4h,    int n5l,    int n5h,
                                    int *max_len)
{
    float   *vect;
    int     i, j, k, m, counter,
            bulletproof;
/*__________________________________
*   Bullet proofing
*   Make soure that you only are looping
*   in one direction
*___________________________________*/    
    bulletproof = 0;
    if ( (n1dh - n1dl) > 0) bulletproof++;
    if ( (n2dh - n2dl) > 0) bulletproof++;
    if ( (n3dh - n3dl) > 0) bulletproof++;
    if ( (n5h  - n5l)  > 0) bulletproof++;
    if (bulletproof > 1)
    {
        fprintf(stderr,"\n\n ______________________________________________\n");
        fprintf(stderr,"nrutil+.c\n");
        fprintf(stderr,"convert_darray_6d_to_vector\n");
        fprintf(stderr,"Currently this function only works if you loop in ONE direction\n");
        fprintf(stderr,"\n ______________________________________________\n");
        exit(1);
    }

/*__________________________________
* Find the max len. of the vector
* and allocate memory for it
*___________________________________*/
    *max_len     = (int)(n1dh - n1dl + 1) *(n2dh - n2dl + 1)* (n3dh - n3dl + 1)*
                    (n5h - n5l + 1);
                   
   assert(*max_len >= 0); 
   vect = vector_nr(1,*max_len);
/*__________________________________
*
*___________________________________*/
    counter = 0;
    for(k = n3dl; k <= n3dh; k++)
    {
        for(j = n2dl; j <= n2dh; j++)
        {
            for(i = n1dl; i <= n1dh; i++)
            {
                for(m = n5l; m <= n5h; m++)
                {
                     counter = counter +1;
                     vect[counter] = (float)*t[i][j][k][n4l][m]; 
                     /*__________________________________
                     * At either face
                     *___________________________________*/        
                     if (counter == 1)
                         vect[counter] = (float)*t[i][j][k][n4l][m]; 

                     if (counter == *max_len)
                         vect[counter] = (float)*t[i][j][k][n4h][m]; 
                }
            }
        }
    }
    return vect;
}
/*______________________________________________________________________
*
*-----------------------------------------------------------------------*/
void free_vector_nr(float *v, long nl, long nh)
/* free a float vector allocated with vector_nr() */
{
	free((FREE_ARG) (v+nl-NR_END));
}

void free_ivector_nr(int *v, long nl, long nh)
/* free an int vector allocated with ivector_nr() */
{
	free((FREE_ARG) (v+nl-NR_END));
}

void free_cvector_nr(unsigned char *v, long nl, long nh)
/* free an unsigned char vector allocated with cvector_nr() */
{
	free((FREE_ARG) (v+nl-NR_END));
}

void free_lvector_nr(unsigned long *v, long nl, long nh)
/* free an unsigned long vector allocated with lvector_nr() */
{
	free((FREE_ARG) (v+nl-NR_END));
}

void free_dvector_nr(double *v, long nl, long nh)
/* free a double vector allocated with dvector_nr() */
{
	free((FREE_ARG) (v+nl-NR_END));
}

void free_matrix(float **m, long n1dl, long n1dh, long n2dl, long n2dh)
/* free a float matrix allocated by matrix() */
{
	free((FREE_ARG) (m[n1dl]+n2dl-NR_END));
	free((FREE_ARG) (m+n1dl-NR_END));
}

void free_dmatrix(double **m, long n1dl, long n1dh, long n2dl, long n2dh)
/* free a double matrix allocated by dmatrix() */
{
	free((FREE_ARG) (m[n1dl]+n2dl-NR_END));
	free((FREE_ARG) (m+n1dl-NR_END));
}

void free_imatrix(int **m, long n1dl, long n1dh, long n2dl, long n2dh)
/* free an int matrix allocated by imatrix() */
{
	free((FREE_ARG) (m[n1dl]+n2dl-NR_END));
	free((FREE_ARG) (m+n1dl-NR_END));
}

void free_submatrix(float **b, long n1dl, long n1dh, long n2dl, long n2dh)
/* free a submatrix allocated by submatrix() */
{
	free((FREE_ARG) (b+n1dl-NR_END));
}

void free_convert_matrix(float **b, long n1dl, long n1dh, long n2dl, long n2dh)
/* free a matrix allocated by convert_matrix() */
{
	free((FREE_ARG) (b+n1dl-NR_END));
}

void free_f3tensor(float ***t, long n1dl, long n1dh, long n2dl, long n2dh,
	long n3dl, long n3dh)
/* free a float f3tensor allocated by f3tensor() */
{
	free((FREE_ARG) (t[n1dl][n2dl]+n3dl-NR_END));
	free((FREE_ARG) (t[n1dl]+n2dl-NR_END));
	free((FREE_ARG) (t+n1dl-NR_END));
}

/*__________________________________
* Added by Todd Harman 2.29.99
*___________________________________*/
void free_darray_3d(double ***t, long n1dl, long n1dh, long n2dl, long n2dh,
	long n3dl, long n3dh)
/* free a double 3darray allocated by array_3d() */
{
	free((FREE_ARG) (t[n1dl][n2dl]+n3dl-NR_END));
	free((FREE_ARG) (t[n1dl]+n2dl-NR_END));
	free((FREE_ARG) (t+n1dl-NR_END));
}

void free_iarray_3d(int ***t, long n1dl, long n1dh, long n2dl, long n2dh,
	long n3dl, long n3dh)
/* free a double 3darray allocated by array_3d() */
{
	free((FREE_ARG) (t[n1dl][n2dl]+n3dl-NR_END));
	free((FREE_ARG) (t[n1dl]+n2dl-NR_END));
	free((FREE_ARG) (t+n1dl-NR_END));
}

void free_darray_4d(double ****t, long n1dl, long n1dh, long n2dl, long n2dh,
	long n3dl, long n3dh, long n4dl, long n4dh)
/* free a double 4d array allocated by array_4d() */
{
       free((FREE_ARG) (t[n1dl][n2dl][n3dl]+n4dl-NR_END));
	free((FREE_ARG) (t[n1dl][n2dl]+n3dl-NR_END));
	free((FREE_ARG) (t[n1dl]+n2dl-NR_END));
	free((FREE_ARG) (t+n1dl-NR_END));
}

void free_darray_5d(double *****t, long n1dl, long n1dh, long n2dl, long n2dh,
	long n3dl, long n3dh, long n4dl, long n4dh, long n5dl, long n5dh)
/* free a double 5d array allocated by array_5d() */
{
       free((FREE_ARG) (t[n1dl][n2dl][n3dl][n4dl]+n5dl-NR_END));
       free((FREE_ARG) (t[n1dl][n2dl][n3dl]+n4dl-NR_END));
	free((FREE_ARG) (t[n1dl][n2dl]+n3dl-NR_END));
	free((FREE_ARG) (t[n1dl]+n2dl-NR_END));
	free((FREE_ARG) (t+n1dl-NR_END));
}

void free_darray_6d(double ******t, long n1dl, long n1dh, long n2dl, long n2dh,
	long n3dl,    long n3dh,   long n4dl,  long n4dh,  long n5dl,  long n5dh,
       long n6dl,   long n6dh)
/* free a double 6d array allocated by array_6d() */
{
       free((FREE_ARG) (t[n1dl][n2dl][n3dl][n4dl][n5dl]+n6dl-NR_END));
       free((FREE_ARG) (t[n1dl][n2dl][n3dl][n4dl]+n5dl-NR_END));
       free((FREE_ARG) (t[n1dl][n2dl][n3dl]+n4dl-NR_END));
	free((FREE_ARG) (t[n1dl][n2dl]+n3dl-NR_END));
	free((FREE_ARG) (t[n1dl]+n2dl-NR_END));
	free((FREE_ARG) (t+n1dl-NR_END));
}


/*______________________________________________________________________
*   To shutup the warning messages from -fullwarn (Steve Parker)
*_______________________________________________________________________*/
#if defined(__sgi) && !defined(__GNUC__) && (_MIPS_SIM != _MIPS_SIM_ABI32)
#pragma reset woff 1174
#endif
