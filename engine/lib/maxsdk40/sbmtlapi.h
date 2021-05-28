/**********************************************************************
 *<
	FILE:			sbmtlapi.h

	DESCRIPTION:	Object Sub Material API

	CREATED BY:		Christer Janson

	HISTORY:		1-19-98

 *>	Copyright (c) 1998, All Rights Reserved.
 **********************************************************************/

// By deriving your object or modifier from this class you will support
// direct assignment of sub materials to selected faces/elements.
// For a reference implementation of this class, please refer to
// maxsdk\samples\pack1\triobjed.cpp

#ifndef __SBMTLAPI_H__
#define __SBMTLAPI_H__

class ISubMtlAPI {
public:
	// Return a material ID that is currently not used by the object.
	// If the current face selection share once single MtlID that is not
	// used by any other faces, you should use it.
	virtual MtlID	GetNextAvailMtlID(ModContext* mc) = 0;
	// Indicate if you are active in the modifier panel and have an 
	// active face selection
	virtual BOOL	HasFaceSelection(ModContext* mc) = 0;
	// Set the selected faces to the specified material ID.
	// If bResetUnsel is TRUE, then you should set the remaining
	// faces material ID's to 0
	virtual void	SetSelFaceMtlID(ModContext* mc, MtlID id, BOOL bResetUnsel = FALSE) = 0;
	// Return the material ID of the selected face(s).
	// If multiple faces are selected they should all have the same MtlID -
	// otherwise you should return -1.
	// If faces other than the selected share the same material ID, then 
	// you should return -1.
	virtual int		GetSelFaceUniqueMtlID(ModContext* mc) = 0;
	// Return the material ID of the selected face(s).
	// If multiple faces are selected they should all have the same MtlID,
	// otherwise you should return -1.
	virtual int		GetSelFaceAnyMtlID(ModContext* mc) = 0;
	// Return the highest MtlID on the object.
	virtual	int		GetMaxMtlID(ModContext* mc) = 0;
};

#endif //__SBMTLAPI_H__

