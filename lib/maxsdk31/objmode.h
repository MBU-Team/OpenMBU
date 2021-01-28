/**********************************************************************
 *<
	FILE: objmode.h

	DESCRIPTION: Provides some standard modes for subobject manipulation

	CREATED BY: Rolf Berteig

	HISTORY: Created 3-14-95

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef __OBJMODE__
#define __OBJMODE__


class Transformer {
	protected:
		Matrix3 tmAxis;
		IObjParam *ip;
		IPoint2 mBase, mCur;
		
	public:
		Transformer(IObjParam *i) { ip = i; }
		CoreExport virtual void BeginDrag( IPoint2& m, Matrix3& tmAxis );
		CoreExport virtual void EndDrag( IPoint2& m );
		CoreExport void SetMouse( IPoint2& m );
#ifdef _OSNAP
		CoreExport virtual void SnapPreview(ViewExp *vpt, IPoint2 in, IPoint2 out, Matrix3 *m , DWORD flags){};
#endif
		virtual void Accumulate() {mBase=mCur;}
		Matrix3& Axis() { return tmAxis; }
		IPoint2 Base() { return mBase; }
	};

class MoveTransformer : public Transformer {
 		Point3 lastDelta;
		Point3 absSnapOrigin;
		BOOL selfSnap;
 	public:
 		void SetSelfSnap(BOOL ss) {selfSnap=ss;}
		BOOL GetSelfSnap() {return selfSnap;}
		CoreExport void BeginDrag( IPoint2& m, Matrix3& tmAxis );
 		CoreExport Point3 Eval(ViewExp *vpt,BOOL status=TRUE);
#ifdef DESIGN_VER
		CoreExport virtual void EndDrag( IPoint2& m );
#endif
		MoveTransformer(IObjParam *i,BOOL so=FALSE) : Transformer(i) {lastDelta = Point3(0,0,0); selfSnap=so;}
		CoreExport void Accumulate();
#ifdef _OSNAP
		CoreExport void SnapPreview(ViewExp *vpt, IPoint2 in, IPoint2 out, Matrix3 *m , DWORD flags);
#endif
 		};	

class RotateTransformer : public Transformer {
 		Quat qPrev;
 	public:
 		CoreExport AngAxis Eval(ViewExp *vpt,BOOL status=TRUE);
		CoreExport void BeginDrag( IPoint2& m, Matrix3& tmAxis );
		RotateTransformer(IObjParam *i) : Transformer(i) {qPrev.Identity();}
#ifdef _OSNAP
		CoreExport void SnapPreview(ViewExp *vpt, IPoint2 in, IPoint2 out, Matrix3 *m , DWORD flags);
#endif
 		};	

class ScaleTransformer : public Transformer {
 	public:
 		CoreExport Point3 Eval(ViewExp *vpt,BOOL status=TRUE);
		ScaleTransformer(IObjParam *i) : Transformer(i) {}
#ifdef _OSNAP
		CoreExport void BeginDrag( IPoint2& m, Matrix3& tmAxis );
		CoreExport void SnapPreview(ViewExp *vpt, IPoint2 in, IPoint2 out, Matrix3 *m , DWORD flags);
#endif
 		};	


class ChangeFGObject : public ChangeForegroundCallback {	
		ReferenceTarget *obj;
		BOOL valid;
	public:		
		ChangeFGObject() { obj = NULL; valid = TRUE; }
		ChangeFGObject(ReferenceTarget *o) { obj = o; valid = TRUE; }
		void SetObj(ReferenceTarget *o ) { obj = o; }
		
		BOOL IsValid() { return valid; }
		void Invalidate() { valid = FALSE; }
		void Validate() { valid = TRUE; }
		void callback(TimeValue t,IScene *scene)
			{
			obj->FlagDependents(t);			
			}
	};



class SelectionProcessor : public MouseCallBack {
	private:
		MouseCallBack *mcallback;
		BOOL brokenThresh, hitSel, drag, toggle, cloning, clear, invert;
		IPoint2 offset;
		IPoint2 om,lm;

	protected:
		IObjParam *ip;

		virtual BOOL HitTest( ViewExp *vpt, IPoint2 *p, int type, int flags )=0;
		virtual BOOL AnyHits( ViewExp *vpt )=0;
		virtual HCURSOR GetTransformCursor()=0;
		virtual void Select(ViewExp *vpt,BOOL all,BOOL clear,BOOL invert)=0;
		virtual void DeSelect(ViewExp *vpt,BOOL all)=0;
		virtual void ClearSelection()=0;
		virtual void CloneSelected(int initialCall=TRUE)=0;
		virtual void AbortClone()=0;
		virtual void AcceptClone()=0;
		virtual void SelectChildren(ViewExp *vpt) {}

	public:
		SelectionProcessor(MouseCallBack *m,IObjParam *i) 
			{ mcallback = m; ip = i; offset = IPoint2(0,0); }
		
		CoreExport virtual int proc( 
			HWND hwnd, 
			int msg, 
			int point, 
			int flags, 
			IPoint2 m );

		CoreExport void pan(IPoint2 d);
	};


class TransformModBox : public MouseCallBack {
	protected:
		BaseObject *obj;
		IObjParam *ip;		
		Matrix3 ptm;

	public:
		CoreExport TransformModBox(BaseObject *o, IObjParam *i);
		CoreExport ~TransformModBox();

		// These are called before and after the DoTransform operations
		CoreExport virtual void PreTransform();
		CoreExport virtual void PreTransformHolding();
		CoreExport virtual void PostTransformHolding();
		CoreExport virtual void PostTransform();
		CoreExport virtual void CancelTransform();

		virtual Transformer& GetTransformer()=0;
		virtual void DoTransform(ViewExp *vpt)=0;
		virtual HCURSOR GetTransformCursor()=0;
		virtual int UndoStringID()=0;

		CoreExport int proc( 
			HWND hwnd, 
			int msg, 
			int point, 
			int flags, 
			IPoint2 m );
	};


class GenModSelectionProcessor : public SelectionProcessor {
	protected:
		BaseObject *obj;
		BOOL transformGizmoActive;

		CoreExport BOOL HitTest( ViewExp *vpt, IPoint2 *p, int type, int flags );
		BOOL AnyHits( ViewExp *vpt ) { return vpt->NumSubObjHits(); }		
		CoreExport void Select(ViewExp *vpt,BOOL all,BOOL clear,BOOL invert);
		CoreExport void DeSelect(ViewExp *vpt,BOOL all);
		CoreExport void ClearSelection();
		CoreExport void CloneSelected(int initialCall=TRUE);
		CoreExport void AbortClone();
		CoreExport void AcceptClone();

	public:
		GenModSelectionProcessor(MouseCallBack *mc, BaseObject *o, IObjParam *i) 
			: SelectionProcessor(mc,i) {obj = o; transformGizmoActive=FALSE; }
	};

class SubModSelectionProcessor : public GenModSelectionProcessor {
	private:		
		TransformModBox *tproc;
		BOOL supportTransformGizmo;

	protected:
		HCURSOR GetTransformCursor() { return tproc ? tproc->GetTransformCursor() : LoadCursor(NULL, IDC_ARROW); }
		
	public:
		SubModSelectionProcessor(TransformModBox *mc, BaseObject *o, IObjParam *i) 
			: GenModSelectionProcessor(mc,o,i) { 
				tproc = mc; supportTransformGizmo=FALSE; }

		// Transform Gizmo Interface
		BOOL SupportTransformGizmo() { return supportTransformGizmo; }
		void DeactivateTransformGizmo() {
			if (transformGizmoActive) {
				ip->DeactivateTransformGizmo();
				transformGizmoActive = FALSE;
				}
			}
		// End of Transform Gizmo Interface
		// Private gizmo stuff
		void SetSupportTransformGizmo(BOOL b) { supportTransformGizmo = b; }
	};


class MoveModBox : public TransformModBox {
	private:
		MoveTransformer moveTrans;
	public:
		MoveModBox(BaseObject *o, IObjParam *i) : moveTrans(i,TRUE), TransformModBox(o,i) {}
		Transformer& GetTransformer() { return moveTrans; }
		CoreExport void DoTransform(ViewExp *vpt);
		HCURSOR GetTransformCursor() { return ip->GetSysCursor(SYSCUR_MOVE); }
		CoreExport int UndoStringID();
		};
class RotateModBox : public TransformModBox {
	private:
		RotateTransformer rotTrans;
	public:
		RotateModBox(BaseObject *o, IObjParam *i) : rotTrans(i), TransformModBox(o,i) {}
		Transformer& GetTransformer() { return rotTrans; }
		CoreExport void DoTransform(ViewExp *vpt);
		HCURSOR GetTransformCursor() { return ip->GetSysCursor(SYSCUR_ROTATE); }
		CoreExport int UndoStringID();
		};
class ScaleModBox : public TransformModBox {
	private:
		ScaleTransformer scaleTrans;
	public:
		ScaleModBox(BaseObject *o, IObjParam *i) : scaleTrans(i), TransformModBox(o,i) {}
		Transformer& GetTransformer() { return scaleTrans; }
		CoreExport void DoTransform(ViewExp *vpt);
		CoreExport HCURSOR GetTransformCursor();
		CoreExport int UndoStringID();
		};
class SelectModBox : public TransformModBox {
	private:
		MoveTransformer moveTrans;
	public:
		SelectModBox(BaseObject *o, IObjParam *i) : moveTrans(i), TransformModBox(o,i) {}
		Transformer& GetTransformer() { return moveTrans; }
		void DoTransform(ViewExp *vpt) {}
		HCURSOR GetTransformCursor() { return ip->GetSysCursor(SYSCUR_SELECT); }
		CoreExport int UndoStringID();
		};


class MoveModBoxCMode : public CommandMode {
	
	private:
		ChangeFGObject fgProc;
		SubModSelectionProcessor mouseProc;
		MoveModBox transProc;
		IObjParam *ip;

	public:
		MoveModBoxCMode( BaseObject *o, IObjParam *i ) : 
			fgProc(o), transProc(o,i), mouseProc(&transProc,o,i) 
				{ ip = i; mouseProc.SetSupportTransformGizmo(TRUE); }
		
		int Class() { return MOVE_COMMAND; }
		int ID() { return CID_SUBOBJMOVE; }
		MouseCallBack *MouseProc(int *numPoints) { *numPoints=2; return &mouseProc; }
		ChangeForegroundCallback *ChangeFGProc() { return &fgProc; }
		BOOL ChangeFG( CommandMode *oldMode ) { return oldMode->ChangeFGProc() != &fgProc; }
		void EnterMode() { ip->SetToolButtonState(MOVE_BUTTON,TRUE); }
		void ExitMode() { ip->SetToolButtonState(MOVE_BUTTON,FALSE); }
	};

class RotateModBoxCMode : public CommandMode {
	
	private:
		ChangeFGObject fgProc;
		SubModSelectionProcessor mouseProc;
		RotateModBox transProc;
		IObjParam *ip;

	public:
		RotateModBoxCMode( BaseObject *o, IObjParam *i ) : 
			fgProc(o), transProc(o,i), mouseProc(&transProc,o,i) 
				{ ip = i; mouseProc.SetSupportTransformGizmo(TRUE); }
		
		int Class() { return ROTATE_COMMAND; }
		int ID() { return CID_SUBOBJROTATE; }
		MouseCallBack *MouseProc(int *numPoints) { *numPoints=2; return &mouseProc; }
		ChangeForegroundCallback *ChangeFGProc() { return &fgProc; }
		BOOL ChangeFG( CommandMode *oldMode ) { return oldMode->ChangeFGProc() != &fgProc; }
		void EnterMode() { ip->SetToolButtonState(ROTATE_BUTTON,TRUE); }
		void ExitMode() { ip->SetToolButtonState(ROTATE_BUTTON,FALSE); }
	};

class UScaleModBoxCMode : public CommandMode {
	
	private:
		ChangeFGObject fgProc;
		SubModSelectionProcessor mouseProc;
		ScaleModBox transProc;
		IObjParam *ip;

	public:
		UScaleModBoxCMode( BaseObject *o, IObjParam *i ) : 
			fgProc(o), transProc(o,i), mouseProc(&transProc,o,i) 
				{ ip = i; mouseProc.SetSupportTransformGizmo(TRUE); }
		
		int Class() { return USCALE_COMMAND; }
		int ID() { return CID_SUBOBJUSCALE; }
		MouseCallBack *MouseProc(int *numPoints) { *numPoints=2; return &mouseProc; }
		ChangeForegroundCallback *ChangeFGProc() { return &fgProc; }
		BOOL ChangeFG( CommandMode *oldMode ) { return oldMode->ChangeFGProc() != &fgProc; }
		void EnterMode() { ip->SetToolButtonState(USCALE_BUTTON,TRUE); }
		void ExitMode() { ip->SetToolButtonState(USCALE_BUTTON,FALSE); }
	};

class NUScaleModBoxCMode : public CommandMode {
	
	private:
		ChangeFGObject fgProc;
		SubModSelectionProcessor mouseProc;
		ScaleModBox transProc;
		IObjParam *ip;

	public:
		NUScaleModBoxCMode( BaseObject *o, IObjParam *i ) : 
			fgProc(o), transProc(o,i), mouseProc(&transProc,o,i) 
				{ ip = i; mouseProc.SetSupportTransformGizmo(TRUE); }
		
		int Class() { return SCALE_COMMAND; }
		int ID() { return CID_SUBOBJSCALE; }
		MouseCallBack *MouseProc(int *numPoints) { *numPoints=2; return &mouseProc; }
		ChangeForegroundCallback *ChangeFGProc() { return &fgProc; }
		BOOL ChangeFG( CommandMode *oldMode ) { return oldMode->ChangeFGProc() != &fgProc; }
		void EnterMode() { ip->SetToolButtonState(NUSCALE_BUTTON,TRUE); }
		void ExitMode() { ip->SetToolButtonState(NUSCALE_BUTTON,FALSE); }
	};

class SquashModBoxCMode : public CommandMode {
	
	private:
		ChangeFGObject fgProc;
		SubModSelectionProcessor mouseProc;
		ScaleModBox transProc;
		IObjParam *ip;

	public:
		SquashModBoxCMode( BaseObject *o, IObjParam *i ) : 
			fgProc(o), transProc(o,i), mouseProc(&transProc,o,i) 
				{ ip = i; mouseProc.SetSupportTransformGizmo(TRUE); }
		
		int Class() { return SQUASH_COMMAND; }
		int ID() { return CID_SUBOBJSQUASH; }
		MouseCallBack *MouseProc(int *numPoints) { *numPoints=2; return &mouseProc; }
		ChangeForegroundCallback *ChangeFGProc() { return &fgProc; }
		BOOL ChangeFG( CommandMode *oldMode ) { return oldMode->ChangeFGProc() != &fgProc; }
		void EnterMode() { ip->SetToolButtonState(SQUASH_BUTTON,TRUE); }
		void ExitMode() { ip->SetToolButtonState(SQUASH_BUTTON,FALSE); }
	};

class SelectModBoxCMode : public CommandMode {
	
	private:
		ChangeFGObject fgProc;
		SubModSelectionProcessor mouseProc;
		SelectModBox transProc;
		IObjParam *ip;

	public:
		SelectModBoxCMode( BaseObject *o, IObjParam *i ) : 
			fgProc(o), transProc(o,i), mouseProc(NULL/*&transProc*/,o,i) 
				{ ip = i; }
		
		int Class() { return SELECT_COMMAND; }
		int ID() { return CID_SUBOBJSELECT; }
		MouseCallBack *MouseProc(int *numPoints) { *numPoints=2; return &mouseProc; }
		ChangeForegroundCallback *ChangeFGProc() { return &fgProc; }
		BOOL ChangeFG( CommandMode *oldMode ) { return oldMode->ChangeFGProc() != &fgProc; }
		void EnterMode() { ip->SetToolButtonState(SELECT_BUTTON,TRUE); }
		void ExitMode() { ip->SetToolButtonState(SELECT_BUTTON,FALSE); }
	};



///////////////////////////////////////////////////////////////////////////

class TransformCtrlApparatus : public MouseCallBack {
	protected:
		Control *ctrl;
		IObjParam *ip;		
		Matrix3 ptm;

	public:
		CoreExport TransformCtrlApparatus(Control *c, IObjParam *i);
		CoreExport ~TransformCtrlApparatus();

		virtual Transformer& GetTransformer()=0;
		virtual void DoTransform(ViewExp *vpt)=0;
		virtual HCURSOR GetTransformCursor()=0;
		virtual int UndoStringID()=0;

		CoreExport int proc( 
			HWND hwnd, 
			int msg, 
			int point, 
			int flags, 
			IPoint2 m );
	};


class GenControlSelectionProcessor : public SelectionProcessor {
	protected:
		Control *ctrl;
		BOOL transformGizmoActive;

		CoreExport BOOL HitTest( ViewExp *vpt, IPoint2 *p, int type, int flags );
		BOOL AnyHits( ViewExp *vpt ) { return vpt->NumSubObjHits(); }		
		CoreExport void Select(ViewExp *vpt,BOOL all,BOOL clear,BOOL invert);
		CoreExport void DeSelect(ViewExp *vpt,BOOL all);
		CoreExport void ClearSelection();
		void CloneSelected(int initialCall=TRUE) {};
		void AbortClone() {};
		void AcceptClone() {};

	public:
		GenControlSelectionProcessor(MouseCallBack *mc,Control *c,IObjParam *i) 
			: SelectionProcessor(mc,i) {ctrl=c; transformGizmoActive=FALSE; }
	};

class SubControlSelectionProcessor : public GenControlSelectionProcessor {
	private:		
		TransformCtrlApparatus *tproc;
		BOOL supportTransformGizmo;

	protected:
		HCURSOR GetTransformCursor() { return tproc->GetTransformCursor(); }
		
	public:
		SubControlSelectionProcessor(TransformCtrlApparatus *tc,Control *c,IObjParam *i) 
			: GenControlSelectionProcessor(tc,c,i) { 
				tproc = tc; supportTransformGizmo=FALSE; }

		// Transform Gizmo Interface
		BOOL SupportTransformGizmo() { return supportTransformGizmo; }
		void DeactivateTransformGizmo() {
			if (transformGizmoActive) {
				ip->DeactivateTransformGizmo();
				transformGizmoActive = FALSE;
				}
			}
		// End of Transform Gizmo Interface
		// Private gizmo stuff
		void SetSupportTransformGizmo(BOOL b) { supportTransformGizmo = b; }

	};


class MoveCtrlApparatus : public TransformCtrlApparatus {
	private:
		MoveTransformer moveTrans;
	public:
		MoveCtrlApparatus(Control *c, IObjParam *i) : moveTrans(i,TRUE), TransformCtrlApparatus(c,i) {}
		Transformer& GetTransformer() { return moveTrans; }
		CoreExport void DoTransform(ViewExp *vpt);
		HCURSOR GetTransformCursor() { return ip->GetSysCursor(SYSCUR_MOVE); }
		CoreExport int UndoStringID();
		};
class RotateCtrlApparatus : public TransformCtrlApparatus {
	private:
		RotateTransformer rotTrans;
	public:
		RotateCtrlApparatus(Control *c, IObjParam *i) : rotTrans(i), TransformCtrlApparatus(c,i) {}
		Transformer& GetTransformer() { return rotTrans; }
		CoreExport void DoTransform(ViewExp *vpt);
		HCURSOR GetTransformCursor() { return ip->GetSysCursor(SYSCUR_ROTATE); }
		CoreExport int UndoStringID();
		};
class ScaleCtrlApparatus : public TransformCtrlApparatus {
	private:
		ScaleTransformer scaleTrans;
	public:
		ScaleCtrlApparatus(Control *c, IObjParam *i) : scaleTrans(i), TransformCtrlApparatus(c,i) {}
		Transformer& GetTransformer() { return scaleTrans; }
		CoreExport void DoTransform(ViewExp *vpt);
		CoreExport HCURSOR GetTransformCursor();
		CoreExport int UndoStringID();
		};
class SelectCtrlApparatus : public TransformCtrlApparatus {
	private:
		MoveTransformer moveTrans;
	public:
		SelectCtrlApparatus(Control *c, IObjParam *i) : moveTrans(i), TransformCtrlApparatus(c,i) {}
		Transformer& GetTransformer() { return moveTrans; }
		void DoTransform(ViewExp *vpt) {}
		HCURSOR GetTransformCursor() { return ip->GetSysCursor(SYSCUR_SELECT); }
		CoreExport int UndoStringID();
		};


class MoveCtrlApparatusCMode : public CommandMode {
	
	private:
		ChangeFGObject fgProc;
		SubControlSelectionProcessor mouseProc;
		MoveCtrlApparatus transProc;
		IObjParam *ip;

	public:
		MoveCtrlApparatusCMode( Control *c, IObjParam *i ) : 
			fgProc(c), transProc(c,i), mouseProc(&transProc,c,i) 
				{ ip = i; mouseProc.SetSupportTransformGizmo(TRUE); }
		
		int Class() { return MOVE_COMMAND; }
		int ID() { return CID_SUBOBJMOVE; }
		MouseCallBack *MouseProc(int *numPoints) { *numPoints=2; return &mouseProc; }
		ChangeForegroundCallback *ChangeFGProc() { return &fgProc; }
		BOOL ChangeFG( CommandMode *oldMode ) { return oldMode->ChangeFGProc() != &fgProc; }
		void EnterMode() { ip->SetToolButtonState(MOVE_BUTTON,TRUE); }
		void ExitMode() { ip->SetToolButtonState(MOVE_BUTTON,FALSE); }
	};

class RotateCtrlApparatusCMode : public CommandMode {
	
	private:
		ChangeFGObject fgProc;
		SubControlSelectionProcessor mouseProc;
		RotateCtrlApparatus transProc;
		IObjParam *ip;

	public:
		RotateCtrlApparatusCMode( Control *c, IObjParam *i ) : 
			fgProc(c), transProc(c,i), mouseProc(&transProc,c,i) 
				{ ip = i; mouseProc.SetSupportTransformGizmo(TRUE); }
		
		int Class() { return ROTATE_COMMAND; }
		int ID() { return CID_SUBOBJROTATE; }
		MouseCallBack *MouseProc(int *numPoints) { *numPoints=2; return &mouseProc; }
		ChangeForegroundCallback *ChangeFGProc() { return &fgProc; }
		BOOL ChangeFG( CommandMode *oldMode ) { return oldMode->ChangeFGProc() != &fgProc; }
		void EnterMode() { ip->SetToolButtonState(ROTATE_BUTTON,TRUE); }
		void ExitMode() { ip->SetToolButtonState(ROTATE_BUTTON,FALSE); }
	};

class UScaleCtrlApparatusCMode : public CommandMode {
	
	private:
		ChangeFGObject fgProc;
		SubControlSelectionProcessor mouseProc;
		ScaleCtrlApparatus transProc;
		IObjParam *ip;

	public:
		UScaleCtrlApparatusCMode( Control *c, IObjParam *i ) : 
			fgProc(c), transProc(c,i), mouseProc(&transProc,c,i) 
				{ ip = i; mouseProc.SetSupportTransformGizmo(TRUE); }
		
		int Class() { return USCALE_COMMAND; }
		int ID() { return CID_SUBOBJUSCALE; }
		MouseCallBack *MouseProc(int *numPoints) { *numPoints=2; return &mouseProc; }
		ChangeForegroundCallback *ChangeFGProc() { return &fgProc; }
		BOOL ChangeFG( CommandMode *oldMode ) { return oldMode->ChangeFGProc() != &fgProc; }
		void EnterMode() { ip->SetToolButtonState(USCALE_BUTTON,TRUE); }
		void ExitMode() { ip->SetToolButtonState(USCALE_BUTTON,FALSE); }
	};

class NUScaleCtrlApparatusCMode : public CommandMode {
	
	private:
		ChangeFGObject fgProc;
		SubControlSelectionProcessor mouseProc;
		ScaleCtrlApparatus transProc;
		IObjParam *ip;

	public:
		NUScaleCtrlApparatusCMode( Control *c, IObjParam *i ) : 
			fgProc(c), transProc(c,i), mouseProc(&transProc,c,i) 
				{ ip = i; mouseProc.SetSupportTransformGizmo(TRUE); }
		
		int Class() { return SCALE_COMMAND; }
		int ID() { return CID_SUBOBJSCALE; }
		MouseCallBack *MouseProc(int *numPoints) { *numPoints=2; return &mouseProc; }
		ChangeForegroundCallback *ChangeFGProc() { return &fgProc; }
		BOOL ChangeFG( CommandMode *oldMode ) { return oldMode->ChangeFGProc() != &fgProc; }
		void EnterMode() { ip->SetToolButtonState(NUSCALE_BUTTON,TRUE); }
		void ExitMode() { ip->SetToolButtonState(NUSCALE_BUTTON,FALSE); }
	};

class SquashCtrlApparatusCMode : public CommandMode {
	
	private:
		ChangeFGObject fgProc;
		SubControlSelectionProcessor mouseProc;
		ScaleCtrlApparatus transProc;
		IObjParam *ip;

	public:
		SquashCtrlApparatusCMode( Control *c, IObjParam *i ) : 
			fgProc(c), transProc(c,i), mouseProc(&transProc,c,i) 
				{ ip = i; mouseProc.SetSupportTransformGizmo(TRUE); }
		
		int Class() { return SQUASH_COMMAND; }
		int ID() { return CID_SUBOBJSQUASH; }
		MouseCallBack *MouseProc(int *numPoints) { *numPoints=2; return &mouseProc; }
		ChangeForegroundCallback *ChangeFGProc() { return &fgProc; }
		BOOL ChangeFG( CommandMode *oldMode ) { return oldMode->ChangeFGProc() != &fgProc; }
		void EnterMode() { ip->SetToolButtonState(SQUASH_BUTTON,TRUE); }
		void ExitMode() { ip->SetToolButtonState(SQUASH_BUTTON,FALSE); }
	};

class SelectCtrlApparatusCMode : public CommandMode {
	
	private:
		ChangeFGObject fgProc;
		SubControlSelectionProcessor mouseProc;
		SelectCtrlApparatus transProc;
		IObjParam *ip;

	public:
		SelectCtrlApparatusCMode( Control *c, IObjParam *i ) :
			fgProc(c), transProc(c,i), mouseProc(NULL,c,i) 
				{ ip = i; }
		
		int Class() { return SELECT_COMMAND; }
		int ID() { return CID_SUBOBJSELECT; }
		MouseCallBack *MouseProc(int *numPoints) { *numPoints=2; return &mouseProc; }
		ChangeForegroundCallback *ChangeFGProc() { return &fgProc; }
		BOOL ChangeFG( CommandMode *oldMode ) { return oldMode->ChangeFGProc() != &fgProc; }
		void EnterMode() { ip->SetToolButtonState(SELECT_BUTTON,TRUE); }
		void ExitMode() { ip->SetToolButtonState(SELECT_BUTTON,FALSE); }
	};



#define MOVE_INTERSECTION	1
#define MOVE_PROJECTION		2

float CoreExport GetPerspMouseSpeed();
void CoreExport SetPerspMouseSpeed(float speed);

void CoreExport SetMoveModeType(int moveType);
int CoreExport GetMoveModeType();

void CoreExport SetRotationIncrement(float inc);
float CoreExport GetRotationIncrement();

#endif __OBJMODE__


