/**********************************************************************
 *<
	FILE: IComponent.h

	DESCRIPTION:  An interface representing the composite pattern.

	CREATED BY: John Hutchinson

	HISTORY: created 9/1/98

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/
#ifndef _ICOMP
#define _ICOMP

/* 
The composite pattern relies on a common interface for leaves and composites.
This class embodies that commonality. Classes which realize this interface 
will generally be of two forms: composites and leaves. A composite will 
implement its methods by iterating over its components and making a recursive
call. A leaf will actually do something. 

*/
class ComponentIterator;
class INode;
class Object;

class IComponent {
public:
	virtual void DisposeTemporary() = 0;
	virtual void Remove(ComponentIterator& c) = 0;
	virtual void Add(TimeValue t, Object* obj, TSTR objname, Matrix3& oppTm, Matrix3& boolTm, Matrix3& parTM, Control* opCont = NULL) = 0;
	virtual void Add(TimeValue t, INode *node, Matrix3& boolTm, bool delnode = true) = 0;

	virtual ComponentIterator * MakeIterator() /*const*/ = 0;
	virtual ComponentIterator * MakeReverseIterator() /*const*/ = 0;

	virtual Object* MakeLeaf(Object* o) = 0;
	virtual bool IsLeaf() const = 0;

	virtual void SelectOp(int which, BOOL selected) = 0;
	virtual void ClearSelection() = 0;
	virtual int GetSubSelectionCount() = 0;
};


class ComponentIterator {
public:
	virtual void First() = 0;
	virtual void Next() = 0;
	virtual bool IsDone() /*const*/ = 0; //Should be const but we want to call nonconst methods from within
	virtual int SubObjIndex() = 0;
	virtual IComponent* GetComponent(void** ipp = NULL, ULONG iid = I_BASEOBJECT) const = 0;
	//Note: this object may be dynamically allocated. Calls must 
	//be matched with calls to DisposeTemporary()

	virtual Object* GetComponentObject() const = 0;
	virtual INode* GetComponentINode(INode *node) const = 0;
	virtual void DisposeComponentINode(INode *node) const = 0;
	virtual Control* GetComponentControl() const = 0;//This need not be released
	virtual bool Selected() const = 0;//is the corresponding component selected?
	virtual bool Hidden() const = 0;//is the corresponding component hidden?
	virtual void DeleteThis() = 0;
	
	//JH 4/12/99 Adding some methods for better display control
	virtual bool WantsDisplay(DWORD flags) = 0;
	virtual Point3 DisplayColor(DWORD flags, INode* n = NULL, GraphicsWindow* gw = NULL) = 0;
protected:
	virtual bool ValidIndex(int which) const = 0;
};

class CompositeClassDesc: public ClassDesc
{
public:
	virtual int				NumValenceTypes(){return 0;}
	virtual Class_ID		GetValenceClassID(int which) {return Class_ID(0,0);}
};

#endif //_ICOMP
