/**********************************************************************
 *<
	FILE: bitarray.h

	DESCRIPTION:

	CREATED BY: Dan Silva

	HISTORY:

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef BITARRAY__H
#define BITARRAY__H

#include <windef.h>
#include <ioapi.h>

class BitArrayCallback {
	public:
	virtual void proc(int n)=0;
	};

// Direction indicators for BitArray::Rotate and BitArray::Shift
#define LEFT_BITSHIFT 0
#define RIGHT_BITSHIFT 1

class BitArray {
	DWORD* bits;
	long numBits;
	public:
		DllExport void SetSize(int n, int save=0);  // save=1:preserve old bit values
		int	GetSize() const { return numBits; }
		DllExport void ClearAll();
		DllExport void SetAll();
	    DllExport void Set(int i);       	// set ith bit to 1
	    DllExport void Clear(int i);     	// set ith bit to 0 
		DllExport void Set(int i, int b);  // set ith bit to b
		DllExport int operator[](int i) const;  // get ith bit
		DllExport int NumberSet();      // how many bits are 1's.
		DllExport BOOL IsEmpty();      // are NO bits set?.  much faster than NumberSet
		DllExport void Compress();    
		DllExport void Expand();
		DllExport void Reverse(BOOL keepZero = FALSE);	// keepZero=TRUE keeps zero bit where it is
		DllExport void Rotate(int direction, int count);				// With wraparound
		DllExport void Shift(int direction, int count, int where=0);	// Without wraparound
		DllExport void EnumSet(BitArrayCallback &cb);  // enumerates elements that are 1's
		DllExport void DeleteSet (BitArray & dset, int mult=1);
		DllExport IOResult Save(ISave* isave);
		DllExport IOResult Load(ILoad* iload);

		BitArray() { bits = NULL; 	numBits = 0; }
		DllExport BitArray(int n);
		DllExport BitArray(const BitArray& b);
		DllExport ~BitArray();

		// Comparison operator
		DllExport BOOL operator==(const BitArray& b);
		
		// Assignment operators
		DllExport BitArray& operator=(const BitArray& b);

		// Assignment operators: These require arrays of the same size!
		DllExport BitArray& operator&=(const BitArray& b);  // AND=
		DllExport BitArray& operator|=(const BitArray& b);  // OR=
		DllExport BitArray& operator^=(const BitArray& b);  // XOR=

		// Binary operators: These require arrays of the same size!
		DllExport BitArray operator&(const BitArray&) const; // AND
		DllExport BitArray operator|(const BitArray&) const; // OR
		DllExport BitArray operator^(const BitArray&) const; // XOR

		// Unary operators
		DllExport BitArray operator~(); // unary NOT function

	};






#endif
