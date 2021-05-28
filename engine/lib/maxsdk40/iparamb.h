/**********************************************************************
 *<
	FILE: iparamb.h

	DESCRIPTION: Interface to Parameter blocks

	CREATED BY: Rolf Berteig

	HISTORY: created 1/25/95

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef __IPARAMB__
#define __IPARAMB__

class UserType : public ReferenceTarget {
	public:		
		virtual ~UserType() {};
		virtual Control* CreateController()=0;
		virtual BOOL operator==( const UserType &t )=0;
		virtual UserType& operator=( const UserType &t )=0;		
	};


// Built in data types
#include "paramtype.h"

// Chunk IDs for loading/saving
#define PB_COUNT_CHUNK			0x0001
#define PB_PARAM_CHUNK			0x0002
#define PB_INDEX_CHUNK			0x0003
#define PB_ANIMATABLE_CHUNK		0x0004
#define PB_VERSION_CHUNK		0x0005
#define PB_FLOAT_CHUNK			(TYPE_FLOAT + 0x100)
#define PB_INT_CHUNK			(TYPE_INT + 0x100)
#define PB_RGBA_CHUNK			(TYPE_RGBA + 0x100)
#define PB_POINT3_CHUNK			(TYPE_POINT3 + 0x100)
#define PB_BOOL_CHUNK			(TYPE_BOOL + 0x100)

#define PB_TYPE_CHUNK			0x0200
#define PB_TYPE_FLOAT_CHUNK		(PB_TYPE_CHUNK + TYPE_FLOAT)
#define PB_TYPE_INT_CHUNK		(PB_TYPE_CHUNK + TYPE_INT)
#define PB_TYPE_RGBA_CHUNK		(PB_TYPE_CHUNK + TYPE_RGBA)
#define PB_TYPE_POINT3_CHUNK	(PB_TYPE_CHUNK + TYPE_POINT3)
#define PB_TYPE_BOOL_CHUNK		(PB_TYPE_CHUNK + TYPE_BOOL)
#define PB_TYPE_USER_CHUNK		(PB_TYPE_CHUNK + TYPE_USER)


// When a client of a param block receives the REFMSG_GET_PARAM_NAME
// message, the partID field is set to point at one of these structures.
// The client should fill in the parameter name.
class GetParamName {
	public:
		TSTR name;
		int index;
		GetParamName(TSTR n,int i) { name=n;index=i; }
	};

// When a client of a param block receives the REFMSG_GET_PARAM_DIM
// message, the partID field is set to point at one of these structs.
// The client should set dim to point at it's dim descriptor.
class GetParamDim {
	public:
		ParamDimension *dim;
		int index;
		GetParamDim(int i) {index=i;dim=NULL;}
	};


// To create a parameter block, pass an array of these descriptors
// into the Create function. 
// Items in the parameter block can be refered to by index. The 
// index is derived from the order in which the descriptors appear
// in the array. If a parameter is a UserType, then a pointer to a 
// new UserType must be passed in. The parameter block will be responsible
// for deleting it when it is done with it.

class ParamBlockDesc {
	public:
		ParamType type;
		UserType *user;	
		BOOL animatable;
	};

// This version of the descriptor has an ID for each parameter.
class ParamBlockDescID {
	public:
		ParamType type;
		UserType *user;	
		BOOL animatable;
		DWORD id;
	};

class IParamBlock;

// This class represents a virtual array of parameters.
// Parameter blocks are one such implementation of this class, but
// it can also be useful to implement a class that abstracts non-
// parameter block variables. 
//
// The ParamMap class (see IParamM.h) uses this base class so that
// a ParamMap can be used to control UI for not only parameter blocks
// but variables stored outside of parameter blocks.
class IParamArray {
	public:
		virtual BOOL SetValue( int i, TimeValue t, float v ) {return FALSE;}
		virtual BOOL SetValue( int i, TimeValue t, int v ) {return FALSE;}
		virtual BOOL SetValue( int i, TimeValue t, Point3& v ) {return FALSE;}
		
		virtual BOOL GetValue( int i, TimeValue t, float &v, Interval &ivalid ) {return FALSE;}
		virtual BOOL GetValue( int i, TimeValue t, int &v, Interval &ivalid ) {return FALSE;}
		virtual BOOL GetValue( int i, TimeValue t, Point3 &v, Interval &ivalid ) {return FALSE;}
	
		// If it is a param block, this will get a pointer to it, otherwise it will return NULL.
		// Note that casting won't work because of multiple iheritance.
		virtual IParamBlock *GetParamBlock() {return NULL;}

		// Checks to see if a keyframe exists for the given parameter at the given time
		virtual BOOL KeyFrameAtTime(int i, TimeValue t) {return FALSE;}
	};
		 
class IParamBlock : 			
			public ReferenceTarget,
			public IParamArray {
	public:
		// Get's the super class of a parameters controller
		virtual SClass_ID GetAnimParamControlType(int anim)=0;

		// Get the param type
		virtual ParamType GetParameterType(int i)=0;

		// one for each known type
		virtual BOOL SetValue( int i, TimeValue t, float v )=0;
		virtual BOOL SetValue( int i, TimeValue t, int v )=0;		
		virtual BOOL SetValue( int i, TimeValue t, Point3& v )=0;		
		virtual BOOL SetValue( int i, TimeValue t, Color& v )=0; // uses Point3 controller
		
		// one for each known type
		virtual BOOL GetValue( int i, TimeValue t, float &v, Interval &ivalid )=0;
		virtual BOOL GetValue( int i, TimeValue t, int &v, Interval &ivalid )=0;
		virtual BOOL GetValue( int i, TimeValue t, Point3 &v, Interval &ivalid )=0;
		virtual BOOL GetValue( int i, TimeValue t, Color &v, Interval &ivalid )=0; // uses Point3 Controller

		virtual Color  GetColor(int i, TimeValue t=0)=0;										
		virtual Point3 GetPoint3(int i, TimeValue t=0)=0;										
		virtual int    GetInt(int i, TimeValue t=0)=0;										
		virtual float  GetFloat(int i, TimeValue t=0)=0;										

		virtual DWORD GetVersion()=0;
		virtual int NumParams()=0;

		virtual void RemoveController(int i)=0;
		virtual Control* GetController(int i)=0;
		virtual void SetController(int i, Control *c, BOOL preserveFrame0Value=TRUE)=0;
		virtual	void SwapControllers(int j, int k )=0;

		// Given the parameter index, what is the refNum?
		virtual	int GetRefNum(int paramNum)=0;

		// Given the parameter index what is the animNum?
		virtual	int GetAnimNum(int paramNum)=0;

		// Given the animNum what is the parameter index?
		virtual	int AnimNumToParamNum(int animNum)=0;

		// Inherited from IParamArray
		IParamBlock *GetParamBlock() {return this;}
				
		// This is only for use in a RescaleWorldUnits() implementation:
		// The param block implementation of RescaleWorldUnits scales only tracks
		// that have dimension type = stdWorldDim. If letting the param block handle 
		// the rescaling is not sufficient, call this on just the parameters you need to rescale.
		virtual void RescaleParam(int paramNum, float f)=0;

		// When a NotifyRefChanged is received from a param block, you 
		// can call this method to find out which parameter generated the notify.
		virtual int LastNotifyParamNum()=0;
		};


CoreExport IParamBlock *CreateParameterBlock(ParamBlockDesc *pdesc, int count);

// Note: version must fit into 16 bits. (5/20/97)
CoreExport IParamBlock *CreateParameterBlock(ParamBlockDescID *pdesc, int count, DWORD version);

// This creates a new parameter block, based on an existing parameter block of
// a later version. The new parameter block inherits any parameters from
// the old parameter block whose parameter IDs match.
CoreExport IParamBlock *UpdateParameterBlock(
	ParamBlockDescID *pdescOld, int oldCount, IParamBlock *oldPB,
	ParamBlockDescID *pdescNew, int newCount, DWORD newVersion);


// ----------------------------------------------------------
// A handy post load call back for fixing up parameter blocks.

// This structure describes a version of the parameter block.
class ParamVersionDesc {
	public:
		ParamBlockDescID *desc;
		int count;
		DWORD version;
		ParamVersionDesc(ParamBlockDescID *d,int c,int v) {desc=d;count=c;version=v;}
	};

// This will look up the version of the loaded callback and 
// fix it up so it matches the current version.
// NOTE: this thing deletes itself when it's done.
class ParamBlockPLCB : public PostLoadCallback {
	public:
		ParamVersionDesc *versions;
		int count;
		ParamVersionDesc *cur;		
		ReferenceTarget *targ;
		int pbRefNum;

		ParamBlockPLCB(
			ParamVersionDesc *v,int cnt,ParamVersionDesc *c,
			ReferenceTarget *t,int refNum)
			{versions=v;count=cnt;cur=c;targ=t;pbRefNum=refNum;}
		CoreExport void proc(ILoad *iload);
		int Priority() { return 0; }
		CoreExport INT_PTR Execute(int cmd, ULONG_PTR arg1=0, ULONG_PTR arg2=0, ULONG_PTR arg3=0);
	};


#endif


