/**********************************************************************
 *<
	FILE: IMaterial.h

	DESCRIPTION: Material extension Interface class

	CREATED BY: Nikolai Sander

	HISTORY:

 *>	Copyright (c) 2000, All Rights Reserved.
 **********************************************************************/

#ifndef _MTATERIAL_H_
#define _MTATERIAL_H_

#define IMATERIAL_INTERFACE_ID Interface_ID(0x578773a5, 0x44cc0d7d)

class MtlBase;
class INode;

class IMaterial : public BaseInterface
{
public:
	// We'll do this as soon as Animatable derives from GenericInterfaceServer<BaseInterface>
	virtual void SetMtl(MtlBase *mtl)=0;
	virtual MtlBase *GetMtl()=0;

	virtual void SetINode(INode *node)=0;
	virtual INode *GetINode()=0;
};

#endif