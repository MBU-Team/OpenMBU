/**********************************************************************
 *<
	FILE: hierclas.h

	DESCRIPTION: Simple utility class for describing hierarchies

	CREATED BY: Tom Hudson

	HISTORY: Created 3 July 1995

 *>	Copyright (c) 1995, All Rights Reserved.
 **********************************************************************/

#ifndef __HIERCLAS__H
#define __HIERCLAS__H

#define INVALID_HIERARCHY -1

class HierarchyEntry {
	public:
		int data;
		int children;
		HierarchyEntry *parent;
		HierarchyEntry *sibling;
		HierarchyEntry *child;
		TSTR sortKey;
		UtilExport HierarchyEntry();
		UtilExport HierarchyEntry(int d, HierarchyEntry *p, HierarchyEntry *s);
		UtilExport int HierarchyLevel();
		UtilExport void AddChild(int d);
		UtilExport int GetChild(int index);
		int Children() { return children; }
		UtilExport void Sort();
	};

class GenericHierarchy {
	private:
		HierarchyEntry root;
		void FreeTree(HierarchyEntry* start = NULL);
		BOOL isSorted;
		void CopyTree(int parent, HierarchyEntry* ptr);
	public:
		GenericHierarchy() { root = HierarchyEntry(-1,NULL,NULL); isSorted = FALSE; }
		UtilExport ~GenericHierarchy();
		UtilExport void AddEntry(int data, int parent = -1);		// Add one entry, given its parent
		UtilExport int Entries();									// Total number of members in the hierarchy
		UtilExport HierarchyEntry* GetStart() { return root.child; } // Get the first item under the root
		UtilExport HierarchyEntry* FindEntry(int data, HierarchyEntry* start = NULL);
		UtilExport int NumberOfChildren(int data);					// The number of children for this item
		UtilExport int GetChild(int data, int index);				// Get the nth child of this item
		UtilExport void New();										// Clear out the hierarchy tree
		UtilExport void Sort();										// Sort tree by children/siblings
		UtilExport BOOL IsCompatible(GenericHierarchy& hier);		// Are they compatible?
		UtilExport void Dump(HierarchyEntry* start = NULL);			// DebugPrint the tree
		UtilExport GenericHierarchy& operator=(GenericHierarchy& from);	// Copy operator
		UtilExport TSTR& SortKey();									// Get the sort key for the hierarchy
	};

#endif __HIERCLAS__H
