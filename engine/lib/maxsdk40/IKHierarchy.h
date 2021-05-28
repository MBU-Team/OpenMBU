/**********************************************************************
 *<
	FILE: IKHierarchy.h

	DESCRIPTION:  Geometrical representation of the ik problem. Note that
				  this file should not dependent on Max SDK, except for
				  some math classes, such as Matrix3, Point3, etc.

	CREATED BY: Jianmin Zhao

	HISTORY: created 16 March 2000

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/
#ifndef __IKHierarchy__H
#define __IKHierarchy__H

namespace IKSys {
  class ZeroPlaneMap {
  public:
	virtual Point3 operator()(const Point3& EEAxis) const =0;
	virtual ~ZeroPlaneMap() {}
  };

// A LinkChain consists of a RootLink and a number of Links.
// A RootLink consists of a rotation plus a rigidExtend. It transforms
// like this:
//	To_Coordinate_Frame = rigidExtend * rotXYZ * From_Coordinate_Frame.
// where rotXYZ = Rot_x(rotXYZ[0]) * Rot_y(rotXYZ[1]) * Rot_z(rotXYZ[2]).
// 
// * Note that not all the x, y, and z, are degrees of freedom. Only
// Active() ones are. We put the whole rotation here so that some
// solver may choose to use it as a full rotation and then clamp the
// result to the permissible range.
// 
// * LinkMatrix(bool include_rot) returns rigidExtend if include_rot is
// false and returns the whole matrix from the From_Coordinate_Fram to
// To_Coordinate_Frame, i.e., rigidExtend*rotXYZ.rotXYZ are not all degrees of freedom. Only the active ones are. 
//
// * Matrix3& ApplyLinkMatrix(Matrix3& mat, bool) applies the LinkMatrix() to
// the input matrix from the left, i.e., mat = LinkMatrix(bool)*mat,
// and returns the reference to the input matrix.
//
// * Matrix3& RotateByAxis(Matrix3&, unsigned i) pre-applies the
// rotation about x, y, or z (corresponding to i=0,1,or 2).
// Therefore, starting with the identity matrix, mat,
//	ApplyLinkMatrix(
//		RotateByAxis(
//			RotateByAxis(
//				RotateByAxis(mat, 2),
//				1),
//			0),
//		false)
//  should equal to LinkMatrix(true).
//
  class RootLink {
  public:
	RootLink():flags(7){} // x,y,z, are all active. No joint limits.
	Point3		rotXYZ;
	Point3		initXYZ;
	Point3		llimits;
	Point3		ulimits;
	Matrix3		rigidExtend;
	bool		GetActive(unsigned i) const { return flags&(1<<i)?true:false;}
	bool		GetLLimited(unsigned i) const { return flags&(1<<(i+3))?true:false;}
	bool		GetULimited(unsigned i) const { return flags&(1<<(i+6))?true:false;}
	Matrix3&	RotateByAxis(Matrix3& mat, unsigned i) const;
	Matrix3		LinkMatrix(bool include_rot) const;
	Matrix3&	ApplyLinkMatrix(Matrix3& mat, bool include_rot) const;
	// Set methods:
	//
	void		SetActive(unsigned i, bool s);
	void		SetLLimited(unsigned i, bool s);
	void		SetULimited(unsigned i, bool s);
  private:
	unsigned	flags;
  };

// A Link is a 1-dof rotation followed by a rigidExtend. The dof
// axis is specified by dofAxis. It is always active.
// 
// * LinkMatrix(true) == rigidExtend * Rotation(dofAxis, dofValue).
//   LinkMatrix(false) == rigidExtend.
//
// * Matrix3& ApplyLinkMatrix(Matrix3& mat, bool) pre-applies the
// LinkMatrix(bool) to the input matrix, mat.
//
// * A typical 3-dof (xyz) joint is decomposed into three links. z and
// y dofs don't have rigid extension, called NullLink(). Let's use
//		++o
// to denote NullLink() and
//		---o
// to denote !NullLink(). Then, a 3-dof joint will be decomposed into
// three Links, as:
//		---o++o++o
//         x  y  z
//
// * For an xyz rotation joint, if y is not active (Active unchecked),
// then y will be absorbed into the z-link, as:
//		---o---o
//         x   z
// In this case, the z-link is not NullLink(). But its length is
// zero. It is called ZeroLengh() link.
//
  class Link {
  public:
	Link():rigidExtend(0),dofAxis(RotZ){}
	~Link(){if (rigidExtend) delete rigidExtend; rigidExtend = 0;}
	enum DofAxis {
	  TransX,
	  TransY,
	  TransZ,
	  RotX,
	  RotY,
	  RotZ
	};
	DofAxis		dofAxis;
	float		dofValue;
	float		initValue;
	Point2		limits;
	bool		NullLink() const {return rigidExtend?false:true;}
	bool		ZeroLength() const {
	  return NullLink() ? true :
		(rigidExtend->GetIdentFlags() & POS_IDENT) ? true : false; }
	bool		LLimited() const { return llimited?true:false; }
	bool		ULimited() const { return ulimited?true:false; }
	Matrix3		DofMatrix() const;
	Matrix3&	DofMatrix(Matrix3& mat) const;	
	Matrix3		LinkMatrix(bool include_dof =true) const;
	Matrix3&	ApplyLinkMatrix(Matrix3& mat, bool include_dof =true) const;
	// Set methods:
	//
	void		SetLLimited(bool s) { llimited = s?1:0; }
	void		SetULimited(bool s) { ulimited = s?1:0; }
	void		SetRigidExtend(const Matrix3& mat);
  private:
	Matrix3*	rigidExtend;
	byte		llimited : 1;
	byte		ulimited : 1;
  };

// A LinkChain consists of a RootLink and LinkCount() of Links.
// 
// * parentMatrix is where the root joint starts with respect to the
// world. It should not concern the solver. Solvers should derive their
// solutions in the parent space.
//
// * goal is represented in the parent space, i.e.,
//		goal_in_world = goal * parentMatrix
//
// * Bone(): The Link of index i may be a NullLink(). Bone(i) gives
// the index j so that j >= i and LinkOf(j).NullLink() is false. If j
// >= LinkCount() means that the chain ends up with NullLink().
//
// * PreBone(i) gives the index, j, so that j < i and LinkOf(j) is not
// NullLink(). For the following 3-dof joint:
//		---o++o++o---o
//            i
// Bone(i) == i+1, and PreBone(i) == i-2. Therefore, degrees of
// freedom of LinkOf(i) == Bone(i) - PreBone(i).
// 
// * A typical two bone chain with elbow being a ball joint has this
// structure:
//		---o++o++o---O
//         2  1  0   rootLink
// It has 3 links in addition to the root link.
//
// * A two-bone chain with the elbow being a hinge joint has this
// structure:
//		---o---O
//         0   rootLink
// It has one link. Geometrically, the axis of LinkOf(0) should be
// perpendicular to the two bones.
//
// * The matrix at the end effector is
//		End_Effector_matrix == LinkOf(n-1).LinkMatrix(true) * ... *
//			LinkOf(0).LinkMatrix(true) * rootLink.LinkMatrix(true).
//
// * swivelAngle, chainNormal, and defaultZeroMap concerns solvers that
// answer true to IKSolver::UseSwivelAngle().
//
// * chainNormal is the normal to the plane that is intrinsic to the
// chain when it is constructed. It is represented in the object space
// of the root joint.
//
// * A zero-map is a map that maps the end effector axis (EEA) to a
// plane normal perpendicular to the EEA. The IK System will provide a
// default one to the solver. However, a solver may choose to use its
// own.
//
// * Given the swivelAngle, the solver is asked to adjust the rotation
// at the root joint, root_joint_rotation, so that:
// (A)  EEA stays fixed
// (B)	chainNormal * root_joint_rotation
//		== zeroMap(EEA) * RotationAboutEEA(swivelAngle)
// By definition, zeroMap(EEA) is always perpendicular to EEA. At the
// initial pose, chainNormal is also guarranteed to be perpendicular 
// to zeroMap(EEA). When it is not, root_joint_rotation has to
// maintain (A) absolutely and satisfy (B) as good as it is possible.
//
  class LinkChain {
  public:
	enum SAParentSpace {
	  kSAInGoal,
	  kSAInStartJoint
	};

	LinkChain():links(0),linkCount(0),defaultZeroMap(0),swivelAngle(0){}
	LinkChain(unsigned lc):linkCount(lc),defaultZeroMap(0),swivelAngle(0)
		{links = new Link[lc];}
	virtual		~LinkChain(){delete[] links; links = NULL;}
	virtual void* GetInterface(ULONG i) const { return NULL; }
	Matrix3		parentMatrix;
	RootLink	rootLink;
	const Link&	LinkOf(unsigned i) const {return links[i];}
	Link&		LinkOf(unsigned i) {return links[i];}
	unsigned	LinkCount() const { return linkCount; }
	int			PreBone(unsigned i) const;
	unsigned	Bone(unsigned i) const;
	bool		useVHTarget;
	union {
	  float		swivelAngle;
	  float		vhTarget[3];
	};
	SAParentSpace swivelAngleParent;
	Point3		chainNormal; // plane normal
	const ZeroPlaneMap*	defaultZeroMap;
	Matrix3		goal;
  protected:
	void SetLinkCount(unsigned lc){
	  delete links;
	  linkCount = lc;
	  links = new Link[linkCount];}
  private:
	Link*		links;
	unsigned	linkCount;
  };

  // Inlines:
  //
  inline void RootLink::SetActive(unsigned i, bool s)
  {
	unsigned mask = 1 << i;
	if (s) flags |= mask;
	else flags &= ~mask;
  }

  inline void RootLink::SetLLimited(unsigned i, bool s)
  {
	unsigned mask = 1 << (3 + i);
	if (s) flags |= mask;
	else flags &= ~mask;
  }
	
  inline void RootLink::SetULimited(unsigned i, bool s)
  {
	unsigned mask = 1 << (6 + i);
	if (s) flags |= mask;
	else flags &= ~mask;
  }

  inline Matrix3& RootLink::RotateByAxis(Matrix3& mat, unsigned i) const
  {
	switch (i) {
	case 0: mat.PreRotateX(rotXYZ[0]); return mat;
	case 1: mat.PreRotateY(rotXYZ[1]); return mat;
	case 2: mat.PreRotateZ(rotXYZ[2]); return mat;
	default: return mat;
	}
  }

  inline Matrix3& RootLink::ApplyLinkMatrix(Matrix3& mat, bool include_rot) const
  {
	if (include_rot) {
	  RotateByAxis(mat, 2);
	  RotateByAxis(mat, 1);
	  RotateByAxis(mat, 0);
	}
	mat = rigidExtend * mat;
	return mat;
  }

  inline Matrix3 RootLink::LinkMatrix(bool include_rot) const
  {
	Matrix3 mat(TRUE);
	return ApplyLinkMatrix(mat, include_rot);
  }

  inline void Link::SetRigidExtend(const Matrix3& mat)
  {
	if (mat.IsIdentity()) {
	  if (rigidExtend) {
		delete rigidExtend;
		rigidExtend = NULL;
	  }
	} else {
	  if (rigidExtend) *rigidExtend = mat;
	  else rigidExtend = new Matrix3(mat);
	}
  }

  inline Matrix3 Link::DofMatrix() const
  {
	switch (dofAxis) {
	case TransX:
	case TransY:
	case TransZ:
	  {
		Point3 p(0.0f,0.0f,0.0f);
		p[dofAxis] = dofValue;
		return TransMatrix(p);
	  }
	case RotX:
	  return RotateXMatrix(dofValue);
	case RotY:
	  return RotateYMatrix(dofValue);
	case RotZ:
	  return RotateZMatrix(dofValue);
	default:
	  return Matrix3(1);
	}
  }

  inline Matrix3& Link::DofMatrix(Matrix3& mat) const
  {
	switch (dofAxis) {
	case TransX:
	case TransY:
	case TransZ:
	  {
		Point3 p(0.0f,0.0f,0.0f);
		p[dofAxis] = dofValue;
		mat.PreTranslate(p);
	  }
	  return mat;
	case RotX:
	  mat.PreRotateX(dofValue);	return mat;
	case RotY:
	  mat.PreRotateY(dofValue);	return mat;
	case RotZ:
	  mat.PreRotateZ(dofValue);	return mat;
	default:
	  return mat;
	}
  }

  inline Matrix3 Link::LinkMatrix(bool include_dof) const
  {
	Matrix3 ret;
	if (include_dof) {
	  ret = DofMatrix();
	  ApplyLinkMatrix(ret, false);
	} else {
	  ret = rigidExtend ? *rigidExtend : Matrix3(1);
	}
	return ret;
  }

  inline Matrix3& Link::ApplyLinkMatrix(Matrix3& mat, bool include_dof) const
  // premultiply mat
  {
	if (include_dof) DofMatrix(mat);
	if (rigidExtend) mat = *rigidExtend * mat;
	return mat;
  }

  inline int LinkChain::PreBone(unsigned i) const
  // return number < i. Returning -1 means that the previous bone is the root
  // link.
  {
	for (int j = i - 1; j >= 0; --j)
	  if (!links[j].ZeroLength()) break;
	return j;
  }

  inline unsigned LinkChain::Bone(unsigned i) const
  // return number >= i.
  {
	for (size_t j = i; j < linkCount; ++j)
	  if (!links[j].ZeroLength()) break;
	return j;
  }
}; // namespace IKSys
#endif __IKHierarchy__H
