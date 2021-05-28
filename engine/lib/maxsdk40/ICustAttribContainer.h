/**********************************************************************
 *<
	FILE:  ICustAttribContainer.h

	DESCRIPTION:  Defines ICustAttribContainer class

	CREATED BY: Nikolai Sander

	HISTORY: created 5/22/00

 *>	Copyright (c) 2000, All Rights Reserved.
 **********************************************************************/

#ifndef _ICUSTATTRIBCONTAINER_H_
#define _ICUSTATTRIBCONTAINER_H_

class CustAttrib;

class ICustAttribContainer : public ReferenceTarget
{
public:
	virtual int GetNumCustAttribs()=0;
	virtual CustAttrib *GetCustAttrib(int i)=0;
	virtual void AppendCustAttrib(CustAttrib *attribute)=0;
	virtual void SetCustAttrib(int i, CustAttrib *attribute)=0;
	virtual void InsertCustAttrib(int i, CustAttrib *attribute)=0;
	virtual void RemoveCustAttrib(int i)=0;
	virtual ParamDlg* CreateParamDlg(HWND hwMtlEdit, IMtlParams *imp)=0;
	virtual void CopyParametersFrom(ReferenceMaker *from, RemapDir &remap)=0;
	virtual Animatable *GetOwner()=0;
	virtual void DeleteThis()=0;
};

#endif