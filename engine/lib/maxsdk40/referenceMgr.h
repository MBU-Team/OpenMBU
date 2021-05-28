/* -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

   FILE: referenceMgr.h

	 DESCRIPTION: manages references to any maxsdk ReferenceTarget

	 CREATED BY: michael malone (mjm)

	 HISTORY: created February 01, 2000

   	 Copyright (c) 2000, All Rights Reserved

// -----------------------------------------------------------------------------
// -------------------------------------------------------------------------- */
#ifndef __REFERENCE_MANAGER__H
#define __REFERENCE_MANAGER__H

// max headers
#include <max.h>


class RefTarget
{
protected:
	RefTargetHandle mRef;
	Interval mValid;
	bool mHeld;

public:
	RefTarget() : mRef(NULL), mHeld(false) { }
	RefTarget(RefTargetHandle ref, const Interval& valid = NEVER) : mRef(ref), mHeld(false) { mValid = valid; }
	RefTarget(const RefTarget& refTarget) : mRef(refTarget.mRef), mHeld(false) { mValid = refTarget.mValid; }
	virtual RefTarget& operator=(const RefTarget& refTarget) { mRef = refTarget.mRef; mValid = refTarget.mValid; return *this; }

	RefTargetHandle GetRefTargetHandle() const { return mRef; }
	void SetRefTargetHandle(RefTargetHandle ref) { mRef = ref; }
	const Interval& GetValidity() const { return mValid; }
	void SetValidity(const Interval& valid) { mValid = valid; }
	bool IsHeld() const { return mHeld; }
	void SetHeld(bool held) { mHeld = held; }
}; 

template <class T> class RefMgr;

// template type must be class RefTarget (or derived from it)
// We handle undo and redo special in the RefMgr class. This is because
// We need to save the object that we are storing along with the
// reference. We override DeleteReference to add a special undo
// record to the undo stack, and we also add an undo record at the
template <class T> class RefMgrAddDeleteRestore : public RestoreObj, public ReferenceMaker
{
	RefMgr<T>*			mMgr;			// The RefMgr that we are restoring
	T*					mRefTarg;		// The data that needs to be restored
										// This is NULL if the reference needs removing
	RefTargetHandle		mRef;			// The ReferenceTarget we are restoring

public:
	RefMgrAddDeleteRestore(RefMgr<T>* mgr, int which, RefTargetHandle ref)
		: mMgr(NULL), mRefTarg(which < 0 ? NULL : mgr->GetRefTarget(which)), mRef(ref)
	{
		MakeRefByID(FOREVER, 0, mgr);
		if (mRefTarg != NULL)
			mRefTarg->SetHeld(true);
	}
		
	~RefMgrAddDeleteRestore()
	{
		if (mRefTarg != NULL) {
			if (mMgr != NULL)
				mMgr->RemoveRef(mRefTarg);
			delete mRefTarg;
		}
		DeleteAllRefs();
	}

	// Restore handles both undo and redo. It swaps mRefTarg with any
	// reference already in the RefMgr.
	void Restore(int isUndo)
	{
		if (mMgr != NULL) {
			if (mRefTarg == NULL) {
				int i = mMgr->FindRefTargetHandleIndex(mRef);
				if (i >= 0) {
					mRefTarg = mMgr->mRefTargetPtrs[i];
					mRefTarg->SetHeld(true);
					mMgr->mRefTargetPtrs.Delete(i, 1);
				}
			} else {
				mMgr->RemoveRef(mRefTarg);

				mRefTarg->SetRefTargetHandle(mRef);
				mMgr->mRefTargetPtrs.Append(1, &mRefTarg, 3);

				mRefTarg->SetHeld(false);
				mRefTarg = NULL;
			}
		}
	}
				
	void Redo() { Restore(false); }
	TSTR Description() { return _T("RefMgrAddDeleteRestore"); }

	virtual RefResult NotifyRefChanged(
		Interval			changeInt,
		RefTargetHandle		hTarget,
		PartID&				partID,
		RefMessage			message
	)
	{
		switch (message) {
		case REFMSG_TARGET_DELETED:
			mMgr = NULL;
			break;
		}

		return REF_SUCCEED;
	}
	virtual int NumRefs() { return 1; }
	virtual RefTargetHandle GetReference(int i) { return i == 0 ? mMgr : NULL; }
	virtual void SetReference(int i, RefTargetHandle rtarg)
	{
		if (i == 0)
			mMgr = static_cast<RefMgr<T>*>(rtarg);
	}
};


// template type must be class RefTarget (or derived from it)
template <class T> class RefMgr : public ReferenceTarget
{
	friend class RefMgrAddDeleteRestore<T>;

protected:
	typedef RefResult (* TNotifyCB)(void *cbParam, Interval changeInt, T* pRefTarget, RefTargetHandle hTarger, PartID& partID, RefMessage message);

	Tab<T*> mRefTargetPtrs;
	TNotifyCB mNotifyCB;
	void *mNotifyCBParam;

public:
	RefMgr() : mNotifyCB(NULL), mNotifyCBParam(NULL) { }

	virtual ~RefMgr() { DeleteAllRefs(); }

	virtual void DeleteThis() { delete this; }

	virtual void Init(TNotifyCB notifyCB=NULL, void *notifyCBParam=NULL)
	{
		mRefTargetPtrs.ZeroCount();
		mNotifyCB = notifyCB;
		mNotifyCBParam = notifyCBParam;
	}

	virtual int Count() { return mRefTargetPtrs.Count(); }

	virtual int FindRefTargetHandleIndex(RefTargetHandle rtarg)
	{
		int n = mRefTargetPtrs.Count();
		for (int i=0; i<n; i++)
		{
	//		if (mRefTargetPtrs[i] == rtarg)
			if (mRefTargetPtrs[i]->GetRefTargetHandle() == rtarg)
				return i;
		}
		return -1;
	}

	virtual int FindRefTargetIndex(T *pRefTarget)
	{
		int n = mRefTargetPtrs.Count();
		for (int i=0; i<n; i++)
		{
	//		if (mRefTargetPtrs[i] == rtarg)
			if (mRefTargetPtrs[i]->GetRefTargetHandle() == pRefTarget->GetRefTargetHandle())
				return i;
		}
		return -1;
	}

	virtual T* FindRefTarget(RefTargetHandle rtarg)
	{
		int index = FindRefTargetHandleIndex(rtarg);
		return (index == -1) ? NULL : mRefTargetPtrs[index];
	}

/*
	virtual T* FindRefTarget(const T& refTarget)
	{
		int index = FindRefTargetIndex(refTarget);
		return (index == -1) ? NULL : &mRefTargetPtrs[index];
	}
*/

	virtual T* GetRefTarget(int i)
	{
		if ( i < mRefTargetPtrs.Count() )
			return mRefTargetPtrs[i];
		else
			return NULL;
	}

	virtual bool AddUndo(int which, RefTargetHandle ref)
	{
		bool undo = false;
		if (!theHold.RestoreOrRedoing()) {
			int resumeCount = 0;

			while (theHold.IsSuspended()) {
				theHold.Resume();
				++resumeCount;
			}

			undo = theHold.Holding() != 0;
			if (undo)
				theHold.Put(new RefMgrAddDeleteRestore<T>(this, which, ref));

			while (--resumeCount >= 0)
				theHold.Suspend();
		}
		return undo;
	}

//	virtual void AddRef(RefTargetHandle rtarg)
	virtual bool AddRef(T* pRefTarget)
	{
		// make sure rtarg is valid and hasn't already been referenced
//		if ( (rtarg != NULL) && (FindRefIndex(rtarg) == -1) )
		if ( (pRefTarget->GetRefTargetHandle() != NULL) && (FindRefTargetIndex(pRefTarget) == -1) )
		{
//			if (rtarg->MakeReference(FOREVER, this) != REF_SUCCEED)
			if ( pRefTarget->GetRefTargetHandle()->MakeReference(FOREVER, this) != REF_SUCCEED )
				return false;
	//		mRefTargetPtrs.Append(1, &rtarg, 3);
//			mRefTargetPtrs.Append(1, &T(rtarg), 3);
			NotifyDependents(FOREVER, 0, REFMSG_SUBANIM_STRUCTURE_CHANGED);
			mRefTargetPtrs.Append(1, &pRefTarget, 3);

			// Handle undo if we need to
			AddUndo(-1, pRefTarget->GetRefTargetHandle());
			return true;
		}
		else
			return false;
	}

// mjm - begin - 06.20.00
	virtual bool RemoveRef(RefTargetHandle rtarg)
	{
		return RemoveRef( FindRefTargetHandleIndex(rtarg) );
/*
		if (i >= 0)
		{
			DbgAssert( mRefTargetPtrs[i].GetRefTargetHandle() == rtarg );
			DeleteReference(i);
			delete mRefTargetPtrs[i];
			mRefTargetPtrs.Delete(i, 1);
		}
*/
	}

	virtual bool RemoveRef(T* pRefTarget)
	{
		return RemoveRef( FindRefTargetIndex(pRefTarget) );
/*
		if (i >= 0)
		{
			DbgAssert( mRefTargetPtrs[i] == pRefTarget );
			DeleteReference(i);
			delete pRefTarget;
			mRefTargetPtrs.Delete(i, 1);
		}
*/
	}

	virtual bool RemoveRef(int index)
	{
		if ( (index >= 0) && ( index < Count() ) )
		{
			DeleteReference(index);
			T* targ = mRefTargetPtrs[index];
			if (targ != NULL && !targ->IsHeld())
				delete targ;
			mRefTargetPtrs.Delete(index, 1);
			NotifyDependents(FOREVER, 0, REFMSG_SUBANIM_STRUCTURE_CHANGED);
			return true;
		}
		return false;
	}
// mjm - end

	virtual void Clear()
	{
		DeleteAllRefsFromMe();
		for ( int i = mRefTargetPtrs.Count(); --i >= 0; ) {
			T* targ = mRefTargetPtrs[i];
			if (targ != NULL && !targ->IsHeld())
				delete targ;
		}
		mRefTargetPtrs.ZeroCount();
		NotifyDependents(FOREVER, 0, REFMSG_SUBANIM_STRUCTURE_CHANGED);
	}


	// from class ReferenceMaker
	virtual int NumRefs() { return Count(); }

	// In order to make undo work we need to keep an undo record
	// for the T* object we keep with the reference.
	virtual RefResult DeleteReference(int which) {
		RefTargetHandle ref = GetReference(which);
		RefResult ret = ReferenceTarget::DeleteReference(which);
		if (ret == REF_SUCCEED) {
			AddUndo(which, ref);
		}
		return ret;
	}

	virtual ReferenceTarget* GetReference(int i)
	{
		if ( i < mRefTargetPtrs.Count() )
	//		return mRefTargetPtrs[i];
			return mRefTargetPtrs[i]->GetRefTargetHandle();
		else
			return NULL;
	}

	virtual void SetReference(int i, RefTargetHandle rtarg)
	{
		// Ignore SetReference during redo or undo, this is because the
		// RefMgrAddDeleteRestore will fix up everything.
		if (!theHold.RestoreOrRedoing()) {
			int count = mRefTargetPtrs.Count();
			assert(i < count || rtarg == NULL);
			if ( i < count )
		//		mRefTargetPtrs[i] = rtarg;
				mRefTargetPtrs[i]->SetRefTargetHandle(rtarg);
		}
	}

	virtual RefResult NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget, PartID& partID,  RefMessage message)
	{
		RefResult refResult(REF_SUCCEED);
		if (mNotifyCB)
		{
			T* pRefTarget = FindRefTarget(hTarget);
			refResult = mNotifyCB(mNotifyCBParam, changeInt, pRefTarget, hTarget, partID,  message);
		}

		switch (message)
		{
			case REFMSG_TARGET_DELETED:
			{
// mjm - begin - 6.20.00
//				DeleteRefTargetPtr(hTarget);
				int i = FindRefTargetHandleIndex(hTarget);
				assert(i >= 0);
				if (i >= 0)
				{
					// We need to handle undo
					if (!AddUndo(i, hTarget))
						delete mRefTargetPtrs[i];
					mRefTargetPtrs.Delete(i, 1);
				}
// mjm - end
				break;
			}
		}
		return refResult;
	}
};

#endif
