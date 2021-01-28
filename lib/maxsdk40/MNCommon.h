// Common macros for all MNMath.

#ifndef __MN_COMMON_H__
#define __MN_COMMON_H__

#ifndef MNEPS
#define MNEPS 1e-04f
#endif

// Selection levels for internal use:
#define MN_SEL_DEFAULT 0	// Default = use whatever's in mesh.
#define MN_SEL_OBJECT 1
#define MN_SEL_VERTEX 2
#define MN_SEL_EDGE 3
#define MN_SEL_FACE 4

// Got sick of redoing the following everywhere:
class FlagUser {
	DWORD FlagUserFlags;
public:
	FlagUser () { FlagUserFlags=0; }
	void SetFlag (DWORD fl, bool val=TRUE) { if (val) FlagUserFlags |= fl; else FlagUserFlags -= (FlagUserFlags & fl); }
	void ClearFlag (DWORD fl) { FlagUserFlags -= (FlagUserFlags & fl); }
	bool GetFlag (DWORD fl) const { return (FlagUserFlags & fl) ? 1 : 0; }
	void ClearAllFlags () { FlagUserFlags = 0; }

	void CopyFlags (DWORD fl) { FlagUserFlags = fl; }
	void CopyFlags (const FlagUser & fu) { FlagUserFlags = fu.FlagUserFlags; }
	void CopyFlags (const FlagUser * fu) { FlagUserFlags = fu->FlagUserFlags; }

	void CopyFlags (DWORD fl, DWORD mask) { FlagUserFlags |= (fl & mask); }
	void CopyFlags (const FlagUser &fu, DWORD mask) { FlagUserFlags |= (fu.FlagUserFlags & mask); }
	void CopyFlags (const FlagUser *fu, DWORD mask) { FlagUserFlags |= (fu->FlagUserFlags & mask); }

	void OrFlags (const FlagUser & fu) { FlagUserFlags |= fu.FlagUserFlags; }
	void OrFlags (const FlagUser * fu) { FlagUserFlags |= fu->FlagUserFlags; }

	void AndFlags (const FlagUser & fu) { FlagUserFlags &= fu.FlagUserFlags; }
	void AndFlags (const FlagUser * fu) { FlagUserFlags &= fu->FlagUserFlags; }

	bool FlagMatch (DWORD fmask, DWORD fl) const {
		return ((FlagUserFlags & fmask) == (fl & fmask));
	}
	bool FlagMatch (DWORD fmask, const FlagUser & fu) const {
		return ((FlagUserFlags & fmask) == (fu.FlagUserFlags & fmask));
	}
	bool FlagMatch (DWORD fmask, const FlagUser * fu) const {
		return ((FlagUserFlags & fmask) == (fu->FlagUserFlags & fmask));
	}

	DWORD ExportFlags () const { return FlagUserFlags; }
	void ImportFlags (DWORD fl) { FlagUserFlags = fl; }

	IOResult WriteFlags (ISave *isave, ULONG *nb) const {
		return isave->Write(&FlagUserFlags, sizeof(DWORD), nb);
	}
	IOResult ReadFlags (ILoad *iload, ULONG *nb) {
		return iload->Read (&FlagUserFlags, sizeof(DWORD), nb);
	}
};

#endif	// __MN_COMMON_H__
