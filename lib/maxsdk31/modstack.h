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
	};

// Create a world space or object space derived object.
// If the given object pointer is non-NULL then the derived
// object will be set up to reference that object.
CoreExport IDerivedObject *CreateWSDerivedObject(Object *pob=NULL);
CoreExport IDerivedObject *CreateDerivedObject(Object *pob=NULL);


#endif //__MODSTACK__
