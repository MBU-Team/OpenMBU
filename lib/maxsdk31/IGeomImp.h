/**********************************************************************
 *<
	FILE: IGeomImp.h

	DESCRIPTION: Declaration of the interface which a puppet must implement.
				A puppet is essentially a geometry machine with a series of 
				set poses and operations. 
	CREATED BY: John Hutchinson

	HISTORY: Dec 9, 1997

 *>	Copyright (c) 1997, All Rights Reserved.
 **********************************************************************/
#ifndef __IGEOMPUPPET_H
#define __IGEOMPUPPET_H
#include "export.h"
#include <windows.h>
#include "maxtypes.h"

//FIX ME
//THis should not depend on GE
//but for now we use some of its classes
class AcGePoint3d;
class AcGeVector3d;
class AcGePlane;
class AcGePoint2d;
class AcGeVector2d;


#define OP_TIMING

//FIX ME
//This shouldn't depend on amodeler
//but for now we use some of its classes
//JH 3/9/99 Removing references to vertexdata
//#include "../msdkamodeler/inc/vertdata.h"

//need the mesh class for TriangulateMesh
#include "max.h"
class INode;
class ViewExp;

extern inline AcGePoint3d* MaxArToGeAr(const Point3* maxPt, int numverts);

#define MAX_MESH_ID			0x00000001
#define AMODELER_BODY_ID	0x00000002
class OpTimer;
class IGeomImp {
	protected:

	public:
	static DllExport OpTimer opstatistics; //exported from core, initialized in hostcomp.cpp
	//access to the actual internal geometry representation
//	virtual void SetInternalRepresentation(LONG RepID, void *rep);
	virtual void Init(void *prep = NULL, LONG RepID = -1) = 0;
	virtual void *GetInternalRepresentation(LONG RepID, bool* needsdelete)= 0;// access to the wrapped cache
	virtual LONG NativeRepresentationID() = 0;
	///////////////////////////////////////////////////////////////////////////
    // 
    // pseudoconstructors
    // 
	//JH In the process of overloading these for ease of use in MAX 03/26/99

    virtual bool createBox(const Point3& p) = 0;
    virtual bool createBox(const AcGePoint3d& p, const AcGeVector3d& vec) = 0;

    virtual bool createSphere(float radius, int sides, BOOL smooth = TRUE) = 0;
    virtual bool createSphere(const AcGePoint3d& p, double radius, int approx) = 0;

    virtual bool createCylinder(float height, float radius, int sides , BOOL smooth = TRUE) = 0;
    virtual bool createCylinder(const AcGePoint3d& axisStart, const AcGePoint3d& axisEnd, 
         double radius, int approx) = 0;

    virtual bool createCone(float height, float radius1, float radius2, int sides , BOOL smooth = TRUE) = 0;
    virtual bool createCone(const AcGePoint3d& axisStart, const AcGePoint3d& axisEnd, 
        const AcGeVector3d& baseNormal, double radius1, double radius2, int approx) = 0;

    virtual bool createPipe(float height, float radius1, float radius2, int sides , BOOL smooth = TRUE) = 0;
    virtual bool createPipe(const AcGePoint3d& axisStart, const AcGePoint3d& axisEnd, 
        const AcGeVector3d& baseNormal,
        double dblOuterRadius, double dblInnerRadius, 
        int approx) = 0;

	virtual void createPipeConic(const AcGePoint3d& axisStart, const AcGePoint3d& axisEnd,
        const AcGeVector3d& baseNormal,
		double outterRadius1, double innerRadius1,
		double outterRadius2, double innerRadius2, 
        int approx) = 0;

	virtual void createTetrahedron(const AcGePoint3d& p1, const AcGePoint3d& p2,
		const AcGePoint3d& p3, const AcGePoint3d& p4) = 0;

	virtual bool createTorus(float majorRadius, float minorRadius, int sidesa, int segs, BOOL smooth = TRUE) = 0;
	virtual bool createTorus(const AcGePoint3d& axisStart, const AcGePoint3d& axisEnd,
		double majorRadius, double minorRadius, int majorApprox, int minorApprox) = 0;
/*
	virtual void createReducingElbow(const AcGePoint3d& elbowCenter, const AcGePoint3d& endCenter1,
        const AcGePoint3d& endCenter2, double endRadius1, double endRadius2,
        int majorApprox, int minorApprox) = 0;
	virtual void createRectToCircleReducer(
        const AcGePoint3d& baseCorner,
        const AcGeVector2d& baseSizes, 
        const AcGePoint3d& circleCenter,
        const AcGeVector3d& circleNormal, 
        double circleRadius, 
        int approx) = 0;
*/
	virtual bool createConvexHull(const AcGePoint3d vertices[], int numVertices) = 0;
	virtual bool createConvexHull(const Point3 vertices[], int numVertices) = 0;

    // Create a body consisting of one face
    //
/*
    virtual void createFace(const AcGePoint3d vertices[], AModeler::PolygonVertexData* vertexData[], 
        int numVertices, const AcGeVector3d &normal) = 0;
*/
    virtual void createFace(const AcGePoint3d vertices[], int numVertices, BOOL smooth = TRUE) = 0;

	//The boolean operators
	virtual void prepBoolean() = 0;
	virtual void operator +=(IGeomImp& b) = 0; 
    virtual void operator -=(IGeomImp& b) = 0; 
    virtual void operator *=(IGeomImp& b) = 0; 
    virtual int interfere  (IGeomImp& b) = 0;

	//combination
	virtual bool Combine(IGeomImp& b) = 0; 

	//assignment
	virtual IGeomImp& operator =(IGeomImp& b) = 0;

    
	// The section method removes part of the body in the positive halfplane
    //
	virtual void  section(const Matrix3& tm) = 0;
    virtual void  section(const AcGePlane& p) = 0; 

    // Sweeps
    //
	virtual bool createPyramid(float width, float depth, float height)=0;
	virtual bool createPyramid(
        const AcGePoint3d vertices[], 
//        AModeler::PolygonVertexData* vertexData[],
	    int numVertices, 
        const AcGeVector3d &plgNormal, 
        const AcGePoint3d &apex) = 0;

//	virtual bool createExtrusion(const Point3 vertices[], int numVertices, const float height )=0;
	virtual bool createExtrusion(		
		PolyPt vertices[], 
		int numVertices, 
		const float height,
		const BOOL smooth,
		const BOOL genMatIDs,
		const BOOL useShapeIDs)=0;

	virtual bool createExtrusion(
        const AcGePoint3d vertices[], 
//        AModeler::PolygonVertexData* vertexData[],
		int numVertices, 
        const AcGeVector3d &plgNormal, 
        const AcGeVector3d &extusionVector, 
        const AcGePoint3d &fixedPt, 
        double scaleFactor, 
        double twistAngle) = 0;

    virtual void createAxisRevolution(
        const AcGePoint3d vertices[],
//        AModeler::PolygonVertexData* vertexData[],
        int numVertices,
        const AcGeVector3d &normal,
        const AcGePoint3d &axisStart,
        const AcGePoint3d &axisEnd,
        double revolutionAngle,
        int approx,
        const AcGePoint3d &fixedPt,
        double scaleFactor,
        double twistAngle) = 0;

    virtual void createEndpointRevolution(
        const AcGePoint3d vertices[],
//        AModeler::PolygonVertexData* vertexData[],
		int numVertices,
        const AcGeVector3d &normal,
        double revolutionAngle,
        int approx)=0;
/*
    void createSkin(
        AsdkBody* profiles[],
        int numProfiles,
        bool isClosed,
        MorphingMap* morphingMaps[]);
*/

	virtual void createExtrusionAlongPath(
		const AcGePoint3d vertices[], 
//        AModeler::PolygonVertexData* vertexData[], 
		int numVerticesm, 
        bool pathIsClosed, 
        bool bCheckValidity,
        const AcGePoint3d &scaleTwistFixedPt, 
        double scaleFactor, 
        double twistAngle,
		const AcGeVector3d &extusionVector) = 0;

	virtual void createWallCorner(
                    const AcGePoint2d& pt1,     // Start of wall 1
                    const AcGePoint2d& pt2,     // End of wall 1, start of wall 2
                    const AcGePoint2d& pt3,     // End of wall 2
                    bool           materialToTheLeft,
                    double         width1,  // Wall 1 width
                    double         width2,  // Wall 2 width
                    double         height,  // Wall height
					AcGePlane&         matingPlane1,
					AcGePlane&         matingPlane2,
                    bool&          wall1NeedsToBeSectioned,
                    bool&          wall2NeedsToBeSectioned) = 0;

	///////////////////////////////////////////////////////////////////////////
	//
	// transforms
	//

	virtual void transform(Matrix3 & mat, bool override_locks = true) = 0;


    ///////////////////////////////////////////////////////////////////////////
    // 
    // triangulation
    // 

	
	// Saves the triangles directly to a Mesh //
	virtual void triangulateMesh(TimeValue t, Mesh& m, 
	TesselationType type = kTriangles, bool cacheTriangles    = true)  = 0;

/* TO DO
	// saves the triangle back to the callback object 
	virtual void triangulate   (TimeValue t, OutputTriangleCallback*, 
	TriangulationType type = kGenerateTriangles, bool cacheTriangles    = true) = 0;
*/
    ///////////////////////////////////////////////////////////////////////////
    // 
    // display
    // 
	virtual void Display(TimeValue t, INode* inode, ViewExp *vpt, int flags) = 0;
	virtual void Invalidate() = 0;

	//Validity
	virtual bool isNull() = 0;

	//////////////////////////////////////////////////////////////////////////
	//
	// Copy
	//
	virtual IGeomImp * Copy(void) = 0;

	//////////////////////////////////////////////////////////////////////////
	//
	// BondingBox methods
	//
	virtual void GetDeformBBox(TimeValue t, Box3& box, Matrix3 *tm=NULL, BOOL useSel=FALSE ) = 0;


	//////////////////////////////////////////////////////////////////////////
	//
	// Constraining transform
	//
	virtual bool GetTransformLock(int type, int axis) = 0;
	virtual void SetTransformLock(int type, int axis, BOOL onOff) = 0;
};

//#ifdef IMPORTING
//#error
//#endif

#define HIRES

class OpTimer: public MeshOpProgress
{
private:
#ifdef HIRES
	LARGE_INTEGER  opstart;
	LARGE_INTEGER  optime;
	LARGE_INTEGER  cumtime;
	LARGE_INTEGER  res;
#else
	int opstart, optime, cumtime;
#endif
	int m_errorcode;
	int m_errcount;
	int m_opcount;
	TSTR curopname;
public:
	//From MeshOpProgress
	OpTimer():m_errorcode(0), m_opcount(0){curopname = "??";}
	void Init(int total)
	{
		++m_opcount;
		m_errorcode = 0;
		
#ifdef HIRES
		QueryPerformanceFrequency(&res);
		optime.QuadPart = 0;
		QueryPerformanceCounter(&opstart);
#else
		opstart = GetTickCount();
#endif
	}
	BOOL Progress(int p)
	{
#ifdef HIRES
		LARGE_INTEGER opnow;
		QueryPerformanceCounter(&opnow);
		cumtime.QuadPart += (opnow.QuadPart - (opstart.QuadPart + optime.QuadPart));
		optime.QuadPart = opnow.QuadPart - opstart.QuadPart;
#else
		integer opnow = GetTickCount();
		cumtime += (opnow - (opstart + optime));
		optime = opnow - opstart; 
#endif
		return true;
	}
	void OutputMetrics(HWND hDlg);
	void InitName(char *opname){curopname = opname;}
	void ExtendName(char *opname){curopname += opname;}
	void FlagError(int errcode = 999){m_errorcode = errcode;++m_errcount;}
	void Reset(){
		m_errcount = 0; m_opcount=0;
#ifdef HIRES
		cumtime.QuadPart = 0;
#else
		optime = 0; 
#endif

	}
};

extern "C" __declspec(dllexport) IGeomImp* CreateAmodelerBody();
extern "C" __declspec(dllexport) IGeomImp* CreateMeshAdapter();
#endif //__IGEOMPUPPET_H