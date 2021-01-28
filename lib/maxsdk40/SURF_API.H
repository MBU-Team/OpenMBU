/**********************************************************************
 *<
    FILE: surf_api.h

    DESCRIPTION:  Provides the SDK api for NURBS surfaces

    CREATED BY: Charlie Thaeler

    HISTORY: created 15 April, 1997

 *> Copyright (c) 1997, All Rights Reserved.
 **********************************************************************/

#ifndef SURF_API_H
#define SURF_API_H
#include "point3.h"
#include "maxtess.h"


#define EDITABLE_SURF_CLASS_ID Class_ID(0x76a11646, 0x12a822fb)
#define FITPOINT_PLANE_CLASS_ID Class_ID(0x76a11646, 0xbadbeff)
#define EDITABLE_CVCURVE_CLASS_ID Class_ID(0x76a11646, 0x12a83145)
#define EDITABLE_FPCURVE_CLASS_ID Class_ID(0x76a11646, 0x12a92143)

#define NURBS_NAME_SIZE 80


typedef ULONG_PTR NURBSId;  // NOTE: THESE ARE NOT PERSISTANT ACROSS SESSIONS
								//       and should NOT be saved to a file.
    // WIN64 Cleanup: Shuler


typedef Tab<NURBSId> NURBSIdTab;
typedef Tab<BOOL> BoolTab;

enum NURBSResult {
	kNOk,
	kNInvalidObject,
	kNInvalidId,
	kNInvalidParameter,
	kNBad
};

enum NURBSMirrorAxis {
	kMirrorX,
	kMirrorY,
	kMirrorZ,
	kMirrorXY,
	kMirrorXZ,
	kMirrorYZ
};

enum NURBSConstType {
	kNConstOnObject,
	kNConstOffset,
	kNConstNormal,
	kNConstTangent
};

enum NURBSKind {
	kNURBSPoint,
	kNURBSTexturePoint,
	kNURBSCV,
	kNURBSCurve,
	kNURBSSurface
};

enum NURBSParamaterization {
    kCentripetal,
	kUniform,
};

enum NURBSAutoParam {
	kNotAutomatic,
    kAutoCentripetal,
	kAutoUniform,
};

enum NURBSType {
	kNPoint,
	kNPointCPoint, // constrained points
	kNCurveCPoint,
	kNCurveCurveIntersectionPoint,
	kNSurfaceCPoint,
	kNCurveSurfaceIntersectionPoint,
	kNTexturePoint,

	kNCV,

	kNCVCurve,
	kNPointCurve,
	kNBlendCurve,
	kNOffsetCurve,
	kNXFormCurve,
	kNMirrorCurve,
	kNFilletCurve,
	kNChamferCurve,
	kNIsoCurve,
	kNProjectVectorCurve,
	kNProjectNormalCurve,
	kNSurfSurfIntersectionCurve,
	kNCurveOnSurface,
	kNPointCurveOnSurface,
	kNSurfaceNormalCurve,
	kNSurfaceEdgeCurve,

	kNCVSurface,
	kNPointSurface,
	kNBlendSurface,
	kNOffsetSurface,
	kNXFormSurface,
	kNMirrorSurface,
	kNRuledSurface,
	kNULoftSurface,
	kNExtrudeSurface,
	kNLatheSurface,
	kNUVLoftSurface,
	kNNBlendSurface,
	kN1RailSweepSurface,
	kN2RailSweepSurface,
	kNCapSurface,
	kNMultiCurveTrimSurface,
	kNFilletSurface
};


enum NURBSTessType {
	kNTessSurface,
	kNTessDisplacement,
	kNTessCurve
};

enum NURBSSubObjectLevel {
    kNTopLevel = 0,
    kNSurfaceCVLevel,
    kNSurfaceLevel,
    kNCurveCVLevel,
    kNPointLevel,
    kNCurveLevel,
    kNImportsLevel,
};



class NURBSSet;

typedef Tab<NURBSId> NURBSIdTab;

extern int FindIndex(NURBSIdTab ids, NURBSId id);

class NURBSObject {
	friend class NURBSSet;
protected:
	TCHAR mName[NURBS_NAME_SIZE];
	NURBSType mType;
	NURBSKind mKind;
	NURBSId mId;
	Object *mpObject;
	NURBSSet* mpNSet;
	BOOL mSelected;
	DllExport void Clean(NURBSIdTab ids);
public:
	DllExport NURBSObject(void);
	DllExport virtual ~NURBSObject(void);
	DllExport NURBSObject & operator=(const NURBSObject& pt);
	DllExport void SetName(TCHAR *name);
	DllExport TCHAR *GetName(void);
	DllExport NURBSType GetType();
	DllExport NURBSKind GetKind();
	DllExport NURBSId GetId();
	DllExport void SetId(NURBSId id);
	DllExport void SetNSet(NURBSSet *nset);
	DllExport void SetObject(Object *object);
	DllExport Object* GetMAXObject();
	DllExport NURBSSet* GetNSet();
	DllExport int GetIndex();
	DllExport BOOL IsSelected();
	DllExport void SetSelected(BOOL set);
};


class NURBSPoint : public NURBSObject {
protected:
	double mX, mY, mZ;
public:
	DllExport NURBSPoint();
	DllExport virtual Point3 GetPosition(TimeValue t);
	DllExport virtual void GetPosition(TimeValue t, float& x, float& y, float& z);
	DllExport virtual void GetPosition(TimeValue t, double& x, double& y, double& z);
};

class NURBSTexturePoint : public NURBSObject {
protected:
	double mX, mY;
    int    mUIndex, mVIndex;
public:
	DllExport NURBSTexturePoint();
	DllExport virtual Point2 GetPosition(TimeValue t);
	DllExport virtual void GetPosition(TimeValue t, float& x, float& y);
	DllExport virtual void GetPosition(TimeValue t, double& x, double& y);
	DllExport void SetPosition(TimeValue t, Point2 pt);
	DllExport void SetPosition(TimeValue t, float x, float y);
	DllExport void SetPosition(TimeValue t, double x, double y);

    DllExport void SetIndices(int uIndex, int vIndex);
};


class NURBSIndependentPoint : public NURBSPoint {
public:
	DllExport NURBSIndependentPoint(void);
	DllExport virtual ~NURBSIndependentPoint(void);
	DllExport NURBSIndependentPoint & operator=(const NURBSIndependentPoint& pt);
	DllExport BOOL operator==(const NURBSIndependentPoint& pt);
	DllExport BOOL operator!=(const NURBSIndependentPoint& pt);
	DllExport void SetPosition(TimeValue t, Point3 pt);
	DllExport void SetPosition(TimeValue t, float x, float y, float z);
	DllExport void SetPosition(TimeValue t, double x, double y, double z);

};


class NURBSControlVertex : public NURBSObject {
	double mX, mY, mZ;
	double mW;  // weight
public:
	DllExport NURBSControlVertex(void);
	DllExport virtual ~NURBSControlVertex(void);
	DllExport NURBSControlVertex & operator=(const NURBSControlVertex& cv);
	DllExport BOOL operator==(const NURBSControlVertex& cv);
	DllExport BOOL operator!=(const NURBSControlVertex& cv);
	DllExport void SetPosition(TimeValue t, Point3 pt);
	DllExport void SetPosition(TimeValue t, float x, float y, float z);
	DllExport void SetPosition(TimeValue t, double x, double y, double z);
	DllExport Point3 GetPosition(TimeValue t);
	DllExport void GetPosition(TimeValue t, float& x, float& y, float& z);
	DllExport void GetPosition(TimeValue t, double& x, double& y, double& z);
	DllExport void SetWeight(TimeValue t, float wt);
	DllExport void SetWeight(TimeValue t, double wt);
	DllExport void GetWeight(TimeValue t, float& wt);
	DllExport double GetWeight(TimeValue t);
	DllExport void GetWeight(TimeValue t, double& wt);
};


class NURBSPointConstPoint : public NURBSPoint {
	friend class NURBSSet;
protected:
	NURBSId mParentId;
	int mParentIndex;
	NURBSConstType mCType;
	Point3 mOffset;
	DllExport void Clean(NURBSIdTab ids);

public:
	DllExport NURBSPointConstPoint(void);
	DllExport virtual ~NURBSPointConstPoint(void);
	DllExport NURBSPointConstPoint & operator=(const NURBSPointConstPoint& pt);
	DllExport void SetParent(int index);
	DllExport void SetParentId(NURBSId id);
	DllExport int GetParent(void);
	DllExport NURBSId GetParentId(void);
	DllExport void SetPointType(NURBSConstType type);
	DllExport NURBSConstType GetPointType(void);
	DllExport void SetOffset(TimeValue t, Point3 pt);
	DllExport Point3 GetOffset(TimeValue t);
};

class NURBSCurveConstPoint : public NURBSPoint {
	friend class NURBSSet;
protected:
	NURBSId mParentId;
	int mParentIndex;
	NURBSConstType mCType;
	Point3 mOffset;
	float mNormal;
	float mUTangent;
	double mUParam;
    BOOL mTrimCurve;
    BOOL mFlipTrim;

	DllExport void Clean(NURBSIdTab ids);
public:
	DllExport NURBSCurveConstPoint(void);
	DllExport virtual ~NURBSCurveConstPoint(void);
	DllExport NURBSCurveConstPoint & operator=(const NURBSCurveConstPoint& pt);
	DllExport void SetParent(int index);
	DllExport void SetParentId(NURBSId id);
	DllExport int GetParent(void);
	DllExport NURBSId GetParentId(void);
	DllExport void SetPointType(NURBSConstType type);
	DllExport NURBSConstType GetPointType(void);
	DllExport void SetOffset(TimeValue t, Point3 pt);
	DllExport Point3 GetOffset(TimeValue t);
	DllExport void SetUParam(TimeValue t, double param);
	DllExport double GetUParam(TimeValue t);
	DllExport void SetNormal(TimeValue t, float dist);
	DllExport float GetNormal(TimeValue t);
	DllExport void SetUTangent(TimeValue t, float dist);
	DllExport float GetUTangent(TimeValue t);
    DllExport BOOL GetTrimCurve();
    DllExport void SetTrimCurve(BOOL trim);
    DllExport BOOL GetFlipTrim();
    DllExport void SetFlipTrim(BOOL flip);
};

class NURBSCurveCurveIntersectionPoint : public NURBSPoint {
	friend class NURBSSet;
protected:
	NURBSId mParentId[2];
	int mParentIndex[2];
    double mCurveParam[2];
    BOOL mTrimCurve[2];
    BOOL mFlipTrim[2];

	DllExport void Clean(NURBSIdTab ids);
public:
	DllExport NURBSCurveCurveIntersectionPoint(void);
	DllExport virtual ~NURBSCurveCurveIntersectionPoint(void);
	DllExport NURBSCurveCurveIntersectionPoint & operator=(const NURBSCurveCurveIntersectionPoint &pt);
    DllExport void SetCurveParam(int curveNum, double param);
    DllExport double GetCurveParam(int curveNum);
	DllExport void SetParent(int pnum, int index);
	DllExport void SetParentId(int pnum, NURBSId id);
	DllExport int GetParent(int pnum);
	DllExport NURBSId GetParentId(int pnum);

    DllExport BOOL GetTrimCurve(int pnum);
    DllExport void SetTrimCurve(int pnum, BOOL trim);
    DllExport BOOL GetFlipTrim(int pnum);
    DllExport void SetFlipTrim(int pnum, BOOL flip);
};

class NURBSSurfConstPoint : public NURBSPoint {
	friend class NURBSSet;
protected:
	NURBSId mParentId;
	int mParentIndex;
	NURBSConstType mCType;
	Point3 mOffset;
	float mNormal;
	float mUTangent;
	double mUParam;
	float mVTangent;
	double mVParam;
	DllExport void Clean(NURBSIdTab ids);
public:
	DllExport NURBSSurfConstPoint(void);
	DllExport virtual ~NURBSSurfConstPoint(void);
	DllExport NURBSSurfConstPoint & operator=(const NURBSSurfConstPoint& pt);
	DllExport void SetParent(int index);
	DllExport void SetParentId(NURBSId id);
	DllExport int GetParent(void);
	DllExport NURBSId GetParentId(void);
	DllExport void SetPointType(NURBSConstType type);
	DllExport NURBSConstType GetPointType(void);
	DllExport void SetUParam(TimeValue t, double param);
	DllExport double GetUParam(TimeValue t);
	DllExport void SetVParam(TimeValue t, double param);
	DllExport double GetVParam(TimeValue t);
	DllExport void SetOffset(TimeValue t, Point3 pt);
	DllExport Point3 GetOffset(TimeValue t);
	DllExport void SetNormal(TimeValue t, float dist);
	DllExport float GetNormal(TimeValue t);
	DllExport void SetUTangent(TimeValue t, float dist);
	DllExport float GetUTangent(TimeValue t);
	DllExport void SetVTangent(TimeValue t, float dist);
	DllExport float GetVTangent(TimeValue t);
};

class NURBSCurveSurfaceIntersectionPoint : public NURBSPoint {
	friend class NURBSSet;
protected:
	// parent 0 should be the surface parent 1 should be the curve
	NURBSId mParentId[2];
	int mParentIndex[2];
	double mSeed;
    BOOL mTrimCurve;
    BOOL mFlipTrim;

	DllExport void Clean(NURBSIdTab ids);
public:
	DllExport NURBSCurveSurfaceIntersectionPoint(void);
	DllExport virtual ~NURBSCurveSurfaceIntersectionPoint(void);
	DllExport NURBSCurveSurfaceIntersectionPoint & operator=(const NURBSCurveSurfaceIntersectionPoint &pt);
    DllExport void SetSeed(double seed);
    DllExport double GetSeed();
	DllExport void SetParent(int pnum, int index);
	DllExport void SetParentId(int pnum, NURBSId id);
	DllExport int GetParent(int pnum);
	DllExport NURBSId GetParentId(int pnum);

    DllExport BOOL GetTrimCurve();
    DllExport void SetTrimCurve(BOOL trim);
    DllExport BOOL GetFlipTrim();
    DllExport void SetFlipTrim(BOOL flip);
};





typedef Tab<NURBSControlVertex> NURBSCVTab;
typedef Tab<double> NURBSKnotTab;



enum NURBSTrimDirection // defines the side to Keep
{
    kNone = 0,
    kPositive = 1,
    kNegative = 2
};


class NURBSTrimPoint {
public:
    DllExport NURBSTrimPoint(double parameter, NURBSTrimDirection direction) :
        mParameter(parameter), mDirection(direction) {}
    DllExport double GetParameter() {return mParameter; }
    DllExport NURBSTrimDirection GetDirection() {return mDirection; }

private:
    double mParameter;
    NURBSTrimDirection mDirection;
};



class NURBSCurve : public NURBSObject {
protected:
	friend class NURBSCVCurve;
	friend class NURBSPointCurve;
	friend class NURBSBlendCurve;
	friend class NURBSOffsetCurve;
	friend class NURBSXFormCurve;
	friend class NURBSMirrorCurve;
	friend class NURBSFilletCurve;
	friend class NURBSChamferCurve;
	friend class NURBSIsoCurve;
	friend class NURBSSurfaceEdgeCurve;
	friend class NURBSProjectVectorCurve;
	friend class NURBSProjectNormalCurve;
	friend class NURBSSurfaceNormalCurve;
	friend class NURBSNBlendSurface;
	friend class NURBSRuledSurface;
	friend class NURBSULoftSurface;
	friend class NURBSUVLoftSurface;
	friend class NURBSExtrudeSurface;
	friend class NURBSLatheSurface;
	friend class NURBSCapSurface;
	friend class NURBS1RailSweepSurface;
	friend class NURBS2RailSweepSurface;
	friend class NURBSMultiCurveTrimSurface;

	int mMatID;

public:
	DllExport NURBSCurve(void);
	DllExport virtual ~NURBSCurve(void);
	DllExport NURBSCurve & operator=(const NURBSCurve& curve);
	DllExport BOOL IsClosed(void);

    DllExport int NumTrimPoints();
    DllExport NURBSTrimPoint GetTrimPoint(TimeValue t, int i);
    
	DllExport BOOL Evaluate(TimeValue t, double u, Point3& pt, Point3& tangent);
	DllExport void GetParameterRange(TimeValue t, double& uMin, double& uMax);
	DllExport BOOL GetNURBSData(TimeValue t,
								int& degree,
								int& numCVs,
								NURBSCVTab& cvs,
								int& numKnots,
								NURBSKnotTab& knots);
	DllExport int MatID();
	DllExport void MatID(int id);
};


class NURBSCVCurve : public NURBSCurve {
	friend class NURBSSet;
protected:
	NURBSControlVertex *mpCVs;
	double *mpKnots;
	BOOL mClosed;
	int mOrder;
	int mNumKnots;
	int mNumCVs;
	NURBSAutoParam mAutoParam;
public:
	DllExport NURBSCVCurve(void);
	DllExport virtual ~NURBSCVCurve(void);
	DllExport NURBSCVCurve & operator=(const NURBSCVCurve& curve);
	DllExport void Close(void);
	DllExport BOOL IsClosed(void);

	DllExport void SetOrder(int order);
	DllExport int GetOrder(void);
	DllExport void SetNumKnots(int num);         // data is NOT maintained
	DllExport int GetNumKnots(void);
	DllExport void SetNumCVs(int num);           // data is NOT maintained
	DllExport void GetNumCVs(int& num);
	DllExport int GetNumCVs(void);
	DllExport double GetKnot(int index);
	DllExport void SetKnot(int index, double value);
	DllExport NURBSControlVertex* GetCV(int index);
	DllExport void SetCV(int index, NURBSControlVertex &cv);
	DllExport void SetTransformMatrix(TimeValue t, SetXFormPacket& xPack);
	DllExport Matrix3 GetTransformMatrix(TimeValue t);
	DllExport NURBSAutoParam AutoParam();
	DllExport void AutoParam(TimeValue t, NURBSAutoParam param);
	DllExport void Reparameterize(TimeValue t, NURBSParamaterization param);

	DllExport void EndsOverlap(BOOL& overlap);
	DllExport void Refine(TimeValue t, double u); // looses animation
	DllExport void Insert(TimeValue t, double u);
};


class NURBSPointCurve : public NURBSCurve {
	friend class NURBSSet;
protected:
	NURBSIndependentPoint *mpPts;
	BOOL mClosed;
	int mNumPts;
public:
	DllExport NURBSPointCurve(void);
	DllExport virtual ~NURBSPointCurve(void);
	DllExport NURBSPointCurve & operator=(const NURBSPointCurve& curve);
	DllExport void Close(void);
	DllExport BOOL IsClosed(void);

	DllExport void SetNumPts(int num);       // data is NOT maintained
	DllExport int GetNumPts(void);
	DllExport void GetNumPts(int &num);
	DllExport NURBSIndependentPoint* GetPoint(int index);
	DllExport void SetPoint(int index, NURBSIndependentPoint &pt);
	DllExport void SetTransformMatrix(TimeValue t, SetXFormPacket& xPack);
	DllExport Matrix3 GetTransformMatrix(TimeValue t);
	DllExport void Refine(TimeValue t, double u); // looses animation
};


class NURBSBlendCurve : public NURBSCurve {
	friend class NURBSSet;
	NURBSId mParentId[2];
	int mParentIndex[2];
	BOOL mParentEnd[2];
	double mTension[2];
	DllExport void Clean(NURBSIdTab ids);
public:
	DllExport NURBSBlendCurve(void);
	DllExport virtual ~NURBSBlendCurve(void);
	DllExport NURBSBlendCurve & operator=(const NURBSBlendCurve& curve);
	DllExport void SetParent(int pnum, int index);
	DllExport void SetParentId(int pnum, NURBSId id);
	DllExport int GetParent(int pnum);
	DllExport NURBSId GetParentId(int pnum);
	DllExport void SetEnd(int pnum, BOOL end);
	DllExport BOOL GetEnd(int pnum);
	DllExport void SetTension(TimeValue t, int pnum, double ten);
	DllExport double GetTension(TimeValue t, int pnum);
};


class NURBSOffsetCurve : public NURBSCurve {
	friend class NURBSSet;
	NURBSId mParentId;
	int mParentIndex;
	double mDistance;
	DllExport void Clean(NURBSIdTab ids);
public:
	DllExport NURBSOffsetCurve(void);
	DllExport virtual ~NURBSOffsetCurve(void);
	DllExport NURBSOffsetCurve & operator=(const NURBSOffsetCurve& curve);
	DllExport void SetParent(int index);
	DllExport void SetParentId(NURBSId id);
	DllExport int GetParent(void);
	DllExport NURBSId GetParentId(void);
	DllExport void SetDistance(TimeValue t, double d);
	DllExport double GetDistance(TimeValue t);
};

class NURBSXFormCurve : public NURBSCurve {
	friend class NURBSSet;
	NURBSId mParentId;
	int mParentIndex;
	Matrix3 mXForm;
	DllExport void Clean(NURBSIdTab ids);
public:
	DllExport NURBSXFormCurve(void);
	DllExport virtual ~NURBSXFormCurve(void);
	DllExport NURBSXFormCurve & operator=(const NURBSXFormCurve& curve);
	DllExport void SetParent(int index);
	DllExport void SetParentId(NURBSId id);
	DllExport int GetParent(void);
	DllExport NURBSId GetParentId(void);
	DllExport void SetXForm(TimeValue t, Matrix3& mat);
	DllExport Matrix3& GetXForm(TimeValue t);
};

class NURBSMirrorCurve : public NURBSCurve {
	friend class NURBSSet;
	NURBSId mParentId;
	int mParentIndex;
	NURBSMirrorAxis mAxis;
	Matrix3 mXForm;
	double mDistance;
	DllExport void Clean(NURBSIdTab ids);
public:
	DllExport NURBSMirrorCurve(void);
	DllExport virtual ~NURBSMirrorCurve(void);
	DllExport NURBSMirrorCurve & operator=(const NURBSMirrorCurve& curve);
	DllExport void SetParent(int index);
	DllExport void SetParentId(NURBSId id);
	DllExport int GetParent(void);
	DllExport NURBSId GetParentId(void);
	DllExport void SetAxis(NURBSMirrorAxis axis);
	DllExport NURBSMirrorAxis GetAxis(void);
	DllExport void SetXForm(TimeValue t, Matrix3& mat);
	DllExport Matrix3& GetXForm(TimeValue t);
	DllExport void SetDistance(TimeValue t, double d);
	DllExport double GetDistance(TimeValue t);
};


class NURBSFilletCurve : public NURBSCurve {
	friend class NURBSSet;
	NURBSId mParentId[2];
	int mParentIndex[2];
	BOOL mParentEnd[2];
	double mRadius;
    BOOL mTrimCurve[2];
    BOOL mFlipTrim[2];

	DllExport void Clean(NURBSIdTab ids);
public:
	DllExport NURBSFilletCurve(void);
	DllExport virtual ~NURBSFilletCurve(void);
	DllExport NURBSFilletCurve & operator=(const NURBSFilletCurve& curve);
	DllExport void SetParent(int pnum, int index);
	DllExport void SetParentId(int pnum, NURBSId id);
	DllExport int GetParent(int pnum);
	DllExport NURBSId GetParentId(int pnum);
	DllExport void SetEnd(int pnum, BOOL end);
	DllExport BOOL GetEnd(int pnum);
	DllExport void SetRadius(TimeValue t, double radius);
	DllExport double GetRadius(TimeValue t);

    DllExport BOOL GetTrimCurve(int pnum);
    DllExport void SetTrimCurve(int pnum, BOOL trim);
    DllExport BOOL GetFlipTrim(int pnum);
    DllExport void SetFlipTrim(int pnum, BOOL flip);
};


class NURBSChamferCurve : public NURBSCurve {
	friend class NURBSSet;
	NURBSId mParentId[2];
	int mParentIndex[2];
	BOOL mParentEnd[2];
	double mLength[2];
    BOOL mTrimCurve[2];
    BOOL mFlipTrim[2];

	DllExport void Clean(NURBSIdTab ids);
public:
	DllExport NURBSChamferCurve(void);
	DllExport virtual ~NURBSChamferCurve(void);
	DllExport NURBSChamferCurve & operator=(const NURBSChamferCurve& curve);
	DllExport void SetParent(int pnum, int index);
	DllExport void SetParentId(int pnum, NURBSId id);
	DllExport int GetParent(int pnum);
	DllExport NURBSId GetParentId(int pnum);
	DllExport void SetEnd(int pnum, BOOL end);
	DllExport BOOL GetEnd(int pnum);
	DllExport void SetLength(TimeValue t, int pnum, double length);
	DllExport double GetLength(TimeValue t, int pnum);

    DllExport BOOL GetTrimCurve(int pnum);
    DllExport void SetTrimCurve(int pnum, BOOL trim);
    DllExport BOOL GetFlipTrim(int pnum);
    DllExport void SetFlipTrim(int pnum, BOOL flip);
};


class NURBSIsoCurve : public NURBSCurve {
	friend class NURBSSet;
	NURBSId mParentId;
	int mParentIndex;
	BOOL mIsU;  // false for V...
	double mParam;
	BOOL mTrim;
	BOOL mFlipTrim;
	Point2 mSeed;
	DllExport void Clean(NURBSIdTab ids);
public:
	DllExport NURBSIsoCurve(void);
	DllExport virtual ~NURBSIsoCurve(void);
	DllExport NURBSIsoCurve & operator=(const NURBSIsoCurve& curve);
	DllExport void SetParent(int index);
	DllExport void SetParentId(NURBSId id);
	DllExport int GetParent(void);
	DllExport NURBSId GetParentId(void);
	DllExport void SetDirection(BOOL isU);
	DllExport BOOL GetDirection(void);
	DllExport void SetParam(TimeValue t, double p);
	DllExport double GetParam(TimeValue t);
	DllExport BOOL GetTrim();
	DllExport void SetTrim(BOOL trim);
	DllExport BOOL GetFlipTrim();
	DllExport void SetFlipTrim(BOOL flip);
	DllExport Point2 GetSeed();
	DllExport void SetSeed(Point2& seed);
};


class NURBSSurfaceEdgeCurve : public NURBSCurve {
	friend class NURBSSet;
	NURBSId mParentId;
	int mParentIndex;
	Point2 mSeed;
	DllExport void Clean(NURBSIdTab ids);
public:
	DllExport NURBSSurfaceEdgeCurve(void);
	DllExport virtual ~NURBSSurfaceEdgeCurve(void);
	DllExport NURBSSurfaceEdgeCurve & operator=(const NURBSSurfaceEdgeCurve& curve);
	DllExport void SetParent(int index);
	DllExport void SetParentId(NURBSId id);
	DllExport int GetParent(void);
	DllExport NURBSId GetParentId(void);
	DllExport Point2 GetSeed();
	DllExport void SetSeed(Point2& seed);
};


class NURBSProjectVectorCurve : public NURBSCurve {
	friend class NURBSSet;
	// parent 0 should be the surface parent 1 should be the curve
	NURBSId mParentId[2];
	int mParentIndex[2];
	BOOL mTrim;
	BOOL mFlipTrim;
	Point2 mSeed;
	Point3 mPVec;
	DllExport void Clean(NURBSIdTab ids);
public:
	DllExport NURBSProjectVectorCurve(void);
	DllExport virtual ~NURBSProjectVectorCurve(void);
	DllExport NURBSProjectVectorCurve & operator=(const NURBSProjectVectorCurve& curve);
	DllExport void SetParent(int pnum, int index);
	DllExport void SetParentId(int pnum, NURBSId id);
	DllExport int GetParent(int pnum);
	DllExport NURBSId GetParentId(int pnum);
	DllExport BOOL GetTrim();
	DllExport void SetTrim(BOOL trim);
	DllExport BOOL GetFlipTrim();
	DllExport void SetFlipTrim(BOOL flip);
	DllExport Point2 GetSeed();
	DllExport void SetSeed(Point2& seed);
	DllExport void SetPVec(TimeValue t, Point3& pvec); // projection vector
	DllExport Point3& GetPVec(TimeValue t);
};



class NURBSProjectNormalCurve : public NURBSCurve {
	friend class NURBSSet;
	// parent 0 should be the surface parent 1 should be the curve
	NURBSId mParentId[2];
	int mParentIndex[2];
	BOOL mTrim;
	BOOL mFlipTrim;
	Point2 mSeed;
	DllExport void Clean(NURBSIdTab ids);
public:
	DllExport NURBSProjectNormalCurve(void);
	DllExport virtual ~NURBSProjectNormalCurve(void);
	DllExport NURBSProjectNormalCurve & operator=(const NURBSProjectNormalCurve& curve);
	DllExport void SetParent(int pnum, int index);
	DllExport void SetParentId(int pnum, NURBSId id);
	DllExport int GetParent(int pnum);
	DllExport NURBSId GetParentId(int pnum);
	DllExport BOOL GetTrim();
	DllExport void SetTrim(BOOL trim);
	DllExport BOOL GetFlipTrim();
	DllExport void SetFlipTrim(BOOL flip);
	DllExport Point2 GetSeed();
	DllExport void SetSeed(Point2& seed);
};


class NURBSSurfSurfIntersectionCurve : public NURBSCurve {
	friend class NURBSSet;
	// parent 0 should be the surface parent 1 should be the curve
	NURBSId mParentId[2];
	int mParentIndex[2];
	BOOL mTrim[2];
	BOOL mFlipTrim[2];
	Point2 mSeed;
	DllExport void Clean(NURBSIdTab ids);
public:
	DllExport NURBSSurfSurfIntersectionCurve(void);
	DllExport virtual ~NURBSSurfSurfIntersectionCurve(void);
	DllExport NURBSSurfSurfIntersectionCurve & operator=(const NURBSSurfSurfIntersectionCurve& curve);
	DllExport void SetParent(int pnum, int index);
	DllExport void SetParentId(int pnum, NURBSId id);
	DllExport int GetParent(int pnum);
	DllExport NURBSId GetParentId(int pnum);
	DllExport BOOL GetTrim(int tnum);
	DllExport void SetTrim(int tnum, BOOL trim);
	DllExport BOOL GetFlipTrim(int tnum);
	DllExport void SetFlipTrim(int tnum, BOOL flip);
	DllExport Point2 GetSeed();
	DllExport void SetSeed(Point2& seed);
};



class NURBSCurveOnSurface : public NURBSCVCurve {
	friend class NURBSSet;
	NURBSId mParentId;
	int mParentIndex;
	BOOL mTrim;
	BOOL mFlipTrim;
	DllExport void Clean(NURBSIdTab ids);
public:
	DllExport NURBSCurveOnSurface(void);
	DllExport virtual ~NURBSCurveOnSurface(void);
	DllExport NURBSCurveOnSurface & operator=(const NURBSCurveOnSurface& curve);
	DllExport void SetParent(int index);
	DllExport void SetParentId(NURBSId id);
	DllExport int GetParent();
	DllExport NURBSId GetParentId();
	DllExport BOOL GetTrim();
	DllExport void SetTrim(BOOL trim);
	DllExport BOOL GetFlipTrim();
	DllExport void SetFlipTrim(BOOL flip);
};

class NURBSPointCurveOnSurface : public NURBSPointCurve {
	friend class NURBSSet;
	NURBSId mParentId;
	int mParentIndex;
	BOOL mTrim;
	BOOL mFlipTrim;
	DllExport void Clean(NURBSIdTab ids);
public:
	DllExport NURBSPointCurveOnSurface(void);
	DllExport virtual ~NURBSPointCurveOnSurface(void);
	DllExport NURBSPointCurveOnSurface & operator=(const NURBSPointCurveOnSurface& curve);
	DllExport void SetParent(int index);
	DllExport void SetParentId(NURBSId id);
	DllExport int GetParent();
	DllExport NURBSId GetParentId();
	DllExport BOOL GetTrim();
	DllExport void SetTrim(BOOL trim);
	DllExport BOOL GetFlipTrim();
	DllExport void SetFlipTrim(BOOL flip);
};


class NURBSSurfaceNormalCurve : public NURBSCurve {
	friend class NURBSSet;
	NURBSId mParentId;
	int mParentIndex;
	double mDistance;
	DllExport void Clean(NURBSIdTab ids);
public:
	DllExport NURBSSurfaceNormalCurve(void);
	DllExport virtual ~NURBSSurfaceNormalCurve(void);
	DllExport NURBSSurfaceNormalCurve & operator=(const NURBSSurfaceNormalCurve& curve);
	DllExport void SetParent(int index);
	DllExport void SetParentId(NURBSId id);
	DllExport int GetParent();
	DllExport NURBSId GetParentId();
	DllExport void SetDistance(TimeValue t, double dist);
	DllExport double GetDistance(TimeValue t);
};




enum NURBSTexSurfType {
	kNMapDefault,
	kNMapUserDefined,
	kNMapSufaceMapper
};

class NURBSTextureSurface {
	NURBSTexSurfType mMapperType;
	NURBSTexturePoint *mpPoints;
	int mNumUPoints;
	int mNumVPoints;

	NURBSId mParentId;
	int mParentIndex;
public:
	DllExport NURBSTextureSurface(void);
	DllExport ~NURBSTextureSurface(void);
	DllExport NURBSTextureSurface(NURBSTextureSurface& tsurf);
	DllExport NURBSTextureSurface & operator=(const NURBSTextureSurface& surf);
	DllExport NURBSTexSurfType MapperType();
	DllExport void SetMapperType(NURBSTexSurfType type);

	DllExport void SetParent(int index);
	DllExport void SetParentId(NURBSId id);
	DllExport int GetParent();
	DllExport NURBSId GetParentId();

	DllExport void SetNumPoints(int u, int v);
	DllExport int GetNumUPoints(void);
	DllExport int GetNumVPoints(void);
	DllExport void GetNumPoints(int &u, int &v);
	DllExport NURBSTexturePoint* GetPoint(int u, int v);
	DllExport void SetPoint(int u, int v, NURBSTexturePoint& pnt);
};



class NURBSTextureChannel;

class NURBSTextureChannelSet {
	friend class NURBSSurface;
    ~NURBSTextureChannelSet();

private:
    NURBSTextureChannel* GetChannelByIndex(int index) { return mTextureChannels[index]; }
    NURBSTextureChannel* GetChannel(int channel);
    NURBSTextureChannel* AddChannel(int channel);
    int NumChannels() { return mTextureChannels.Count(); }

    Tab<NURBSTextureChannel*> mTextureChannels;
};

class NURBSTextureChannel {
	friend class NURBSSurface;
	friend class NURBSTextureChannelSet;
private:
    DllExport NURBSTextureChannel(int channel);

    DllExport int GetChannel() { return mChannel; }

    DllExport BOOL GenerateUVs();
	DllExport void SetGenerateUVs(BOOL state);
	DllExport Point2 GetTextureUVs(TimeValue t, int i);
	DllExport void SetTextureUVs(TimeValue t, int i, Point2 pt);
	DllExport void GetTileOffset(TimeValue t, float &ut, float &vt, float &uo, float &vo, float &a);
	DllExport void SetTileOffset(TimeValue t, float ut, float vt, float uo, float vo, float a);

    DllExport NURBSTextureSurface& GetTextureSurface() {return mTexSurface; }
    DllExport void SetTextureSurface(NURBSTextureSurface& texSurf);

    int mChannel;

	BOOL mGenUVs;
	Point2 mTexUVs[4];

	float mUTile;
    float mVTile;
    float mUOffset;
    float mVOffset;
    float mAngle;

	NURBSTextureSurface mTexSurface;
};

class NURBSSurface : public NURBSObject {
	friend class NURBSCVSurface;
	friend class NURBSPointSurface;
	friend class NURBSBlendSurface;
	friend class NURBSNBlendSurface;
	friend class NURBSOffsetSurface;
	friend class NURBSXFormSurface;
	friend class NURBSMirrorSurface;
	friend class NURBSCapSurface;
	friend class NURBSIsoCurve;
	friend class NURBSProjectVectorCurve;
	friend class NURBSProjectNormalCurve;
	friend class NURBSSurfSurfIntersectionCurve;
	friend class NURBSCurveOnSurface;
	friend class NURBSPointCurveOnSurface;
	friend class NURBSMultiCurveTrimSurface;
	friend class NURBSTextureChannel;
	friend class NURBSTextureChannelSet;

private:
	DllExport NURBSTextureChannel* GetChannel(int index);

protected:
    NURBSTextureChannelSet mTextureChannelSet;

	BOOL mFlipNormals;
	BOOL mRenderable;
	int mMatID;
	BOOL mClosedInU, mClosedInV;

	// new for R3 -- optional
	TessApprox *mpVTess;
	TessApprox *mpRTess;
	TessApprox *mpRTessDisp;
	TessApprox *mpVTessCurve;
	TessApprox *mpRTessCurve;

    // Internal surface cache
    // 
    void* mpSurfCache;

public:

    // Internal use only cache
    void* GetSurfCache() { return mpSurfCache; }
    void SetSurfCache(void* pCache) { mpSurfCache = pCache; }

	DllExport NURBSSurface(void);
	DllExport virtual ~NURBSSurface(void);
	DllExport NURBSSurface & operator=(const NURBSSurface& surf);
	DllExport BOOL Renderable();
	DllExport void Renderable(BOOL state);
	DllExport BOOL FlipNormals();
	DllExport void FlipNormals(BOOL state);
	DllExport BOOL GenerateUVs(int channel = 0);
	DllExport void SetGenerateUVs(BOOL state, int channel = 0);
	DllExport int MatID();
	DllExport void MatID(int id);
	DllExport Point2 GetTextureUVs(TimeValue t, int i, int channel = 0);
	DllExport void SetTextureUVs(TimeValue t, int i, Point2 pt, int channel = 0);
	DllExport void GetTileOffset(TimeValue t, float &ut, float &vt, float &uo, float &vo, float &a, int channel = 0);
	DllExport void SetTileOffset(TimeValue t, float ut, float vt, float uo, float vo, float a, int channel = 0);

    DllExport NURBSTextureSurface& GetTextureSurface(int channel);
    DllExport void SetTextureSurface(int channel, NURBSTextureSurface& texSurf);

    DllExport int NumChannels();
	DllExport int GetChannelFromIndex(int index);



	DllExport BOOL IsClosedInU(void);
	DllExport BOOL IsClosedInV(void);

	DllExport BOOL Evaluate(TimeValue t, double u, double v, Point3& pt,
			Point3& dPdU, Point3& dPdV);
	DllExport BOOL Evaluate(TimeValue t, double u, double v, Point3& pt,
			Point3& dPdU, Point3& dPdV, Point3& d2PdU2, Point3& d2PdV2, Point3& d2PdUdV);
	DllExport void GetParameterRange(TimeValue t, double& uMin, double& uMax, double& vMin, double& vMax);
	DllExport BOOL GetNURBSData(TimeValue t,
								int& degreeInU,
								int& degreeInV,
								int& numInU,
								int& numInV,
								NURBSCVTab& cvs,
								int& numKnotsInU,
								int& numKnotsInV,
								NURBSKnotTab& uKnots, 
								NURBSKnotTab& vKnots);
	DllExport BOOL GetCLPTextureSurfaceData(TimeValue t, int channel,
								int& degreeInU,
								int& degreeInV,
								int& numInU,
								int& numInV,
								NURBSCVTab& cvs,
								int& numKnotsInU,
								int& numKnotsInV,
								NURBSKnotTab& uKnots, 
								NURBSKnotTab& vKnots);
	DllExport int NumTrimLoops(TimeValue t);
	DllExport int NumCurvesInLoop(TimeValue t, int loop);
    DllExport BOOL Get2dTrimCurveData(TimeValue t, int loop, int curve,
                                        int& degree,
                                        int& numCVs,
                                        NURBSCVTab& cvs,
                                        int& numKnots,
                                        NURBSKnotTab& knots);
	DllExport BOOL Get3dTrimCurveData(TimeValue t, int loop, int curve,
										int& degree,
										int& numCVs,
										NURBSCVTab& cvs,
										int& numKnots,
										NURBSKnotTab& knots);
	DllExport TessApprox* GetProdTess(NURBSTessType type=kNTessSurface);
	DllExport TessApprox* GetViewTess(NURBSTessType type=kNTessSurface);
	DllExport void SetProdTess(TessApprox& tess, NURBSTessType type=kNTessSurface);
	DllExport void SetViewTess(TessApprox& tess, NURBSTessType type=kNTessSurface);
	DllExport void ClearViewTess(NURBSTessType type=kNTessSurface);
	DllExport void ClearProdTess(NURBSTessType type=kNTessSurface);
};


class NURBSCVSurface : public NURBSSurface {
	friend class NURBSSet;
	NURBSControlVertex *mpCVs;
	double *mpUKnots;
	double *mpVKnots;
	int mUOrder;
	int mVOrder;
	int mNumUCVs;
	int mNumVCVs;
	int mNumUKnots;
	int mNumVKnots;
	BOOL mRigid;
	NURBSAutoParam mAutoParam;
public:
	DllExport NURBSCVSurface(void);
	DllExport virtual ~NURBSCVSurface(void);
	DllExport NURBSCVSurface & operator=(const NURBSCVSurface& surf);
	DllExport BOOL IsRigid();
	DllExport void SetRigid(BOOL isRigid);
	DllExport NURBSAutoParam AutoParam();
	DllExport void AutoParam(TimeValue t, NURBSAutoParam param);
	DllExport void Reparameterize(TimeValue t, NURBSParamaterization param);
	DllExport void CloseInU(void);
	DllExport void CloseInV(void);
	DllExport void SetUOrder(int order);
	DllExport int GetUOrder(void);
	DllExport int GetVOrder(void);
	DllExport void SetVOrder(int order);
	DllExport void SetNumUKnots(int num);         // data is NOT maintained
	DllExport void SetNumVKnots(int num);         // data is NOT maintained
	DllExport int GetNumUKnots(void);
	DllExport int GetNumVKnots(void);
	DllExport void SetNumCVs(int u, int v);       // data is NOT maintained
	DllExport int GetNumUCVs(void);
	DllExport int GetNumVCVs(void);
	DllExport void GetNumCVs(int &u, int &v);
	DllExport double GetUKnot(int index);
	DllExport double GetVKnot(int index);
	DllExport void SetUKnot(int index, double value);
	DllExport void SetVKnot(int index, double value);
	DllExport NURBSControlVertex* GetCV(int u, int v);
	DllExport void SetCV(int u, int v, NURBSControlVertex& cv);

	DllExport void SetTransformMatrix(TimeValue t, SetXFormPacket& mat);
	DllExport Matrix3 GetTransformMatrix(TimeValue t);

	DllExport void EdgesOverlap(BOOL& uOverlap, BOOL& vOverlap);
	// If you refine in U (U_V_Both = 0) you must specify v
	// If you refine in V (U_V_Both = 1) you must specify u
	// If you refine in U and (U_V_Both = -1) you must specify u and v
	DllExport void Refine(TimeValue t, double u, double v, int U_V_Both);
	DllExport void Insert(TimeValue t, double u, double v, int U_V_Both);
};


class NURBSPointSurface : public NURBSSurface {
	friend class NURBSSet;
	NURBSIndependentPoint *mpPts;
	int mNumUPts;
	int mNumVPts;
public:
	DllExport NURBSPointSurface(void);
	DllExport virtual ~NURBSPointSurface(void);
	DllExport NURBSPointSurface & operator=(const NURBSPointSurface& surf);
	DllExport void CloseInU(void);
	DllExport void CloseInV(void);
	DllExport void SetNumPts(int u, int v);       // data is NOT maintained
	DllExport int GetNumUPts(void);
	DllExport int GetNumVPts(void);
	DllExport void GetNumPts(int &u, int &v);
	DllExport NURBSIndependentPoint* GetPoint(int u, int v);
	DllExport void SetPoint(int u, int v, NURBSIndependentPoint& pt);

	DllExport void SetTransformMatrix(TimeValue t, SetXFormPacket& mat);
	DllExport Matrix3 GetTransformMatrix(TimeValue t);

	// If you refine in U (U_V_Both = 0) you must specify v
	// If you refine in V (U_V_Both = 1) you must specify u
	// If you refine in U and (U_V_Both = -1) you must specify u and v
	DllExport void Refine(TimeValue t, double u, double v, int U_V_Both);
};

class NURBSBlendSurface : public NURBSSurface {
	friend class NURBSSet;
	NURBSId mParentId[2];
	int mParentIndex[2];
	int mParentEdge[2];
	BOOL mFlip[2];
	double mTension[2];
	double mCurveStartParam[2];
	DllExport void Clean(NURBSIdTab ids);
public:
	DllExport NURBSBlendSurface(void);
	DllExport virtual ~NURBSBlendSurface(void);
	DllExport NURBSBlendSurface & operator=(const NURBSBlendSurface& surf);
	DllExport void SetParent(int pnum, int index);
	DllExport void SetParentId(int pnum, NURBSId id);
	DllExport int GetParent(int pnum);
	DllExport NURBSId GetParentId(int pnum);
	DllExport void SetEdge(int pnum, int edge);
	DllExport int GetEdge(int pnum);
	DllExport void SetTension(TimeValue t, int pnum, double ten);
	DllExport double GetTension(TimeValue t, int pnum);
	DllExport void SetFlip(int pnum, BOOL flip);
	DllExport BOOL GetFlip(int pnum);

	// only if the parent is a closed curve
	DllExport void SetCurveStartPoint(TimeValue t, int pnum, double startpoint);
	DllExport double GetCurveStartPoint(TimeValue t, int pnum);
};


class NURBSNBlendSurface : public NURBSSurface {
	friend class NURBSSet;
	// The parents can be either curves or surfaces (with edge IDs)
	NURBSId mParentId[4];
	int mParentIndex[4];
	int mParentEdge[4];  // used only if the parent is a surface
	DllExport void Clean(NURBSIdTab ids);
public:
	DllExport NURBSNBlendSurface(void);
	DllExport virtual ~NURBSNBlendSurface(void);
	DllExport NURBSNBlendSurface & operator=(const NURBSNBlendSurface& surf);
	DllExport void SetParent(int pnum, int index);
	DllExport void SetParentId(int pnum, NURBSId id);
	DllExport int GetParent(int pnum);
	DllExport NURBSId GetParentId(int pnum);
	DllExport void SetEdge(int pnum, int edge);
	DllExport int GetEdge(int pnum);
};

class NURBSOffsetSurface : public NURBSSurface {
	friend class NURBSSet;
	NURBSId mParentId;
	int mParentIndex;
	double mDistance;
	DllExport void Clean(NURBSIdTab ids);
public:
	DllExport NURBSOffsetSurface(void);
	DllExport virtual ~NURBSOffsetSurface(void);
	DllExport NURBSOffsetSurface & operator=(const NURBSOffsetSurface& surf);
	DllExport void SetParent(int index);
	DllExport void SetParentId(NURBSId id);
	DllExport int GetParent(void);
	DllExport NURBSId GetParentId(void);
	DllExport void SetDistance(TimeValue t, double d);
	DllExport double GetDistance(TimeValue t);
};

class NURBSXFormSurface : public NURBSSurface {
	friend class NURBSSet;
	NURBSId mParentId;
	int mParentIndex;
	Matrix3 mXForm;
	DllExport void Clean(NURBSIdTab ids);
public:
	DllExport NURBSXFormSurface(void);
	DllExport virtual ~NURBSXFormSurface(void);
	DllExport NURBSXFormSurface & operator=(const NURBSXFormSurface& surf);
	DllExport void SetParent(int index);
	DllExport void SetParentId(NURBSId id);
	DllExport int GetParent(void);
	DllExport NURBSId GetParentId(void);
	DllExport void SetXForm(TimeValue t, Matrix3& mat);
	DllExport Matrix3& GetXForm(TimeValue t);
};

class NURBSMirrorSurface : public NURBSSurface {
	friend class NURBSSet;
	NURBSId mParentId;
	int mParentIndex;
	NURBSMirrorAxis mAxis;
	Matrix3 mXForm;
	double mDistance;
	DllExport void Clean(NURBSIdTab ids);
public:
	DllExport NURBSMirrorSurface(void);
	DllExport virtual ~NURBSMirrorSurface(void);
	DllExport NURBSMirrorSurface & operator=(const NURBSMirrorSurface& surf);
	DllExport void SetParent(int index);
	DllExport void SetParentId(NURBSId id);
	DllExport int GetParent(void);
	DllExport NURBSId GetParentId(void);
	DllExport void SetAxis(NURBSMirrorAxis axis);
	DllExport NURBSMirrorAxis GetAxis(void);
	DllExport void SetXForm(TimeValue t, Matrix3& mat);
	DllExport Matrix3& GetXForm(TimeValue t);
	DllExport void SetDistance(TimeValue t, double d);
	DllExport double GetDistance(TimeValue t);
};

class NURBSRuledSurface : public NURBSSurface {
	friend class NURBSSet;
	NURBSId mParentId[2];
	int mParentIndex[2];
	BOOL mFlip[2];
	double mCurveStartParam[2];
	DllExport void Clean(NURBSIdTab ids);
public:
	DllExport NURBSRuledSurface(void);
	DllExport virtual ~NURBSRuledSurface(void);
	DllExport NURBSRuledSurface & operator=(const NURBSRuledSurface& surf);
	DllExport void SetParent(int pnum, int index);
	DllExport void SetParentId(int pnum, NURBSId id);
	DllExport int GetParent(int pnum);
	DllExport NURBSId GetParentId(int pnum);
	DllExport void SetFlip(int pnum, BOOL flip);
	DllExport BOOL GetFlip(int pnum);
	DllExport void SetCurveStartPoint(TimeValue t, int pnum, double startpoint);
	DllExport double GetCurveStartPoint(TimeValue t, int pnum);
};


class NURBSULoftSurface : public NURBSSurface {
	friend class NURBSSet;
	Tab<NURBSId> mParentId;
	Tab<int> mParentIndex;
	Tab<BOOL> mFlip;
	Tab<double>mCurveStartParam;
	Tab<double>mTension;
	Tab<BOOL> mUseTangents;
	Tab<BOOL> mFlipTangents;
	BOOL mAutoAlign;
	BOOL mCloseLoft;
	DllExport void Clean(NURBSIdTab ids);
public:
	DllExport NURBSULoftSurface(void);
	DllExport virtual ~NURBSULoftSurface(void);
	DllExport NURBSULoftSurface & operator=(const NURBSULoftSurface& surf);
	DllExport void SetNumCurves(int num);
	DllExport int GetNumCurves(void);
	DllExport int AppendCurve(int index, BOOL flip, double startpoint=0.0,
								double tension=0.0, BOOL useTangent=FALSE, BOOL flipTangent=FALSE);
	DllExport int AppendCurve(NURBSId id, BOOL flip, double startpoint=0.0,
								double tension=0.0, BOOL useTangent=FALSE, BOOL flipTangent=FALSE);
	DllExport void SetParent(int pnum, int index);
	DllExport void SetParentId(int pnum, NURBSId id);
	DllExport int GetParent(int pnum);
	DllExport NURBSId GetParentId(int pnum);
	DllExport void SetFlip(int pnum, BOOL flip);
	DllExport BOOL GetFlip(int pnum);
	DllExport void SetCurveStartPoint(TimeValue t, int pnum, double startpoint);
	DllExport double GetCurveStartPoint(TimeValue t, int pnum);

	DllExport void SetCurveTension(TimeValue t, int pnum, double tension);
	DllExport double GetCurveTension(TimeValue t, int pnum);
	DllExport void SetCurveUseSurfaceTangent(int pnum, BOOL useTangent);
	DllExport BOOL GetCurveUseSurfaceTangent(int pnum);
	DllExport void SetFlipTangent(int pnum, BOOL flipTangent);
	DllExport BOOL GetFlipTangent(int pnum);
	DllExport void SetAutoAlign(BOOL autoalign);
	DllExport BOOL GetAutoAlign();
	DllExport void SetCloseLoft(BOOL closeLoft);
	DllExport BOOL GetCloseLoft();
};


class NURBSUVLoftSurface : public NURBSSurface {
	friend class NURBSSet;
	Tab<NURBSId> mUParentId;
	Tab<int> mUParentIndex;
	Tab<NURBSId> mVParentId;
	Tab<int> mVParentIndex;
	DllExport void Clean(NURBSIdTab ids);
public:
	DllExport NURBSUVLoftSurface(void);
	DllExport virtual ~NURBSUVLoftSurface(void);
	DllExport NURBSUVLoftSurface & operator=(const NURBSUVLoftSurface& surf);

	DllExport void SetNumUCurves(int num);
	DllExport int GetNumUCurves(void);
	DllExport int AppendUCurve(int index);
	DllExport int AppendUCurve(NURBSId id);
	DllExport void SetUParent(int pnum, int index);
	DllExport void SetUParentId(int pnum, NURBSId id);
	DllExport int GetUParent(int pnum);
	DllExport NURBSId GetUParentId(int pnum);

	DllExport void SetNumVCurves(int num);
	DllExport int GetNumVCurves(void);
	DllExport int AppendVCurve(int index);
	DllExport int AppendVCurve(NURBSId id);
	DllExport void SetVParent(int pnum, int index);
	DllExport void SetVParentId(int pnum, NURBSId id);
	DllExport int GetVParent(int pnum);
	DllExport NURBSId GetVParentId(int pnum);
};



class NURBSExtrudeSurface : public NURBSSurface {
	friend class NURBSSet;
	NURBSId mParentId;
	int mParentIndex;
	Matrix3 mXForm;
	double mDistance;
	double mCurveStartParam;
	DllExport void Clean(NURBSIdTab ids);
public:
	DllExport NURBSExtrudeSurface(void);
	DllExport virtual ~NURBSExtrudeSurface(void);
	DllExport NURBSExtrudeSurface & operator=(const NURBSExtrudeSurface& surf);
	DllExport void SetParent(int index);
	DllExport void SetParentId(NURBSId id);
	DllExport int GetParent(void);
	DllExport NURBSId GetParentId(void);
	DllExport void SetAxis(TimeValue t, Matrix3& ray);
	DllExport Matrix3& GetAxis(TimeValue t);
	DllExport void SetDistance(TimeValue t, double d);
	DllExport double GetDistance(TimeValue t);
	DllExport void SetCurveStartPoint(TimeValue t, double startpoint);
	DllExport double GetCurveStartPoint(TimeValue t);
};


class NURBSLatheSurface : public NURBSSurface {
	friend class NURBSSet;
	NURBSId mParentId;
	int mParentIndex;
	Matrix3 mXForm;
	double mRotation;
	double mCurveStartParam;
	DllExport void Clean(NURBSIdTab ids);
public:
	DllExport NURBSLatheSurface(void);
	DllExport virtual ~NURBSLatheSurface(void);
	DllExport NURBSLatheSurface & operator=(const NURBSLatheSurface& surf);
	DllExport void SetParent(int index);
	DllExport void SetParentId(NURBSId id);
	DllExport int GetParent(void);
	DllExport NURBSId GetParentId(void);
	DllExport void SetAxis(TimeValue t, Matrix3& ray);
	DllExport Matrix3& GetAxis(TimeValue t);
	DllExport void SetRotation(TimeValue t, double degrees);
	DllExport double GetRotation(TimeValue t);
	DllExport void SetCurveStartPoint(TimeValue t, double startpoint);
	DllExport double GetCurveStartPoint(TimeValue t);
};


class NURBSCapSurface : public NURBSSurface {
	friend class NURBSSet;
	NURBSId mParentId;
	int mParentIndex;
	int mParentEdge;
	double mCurveStartParam;
	DllExport void Clean(NURBSIdTab ids);
public:
	DllExport NURBSCapSurface(void);
	DllExport virtual ~NURBSCapSurface(void);
	DllExport NURBSCapSurface & operator=(const NURBSCapSurface& surf);
	DllExport void SetParent(int index);
	DllExport void SetParentId(NURBSId id);
	DllExport int GetParent(void);
	DllExport NURBSId GetParentId(void);
	DllExport void SetEdge(int edge);
	DllExport int GetEdge();

	// only if the parent is a closed curve
	DllExport void SetCurveStartPoint(TimeValue t, double startpoint);
	DllExport double GetCurveStartPoint(TimeValue t);
};


class NURBS1RailSweepSurface : public NURBSSurface {
	friend class NURBSSet;
	NURBSId mRailId;
	int mRailIndex;
	Tab<NURBSId> mParentId;
	Tab<int> mParentIndex;
	Tab<BOOL> mFlip;
	Tab<double> mCurveStartParam;
	BOOL mParallel;
	BOOL mSnapCrossSections;
	BOOL mRoadlike;
	Matrix3 mXForm;
	DllExport void Clean(NURBSIdTab ids);
public:
	DllExport NURBS1RailSweepSurface(void);
	DllExport virtual ~NURBS1RailSweepSurface(void);
	DllExport NURBS1RailSweepSurface & operator=(const NURBS1RailSweepSurface& surf);
	DllExport void SetParentRail(int index);
	DllExport void SetParentRailId(NURBSId id);
	DllExport int GetParentRail();
	DllExport NURBSId GetParentRailId();
	DllExport void SetNumCurves(int num);
	DllExport int GetNumCurves(void);
	DllExport int AppendCurve(int index, BOOL flip, double startpoint=0.0);
	DllExport int AppendCurve(NURBSId id, BOOL flip, double startpoint=0.0);
	DllExport void SetParent(int pnum, int index);
	DllExport void SetParentId(int pnum, NURBSId id);
	DllExport int GetParent(int pnum);
	DllExport NURBSId GetParentId(int pnum);
	DllExport void SetFlip(int pnum, BOOL flip);
	DllExport BOOL GetFlip(int pnum);
	DllExport void SetParallel(BOOL para);
	DllExport BOOL GetParallel();
	DllExport void SetCurveStartPoint(TimeValue t, int pnum, double startpoint);
	DllExport double GetCurveStartPoint(TimeValue t, int pnum);
	DllExport void SetSnapCS(BOOL snapCS);
	DllExport BOOL GetSnapCS();
	DllExport void SetRoadlike(BOOL roadlike);
	DllExport BOOL GetRoadlike();
	DllExport void SetAxis(TimeValue t, Matrix3& ray);
	DllExport Matrix3& GetAxis(TimeValue t);
};


class NURBS2RailSweepSurface : public NURBSSurface {
	friend class NURBSSet;
	Tab<NURBSId> mParentId;
	Tab<int> mParentIndex;
	Tab<BOOL> mFlip;
	NURBSId mRailParentId[2];
	int mRailParentIndex[2];
	BOOL mParallel;
	BOOL mScale;
	BOOL mSnapCrossSections;
	Tab<double> mCurveStartParam;
	DllExport void Clean(NURBSIdTab ids);
public:
	DllExport NURBS2RailSweepSurface(void);
	DllExport virtual ~NURBS2RailSweepSurface(void);
	DllExport NURBS2RailSweepSurface & operator=(const NURBS2RailSweepSurface& surf);

	DllExport void SetNumCurves(int num);
	DllExport int GetNumCurves(void);
	DllExport int AppendCurve(int index, BOOL flip, double startpoint=0.0);
	DllExport int AppendCurve(NURBSId id, BOOL flip, double startpoint=0.0);
	DllExport void SetParent(int pnum, int index);
	DllExport void SetParentId(int pnum, NURBSId id);
	DllExport int GetParent(int pnum);
	DllExport NURBSId GetParentId(int pnum);
	DllExport void SetFlip(int pnum, BOOL flip);
	DllExport BOOL GetFlip(int pnum);
	DllExport void SetParallel(BOOL para);
	DllExport BOOL GetParallel();
	DllExport void SetScale(BOOL scale);
	DllExport BOOL GetScale();
	DllExport void SetSnapCS(BOOL snapCS);
	DllExport BOOL GetSnapCS();

	DllExport void SetRailParent(int pnum, int index);
	DllExport void SetRailParentId(int pnum, NURBSId id);
	DllExport int GetRailParent(int pnum);
	DllExport NURBSId GetRailParentId(int pnum);

	DllExport void SetCurveStartPoint(TimeValue t, int pnum, double startpoint);
	DllExport double GetCurveStartPoint(TimeValue t, int pnum);
};



class NURBSMultiCurveTrimSurface : public NURBSSurface {
	friend class NURBSSet;
	Tab<NURBSId> mParentId;
	Tab<int> mParentIndex;
	NURBSId mSurfaceId;
	int mSurfaceIndex;
	BOOL mFlipTrim;
	DllExport void Clean(NURBSIdTab ids);
public:
	DllExport NURBSMultiCurveTrimSurface(void);
	DllExport virtual ~NURBSMultiCurveTrimSurface(void);
	DllExport NURBSMultiCurveTrimSurface & operator=(const NURBSMultiCurveTrimSurface& surf);

	DllExport void SetNumCurves(int num);
	DllExport int GetNumCurves(void);
	DllExport int AppendCurve(int index);
	DllExport int AppendCurve(NURBSId id);
	DllExport void SetParent(int pnum, int index);
	DllExport void SetParentId(int pnum, NURBSId id);
	DllExport int GetParent(int pnum);
	DllExport NURBSId GetParentId(int pnum);

	DllExport void SetSurfaceParent(int index);
	DllExport void SetSurfaceParentId(NURBSId id);
	DllExport int GetSurfaceParent();
	DllExport NURBSId GetSurfaceParentId();

	DllExport BOOL GetFlipTrim();
	DllExport void SetFlipTrim(BOOL flip);
};



class NURBSFilletSurface : public NURBSSurface {
	friend class NURBSSet;
	// parent 0 should be the surface parent 1 should be the curve
	NURBSId mParentId[2];
	int mParentIndex[2];
	BOOL mCubic;
	float mRadius[2];
	Point2 mSeed[2];
    BOOL mTrimSurface[2];
    BOOL mFlipTrim[2];
	DllExport void Clean(NURBSIdTab ids);
public:
	DllExport NURBSFilletSurface(void);
	DllExport virtual ~NURBSFilletSurface(void);
	DllExport NURBSFilletSurface & operator=(const NURBSFilletSurface& curve);
	DllExport void SetParent(int pnum, int index);
	DllExport void SetParentId(int pnum, NURBSId id);
	DllExport int GetParent(int pnum);
	DllExport NURBSId GetParentId(int pnum);
	DllExport Point2 GetSeed(int pnum);
	DllExport void SetSeed(int pnum, Point2& seed);
	DllExport BOOL IsCubic();
	DllExport void SetCubic(BOOL cubic);
	DllExport float GetRadius(TimeValue t, int rnum);
	DllExport void SetRadius(TimeValue t, int rnum, float radius);

    DllExport BOOL GetTrimSurface(int pnum);
    DllExport void SetTrimSurface(int pnum, BOOL trim);
    DllExport BOOL GetFlipTrim(int pnum);
    DllExport void SetFlipTrim(int pnum, BOOL flip);
};



class NURBSDisplay {
public:
	DllExport NURBSDisplay();
	DllExport NURBSDisplay & operator=(const NURBSDisplay& disp);

	BOOL mDisplayCurves;
	BOOL mDisplaySurfaces;
	BOOL mDisplayLattices;
	BOOL mDisplaySurfCVLattices;
	BOOL mDisplayCurveCVLattices;
	BOOL mDisplayDependents;
	BOOL mDisplayTrimming;
	BOOL mDegradeOnMove;
    BOOL mDisplayShadedLattice;
};



class NURBSFuseSurfaceCV {
public:
	DllExport NURBSFuseSurfaceCV();
	int mSurf1, mSurf2;
	int mRow1, mCol1, mRow2, mCol2;
};


class NURBSFuseCurveCV {
public:
	DllExport NURBSFuseCurveCV();
	int mCurve1, mCurve2;
	int mCV1, mCV2;
};

class NURBSSet {
protected:
	friend DllExport Object* CreateNURBSObject(IObjParam* ip, NURBSSet *nset, Matrix3& mat);
	friend DllExport int AddNURBSObjects(Object* MAXobj, IObjParam* ip, NURBSSet *nset);
    friend DllExport BOOL GetNURBSSet(Object *object, TimeValue t, NURBSSet &nset, BOOL Relational);
	TessApprox *mpVTess;
	TessApprox *mpRTess;
	// new for R3 -- optional
	TessApprox *mpRTessDisp;
	TessApprox *mpVTessCurve;
	TessApprox *mpRTessCurve;

	float mTessMerge;
	Tab<NURBSObject*> mObjects;
	Object *mpObject;
	NURBSDisplay mDisplay;

public:
	DllExport NURBSSet(void);
	DllExport virtual ~NURBSSet(void);
	DllExport void Clean();	// this method removes any relation to a live NURBS object
	DllExport int GetNumObjects();
	DllExport void SetObject(int index, NURBSObject* obj);
	DllExport int AppendObject(NURBSObject* obj);
	DllExport void RemoveObject(int index);
	DllExport void DeleteObjects();
	DllExport NURBSObject* GetNURBSObject(int index);
	DllExport NURBSObject* GetNURBSObject(NURBSId id);
	DllExport TessApprox* GetProdTess(NURBSTessType type=kNTessSurface);
	DllExport TessApprox* GetViewTess(NURBSTessType type=kNTessSurface);
	DllExport void SetProdTess(TessApprox& tess, NURBSTessType type=kNTessSurface);
	DllExport void SetViewTess(TessApprox& tess, NURBSTessType type=kNTessSurface);
	DllExport void ClearViewTess(NURBSTessType type=kNTessSurface);
	DllExport void ClearProdTess(NURBSTessType type=kNTessSurface);
	DllExport float GetTessMerge();
	DllExport void SetTessMerge(float merge);
	DllExport Object* GetMAXObject();
	DllExport NURBSDisplay GetDisplaySettings();
	DllExport void SetDisplaySettings(NURBSDisplay& disp);


	Tab<NURBSFuseSurfaceCV> mSurfFuse;
	Tab<NURBSFuseCurveCV> mCurveFuse;
};




typedef NURBSResult (*SurfParamRangeProc)(double& uMin, double& uMax, double& vMin, double& vMax);
typedef NURBSResult (*SurfEvalProc)(double u, double v, Point3& pt);
typedef NURBSResult (*SurfEvalTan)(double u, double v, Point3& uTan, Point3& vTan);
typedef NURBSResult (*SurfEvalMixedProc)(double u, double v, Point3& mixed);

// base class for a proceedurally defined surfaces
// NOTE THIS IS NOT SUBCLASSED FROM NURBSObject
// You must use the GenNURBSCVSurfaceProcedurally
class NURBSProceeduralSurface {
public:
	SurfParamRangeProc	mParamProc;		// this one MUST be implemented
	SurfEvalProc		mEvalProc;		// this one MUST be implemented
	SurfEvalTan			mEvalTanProc;	// this one is optional
	SurfEvalMixedProc	mEvalMixedProc; // this one is optional
	DllExport NURBSProceeduralSurface(SurfParamRangeProc param, SurfEvalProc eval,
							SurfEvalTan tan, SurfEvalMixedProc mixed);
};
DllExport NURBSResult GenNURBSCVSurfaceProceedurally(NURBSProceeduralSurface *pSurf, double tolerence, NURBSCVSurface& surf);



typedef NURBSResult (*CurveParamRangeProc)(double& tMin, double& tMax);
typedef NURBSResult (*CurveEvalProc)(double u, Point3& pt);
typedef NURBSResult (*CurveEvalTan)(double u, Point3& pt, Point3& tan);
typedef NURBSResult (*CurveArcLengthProc)(double& arcLength);
// base class for a proceedurally defined curves
// NOTE THIS IS NOT SUBCLASSED FROM NURBSObject
// You must use the GenNURBSCVCurveProcedurally
class NURBSProceeduralCurve {
public:
	CurveParamRangeProc	mParamProc;		// this one MUST be implemented
	CurveEvalProc		mEvalProc;		// this one MUST be implemented
	CurveEvalTan		mEvalTanProc;	// this one is optional
	CurveArcLengthProc	mArcLengthProc; // this one is optional
	DllExport NURBSProceeduralCurve(CurveParamRangeProc param, CurveEvalProc eval,
							CurveEvalTan tan, CurveArcLengthProc arclen);
};

DllExport NURBSResult GenNURBSCVCurveProceedurally(NURBSProceeduralCurve *pCrv, double tolerence, NURBSCVCurve& crv);






DllExport NURBSResult GenNURBSLatheSurface(NURBSCVCurve& curve, Point3& origin, Point3& north,
									float start, float end, NURBSCVSurface& surf);
DllExport NURBSResult GenNURBSSphereSurface(float radius, Point3& center, Point3& northAxis, Point3& refAxis,
					float startAngleU, float endAngleU, float startAngleV, float endAngleV, BOOL open, NURBSCVSurface& surf);
DllExport NURBSResult GenNURBSCylinderSurface(float radius, float height, Point3& origin, Point3& symAxis, Point3& refAxis,
					float startAngle, float endAngle, BOOL open, NURBSCVSurface& surf);
DllExport NURBSResult GenNURBSConeSurface(float radius1, float radius2, float height, Point3& origin, Point3& symAxis, Point3& refAxis,
					float startAngle, float endAngle, BOOL open, NURBSCVSurface& surf);
DllExport NURBSResult GenNURBSTorusSurface(float majorRadius, float minorRadius, Point3& origin,
					Point3& symAxis, Point3& refAxis, float startAngleU, float endAngleU,
					float startAngleV, float endAngleV, BOOL open, NURBSCVSurface& surf);

DllExport Object *CreateNURBSObject(IObjParam* ip, NURBSSet *nset, Matrix3& mat);
DllExport int AddNURBSObjects(Object* obj, IObjParam* ip, NURBSSet *nset);

DllExport Object *CreateNURBSLatheShape(IObjParam* ip, TSTR name, TimeValue t, ShapeObject *shape,
                     Matrix3& axis, float degrees, int capStart, int capEnd,
                     int capType, BOOL weldCore, BOOL flipNormals, BOOL texturing,
					 int segs, BOOL matIds, BOOL shapeIDs);
DllExport Object *CreateNURBSExtrudeShape(IObjParam* ip, TSTR name, TimeValue t, ShapeObject *shape, float amount,
					   int capStart, int capEnd, int capType, BOOL texturing,
					   BOOL matIds, BOOL shapeIDs);

DllExport BOOL GetNURBSSet(Object *object, TimeValue t, NURBSSet &nset, BOOL Relational);



// modify extant objects
DllExport NURBSResult SetSurfaceApprox(Object* obj, BOOL viewport, TessApprox *tess, BOOL clearSurfs=FALSE);
DllExport NURBSResult SetCurveApprox(Object* obj, BOOL viewport, TessApprox *tess, BOOL clearSurfs);
DllExport NURBSResult SetDispApprox(Object* obj, TessApprox *tess, BOOL clearSurfs);
DllExport NURBSResult SetSurfaceDisplaySettings(Object* obj, NURBSDisplay& disp);
DllExport NURBSResult GetSurfaceDisplaySettings(Object* obj, NURBSDisplay& disp);

DllExport NURBSResult Transform(Object* obj, NURBSIdTab& ids, SetXFormPacket& xPack, Matrix3& mat, TimeValue t);

DllExport NURBSResult BreakCurve(Object* obj, NURBSId id, double u, TimeValue t);
DllExport NURBSResult BreakSurface(Object* obj, NURBSId id, BOOL breakU, double param, TimeValue t);

DllExport NURBSResult JoinCurves(Object* obj, NURBSId id1, NURBSId id2, BOOL begin1, BOOL begin2,
								 double tolerance, double ten1, double ten2, TimeValue t);
DllExport NURBSResult JoinSurfaces(Object* obj, NURBSId id1, NURBSId id2, int edge1, int edge2,
								   double tolerance, double ten1, double ten2, TimeValue t);

DllExport NURBSResult ZipCurves(Object* obj, NURBSId id1, NURBSId id2, BOOL begin1, BOOL begin2,
								 double tolerance, TimeValue t);
DllExport NURBSResult ZipSurfaces(Object* obj, NURBSId id1, NURBSId id2, int edge1, int edge2,
								   double tolerance, TimeValue t);

DllExport NURBSId MakeIndependent(Object* obj, NURBSId id, TimeValue t);
DllExport NURBSId MakeRigid(Object* obj, NURBSId id, TimeValue t);
DllExport void SetApproxPreset(Object* pObj, int i);
DllExport void ToggleShadedLattice(Object* pObj);
DllExport TessApprox* GetTessPreset(int which, int preset);
DllExport void SetTessPreset(int which, int preset, TessApprox& tess);

DllExport Object *BuildEMObjectFromLofterObject(Object *loftObject, double tolerance);
DllExport Object *BuildEMObjectFromPatchObject(Object *patchObject);

typedef Tab<NURBSId> NURBSIdList;

DllExport Object *DetachObjects(TimeValue t, INode *pNode, Object* pobj, NURBSIdList list, char *newObjName, BOOL copy, BOOL relational);


DllExport NURBSSubObjectLevel GetSelectionLevel(Object* pObj);
DllExport NURBSResult SetSelectionLLevel(Object* pObj, NURBSSubObjectLevel level);

DllExport NURBSResult GetSelection(Object* pObj, NURBSSubObjectLevel level,
                                   BitArray& selset);

DllExport NURBSResult SetSelection(Object* pObj, NURBSSubObjectLevel level,
                                   BitArray& selset);

DllExport NURBSResult
MoveCurrentSelection(Object* pObj, NURBSSubObjectLevel level,
                     TimeValue t, Matrix3& partm, Matrix3& tmAxis,
                     Point3& val, BOOL localOrigin);

DllExport NURBSResult
RotateCurrentSelection(Object* pObj, NURBSSubObjectLevel level,
                       TimeValue t, Matrix3& partm, Matrix3& tmAxis,
                       Quat& val, BOOL localOrigin);

DllExport NURBSResult 
ScaleCurrentSelection(Object* pObj, NURBSSubObjectLevel level,
                      TimeValue t, Matrix3& partm, Matrix3& tmAxis,
                      Point3& val, BOOL localOrigin);


// Get the number of sub-objects at a particular level
DllExport int
SubObjectCount(Object* pObj, NURBSSubObjectLevel level);

// number of named sets at a particular level
DllExport int 
NamedSelSetCount(Object* pObj, NURBSSubObjectLevel level);

// Get ith named sel set name
DllExport TCHAR* 
GetNamedSelSetName(Object* pObj, NURBSSubObjectLevel level, int i);  

// Set the bit array to the named selection set
DllExport NURBSResult 
GetNamedSelSet(Object* pObj, NURBSSubObjectLevel level, TCHAR* name, BitArray& selSet);

// Set the named selection set the selection in BitArray
DllExport NURBSResult 
SetNamedSelSet(Object* pObj, NURBSSubObjectLevel level, TCHAR* name, BitArray& sel); 

// Add a new named selection set
DllExport NURBSResult
AppendNamedSelSet(Object* pObj, NURBSSubObjectLevel level, TCHAR* name, BitArray& sel); 

DllExport NURBSResult
DeleteCurrentSelection(Object* pObj, NURBSSubObjectLevel level);

DllExport NURBSResult
MapNURBSIdToSelSetIndex(Object* pObj, NURBSId id, int& index, NURBSSubObjectLevel& level);

DllExport NURBSResult
MapSelSetIndexToNURBSId(Object* pObj, int index, NURBSSubObjectLevel level, NURBSId& id);

DllExport void 
ApplyUVWMapAsTextureSurface(Object* pObj, int type, float utile, float vtile,
                            float wtile, int uflip, int vflip, int wflip, int cap,
                            const Matrix3 &tm,int channel);

// Has the same affect as the "Update" button on the "Surface Mapper" WSM.
DllExport void UpdateSurfaceMapper(Modifier* pMod);

#endif
