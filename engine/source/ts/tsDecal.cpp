//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "ts/tsDecal.h"
#include "math/mMath.h"
#include "math/mathIO.h"
#include "ts/tsShapeInstance.h"
#include "core/frameAllocator.h"

// Not worth the effort, much less the effort to comment, but if the draw types
// are consecutive use addition rather than a table to go from index to command value...
/*
#if ((GL_TRIANGLES+1==GL_TRIANGLE_STRIP) && (GL_TRIANGLE_STRIP+1==GL_TRIANGLE_FAN))
   #define getDrawType(a) (GL_TRIANGLES+(a))
#else
   static U32 drawTypes[] = { GL_TRIANGLES, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN };
   #define getDrawType(a) (drawTypes[a])
#endif
*/
// temp variables for saving and restoring parameters during render
static S32 decalSaveFogMethod;
static F32 decalSaveAlphaAlways;
static F32 gSaveFadeVal;

// from tsmesh
extern void forceFaceCamera();
extern void forceFaceCameraZAxis();

void TSDecalMesh::render(S32 frame, S32 decalFrame, TSMaterialList* materials)
{
    /*
       if (targetMesh == NULL || texgenS.empty() || texgenT.empty() || indices.empty())
          return;

       // we need to face the camera just like our target does
       if (targetMesh->getFlags(TSMesh::Billboard))
       {
          if (targetMesh->getFlags(TSMesh::BillboardZAxis))
             forceFaceCameraZAxis();
          else
             forceFaceCamera();
       }

       S32 firstVert  = targetMesh->vertsPerFrame * frame;

       // generate texture coords
       S32 sz = indices.size();
       Point4F s = texgenS[decalFrame];
       Point4F t = texgenT[decalFrame];
       Point3F * verts = &targetMesh->verts[firstVert];
       U32 waterMark = FrameAllocator::getWaterMark();
       F32 * ptr = (F32*)FrameAllocator::alloc(sizeof(F32)*2*targetMesh->vertsPerFrame);
       S16 * idx = (S16*)indices.address();
       S16 * idxEnd = idx + indices.size();
       for (; idx<idxEnd; idx++)
       {
          Point3F * v = &verts[*idx];
          Point2F & tv = (Point2F&)ptr[2*(*idx)];
          tv.x = v->x * s.x + v->y * s.y + v->z * s.z + s.w;
          tv.y = v->x * t.x + v->y * t.y + v->z * t.z + t.w;
       }

       // set up vertex arrays
       glVertexPointer(3,GL_FLOAT,0,verts);
       glNormalPointer(GL_FLOAT,0,targetMesh->getNormals(firstVert));
       glTexCoordPointer(2,GL_FLOAT,0,ptr);

       // NOTE: we don't lock these arrays since decals tend to use only a small subset of the vertices

       // material change?  We only need to do this once.
       if ( (TSShapeInstance::smRenderData.materialIndex ^ materialIndex) & (TSDrawPrimitive::MaterialMask|TSDrawPrimitive::NoMaterial))
          TSMesh::setMaterial(materialIndex,materials);

       S32 i;
       S32 a = startPrimitive[decalFrame];
       S32 b = decalFrame+1<startPrimitive.size() ? startPrimitive[decalFrame+1] : primitives.size();
       for (i=a; i<b; i++)
       {
          TSDrawPrimitive & draw = primitives[i];
          AssertFatal((draw.matIndex & TSDrawPrimitive::Indexed)!=0,"TSMesh::render: rendering of non-indexed meshes no longer supported");

          S32 drawType  = getDrawType(draw.matIndex>>30);
          glDrawElements(drawType,draw.numElements,GL_UNSIGNED_SHORT,&indices[draw.start]);
       }

       // return frameAllocator memory
       FrameAllocator::setWaterMark(waterMark);
    */
}

void TSDecalMesh::initDecalMaterials()
{
    /*
       TSShapeInstance::smRenderData.materialFlags = TSMaterialList::S_Wrap | TSMaterialList::T_Wrap;
       TSShapeInstance::smRenderData.materialIndex = TSDrawPrimitive::NoMaterial;
       TSShapeInstance::smRenderData.environmentMapMethod = TSShapeInstance::NO_ENVIRONMENT_MAP;
       TSShapeInstance::smRenderData.detailMapMethod = TSShapeInstance::NO_DETAIL_MAP;
       TSShapeInstance::smRenderData.baseTE = 0;

       // adjust for fading
       gSaveFadeVal = TSMesh::getOverrideFade();
       TSMesh::setOverrideFade( 1.0f );
       TSShapeInstance::smRenderData.vertexAlpha.always = gSaveFadeVal;
       // setMaterial will end up changing vertex color if needed...

       // draw one-sided...
       glEnable(GL_CULL_FACE);
       glFrontFace(GL_CW);

       // enable vertex arrays...
       glEnableClientState(GL_VERTEX_ARRAY);
       glEnableClientState(GL_NORMAL_ARRAY);
       glEnableClientState(GL_TEXTURE_COORD_ARRAY);

       // bias depth values to fight z-fighting
       glEnable(GL_POLYGON_OFFSET_FILL);
       glPolygonOffset(-2,-2);

       // when blending we modulate and draw using src_alpha, 1-src_alpha...
       glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
       glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

       // but we don't blend by default...
       glDisable(GL_BLEND);
       glDepthMask(GL_TRUE);

       // this should be off by default, but we'll end up turning it on asap...
       glDisable(GL_TEXTURE_2D);

       // fog the decals (or rather, don't...we'll fog the whole shape later if
       // we are 2-passing it, otherwise we just want to fade the decal out)...
       decalSaveFogMethod = TSShapeInstance::smRenderData.fogMethod;
       if (decalSaveFogMethod != TSShapeInstance::NO_FOG)
          decalSaveAlphaAlways = TSShapeInstance::smRenderData.alwaysAlphaValue;
       TSShapeInstance::smRenderData.fogMethod = TSShapeInstance::FOG_TWO_PASS; // decal will be faded (since decal is translucent)
    */
}

void TSDecalMesh::resetDecalMaterials()
{
    /*
       glDisableClientState(GL_VERTEX_ARRAY);
       glDisableClientState(GL_NORMAL_ARRAY);
       glDisableClientState(GL_TEXTURE_COORD_ARRAY);

       glDisable(GL_POLYGON_OFFSET_FILL);

       glDisable(GL_TEXTURE_2D);
       glDisable(GL_BLEND);
       glDepthMask(GL_TRUE);

       glDisable(GL_CULL_FACE);

       TSMesh::setOverrideFade( gSaveFadeVal );

       // fog the decals (or rather, don't...we'll fog the whole shape later if
       // we are 2-passing it, otherwise we just want to fade the decal out)...
       if (decalSaveFogMethod != TSShapeInstance::NO_FOG)
          TSShapeInstance::smRenderData.alwaysAlphaValue = decalSaveAlphaAlways;
       TSShapeInstance::smRenderData.fogMethod = decalSaveFogMethod;
    */
}

//-----------------------------------------------------
// TSDecalMesh assembly/dissembly methods
// used for transfer to/from memory buffers
//-----------------------------------------------------

#define alloc TSShape::alloc

void TSDecalMesh::assemble(bool)
{
    if (TSShape::smReadVersion < 20)
    {
        // read empty mesh...decals used to be derived from meshes
        alloc.checkGuard();
        alloc.getPointer32(15);
    }

    S32 sz = alloc.get32();
    S32* ptr32 = alloc.copyToShape32(0); // get current shape address w/o doing anything
    for (S32 i = 0; i < sz; i++)
    {
        alloc.copyToShape16(2);
        alloc.copyToShape32(1);
    }
    alloc.align32();
    primitives.set(ptr32, sz);

    sz = alloc.get32();
    S16* ptr16 = alloc.copyToShape16(sz);
    alloc.align32();
    indices.set(ptr16, sz);

    if (TSShape::smReadVersion < 20)
    {
        // read more empty mesh stuff...decals used to be derived from meshes
        alloc.getPointer32(3);
        alloc.checkGuard();
    }

    sz = alloc.get32();
    ptr32 = alloc.copyToShape32(sz);
    startPrimitive.set(ptr32, sz);

    if (TSShape::smReadVersion >= 19)
    {
        ptr32 = alloc.copyToShape32(sz * 4);
        texgenS.set(ptr32, startPrimitive.size());
        ptr32 = alloc.copyToShape32(sz * 4);
        texgenT.set(ptr32, startPrimitive.size());
    }
    else
    {
        texgenS.set(NULL, 0);
        texgenT.set(NULL, 0);
    }

    materialIndex = alloc.get32();

    alloc.checkGuard();
}

void TSDecalMesh::disassemble()
{
    alloc.set32(primitives.size());
    for (S32 i = 0; i < primitives.size(); i++)
    {
        alloc.copyToBuffer16((S16*)&primitives[i], 2);
        alloc.copyToBuffer32(((S32*)&primitives[i]) + 1, 1);
    }

    alloc.set32(indices.size());
    alloc.copyToBuffer16((S16*)indices.address(), indices.size());

    alloc.set32(startPrimitive.size());
    alloc.copyToBuffer32((S32*)startPrimitive.address(), startPrimitive.size());

    alloc.copyToBuffer32((S32*)texgenS.address(), texgenS.size() * 4);
    alloc.copyToBuffer32((S32*)texgenT.address(), texgenT.size() * 4);

    alloc.set32(materialIndex);

    alloc.setGuard();
}






