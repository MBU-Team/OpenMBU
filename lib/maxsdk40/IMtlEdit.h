/**********************************************************************
 *<
	FILE: IMtlEdit.h

	DESCRIPTION: Material Editor Interface

	CREATED BY: Nikolai Sander

	HISTORY: Created 6/22/00

 *>	Copyright (c) 2000, All Rights Reserved.
 **********************************************************************/

class MtlBase;

class IMtlEditInterface : public FPStaticInterface 
{
public:
// function IDs 
	enum { 
		   fnIdGetCurMtl,
		   fnIdSetActiveMtlSlot,
		   fnIdGetActiveMtlSlot,
		   fnIdPutMtlToMtlEditor,
		   fnIdGetTopMtlSlot,
		   fnIdOkMtlForScene,
		   fnIdUpdateMtlEditorBrackets,
	};

	virtual MtlBase *GetCurMtl() = 0;
	virtual void SetActiveMtlSlot(int i, BOOL forceUpdate = FALSE)=0;
	virtual int GetActiveMtlSlot()=0;
	virtual void PutMtlToMtlEditor(MtlBase *mtlBase, int slot)=0;
	virtual MtlBase* GetTopMtlSlot(int slot)=0;
	virtual BOOL OkMtlForScene(MtlBase *m)=0;
	virtual void UpdateMtlEditorBrackets()=0;

};


#define MTLEDIT_INTERFACE Interface_ID(0x2c7b3f6e, 0x16fb35d4)
inline IMtlEditInterface* GetMtlEditInterface () { return (IMtlEditInterface *)GetCOREInterface(MTLEDIT_INTERFACE); }
