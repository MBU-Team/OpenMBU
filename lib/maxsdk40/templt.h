
/**********************************************************************
 *<
	FILE: templt.h

	DESCRIPTION:  Defines 2D Template Object

	CREATED BY: Tom Hudson

	HISTORY: created 31 October 1995

 *>	Copyright (c) 1995, All Rights Reserved.
 **********************************************************************/

#ifndef __TEMPLT_H__ 

#define __TEMPLT_H__

class PolyLine;
class Spline3D;

// Intersection callbacks

class IntersectionCallback2D {
	public:
		virtual BOOL Intersect(Point2 p, int piece)=0; // Return FALSE to stop intersect tests
	};

class IntersectionCallback3D {
	public:
		virtual BOOL Intersect(Point3 p, int piece)=0; // Return FALSE to stop intersect tests
	};

// A handy 2D floating-point box class

class Box2D {
	public:
		BOOL empty;
		Point2 min, max;
		Box2D() { empty = TRUE; }
		void SetEmpty() { empty = TRUE; }
		CoreExport Box2D& operator+=(const Point2& p);	// expand this box to include p
	};

// This object is used to test shapes for self-intersection, clockwise status, point
// surrounding and intersection with other templates.  The last and first points will be the
// same if it is closed.

class Template3D;

class Template {
	public:
		int points;
		BOOL closed;
		Point2 *pts;
		Template(Spline3D *spline);
		Template(PolyLine *line);
		Template(Template3D *t3);
		void Create(PolyLine *line);
		~Template();
		int Points() { return points; }
		BOOL SurroundsPoint(Point2& point);
		BOOL IsClockWise();
		BOOL SelfIntersects(BOOL findAll = FALSE, IntersectionCallback2D *cb = NULL);
		BOOL Intersects(Template &t, BOOL findAll = FALSE, IntersectionCallback2D *cb = NULL);
		Box2D Bound();
	};

// This is a version for 3D use -- the various tests (SurroundsPoint, SelfIntersects, etc.
// are all performed on the X and Y coordinates only, discarding Z.  The IntersectionCallback
// returns the intersection point on the template in 3D.

class Template3D {
	private:
		Template *template2D;
	public:
		int points;
		BOOL closed;
		Point3 *pts;
		Template3D(Spline3D *spline);
		Template3D(PolyLine *line);
		void Create(PolyLine *line);
		~Template3D();
		int Points() { return points; }
		BOOL SurroundsPoint(Point2& point);	// 2D test!
		BOOL IsClockWise(); // 2D test!
		BOOL SelfIntersects(BOOL findAll = FALSE, IntersectionCallback3D *cb = NULL);	// 2D test!
		BOOL Intersects(Template3D &t, BOOL findAll = FALSE, IntersectionCallback3D *cb = NULL); // 2D test!
		Box2D Bound();
		Box3 Bound3D();
		void Ready2DTemplate();
	};

#endif // __TEMPLT_H__
