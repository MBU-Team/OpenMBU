/**********************************************************************
 *<
	FILE:  notetrck.h

	DESCRIPTION:  Note track plug-in class

	CREATED BY: Rolf Berteig

	HISTORY: created July 20, 1995

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/


#ifndef __NOTETRCK_H__
#define __NOTETRCK_H__

class NoteTrack : public ReferenceTarget {
	public:
		SClass_ID SuperClassID() {return SClass_ID(NOTETRACK_CLASS_ID);}
		RefResult AutoDelete() {return REF_SUCCEED;}
	};

class NoteAnimProperty : public AnimProperty {
	public:		
		NoteTrack *note;
		DWORD ID() {return PROPID_NOTETRACK;}

		NoteAnimProperty(NoteTrack *n=NULL) {note = n;}
		~NoteAnimProperty() {if (note) note->DeleteMe();}
	};


CoreExport NoteTrack *NewDefaultNoteTrack();




// Standard note track plug-in class definitions:

// Note Key Flags
#define NOTEKEY_SELECTED	(1<<0)
#define NOTEKEY_LOCKED		(1<<1)
#define NOTEKEY_FLAGGED		(1<<2)

class NoteKey {
	public:
		TimeValue time;
		TSTR note;
		DWORD flags;
		
		NoteKey(TimeValue t,const TSTR &n,DWORD f=0) {time=t;note=n;flags=f;}
		NoteKey(NoteKey& n) {time=n.time;note=n.note;flags=n.flags;}

		void SetFlag(DWORD mask) { flags|=(mask); }
		void ClearFlag(DWORD mask) { flags &= ~(mask); }
		BOOL TestFlag(DWORD mask) { return(flags&(mask)?1:0); }
	};

class NoteKeyTab : public Tab<NoteKey*> {
	public:
		~NoteKeyTab() {Clear();}
		CoreExport void Clear();
		void DelKey(int i) {delete (*this)[i]; Delete(i,1);}
		CoreExport NoteKeyTab &operator=(NoteKeyTab &keys);
		CoreExport void KeysChanged();
	};

class NoteKeyClipObject : public TrackClipObject {
	public:
		NoteKeyTab tab;

		Class_ID ClassID() {return Class_ID(NOTETRACK_CLASS_ID,0);}
		SClass_ID SuperClassID() { return NOTETRACK_CLASS_ID; }
		void DeleteThis() {delete this;}

		NoteKeyClipObject(Interval iv) : TrackClipObject(iv) {}
	};

class DefNoteTrack : public NoteTrack {
	public:
		NoteKeyTab keys;
		
		DefNoteTrack() {}
		DefNoteTrack(DefNoteTrack &n) {keys=n.keys;}
		DefNoteTrack& operator=(DefNoteTrack &track) {keys=track.keys;return *this;}
		CoreExport void HoldTrack();

		Class_ID ClassID() {return Class_ID(NOTETRACK_CLASS_ID,0);}

		// Tree view methods from animatable
		int NumKeys() {return keys.Count();}
		TimeValue GetKeyTime(int index) {return keys[index]->time;}
		CoreExport void MapKeys(TimeMap *map,DWORD flags );
		CoreExport void DeleteKeys( DWORD flags );
		CoreExport void CloneSelectedKeys(BOOL offset);		
		CoreExport void DeleteTime( Interval iv, DWORD flags );
		CoreExport void ReverseTime( Interval iv, DWORD flags );
		CoreExport void ScaleTime( Interval iv, float s);
		CoreExport void InsertTime( TimeValue ins, TimeValue amount );
		CoreExport void AddNewKey(TimeValue t,DWORD flags);
		CoreExport int GetSelKeyCoords(TimeValue &t, float &val,DWORD flags);
		CoreExport void SetSelKeyCoords(TimeValue t, float val,DWORD flags);
		CoreExport int GetTrackVSpace( int lineHeight ) {return 1;}
		CoreExport BOOL CanCopyTrack(Interval iv,DWORD flags) {return 1;}
		CoreExport BOOL CanPasteTrack(TrackClipObject *cobj,Interval iv,DWORD flags) {return cobj->ClassID()==ClassID();}
		CoreExport TrackClipObject *CopyTrack(Interval iv,DWORD flags);
		CoreExport void PasteTrack(TrackClipObject *cobj,Interval iv,DWORD flags);
		CoreExport Interval GetTimeRange(DWORD flags) ;		
		CoreExport int HitTestTrack(TrackHitTab& hits,Rect& rcHit,Rect& rcTrack,float zoom,int scroll,DWORD flags );
		CoreExport int PaintTrack(ParamDimensionBase *dim,HDC hdc,Rect& rcTrack,Rect& rcPaint,float zoom,int scroll,DWORD flags );
		CoreExport void SelectKeys( TrackHitTab& sel, DWORD flags );
		CoreExport void SelectKeyByIndex(int i,BOOL sel);
		CoreExport int NumSelKeys();
		CoreExport void FlagKey(TrackHitRecord hit);
		CoreExport int GetFlagKeyIndex();		
		CoreExport BOOL IsAnimated() {return TRUE;}
		CoreExport void EditTrackParams(TimeValue t,ParamDimensionBase *dim,TCHAR *pname,HWND hParent,IObjParam *ip,DWORD flags);
		CoreExport int TrackParamsType() {return TRACKPARAMS_KEY;}
		CoreExport BOOL SupportTimeOperations() {return TRUE;}

		CoreExport IOResult Save(ISave *isave);
		CoreExport IOResult Load(ILoad *iload);

		void DeleteThis() {delete this;}
		RefResult NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget,
	         PartID& partID, RefMessage message) {return REF_SUCCEED;}
		RefTargetHandle Clone(RemapDir &remap);
	};


#endif // __NOTETRCK_H__

