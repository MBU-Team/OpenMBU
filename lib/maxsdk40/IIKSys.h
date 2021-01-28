/**********************************************************************
 *<
	FILE: IIKSys.h

	DESCRIPTION:  Interfaces into IK sub-system classes/functions. 

	CREATED BY: Jianmin Zhao

	HISTORY: created 3 April 2000

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef __IIKSys__H
#define __IIKSys__H

#include "iFnPub.h"

// Class ids
#define IKCONTROL_CLASS_ID Class_ID(0xe6f5815, 0x717f99a4)
#define IKCHAINCONTROL_CLASS_ID Class_ID(0x78724378, 0x8a4fd9)
#define GET_IKCHAIN_CD (GetCOREInterface()->GetDllDir().ClassDir().FindClass(CTRL_MATRIX3_CLASS_ID, IKCHAINCONTROL_CLASS_ID))

#define IK_FP_INTERFACE_ID Interface_ID(0x5b734601, 0x7c7c7ece)
#define IKCHAIN_FP_INTERFACE_ID Interface_ID(0x5a5e7cbe, 0x55367776)

#define GET_IK_OPS_INTERFACE ((IKCmdOps*)GetCOREInterface(IK_FP_INTERFACE_ID))
#define GET_IKCHAIN_FP_INTERFACE ((IKChainActions*)GET_IKCHAIN_CD->GetInterface(IKCHAIN_FP_INTERFACE_ID))

class IKCmdOps : public FPStaticInterface {
public:
  virtual INode* CreateIKChain(INode* start, INode* end, const TCHAR* solver) =0;
};

class IKChainActions : public FPStaticInterface {
public:
  virtual FPStatus SnapAction() =0;
  virtual FPStatus IKSnapAction() =0;
  virtual FPStatus FKSnapAction() =0;
  virtual BOOL IsSnapEnabled() =0;
};

namespace IKSys {
  class ZeroPlaneMap;
}
//
// IIKChainControl
//
class IKSolver;
class IIKChainControl {
public:
  // Reference index:
  //
  enum {
	kPBlockRef = 0,	// ParamBlock
	kGoalTMRef,		// Matrix3 controller
	kEndJointRef,	// INode
	kEnableRef,		// Bool (float) controller
	kStartJointRef, // INode
	kLastRef
  };
  // Parameter block index:
  //
  enum {
	kParamBlock,
	kNumParamBlocks
  };
  // Paramter index of parameters in param-block of index kParamBlock:
  //
  enum {
	kStartJoint,	// INode, referenced by kStartJointRef
	kEndJoint,		// INode, referenced by kEndJointRef
	kSolverName,	// String
	kAutoEnable,	// BOOL
	kSwivel,		// Angle
	kPosThresh,		// Float
	kRotThresh,		// Float
	kIteration,		// Integer
	kEEDisplay,		// BOOL
	kEESize,		// Float
	kGoalDisplay,	// BOOL
	kGoalSize,		// Float
	kVHDisplay,		// BOOL
	kVHSize,		// Float
	kVHLength,		// Float
	kSolverDisplay,	// BOOL
	kAutoSnap,		// BOOL
	kVHUseTarget,	// BOOL
	kVHTarget,		// INode
	kSAParent,		// RadioBtn_Index
	kLastParam
  };

  enum SAParentSpace {
	kSAInGoal,
	kSAInStartJoint
  };

  // IK chain delimiters
  virtual INode*		StartJoint() const =0;
  virtual INode*		EndJoint() const =0;
  virtual INode*		GetNode() const =0;

  // Vector Handle
  // InitPlane/InitEEAxis are the normal and End Effector Axis at the
  // initial pose. They are represented in the parent space.
  // ChainNormal is the InitPlane represented in the object space:
  // InitPlane() == ChainNormal * StartJoint()->InittialRotation()
  virtual Point3		ChainNormal(TimeValue t, Interval& valid) =0;
  virtual Point3		InitPlane(TimeValue t) =0;
  virtual Point3		InitEEAxis(TimeValue t) =0;
  virtual float			InitChainLength(TimeValue t) =0;
  virtual float			SwivelAngle(TimeValue, Interval&)=0;
  virtual const IKSys::ZeroPlaneMap*
  						DefaultZeroPlaneMap(TimeValue t) =0;
  virtual SAParentSpace SwivelAngleParent() const =0;
  
  // Solver
  virtual IKSolver*		Solver() const =0;
  virtual bool			SolverEnabled(TimeValue, Interval* =0)=0;
  virtual bool			CanAutoEnable() const =0;
  virtual bool			AutoEnableSet() const =0;
  virtual bool			Valid() const =0;
};

namespace IKSys {
  enum DofAxis {
	TransX = 0,	TransY,	TransZ,
	RotX, RotY,	RotZ,
	DofX = 0, DofY,	DofZ
  };
  enum JointType {
	SlidingJoint,
	RotationalJoint
  };
  struct DofSet {
	DofSet() : bits(0) {}
	DofSet(const DofSet& src) : bits(src.bits) {}
	void Add(DofAxis dn) { bits |= (1 << dn); }
	void Clear(DofAxis dn) { bits &= ~(unsigned(1 << dn)); }
	int Include(DofAxis dn) const { return bits & (1 << dn); }
	int Count() const;
	unsigned bits;
  };
  inline int DofSet::Count() const
  {
	for (unsigned b = bits, ret = 0, i = TransX; i <= RotZ; ++i) {
	  if (b == 0) break;
	  else if (b & 01u) {
		ret++;
	  }
	  b = b >> 1;
	}
	return ret;
  }
}; // namespace IKSys
//
// IIKControl
//
class IIKControl {
public:
  typedef IKSys::DofAxis DofAxis;
  typedef IKSys::JointType JointType;
  typedef IKSys::DofSet DofSet;

  //Queries
  virtual bool		DofActive(DofAxis) const =0;
  virtual DofSet	ActiveTrans() const =0;
  virtual DofSet	ActiveRot() const =0;
  virtual DofSet	ActiveDofs() const =0;
  virtual INodeTab	IKChains(JointType) const =0;
  virtual bool		DofLowerLimited(DofAxis) const =0;
  virtual bool		DofUpperLimited(DofAxis) const =0;
  virtual Point2	DofLimits(DofAxis) const =0;
  virtual Point3	TransLowerLimits() const =0;
  virtual Point3	TransUpperLimits() const =0;
  virtual Point3	RotLowerLimits() const =0;
  virtual Point3	RotUpperLimits() const =0;
  virtual bool		IKBound(TimeValue t, JointType jt)=0;
  virtual Control*	FKSubController() const =0;
  virtual INode*	GetNode() const =0;
  virtual Point3	PrefPosition(TimeValue t, Interval& validityInterval) =0;
  virtual Point3	PrefRotation(TimeValue t, Interval& validityInterval) =0;

  // DOF values
  virtual Point3	TransValues(TimeValue, Interval* =0)=0;
  virtual Point3	RotValues(TimeValue, Interval* =0)=0;
  virtual void		AssignTrans(const Point3&, const Interval&)=0;
  virtual void		AssignRot(const Point3&, const Interval&)=0;
  virtual void		AssignActiveTrans(const Point3&, const Interval&)=0;
  virtual void		AssignActiveRot(const Point3&, const Interval&)=0;
  virtual void		AssignActiveTrans(const DofSet&, const float[],
									  const Interval&)=0;
  virtual void		AssignActiveRot(const DofSet&, const float[],
									const Interval&)=0;
  virtual void		SetTransValid(const Interval& valid) =0;
  virtual void		SetRotValid(const Interval& valid) =0;
  virtual void		SetTRValid(const Interval& valid) =0;
  virtual void		SetPrefTrans(const Point3& val, TimeValue t =0) =0;
  virtual void		SetPrefRot(const Point3& val, TimeValue t =0) =0;
  virtual void		SetPrefTR(const Point3& trans, const Point3& rot,
							  TimeValue t =0) =0;
};
#endif // #ifndef __IIKSys__H
