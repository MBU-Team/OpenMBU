/**********************************************************************
 *<
	FILE: namesel.h

	DESCRIPTION:  A named sel set class for sub-object named selections ets

	CREATED BY: Rolf Berteig

	HISTORY: 3/18/96

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef __NAMEDSEL__
#define __NAMEDSEL__

class GenericNamedSelSetList {
	public:
		Tab<TSTR*> names;
		Tab<BitArray*> sets;
		Tab<DWORD> ids;

		CoreExport ~GenericNamedSelSetList();		
		CoreExport BitArray *GetSet(TSTR &name);
		CoreExport BitArray *GetSet(DWORD id);
		CoreExport BitArray *GetSetByIndex(int index);
		int Count() {return sets.Count();}
		CoreExport void AppendSet(BitArray &nset,DWORD id=0,TSTR &name=TSTR(""));
		CoreExport void InsertSet(int pos, BitArray &nset,DWORD id=0,TSTR &name=TSTR(""));
		CoreExport int InsertSet(BitArray &nset,DWORD id=0,TSTR &name=TSTR(""));
		CoreExport BOOL RemoveSet(TSTR &name);
		CoreExport BOOL RemoveSet(DWORD id);
		CoreExport IOResult Load(ILoad *iload);
		CoreExport IOResult Save(ISave *isave);
		CoreExport void SetSize(int size);
		CoreExport GenericNamedSelSetList& operator=(GenericNamedSelSetList& from);
		CoreExport void DeleteSetElements(BitArray &set,int m=1);
		CoreExport void DeleteSet(int i);
		CoreExport BOOL RenameSet(TSTR &oldName, TSTR &newName);
		CoreExport void Alphabetize ();	// Bubble Sort!
		BitArray &operator[](int i) {return *sets[i];}
	};

// TH: These methods are implemented in core\namesel.cpp.
// This class is used by the edit mesh and edit patch modifiers.

#endif

