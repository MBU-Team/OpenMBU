//-----------------------------------------------------------------------------
// Torque Game Engine Advanced 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _INTERIORSIMPLEMESH_H_
#define _INTERIORSIMPLEMESH_H_

#include "core/tVector.h"
//#include "dgl/dgl.h"
#include "math/mBox.h"
#include "core/fileStream.h"
#include "ts/tsShapeInstance.h"
#include "renderInstance/renderInstMgr.h"


class InteriorSimpleMesh
{
public:
	class primitive
	{
	public:
		bool alpha;
		U32 texS;
		U32 texT;
		S32 diffuseIndex;
		S32 lightMapIndex;
		U32 start;
		U32 count;

      // used to relight the surface in-engine...
      PlaneF lightMapEquationX;
      PlaneF lightMapEquationY;
      Point2I lightMapOffset;
      Point2I lightMapSize;

		primitive()
		{
			alpha = false;
			texS = GFXAddressWrap;
			texT = GFXAddressWrap;
			diffuseIndex = 0;
			lightMapIndex = 0;
			start = 0;
			count = 0;

         lightMapEquationX = PlaneF(0, 0, 0, 0);
         lightMapEquationY = PlaneF(0, 0, 0, 0);
         lightMapOffset = Point2I(0, 0);
         lightMapSize = Point2I(0, 0);
		}
	};

	InteriorSimpleMesh()
	{
		materialList = NULL;
		clear();
	}
	~InteriorSimpleMesh(){clear();}
	void clear(bool wipeMaterials = true)
	{
      vertBuff = NULL;
      primBuff = NULL;
      packedIndices.clear();
      packedPrimitives.clear();

		hasSolid = false;
		hasTranslucency = false;
		bounds = Box3F(-1, -1, -1, 1, 1, 1);
      transform.identity();
      scale.set(1.0f, 1.0f, 1.0f);

		primitives.clear();
		indices.clear();
		verts.clear();
		norms.clear();
		diffuseUVs.clear();
		lightmapUVs.clear();

		if(wipeMaterials && materialList)
			delete materialList;

      if (wipeMaterials)
		   materialList = NULL;
	}

   void render(const RenderInst &copyinst, U32 interiorlmhandle, U32 instancelmhandle,
      LightInfo *pridynlight, LightInfo *secdynlight, GFXTexHandle *pridyntex, GFXTexHandle *secdyntex);

	void calculateBounds()
	{
		bounds = Box3F(F32_MAX, F32_MAX, F32_MAX, -F32_MAX, -F32_MAX, -F32_MAX);
		for(U32 i=0; i<verts.size(); i++)
		{
			bounds.max.setMax(verts[i]);
			bounds.min.setMin(verts[i]);
		}
	}

   Vector<U16> packedIndices;
   Vector<primitive> packedPrimitives;/// tri-list instead of strips
   GFXVertexBufferHandle<GFXVertexPNTTBN> vertBuff;
   GFXPrimitiveBufferHandle primBuff;
   void buildBuffers();
   void buildTangent(U32 i0, U32 i1, U32 i2, Vector<Point3F> &tang, Vector<Point3F> &binorm);
   void packPrimitive(primitive &primnew, const primitive &primold, Vector<U16> &indicesnew,
      bool flipped, Vector<Point3F> &tang, Vector<Point3F> &binorm);
   bool prepForRendering(const char *path);

	bool hasSolid;
	bool hasTranslucency;
	Box3F bounds;
   MatrixF transform;
   Point3F scale;

	Vector<primitive> primitives;

	// same index relationship...
	Vector<U16> indices;
	Vector<Point3F> verts;
	Vector<Point3F> norms;
	Vector<Point2F> diffuseUVs;
	Vector<Point2F> lightmapUVs;

	TSMaterialList *materialList;

   bool containsPrimitiveType(bool translucent)
   {
      for(U32 i=0; i<primitives.size(); i++)
      {
         if(primitives[i].alpha == translucent)
            return true;
      }
      return false;
   }

	void copyTo(InteriorSimpleMesh &dest)
	{
		dest.clear();
		dest.hasSolid = hasSolid;
		dest.hasTranslucency = hasTranslucency;
		dest.bounds = bounds;
      dest.transform = transform;
      dest.scale = scale;
		dest.primitives = primitives;
		dest.indices = indices;
		dest.verts = verts;
		dest.norms = norms;
		dest.diffuseUVs = diffuseUVs;
		dest.lightmapUVs = lightmapUVs;

		if(materialList)
			dest.materialList = new TSMaterialList(materialList);
	}
	//bool castRay(const Point3F &start, const Point3F &end, RayInfo* info);
	bool castPlanes(PlaneF left, PlaneF right, PlaneF top, PlaneF bottom);

   bool read(Stream& stream);
   bool write(Stream& stream) const;
};

#endif //_INTERIORSIMPLEMESH_H_

