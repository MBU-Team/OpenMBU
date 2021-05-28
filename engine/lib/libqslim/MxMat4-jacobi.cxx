/************************************************************************

  Jacobi iteration on 4x4 matrices.

  Copyright (C) 1998 Michael Garland.  See "COPYING.txt" for details.

  NOTE: This code was adapted from VTK source code (vtkMath.cxx) which
        seems to have been adapted directly from Numerical Recipes in C.
	See Hoppe's Recon software for an alternative adaptation of the
	Numerical Recipes code.
  
  $Id: MxMat4-jacobi.cxx,v 1.2 2006/09/07 00:05:52 bramage Exp $

 ************************************************************************/

#include "stdmix.h"
#include "MxMat4.h"

#define ROT(a,i,j,k,l) g=a[i][j];h=a[k][l];a[i][j]=g-s*(h+g*tau);a[k][l]=h+s*(g-h*tau);

#define MAX_ROTATIONS 60

static
bool internal_jacobi4(double a[4][4], double w[4], double v[4][4])
{
    int i, j, k, iq, ip;
    double tresh, theta, tau, t, sm, s, h, g, c;
    double b[4], z[4], tmp;

    // initialize
    for (ip=0; ip<4; ip++) 
    {
	for (iq=0; iq<4; iq++) v[ip][iq] = 0.0;
	v[ip][ip] = 1.0;
    }
    for (ip=0; ip<4; ip++) 
    {
	b[ip] = w[ip] = a[ip][ip];
	z[ip] = 0.0;
    }

    // begin rotation sequence
    for (i=0; i<MAX_ROTATIONS; i++) 
    {
	sm = 0.0;
	for (ip=0; ip<3; ip++) 
	{
	    for (iq=ip+1; iq<4; iq++) sm += fabs(a[ip][iq]);
	}
	if (sm == 0.0) break;

	if (i < 4) tresh = 0.2*sm/(16);
	else tresh = 0.0;

	for (ip=0; ip<3; ip++) 
	{
	    for (iq=ip+1; iq<4; iq++) 
	    {
		g = 100.0*fabs(a[ip][iq]);
		if (i > 4 && (fabs(w[ip])+g) == fabs(w[ip])
		    && (fabs(w[iq])+g) == fabs(w[iq]))
		{
		    a[ip][iq] = 0.0;
		}
		else if (fabs(a[ip][iq]) > tresh) 
		{
		    h = w[iq] - w[ip];
		    if ( (fabs(h)+g) == fabs(h)) t = (a[ip][iq]) / h;
		    else 
		    {
			theta = 0.5*h / (a[ip][iq]);
			t = 1.0 / (fabs(theta)+sqrt(1.0+theta*theta));
			if (theta < 0.0) t = -t;
		    }
		    c = 1.0 / sqrt(1+t*t);
		    s = t*c;
		    tau = s/(1.0+c);
		    h = t*a[ip][iq];
		    z[ip] -= h;
		    z[iq] += h;
		    w[ip] -= h;
		    w[iq] += h;
		    a[ip][iq]=0.0;
		    for (j=0;j<ip-1;j++) 
		    {
			ROT(a,j,ip,j,iq)
			    }
		    for (j=ip+1;j<iq-1;j++) 
		    {
			ROT(a,ip,j,j,iq)
			    }
		    for (j=iq+1; j<4; j++) 
		    {
			ROT(a,ip,j,iq,j)
			    }
		    for (j=0; j<4; j++) 
		    {
			ROT(v,j,ip,j,iq)
			    }
		}
	    }
	}

	for (ip=0; ip<4; ip++) 
	{
	    b[ip] += z[ip];
	    w[ip] = b[ip];
	    z[ip] = 0.0;
	}
    }

    if ( i >= MAX_ROTATIONS )
    {
	mxmsg_signal(MXMSG_WARN, "Error computing eigenvalues.", "jacobi");
	return false;
    }

    // sort eigenfunctions
    for (j=0; j<4; j++) 
    {
	k = j;
	tmp = w[k];
	for (i=j; i<4; i++)
	{
	    if (w[i] >= tmp) 
	    {
		k = i;
		tmp = w[k];
	    }
	}
	if (k != j) 
	{
	    w[k] = w[j];
	    w[j] = tmp;
	    for (i=0; i<4; i++) 
	    {
		tmp = v[i][j];
		v[i][j] = v[i][k];
		v[i][k] = tmp;
	    }
	}
    }
    // insure eigenvector consistency (i.e., Jacobi can compute vectors that
    // are negative of one another (.707,.707,0) and (-.707,-.707,0). This can
    // reek havoc in hyperstreamline/other stuff. We will select the most
    // positive eigenvector.
    int numPos;
    for (j=0; j<4; j++)
    {
	for (numPos=0, i=0; i<4; i++) if ( v[i][j] >= 0.0 ) numPos++;
	if ( numPos < 3 ) for(i=0; i<4; i++) v[i][j] *= -1.0;
    }

    return true;
}


#undef ROT
#undef MAX_ROTATIONS

bool jacobi(const Mat4& m, Vec4& eig_vals, Vec4 eig_vecs[4])
{
    double a[4][4], w[4], v[4][4];
    int i,j;

    for(i=0;i<4;i++) for(j=0;j<4;j++) a[i][j] = m(i,j);

    bool result = internal_jacobi4(a, w, v);
    if( result )
    {
	for(i=0;i<4;i++) eig_vals[i] = w[i];

	for(i=0;i<4;i++) for(j=0;j<4;j++) eig_vecs[i][j] = v[j][i];
    }

    return result;
}
