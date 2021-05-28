/**********************************************************************
 *<
	FILE: box2.h

	DESCRIPTION:

	CREATED BY: Dan Silva

	HISTORY:

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef _BOX2_H 

#define _BOX2_H 

#include "ipoint2.h"
#include "point2.h"
#include <windef.h>


class Box2: public RECT {
	public:
	DllExport Box2();
	DllExport Box2(const IPoint2 a, const IPoint2 b);
	DllExport int IsEmpty();
	DllExport void SetEmpty();
	DllExport void Rectify();   // makes top<bottom, left<right
	DllExport void Scale(float f);
	DllExport void Translate(IPoint2 t);

	IPoint2 GetCenter() { return IPoint2((left+right)/2, (top+bottom)/2); }
	int x() { return min(left,right); }
	int y() { return min(top,bottom); }
	int w() { return abs(right-left)+1; }
	int h() { return abs(bottom-top)+1; }
	
	void SetW(int w) { right = left + w -1; } 
	void SetH(int h) { bottom = top + h -1; } 
	void SetX(int x) { left = x; }
	void SetY(int y) { top = y; }
	void SetWH(int w, int h) { SetW(w); SetH(h); }
	void SetXY(int x, int y) { SetX(x); SetY(y); }

	DllExport Box2& operator=(const RECT& r);
	DllExport Box2& operator=(RECT& r);
	DllExport Box2& operator+=(const Box2& b);
	DllExport Box2& operator+=(const IPoint2& p);
	int operator==( const Box2& b ) const { 	return (left==b.left && right==b.right && top==b.top && bottom==b.bottom); }
	DllExport int Contains(const IPoint2& p) const;  // is point in this box?
	};

typedef Box2 Rect;


struct FBox2 {
	Point2 pmin;
	Point2 pmax;
	int IsEmpty() { return pmin.x>pmax.x?1:0; }
	void SetEmpty() { pmin = Point2(1E30,1E30); pmax = -pmin; }
	FBox2& operator=(const FBox2& r) { pmin = r.pmin; pmax = r.pmax; return *this; }
	DllExport FBox2& operator+=(const Point2& p);
	DllExport FBox2& operator+=(const FBox2& b);
	DllExport int Contains(const Point2& p) const;  // is point in this box?
	};

#endif
