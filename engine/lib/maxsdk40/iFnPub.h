/**********************************************************************
 *<
	FILE: iFnPub.h

	DESCRIPTION: Interface to Plugin Function Publishing system

	CREATED BY: John Wainwright

	HISTORY: created 2/15/00

 *>	Copyright (c) 2000 Autodesk, All Rights Reserved.
 **********************************************************************/

#ifndef __IFNPUB__
#define __IFNPUB__

#include "baseInterface.h"

// classes defined in this header
class FPInterface;
	class FPInterfaceDesc;
		class FPStaticInterface;
	class FPMixinInterface;

class FPFunctionDef;
class FPActionDef;
class FPParamDef;
class FPPropDef;
typedef int FPStatus;
class FPParams;
class FPParamOptions;
class FPValidator;
class FPEnum;
class FPValue;
class IObject;
class MAXException;
class FPMacroEmitter;

typedef	short FunctionID;	// ID for individual functions within an interface (local)
#define FP_NO_FUNCTION ((FunctionID)-1)	// special ID indicating no function bound
typedef short EnumID;
#define FP_NO_ENUM ((EnumID)-1)

// built-in interface IDs
#define NULLINTERFACE_ID	Interface_ID(0, 0)  // null interface
// BASEINTERFACE_ID (1,1) in baseInterface.h
#define FPINTERFACE_ID		Interface_ID(0, 2)  // root FPInterface
#define FPMIXININTERFACE_ID Interface_ID(0, 3)  // root FPMixinInterface
#define FPINTERFACEDESC_ID	Interface_ID(0, 4)  // root FPInterfaceDesc

// standard interface accessors
inline FPInterface*		 GetFPInterface(BaseInterface* bi)     { return reinterpret_cast<FPInterface*>(bi->GetInterface(FPINTERFACE_ID)); }
inline FPInterfaceDesc*  GetFPInterfaceDesc(BaseInterface* bi) { return reinterpret_cast<FPInterfaceDesc*>(bi->GetInterface(FPINTERFACEDESC_ID)); }
inline FPMixinInterface* GetFPMixinInterface(BaseInterface* bi){ return reinterpret_cast<FPMixinInterface*>(bi->GetInterface(FPMIXININTERFACE_ID)); }

// external classes
class ClassDesc;
class MSPluginClass;
class Rollout;
class MaxIcon;
class ActionTable;
class Point3;
class TSTR;
class PBBitmap;
class Mtl;
class Texmap;
class INode;
class ReferenceTarget;
class Matrix3;
class AngAxis;
class Quat;
class Ray;
class Point2;
class BitArray;
class ClassDesc;
class Mesh;
class Object;
class Control;
class Interval;
class Color;
class Value;

typedef INT_PTR StringResID;
typedef INT_PTR ResID;
typedef short MapID;

// FPStatus values
#define FPS_FAIL				0
#define FPS_NO_SUCH_FUNCTION	-1
#define FPS_ACTION_DISABLED		-2
#define FPS_OK					1

// FPInterface class, the base class for FnPub interfaces.  
//   contains the basic dispatching code & virtual base methods
//	 The prime subclass is FPInterfaceDesc, which contains the 
//	 interface metadata in a singleton instance.  There are two
//   typedefs, FPStaticInterface used as the base class for static and core 
//   interfaces, and FPMixinInterface used as the
//   base class for mixin (object-based) interfaces

class FPInterface : public BaseInterface
{
protected:
	virtual FPStatus _dispatch_fn(FunctionID fid, TimeValue t, FPValue& result, FPParams* p) { return FPS_NO_SUCH_FUNCTION; }
public:
	static CoreExport FPInterfaceDesc nullInterface;			//  null interface descriptor, can be returned by interfaces not publishing metadata

	// from BaseInterface
	virtual BaseInterface*			GetInterface(Interface_ID id) 
									{ 
										if (id == FPINTERFACE_ID) return this; 
										else if (id == FPINTERFACEDESC_ID) return (BaseInterface*)GetDesc();
										else return BaseInterface::GetInterface(id); 
									}
	
	// metadata access
	virtual FPInterfaceDesc*		GetDesc() = 0;

	// Fn calling
	virtual CoreExport FPStatus		Invoke(FunctionID fid, TimeValue t=0, FPParams* params=NULL);
	virtual inline FPStatus			Invoke(FunctionID fid, FPParams* params) { return Invoke(fid, 0, params); }
	virtual CoreExport FPStatus		Invoke(FunctionID fid, TimeValue t, FPValue& result, FPParams* params=NULL);
	virtual inline FPStatus			Invoke(FunctionID fid, FPValue& result, FPParams* params=NULL) { return Invoke(fid, 0, result, params); }
	virtual CoreExport FunctionID	FindFn(TCHAR* name);

	// predicate access
	virtual CoreExport BOOL			IsEnabled(FunctionID actionID);
	virtual CoreExport BOOL			IsChecked(FunctionID actionID);
	virtual CoreExport BOOL			IsVisible(FunctionID actionID);
	virtual CoreExport FunctionID	GetIsEnabled(FunctionID actionID);
	virtual CoreExport FunctionID	GetIsChecked(FunctionID actionID);
	virtual CoreExport FunctionID	GetIsVisible(FunctionID actionID);

	// Action interface access & control
	virtual ActionTable*			GetActionTable() { return NULL; }
	// global Actions enable/disable
	virtual void					EnableActions(BOOL onOff) { }
};

#include <paramtype.h>

// FPInterfaceDesc class, an FPInterface that contains the metadata for an interface, a distinhished
//						  singleton instance per interface kept in a Tab<> in ClassDesc
//						  This class is subclassed typically by static and core inerfaces and instantiated
//						  by mixins to provide their metadata
class FPInterfaceDesc : public FPInterface 
{
protected:
	CoreExport void	load_descriptor(Interface_ID id, TCHAR* int_name, StringResID descr, ClassDesc* cd, ULONG flag, va_list ap);

public:
	// interface metadata 
	Interface_ID	ID;				// my unique ID
	TSTR			internal_name;	// fixed, internal name
	StringResID		description;	// description string resource
	ClassDesc*		cd;				// publishing plugin's ClassDesc
	USHORT			flags;			// flag bits
	Tab<FPFunctionDef*> functions;	// descriptors for individual functions in this interface
	Tab<FPPropDef*> props;			//    "         "      "      properties in this interface
	Tab<FPEnum*>	enumerations;	// any symbolic enums for the interface
	// scripted plug-in stuff if this belongs to a scripted plug-in class
	MSPluginClass*	pc;				// the scripted class if non-NULL (gc-protected by the scripted plugin class)
	Rollout*		rollout;		// rollout if specified (gc-protected by the scripted plugin class)
	// Action-interface stuff
	ActionTable*	action_table;	// the table published for this action interface

	// constructors
	CoreExport FPInterfaceDesc() { }
	CoreExport FPInterfaceDesc(Interface_ID id, TCHAR* int_name, StringResID descr, ClassDesc* cd, ULONG flag, ...);
	CoreExport ~FPInterfaceDesc();
	virtual void Init() { }

	// from GenericInterface, default lifetime is noRelease
	LifetimeType			LifetimeControl() { return noRelease; }

	// from BaseInterface
	virtual BaseInterface*	GetInterface(Interface_ID id) { if (id == FPINTERFACEDESC_ID) return this; else return FPInterface::GetInterface(id); }
	
	// construction utilities
	CoreExport void			LoadDescriptor(Interface_ID id, TCHAR* int_name, StringResID descr, ClassDesc* cd, ULONG flag, ...);
	CoreExport void			SetClassDesc(ClassDesc* i_cd);
	CoreExport va_list		check_fn(va_list ap, int id);
	CoreExport va_list		scan_fn(va_list ap, int id, int index);
	CoreExport va_list		check_prop(va_list ap, int id);
	CoreExport va_list		scan_prop(va_list ap, int id, int index);
	CoreExport va_list		check_enum(va_list ap, EnumID id);
	CoreExport va_list		scan_enum(va_list ap, EnumID id, int index);

	// metadata access
	FPInterfaceDesc*		GetDesc() { return this; }
	Interface_ID			GetID() { return ID; }
	CoreExport FPFunctionDef* GetFnDef(FunctionID fid);
	ActionTable*			GetActionTable() { return action_table; }

	// global Actions enable/disable
	CoreExport void			EnableActions(BOOL onOff);

	// overridable HInstance and resource access from owning module
	CoreExport virtual HINSTANCE HInstance();
	CoreExport virtual TCHAR* GetRsrcString(StringResID id);
};

// FPInterfaceDesc flag bits
// per interface flags
#define FP_ACTIONS			0x0001	// marks this as an Action Interface, holding only UI modal, zero param action fns
#define FP_MIXIN			0x0002	// marks this as a Mixin Interface, it is implemented directly by the plugin class, so the 
									// methods in it are virtual on the plugin's objects
#define FP_CORE				0x0004	// marks this as a MAX Core Interface, available through GetCOREInterface(Interface_ID) 
#define FP_STATIC_METHODS	0x0008	// this interface is used as a static method interface in MAXScript, properties are not directly callable
#define FP_TEST_INTERFACE	0x0010	// a non-public test interface, Discreet internal use only 
	
// per descriptor internal flags
#define	FP_SCRIPTED_CLASS	0x1000	// belongs to a scripted plug-in class
#define	FP_TEMPORARY		0x2000	// temporary descripter built during scene load to support schema migration

// static interface
class FPStaticInterface : public FPInterfaceDesc
{
};

// mixin interface
class FPMixinInterface : public FPInterface
{
public:
	// from GenericInterface, default lifetime is immediateRelease
	virtual LifetimeType	LifetimeControl() { return immediateRelease; }

	// from BaseInterface
	virtual BaseInterface*	GetInterface(Interface_ID id) { if (id == FPMIXININTERFACE_ID) return this; else return FPInterface::GetInterface(id); }
	
	// utility metadata accessors...
	// use GetDescByID() to directly implement FPInterface::GetDesc() in your public virtual base mixin class, 
	//   like this:     FPInterfacedesc* GetDesc() { return GetDescByID(THIS_INTERFACE_ID); }
	//   then implement a GetDescByID() in the implementing class to avoid link export issues
	virtual FPInterfaceDesc* GetDescByID(Interface_ID id) { return &nullInterface; }

	// override GetID() in those interfaces that don't publish metadata but have a unique Interface_ID for quick internal identification 
	//   in implementation code that might be shared by a bunch of mixin interfaces
	virtual Interface_ID	 GetID() { return GetDesc()->ID; }
};

// FPFunctionDef, contains descriptor for each published function
//				  live in Tab<> in FPInterface
class FPFunctionDef
{
public:

	DWORD			cbStruct;			// size of the struct
	FunctionID		ID;				// interface-local ID, used to identify fn in calls
	TSTR			internal_name;	// fixed, internal name
	StringResID		description;	// description string resource
	USHORT			flags;			// flag bits
	ParamType2		result_type;	// type of value returned by fn
	EnumID			enumID;			// ID of symbolic enumeration in owning interface if any
	FPActionDef*	action_def;		// extra metadata if function in an Action interface
	Tab<FPParamDef*> params;		// descriptors for parameters to this fn 
	SHORT			keyparam_count; // count of keyword-parameters (with f_keyArgDefault's)

	FPFunctionDef() {cbStruct = sizeof(FPFunctionDef); flags = 0; description = 0; action_def = NULL; enumID = FP_NO_ENUM; keyparam_count = 0; }
	CoreExport ~FPFunctionDef();
};

class FPPropDef
{
public:
	DWORD			cbStruct;		// size of the struct
	FunctionID		getter_ID;		// interface-local ID for getter method
	FunctionID		setter_ID;		// interface-local ID for setter method
	TSTR			internal_name;	// fixed, internal name
	StringResID		description;	// description string resource
	USHORT			flags;			// flag bits
	ParamType2		prop_type;		// property type
	EnumID			enumID;			// ID of symbolic enumeration in owning interface if any
	FPParamDef*		setter_param;	// present if non-NULL, used for setter param options

	FPPropDef() { cbStruct = sizeof(FPPropDef); flags = 0; description = 0; getter_ID = setter_ID = FPS_NO_SUCH_FUNCTION; enumID = FP_NO_ENUM; setter_param = NULL; }
	CoreExport ~FPPropDef();
};

// function def flag bits	
#define FP_NO_REDRAW		  0x0001	// do not flag need for viewport redraw when function is invoked, MAXScript defaults to flag redraw
#define FP_VAR_ARGS			  0x0002	// variable number of args, pass args directly in a FPParams instance
#define FP_CLIENT_OWNS_RESULT 0x0004	// client owns lifetime of pointer-based results, should delete when finished using
// internal function bits
#define FP_ACTION			  0x0100	// indicates an action function
#define FP_HAS_UI			  0x0200	// action has UI specifiec
#define FP_ICONRES			  0x0400	// icon via res ID
#define FP_ICONFILE			  0x0800	// icon via bmp file + index
#define FP_HAS_SHORTCUT		  0x1000	// has default KB shortct
#define FP_HAS_KEYARGS		  0x2000	// fn has some optional keyword args defined

// FPActionDef,  contains extra descriptor info for function if fn is in an Action interface 

class FPActionDef
{
public:
	DWORD		cbStruct;			// size of the struct
	TSTR		internal_cat;		// fixed, internal category name
	StringResID category;			// localizable category resID
	FunctionID	isEnabled_id;		// interface function IDs for the isEnabled predicate for this action
	FunctionID	isChecked_id;		//   "  " for isChecked predicate
	FunctionID	isVisible_id;		//   "  " for isVisible predicate
	ResID		icon_resID;			// icon as resource ID
	TSTR		icon_file;			// icon as UI .bmp filename, index pair, as per CUI icon specifications...
	short		icon_index;	
	MaxIcon*	icon;
	StringResID	button_text;		// button text string resID, defaults to function description
	StringResID	tool_tip;			// tooltip string resID, defaults to function description
	StringResID	menu_text;			// menu item text string resID, defaults to buttonText or function description
	ControlType2 ctrl_type;			// type of UI control, if f_ui specified
	ResID		ctrl_pbID;			// control's host parammap pblock ID
	MapID		ctrl_mapID;			// control's host parammap map ID within the block
	int			ctrl_id;			// control dialog item ID
	COLORREF	ctrl_hiCol;			// highlight colorif check button
	ACCEL		shortcut;			// default keyboard shortcut
	FPMacroEmitter* macro_emitter;	// if non-NULL, callback object to emit macros to macroRecorder

	CoreExport FPActionDef();
	CoreExport ~FPActionDef();
};

// fn def option tags for Action functions & parameters
enum {
	// Action options
	f_category = -(1<<30),			// category name, as internal TCHAR* and localizable string resID, defaults to interface name
	f_predicates,					// supply 3 functionIDs for isEnabled, isChecked, isVisible predicates 
	f_isEnabled,					// isEnabled predicate functionID
	f_isChecked,					// isChecked predicate functionID
	f_isVisible,					// isVisible predicate functionID
	f_iconRes,						// icon as resource ID
	f_icon,							// icon as UI .bmp filename, index pair, as per CUI icon specifications
	f_buttonText,					// button text string resID, defaults to function description
	f_toolTip,						// tooltip string resID, defaults to function description
	f_menuText,						// menu item text string resID, defaults to buttonText or function description
	f_ui,							// UI spec if paramMap2-implemented UI (pmap blockID, mapID, control type, button or checkbutton resID, hilight col if chkbtn)
	f_shortCut,						// default keyboard short cut, as pair: virt ACCEL flags word, keycode  (first two items in Win32 ACCEL struct)
	f_macroEmitter,					// provide callback object to handle macro emmission
	// param options
	f_range,						// valid range, two type-specific vals
	f_validator,					// validator object, FPValidator*
	f_inOut,						// in, out flags FPP_IN_PARM, FPP_OUT_PARAM or both, defaults to both
	f_keyArgDefault,				// marks this as an optional keyArg param and gives default value which must me of type to match param type
	f_index,						// no args, of present indicates values used as indexes, client can map own origin to always 0-origin internally
};

// FPParamDef, contains descriptor for each published function
//			   live in Tab<> in FPInterface

// per-param flags
#define FPP_HAS_RANGE		0x0001
#define FPP_HAS_VALIDATOR	0x0002
#define FPP_IN_PARAM		0x0004  // in-out flags used by _BR ref types to decide when to pass in source values or hand back returns
#define FPP_OUT_PARAM		0x0008  //   " "
#define FPP_IN_OUT_PARAM	0x000C  //   "  "  both
#define FPP_KEYARG			0x0010  // if p_keyArgDefault supplied, client canuse keyword args if supported for this param
#define FPP_INDEX			0x0020  // parameter values used as indexes, always 0-origin internally, allows client to map to other origins

class FPParamDef
{
public:
	DWORD			cbStruct;			// size of the struct
	TSTR			internal_name;
	StringResID		description;
	ParamType2		type;
	EnumID			enumID;			// ID of symbolic enumeration in owning interface if any
	USHORT			flags;
	FPParamOptions*	options;		// present if non-NULL

	FPParamDef() : cbStruct(sizeof(FPParamDef)), description(0), options(NULL), enumID(FP_NO_ENUM), flags(FPP_IN_OUT_PARAM) { }
	CoreExport ~FPParamDef() ;
};

// FPParams,  contains a Tab<> of FPValue's being the actual parameters for an FP Fn call
//			  at present, FP Fn arguments are positional. We could expand this to allow 
//			  optional, order-independent keyword params

class FPParams
{
public:
	Tab<FPValue>	params;

			  FPParams() { }
	CoreExport FPParams(int count, ...);
	CoreExport ~FPParams();

	CoreExport void Load(int count, ...);
};

// symbolic enums for an interface, 
//   used by metadata clients to support symbolic value for TYPE_ENUM types (ints)

class FPEnum
{
public:
	EnumID	ID;			// ID for this enumeration
	typedef struct 
	{
		TCHAR*	name;	// enum symbolic name
		int		code;	// equivalent int code
	} enum_code;
	Tab<enum_code> enumeration; 
};

// FPValue, a variant structure containing a single value, passable as a FP Fn parameter or result
class PBBitmap;
class Texmap;
class Value;

class FPValue
{
public:
	ParamType2	type;
	union 
	{
		int					i;
		float				f;
		DWORD				d;
		bool				b;
		int*				iptr;
		float*				fptr;
		Point3*				p;
		TimeValue			t;
		TCHAR*				s;
		TSTR*				tstr;
		PBBitmap*			bm;
		Mtl*				mtl;
		Texmap*				tex;
		INode*				n;
		ReferenceTarget*	r;
		Matrix3*			m;
		AngAxis*			aa;
		Quat*				q;
		Ray*				ray;
		Point2*				p2;
		BitArray*			bits;
		ClassDesc*			cd;
		Mesh*				msh;
		Object*				obj;
		Control*			ctrl;
		Interval*			intvl;
		POINT*				pt;
		HWND				hwnd;
		IObject*			iobj;
		FPInterface*		fpi;
		void*				ptr;
		Color*				clr;
		FPValue*			fpv;
		Value*				v;
		DWORD*				dptr;
		bool*				bptr;

		// Tab<>s of above
		Tab<int>*			i_tab;
		Tab<float>*			f_tab;
		Tab<Point3*>*		p_tab;
		Tab<TimeValue>*		t_tab;
		Tab<TCHAR*>*		s_tab;
		Tab<TSTR*>*			tstr_tab;
		Tab<PBBitmap*>*		bm_tab;
		Tab<Mtl*>*			mtl_tab;
		Tab<Texmap*>*		tex_tab;
		Tab<INode*>*		n_tab;
		Tab<ReferenceTarget*>*	r_tab;
		Tab<Matrix3*>*		m3_tab;
		Tab<AngAxis*>*		aa_tab;
		Tab<Quat*>*			q_tab;
		Tab<Ray*>*			ray_tab;
		Tab<Point2*>*		p2_tab;
		Tab<BitArray*>*		bits_tab;
		Tab<ClassDesc*>*	cd_tab;
		Tab<Mesh*>*			msh_tab;
		Tab<Object*>*		obj_tab;
		Tab<Control*>*		ctrl_tab;
		Tab<Interval*>*		intvl_tab;
		Tab<POINT*>*		pt_tab;
		Tab<HWND>*			hwnd_tab;
		Tab<IObject*>*		iobj_tab;
		Tab<FPInterface*>*	fpi_tab;
		Tab<void*>*			ptr_tab;
		Tab<Color*>*		clr_tab;
		Tab<FPValue*>*		fpv_tab;
		Tab<Value*>*		v_tab;
		Tab<DWORD>*			d_tab;
		Tab<bool*>*			b_tab;
	};

			   FPValue() { Init(); }
			   FPValue(FPValue& from) { Init(); *this = from; }
			   FPValue(int type, ...) { va_list ap; va_start(ap, type); ap = Loadva(type, ap); va_end(ap); }
	CoreExport ~FPValue() { Free(); }
	CoreExport void Free();

	void	  Init() { type = (ParamType2)TYPE_INT; s = NULL; }
	
	CoreExport FPValue& operator=(FPValue& sv);
	CoreExport va_list Loadva(int type, va_list ap, bool ptr=false);
	inline    void Load(int type, ...) { va_list ap; va_start(ap, type); ap = Loadva(type, ap); va_end(ap); }
	inline    void LoadPtr(int type, ...) { va_list ap; va_start(ap, type); ap = Loadva(type, ap, true); va_end(ap); }
};

// optional param-specific descriptor info
class FPParamOptions
{
public:
	DWORD			cbStruct;			// size of the struct
	FPValue			range_low;		// range values if specified
	FPValue			range_high;
	FPValidator*	validator;		// validator if specified
	FPValue			keyarg_default; // default if value is optional keyword arg

	FPParamOptions() : cbStruct(sizeof(FPParamOptions)), validator(NULL) { }
};

// virtual base class for parameter validation objects
class FPValidator : public InterfaceServer 
{
public:
	// validate val for the given param in function in interface
	virtual bool Validate(FPInterface* fpi, FunctionID fid, int paramNum, FPValue& val, TSTR& msg)=0;
};

// virtual base class for action function macroRecorder emitter objects
class FPMacroEmitter
{
public:
	// gen macro for a call to given action fn
	virtual void EmitMacro(FPInterface* fpi, FPFunctionDef* fd)=0;
};

// IObject class, virtual base class for random classes that want to 
//                implement GetInterface().  Similar to IUnknown in COM.
//                would be used to pass interface-based objects not
//                otherwise supported by the FPValue base types.
//				  MAXScript handles these and will use GetInterface() to 
//                publish interface & methods as properties of the IObjects
class IObject : public BaseInterfaceServer
{
public:
	// inherits interface access and iteration from BaseInterfaceServer
	//
	//   virtual BaseInterface* GetInterface(Interface_ID id);
	//   virtual int NumInterfaces();			
	//   virtual BaseInterface* GetInterfaceAt(int i)

	// object/class name
	virtual TCHAR* GetIObjectName() { return _T(""); }							
	// interface enumeration...
	// IObject ref management (can be implemented by dynamically-allocated IObjects for
	//                         ref-count based lifetime control)
	virtual void AcquireIObject() { }
	virtual void ReleaseIObject() { }
	// virtual destructor
	virtual void DeleteIObject() { }
};

// base exception class for FP-based exceptions.  FnPub functions can throw this
//  instances of this class or subclasses to signal error conditions.
class MAXException
{
public:
	TSTR	message;
	int		error_code;

	MAXException(TCHAR* msg, int code=0) : message(msg), error_code(code) { }
};

// publishing DESCRIPTOR & FUNCTION_MAP macros

#define DECLARE_DESCRIPTOR(_interface)					\
	public:												\
	_interface() { }									\
	_interface(Interface_ID id, TCHAR* name,			\
			   StringResID descr, ClassDesc* cd,		\
			   ULONG flags, ...)						\
	{													\
		va_list ap;										\
		va_start(ap, flags);							\
		load_descriptor(id, name, descr, cd, flags, ap); \
		va_end(ap);										\
	}								
					
#define DECLARE_DESCRIPTOR_NDC(_interface)				\
	public:												\
	_interface(Interface_ID id, TCHAR* name,			\
			   StringResID descr, ClassDesc* cd,		\
			   ULONG flags, ...)						\
	{													\
		va_list ap;										\
		va_start(ap, flags);							\
		load_descriptor(id, name, descr, cd, flags, ap); \
		va_end(ap);										\
	}								
					
#define BEGIN_FUNCTION_MAP								\
	public:												\
	FPStatus _dispatch_fn(FunctionID fid, TimeValue t,	\
					FPValue& result, FPParams* p)		\
	{													\
		FPStatus status = FPS_OK;						\
		switch (fid)									\
		{
	
#define END_FUNCTION_MAP								\
			default: status = FPS_NO_SUCH_FUNCTION;		\
		}												\
		return status;									\
	}

#define NO_FUNCTION_MAP									\
	public:												\
	FPStatus _dispatch_fn(FunctionID fid, TimeValue t,	\
					FPValue& result, FPParams* p)		\
	{													\
		return FPS_NO_SUCH_FUNCTION;					\
	}

// ----------- indivudal MAP entry macros ----------

#define FP_FIELD(_type, _v)			(_type##_FIELD(_v))

// Action function

#define FN_ACTION(_fid, _fn)							\
	case _fid:											\
		status = _fn();									\
		break;	

// predicates

#define FN_PRED(_fid, _fn)								\
	case _fid:											\
		result.Load(TYPE_BOOL, _fn());					\
		break;	
#define FN_PREDS(_fid1, _fn1, _fid2, _fn2, _fid3, _fn3) \
	case _fid1:											\
		result.Load(TYPE_BOOL, _fn1());					\
		break;											\
	case _fid2:											\
		result.Load(TYPE_BOOL, _fn2());					\
		break;											\
	case _fid3:											\
		result.Load(TYPE_BOOL, _fn3());					\
		break;	

// property FN_MAP macros
#define PROP_FNS(_getID, _getFn, _setID, _setFn, _ptype) \
	case _getID:										\
		result.LoadPtr(_ptype,	_ptype##_RSLT(			\
			_getFn()));									\
		break;											\
	case _setID:										\
		_setFn(FP_FIELD(_ptype, p->params[0]));			\
		break;	
#define RO_PROP_FN(_getID, _getFn, _ptype)				\
	case _getID:										\
		result.LoadPtr(_ptype,	_ptype##_RSLT(			\
			_getFn()));									\
		break;
#define PROP_TFNS(_getID, _getFn, _setID, _setFn, _ptype) \
	case _getID:										\
		result.LoadPtr(_ptype,	_ptype##_RSLT(		\
			_getFn(t)));	\
		break;											\
	case _setID:										\
		_setFn(FP_FIELD(_ptype, p->params[0]), t);		\
		break;	
#define RO_PROP_TFN(_getID, _getFn, _ptype)				\
	case _getID:										\
		result.LoadPtr(_ptype,	_ptype##_RSLT(		\
			_getFn(t)));	\
		break;

// property FN_MAP macros for the Static Method Interfaces used in MAXScript
#define SM_PROP_FNS(_getID, _getFn, _setID, _setFn, _ptype) \
	case _getID:										\
		result.LoadPtr(_ptype,	_ptype##_RSLT(		\
			_getFn(FP_FIELD(TYPE_FPVALUE_BR, p->params[0])))); \
		break;											\
	case _setID:										\
		_setFn(FP_FIELD(TYPE_FPVALUE_BR, p->params[0]), FP_FIELD(_ptype, p->params[1])); \
		break;	
#define SM_RO_PROP_FN(_getID, _getFn, _ptype)			\
	case _getID:										\
		result.LoadPtr(_ptype,	_ptype##_RSLT(		\
			_getFn(FP_FIELD(TYPE_FPVALUE_BR, p->params[0]))));\
		break;
#define SM_PROP_TFNS(_getID, _getFn, _setID, _setFn, _ptype) \
	case _getID:										\
		result.LoadPtr(_ptype,	_ptype##_RSLT(		\
			_getFn(FP_FIELD(TYPE_FPVALUE_BR, p->params[0]), t)));\
		break;											\
	case _setID:										\
		_setFn(FP_FIELD(TYPE_FPVALUE_BR, p->params[0]), FP_FIELD(_ptype, p->params[1]), t); \
		break;	
#define SM_RO_PROP_TFN(_getID, _getFn, _ptype)			\
	case _getID:										\
		result.LoadPtr(_ptype,	_ptype##_RSLT(		\
			_getFn(FP_FIELD(TYPE_FPVALUE_BR, p->params[0]), t)));	\
		break;

// functions with return value, no time
#define FN_VA(_fid, _rtype, _f)							\
	case _fid:											\
		result.LoadPtr(_rtype,	_rtype##_RSLT(			\
			_f(p)));									\
		break;

#define FN_0(_fid, _rtype, _f)							\
	case _fid:											\
		result.LoadPtr(_rtype,	_rtype##_RSLT(			\
			_f()));										\
		break;

#define FN_1(_fid, _rtype, _f, _p1)						\
	case _fid:											\
		result.LoadPtr(_rtype,	_rtype##_RSLT(			\
			_f(FP_FIELD(_p1, p->params[0]))));			\
		break;	

#define FN_2(_fid, _rtype, _f, _p1, _p2)				\
	case _fid:											\
		result.LoadPtr(_rtype,	_rtype##_RSLT(			\
					_f(FP_FIELD(_p1, p->params[0]),		\
					   FP_FIELD(_p2, p->params[1]))));	\
		break;	
#define FN_3(_fid, _rtype, _f, _p1, _p2, _p3)			\
	case _fid:											\
		result.LoadPtr(_rtype,	_rtype##_RSLT(			\
					_f(FP_FIELD(_p1, p->params[0]),		\
					   FP_FIELD(_p2, p->params[1]),		\
					   FP_FIELD(_p3, p->params[2]))));	\
		break;	
#define FN_4(_fid, _rtype, _f, _p1, _p2, _p3, _p4)		\
	case _fid:											\
		result.LoadPtr(_rtype,	_rtype##_RSLT(			\
					_f(FP_FIELD(_p1, p->params[0]),		\
					   FP_FIELD(_p2, p->params[1]),		\
					   FP_FIELD(_p3, p->params[2]),		\
					   FP_FIELD(_p4, p->params[3]))));	\
		break;	
#define FN_5(_fid, _rtype, _f, _p1, _p2, _p3, _p4, _p5)	\
	case _fid:											\
		result.LoadPtr(_rtype,	_rtype##_RSLT(		\
					_f(FP_FIELD(_p1, p->params[0]),		\
					   FP_FIELD(_p2, p->params[1]),		\
					   FP_FIELD(_p3, p->params[2]),		\
					   FP_FIELD(_p4, p->params[3]),		\
					   FP_FIELD(_p5, p->params[4]))));	\
		break;	

#define FN_6(_fid, _rtype, _f, _p1, _p2, _p3, _p4, _p5, _p6)	\
	case _fid:											\
		result.LoadPtr(_rtype,	_rtype##_RSLT(		\
					_f(FP_FIELD(_p1, p->params[0]),		\
					   FP_FIELD(_p2, p->params[1]),		\
					   FP_FIELD(_p3, p->params[2]),		\
					   FP_FIELD(_p4, p->params[3]),		\
					   FP_FIELD(_p5, p->params[4]),		\
					   FP_FIELD(_p6, p->params[5]))));	\
		break;	

#define FN_7(_fid, _rtype, _f, _p1, _p2, _p3, _p4, _p5, _p6, _p7)	\
	case _fid:											\
		result.LoadPtr(_rtype,	_rtype##_RSLT(		\
					_f(FP_FIELD(_p1, p->params[0]),		\
					   FP_FIELD(_p2, p->params[1]),		\
					   FP_FIELD(_p3, p->params[2]),		\
					   FP_FIELD(_p4, p->params[3]),		\
					   FP_FIELD(_p5, p->params[4]),		\
					   FP_FIELD(_p6, p->params[5]),		\
					   FP_FIELD(_p7, p->params[6]))));	\
		break;	

// void functions, no time
#define VFN_VA(_fid, _f)								\
	case _fid:											\
			_f(p);										\
		break;
#define VFN_0(_fid, _f)									\
	case _fid:											\
			_f();										\
		break;
#define VFN_1(_fid, _f, _p1)							\
	case _fid:											\
			_f(FP_FIELD(_p1, p->params[0]));			\
		break;	
#define VFN_2(_fid, _f, _p1, _p2)						\
	case _fid:											\
			_f(FP_FIELD(_p1, p->params[0]),				\
			   FP_FIELD(_p2, p->params[1]));			\
		break;	
#define VFN_3(_fid, _f, _p1, _p2, _p3)					\
	case _fid:											\
			_f(FP_FIELD(_p1, p->params[0]),				\
					FP_FIELD(_p2, p->params[1]),		\
					FP_FIELD(_p3, p->params[2]));		\
		break;	
#define VFN_4(_fid, _f, _p1, _p2, _p3, _p4)				\
	case _fid:											\
			_f(FP_FIELD(_p1, p->params[0]),				\
					FP_FIELD(_p2, p->params[1]),		\
					FP_FIELD(_p3, p->params[2]),		\
					FP_FIELD(_p4, p->params[3]));		\
		break;	
#define VFN_5(_fid, _f, _p1, _p2, _p3, _p4, _p5)		\
	case _fid:											\
			_f(FP_FIELD(_p1, p->params[0]),				\
					FP_FIELD(_p2, p->params[1]),		\
					FP_FIELD(_p3, p->params[2]),		\
					FP_FIELD(_p4, p->params[3]),		\
					FP_FIELD(_p5, p->params[4]));		\
		break;	
#define VFN_6(_fid, _f, _p1, _p2, _p3, _p4, _p5, _p6)		\
	case _fid:											\
			_f(FP_FIELD(_p1, p->params[0]),				\
					FP_FIELD(_p2, p->params[1]),		\
					FP_FIELD(_p3, p->params[2]),		\
					FP_FIELD(_p4, p->params[3]),		\
					FP_FIELD(_p5, p->params[4]),		\
					FP_FIELD(_p6, p->params[5]));		\
		break;	
#define VFN_7(_fid, _f, _p1, _p2, _p3, _p4, _p5, _p6, _p7)		\
	case _fid:											\
			_f(FP_FIELD(_p1, p->params[0]),				\
					FP_FIELD(_p2, p->params[1]),		\
					FP_FIELD(_p3, p->params[2]),		\
					FP_FIELD(_p4, p->params[3]),		\
					FP_FIELD(_p5, p->params[4]),		\
					FP_FIELD(_p6, p->params[5]),		\
					FP_FIELD(_p7, p->params[6]));		\
		break;	

// functions with const return value, no time
#define CFN_VA(_fid, _rtype, _f)							\
	case _fid:											\
		result.LoadPtr(_rtype,	_rtype##_RSLT(const_cast<_rtype##_TYPE>( \
			_f(p))));	\
		break;

#define CFN_0(_fid, _rtype, _f)							\
	case _fid:											\
		result.LoadPtr(_rtype,	_rtype##_RSLT(const_cast<_rtype##_TYPE>( \
			_f())));	\
		break;

#define CFN_1(_fid, _rtype, _f, _p1)						\
	case _fid:											\
		result.LoadPtr(_rtype,	_rtype##_RSLT(const_cast<_rtype##_TYPE>( \
			_f(FP_FIELD(_p1, p->params[0])))));	\
		break;	

#define CFN_2(_fid, _rtype, _f, _p1, _p2)				\
	case _fid:											\
		result.LoadPtr(_rtype,	_rtype##_RSLT(const_cast<_rtype##_TYPE>( \
					_f(FP_FIELD(_p1, p->params[0]),		\
					   FP_FIELD(_p2, p->params[1])))));	\
		break;	
#define CFN_3(_fid, _rtype, _f, _p1, _p2, _p3)			\
	case _fid:											\
		result.LoadPtr(_rtype,	_rtype##_RSLT(const_cast<_rtype##_TYPE>( \
					_f(FP_FIELD(_p1, p->params[0]),		\
					   FP_FIELD(_p2, p->params[1]),		\
					   FP_FIELD(_p3, p->params[2])))));	\
		break;	
#define CFN_4(_fid, _rtype, _f, _p1, _p2, _p3, _p4)		\
	case _fid:											\
		result.LoadPtr(_rtype,	_rtype##_RSLT(const_cast<_rtype##_TYPE>( \
					_f(FP_FIELD(_p1, p->params[0]),		\
					   FP_FIELD(_p2, p->params[1]),		\
					   FP_FIELD(_p3, p->params[2]),		\
					   FP_FIELD(_p4, p->params[3])))));	\
		break;	
#define CFN_5(_fid, _rtype, _f, _p1, _p2, _p3, _p4, _p5)	\
	case _fid:											\
		result.LoadPtr(_rtype,	_rtype##_RSLT(const_cast<_rtype##_TYPE>( \
					_f(FP_FIELD(_p1, p->params[0]),		\
					   FP_FIELD(_p2, p->params[1]),		\
					   FP_FIELD(_p3, p->params[2]),		\
					   FP_FIELD(_p4, p->params[3]),		\
					   FP_FIELD(_p5, p->params[4])))));	\
		break;	

// value returning functions, with time
#define FNT_VA(_fid, _rtype, _f)						\
	case _fid:											\
		result.LoadPtr(_rtype,	_rtype##_RSLT(			\
			_f(p, t)));	\
		break;
#define FNT_0(_fid, _rtype, _f)							\
	case _fid:											\
		result.LoadPtr(_rtype,	_rtype##_RSLT(			\
			_f(t)));	\
		break;
#define FNT_1(_fid, _rtype, _f, _p1)					\
	case _fid:											\
		result.LoadPtr(_rtype,	_rtype##_RSLT(			\
			_f(FP_FIELD(_p1, p->params[0]),				\
					t)));	\
		break;	
#define FNT_2(_fid, _rtype, _f, _p1, _p2)				\
	case _fid:											\
		result.LoadPtr(_rtype,	_rtype##_RSLT(			\
					_f(FP_FIELD(_p1, p->params[0]),		\
					   FP_FIELD(_p2, p->params[1]),		\
					   t)));	\
		break;	
#define FNT_3(_fid, _rtype, _f, _p1, _p2, _p3)			\
	case _fid:											\
		result.LoadPtr(_rtype,	_rtype##_RSLT(			\
					_f(FP_FIELD(_p1, p->params[0]),		\
					   FP_FIELD(_p2, p->params[1]),		\
					   FP_FIELD(_p3, p->params[2]),		\
					   t)));	\
		break;	
#define FNT_4(_fid, _rtype, _f, _p1, _p2, _p3, _p4)		\
	case _fid:											\
		result.LoadPtr(_rtype,	_rtype##_RSLT(		\
					_f(FP_FIELD(_p1, p->params[0]),		\
					   FP_FIELD(_p2, p->params[1]),		\
					   FP_FIELD(_p3, p->params[2]),		\
					   FP_FIELD(_p4, p->params[3]),		\
					   t)));	\
		break;	
#define FNT_5(_fid, _rtype, _f, _p1, _p2, _p3, _p4, _p5) \
	case _fid:											\
		result.LoadPtr(_rtype,	_rtype##_RSLT(		\
					_f(FP_FIELD(_p1, p->params[0]),		\
					   FP_FIELD(_p2, p->params[1]),		\
					   FP_FIELD(_p3, p->params[2]),		\
					   FP_FIELD(_p4, p->params[3]),		\
					   FP_FIELD(_p5, p->params[4]),		\
					   t)));	\
		break;	
#define FNT_6(_fid, _rtype, _f, _p1, _p2, _p3, _p4, _p5, _p6) \
	case _fid:											\
		result.LoadPtr(_rtype,	_rtype##_RSLT(		\
					_f(FP_FIELD(_p1, p->params[0]),		\
					   FP_FIELD(_p2, p->params[1]),		\
					   FP_FIELD(_p3, p->params[2]),		\
					   FP_FIELD(_p4, p->params[3]),		\
					   FP_FIELD(_p5, p->params[4]),		\
					   FP_FIELD(_p6, p->params[5]),		\
					   t)));	\
		break;	
#define FNT_7(_fid, _rtype, _f, _p1, _p2, _p3, _p4, _p5, _p6, _p7) \
	case _fid:											\
		result.LoadPtr(_rtype,	_rtype##_RSLT(		\
					_f(FP_FIELD(_p1, p->params[0]),		\
					   FP_FIELD(_p2, p->params[1]),		\
					   FP_FIELD(_p3, p->params[2]),		\
					   FP_FIELD(_p4, p->params[3]),		\
					   FP_FIELD(_p5, p->params[4]),		\
					   FP_FIELD(_p6, p->params[5]),		\
					   FP_FIELD(_p7, p->params[6]),		\
					   t)));	\
		break;	

// void functions, with time
#define VFNT_VA(_fid, _f)								\
	case _fid:											\
			_f(p, t);									\
		break;
#define VFNT_0(_fid, _f)								\
	case _fid:											\
			_f(t);										\
		break;
#define VFNT_1(_fid, _f, _p1)							\
	case _fid:											\
			_f(FP_FIELD(_p1, p->params[0]),				\
					t);									\
		break;	
#define VFNT_2(_fid, _f, _p1, _p2)						\
	case _fid:											\
			_f(FP_FIELD(_p1, p->params[0]),				\
			   FP_FIELD(_p2, p->params[1]),				\
			   t);										\
		break;	
#define VFNT_3(_fid, _f, _p1, _p2, _p3)					\
	case _fid:											\
			_f(FP_FIELD(_p1, p->params[0]),				\
					FP_FIELD(_p2, p->params[1]),		\
					FP_FIELD(_p3, p->params[2]),		\
					t);									\
		break;	
#define VFNT_4(_fid, _f, _p1, _p2, _p3, _p4)			\
	case _fid:											\
			_f(FP_FIELD(_p1, p->params[0]),				\
					FP_FIELD(_p2, p->params[1]),		\
					FP_FIELD(_p3, p->params[2]),		\
					FP_FIELD(_p4, p->params[3]),		\
					t);									\
		break;	
#define VFNT_5(_fid, _f, _p1, _p2, _p3, _p4, _p5)		\
	case _fid:											\
			_f(FP_FIELD(_p1, p->params[0]),				\
					FP_FIELD(_p2, p->params[1]),		\
					FP_FIELD(_p3, p->params[2]),		\
					FP_FIELD(_p4, p->params[3]),		\
					FP_FIELD(_p5, p->params[4]),		\
					t);									\
		break;	
#define VFNT_6(_fid, _f, _p1, _p2, _p3, _p4, _p5, _p6)	\
	case _fid:											\
			_f(FP_FIELD(_p1, p->params[0]),				\
					FP_FIELD(_p2, p->params[1]),		\
					FP_FIELD(_p3, p->params[2]),		\
					FP_FIELD(_p4, p->params[3]),		\
					FP_FIELD(_p5, p->params[4]),		\
					FP_FIELD(_p6, p->params[5]),		\
					t);									\
		break;	
#define VFNT_7(_fid, _f, _p1, _p2, _p3, _p4, _p5, _p6, _p7)	\
	case _fid:											\
			_f(FP_FIELD(_p1, p->params[0]),				\
			   FP_FIELD(_p2, p->params[1]),				\
			   FP_FIELD(_p3, p->params[2]),				\
			   FP_FIELD(_p4, p->params[3]),				\
			   FP_FIELD(_p5, p->params[4]),				\
			   FP_FIELD(_p6, p->params[5]),				\
			   FP_FIELD(_p7, p->params[6]),				\
			   t);										\
		break;	
#define VFNT_8(_fid, _f, _p1, _p2, _p3, _p4, _p5, _p6, _p7, _p8)	\
	case _fid:											\
			_f(FP_FIELD(_p1, p->params[0]),				\
			   FP_FIELD(_p2, p->params[1]),				\
			   FP_FIELD(_p3, p->params[2]),				\
			   FP_FIELD(_p4, p->params[3]),				\
			   FP_FIELD(_p5, p->params[4]),				\
			   FP_FIELD(_p6, p->params[5]),				\
			   FP_FIELD(_p7, p->params[6]),				\
			   FP_FIELD(_p8, p->params[7]),				\
			   t);										\
		break;	
#define VFNT_9(_fid, _f, _p1, _p2, _p3, _p4, _p5, _p6, _p7, _p8, _p9)	\
	case _fid:											\
			_f(FP_FIELD(_p1, p->params[0]),				\
			   FP_FIELD(_p2, p->params[1]),				\
			   FP_FIELD(_p3, p->params[2]),				\
			   FP_FIELD(_p4, p->params[3]),				\
			   FP_FIELD(_p5, p->params[4]),				\
			   FP_FIELD(_p6, p->params[5]),				\
			   FP_FIELD(_p7, p->params[6]),				\
			   FP_FIELD(_p8, p->params[7]),				\
			   FP_FIELD(_p9, p->params[8]),				\
			   t);										\
		break;	
#define VFNT_10(_fid, _f, _p1, _p2, _p3, _p4, _p5, _p6, _p7, _p8, _p9, _p10)	\
	case _fid:											\
			_f(FP_FIELD(_p1, p->params[0]),				\
			   FP_FIELD(_p2, p->params[1]),				\
			   FP_FIELD(_p3, p->params[2]),				\
			   FP_FIELD(_p4, p->params[3]),				\
			   FP_FIELD(_p5, p->params[4]),				\
			   FP_FIELD(_p6, p->params[5]),				\
			   FP_FIELD(_p7, p->params[6]),				\
			   FP_FIELD(_p8, p->params[7]),				\
			   FP_FIELD(_p9, p->params[8]),				\
			   FP_FIELD(_p10, p->params[9]),			\
			   t);										\
		break;	
#define VFNT_11(_fid, _f, _p1, _p2, _p3, _p4, _p5, _p6, _p7, _p8, _p9, _p10, _p11)	\
	case _fid:											\
			_f(FP_FIELD(_p1, p->params[0]),				\
			   FP_FIELD(_p2, p->params[1]),				\
			   FP_FIELD(_p3, p->params[2]),				\
			   FP_FIELD(_p4, p->params[3]),				\
			   FP_FIELD(_p5, p->params[4]),				\
			   FP_FIELD(_p6, p->params[5]),				\
			   FP_FIELD(_p7, p->params[6]),				\
			   FP_FIELD(_p8, p->params[7]),				\
			   FP_FIELD(_p9, p->params[8]),				\
			   FP_FIELD(_p10, p->params[9]),			\
			   FP_FIELD(_p11, p->params[10]),			\
			   t);										\
		break;	

// parameter type field selectors
#define TYPE_FLOAT_FP_FIELD				f
#define TYPE_INT_FP_FIELD				i
#define TYPE_RGBA_FP_FIELD				p
#define TYPE_POINT3_FP_FIELD			p
#define TYPE_BOOL_FP_FIELD				i
#define TYPE_ANGLE_FP_FIELD				f
#define TYPE_PCNT_FRAC_FP_FIELD			f
#define TYPE_WORLD_FP_FIELD				f
#define TYPE_STRING_FP_FIELD			s
#define TYPE_FILENAME_FP_FIELD			s
#define TYPE_HSV_FP_FIELD				p
#define TYPE_COLOR_CHANNEL_FP_FIELD		f
#define TYPE_TIMEVALUE_FP_FIELD			i
#define TYPE_RADIOBTN_INDEX_FP_FIELD	i
#define TYPE_MTL_FP_FIELD				mtl
#define TYPE_TEXMAP_FP_FIELD			tex
#define TYPE_BITMAP_FP_FIELD			bm
#define TYPE_INODE_FP_FIELD				n
#define TYPE_REFTARG_FP_FIELD			r
#define TYPE_INDEX_FP_FIELD				i
#define TYPE_MATRIX3_FP_FIELD			m
#define TYPE_VOID_FP_FIELD				void_paramtype_bad
#define TYPE_ENUM_FP_FIELD				i
#define TYPE_INTERVAL_FP_FIELD			intvl
#define TYPE_ANGAXIS_FP_FIELD			aa
#define TYPE_QUAT_FP_FIELD				q
#define TYPE_RAY_FP_FIELD				ray
#define TYPE_POINT2_FP_FIELD			p2
#define TYPE_BITARRAY_FP_FIELD			bits
#define TYPE_CLASS_FP_FIELD				cd
#define TYPE_MESH_FP_FIELD				msh
#define TYPE_OBJECT_FP_FIELD			obj
#define TYPE_CONTROL_FP_FIELD			ctrl
#define TYPE_POINT_FP_FIELD				pt
#define TYPE_TSTR_FP_FIELD				tstr
#define TYPE_IOBJECT_FP_FIELD			iobj
#define TYPE_INTERFACE_FP_FIELD			fpi
#define TYPE_HWND_FP_FIELD				hwnd
#define TYPE_NAME_FP_FIELD				s
#define TYPE_COLOR_FP_FIELD				clr
#define TYPE_FPVALUE_FP_FIELD			fpv
#define TYPE_VALUE_FP_FIELD				v
#define TYPE_DWORD_FP_FIELD				d
#define TYPE_bool_FP_FIELD				b

// Tab<>s of the above...

#define TYPE_FLOAT_TAB_FP_FIELD				f_tab
#define TYPE_INT_TAB_FP_FIELD				i_tab
#define TYPE_RGBA_TAB_FP_FIELD				p_tab
#define TYPE_POINT3_TAB_FP_FIELD			p_tab
#define TYPE_BOOL_TAB_FP_FIELD				i_tab
#define TYPE_ANGLE_TAB_FP_FIELD				f_tab
#define TYPE_PCNT_FRAC_TAB_FP_FIELD			f_tab
#define TYPE_WORLD_TAB_FP_FIELD				f_tab
#define TYPE_STRING_TAB_FP_FIELD			s_tab
#define TYPE_FILENAME_TAB_FP_FIELD			s_tab
#define TYPE_HSV_TAB_FP_FIELD				p_tab
#define TYPE_COLOR_CHANNEL_TAB_FP_FIELD		f_tab
#define TYPE_TIMEVALUE_TAB_FP_FIELD			i_tab
#define TYPE_RADIOBTN_INDEX_TAB_FP_FIELD	i_tab
#define TYPE_MTL_TAB_FP_FIELD				mtl_tab
#define TYPE_TEXMAP_TAB_FP_FIELD			tex_tab
#define TYPE_BITMAP_TAB_FP_FIELD			bm_tab
#define TYPE_INODE_TAB_FP_FIELD				n_tab
#define TYPE_REFTARG_TAB_FP_FIELD			r_tab
#define TYPE_INDEX_TAB_FP_FIELD				i_tab
#define TYPE_MATRIX3_TAB_FP_FIELD			m_tab
#define TYPE_ENUM_TAB_FP_FIELD				i_tab
#define TYPE_INTERVAL_TAB_FP_FIELD			intvl_tab
#define TYPE_ANGAXIS_TAB_FP_FIELD			aa_tab
#define TYPE_QUAT_TAB_FP_FIELD				q_tab
#define TYPE_RAY_TAB_FP_FIELD				ray_tab
#define TYPE_POINT2_TAB_FP_FIELD			p2_tab
#define TYPE_BITARRAY_TAB_FP_FIELD			bits_tab
#define TYPE_CLASS_TAB_FP_FIELD				cd_tab
#define TYPE_MESH_TAB_FP_FIELD				msh_tab
#define TYPE_OBJECT_TAB_FP_FIELD			obj_tab
#define TYPE_CONTROL_TAB_FP_FIELD			ctrl_tab
#define TYPE_POINT_TAB_FP_FIELD				pt_tab
#define TYPE_TSTR_TAB_FP_FIELD				tstr_tab
#define TYPE_IOBJECT_TAB_FP_FIELD			iobj_tab
#define TYPE_INTERFACE_TAB_FP_FIELD			fpi_tab
#define TYPE_HWND_TAB_FP_FIELD				hwnd_tab
#define TYPE_NAME_TAB_FP_FIELD				s_tab
#define TYPE_COLOR_TAB_FP_FIELD				clr_tab
#define TYPE_FPVALUE_TAB_FP_FIELD			fpv_tab
#define TYPE_VALUE_TAB_FP_FIELD				v_tab
#define TYPE_DWORD_TAB_FP_FIELD				d_tab
#define TYPE_bool_TAB_FP_FIELD				b_tab

// by-pointer fields
#define TYPE_FLOAT_BP_FP_FIELD				fptr		
#define TYPE_INT_BP_FP_FIELD				iptr		
#define TYPE_BOOL_BP_FP_FIELD				iptr		
#define TYPE_ANGLE_BP_FP_FIELD				fptr		
#define TYPE_PCNT_FRAC_BP_FP_FIELD			fptr	
#define TYPE_WORLD_BP_FP_FIELD				fptr
#define TYPE_COLOR_CHANNEL_BP_FP_FIELD		fptr
#define TYPE_TIMEVALUE_BP_FP_FIELD			iptr
#define TYPE_RADIOBTN_INDEX_BP_FP_FIELD		iptr
#define TYPE_INDEX_BP_FP_FIELD				iptr
#define TYPE_ENUM_BP_FP_FIELD				iptr
#define TYPE_DWORD_BP_FP_FIELD				dptr
#define TYPE_bool_BP_FP_FIELD				bptr

// by-reference fields
#define TYPE_FLOAT_BR_FP_FIELD				fptr		
#define TYPE_INT_BR_FP_FIELD				iptr		
#define TYPE_RGBA_BR_FP_FIELD				p	
#define TYPE_POINT3_BR_FP_FIELD				p	
#define TYPE_BOOL_BR_FP_FIELD				iptr		
#define TYPE_ANGLE_BR_FP_FIELD				fptr		
#define TYPE_PCNT_FRAC_BR_FP_FIELD			fptr	
#define TYPE_WORLD_BR_FP_FIELD				fptr
#define TYPE_HSV_BR_FP_FIELD				p
#define TYPE_COLOR_CHANNEL_BR_FP_FIELD		f
#define TYPE_TIMEVALUE_BR_FP_FIELD			iptr
#define TYPE_RADIOBTN_INDEX_BR_FP_FIELD		iptr
#define TYPE_BITMAP_BR_FP_FIELD				bm
#define TYPE_INDEX_BR_FP_FIELD				iptr
#define TYPE_ENUM_BR_FP_FIELD				iptr
#define TYPE_REFTARG_BR_FP_FIELD			r
#define TYPE_MATRIX3_BR_FP_FIELD			m
#define TYPE_ANGAXIS_BR_FP_FIELD			aa
#define TYPE_QUAT_BR_FP_FIELD				q
#define TYPE_BITARRAY_BR_FP_FIELD			bits
#define TYPE_RAY_BR_FP_FIELD				ray
#define TYPE_POINT2_BR_FP_FIELD				p2
#define TYPE_MESH_BR_FP_FIELD				msh
#define TYPE_INTERVAL_BR_FP_FIELD			intvl
#define TYPE_POINT_BR_FP_FIELD				pt
#define TYPE_TSTR_BR_FP_FIELD				tstr
#define TYPE_COLOR_BR_FP_FIELD				clr
#define TYPE_FPVALUE_BR_FP_FIELD			fpv
#define TYPE_DWORD_BR_FP_FIELD				dptr
#define TYPE_bool_BR_FP_FIELD				bptr

// Tab<> by-reference fields
#define TYPE_FLOAT_TAB_BR_FP_FIELD			f_tab
#define TYPE_INT_TAB_BR_FP_FIELD			i_tab
#define TYPE_RGBA_TAB_BR_FP_FIELD			p_tab
#define TYPE_POINT3_TAB_BR_FP_FIELD			p_tab
#define TYPE_BOOL_TAB_BR_FP_FIELD			i_tab
#define TYPE_ANGLE_TAB_BR_FP_FIELD			f_tab
#define TYPE_PCNT_FRAC_TAB_BR_FP_FIELD		f_tab
#define TYPE_WORLD_TAB_BR_FP_FIELD			f_tab
#define TYPE_STRING_TAB_BR_FP_FIELD			s_tab
#define TYPE_FILENAME_TAB_BR_FP_FIELD		s_tab
#define TYPE_HSV_TAB_BR_FP_FIELD			p_tab
#define TYPE_COLOR_CHANNEL_TAB_BR_FP_FIELD	f_tab
#define TYPE_TIMEVALUE_TAB_BR_FP_FIELD		i_tab
#define TYPE_RADIOBTN_INDEX_TAB_BR_FP_FIELD i_tab
#define TYPE_MTL_TAB_BR_FP_FIELD			mtl_tab
#define TYPE_TEXMAP_TAB_BR_FP_FIELD			tex_tab
#define TYPE_BITMAP_TAB_BR_FP_FIELD			bm_tab
#define TYPE_INODE_TAB_BR_FP_FIELD			n_tab
#define TYPE_REFTARG_TAB_BR_FP_FIELD		r_tab
#define TYPE_INDEX_TAB_BR_FP_FIELD			i_tab
#define TYPE_ENUM_TAB_BR_FP_FIELD			i_tab
#define TYPE_MATRIX3_TAB_BR_FP_FIELD		m_tab
#define TYPE_ANGAXIS_TAB_BR_FP_FIELD		aa_tab
#define TYPE_QUAT_TAB_BR_FP_FIELD			q_tab
#define TYPE_BITARRAY_TAB_BR_FP_FIELD		bits_tab
#define TYPE_CLASS_TAB_BR_FP_FIELD			cd_tab
#define TYPE_RAY_TAB_BR_FP_FIELD			ray_tab
#define TYPE_POINT2_TAB_BR_FP_FIELD			p2_tab
#define TYPE_MESH_TAB_BR_FP_FIELD			msh_tab
#define TYPE_OBJECT_TAB_BR_FP_FIELD			obj_tab
#define TYPE_CONTROL_TAB_BR_FP_FIELD		ctrl_tab
#define TYPE_INTERVAL_TAB_BR_FP_FIELD		intvl_tab
#define TYPE_POINT_TAB_BR_FP_FIELD			pt_tab
#define TYPE_HWND_TAB_BR_FP_FIELD			hwnd_tab
#define TYPE_TSTR_TAB_BR_FP_FIELD			tstr_tab
#define TYPE_IOBJECT_TAB_BR_FP_FIELD		iobj_tab
#define TYPE_INTERFACE_TAB_BR_FP_FIELD		fpi_tab
#define TYPE_NAME_TAB_BR_FP_FIELD			s_tab
#define TYPE_COLOR_TAB_BR_FP_FIELD			clr_tab
#define TYPE_FPVALUE_TAB_BR_FP_FIELD		fpv_tab
#define TYPE_VALUE_TAB_BR_FP_FIELD			v_tab
#define TYPE_DWORD_TAB_BR_FP_FIELD			d_tab
#define TYPE_bool_TAB_BR_FP_FIELD			b_tab

// by-value fields
#define TYPE_RGBA_BV_FP_FIELD				p	
#define TYPE_POINT3_BV_FP_FIELD				p	
#define TYPE_HSV_BV_FP_FIELD				p
#define TYPE_BITMAP_BV_FP_FIELD				bm
#define TYPE_MATRIX3_BV_FP_FIELD			m
#define TYPE_ANGAXIS_BV_FP_FIELD			aa
#define TYPE_QUAT_BV_FP_FIELD				q
#define TYPE_BITARRAY_BV_FP_FIELD			bits
#define TYPE_RAY_BV_FP_FIELD				ray
#define TYPE_POINT2_BV_FP_FIELD				p2
#define TYPE_MESH_BV_FP_FIELD				msh
#define TYPE_INTERVAL_BV_FP_FIELD			intvl
#define TYPE_POINT_BV_FP_FIELD				pt
#define TYPE_TSTR_BV_FP_FIELD				tstr
#define TYPE_COLOR_BV_FP_FIELD				clr
#define TYPE_FPVALUE_BV_FP_FIELD			fpv
#define TYPE_CLASS_BV_FP_FIELD				cd

// by-val Tab<> fields
#define TYPE_FLOAT_TAB_BV_FP_FIELD			f_tab
#define TYPE_INT_TAB_BV_FP_FIELD			i_tab
#define TYPE_RGBA_TAB_BV_FP_FIELD			p_tab
#define TYPE_POINT3_TAB_BV_FP_FIELD			p_tab
#define TYPE_BOOL_TAB_BV_FP_FIELD			i_tab
#define TYPE_ANGLE_TAB_BV_FP_FIELD			f_tab
#define TYPE_PCNT_FRAC_TAB_BV_FP_FIELD		f_tab
#define TYPE_WORLD_TAB_BV_FP_FIELD			f_tab
#define TYPE_STRING_TAB_BV_FP_FIELD			s_tab
#define TYPE_FILENAME_TAB_BV_FP_FIELD		s_tab
#define TYPE_HSV_TAB_BV_FP_FIELD			p_tab
#define TYPE_COLOR_CHANNEL_TAB_BV_FP_FIELD	f_tab
#define TYPE_TIMEVALUE_TAB_BV_FP_FIELD		i_tab
#define TYPE_RADIOBTN_INDEX_TAB_BV_FP_FIELD i_tab
#define TYPE_MTL_TAB_BV_FP_FIELD			mtl_tab
#define TYPE_TEXMAP_TAB_BV_FP_FIELD			tex_tab
#define TYPE_BITMAP_TAB_BV_FP_FIELD			bm_tab
#define TYPE_INODE_TAB_BV_FP_FIELD			n_tab
#define TYPE_REFTARG_TAB_BV_FP_FIELD		r_tab
#define TYPE_INDEX_TAB_BV_FP_FIELD			i_tab
#define TYPE_ENUM_TAB_BV_FP_FIELD			i_tab
#define TYPE_MATRIX3_TAB_BV_FP_FIELD		m_tab
#define TYPE_ANGAXIS_TAB_BV_FP_FIELD		aa_tab
#define TYPE_QUAT_TAB_BV_FP_FIELD			q_tab
#define TYPE_BITARRAY_TAB_BV_FP_FIELD		bits_tab
#define TYPE_CLASS_TAB_BV_FP_FIELD			cd_tab
#define TYPE_RAY_TAB_BV_FP_FIELD			ray_tab
#define TYPE_POINT2_TAB_BV_FP_FIELD			p2_tab
#define TYPE_MESH_TAB_BV_FP_FIELD			msh_tab
#define TYPE_OBJECT_TAB_BV_FP_FIELD			obj_tab
#define TYPE_CONTROL_TAB_BV_FP_FIELD		ctrl_tab
#define TYPE_INTERVAL_TAB_BV_FP_FIELD		intvl_tab
#define TYPE_POINT_TAB_BV_FP_FIELD			pt_tab
#define TYPE_HWND_TAB_BV_FP_FIELD			hwnd_tab
#define TYPE_TSTR_TAB_BV_FP_FIELD			tstr_tab
#define TYPE_IOBJECT_TAB_BV_FP_FIELD		iobj_tab
#define TYPE_INTERFACE_TAB_BV_FP_FIELD		fpi_tab
#define TYPE_NAME_TAB_BV_FP_FIELD			s_tab
#define TYPE_COLOR_TAB_BV_FP_FIELD			s_tab
#define TYPE_FPVALUE_TAB_BV_FP_FIELD		fpv_tab
#define TYPE_VALUE_TAB_BV_FP_FIELD			v_tab
#define TYPE_DWORD_TAB_BV_FP_FIELD			d_tab
#define TYPE_bool_TAB_BV_FP_FIELD			b_tab

// field access macros...  

// base types, yield 'conventional' type passing conventions
//   ie, ints, floats, points, colors, 3D math types are passed as values, all 
//   others passed as pointers

#define TYPE_FLOAT_FIELD(_v)			(((_v).f))		
#define TYPE_INT_FIELD(_v)				(((_v).i))		
#define TYPE_RGBA_FIELD(_v)				(*((_v).p))	
#define TYPE_POINT3_FIELD(_v)			(*((_v).p))	
#define TYPE_BOOL_FIELD(_v)				(((_v).i))		
#define TYPE_ANGLE_FIELD(_v)			(((_v).f))		
#define TYPE_PCNT_FRAC_FIELD(_v)		(((_v).f))	
#define TYPE_WORLD_FIELD(_v)			(((_v).f))
#define TYPE_STRING_FIELD(_v)			(((_v).s))
#define TYPE_FILENAME_FIELD(_v)			(((_v).s))
#define TYPE_HSV_FIELD(_v)				(*((_v).p))
#define TYPE_COLOR_CHANNEL_FIELD(_v)	(((_v).f))
#define TYPE_TIMEVALUE_FIELD(_v)		(((_v).i))
#define TYPE_RADIOBTN_INDEX_FIELD(_v)	(((_v).i))
#define TYPE_MTL_FIELD(_v)				(((_v).mtl))
#define TYPE_TEXMAP_FIELD(_v)			(((_v).tex))
#define TYPE_BITMAP_FIELD(_v)			(((_v).bm))
#define TYPE_INODE_FIELD(_v)			(((_v).n))
#define TYPE_REFTARG_FIELD(_v)			(((_v).r))
#define TYPE_INDEX_FIELD(_v)			(((_v).i))
#define TYPE_ENUM_FIELD(_v)				(((_v).i))
#define TYPE_MATRIX3_FIELD(_v)			(*((_v).m))
#define TYPE_ANGAXIS_FIELD(_v)			(*((_v).aa))
#define TYPE_QUAT_FIELD(_v)				(*((_v).q))
#define TYPE_BITARRAY_FIELD(_v)			(((_v).bits))
#define TYPE_CLASS_FIELD(_v)			(((_v).cd))
#define TYPE_RAY_FIELD(_v)				(*((_v).ray))
#define TYPE_POINT2_FIELD(_v)			(*((_v).p2))
#define TYPE_MESH_FIELD(_v)				(((_v).msh))
#define TYPE_OBJECT_FIELD(_v)			(((_v).obj))
#define TYPE_CONTROL_FIELD(_v)			(((_v).ctrl))
#define TYPE_INTERVAL_FIELD(_v)			(*((_v).intvl))
#define TYPE_POINT_FIELD(_v)			(*((_v).pt))
#define TYPE_TSTR_FIELD(_v)				(*((_v).tstr))
#define TYPE_IOBJECT_FIELD(_v)			(((_v).iobj))
#define TYPE_INTERFACE_FIELD(_v)		(((_v).fpi))
#define TYPE_HWND_FIELD(_v)				(((_v).hwnd))
#define TYPE_NAME_FIELD(_v)				(((_v).s))
#define TYPE_COLOR_FIELD(_v)			(((_v).clr))
#define TYPE_FPVALUE_FIELD(_v)			(((_v).fpv))
#define TYPE_VALUE_FIELD(_v)			(((_v).v))
#define TYPE_DWORD_FIELD(_v)			(((_v).d))
#define TYPE_bool_FIELD(_v)				(((_v).b))

// all Tab<> types passed by pointer

#define TYPE_FLOAT_TAB_FIELD(_v)			(((_v).f_tab))
#define TYPE_INT_TAB_FIELD(_v)				(((_v).i_tab))
#define TYPE_RGBA_TAB_FIELD(_v)				(((_v).p_tab))
#define TYPE_POINT3_TAB_FIELD(_v)			(((_v).p_tab))
#define TYPE_BOOL_TAB_FIELD(_v)				(((_v).i_tab))
#define TYPE_ANGLE_TAB_FIELD(_v)			(((_v).f_tab))
#define TYPE_PCNT_FRAC_TAB_FIELD(_v)		(((_v).f_tab))
#define TYPE_WORLD_TAB_FIELD(_v)			(((_v).f_tab))
#define TYPE_STRING_TAB_FIELD(_v)			(((_v).s_tab))
#define TYPE_FILENAME_TAB_FIELD(_v)			(((_v).s_tab))
#define TYPE_HSV_TAB_FIELD(_v)				(((_v).p_tab))
#define TYPE_COLOR_CHANNEL_TAB_FIELD(_v)	(((_v).f_tab))
#define TYPE_TIMEVALUE_TAB_FIELD(_v)		(((_v).i_tab))
#define TYPE_RADIOBTN_INDEX_TAB_FIELD(_v)	(((_v).i_tab))
#define TYPE_MTL_TAB_FIELD(_v)				(((_v).mtl_tab))
#define TYPE_TEXMAP_TAB_FIELD(_v)			(((_v).tex_tab))
#define TYPE_BITMAP_TAB_FIELD(_v)			(((_v).bm_tab))
#define TYPE_INODE_TAB_FIELD(_v)			(((_v).n_tab))
#define TYPE_REFTARG_TAB_FIELD(_v)			(((_v).r_tab))
#define TYPE_INDEX_TAB_FIELD(_v)			(((_v).i_tab))
#define TYPE_ENUM_TAB_FIELD(_v)				(((_v).i_tab))
#define TYPE_MATRIX3_TAB_FIELD(_v)			(((_v).m_tab))
#define TYPE_ANGAXIS_TAB_FIELD(_v)			(((_v).aa_tab))
#define TYPE_QUAT_TAB_FIELD(_v)				(((_v).q_tab))
#define TYPE_BITARRAY_TAB_FIELD(_v)			(((_v).bits_tab))
#define TYPE_CLASS_TAB_FIELD(_v)			(((_v).cd_tab))
#define TYPE_RAY_TAB_FIELD(_v)				(((_v).ray_tab))
#define TYPE_POINT2_TAB_FIELD(_v)			(((_v).p2_tab))
#define TYPE_MESH_TAB_FIELD(_v)				(((_v).msh_tab))
#define TYPE_OBJECT_TAB_FIELD(_v)			(((_v).obj_tab))
#define TYPE_CONTROL_TAB_FIELD(_v)			(((_v).ctrl_tab))
#define TYPE_INTERVAL_TAB_FIELD(_v)			(((_v).intvl_tab))
#define TYPE_POINT_TAB_FIELD(_v)			(((_v).pt_tab))
#define TYPE_TSTRT_TAB_FIELD(_v)			(((_v).tstr_tab))
#define TYPE_IOBJECT_TAB_FIELD(_v)			(((_v).iobj_tab))
#define TYPE_INTERFACE_TAB_FIELD(_v)		(((_v).fpi_tab))
#define TYPE_HWND_TAB_FIELD(_v)				(((_v).hwnd_tab))
#define TYPE_NAME_TAB_FIELD(_v)				(((_v).s_tab))
#define TYPE_COLOR_TAB_FIELD(_v)			(((_v).clr_tab))
#define TYPE_FPVALUE_TAB_FIELD(_v)			(((_v).fpv_tab))
#define TYPE_VALUE_TAB_FIELD(_v)			(((_v).v_tab))
#define TYPE_DWORD_TAB_FIELD(_v)			(((_v).d_tab))
#define TYPE_bool_TAB_FIELD(_v)				(((_v).b_tab))

// the following variants all assume a pointer is used as the source of the 
// param, but deliver it to the called interface function in the given mode, 
//  _BP -> a pointer, eg, int* x
//  _BR -> a reference, eg, int& x
//  _BV -> a dereferenced value, only for pointer-based types, derived by *fpvalue.ptr

// * (pointer) field access macros
// pass by-pointer types for int & float types, implies * parameters, int* & float* are passed via .ptr fields, only for FnPub use
#define TYPE_FLOAT_BP_FIELD(_v)				(((_v).fptr))		
#define TYPE_INT_BP_FIELD(_v)				(((_v).iptr))		
#define TYPE_BOOL_BP_FIELD(_v)				(((_v).iptr))		
#define TYPE_ANGLE_BP_FIELD(_v)				(((_v).fptr))		
#define TYPE_PCNT_FRAC_BP_FIELD(_v)			(((_v).fptr))	
#define TYPE_WORLD_BP_FIELD(_v)				(((_v).fptr))
#define TYPE_COLOR_CHANNEL_BP_FIELD(_v)		(((_v).fptr))
#define TYPE_TIMEVALUE_BP_FIELD(_v)			(((_v).iptr))
#define TYPE_RADIOBTN_INDEX_BP_FIELD(_v)	(((_v).iptr))
#define TYPE_INDEX_BP_FIELD(_v)				(((_v).iptr))
#define TYPE_ENUM_BP_FIELD(_v)				(((_v).iptr))
#define TYPE_DWORD_BP_FIELD(_v)				(((_v).dptr))
#define TYPE_bool_BP_FIELD(_v)				(((_v).bptr))
// there are no specific by-pointer Tab<> types, all Tab<> types are by-pointer by default

// & (reference) field access macros
// pass by-ref types, implies & parameters, int& & float& are passed via .ptr fields, only for FnPub use
#define TYPE_FLOAT_BR_FIELD(_v)				(*((_v).fptr))		
#define TYPE_INT_BR_FIELD(_v)				(*((_v).iptr))		
#define TYPE_RGBA_BR_FIELD(_v)				(*((_v).p))	
#define TYPE_POINT3_BR_FIELD(_v)			(*((_v).p))	
#define TYPE_BOOL_BR_FIELD(_v)				(*((_v).iptr))		
#define TYPE_ANGLE_BR_FIELD(_v)				(*((_v).fptr))		
#define TYPE_PCNT_FRAC_BR_FIELD(_v)			(*((_v).fptr))	
#define TYPE_WORLD_BR_FIELD(_v)				(*((_v).fptr))
#define TYPE_HSV_BR_FIELD(_v)				(*((_v).p))
#define TYPE_COLOR_CHANNEL_BR_FIELD(_v)		(*((_v).f))
#define TYPE_TIMEVALUE_BR_FIELD(_v)			(*((_v).iptr))
#define TYPE_RADIOBTN_INDEX_BR_FIELD(_v)	(*((_v).iptr))
#define TYPE_BITMAP_BR_FIELD(_v)			(*((_v).bm))
#define TYPE_INDEX_BR_FIELD(_v)				(*((_v).iptr))
#define TYPE_ENUMBR_FIELD(_v)				(*((_v).iptr))
#define TYPE_REFTARG_BR_FIELD(_v)			(*((_v).r))
#define TYPE_MATRIX3_BR_FIELD(_v)			(*((_v).m))
#define TYPE_ANGAXIS_BR_FIELD(_v)			(*((_v).aa))
#define TYPE_QUAT_BR_FIELD(_v)				(*((_v).q))
#define TYPE_BITARRAY_BR_FIELD(_v)			(*((_v).bits))
#define TYPE_RAY_BR_FIELD(_v)				(*((_v).ray))
#define TYPE_POINT2_BR_FIELD(_v)			(*((_v).p2))
#define TYPE_MESH_BR_FIELD(_v)				(*((_v).msh))
#define TYPE_INTERVAL_BR_FIELD(_v)			(*((_v).intvl))
#define TYPE_POINT_BR_FIELD(_v)				(*((_v).pt))
#define TYPE_TSTR_BR_FIELD(_v)				(*((_v).tstr))
#define TYPE_COLOR_BR_FIELD(_v)				(*((_v).clr))
#define TYPE_FPVALUE_BR_FIELD(_v)			(*((_v).fpv))
#define TYPE_DWORD_BR_FIELD(_v)				(*((_v).d))
#define TYPE_bool_BR_FIELD(_v)				(*((_v).b))

// refs to Tab<>s

#define TYPE_FLOAT_TAB_BR_FIELD(_v)			(*((_v).f_tab))
#define TYPE_INT_TAB_BR_FIELD(_v)			(*((_v).i_tab))
#define TYPE_RGBA_TAB_BR_FIELD(_v)			(*((_v).p_tab))
#define TYPE_POINT3_TAB_BR_FIELD(_v)		(*((_v).p_tab))
#define TYPE_BOOL_TAB_BR_FIELD(_v)			(*((_v).i_tab))
#define TYPE_ANGLE_TAB_BR_FIELD(_v)			(*((_v).f_tab))
#define TYPE_PCNT_FRAC_TAB_BR_FIELD(_v)		(*((_v).f_tab))
#define TYPE_WORLD_TAB_BR_FIELD(_v)			(*((_v).f_tab))
#define TYPE_STRING_TAB_BR_FIELD(_v)		(*((_v).s_tab))
#define TYPE_FILENAME_TAB_BR_FIELD(_v)		(*((_v).s_tab))
#define TYPE_HSV_TAB_BR_FIELD(_v)			(*((_v).p_tab))
#define TYPE_COLOR_CHANNEL_TAB_BR_FIELD(_v)	(*((_v).f_tab))
#define TYPE_TIMEVALUE_TAB_BR_FIELD(_v)		(*((_v).i_tab))
#define TYPE_RADIOBTN_INDEX_TAB_BR_FIELD(_v) (*((_v).i_tab))
#define TYPE_MTL_TAB_BR_FIELD(_v)			(*((_v).mtl_tab))
#define TYPE_TEXMAP_TAB_BR_FIELD(_v)		(*((_v).tex_tab))
#define TYPE_BITMAP_TAB_BR_FIELD(_v)		(*((_v).bm_tab))
#define TYPE_INODE_TAB_BR_FIELD(_v)			(*((_v).n_tab))
#define TYPE_REFTARG_TAB_BR_FIELD(_v)		(*((_v).r_tab))
#define TYPE_INDEX_TAB_BR_FIELD(_v)			(*((_v).i_tab))
#define TYPE_ENUM_TAB_BR_FIELD(_v)			(*((_v).i_tab))
#define TYPE_MATRIX3_TAB_BR_FIELD(_v)		(*((_v).m_tab))
#define TYPE_ANGAXIS_TAB_BR_FIELD(_v)		(*((_v).aa_tab))
#define TYPE_QUAT_TAB_BR_FIELD(_v)			(*((_v).q_tab))
#define TYPE_BITARRAY_TAB_BR_FIELD(_v)		(*((_v).bits_tab))
#define TYPE_CLASS_TAB_BR_FIELD(_v)			(*((_v).cd_tab))
#define TYPE_RAY_TAB_BR_FIELD(_v)			(*((_v).ray_tab))
#define TYPE_POINT2_TAB_BR_FIELD(_v)		(*((_v).p2_tab))
#define TYPE_MESH_TAB_BR_FIELD(_v)			(*((_v).msh_tab))
#define TYPE_OBJECT_TAB_BR_FIELD(_v)		(*((_v).obj_tab))
#define TYPE_CONTROL_TAB_BR_FIELD(_v)		(*((_v).ctrl_tab))
#define TYPE_INTERVAL_TAB_BR_FIELD(_v)		(*((_v).intvl_tab))
#define TYPE_POINT_TAB_BR_FIELD(_v)			(*((_v).pt_tab))
#define TYPE_HWND_TAB_BR_FIELD(_v)			(*((_v).hwnd_tab))
#define TYPE_TSTR_TAB_BR_FIELD(_v)			(*((_v).tstr_tab))
#define TYPE_IOBJECT_TAB_BR_FIELD(_v)		(*((_v).iobj_tab))
#define TYPE_INTERFACE_TAB_BR_FIELD(_v)		(*((_v).fpi_tab))
#define TYPE_NAME_TAB_BR_FIELD(_v)			(*((_v).s_tab))
#define TYPE_COLOR_TAB_BR_FIELD(_v)			(*((_v).clr_tab))
#define TYPE_FPVALUE_TAB_BR_FIELD(_v)		(*((_v).fpv_tab))
#define TYPE_VALUE_TAB_BR_FIELD(_v)			(*((_v).v_tab))
#define TYPE_DWORD_TAB_BR_FIELD(_v)			(*((_v).d_tab))
#define TYPE_bool_TAB_BR_FIELD(_v)			(*((_v).b_tab))
	
// by value field access macros
// pass by-value types, implies dereferencing the (meaningful) pointer-based values, only for FnPub use
#define TYPE_RGBA_BV_FIELD(_v)				(*((_v).p))	
#define TYPE_POINT3_BV_FIELD(_v)			(*((_v).p))	
#define TYPE_HSV_BV_FIELD(_v)				(*((_v).p))
#define TYPE_BITMAP_BV_FIELD(_v)			(*((_v).bm))
#define TYPE_MATRIX3_BV_FIELD(_v)			(*((_v).m))
#define TYPE_ANGAXIS_BV_FIELD(_v)			(*((_v).aa))
#define TYPE_QUAT_BV_FIELD(_v)				(*((_v).q))
#define TYPE_BITARRAY_BV_FIELD(_v)			(*((_v).bits))
#define TYPE_RAY_BV_FIELD(_v)				(*((_v).ray))
#define TYPE_POINT2_BV_FIELD(_v)			(*((_v).p2))
#define TYPE_MESH_BV_FIELD(_v)				(*((_v).msh))
#define TYPE_INTERVAL_BV_FIELD(_v)			(*((_v).intvl))
#define TYPE_POINT_BV_FIELD(_v)				(*((_v).pt))
#define TYPE_TSTR_BV_FIELD(_v)				(*((_v).tstr))
#define TYPE_COLOR_BV_FIELD(_v)				(*((_v).clr))
#define TYPE_FPVALUE_BV_FIELD(_v)			(*((_v).fpv))
#define TYPE_CLASS_BV_FIELD(_v)				(*((_v).cd))

// pass by-val Tab<> types
#define TYPE_FLOAT_TAB_BV_FIELD(_v)			(*((_v).f_tab))
#define TYPE_INT_TAB_BV_FIELD(_v)			(*((_v).i_tab))
#define TYPE_RGBA_TAB_BV_FIELD(_v)			(*((_v).p_tab))
#define TYPE_POINT3_TAB_BV_FIELD(_v)		(*((_v).p_tab))
#define TYPE_BOOL_TAB_BV_FIELD(_v)			(*((_v).i_tab))
#define TYPE_ANGLE_TAB_BV_FIELD(_v)			(*((_v).f_tab))
#define TYPE_PCNT_FRAC_TAB_BV_FIELD(_v)		(*((_v).f_tab))
#define TYPE_WORLD_TAB_BV_FIELD(_v)			(*((_v).f_tab))
#define TYPE_STRING_TAB_BV_FIELD(_v)		(*((_v).s_tab))
#define TYPE_FILENAME_TAB_BV_FIELD(_v)		(*((_v).s_tab))
#define TYPE_HSV_TAB_BV_FIELD(_v)			(*((_v).p_tab))
#define TYPE_COLOR_CHANNEL_TAB_BV_FIELD(_v)	(*((_v).f_tab))
#define TYPE_TIMEVALUE_TAB_BV_FIELD(_v)		(*((_v).i_tab))
#define TYPE_RADIOBTN_INDEX_TAB_BV_FIELD(_v) (*((_v).i_tab))
#define TYPE_MTL_TAB_BV_FIELD(_v)			(*((_v).mtl_tab))
#define TYPE_TEXMAP_TAB_BV_FIELD(_v)		(*((_v).tex_tab))
#define TYPE_BITMAP_TAB_BV_FIELD(_v)		(*((_v).bm_tab))
#define TYPE_INODE_TAB_BV_FIELD(_v)			(*((_v).n_tab))
#define TYPE_REFTARG_TAB_BV_FIELD(_v)		(*((_v).r_tab))
#define TYPE_INDEX_TAB_BV_FIELD(_v)			(*((_v).i_tab))
#define TYPE_ENUM_TAB_BV_FIELD(_v)			(*((_v).i_tab))
#define TYPE_MATRIX3_TAB_BV_FIELD(_v)		(*((_v).m_tab))
#define TYPE_ANGAXIS_TAB_BV_FIELD(_v)		(*((_v).aa_tab))
#define TYPE_QUAT_TAB_BV_FIELD(_v)			(*((_v).q_tab))
#define TYPE_BITARRAY_TAB_BV_FIELD(_v)		(*((_v).bits_tab))
#define TYPE_CLASS_TAB_BV_FIELD(_v)			(*((_v).cd_tab))
#define TYPE_RAY_TAB_BV_FIELD(_v)			(*((_v).ray_tab))
#define TYPE_POINT2_TAB_BV_FIELD(_v)		(*((_v).p2_tab))
#define TYPE_MESH_TAB_BV_FIELD(_v)			(*((_v).msh_tab))
#define TYPE_OBJECT_TAB_BV_FIELD(_v)		(*((_v).obj_tab))
#define TYPE_CONTROL_TAB_BV_FIELD(_v)		(*((_v).ctrl_tab))
#define TYPE_INTERVAL_TAB_BV_FIELD(_v)		(*((_v).intvl_tab))
#define TYPE_POINT_TAB_BV_FIELD(_v)			(*((_v).pt_tab))
#define TYPE_HWND_TAB_BV_FIELD(_v)			(*((_v).hwnd_tab))
#define TYPE_TSTR_TAB_BV_FIELD(_v)			(*((_v).tstr_tab))
#define TYPE_IOBJECT_TAB_BV_FIELD(_v)		(*((_v).iobj_tab))
#define TYPE_INTERFACE_TAB_BV_FIELD(_v)		(*((_v).fpi_tab))
#define TYPE_NAME_TAB_BV_FIELD(_v)			(*((_v).s_tab))
#define TYPE_COLOR_TAB_BV_FIELD(_v)			(*((_v).s_tab))
#define TYPE_FPVALUE_TAB_BV_FIELD(_v)		(*((_v).fpv_tab))
#define TYPE_VALUE_TAB_BV_FIELD(_v)			(*((_v).v_tab))
#define TYPE_DWORD_TAB_BV_FIELD(_v)			(*((_v).d_tab))
#define TYPE_bool_TAB_BV_FIELD(_v)			(*((_v).b_tab))

// --- type result operators ----------------------

// used to generate an rvalue from the type's corresponding C++ type
// for assignment to the type's carrying field in FPValue.
// mostly empty, used by BY_REF & BY_VAL types to get pointers, since these
// types are actualy carried by pointer fields

// base types
#define TYPE_FLOAT_RSLT				
#define TYPE_INT_RSLT				
#define TYPE_RGBA_RSLT				
#define TYPE_POINT3_RSLT			
#define TYPE_BOOL_RSLT				
#define TYPE_ANGLE_RSLT				
#define TYPE_PCNT_FRAC_RSLT			
#define TYPE_WORLD_RSLT				
#define TYPE_STRING_RSLT			
#define TYPE_FILENAME_RSLT			
#define TYPE_HSV_RSLT				
#define TYPE_COLOR_CHANNEL_RSLT		
#define TYPE_TIMEVALUE_RSLT			
#define TYPE_RADIOBTN_INDEX_RSLT	
#define TYPE_MTL_RSLT				
#define TYPE_TEXMAP_RSLT			
#define TYPE_BITMAP_RSLT			
#define TYPE_INODE_RSLT				
#define TYPE_REFTARG_RSLT			
#define TYPE_INDEX_RSLT				
#define TYPE_ENUM_RSLT				
#define TYPE_MATRIX3_RSLT			
#define TYPE_VOID_RSLT				
#define TYPE_INTERVAL_RSLT			
#define TYPE_ANGAXIS_RSLT			
#define TYPE_QUAT_RSLT				
#define TYPE_RAY_RSLT				
#define TYPE_POINT2_RSLT			
#define TYPE_BITARRAY_RSLT			
#define TYPE_CLASS_RSLT				
#define TYPE_MESH_RSLT				
#define TYPE_OBJECT_RSLT			
#define TYPE_CONTROL_RSLT			
#define TYPE_POINT_RSLT				
#define TYPE_TSTR_RSLT				
#define TYPE_IOBJECT_RSLT			
#define TYPE_INTERFACE_RSLT			
#define TYPE_HWND_RSLT				
#define TYPE_NAME_RSLT				
#define TYPE_COLOR_RSLT				
#define TYPE_FPVALUE_RSLT				
#define TYPE_VALUE_RSLT				
#define TYPE_DWORD_RSLT				
#define TYPE_bool_RSLT				

// Tab<>s of the above...

#define TYPE_FLOAT_TAB_RSLT				
#define TYPE_INT_TAB_RSLT				
#define TYPE_RGBA_TAB_RSLT				
#define TYPE_POINT3_TAB_RSLT			
#define TYPE_BOOL_TAB_RSLT				
#define TYPE_ANGLE_TAB_RSLT				
#define TYPE_PCNT_FRAC_TAB_RSLT			
#define TYPE_WORLD_TAB_RSLT				
#define TYPE_STRING_TAB_RSLT			
#define TYPE_FILENAME_TAB_RSLT			
#define TYPE_HSV_TAB_RSLT				
#define TYPE_COLOR_CHANNEL_TAB_RSLT		
#define TYPE_TIMEVALUE_TAB_RSLT			
#define TYPE_RADIOBTN_INDEX_TAB_RSLT	
#define TYPE_MTL_TAB_RSLT				
#define TYPE_TEXMAP_TAB_RSLT			
#define TYPE_BITMAP_TAB_RSLT			
#define TYPE_INODE_TAB_RSLT				
#define TYPE_REFTARG_TAB_RSLT			
#define TYPE_INDEX_TAB_RSLT				
#define TYPE_ENUM_TAB_RSLT				
#define TYPE_MATRIX3_TAB_RSLT			
#define TYPE_INTERVAL_TAB_RSLT			
#define TYPE_ANGAXIS_TAB_RSLT			
#define TYPE_QUAT_TAB_RSLT				
#define TYPE_RAY_TAB_RSLT				
#define TYPE_POINT2_TAB_RSLT			
#define TYPE_BITARRAY_TAB_RSLT			
#define TYPE_CLASS_TAB_RSLT				
#define TYPE_MESH_TAB_RSLT				
#define TYPE_OBJECT_TAB_RSLT			
#define TYPE_CONTROL_TAB_RSLT			
#define TYPE_POINT_TAB_RSLT				
#define TYPE_TSTR_TAB_RSLT				
#define TYPE_IOBJECT_TAB_RSLT			
#define TYPE_INTERFACE_TAB_RSLT			
#define TYPE_HWND_TAB_RSLT				
#define TYPE_NAME_TAB_RSLT				
#define TYPE_COLOR_TAB_RSLT				
#define TYPE_FPVALUE_TAB_RSLT				
#define TYPE_VALUE_TAB_RSLT				
#define TYPE_DWORD_TAB_RSLT				
#define TYPE_bool_TAB_RSLT				

// by-pointer
//  foo*  = 
#define TYPE_FLOAT_BP_RSLT				
#define TYPE_INT_BP_RSLT				
#define TYPE_BOOL_BP_RSLT				
#define TYPE_ANGLE_BP_RSLT				
#define TYPE_PCNT_FRAC_BP_RSLT			
#define TYPE_WORLD_BP_RSLT				
#define TYPE_COLOR_CHANNEL_BP_RSLT		
#define TYPE_TIMEVALUE_BP_RSLT			
#define TYPE_RADIOBTN_INDEX_BP_RSLT		
#define TYPE_INDEX_BP_RSLT				
#define TYPE_ENUM_BP_RSLT				
#define TYPE_DWORD_BP_RSLT				
#define TYPE_bool_BP_RSLT				

// by-reference 
#define TYPE_FLOAT_BR_RSLT				&	
#define TYPE_INT_BR_RSLT				&
#define TYPE_RGBA_BR_RSLT				&
#define TYPE_POINT3_BR_RSLT				&
#define TYPE_BOOL_BR_RSLT				&
#define TYPE_ANGLE_BR_RSLT				&
#define TYPE_PCNT_FRAC_BR_RSLT			&
#define TYPE_WORLD_BR_RSLT				&
#define TYPE_HSV_BR_RSLT				&
#define TYPE_COLOR_CHANNEL_BR_RSLT		&
#define TYPE_TIMEVALUE_BR_RSLT			&
#define TYPE_RADIOBTN_INDEX_BR_RSLT		&
#define TYPE_BITMAP_BR_RSLT				&
#define TYPE_INDEX_BR_RSLT				&
#define TYPE_ENUM_BR_RSLT				&
#define TYPE_REFTARG_BR_RSLT			&
#define TYPE_MATRIX3_BR_RSLT			&
#define TYPE_ANGAXIS_BR_RSLT			&
#define TYPE_QUAT_BR_RSLT				&
#define TYPE_BITARRAY_BR_RSLT			&
#define TYPE_RAY_BR_RSLT				&
#define TYPE_POINT2_BR_RSLT				&
#define TYPE_MESH_BR_RSLT				&
#define TYPE_INTERVAL_BR_RSLT			&
#define TYPE_POINT_BR_RSLT				&
#define TYPE_TSTR_BR_RSLT				&
#define TYPE_COLOR_BR_RSLT				&
#define TYPE_FPVALUE_BR_RSLT			&
#define TYPE_DWORD_BR_RSLT				&
#define TYPE_bool_BR_RSLT				&

// Tab<> by-reference &
#define TYPE_FLOAT_TAB_BR_RSLT			&
#define TYPE_INT_TAB_BR_RSLT			&
#define TYPE_RGBA_TAB_BR_RSLT			&
#define TYPE_POINT3_TAB_BR_RSLT			&
#define TYPE_BOOL_TAB_BR_RSLT			&
#define TYPE_ANGLE_TAB_BR_RSLT			&
#define TYPE_PCNT_FRAC_TAB_BR_RSLT		&
#define TYPE_WORLD_TAB_BR_RSLT			&
#define TYPE_STRING_TAB_BR_RSLT			&
#define TYPE_FILENAME_TAB_BR_RSLT		&
#define TYPE_HSV_TAB_BR_RSLT			&
#define TYPE_COLOR_CHANNEL_TAB_BR_RSLT	&
#define TYPE_TIMEVALUE_TAB_BR_RSLT		&
#define TYPE_RADIOBTN_INDEX_TAB_BR_RSLT &
#define TYPE_MTL_TAB_BR_RSLT			&
#define TYPE_TEXMAP_TAB_BR_RSLT			&
#define TYPE_BITMAP_TAB_BR_RSLT			&
#define TYPE_INODE_TAB_BR_RSLT			&
#define TYPE_REFTARG_TAB_BR_RSLT		&
#define TYPE_INDEX_TAB_BR_RSLT			&
#define TYPE_ENUM_TAB_BR_RSLT			&
#define TYPE_MATRIX3_TAB_BR_RSLT		&
#define TYPE_ANGAXIS_TAB_BR_RSLT		&
#define TYPE_QUAT_TAB_BR_RSLT			&
#define TYPE_BITARRAY_TAB_BR_RSLT		&
#define TYPE_CLASS_TAB_BR_RSLT			&
#define TYPE_RAY_TAB_BR_RSLT			&
#define TYPE_POINT2_TAB_BR_RSLT			&
#define TYPE_MESH_TAB_BR_RSLT			&
#define TYPE_OBJECT_TAB_BR_RSLT			&
#define TYPE_CONTROL_TAB_BR_RSLT		&
#define TYPE_INTERVAL_TAB_BR_RSLT		&
#define TYPE_POINT_TAB_BR_RSLT			&
#define TYPE_HWND_TAB_BR_RSLT			&
#define TYPE_TSTR_TAB_BR_RSLT			&
#define TYPE_IOBJECT_TAB_BR_RSLT		&
#define TYPE_INTERFACE_TAB_BR_RSLT		&
#define TYPE_NAME_TAB_BR_RSLT			&
#define TYPE_COLOR_TAB_BR_RSLT			&
#define TYPE_FPVALUE_TAB_BR_RSLT		&
#define TYPE_VALUE_TAB_BR_RSLT			&
#define TYPE_DWORD_TAB_BR_RSLT			&
#define TYPE_bool_TAB_BR_RSLT			&

// by-value 
#define TYPE_RGBA_BV_RSLT				&
#define TYPE_POINT3_BV_RSLT				&
#define TYPE_HSV_BV_RSLT				&
#define TYPE_BITMAP_BV_RSLT				&
#define TYPE_MATRIX3_BV_RSLT			&
#define TYPE_ANGAXIS_BV_RSLT			&
#define TYPE_QUAT_BV_RSLT				&
#define TYPE_BITARRAY_BV_RSLT			&
#define TYPE_RAY_BV_RSLT				&
#define TYPE_POINT2_BV_RSLT				&
#define TYPE_MESH_BV_RSLT				&
#define TYPE_INTERVAL_BV_RSLT			&
#define TYPE_POINT_BV_RSLT				&
#define TYPE_TSTR_BV_RSLT				&
#define TYPE_COLOR_BV_RSLT				&
#define TYPE_FPVALUE_BV_RSLT			&
#define TYPE_CLASS_BV_RSLT				&

// by-val Tab<> 
#define TYPE_FLOAT_TAB_BV_RSLT			&
#define TYPE_INT_TAB_BV_RSLT			&
#define TYPE_RGBA_TAB_BV_RSLT			&
#define TYPE_POINT3_TAB_BV_RSLT			&
#define TYPE_BOOL_TAB_BV_RSLT			&
#define TYPE_ANGLE_TAB_BV_RSLT			&
#define TYPE_PCNT_FRAC_TAB_BV_RSLT		&
#define TYPE_WORLD_TAB_BV_RSLT			&
#define TYPE_STRING_TAB_BV_RSLT			&
#define TYPE_FILENAME_TAB_BV_RSLT		&
#define TYPE_HSV_TAB_BV_RSLT			&
#define TYPE_COLOR_CHANNEL_TAB_BV_RSLT	&
#define TYPE_TIMEVALUE_TAB_BV_RSLT		&
#define TYPE_RADIOBTN_INDEX_TAB_BV_RSLT &
#define TYPE_MTL_TAB_BV_RSLT			&
#define TYPE_TEXMAP_TAB_BV_RSLT			&
#define TYPE_BITMAP_TAB_BV_RSLT			&
#define TYPE_INODE_TAB_BV_RSLT			&
#define TYPE_REFTARG_TAB_BV_RSLT		&
#define TYPE_INDEX_TAB_BV_RSLT			&
#define TYPE_ENUM_TAB_BV_RSLT			&
#define TYPE_MATRIX3_TAB_BV_RSLT		&
#define TYPE_ANGAXIS_TAB_BV_RSLT		&
#define TYPE_QUAT_TAB_BV_RSLT			&
#define TYPE_BITARRAY_TAB_BV_RSLT		&
#define TYPE_CLASS_TAB_BV_RSLT			&
#define TYPE_RAY_TAB_BV_RSLT			&
#define TYPE_POINT2_TAB_BV_RSLT			&
#define TYPE_MESH_TAB_BV_RSLT			&
#define TYPE_OBJECT_TAB_BV_RSLT			&
#define TYPE_CONTROL_TAB_BV_RSLT		&
#define TYPE_INTERVAL_TAB_BV_RSLT		&
#define TYPE_POINT_TAB_BV_RSLT			&
#define TYPE_HWND_TAB_BV_RSLT			&
#define TYPE_TSTR_TAB_BV_RSLT			&
#define TYPE_IOBJECT_TAB_BV_RSLT		&
#define TYPE_INTERFACE_TAB_BV_RSLT		&
#define TYPE_NAME_TAB_BV_RSLT			&
#define TYPE_COLOR_TAB_BV_RSLT			&
#define TYPE_FPVALUE_TAB_BV_RSLT		&
#define TYPE_VALUE_TAB_BV_RSLT			&
#define TYPE_DWORD_TAB_BV_RSLT			&
#define TYPE_bool_TAB_BV_RSLT			&

//  types for each of the fields

#define TYPE_FLOAT_TYPE				float
#define TYPE_INT_TYPE				int
#define TYPE_RGBA_TYPE				Point3
#define TYPE_POINT3_TYPE			Point3
#define TYPE_BOOL_TYPE				BOOL
#define TYPE_ANGLE_TYPE				float
#define TYPE_PCNT_FRAC_TYPE			float
#define TYPE_WORLD_TYPE				float
#define TYPE_STRING_TYPE			TCHAR*
#define TYPE_FILENAME_TYPE			TCHAR*
#define TYPE_HSV_TYPE				Point3
#define TYPE_COLOR_CHANNEL_TYPE		float
#define TYPE_TIMEVALUE_TYPE			int
#define TYPE_RADIOBTN_INDEX_TYPE	int
#define TYPE_MTL_TYPE				Mtl*
#define TYPE_TEXMAP_TYPE			Texmap*
#define TYPE_BITMAP_TYPE			PBBitmap*
#define TYPE_INODE_TYPE				INode*
#define TYPE_REFTARG_TYPE			ReferenceTarget*	
#define TYPE_INDEX_TYPE				int
#define TYPE_ENUM_TYPE				int
#define TYPE_MATRIX3_TYPE			Matrix*
#define TYPE_VOID_TYPE				void
#define TYPE_INTERVAL_TYPE			Interval*
#define TYPE_ANGAXIS_TYPE			AngAxis*
#define TYPE_QUAT_TYPE				Quat*
#define TYPE_RAY_TYPE				Ray*
#define TYPE_POINT2_TYPE			Point2*
#define TYPE_BITARRAY_TYPE			BitArray*
#define TYPE_CLASS_TYPE				ClassID*
#define TYPE_MESH_TYPE				Mesh*
#define TYPE_OBJECT_TYPE			Object*
#define TYPE_CONTROL_TYPE			Control*
#define TYPE_POINT_TYPE				POINT*
#define TYPE_TSTR_TYPE				TSTR*
#define TYPE_IOBJECT_TYPE			IObject*
#define TYPE_INTERFACE_TYPE			FPInterface*
#define TYPE_HWND_TYPE				HWND
#define TYPE_NAME_TYPE				TCHAR*
#define TYPE_COLOR_TYPE				Color*
#define TYPE_FPVALUE_TYPE			FPValue*
#define TYPE_VALUE_TYPE				Value*
#define TYPE_DWORD_TYPE				DWORD
#define TYPE_bool_TYPE				bool

// Tab<>s of the above...
#define TYPE_FLOAT_TAB_TYPE				Tab<float>*
#define TYPE_INT_TAB_TYPE				Tab<int>*
#define TYPE_RGBA_TAB_TYPE				Tab<Point3>*
#define TYPE_POINT3_TAB_TYPE			Tab<Point3>*
#define TYPE_BOOL_TAB_TYPE				Tab<BOOL>*
#define TYPE_ANGLE_TAB_TYPE				Tab<float>*
#define TYPE_PCNT_FRAC_TAB_TYPE			Tab<float>*
#define TYPE_WORLD_TAB_TYPE				Tab<float>*
#define TYPE_STRING_TAB_TYPE			Tab<TCHAR*>*
#define TYPE_FILENAME_TAB_TYPE			Tab<TCHAR*>*
#define TYPE_HSV_TAB_TYPE				Tab<Point3>*
#define TYPE_COLOR_CHANNEL_TAB_TYPE		Tab<float>*
#define TYPE_TIMEVALUE_TAB_TYPE			Tab<int>*
#define TYPE_RADIOBTN_INDEX_TAB_TYPE	Tab<int>*
#define TYPE_MTL_TAB_TYPE				Tab<Mtl*>*
#define TYPE_TEXMAP_TAB_TYPE			Tab<Texmap*>*
#define TYPE_BITMAP_TAB_TYPE			Tab<PBBitmap*>*
#define TYPE_INODE_TAB_TYPE				Tab<INode*>*
#define TYPE_REFTARG_TAB_TYPE			Tab<ReferenceTarget*>*
#define TYPE_INDEX_TAB_TYPE				Tab<int>*
#define TYPE_ENUM_TAB_TYPE				Tab<int>*
#define TYPE_MATRIX3_TAB_TYPE			Tab<Matrix*>*
#define TYPE_VOID_TAB_TYPE				Tab<void>*
#define TYPE_INTERVAL_TAB_TYPE			Tab<Interval*>*
#define TYPE_ANGAXIS_TAB_TYPE			Tab<AngAxis*>*
#define TYPE_QUAT_TAB_TYPE				Tab<Quat*>*
#define TYPE_RAY_TAB_TYPE				Tab<Ray*>*
#define TYPE_POINT2_TAB_TYPE			Tab<Point2*>*
#define TYPE_BITARRAY_TAB_TYPE			Tab<BitArray*>*
#define TYPE_CLASS_TAB_TYPE				Tab<ClassID*>*
#define TYPE_MESH_TAB_TYPE				Tab<Mesh*>*
#define TYPE_OBJECT_TAB_TYPE			Tab<Object*>*
#define TYPE_CONTROL_TAB_TYPE			Tab<Control*>*
#define TYPE_POINT_TAB_TYPE				Tab<POINT*>*
#define TYPE_TSTR_TAB_TYPE				Tab<TSTR*>*
#define TYPE_IOBJECT_TAB_TYPE			Tab<IObject*>*
#define TYPE_INTERFACE_TAB_TYPE			Tab<FPInterface*>*
#define TYPE_HWND_TAB_TYPE				Tab<HWND>*
#define TYPE_NAME_TAB_TYPE				Tab<TCHAR*>*
#define TYPE_COLOR_TAB_TYPE				Tab<Color*>*
#define TYPE_FPVALUE_TAB_TYPE			Tab<FPValue*>*
#define TYPE_VALUE_TAB_TYPE				Tab<Value*>*
#define TYPE_DWORD_TAB_TYPE				Tab<DWORD>*
#define TYPE_bool_TAB_TYPE				Tab<bool>*

// by-pointer
//  foo*  = 
#define TYPE_FLOAT_BP_TYPE				float*
#define TYPE_INT_BP_TYPE				int*
#define TYPE_BOOL_BP_TYPE				int*
#define TYPE_ANGLE_BP_TYPE				float*
#define TYPE_PCNT_FRAC_BP_TYPE			float*
#define TYPE_WORLD_BP_TYPE				float*
#define TYPE_COLOR_CHANNEL_BP_TYPE		float*
#define TYPE_TIMEVALUE_BP_TYPE			int*
#define TYPE_RADIOBTN_INDEX_BP_TYPE		int*
#define TYPE_INDEX_BP_TYPE				int*
#define TYPE_ENUM_BP_TYPE				int*
#define TYPE_DWORD_BP_TYPE				DWORD*
#define TYPE_bool_BP_TYPE				bool*

// by-reference 
#define TYPE_FLOAT_BR_TYPE				float&
#define TYPE_INT_BR_TYPE				int&
#define TYPE_RGBA_BR_TYPE				Point3&
#define TYPE_POINT3_BR_TYPE				Point3&
#define TYPE_BOOL_BR_TYPE				int&
#define TYPE_ANGLE_BR_TYPE				float&
#define TYPE_PCNT_FRAC_BR_TYPE			float&
#define TYPE_WORLD_BR_TYPE				float&
#define TYPE_HSV_BR_TYPE				Point3&
#define TYPE_COLOR_CHANNEL_BR_TYPE		float&
#define TYPE_TIMEVALUE_BR_TYPE			int&
#define TYPE_RADIOBTN_INDEX_BR_TYPE		int&
#define TYPE_BITMAP_BR_TYPE				PBBitmap&
#define TYPE_INDEX_BR_TYPE				int&
#define TYPE_ENUM_BR_TYPE				int&
#define TYPE_REFTARG_BR_TYPE			ReferenceTarget&
#define TYPE_MATRIX3_BR_TYPE			Matrix3&
#define TYPE_ANGAXIS_BR_TYPE			AngAxis&
#define TYPE_QUAT_BR_TYPE				Quat&
#define TYPE_BITARRAY_BR_TYPE			BitArray&
#define TYPE_RAY_BR_TYPE				Ray&
#define TYPE_POINT2_BR_TYPE				Point2&
#define TYPE_MESH_BR_TYPE				Mesh&
#define TYPE_INTERVAL_BR_TYPE			Interval&
#define TYPE_POINT_BR_TYPE				POINT&
#define TYPE_TSTR_BR_TYPE				TSTR&
#define TYPE_COLOR_BR_TYPE				Color&
#define TYPE_FPVALUE_BR_TYPE			FPValue&
#define TYPE_DWORD_BR_TYPE				DWORD&
#define TYPE_bool_BR_TYPE				bool&

// Tab<> by-reference 
#define TYPE_FLOAT_TAB_BR_TYPE				Tab<float>&
#define TYPE_INT_TAB_BR_TYPE				Tab<int>&
#define TYPE_RGBA_TAB_BR_TYPE				Tab<Point3>&
#define TYPE_POINT3_TAB_BR_TYPE				Tab<Point3>&
#define TYPE_BOOL_TAB_BR_TYPE				Tab<BOOL>&
#define TYPE_ANGLE_TAB_BR_TYPE				Tab<float>&
#define TYPE_PCNT_FRAC_TAB_BR_TYPE			Tab<float>&
#define TYPE_WORLD_TAB_BR_TYPE				Tab<float>&
#define TYPE_STRING_TAB_BR_TYPE				Tab<TCHAR*>&
#define TYPE_FILENAME_TAB_BR_TYPE			Tab<TCHAR*>&
#define TYPE_HSV_TAB_BR_TYPE				Tab<Point3>&
#define TYPE_COLOR_CHANNEL_TAB_BR_TYPE		Tab<float>&
#define TYPE_TIMEVALUE_TAB_BR_TYPE			Tab<int>&
#define TYPE_RADIOBTN_INDEX_TAB_BR_TYPE		Tab<int>&
#define TYPE_MTL_TAB_BR_TYPE				Tab<Mtl*>&
#define TYPE_TEXMAP_TAB_BR_TYPE				Tab<Texmap*>&
#define TYPE_BITMAP_TAB_BR_TYPE				Tab<PBBitmap*>&
#define TYPE_INODE_TAB_BR_TYPE				Tab<INode*>&
#define TYPE_REFTARG_TAB_BR_TYPE			Tab<ReferenceTarget*>&
#define TYPE_INDEX_TAB_BR_TYPE				Tab<int>&
#define TYPE_ENUM_TAB_BR_TYPE				Tab<int>&
#define TYPE_MATRIX3_TAB_BR_TYPE			Tab<Matrix*>&
#define TYPE_VOID_TAB_BR_TYPE				Tab<void>&
#define TYPE_INTERVAL_TAB_BR_TYPE			Tab<Interval*>&
#define TYPE_ANGAXIS_TAB_BR_TYPE			Tab<AngAxis*>&
#define TYPE_QUAT_TAB_BR_TYPE				Tab<Quat*>&
#define TYPE_RAY_TAB_BR_TYPE				Tab<Ray*>&
#define TYPE_POINT2_TAB_BR_TYPE				Tab<Point2*>&
#define TYPE_BITARRAY_TAB_BR_TYPE			Tab<BitArray*>&
#define TYPE_CLASS_TAB_BR_TYPE				Tab<ClassID*>&
#define TYPE_MESH_TAB_BR_TYPE				Tab<Mesh*>&
#define TYPE_OBJECT_TAB_BR_TYPE				Tab<Object*>&
#define TYPE_CONTROL_TAB_BR_TYPE			Tab<Control*>&
#define TYPE_POINT_TAB_BR_TYPE				Tab<POINT*>&
#define TYPE_TSTR_TAB_BR_TYPE				Tab<TSTR*>&
#define TYPE_IOBJECT_TAB_BR_TYPE			Tab<IObject*>&
#define TYPE_INTERFACE_TAB_BR_TYPE			Tab<FPInterface*>&
#define TYPE_HWND_TAB_BR_TYPE				Tab<HWND>&
#define TYPE_NAME_TAB_BR_TYPE				Tab<TCHAR*>&
#define TYPE_COLOR_TAB_BR_TYPE				Tab<Color*>&
#define TYPE_FPVALUE_TAB_BR_TYPE			Tab<FPValue*>&
#define TYPE_VALUE_TAB_BR_TYPE				Tab<Value*>&
#define TYPE_DWORD_TAB_BR_TYPE				Tab<DWORD>&
#define TYPE_bool_TAB_BR_TYPE				Tab<bool>&

// by-value 
#define TYPE_RGBA_BV_TYPE				Point3		
#define TYPE_POINT3_BV_TYPE				Point3
#define TYPE_HSV_BV_TYPE				Point3
#define TYPE_BITMAP_BV_TYPE				PBBitmap
#define TYPE_MATRIX3_BV_TYPE			Matrix3
#define TYPE_ANGAXIS_BV_TYPE			AngAxis
#define TYPE_QUAT_BV_TYPE				Quat
#define TYPE_BITARRAY_BV_TYPE			BitArray
#define TYPE_RAY_BV_TYPE				Ray
#define TYPE_POINT2_BV_TYPE				Point2
#define TYPE_MESH_BV_TYPE				Mesh
#define TYPE_INTERVAL_BV_TYPE			Interval
#define TYPE_POINT_BV_TYPE				POINT
#define TYPE_TSTR_BV_TYPE				TSTR
#define TYPE_COLOR_BV_TYPE				Color
#define TYPE_FPVALUE_BV_TYPE			FPValue
#define TYPE_CLASS_BV_TYPE				ClassID

// by-val Tab<> 
#define TYPE_FLOAT_TAB_BV_TYPE				Tab<float>
#define TYPE_INT_TAB_BV_TYPE				Tab<int>
#define TYPE_RGBA_TAB_BV_TYPE				Tab<Point3>
#define TYPE_POINT3_TAB_BV_TYPE				Tab<Point3>
#define TYPE_BOOL_TAB_BV_TYPE				Tab<BOOL>
#define TYPE_ANGLE_TAB_BV_TYPE				Tab<float>
#define TYPE_PCNT_FRAC_TAB_BV_TYPE			Tab<float>
#define TYPE_WORLD_TAB_BV_TYPE				Tab<float>
#define TYPE_STRING_TAB_BV_TYPE				Tab<TCHAR*>
#define TYPE_FILENAME_TAB_BV_TYPE			Tab<TCHAR*>
#define TYPE_HSV_TAB_BV_TYPE				Tab<Point3>
#define TYPE_COLOR_CHANNEL_TAB_BV_TYPE		Tab<float>
#define TYPE_TIMEVALUE_TAB_BV_TYPE			Tab<int>
#define TYPE_RADIOBTN_INDEX_TAB_BV_TYPE		Tab<int>
#define TYPE_MTL_TAB_BV_TYPE				Tab<Mtl*>
#define TYPE_TEXMAP_TAB_BV_TYPE				Tab<Texmap*>
#define TYPE_BITMAP_TAB_BV_TYPE				Tab<PBBitmap*>
#define TYPE_INODE_TAB_BV_TYPE				Tab<INode*>
#define TYPE_REFTARG_TAB_BV_TYPE			Tab<ReferenceTarget*>
#define TYPE_INDEX_TAB_BV_TYPE				Tab<int>
#define TYPE_ENUM_TAB_BV_TYPE				Tab<int>
#define TYPE_MATRIX3_TAB_BV_TYPE			Tab<Matrix*>
#define TYPE_VOID_TAB_BV_TYPE				Tab<void>
#define TYPE_INTERVAL_TAB_BV_TYPE			Tab<Interval*>
#define TYPE_ANGAXIS_TAB_BV_TYPE			Tab<AngAxis*>
#define TYPE_QUAT_TAB_BV_TYPE				Tab<Quat*>
#define TYPE_RAY_TAB_BV_TYPE				Tab<Ray*>
#define TYPE_POINT2_TAB_BV_TYPE				Tab<Point2*>
#define TYPE_BITARRAY_TAB_BV_TYPE			Tab<BitArray*>
#define TYPE_CLASS_TAB_BV_TYPE				Tab<ClassID*>
#define TYPE_MESH_TAB_BV_TYPE				Tab<Mesh*>
#define TYPE_OBJECT_TAB_BV_TYPE				Tab<Object*>
#define TYPE_CONTROL_TAB_BV_TYPE			Tab<Control*>
#define TYPE_POINT_TAB_BV_TYPE				Tab<POINT*>
#define TYPE_TSTR_TAB_BV_TYPE				Tab<TSTR*>
#define TYPE_IOBJECT_TAB_BV_TYPE			Tab<IObject*>
#define TYPE_INTERFACE_TAB_BV_TYPE			Tab<FPInterface*>
#define TYPE_HWND_TAB_BV_TYPE				Tab<HWND>
#define TYPE_NAME_TAB_BV_TYPE				Tab<TCHAR*>
#define TYPE_COLOR_TAB_BV_TYPE				Tab<Color*>
#define TYPE_FPVALUE_TAB_BV_TYPE			Tab<FPValue*>
#define TYPE_VALUE_TAB_BV_TYPE				Tab<Value*>
#define TYPE_DWORD_TAB_BV_TYPE				Tab<DWORD>
#define TYPE_bool_TAB_BV_TYPE				Tab<bool>

#endif //__IFNPUB__




