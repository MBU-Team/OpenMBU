/**********************************************************************
 *<
	FILE:  CustAttrib.h

	DESCRIPTION:  Defines CustAttrib class

	CREATED BY: Nikolai Sander

	HISTORY: created 5/25/00

 *>	Copyright (c) 2000, All Rights Reserved.
 **********************************************************************/

#ifndef _ICUSTATTRIB_H_
#define _ICUSTATTRIB_H_

class ICustAttribContainer;

class CustAttrib: public ReferenceTarget
{
public:
	virtual TCHAR* GetName(){ return "Custom Attribute";}
	virtual ParamDlg *CreateParamDlg(HWND hwMtlEdit, IMtlParams *imp){return NULL;}
	virtual bool CheckCopyAttribTo(ICustAttribContainer *to) { return true; }
};

#endif