/**********************************************************************
 *<
	FILE:			ChkMtlAPI.h

	DESCRIPTION:	Enhances material handling for particles API

	CREATED BY:		Eric Peterson

	HISTORY:		3-8-99

 *>	Copyright (c) 2000 Discreet, All Rights Reserved.
 *********************************************************************/

// This header defines interface methods used to support indirect material
// referencing and enhanced face/material associations.  Generally, these
// methods will need to be implemented by the plugin using them and cannot
// be called from a standard library, since the information required is
// intimately associated with the geometry of the 

#ifndef __CHKMTLAPI_H__
#define __CHKMTLAPI_H__

class IChkMtlAPI
{
	public:
		// SupportsParticleIDbyFace returns TRUE if the object can associate
		// a particle number with a face number, and FALSE by default.
		virtual BOOL SupportsParticleIDbyFace() { return FALSE; }

		// GetParticleFromFace returns the particle to which the face identified
		// by faceID belongs.
		virtual int  GetParticleFromFace(int faceID) { return 0; }

		// SupportsIndirMtlRefs returns TRUE if the object can return a material
		// pointer given a face being rendered, and FALSE if the object will be
		// associated for that render pass with only the object applied to the 
		// node,
		virtual BOOL SupportsIndirMtlRefs() { return FALSE; }

		// If SupportsIndirMtlRefs returns TRUE, then the following methods are meaningful.

		// NumberOfMtlsUsed returns the number of different materials used on the object
		// This number is used in enumerating the different materials via GetNthMtl and getNthMaxMtlID.
		virtual int NumberOfMtlsUsed() { return 0; }

		// Returns the different materials used on the object.
		virtual Mtl *GetNthMtl(int n) { return NULL; }

		// Returns the maximum material ID number used with each of the materials on the object
		virtual int  GetNthMaxMtlID(int n) { return 0; }

		// Get MaterialFromFace returns a pointer to the material associated
		// with the face identified by faceID.
		virtual Mtl *GetMaterialFromFace(int faceID) { return NULL; }
};

#endif //__CHKMTLAPI_H__

