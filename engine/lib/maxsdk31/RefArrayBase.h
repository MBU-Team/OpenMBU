/**********************************************************************
 *<
 FILE: RefArrayBase.h

	DESCRIPTION:  A base class for implementing objects which need to maintain
	multiple arrays of references. This implementation makes the following assumptions:

	1) The arrays are implemented as Tab<RefTargetHandle>

	CREATED BY: John Hutchinson

	HISTORY: created 8/24/98

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/
#pragma once
#ifndef _REF_ARRAY_BASE
#define _REF_ARRAY_BASE

#include "object.h"
#include "Irefarray.h"

//SS 4/12/99: Now derives from ShapeObject; if your reference array only
// supports mesh geometry, override SuperClassID() and IsShapeObject().
class RefArrayBase : public ShapeObject, public IRefArray
{
public:
		CoreExport virtual int NumRefCols() const; // the size of arrays (the same size!)

protected:
		//From IRefArray
		CoreExport virtual int TotalRefs() const;

		CoreExport virtual IRefArray* Inner(){return NULL;} //Access to the containee
		CoreExport virtual const IRefArray* Inner() const{return NULL;} //Access to the containee
		CoreExport virtual int InnerArrays() const; //the number of arrays in any containee
		CoreExport virtual int Remap(int row, int col) const; //Converts to linear, inner index

		CoreExport virtual int ArrayOffset() const{return 0;} //The number of non-array references
		CoreExport virtual bool IsSubArrayIndex(int i) const {return i< ArrayOffset();}
		CoreExport virtual RefTargetHandle GetSubArrayReference(int i) {assert(0); return NULL;}
		CoreExport virtual void SetSubArrayReference(int i, RefTargetHandle rtarg) {assert(0);}

		//Local methods
		CoreExport int LinearIndex(int row, int col);

public:
		CoreExport RefArrayBase() : ShapeObject() {}

		//From Animatable
		CoreExport void * GetInterface(ULONG id)
		{
			if (id == I_REFARRAY)
				return static_cast<IRefArray *>(this);
			else
				return GeomObject::GetInterface(id);
		}

		//From ReferenceMaker
		CoreExport virtual	int NumRefs();
		CoreExport virtual RefTargetHandle GetReference(int i);
		CoreExport virtual void SetReference(int i, RefTargetHandle rtarg);

		//Referencemaker overloads
		CoreExport virtual RefTargetHandle GetReference(int row, int col);
		CoreExport virtual void SetReference(int row, int col, RefTargetHandle rtarg);
		CoreExport virtual RefResult MakeRefByID(Interval refInterval, int row, int col, RefTargetHandle rtarg);
		CoreExport virtual RefResult ReplaceReference(int row, int col, RefTargetHandle newtarg, BOOL delOld=TRUE);
		CoreExport virtual RefResult DeleteReference(int row, int col);
		CoreExport virtual BOOL CanTransferReference(int row, int col);
		CoreExport virtual bool FindRef(RefTargetHandle rtarg, int& row, int& col);
		CoreExport virtual void CloneRefs(ReferenceMaker *, RemapDir& remap);

};
#endif //_REF_ARRAY_BASE
