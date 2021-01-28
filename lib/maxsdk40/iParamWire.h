/*	
 *		iParamWire.h - Public interface to Parameter Wiring Manager & Wire Controllers
 *
 *			Copyright © Autodesk, Inc, 2000.  John Wainwright.
 *
 */

#ifndef _H_IPARAMWIRE
#define _H_IPARAMWIRE

#include "iFnPub.h"

class IParamWireMgr;
class IBaseWireControl;

// ---------  parameter wiring manager, provides gerenal access to param wiring functions ---

#define PARAMWIRE_MGR_INTERFACE   Interface_ID(0x490d0e99, 0xbe87c96)
inline IParamWireMgr* GetParamWireMgr() { return (IParamWireMgr*)GetCOREInterface(PARAMWIRE_MGR_INTERFACE); }

// class IParamWireMgr
//    parameter wiring manager interface 
class IParamWireMgr : public FPStaticInterface 
{
public:
	// function IDs 
	enum { startParamWire,
		   openEditor,
		   editParams, editParam,
		   editControllers, editController,
		   popupParamMenu, 
		   connect, connect2Way,
		   disconnect, disconnect2Way,
		}; 

	virtual void StartParamWire()=0;					// launch param wiring UI mode
	virtual void OpenEditor()=0;						// open param wiring dialog on selected objects
	virtual void EditParams(ReferenceTarget* leftParent, int leftSubNum,
							ReferenceTarget* rightParent, int rightSubNum)=0;  // edit params
	virtual void EditParam(ReferenceTarget* parent, int subNum)=0;
	virtual void EditControllers(Control* leftWire, Control* rightWire)=0;
	virtual void EditController(Control* wire)=0;
	virtual bool Connect(ReferenceTarget* fromParent, int fromSubNum,
							ReferenceTarget* toParent, int toSubNum,
							TCHAR* toExpr)=0;		 // set up a one-way wire from -> to
	virtual bool Connect2Way(ReferenceTarget* leftParent, int leftSubNum,
							ReferenceTarget* rightParent, int rightSubNum,
							TCHAR* leftExpr, TCHAR* rightExpr=NULL)=0;     // set up a two-way wire
	virtual bool Disconnect(Control* wireController)=0;  // disconnect one-way
	virtual bool Disconnect2Way(Control* wireController1, Control* wireController2)=0;  // disconnect two-way

}; 

// ------ individual wire controller interface -------------

// wire controller classes & names

#define FLOAT_WIRE_CONTROL_CLASS_ID		Class_ID(0x498702e7, 0x71f11549)
#define POSITION_WIRE_CONTROL_CLASS_ID	Class_ID(0x5065767c, 0x683a42a6)
#define POINT3_WIRE_CONTROL_CLASS_ID	Class_ID(0x4697286a, 0x2f7f05cf)
#define ROTATION_WIRE_CONTROL_CLASS_ID	Class_ID(0x31381913, 0x3a904167)
#define SCALE_WIRE_CONTROL_CLASS_ID		Class_ID(0x7c8f3a2b, 0x1e954d92)

#define WIRE_CONTROLLER_INTERFACE   Interface_ID(0x25ce0f5c, 0x6c303d2f)
inline IBaseWireControl* GetWireControlInterface(Animatable* a) { return (IBaseWireControl*)a->GetInterface(WIRE_CONTROLLER_INTERFACE); }

class IBaseWireControl : public StdControl, public FPMixinInterface {
	public:
		// local 
		// number of wires, wire param access
		virtual int		get_num_wires()=0;			 // how many wires out of this controller (number of dependent params)
		virtual Animatable* get_wire_parent(int i)=0;  // get ith dependent parameter parent animatable
		virtual int		get_wire_subnum(int i)=0;    // get ith dependent parameter subanim num in the animatable
		virtual Control* get_co_controller(int i)=0; // get ith co_controller

		// transfer expression script
		virtual TCHAR*	get_expr_text(int i)=0;
		virtual void	set_expr_text(int i, TCHAR* text)=0;

		// animation sub-controller
		virtual void	set_slave_animation(Control* c)=0;
		virtual Control* get_slave_animation()=0;

		// type predicate
		virtual bool    is_master()=0;
		virtual bool	is_slave()=0;
		virtual bool	is_two_way()=0;

		// parent/subnum transfers
		virtual void transfer_parent(ReferenceTarget* oldp, ReferenceTarget* newp)=0;
		virtual void transfer_subnum(short oldn, short newn)=0;

		// FnPub stuff
		enum { getNumWires, getWireParent, getWireSubnum, getCoController, 
			   getExprText, setExprText, 
			   getSlaveAnimation, setSlaveAnimation, isMaster, isSlave, isTwoWay, };

		// from FPInterface
		FPInterfaceDesc* GetDesc() { return GetDescByID(WIRE_CONTROLLER_INTERFACE); }

		BEGIN_FUNCTION_MAP
			FN_1(getWireParent,		 TYPE_REFTARG,	 get_wire_parent,		TYPE_INDEX);
			FN_1(getWireSubnum,		 TYPE_INT,		 get_wire_subnum,	TYPE_INDEX);
			FN_1(getCoController,	 TYPE_CONTROL,	 get_co_controller,	TYPE_INDEX);
			FN_1(getExprText,		 TYPE_STRING,	 get_expr_text,		TYPE_INDEX);
			VFN_2(setExprText,						 set_expr_text,		TYPE_INDEX, TYPE_STRING);
			RO_PROP_FN(getNumWires,	 get_num_wires,	 TYPE_INT);
			RO_PROP_FN(isMaster,	 is_master,		 TYPE_bool);
			RO_PROP_FN(isSlave,		 is_slave,		 TYPE_bool);
			RO_PROP_FN(isTwoWay,	 is_two_way,	 TYPE_bool);
			PROP_FNS(getSlaveAnimation, get_slave_animation, setSlaveAnimation, set_slave_animation, TYPE_CONTROL);
		END_FUNCTION_MAP

};


#endif
