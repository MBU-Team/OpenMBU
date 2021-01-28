/**********************************************************************
 *<
	FILE: IRefArray.h

	DESCRIPTION:  An interface to facilitate maintaining multiple arrays of references.

	CREATED BY: John Hutchinson

	HISTORY: created 9/1/98

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/
#pragma once
#ifndef _IREFARRAY
#define _IREFARRAY

//Forward declarations
typedef Tab<ReferenceTarget*> ReferenceArray;




class IRefArray {
public :
	virtual int NumRefCols() const = 0; // The size of my arrays (same size!)
	virtual int NumRefRows() const = 0; // The number of arrays I maintain including inner
	virtual void EnlargeAndInitializeArrays(int newsize) = 0; //allocator
	virtual ReferenceArray& RefRow(int which)= 0; //access to the rows
	virtual const ReferenceArray& RefRow(int which) const = 0; //const version of above

protected:
	virtual int TotalRefs() const = 0; // Equivalent of NumRefs

	virtual IRefArray* Inner() = 0; //PI Access to the containee
	virtual const IRefArray* Inner() const = 0; //PI Access to the containee
	virtual int InnerArrays()  const = 0; //the number of arrays in any containee
	virtual int Remap(int row, int col) const = 0; //Converts an index for processing by the inner

	virtual int ArrayOffset() const = 0; // the number of special (non-array) references
	virtual bool IsSubArrayIndex(int i) const = 0;
	virtual RefTargetHandle GetSubArrayReference(int i) = 0;//
	virtual void SetSubArrayReference(int i, RefTargetHandle rtarg) = 0;

public:
	//Referencemaker overloads
	virtual RefTargetHandle GetReference(int row, int col) = 0;
	virtual void SetReference(int row, int col, RefTargetHandle rtarg) = 0;
	virtual RefResult MakeRefByID(Interval refInterval, int row, int col, RefTargetHandle rtarg) = 0;
	virtual RefResult ReplaceReference(int row, int col, RefTargetHandle newtarg, BOOL delOld=TRUE) = 0;
	virtual RefResult DeleteReference(int row, int col) = 0;
	virtual BOOL CanTransferReference(int row, int col) = 0;
	virtual bool FindRef(RefTargetHandle rtarg, int& row, int& col) = 0;
	virtual void CloneRefs(ReferenceMaker *, RemapDir& remap) = 0;
};
#endif //_IREFARRAY
