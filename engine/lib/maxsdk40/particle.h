/**********************************************************************
 *<
	FILE: particle.h

	DESCRIPTION: Particle system object

	CREATED BY: Rolf Berteig

	HISTORY: 10-18-95

 *> Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef __PARTICLE__
#define __PARTICLE__

#include "meshlib.h"
#include "export.h"

class ParticleSys;

typedef struct{
 Point3 center;
 float radius,oradius,rsquare,tover4;
} SphereData;

// Custom particle drawing callback
class CustomParticleDisplay {
	public:
		virtual BOOL DrawParticle(GraphicsWindow *gw,ParticleSys &parts,int i)=0;
	};

class ParticleSys {
	private:
		CustomParticleDisplay *draw;

		void DrawGW(GraphicsWindow *gw,DWORD flags,MarkerType type);

	public:
		Tab<Point3>	points;		// The particles themselves
		Tab<Point3> vels;		// Velocities of each particle (optional)
		Tab<TimeValue> ages;	// Age of each particle (optional)
		Tab<float> radius;
		Tab<float> tension;
		float size;				// World space radius of a particle


		// Draws the particle system into the GW
		DllExport void Render(GraphicsWindow *gw,MarkerType type=POINT_MRKR);
		
		// Hit tests the particle system. Returns TRUE if a particle is hit.
		DllExport BOOL HitTest(GraphicsWindow *gw, HitRegion *hr, 
			int abortOnHit=FALSE,MarkerType type=POINT_MRKR);

		// Gets bounding box
		DllExport Box3 BoundBox(Matrix3 *tm=NULL);

		// Sets all counts to 0
		DllExport void FreeAll();

		// Sets the size. Flags indicate if optional params should be allocated
		DllExport void SetCount(int c,DWORD flags);

		int Count() {return points.Count();}
		Point3& operator[](int i) {return points[i];}

		// Is particle i alive?
		BOOL Alive(int i) {return ages[i]>=0;}

		// Sets custom draw callback
		void SetCustomDraw(CustomParticleDisplay *d) {draw=d;}
	};

// Flags for SetCount()
#define PARTICLE_VELS	(1<<0)
#define PARTICLE_AGES	(1<<1)
#define PARTICLE_RADIUS	(1<<2)
#define PARTICLE_TENSION (1<<3)

class MetaParticle {
	public:
		DllExport int CreateMetas(ParticleSys parts,Mesh *mesh,float threshold,float res,float strength,int many=1);
		DllExport int CreatePodMetas(SphereData *data,int num,Mesh *mesh,float threshold,float res,int many=1);
};
#endif  // __PARTICLE__
