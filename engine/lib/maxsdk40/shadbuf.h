/**********************************************************************
 *<
	FILE: shadbuf.h : Shadow buffer renderer.

	DESCRIPTION:

	CREATED BY: Dan Silva

	HISTORY:

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef __SHADBUF__H
#define __SHADBUF__H

class ShadBufRenderer {
	public:
		// Compute a shadow Z buffer for the current scene from the viewpoint of the light.
		// NOTE: The computed shadow buffer has positive Z values as you go away from the light, which is
		//       the reverse of the MAX coord system.
		// Returns the number of objects that the shadow volume intersected. If this value is 0,
		//   the shadow buffer can be freed to save memory.
		virtual int Render(
			RendContext &rc, 
			RenderGlobalContext *RGC, 
			float *buf,         // preallocated (shadsize*shadsize)  floats 
			BOOL parallel,		// parallel or perspective projection
			int shadsize,      // size of the buffer (shadsize by shadsize pixels)
			float param,   	   // persp:field-of-view (radians) -- parallel : width in world coords
			float aspect,      // aspect ration of the buffer projection 
			float clipDist,    // don't consider objects farther than this from light
			ObjLightDesc *ltDesc, // descriptor for light that was passed in to CreateShadowGenerator
			Matrix3 worldToLight  // world to light transfor for light
			)=0;
		virtual	float Furthest()=0; // after render, this gives the farthest Z in the buffer
		virtual float Closest()=0;  // after render, this gives the closest Z in the buffer.
		virtual void DeleteThis()=0;
	};

CoreExport ShadBufRenderer *NewDefaultShadBufRenderer();

#endif __SHADBUF__H
