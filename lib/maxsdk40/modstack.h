/**********************************************************************
 *<
	FILE: modstack.h

	DESCRIPTION:

	CREATED BY: Rolf Berteig

	HISTORY: created January 20, 1996

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef __MODSTACK__
#define __MODSTACK__


// These are the class IDs for object space derived objects and
// world space derived objects
extern CoreExport Class_ID derivObjClassID;
extern CoreExport Class_ID WSMDerivObjClassID;


class IDerivedObject : public Object {
	public:
		// Adds a modifier to the derived object.
		// before = 0				:Place modifier at the end of the pipeline (top of stack)
		// before = NumModifiers()	:Place modifier at the start of the pipeline (bottom of stack)
		virtual void AddModifier(Modifier *mod, ModContext *mc=NULL, int before=0)=0;				
		virtual void DeleteModifier(int index=0)=0;
		virtual int NumModifiers()=0;		

		// Searches down the pipeline for the base object (an object that is not a
		// derived object). May step into other derived objects. 
		// This function has been moved up to Object, with a default implementation
		// that just returns "this".  It is still implemented by derived objects and
		// WSM's to search down the pipeline.  This allows you to just call it on
		// a Nodes ObjectRef without checking for type.
//		virtual Object *FindBaseObject()=0;
		
		// Get and set the object that this derived object reference.
		// This is the next object down in the stack and may be the base object.
		virtual Object *GetObjRef()=0;
		virtual RefResult ReferenceObject(Object *pob)=0;

		// Access the ith modifier.
		virtual Modifier *GetModifier(int index)=0;

		// Replaces the ith modifier in the stack
		virtual void SetModifier(int index, Modifier *mod)=0;

		// Access the mod context for the ith modifier
		virtual ModContext* GetModContext(int index)=0;
		
		virtual ObjectState Eval(TimeValue t, int modIndex = 0)=0;

		// pass any notifies onto my ObjRef, such as node attaches/deletes - jbw 9.9.00
		void NotifyTarget(int msg, RefMakerHandle rm) { if (GetObjRef()) GetObjRef()->NotifyTarget(msg, rm); }	
	};

// Create a world space or object space derived object.
// If the given object pointer is non-NULL then the derived
// object will be set up to reference that object.
CoreExport IDerivedObject *CreateWSDerivedObject(Object *pob=NULL);
CoreExport IDerivedObject *CreateDerivedObject(Object *pob=NULL);

enum PipeEnumResult {
	PIPE_ENUM_CONTINUE,
	PIPE_ENUM_STOP
};

// This is the callback procedure for pipeline enumeration. The ReferenceTarget 
// passed to the proc can be a Node, Modifier or Object. In case it is a Modifier
// derObj contains the DerivedObject and the index is the index of this modifier 
// in the DerivedObject. In all other cases derObj is NULL and index is 0;
// In case the flag includeEmptyDOs is declared as true, the proc will be called
// even for DerivedObjects, that don't contain any modifiers. In that case the
// object pointer will be NULL, the derObj pointer will contain the DerivedObject
// and the index will be -1.

class GeomPipelineEnumProc : public InterfaceServer
{
public:	
	virtual PipeEnumResult proc(ReferenceTarget *object, IDerivedObject *derObj, int index)=0;
};

//---------------------------------------------------------------------------
// Pipeline Enumeration

// These methods start a pipeline enumeration down the pipeline towards the baseobject
// and over the baseobjects' branches in case it is a compound object.
// A pipleine enumeration can be started from a Node, an Object or from a Modifier. 
// In case it is started from a Modifier, the client has to provide the IDerviedObject the 
// Modifier is applied to and the index of the Modifier in the IDerivedObject. One can use 
// the method Modifier::GetIDerivedObject(); in order to get the IDerviedObject 
// and the index, given a modifier and a ModContext.

CoreExport int EnumGeomPipeline(GeomPipelineEnumProc *gpep, INode *start, bool includeEmptyDOs = false);
CoreExport int EnumGeomPipeline(GeomPipelineEnumProc *gpep, Object *start, bool includeEmptyDOs = false);
CoreExport int EnumGeomPipeline(GeomPipelineEnumProc *gpep, IDerivedObject *start, int modIndex = 0, bool includeEmptyDOs = false);

//---------------------------------------------------------------------------
// Collapse Notification 

// Whenever the modifier stack is collapsed the code has to notify all objects in the 
// stack with a Pre and a Post notification. In order to do this, the 
// NotifyCollapseEnumProc can be used in conjunction with the method 
// EnumGeomPipleine(). In the constructor one can specify, if it is a pre-, 
// or post-collapse notification. In case it is a postcollapse the object that 
// represents the result of the collapse has to be provided as well. The
// INode pointer to the beginning of the pipeline that was collapsed has to be 
// provided in both cases.

class NotifyCollapseEnumProc : public GeomPipelineEnumProc
{
bool bPreCollapse;
INode *node;
Object *collapsedObject;
public:	
	NotifyCollapseEnumProc(bool preCollapse, INode *n, Object *collapsedObj = NULL) : bPreCollapse(preCollapse), node(n), collapsedObject(collapsedObj) {}
	virtual PipeEnumResult proc(ReferenceTarget *object,IDerivedObject *derObj, int index) {
		if(object->ClassID() == Class_ID(BASENODE_CLASS_ID,0))
			return PIPE_ENUM_CONTINUE;

		if(bPreCollapse)
			((BaseObject *) object)->NotifyPreCollapse(node, derObj, index);
		else
			((BaseObject *) object)->NotifyPostCollapse(node, collapsedObject, derObj, index);

		return PIPE_ENUM_CONTINUE;
	}

};

#endif //__MODSTACK__
