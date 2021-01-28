/**********************************************************************
 *<
	FILE: impapi.h

	DESCRIPTION: Geometry import/export API header

	CREATED BY:	Tom Hudson

	HISTORY: Created 26 December 1994

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef _IMPAPI_H_
#define _IMPAPI_H_

// These includes get us the general camera and light interfaces
#include "gencam.h"
#include "genlight.h"

#include "render.h"

// Import Node class

class ImpNode {
public:
	virtual RefResult	Reference(ObjectHandle obj)	= 0;
	virtual void		SetTransform( TimeValue t, Matrix3 tm ) = 0;
	virtual void 		SetName(const TCHAR *newname) = 0;
	virtual void		SetPivot(Point3 p) = 0;
	virtual INode *		GetINode()=0;			// Use with care -- Always use above methods instead
	// I'm stuffing these in here so that I can perhaps add API functions without recompiling
	virtual int			TempFunc1(void *p1=NULL, void *p2=NULL, void *p3=NULL, void *p4=NULL, void *p5=NULL, void *p6=NULL, void *p7=NULL, void *p8=NULL)=0;
	virtual int			TempFunc2(void *p1=NULL, void *p2=NULL, void *p3=NULL, void *p4=NULL, void *p5=NULL, void *p6=NULL, void *p7=NULL, void *p8=NULL)=0;
	virtual int			TempFunc3(void *p1=NULL, void *p2=NULL, void *p3=NULL, void *p4=NULL, void *p5=NULL, void *p6=NULL, void *p7=NULL, void *p8=NULL)=0;
	virtual int			TempFunc4(void *p1=NULL, void *p2=NULL, void *p3=NULL, void *p4=NULL, void *p5=NULL, void *p6=NULL, void *p7=NULL, void *p8=NULL)=0;
	virtual int			TempFunc5(void *p1=NULL, void *p2=NULL, void *p3=NULL, void *p4=NULL, void *p5=NULL, void *p6=NULL, void *p7=NULL, void *p8=NULL)=0;
	virtual int			TempFunc6(void *p1=NULL, void *p2=NULL, void *p3=NULL, void *p4=NULL, void *p5=NULL, void *p6=NULL, void *p7=NULL, void *p8=NULL)=0;
	virtual int			TempFunc7(void *p1=NULL, void *p2=NULL, void *p3=NULL, void *p4=NULL, void *p5=NULL, void *p6=NULL, void *p7=NULL, void *p8=NULL)=0;
	virtual int			TempFunc8(void *p1=NULL, void *p2=NULL, void *p3=NULL, void *p4=NULL, void *p5=NULL, void *p6=NULL, void *p7=NULL, void *p8=NULL)=0;
	virtual int			TempFunc9(void *p1=NULL, void *p2=NULL, void *p3=NULL, void *p4=NULL, void *p5=NULL, void *p6=NULL, void *p7=NULL, void *p8=NULL)=0;
	virtual int			TempFunc10(void *p1=NULL, void *p2=NULL, void *p3=NULL, void *p4=NULL, void *p5=NULL, void *p6=NULL, void *p7=NULL, void *p8=NULL)=0;
	};

// Import Interface class

class ImpInterface {
public:
	virtual ImpNode *		CreateNode() = 0;
	virtual void 			RedrawViews() = 0;
	virtual GenCamera*	 	CreateCameraObject(int type) = 0;
	virtual Object *   		CreateTargetObject() = 0;
	virtual GenLight*	 	CreateLightObject(int type) = 0;
	virtual void *			Create(SClass_ID sclass, Class_ID classid)=0;
	virtual int 			BindToTarget(ImpNode *laNode, ImpNode *targNode)=0;
	virtual void			AddNodeToScene(ImpNode *node)=0;
	virtual void			SetAnimRange(Interval& range)=0;
	virtual Interval		GetAnimRange()=0;
	// Environment settings
	virtual void 			SetEnvironmentMap(Texmap *txm)=0;
	virtual void 			SetAmbient(TimeValue t, Point3 col)=0;
	virtual void 			SetBackGround(TimeValue t,Point3 col)=0;
	virtual void 			SetUseMap(BOOL onoff)=0;
	virtual void 			AddAtmosphere(Atmospheric *atmos)=0;

	virtual int				NewScene()=0;  // delete all existing geometry
	// I'm stuffing these in here so that I can perhaps add API functions without recompiling
	virtual int				TempFunc1(void *p1=NULL, void *p2=NULL, void *p3=NULL, void *p4=NULL, void *p5=NULL, void *p6=NULL, void *p7=NULL, void *p8=NULL)=0;
	virtual int				TempFunc2(void *p1=NULL, void *p2=NULL, void *p3=NULL, void *p4=NULL, void *p5=NULL, void *p6=NULL, void *p7=NULL, void *p8=NULL)=0;
	virtual int				TempFunc3(void *p1=NULL, void *p2=NULL, void *p3=NULL, void *p4=NULL, void *p5=NULL, void *p6=NULL, void *p7=NULL, void *p8=NULL)=0;
	virtual int				TempFunc4(void *p1=NULL, void *p2=NULL, void *p3=NULL, void *p4=NULL, void *p5=NULL, void *p6=NULL, void *p7=NULL, void *p8=NULL)=0;
	virtual int				TempFunc5(void *p1=NULL, void *p2=NULL, void *p3=NULL, void *p4=NULL, void *p5=NULL, void *p6=NULL, void *p7=NULL, void *p8=NULL)=0;
	virtual int				TempFunc6(void *p1=NULL, void *p2=NULL, void *p3=NULL, void *p4=NULL, void *p5=NULL, void *p6=NULL, void *p7=NULL, void *p8=NULL)=0;
	virtual int				TempFunc7(void *p1=NULL, void *p2=NULL, void *p3=NULL, void *p4=NULL, void *p5=NULL, void *p6=NULL, void *p7=NULL, void *p8=NULL)=0;
	virtual int				TempFunc8(void *p1=NULL, void *p2=NULL, void *p3=NULL, void *p4=NULL, void *p5=NULL, void *p6=NULL, void *p7=NULL, void *p8=NULL)=0;
	virtual int				TempFunc9(void *p1=NULL, void *p2=NULL, void *p3=NULL, void *p4=NULL, void *p5=NULL, void *p6=NULL, void *p7=NULL, void *p8=NULL)=0;
	virtual int				TempFunc10(void *p1=NULL, void *p2=NULL, void *p3=NULL, void *p4=NULL, void *p5=NULL, void *p6=NULL, void *p7=NULL, void *p8=NULL)=0;
	virtual FILE *			DumpFile() = 0;			// For debugging -- Stream for dumping debug messages
	};

// Export Interface class

class ExpInterface {
public:
	IScene *		theScene;		// Pointer to the scene
	};

#endif // _IMPAPI_H_
