/**********************************************************************
 *<
	FILE: patchcap.h

	DESCRIPTION:  Patch Capping header file

	CREATED BY: Tom Hudson

	HISTORY: created 21 December, 1995

 *>	Copyright (c) 1995, All Rights Reserved.
 **********************************************************************/

#ifndef __PATCHCAP_H__

#define __PATCHCAP_H__

// This uses the linked-list class templates
#include "linklist.h"

class PatchSeg {
	public:
		int vert;
		int vec1;
		int vec2;
		PatchSeg() {}
		PatchSeg(int v, int v1, int v2) { vert=v; vec1=v1; vec2=v2; }
	};

MakeLinkedList(PatchSeg);	// Creates PatchSegList
MakeLinkedList(PatchSegList);	// Creates PatchSegListList

MakeLinkedList(Patch);		// Creates PatchList
MakeLinkedList(PatchVert);	// Creates PatchVertList
MakeLinkedList(PatchVec);	// Creates PatchVecList

class PatchCapWork {
	public:
		PatchSegListList plist;
		PatchVertList newVerts;
		PatchVecList newVecs;
		PatchList newPatches;
		CoreExport void Dump(char *title = NULL);
	};

#endif // __PATCHCAP_H__
