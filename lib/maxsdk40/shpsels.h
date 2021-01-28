/**********************************************************************
 *<
	FILE: shpsels.h

	DESCRIPTION:  Defines Shape Selection utility objects

	CREATED BY: Tom Hudson

	HISTORY: created 31 October 1995

 *>	Copyright (c) 1995, All Rights Reserved.
 **********************************************************************/

#ifndef __SHPSELS__ 

#define __SHPSELS__

class BezierShape;
class PolyShape;

class ShapeVSel {
	public:
	int polys;
	BitArray *sel;
	CoreExport ShapeVSel();
	CoreExport ShapeVSel(ShapeVSel& from);
	CoreExport ~ShapeVSel();
	CoreExport ShapeVSel& operator=(ShapeVSel& from);
	CoreExport BOOL operator==(ShapeVSel& s);
	CoreExport void Insert(int where,int count=0);
	CoreExport void Delete(int where);
	CoreExport void SetSize(ShapeVSel& selset, BOOL save=FALSE);
	CoreExport void SetSize(BezierShape& shape, BOOL save=FALSE);
	CoreExport void SetSize(PolyShape& shape, BOOL save=FALSE);
	CoreExport BitArray& operator[](int index);
	CoreExport void ClearAll();
	CoreExport void SetAll();
	CoreExport void Toggle();
	CoreExport void Empty();
	CoreExport BOOL IsCompatible(ShapeVSel& selset);
	CoreExport BOOL IsCompatible(BezierShape& shape);
	CoreExport BOOL IsCompatible(PolyShape& shape);
	CoreExport IOResult Save(ISave* isave);
	CoreExport IOResult Load(ILoad* iload);
	};

class ShapeSSel {
	public:
	int polys;
	BitArray *sel;
	CoreExport ShapeSSel();
	CoreExport ShapeSSel(ShapeSSel& from);
	CoreExport ~ShapeSSel();
	CoreExport ShapeSSel& operator=(ShapeSSel& from);
	CoreExport BOOL operator==(ShapeSSel& s);
	CoreExport void Insert(int where,int count=0);
	CoreExport void Delete(int where);
	CoreExport void SetSize(ShapeSSel& selset, BOOL save=FALSE);
	CoreExport void SetSize(BezierShape& shape, BOOL save=FALSE);
	CoreExport void SetSize(PolyShape& shape, BOOL save=FALSE);
	CoreExport BitArray& operator[](int index);
	CoreExport void ClearAll();
	CoreExport void SetAll();
	CoreExport void Toggle();
	CoreExport void Empty();
	CoreExport BOOL IsCompatible(ShapeSSel& selset);
	CoreExport BOOL IsCompatible(BezierShape& shape);
	CoreExport BOOL IsCompatible(PolyShape& shape);
	CoreExport IOResult Save(ISave* isave);
	CoreExport IOResult Load(ILoad* iload);
	};

class ShapePSel {
	public:
	int polys;
	BitArray sel;
	CoreExport ShapePSel();
	CoreExport ShapePSel(ShapePSel& from);
	CoreExport ~ShapePSel();
	CoreExport ShapePSel& operator=(ShapePSel& from);
	CoreExport BOOL operator==(ShapePSel& s);
	CoreExport void Insert(int where);
	CoreExport void Delete(int where);
	CoreExport void SetSize(ShapePSel& selset, BOOL save=FALSE);
	CoreExport void SetSize(BezierShape& shape, BOOL save=FALSE);
	CoreExport void SetSize(PolyShape& shape, BOOL save=FALSE);
	CoreExport void Set(int index);
	CoreExport void Set(int index, int value);
	CoreExport void Clear(int index);
	CoreExport int operator[](int index) const;
	CoreExport void ClearAll();
	CoreExport void SetAll();
	CoreExport void Toggle();
	CoreExport void Empty();
	CoreExport BOOL IsCompatible(ShapePSel& selset);
	CoreExport BOOL IsCompatible(BezierShape& shape);
	CoreExport BOOL IsCompatible(PolyShape& shape);
	CoreExport IOResult Save(ISave* isave);
	CoreExport IOResult Load(ILoad* iload);
	};


#endif __SHPSELS__
