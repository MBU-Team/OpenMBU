/*	
 *		macrorec.h - Interface to MAXScript macro recorder for MAX
 *
 *	The macro recorder 					
 *
 *			Copyright © Autodesk, Inc, 1998.  John Wainwright.
 *
 */

#ifndef _H_MACRORECORD
#define _H_MACRORECORD

#ifndef ScripterExport
#	ifdef BLD_MAXSCRIPT
#		define ScripterExport __declspec( dllexport )
#	else
#		define ScripterExport __declspec( dllimport )
#	endif
#endif

class ParamBlock2;

class MacroRecorder
{
public:
	// script constructors
	virtual BOOL BeginCreate(ClassDesc* cd, int flags = 0) = 0;
	virtual void SetNodeTM(INode* n, Matrix3 m) = 0;
	virtual void ParamBlockSetValue(ParamBlock* pb, int i, BYTE type, ...) = 0;
	virtual void ParamBlock2SetValue(IParamBlock2* pb, int i, int tabIndex, BYTE type, ...) = 0;
	virtual void ParamBlock2SetCount(IParamBlock2* pb, int i, int n) = 0;
	virtual void SetProperty(ReferenceTarget* targ, TCHAR* prop_name, BYTE type, ...) = 0;
	virtual void SetCopy(ReferenceTarget* to_copy) = 0;
	virtual void SetSelProperty(TCHAR* prop_name, BYTE type, ...) = 0;
	virtual void FunctionCall(TCHAR* op_name, int arg_count, int keyarg_count, ...) = 0;
	virtual void ScriptString(TCHAR* s) = 0;
	virtual void Assign(TCHAR* var_name, BYTE type, ...) = 0;
	virtual void Assign(BYTE type, ...) = 0;
	virtual void OpAssign(TCHAR* op, BYTE type, ...) = 0;
	virtual void OperandSequence(int count, BYTE type, ...) = 0;
	virtual BOOL BeginSelectNode() = 0;
	virtual void Select(INode*) = 0;
	virtual void DeSelect(INode*) = 0;
	virtual void MAXCommand(int com) = 0;
	virtual void Cancel() = 0;
	virtual void EmitScript() = 0;
	// scripter info extractors
	virtual TSTR GetSubMtlPropName(Mtl* m, int i) = 0;
	virtual TSTR GetSubTexmapPropName(MtlBase* m, int i) = 0;
	// nestable disable/enable
	virtual void Enable() = 0;
	virtual void Disable() = 0;
	virtual BOOL Enabled() = 0;
	// master enable and option controls
	virtual BOOL MasterEnable() = 0;
	virtual void MasterEnable(BOOL onOff) = 0;
	virtual BOOL ShowCommandPanelSwitch() = 0;
	virtual void ShowCommandPanelSwitch(BOOL onOff) = 0;
	virtual BOOL ShowToolSelections() = 0;
	virtual void ShowToolSelections(BOOL onOff) = 0;
	virtual BOOL ShowMenuSelections() = 0;
	virtual void ShowMenuSelections(BOOL onOff) = 0;
	virtual BOOL EmitAbsoluteSceneNames() = 0;
	virtual void EmitAbsoluteSceneNames(BOOL onOff) = 0;
	virtual BOOL EmitAbsoluteSubObjects() = 0;
	virtual void EmitAbsoluteSubObjects(BOOL onOff) = 0;
	virtual BOOL EmitAbsoluteTransforms() = 0;
	virtual void EmitAbsoluteTransforms(BOOL onOff) = 0;
//	virtual BOOL EmitExplicitCoordinates() = 0;   // deferred in Shiva
//	virtual void EmitExplicitCoordinates(BOOL onOff) = 0;
};
   
#if defined(BLD_CORE) || defined(BLD_PARAMBLK2)
	extern MacroRecorder *macroRecorder;
#else
	extern ScripterExport MacroRecorder *macroRecorder;
#endif

extern ScripterExport void InitMacroRecorder();

// value types:
enum { mr_int, mr_float, mr_string, mr_bool,			// basic C types
	   mr_point3, mr_color, mr_angaxis, mr_quat,		// MAX SDK types...
	   mr_time, mr_reftarg, mr_bitarray, mr_pbbitmap,
	   mr_matrix3, mr_nurbssel, mr_meshselbits, 
	   mr_meshsel, mr_subanim, mr_animatable,
	   mr_classid, mr_nurbsselbits, 
	   mr_sel, mr_funcall, mr_varname, mr_create,		// MAXScript types
	   mr_angle, mr_percent, mr_index, mr_prop,
	   mr_name, 
	   mr_dimfloat, mr_dimpoint3,						// explicitly dimensioned float & point
	};

#define macroRec GetCOREInterface()->GetMacroRecorder()

#endif
