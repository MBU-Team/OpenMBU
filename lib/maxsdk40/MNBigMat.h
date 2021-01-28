// MNBigMat.h
// Created by Steve Anderson, Nov. 22 1996.

// BigMatrix is for when I need good old-fashioned mxn matrices.

// Classes:
// BigMatrix

#ifndef __MN_BIGMAT_H_
#define __MN_BIGMAT_H_

#define BIGMAT_MAX_SIZE 10000

class BigMatrix {
public:
	int m, n;
	float *val;

	BigMatrix () { val=NULL; m=0; n=0; }
	DllExport BigMatrix (int mm, int nn);
	DllExport BigMatrix (const BigMatrix & from);
	~BigMatrix () { Clear (); }

	DllExport void Clear ();
	DllExport int SetSize (int mm, int nn);

	DllExport float *operator[](int i) const;
	DllExport BigMatrix & operator= (const BigMatrix & from);

	DllExport void SetTranspose (BigMatrix & trans) const;
	DllExport float Invert();
	DllExport void Identity ();

	// Debugging functions:
	DllExport void Randomize (float scale);
	DllExport void MNDebugPrint ();

	// Do not use -- does nothing.  (Replaced by MNDebugPrint.)
	DllExport void dump (FILE *fp);
};

DllExport extern BOOL BigMatMult (BigMatrix & a, BigMatrix & b, BigMatrix &c);

#endif
