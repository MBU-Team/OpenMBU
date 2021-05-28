/**********************************************************************
 *<
	FILE: shphier.h

	DESCRIPTION:  Defines Shape Hierarchy Class

	CREATED BY: Tom Hudson

	HISTORY: created 30 December 1995

 *>	Copyright (c) 1995, All Rights Reserved.
 **********************************************************************/

#ifndef __SHPHIER_H__ 

#define __SHPHIER_H__

// This class stores the hierarchy tree of a shape object, along with
// a bitarray with an entry for each polygon in the shape which indicates
// whether that polygon should be reversed in order to provide the proper
// clockwise/counterclockwise ordering for the nested shapes.

class ShapeHierarchy {
	public:
		GenericHierarchy hier;
		BitArray reverse;
		ShapeHierarchy() {}
		ShapeHierarchy(int polys) { New(polys); }
		void New(int polys = 0) { hier.New(); reverse.SetSize(polys); reverse.ClearAll(); }
		ShapeHierarchy &operator=(ShapeHierarchy &from) { hier=from.hier; reverse=from.reverse; return *this; }
	};

#endif // __SHPHIER_H__
