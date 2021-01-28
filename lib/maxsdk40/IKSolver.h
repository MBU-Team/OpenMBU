/**********************************************************************
 *<
	FILE: IKSolver.h

	DESCRIPTION:  IK Solver Class definition

	CREATED BY: Jianmin Zhao

	HISTORY: created 16 March 2000

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef __IKSolver__H
#define __IKSolver__H
#include "IKHierarchy.h"

class IKSolver  : public BaseInterfaceServer {
public:
  typedef unsigned	ReturnCondition;
  enum ConditionBit {
	bLimitReached =			0x00000001,
	bLimitClamped =			0x00000002,
	bMaxIterationReached =	0x00000004,
	// The first eight bits are reserved for mild condition.
	// They are still considered successful.
	bGoalTooCloseToEE =		0x00000100,
	bInvalidArgument =		0x00000200,
	bInvalidInitialValue =	0x00000400
  };
  // Plugins derived from this class are supposed to have this super class id.
  virtual SClass_ID SuperClassID() {return IK_SOLVER_CLASS_ID;}
  virtual Class_ID ClassID() =0;
  virtual void GetClassName(TSTR& s) { s= TSTR(_T("IKSolver")); }  
  virtual ~IKSolver(){}

  // History independent solver does need time input.
  virtual bool		IsHistoryDependent() const =0;
  virtual bool		DoesOneChainOnly() const =0;
  // Interactive solver does need initial pose. It needs current pose.
  virtual bool		IsInteractive() const =0;
  virtual bool		UseSlidingJoint() const =0;
  virtual bool		UseSwivelAngle() const =0;
  // Solutions of an analytic solver is not dependent on Pos/Rot threshold
  // or MaxInteration number.
  virtual bool		IsAnalytic() const { return false; }
  // The result will be simply clamped into joint limits by the ik system
  // if the solver does not do joint limits.
  virtual bool		DoesRootJointLimits() const { return false;}
  virtual bool		DoesJointLimitsButRoot() const { return false;}
  // RotThreshold() is not relevant to solver that does not SolveEERotation().
  // UseSwivelAngle() and SolveEERotation() cannot both be true.
  virtual bool		SolveEERotation() const =0;
  // A solver may have its own zero map. If so, the IK system may want
  // to know through this method.
  virtual const IKSys::ZeroPlaneMap*
			GetZeroPlaneMap(const Point3& a0, const Point3& n0) const
			{ return NULL; }
  virtual float		GetPosThreshold() const =0;
  virtual float		GetRotThreshold() const =0;
  virtual unsigned	GetMaxIteration() const =0;
  virtual void		SetPosThreshold(float) =0;
  virtual void		SetRotThreshold(float) =0;
  virtual void		SetMaxIteration(unsigned) =0;
  // The derived class should override this method if it answers true
  // to DoesOneChainOnly() and false to HistoryDependent().
  // The solver is not designed to be invoked recursively. The
  // recursion logic existing among the ik chains is taken care of by
  // the Max IK (sub-)System.
  virtual ReturnCondition Solve(IKSys::LinkChain&) =0;
};
#endif // #ifndef __IKSolver__H
