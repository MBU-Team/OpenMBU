//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "ts/tsMesh.h"
#include "math/mMath.h"
#include "math/mathIO.h"
#include "ts/tsShape.h"
#include "console/console.h"
#include "ts/tsShapeInstance.h"
#include "sim/sceneObject.h"
#include "ts/tsSortedMesh.h"
#include "core/bitRender.h"
#include "collision/convex.h"
#include "core/frameAllocator.h"
#include "platform/profiler.h"
#include "materials/sceneData.h"
#include "sceneGraph/sceneGraph.h"
#include "materials/matInstance.h"
#include "gfx/gfxCanon.h"
#include "renderInstance/renderInstMgr.h"
#include "game/shapeBase.h"
#include "game/game.h"
#include "lightingSystem/sgLightingModel.h"

// Not worth the effort, much less the effort to comment, but if the draw types
// are consecutive use addition rather than a table to go from index to command value...
/*
#if ((GL_TRIANGLES+1==GL_TRIANGLE_STRIP) && (GL_TRIANGLE_STRIP+1==GL_TRIANGLE_FAN))
   #define getDrawType(a) (GL_TRIANGLES+(a))
#else
   U32 drawTypes[] = { GL_TRIANGLES, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN };
   #define getDrawType(a) (drawTypes[a])
#endif
*/

GFXPrimitiveType drawTypes[] = { GFXTriangleList, GFXTriangleStrip, GFXTriangleFan };
#define getDrawType(a) (drawTypes[a])


// structures used to share data between detail levels...
// used (and valid) during load only
Vector<Point3F*> TSMesh::smVertsList;
Vector<Point3F*> TSMesh::smNormsList;
Vector<U8*>      TSMesh::smEncodedNormsList;
Vector<Point2F*> TSMesh::smTVertsList;
Vector<bool>     TSMesh::smDataCopied;

Vector<Point3F>  TSMesh::smSaveVerts;
Vector<Point3F>  TSMesh::smSaveNorms;
Vector<Point2F>  TSMesh::smSaveTVerts;

Vector<MatrixF*> TSSkinMesh::smInitTransformList;
Vector<S32*>     TSSkinMesh::smVertexIndexList;
Vector<S32*>     TSSkinMesh::smBoneIndexList;
Vector<F32*>     TSSkinMesh::smWeightList;
Vector<S32*>     TSSkinMesh::smNodeIndexList;

Vector<Point3F> gNormalStore;

MatrixF TSMesh::smCamTrans(true);
SceneState* TSMesh::smSceneState = NULL;
SceneObject* TSMesh::smObject = NULL;
GFXCubemap* TSMesh::smCubemap = NULL;
bool TSMesh::smGlowPass = false;
bool TSMesh::smRefractPass = false;


F32 TSMesh::overrideFadeVal = 1.0;

bool TSMesh::smUseTriangles = false; // convert all primitives to triangle lists on load
bool TSMesh::smUseOneStrip = true; // join triangle strips into one long strip on load
S32  TSMesh::smMinStripSize = 1;     // smallest number of _faces_ allowed per strip (all else put in tri list)
bool TSMesh::smUseEncodedNormals = false;

// quick function to force object to face camera -- currently throws out roll :(
void forceFaceCamera(MatrixF& mat)
{
    Point4F p;

    mat.getColumn(3, &p);
    mat.identity();
    mat.setColumn(3, p);
    if (TSShapeInstance::smRenderData.objectScale)
    {
        MatrixF scale(true);
        scale.scale(Point3F(TSShapeInstance::smRenderData.objectScale->x,
            TSShapeInstance::smRenderData.objectScale->y,
            TSShapeInstance::smRenderData.objectScale->z));
        mat.mul(scale);
    }
}

void forceFaceCameraZAxis()
{
    /*
       MatrixF mat;
       dglGetModelview(&mat);
       Point3F z;
       mat.getColumn(2,&z); // this is where the z-axis goes, keep it here but reset x and y
       Point3F x,y;
       if (mFabs(z.y) < 0.99f)
       {
          // mCross(Point3F(0,1,0),tAxis,&x);
          x.set(z.z,0,-z.x);
          x.normalize();
          mCross(z,x,&y);
       }
       else
       {
          // mCross(z,Point3F(1,0,0),&y);
          y.set(0,z.z,-z.y);
          y.normalize();
          mCross(y,z,&x);
       }
       mat.setColumn(0,x);
       mat.setColumn(1,y);
       mat.setColumn(2,z);
       dglLoadMatrix(&mat);
       if (TSShapeInstance::smRenderData.objectScale)
          glScalef(TSShapeInstance::smRenderData.objectScale->x,TSShapeInstance::smRenderData.objectScale->y,TSShapeInstance::smRenderData.objectScale->z);
    */
}

void TSMesh::saveMergeVerts()
{
    S32 startMerge = vertsPerFrame - mergeIndices.size();
    S32 j;
    for (j = 0; j < mergeIndices.size(); j++)
    {
        smSaveVerts[j + mergeBufferStart] = verts[j + startMerge];
        smSaveTVerts[j + mergeBufferStart] = tverts[j + startMerge];
    }
    F32 mult = TSShapeInstance::smRenderData.intraDetailLevel;
    for (j = 0; j < mergeIndices.size(); j++)
    {
        verts[j + startMerge] *= mult;
        Point3F v = mergeIndices[j] < startMerge ? verts[mergeIndices[j]] : smSaveVerts[mergeIndices[j] - startMerge + mergeBufferStart];
        v *= 1.0f - mult;
        verts[j + startMerge] += v;

        tverts[j + startMerge] *= mult;
        Point2F tv = mergeIndices[j] < startMerge ? tverts[mergeIndices[j]] : smSaveTVerts[mergeIndices[j] - startMerge + mergeBufferStart];
        tv *= 1.0f - mult;
        tverts[j + startMerge] += tv;
    }
}

void TSMesh::restoreMergeVerts()
{
    S32 startMerge = vertsPerFrame - mergeIndices.size();
    for (S32 i = 0; i < mergeIndices.size(); i++)
    {
        verts[i + startMerge] = smSaveVerts[i + mergeBufferStart];
        tverts[i + startMerge] = smSaveTVerts[i + mergeBufferStart];
    }
}

void TSMesh::saveMergeNormals()
{
    S32 startMerge = vertsPerFrame - mergeIndices.size();
    S32 j;
    smSaveNorms.setSize(mergeIndices.size());
    for (j = 0; j < mergeIndices.size(); j++)
        smSaveNorms[j] = norms[j + startMerge];
    F32 mult = TSShapeInstance::smRenderData.intraDetailLevel;
    for (j = 0; j < mergeIndices.size(); j++)
    {
        norms[j + startMerge] *= mult;
        Point3F n = mergeIndices[j] < startMerge ? norms[mergeIndices[j]] : smSaveNorms[mergeIndices[j] - startMerge];
        n *= 1.0f - mult;
        norms[j + startMerge] += n;
        // norms[j+startMerge].normalize();
    }
}

void TSMesh::restoreMergeNormals()
{
    if (getFlags(UseEncodedNormals))
        return;

    S32 startMerge = vertsPerFrame - mergeIndices.size();
    for (S32 i = 0; i < mergeIndices.size(); i++)
        norms[i + startMerge] = smSaveNorms[i];
}

//-----------------------------------------------------
// TSMesh render methods
//-----------------------------------------------------

void TSMesh::fillVB(S32 vb, S32 frame, S32 matFrame, TSMaterialList* materials)
{
    /*
       S32 firstVert  = vertsPerFrame * frame;
       S32 firstTVert = vertsPerFrame * matFrame;
       const Point3F *normals = getNormals(firstVert);

       // set up vertex arrays -- already enabled in TSShapeInstance::render
       glVertexPointer(3,GL_FLOAT,0,&verts[firstVert]);
       glNormalPointer(GL_FLOAT,0,normals);
       glTexCoordPointer(2,GL_FLOAT,0,&tverts[firstTVert]);

       TSDrawPrimitive &draw = primitives[0];

       AssertFatal(draw.matIndex & TSDrawPrimitive::Indexed,
                   "TSMesh::render: rendering of non-indexed meshes no longer supported");

       // we do this to enable texturing -- if necessary
       if (((TSShapeInstance::smRenderData.materialIndex ^ draw.matIndex) &
           (TSDrawPrimitive::MaterialMask|TSDrawPrimitive::NoMaterial)) != 0)
          setMaterial(draw.matIndex,materials);

       glFillVertexBufferEXT(vb,0,vertsPerFrame);
    */
}

void TSMesh::morphVB(S32 vb, S32 morph, S32 frame, S32 matFrame, TSMaterialList* materials)
{
    /*
       S32 firstVert  = vertsPerFrame * (frame+1) - morph;
       S32 firstTVert = vertsPerFrame * (matFrame+1) - morph;
       const Point3F *normals = getNormals(firstVert);

       saveMergeNormals(); // verts & tverts saved and restored on tsshapeinstance::setStatics

       // set up vertex arrays -- already enabled in TSShapeInstance::render
       glVertexPointer(3,GL_FLOAT,0,&verts[firstVert]);
       glNormalPointer(GL_FLOAT,0,normals);
       glTexCoordPointer(2,GL_FLOAT,0,&tverts[firstTVert]);

       TSDrawPrimitive &draw = primitives[0];

       AssertFatal(draw.matIndex & TSDrawPrimitive::Indexed,
                   "TSMesh::render: rendering of non-indexed meshes no longer supported");

       // we do this to enable texturing -- if necessary
       if (((TSShapeInstance::smRenderData.materialIndex ^ draw.matIndex) &
           (TSDrawPrimitive::MaterialMask|TSDrawPrimitive::NoMaterial)) != 0)
          setMaterial(draw.matIndex,materials);

       glFillVertexBufferEXT(vb,0,morph);

       restoreMergeNormals();
    */
}

void TSMesh::renderVB(S32 frame, S32 matFrame, TSMaterialList* materials)
{
    /*
       S32 firstVert  = vertsPerFrame * frame;
       S32 firstTVert = vertsPerFrame * matFrame;

       if (getFlags(Billboard))
       {
          if (getFlags(BillboardZAxis))
             forceFaceCameraZAxis();
          else
             forceFaceCamera();
       }

       const Point3F *normals = getNormals(firstVert);

       // lock...
       bool lockArrays = dglDoesSupportCompiledVertexArray();
       if (lockArrays)
          glLockArraysEXT(0,vertsPerFrame);

       for (S32 i=0; i<primitives.size(); i++)
       {
          PROFILE_START(TSShapeInstanceVBMat);
          TSDrawPrimitive &draw = primitives[i];

          AssertFatal(draw.matIndex & TSDrawPrimitive::Indexed,
                      "TSMesh::render: rendering of non-indexed meshes no longer supported");

          // material change?
          if (((TSShapeInstance::smRenderData.materialIndex ^ draw.matIndex) &
               (TSDrawPrimitive::MaterialMask|TSDrawPrimitive::NoMaterial)) != 0)
             setMaterial(draw.matIndex,materials);
          PROFILE_END();

          PROFILE_START(TSShapeInstanceDE);
          S32 drawType = getDrawType(draw.matIndex>>30);

          glDrawElements(drawType,draw.numElements,GL_UNSIGNED_SHORT,&indices[draw.start]);
          PROFILE_END();
       }

       // unlock...
       if (lockArrays)
          glUnlockArraysEXT();
    */
}

void TSMesh::render()
{
    GFX->setVertexBuffer(getVertexBuffer());
    GFX->setPrimitiveBuffer(mPB);

    for (S32 p = 0; p < primitives.size(); p++)
        GFX->drawPrimitive(p);
}

void TSMesh::render(S32 frame, S32 matFrame, TSMaterialList* materials)
{
    if (vertsPerFrame <= 0) return;

    F32 meshVisibility = mVisibility;
    if (meshVisibility < 0.0001f)
        return;

    RenderInst* coreRI = gRenderInstManager.allocInst();
    coreRI->type = RenderInstManager::RIT_Mesh;
    if (smSceneState)
    {
        // Calculate our sort point manually.
        MatrixF objToWorld = GFX->getWorldMatrix();
        Box3F rBox = mBounds;
        objToWorld.mul(rBox);
        //coreRI->sortPoint = rBox.getClosestPoint(smSceneState->getCameraPosition());
        rBox.getCenter(&coreRI->sortPoint);
    } else {
        coreRI->sortPoint.set(0,0,0);
    }

    // setup transforms
    MatrixF objTrans = GFX->getWorldMatrix();
    MatrixF proj = GFX->getProjectionMatrix();
    MatrixF world;

    world.mul(smCamTrans, objTrans);

    if (getFlags(Billboard))
        forceFaceCamera(world);

    GFX->pushWorldMatrix();
    GFX->setWorldMatrix(world);
    proj.mul(world);
    proj.transpose();

    coreRI->worldXform = gRenderInstManager.allocXform();
    *coreRI->worldXform = proj;

    coreRI->objXform = gRenderInstManager.allocXform();
    //Point3F sc = objTrans.getScale();
    //sc.x = 1 / sc.x;
    //sc.y = 1 / sc.y;
    //sc.z = 1 / sc.z;
    //objTrans.scale(sc); // Normalize this
    *coreRI->objXform = objTrans;

    coreRI->vertBuff = &getVertexBuffer();
    coreRI->primBuff = &mPB;

    coreRI->visibility = meshVisibility;

    //-----------------------------------------------------------------
    LightManager* lm = getCurrentClientSceneGraph()->getLightManager();
    LightInfoList baselights;
    lm->sgGetBestLights(baselights);

    Vector<bool*> primfirstpass;
    for (S32 i = 0; i < primitives.size(); i++)
    {
        primfirstpass.push_back(gRenderInstManager.allocPrimitiveFirstPass());
        (*primfirstpass.last()) = true;
    }

    if (baselights.size() < 1)
        baselights.push_back(lm->sgGetSpecialLight(LightManager::sgSunLightType));

    LightInfoDualList duallights;
    lm->sgBuildDualLightLists(baselights, duallights);

    // sun must be first for fog...
    // handled in the render managers...
    /*for(U32 l=1; l<lights.size(); l++)
    {
        LightInfo *sun = lights[l];

        if((sun->mType != LightInfo::Vector) &&
           (sun->mType != LightInfo::Ambient))
           continue;

        lights[l] = lights[0];
        lights[0] = sun;
        break;
    }*/

    for (U32 l = 0; l < duallights.size(); l++)
    {
        bool sunlight = false;
        LightInfoDual* duallight = &duallights[l];

        if (!duallight->sgLightPrimary)
            continue;

        LightInfo* light = duallight->sgLightPrimary;
        LightInfo* secondary = duallight->sgLightSecondary;

        if ((light->mType == LightInfo::Vector) ||
            (light->mType == LightInfo::Ambient))
        {
            //continue;
            sunlight = true;
            coreRI->dynamicLight = NULL;
        }
        else
        {
            //continue;
            // get the model...
            sgLightingModel& lightingmodel = sgLightingModelManager::sgGetLightingModel(
                light->sgLightingModelName);
            lightingmodel.sgSetState(light);
            // get the info...
            F32 maxrad = lightingmodel.sgGetMaxRadius(true);
            light->sgTempModelInfo[0] = 0.5f / maxrad;
            // get the dynamic lighting texture...
            if ((light->mType == LightInfo::Spot) || (light->mType == LightInfo::SGStaticSpot))
                coreRI->dynamicLight = lightingmodel.sgGetDynamicLightingTextureSpot();
            else
                coreRI->dynamicLight = lightingmodel.sgGetDynamicLightingTextureOmni();
            // reset the model...
            lightingmodel.sgResetState();

            if (secondary)
            {
                // get the model...
                sgLightingModel& lightingmodel = sgLightingModelManager::sgGetLightingModel(
                    secondary->sgLightingModelName);
                lightingmodel.sgSetState(secondary);
                // get the info...
                F32 maxrad = lightingmodel.sgGetMaxRadius(true);
                secondary->sgTempModelInfo[0] = 0.5f / maxrad;
                // get the dynamic lighting texture...
                if ((secondary->mType == LightInfo::Spot) || (secondary->mType == LightInfo::SGStaticSpot))
                    coreRI->dynamicLightSecondary = lightingmodel.sgGetDynamicLightingTextureSpot();
                else
                    coreRI->dynamicLightSecondary = lightingmodel.sgGetDynamicLightingTextureOmni();
                // reset the model...
                lightingmodel.sgResetState();
            }
        }
        //-----------------------------------------------------------------


        dMemcpy(&coreRI->light, light, sizeof(coreRI->light));

        coreRI->cubemap = smCubemap;
        coreRI->backBuffTex = &GFX->getSfxBackBuffer();


        for (S32 i = 0; i < primitives.size(); i++)
        {
            TSDrawPrimitive& draw = primitives[i];

            U32 test1 = TSShapeInstance::smRenderData.materialIndex ^ draw.matIndex;
            S32 noMat = test1 & TSDrawPrimitive::NoMaterial;

#ifdef TORQUE_DEBUG
            // for inspection if you happen to be running in a debugger and can't do bit 
            // operations in your head.
            S32 triangles = draw.matIndex & TSDrawPrimitive::Triangles;
            S32 strip = draw.matIndex & TSDrawPrimitive::Strip;
            S32 fan = draw.matIndex & TSDrawPrimitive::Fan;
            S32 indexed = draw.matIndex & TSDrawPrimitive::Indexed;
            S32 matIdx = draw.matIndex & TSDrawPrimitive::MaterialMask;
            S32 type = draw.matIndex & TSDrawPrimitive::TypeMask;
#endif

            U32 matIndex = draw.matIndex & TSDrawPrimitive::MaterialMask;
            MatInstance* matInst = materials->getMaterialInst(matIndex);

            if (matInst)
            {
                //----------------------------------------------
                // TEMP: Remove dynamic lighting stuff till we update it
//                if (!sunlight)
//                {
//                    MatInstance* dmat = MatInstance::getDynamicLightingMaterial(matInst, light, (secondary != NULL));
//                    if (matInst->getMaterial()->translucent || !dmat)
//                        continue;
//                    matInst = dmat;
//                }
                //----------------------------------------------

                RenderInst* ri = gRenderInstManager.allocInst();
                *ri = *coreRI;
                ri->primitiveFirstPass = primfirstpass[i];
                ri->matInst = matInst;
                ri->primBuffIndex = i;

//                if (matInst->isDynamicLightingMaterial_Dual())
//                {
//                    // piggyback onto the primary light...
//                    dMemcpy(&ri->lightSecondary, secondary, sizeof(ri->lightSecondary));
//                }
//                else
                if (secondary)
                {
                    // not dual material, but two lights?
                    // finalize light one...
                    gRenderInstManager.addInst(ri);

                    // create light two...
                    ri = gRenderInstManager.allocInst();
                    *ri = *coreRI;
                    ri->primitiveFirstPass = primfirstpass[i];
                    ri->matInst = matInst;
                    ri->primBuffIndex = i;

                    // make the secondary look like the primary...
                    ri->dynamicLight = ri->dynamicLightSecondary;
                    dMemcpy(&ri->light, secondary, sizeof(ri->light));
                }

                // finalize whichever is left open...
                gRenderInstManager.addInst(ri);
            }
            else
            {
                // no material, need this to support .ifl animations
                RenderInst* ri = gRenderInstManager.allocInst();
                *ri = *coreRI;
                ri->primBuffIndex = i;


                if (materials->isIFL(matIndex))
                {
                    ri->matInst = NULL;
                    ri->worldXform = gRenderInstManager.allocXform();
                    *ri->worldXform = GFX->getWorldMatrix();
                }
                else
                {
                    ri->matInst = gRenderInstManager.getWarningMat();
                }

                // noMat = no EXPORTER material, so can't fill in this RenderInst data
                if (!noMat)
                {
                    U32 matIndex = draw.matIndex & TSDrawPrimitive::MaterialMask;
                    U32 flags = materials->getFlags(matIndex);
                    ri->translucent = flags & TSMaterialList::Translucent;
                    ri->transFlags = !(flags & TSMaterialList::Additive);
                    ri->miscTex = &*(materials->getMaterial(matIndex));
                }
                gRenderInstManager.addInst(ri);
            }
        }
    }

    // clean up
    if (smGlowPass)
    {
        GFX->setZWriteEnable(true);
    }

    GFX->setAlphaBlendEnable(false);
    GFX->setAlphaTestEnable(false);

    GFX->popWorldMatrix();
    GFX->setBaseRenderState();
}

const Point3F* TSMesh::getNormals(S32 firstVert)
{
    if (getFlags(UseEncodedNormals))
    {
        gNormalStore.setSize(vertsPerFrame);
        for (S32 i = 0; i < encodedNorms.size(); i++)
            gNormalStore[i] = decodeNormal(encodedNorms[i + firstVert]);
        return gNormalStore.address();
    }
    return &norms[firstVert];
}

void TSMesh::initMaterials()
{

    S32& emapMethod = TSShapeInstance::smRenderData.environmentMapMethod;
    S32& dmapMethod = TSShapeInstance::smRenderData.detailMapMethod;
    S32& fogMethod = TSShapeInstance::smRenderData.fogMethod;
    S32& dmapTE = TSShapeInstance::smRenderData.detailMapTE;
    S32& emapTE = TSShapeInstance::smRenderData.environmentMapTE;
    S32& baseTE = TSShapeInstance::smRenderData.baseTE;
    S32& fogTE = TSShapeInstance::smRenderData.fogTE;
    TSShapeInstance::smRenderData.detailTextureScale = 1.0f;
    TSShapeInstance::smRenderData.fadeSet = false;
    TSShapeInstance::smRenderData.textureMatrixPushed = false;

    // initialize various sources of vertex alpha
    TSShapeInstance::smRenderData.vertexAlpha.init();
    TSShapeInstance::smRenderData.vertexAlpha.always = TSShapeInstance::smRenderData.alwaysAlphaValue;

    // -------------------------------------------------
 /*
    if (dmapMethod == TSShapeInstance::DETAIL_MAP_MULTI_1)
    {
       glClientActiveTextureARB(GL_TEXTURE0_ARB + dmapTE);
       glEnableClientState(GL_TEXTURE_COORD_ARRAY);
       glClientActiveTextureARB(GL_TEXTURE0_ARB + baseTE);

       // switch to detail map texture unit
       glActiveTextureARB(GL_TEXTURE0_ARB + dmapTE);

       glEnable(GL_TEXTURE_2D);

       glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE_EXT);
       glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB_EXT,GL_MODULATE);
       glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_RGB_EXT,GL_PREVIOUS_EXT);
       glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND0_RGB_EXT,GL_SRC_COLOR);
       glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_RGB_EXT,GL_TEXTURE);
       glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND1_RGB_EXT,GL_SRC_COLOR);
       glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_ALPHA_EXT,GL_REPLACE);
       glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_ALPHA_EXT,GL_PREVIOUS_EXT);
       glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND0_ALPHA_EXT,GL_SRC_ALPHA);
       glTexEnvf(GL_TEXTURE_ENV,GL_RGB_SCALE_EXT,2.0f);

       // switch to base texture unit
       glActiveTextureARB(GL_TEXTURE0_ARB + baseTE);
    }
    else if (dmapMethod == TSShapeInstance::DETAIL_MAP_MULTI_2)
    {
       glClientActiveTextureARB(GL_TEXTURE0_ARB + dmapTE);
       glEnableClientState(GL_TEXTURE_COORD_ARRAY);
       glClientActiveTextureARB(GL_TEXTURE0_ARB + baseTE);

       // switch to detail map texture unit
       glActiveTextureARB(GL_TEXTURE0_ARB + dmapTE);

       glEnable(GL_TEXTURE_2D);

       // following used to "alpha" out detail texture
       F32 f = (1.0f-TSShapeInstance::smRenderData.detailMapAlpha)*0.5f;
       glTexEnvfv(GL_TEXTURE_ENV,GL_TEXTURE_ENV_COLOR,Point4F(f,f,f,1.0f-f));

       // first unit computes "alpha-ed" detail texture
       glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE_EXT);
       glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB_EXT,GL_INTERPOLATE_EXT);
       glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_RGB_EXT,GL_CONSTANT_EXT);
       glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND0_RGB_EXT,GL_SRC_ALPHA);
       glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_RGB_EXT,GL_CONSTANT_EXT);
       glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND1_RGB_EXT,GL_SRC_COLOR);
       glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE2_RGB_EXT,GL_TEXTURE);
       glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND2_RGB_EXT,GL_SRC_COLOR);
       // alpha:  don't need it...
       glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_ALPHA_EXT,GL_REPLACE);
       glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_ALPHA_EXT,GL_CONSTANT_EXT); // don't need alpha, maybe this'll be cheaper than alternatives
       glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND0_ALPHA_EXT,GL_SRC_ALPHA);

       // second unit applies detail texture to base texture
       AssertFatal(dmapTE+1==baseTE,"TSShapeInstance::setupTexturing:  assertion failed (1)");
       glActiveTextureARB(GL_TEXTURE0_ARB+dmapTE+1);
       glEnable(GL_TEXTURE_2D);
       glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE_EXT);
       glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB_EXT,GL_MODULATE);
       glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_RGB_EXT,GL_PREVIOUS_EXT);
       glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND0_RGB_EXT,GL_SRC_COLOR);
       glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_RGB_EXT,GL_TEXTURE);
       glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND1_RGB_EXT,GL_SRC_COLOR);
       glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_ALPHA_EXT,GL_REPLACE);
       glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_ALPHA_EXT,GL_TEXTURE);
       glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND0_ALPHA_EXT,GL_SRC_ALPHA);
       glTexEnvf(GL_TEXTURE_ENV,GL_RGB_SCALE_EXT,2.0f);

       // third unit will apply lighting...
       // we need to use the combine extension...we modulate by primary color rather than fragment color
       glActiveTextureARB(GL_TEXTURE0_ARB+dmapTE+2);
       glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE_EXT);
       glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB_EXT,GL_MODULATE);
       glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_RGB_EXT,GL_PREVIOUS_EXT);
       glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND0_RGB_EXT,GL_SRC_COLOR);
       glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_RGB_EXT,GL_PRIMARY_COLOR_EXT);
       glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND1_RGB_EXT,GL_SRC_COLOR);
       glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_ALPHA_EXT,GL_MODULATE);
       glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_ALPHA_EXT,GL_PREVIOUS_EXT);
       glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND0_ALPHA_EXT,GL_SRC_ALPHA);
       glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_ALPHA_EXT,GL_PRIMARY_COLOR_EXT);
       glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND1_ALPHA_EXT,GL_SRC_ALPHA);

       // switch to base texture unit
       glActiveTextureARB(GL_TEXTURE0_ARB + baseTE);
    }

    if (emapMethod == TSShapeInstance::ENVIRONMENT_MAP_MULTI_1)
    {
       // reflectance map == alpha of texture map (always preferred if no translucency)

       // ---------------------------------
       // set up second texure unit (TE #1)
       glActiveTextureARB(GL_TEXTURE0_ARB + emapTE);
       glEnable(GL_TEXTURE_2D);
       glTexGeni(GL_S,GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
       glTexGeni(GL_T,GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
       glEnable(GL_TEXTURE_GEN_S);
       glEnable(GL_TEXTURE_GEN_T);
       // use combine extension -- RGB:   interpolate between previous and current texture on previous alpha
       //                          Alpha: previous alpha
       glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE_EXT);
       glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB_EXT,GL_INTERPOLATE_EXT);
       glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_RGB_EXT,GL_TEXTURE);
       glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND0_RGB_EXT,GL_SRC_COLOR);
       glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_RGB_EXT,GL_PREVIOUS_EXT);
       glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND1_RGB_EXT,GL_SRC_COLOR);
       glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE2_RGB_EXT,GL_PREVIOUS_EXT);
       glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND2_RGB_EXT,GL_SRC_ALPHA);
       glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_ALPHA_EXT,GL_REPLACE);
       glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_ALPHA_EXT,GL_PREVIOUS_EXT);
       glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND0_ALPHA_EXT,GL_SRC_ALPHA);

       // set the environment map
       glBindTexture(GL_TEXTURE_2D, TSShapeInstance::smRenderData.environmentMapGLName);

       TSShapeInstance::smRenderData.vertexAlpha.emap = TSShapeInstance::smRenderData.environmentMapAlpha;

       // switch to base texture unit
       glActiveTextureARB(GL_TEXTURE0_ARB + baseTE);
    }
    else if (emapMethod == TSShapeInstance::ENVIRONMENT_MAP_MULTI_3)
    {
       // ---------------------------------
       // set up first texure unit (TE #1)
       // Note: TE #1 bound to refelectance
       //       map in tsmesh
       glActiveTextureARB(GL_TEXTURE0_ARB + emapTE);
       glEnable(GL_TEXTURE_2D);
       // use combine extension -- RGB:   previous
       //                          Alpha: modulate previous with primary color
       glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE_EXT);
       glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB_EXT,GL_REPLACE);
       glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_RGB_EXT,GL_PREVIOUS_EXT);
       glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND0_RGB_EXT,GL_SRC_COLOR);
       glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_ALPHA_EXT,GL_MODULATE);
       glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_ALPHA_EXT,GL_PREVIOUS_EXT);
       glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND0_ALPHA_EXT,GL_SRC_ALPHA);
       glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_ALPHA_EXT,GL_PRIMARY_COLOR_EXT);
       glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND1_ALPHA_EXT,GL_SRC_ALPHA);

       // ---------------------------------
       // set up second texure unit (TE #2)
       glActiveTextureARB(GL_TEXTURE0_ARB + emapTE + 1);
       glEnable(GL_TEXTURE_2D);
       glTexGeni(GL_S,GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
       glTexGeni(GL_T,GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
       glEnable(GL_TEXTURE_GEN_S);
       glEnable(GL_TEXTURE_GEN_T);
       // use combine extension -- RGB:   interpolate between previous and current texture on previous alpha
       //                          Alpha: previous alpha
       glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE_EXT);
       glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB_EXT,GL_INTERPOLATE_EXT);
       glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_RGB_EXT,GL_TEXTURE);
       glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND0_RGB_EXT,GL_SRC_COLOR);
       glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_RGB_EXT,GL_PREVIOUS_EXT);
       glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND1_RGB_EXT,GL_SRC_COLOR);
       glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE2_RGB_EXT,GL_PREVIOUS_EXT);
       glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND2_RGB_EXT,GL_SRC_ALPHA);
       glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_ALPHA_EXT,GL_REPLACE);
       glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_ALPHA_EXT,GL_PREVIOUS_EXT);
       glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND0_ALPHA_EXT,GL_SRC_ALPHA);
       // set the environment map
       glBindTexture(GL_TEXTURE_2D, TSShapeInstance::smRenderData.environmentMapGLName);

       // ---------------------------------
       // set up third texure unit (TE #3)
       // Note: TE #3 bound to shape texture
       //       in tsmesh (only uses alpha)
       glActiveTextureARB(GL_TEXTURE0_ARB + emapTE);
       glEnable(GL_TEXTURE_2D);
       // use combine extension -- RGB:   previous RGB
       //                          Alpha: interpolate between 1 and current texture using previous alpha
       //                                 = previous.alpha + (1-previous.alpha) * current.alpha
       //                                 = 1 - (1-previous.alpha) * (1-current.alpha)
       //                                 = correct alpha value of texture w/ emap pre-blended
       glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE_EXT);
       glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB_EXT,GL_REPLACE);
       glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_RGB_EXT,GL_PREVIOUS_EXT);
       glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND0_RGB_EXT,GL_SRC_COLOR);
       glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_ALPHA_EXT,GL_INTERPOLATE_EXT);
       glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_ALPHA_EXT,GL_CONSTANT_EXT);
       glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND0_ALPHA_EXT,GL_SRC_ALPHA);
       glTexEnvfv(GL_TEXTURE_ENV,GL_TEXTURE_ENV_COLOR,Point4F(1.0f,1.0f,1.0f,1.0f));
       glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_ALPHA_EXT,GL_TEXTURE);
       glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND1_ALPHA_EXT,GL_SRC_ALPHA);
       glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE2_ALPHA_EXT,GL_PREVIOUS_EXT);
       glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND2_ALPHA_EXT,GL_SRC_ALPHA);

       // set vertex color alpha
       TSShapeInstance::smRenderData.vertexAlpha.emap = TSShapeInstance::smRenderData.environmentMapAlpha;

       // switch to base texture unit
       glActiveTextureARB(GL_TEXTURE0_ARB + baseTE);
    }

    if (fogMethod == TSShapeInstance::FOG_MULTI_1)
    {
       // ---------------------------------
       // set up fog texure unit
       glActiveTextureARB(GL_TEXTURE0_ARB + fogTE);
       glEnable(GL_TEXTURE_2D);

       glTexEnvfv(GL_TEXTURE_ENV,GL_TEXTURE_ENV_COLOR,TSShapeInstance::smRenderData.fogColor);
       glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE_EXT);
       glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB_EXT,GL_INTERPOLATE_EXT);
       glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_RGB_EXT,GL_TEXTURE);
       glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND0_RGB_EXT,GL_SRC_COLOR);
       glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_RGB_EXT,GL_PREVIOUS_EXT);
       glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND1_RGB_EXT,GL_SRC_COLOR);
       glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND2_RGB_EXT,GL_SRC_ALPHA);
       glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_ALPHA_EXT,GL_REPLACE);
       glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_ALPHA_EXT,GL_PREVIOUS_EXT);
       glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND0_ALPHA_EXT,GL_SRC_ALPHA);

       // the ATI Rage 128 needs a forthcoming driver to do do constant alpha blend
       if (TSShapeInstance::smRenderData.fogTexture)
          glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE2_RGB_EXT,GL_TEXTURE);
       else
          glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE2_RGB_EXT,GL_CONSTANT_EXT);

       // bind constant fog bitmap
       glBindTexture(GL_TEXTURE_2D, TSShapeInstance::smRenderData.fogHandle->getGLName());
       glActiveTextureARB(GL_TEXTURE0_ARB + baseTE);
    }
    else if (fogMethod == TSShapeInstance::FOG_MULTI_1_TEXGEN)
    {
       // set up fog texure unit
       glActiveTextureARB(GL_TEXTURE0_ARB + fogTE);
       glEnable(GL_TEXTURE_2D);
       glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_DECAL);

       // set up fog map
       glBindTexture(GL_TEXTURE_2D, TSShapeInstance::smRenderData.fogMapHandle->getGLName());

       // set up texgen equations
       glTexGeni(GL_S,GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
       glTexGeni(GL_T,GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
       glEnable(GL_TEXTURE_GEN_S);
       glEnable(GL_TEXTURE_GEN_T);
       glTexGenfv(GL_S,GL_OBJECT_PLANE,&TSShapeInstance::smRenderData.fogTexGenS.x);
       glTexGenfv(GL_T,GL_OBJECT_PLANE,&TSShapeInstance::smRenderData.fogTexGenT.x);

       // return to base TE
       glActiveTextureARB(GL_TEXTURE0_ARB + baseTE);
    }

    //----------------------------------
    // set up texture enviroment for base texture

    TSShapeInstance::smRenderData.materialFlags = TSMaterialList::S_Wrap | TSMaterialList::T_Wrap;
    TSShapeInstance::smRenderData.materialIndex = TSDrawPrimitive::NoMaterial;

    // draw one-sided...
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CW);

    // enable vertex arrays...
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    // when blending we modulate and draw using src_alpha, 1-src_alpha...
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

    // we modulate in order to apply lighting...
    glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);

    // but we don't blend by default...
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);

    // lighting?
    TSShapeInstance::smRenderData.lightingOn = glIsEnabled(GL_LIGHTING)!=0;

    // set vertex color (if emapping, vertexColor.w holds emapAlpha)
    TSShapeInstance::smRenderData.vertexAlpha.set();
    Point4F vertexColor(1,1,1,TSShapeInstance::smRenderData.vertexAlpha.current);
    glColor4fv(vertexColor);
    glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE,vertexColor);

    // this should be off by default, but we'll end up turning it on asap...
    glDisable(GL_TEXTURE_2D);
 */
}

void TSMesh::resetMaterials()
{
    /*
       // restore gl state changes made for dmap
       if (TSShapeInstance::smRenderData.detailMapMethod==TSShapeInstance::DETAIL_MAP_MULTI_1 ||
           TSShapeInstance::smRenderData.detailMapMethod==TSShapeInstance::DETAIL_MAP_MULTI_2)
       {
          // set detail maps texture unit
          glActiveTextureARB(GL_TEXTURE0_ARB + TSShapeInstance::smRenderData.detailMapTE);

          glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
          glDisable(GL_TEXTURE_2D);

          // may have changed texture matrix of one of detail-texture texture-environment
          if (mFabs(TSShapeInstance::smRenderData.detailTextureScale-1.0f) > 0.001f)
          {
             glMatrixMode(GL_TEXTURE);
             glLoadIdentity();
             glMatrixMode(GL_MODELVIEW);
          }

          if (TSShapeInstance::smRenderData.detailMapMethod==TSShapeInstance::DETAIL_MAP_MULTI_2)
          {
             glTexEnvfv(GL_TEXTURE_ENV,GL_TEXTURE_ENV_COLOR,Point4F(0,0,0,0));
             glActiveTextureARB(GL_TEXTURE0_ARB + TSShapeInstance::smRenderData.detailMapTE+1);
             glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
             glDisable(GL_TEXTURE_2D);
             glTexEnvf(GL_TEXTURE_ENV,GL_RGB_SCALE_EXT,1.0f);
          }
          else
             glTexEnvf(GL_TEXTURE_ENV,GL_RGB_SCALE_EXT,1.0f);

          // disable tcoord's on detail map's TE
          glClientActiveTextureARB(GL_TEXTURE0_ARB + TSShapeInstance::smRenderData.detailMapTE);
          glDisableClientState(GL_TEXTURE_COORD_ARRAY);
          glClientActiveTextureARB(GL_TEXTURE0_ARB + TSShapeInstance::smRenderData.baseTE);
       }

       // restore gl state changes made for emap
       if (TSShapeInstance::smRenderData.environmentMapMethod==TSShapeInstance::ENVIRONMENT_MAP_MULTI_1)
       {
          // set second texure unit (TE #1)
          glActiveTextureARB(GL_TEXTURE0_ARB + TSShapeInstance::smRenderData.environmentMapTE);
          glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
          glDisable(GL_TEXTURE_2D);
          glDisable(GL_TEXTURE_GEN_S);
          glDisable(GL_TEXTURE_GEN_T);
       }
       else if (TSShapeInstance::smRenderData.environmentMapMethod==TSShapeInstance::ENVIRONMENT_MAP_MULTI_3)
       {
          // set second texure unit (TE #1)
          glActiveTextureARB(GL_TEXTURE0_ARB + TSShapeInstance::smRenderData.environmentMapTE);
          glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
          glDisable(GL_TEXTURE_2D);

          // set third texure unit (TE #2)
          glActiveTextureARB(GL_TEXTURE0_ARB + TSShapeInstance::smRenderData.environmentMapTE + 1);
          glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
          glDisable(GL_TEXTURE_2D);
          glDisable(GL_TEXTURE_GEN_S);
          glDisable(GL_TEXTURE_GEN_T);

          // set fourth texure unit (TE #3)
          glActiveTextureARB(GL_TEXTURE0_ARB + TSShapeInstance::smRenderData.environmentMapTE + 2);
          glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
          glDisable(GL_TEXTURE_2D);
       }

       if (TSShapeInstance::smRenderData.fogMethod == TSShapeInstance::FOG_MULTI_1 || TSShapeInstance::smRenderData.fogMethod == TSShapeInstance::FOG_MULTI_1_TEXGEN)
       {
          glActiveTextureARB(GL_TEXTURE0_ARB + TSShapeInstance::smRenderData.fogTE);
          glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
          glDisable(GL_TEXTURE_GEN_S);
          glDisable(GL_TEXTURE_GEN_T);
          glDisable(GL_TEXTURE_2D);
       }

       // restore gl state changes made for base texture
       if (dglDoesSupportARBMultitexture())
          glActiveTextureARB(GL_TEXTURE0_ARB + TSShapeInstance::smRenderData.baseTE);

       glDisableClientState(GL_VERTEX_ARRAY);
       glDisableClientState(GL_NORMAL_ARRAY);
       glDisableClientState(GL_TEXTURE_COORD_ARRAY);

       glDisable(GL_TEXTURE_2D);
       glDisable(GL_BLEND);
       glDepthMask(GL_TRUE);

       // don't cull by default...
       glDisable(GL_CULL_FACE);

       // if lights were on when we started, make sure they're on when we leave
       if (TSShapeInstance::smRenderData.lightingOn && !glIsEnabled(GL_LIGHTING))
          glEnable(GL_LIGHTING);

       // baseTE != 0?
       if (TSShapeInstance::smRenderData.baseTE!=0)
       {
          glActiveTextureARB(GL_TEXTURE0_ARB);
          glClientActiveTextureARB(GL_TEXTURE0_ARB);
       }

       // restore cloak shifting
       if (TSShapeInstance::smRenderData.textureMatrixPushed)
       {
          glMatrixMode(GL_TEXTURE);
          glPopMatrix();
          glMatrixMode(GL_MODELVIEW);
          TSShapeInstance::smRenderData.textureMatrixPushed = false;
       }
    */
}

// set up materials for mesh rendering
// keeps track of flags via TSShapeInstance::smRenderData.materialFlags and only changes what needs to be changed
// keeps track of material index via TSShapeInstance::smRenderData.materialIndex
void TSMesh::setMaterial(S32 matIndex, TSMaterialList* materials)
{

    if ((matIndex | TSShapeInstance::smRenderData.materialIndex) & TSDrawPrimitive::NoMaterial)
    {
        if (matIndex & TSDrawPrimitive::NoMaterial)
        {
            /*
                     glDisable(GL_TEXTURE_2D);
                     glDisable(GL_BLEND);
                     glDepthMask(GL_TRUE);
            */
            TSShapeInstance::smRenderData.materialIndex = matIndex;
            TSShapeInstance::smRenderData.materialFlags &= ~TSMaterialList::Translucent;
            return;
        }
        //      glEnable(GL_TEXTURE_2D);
    }

    matIndex &= TSDrawPrimitive::MaterialMask;

    U32 flags = materials->getFlags(matIndex);
    U32 bareFlags = flags;
    if (TSShapeInstance::smRenderData.alwaysAlpha || TSShapeInstance::smRenderData.fadeSet)
        flags |= TSMaterialList::Translucent;
    U32 deltaFlags = flags ^ TSShapeInstance::smRenderData.materialFlags;
    if (TSShapeInstance::smRenderData.environmentMapMethod != TSShapeInstance::ENVIRONMENT_MAP_MULTI_1)
        deltaFlags &= ~TSMaterialList::NeverEnvMap;

    // update flags and material index...
    TSShapeInstance::smRenderData.materialFlags = flags;
    TSShapeInstance::smRenderData.materialIndex = matIndex;

    if (flags & TSMaterialList::Translucent)
    {
        GFX->setAlphaBlendEnable(true);

        GFX->setAlphaTestEnable(true);
        GFX->setAlphaRef(1);
        GFX->setAlphaFunc(GFXCmpGreaterEqual);

        // set up register combiners
        GFX->setTextureStageAlphaOp(0, GFXTOPModulate);
        GFX->setTextureStageAlphaOp(1, GFXTOPDisable);
        GFX->setTextureStageAlphaArg1(0, GFXTATexture);
        GFX->setTextureStageAlphaArg2(0, GFXTADiffuse);

        if (flags & TSMaterialList::Additive)
        {
            GFX->setSrcBlend(GFXBlendSrcAlpha);
            GFX->setDestBlend(GFXBlendOne);
        }
        else
        {
            if (flags & TSMaterialList::Subtractive)
            {
                GFX->setSrcBlend(GFXBlendZero);
                GFX->setDestBlend(GFXBlendInvSrcColor);
            }
            else
            {
                GFX->setSrcBlend(GFXBlendSrcAlpha);
                GFX->setDestBlend(GFXBlendInvSrcAlpha);
            }
        }
    }
    else
    {
        GFX->setAlphaBlendEnable(false);
    }

    // set clamp/wrap
    GFXTextureAddressMode smode = GFXAddressClamp;
    GFXTextureAddressMode tmode = GFXAddressClamp;
    if (flags & TSMaterialList::S_Wrap)
        smode = GFXAddressWrap;
    if (flags & TSMaterialList::T_Wrap)
        tmode = GFXAddressWrap;
    GFX->setTextureStageAddressModeU(0, smode);
    GFX->setTextureStageAddressModeV(0, tmode);

    /*
       if (TSShapeInstance::smRenderData.useOverride == false || bareFlags & TSMaterialList::Translucent)
       {
          TextureHandle & tex = materials->getMaterial(matIndex);
          glBindTexture(GL_TEXTURE_2D, tex.getGLName());
       }
       else
          glBindTexture(GL_TEXTURE_2D, TSShapeInstance::smRenderData.override.getGLName());

       // anything change...?
       if (deltaFlags)
       {
          if (deltaFlags & TSMaterialList::NeverEnvMap && !TSShapeInstance::smRenderData.useOverride )
          {
             glActiveTextureARB(GL_TEXTURE0_ARB + TSShapeInstance::smRenderData.environmentMapTE);
             if (bareFlags & TSMaterialList::NeverEnvMap)
                glDisable(GL_TEXTURE_2D);
             else
                glEnable(GL_TEXTURE_2D);
             glActiveTextureARB(GL_TEXTURE0_ARB + TSShapeInstance::smRenderData.baseTE);
          }
          if (flags & TSMaterialList::Translucent)
          {
             if (TSShapeInstance::smRenderData.fogMethod == TSShapeInstance::FOG_TWO_PASS || TSShapeInstance::smRenderData.fogMethod == TSShapeInstance::FOG_TWO_PASS_TEXGEN)
             {
                TSShapeInstance::smRenderData.vertexAlpha.fog = 1.0f - TSShapeInstance::smRenderData.fogColor.w;
             }
             else if ((TSShapeInstance::smRenderData.fogMethod == TSShapeInstance::FOG_MULTI_1 ||
                      TSShapeInstance::smRenderData.fogMethod == TSShapeInstance::FOG_MULTI_1_TEXGEN) &&
                      flags & (TSMaterialList::Additive|TSMaterialList::Subtractive))
             {
                glActiveTextureARB(GL_TEXTURE0_ARB + TSShapeInstance::smRenderData.fogTE);
                glDisable(GL_TEXTURE_2D);
                glActiveTextureARB(GL_TEXTURE0_ARB + TSShapeInstance::smRenderData.baseTE);
                TSShapeInstance::smRenderData.vertexAlpha.fog = 1.0f - TSShapeInstance::smRenderData.fogColor.w;
             }
             glEnable(GL_BLEND);
             glDepthMask(GL_FALSE);
          }
          else
          {
             if (TSShapeInstance::smRenderData.fogMethod == TSShapeInstance::FOG_TWO_PASS || TSShapeInstance::smRenderData.fogMethod == TSShapeInstance::FOG_TWO_PASS_TEXGEN)
             {
                TSShapeInstance::smRenderData.vertexAlpha.fog = 1.0f;
             }
             else if ((TSShapeInstance::smRenderData.fogMethod == TSShapeInstance::FOG_MULTI_1 ||
                      TSShapeInstance::smRenderData.fogMethod == TSShapeInstance::FOG_MULTI_1_TEXGEN) &&
                      flags & (TSMaterialList::Additive|TSMaterialList::Subtractive))
             {
                glActiveTextureARB(GL_TEXTURE0_ARB + TSShapeInstance::smRenderData.fogTE);
                glEnable(GL_TEXTURE_2D);
                glActiveTextureARB(GL_TEXTURE0_ARB + TSShapeInstance::smRenderData.baseTE);
                TSShapeInstance::smRenderData.vertexAlpha.fog = 1.0f;
             }
             glDisable(GL_BLEND);
             glDepthMask(GL_TRUE);
          }
          if (deltaFlags & (TSMaterialList::Additive|TSMaterialList::Subtractive))
          {
             if (flags & TSMaterialList::Additive)
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
             else if (flags & TSMaterialList::Subtractive)
                glBlendFunc(GL_ZERO,GL_ONE_MINUS_SRC_COLOR);
             else
                glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
          }
          if (deltaFlags & TSMaterialList::SelfIlluminating)
          {
             if (TSShapeInstance::smRenderData.detailMapMethod==TSShapeInstance::DETAIL_MAP_MULTI_2)
             {
                // special case:  lighting done on different TE than texture...
                glActiveTextureARB(GL_TEXTURE0_ARB + TSShapeInstance::smRenderData.baseTE + 1);
                if (flags & TSMaterialList::SelfIlluminating)
                {
                   // modulate...
                   glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
                }
                else
                {
                   // we need to use the combine extension...we modulate by primary color rather than fragment color
                   glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE_EXT);
                   glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB_EXT,GL_MODULATE);
                   glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_RGB_EXT,GL_PREVIOUS_EXT);
                   glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND0_RGB_EXT,GL_SRC_COLOR);
                   glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_RGB_EXT,GL_PRIMARY_COLOR_EXT);
                   glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND1_RGB_EXT,GL_SRC_COLOR);
                   glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_ALPHA_EXT,GL_MODULATE);
                   glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_ALPHA_EXT,GL_PREVIOUS_EXT);
                   glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND0_ALPHA_EXT,GL_SRC_ALPHA);
                   glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_ALPHA_EXT,GL_PRIMARY_COLOR_EXT);
                   glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND1_ALPHA_EXT,GL_SRC_ALPHA);
                }
                glActiveTextureARB(GL_TEXTURE0_ARB + TSShapeInstance::smRenderData.baseTE);
             }
             // turn off lights if self-illuminating (or back on if not)
             if (flags & TSMaterialList::SelfIlluminating || !TSShapeInstance::smRenderData.lightingOn)
                glDisable(GL_LIGHTING);
             else
                glEnable(GL_LIGHTING);
          }
       }

       // gotta set these every time since present value depends on texture not gl state
       glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,flags & TSMaterialList::S_Wrap ? GL_REPEAT : GL_CLAMP);
       glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,flags & TSMaterialList::T_Wrap ? GL_REPEAT : GL_CLAMP);

       // emap texture...
       if (TSShapeInstance::smRenderData.environmentMapMethod==TSShapeInstance::ENVIRONMENT_MAP_MULTI_3)
       {
          // set emap's texture unit...
          glActiveTextureARB(GL_TEXTURE0_ARB + TSShapeInstance::smRenderData.environmentMapTE + 2);

          if (TSShapeInstance::smRenderData.useOverride == false || flags & TSMaterialList::Translucent)
          {
             TextureHandle & tex = materials->getMaterial(matIndex);
             glBindTexture(GL_TEXTURE_2D, tex.getGLName());
          }
          else
             glBindTexture(GL_TEXTURE_2D, TSShapeInstance::smRenderData.override.getGLName());
          glActiveTextureARB(GL_TEXTURE0_ARB + TSShapeInstance::smRenderData.environmentMapTE);
          glBindTexture(GL_TEXTURE_2D, materials->getReflectionMap(matIndex)->getGLName());

          // set default texture unit...
          glActiveTextureARB(GL_TEXTURE0_ARB + TSShapeInstance::smRenderData.baseTE);
       }

       // dmap texture...
       if (TSShapeInstance::smRenderData.detailMapMethod==TSShapeInstance::DETAIL_MAP_MULTI_1 ||
           TSShapeInstance::smRenderData.detailMapMethod==TSShapeInstance::DETAIL_MAP_MULTI_2)
       {
          // set detail map's texture unit...
          glActiveTextureARB(GL_TEXTURE0_ARB+TSShapeInstance::smRenderData.detailMapTE);

          TextureHandle * detailMap = materials->getDetailMap(matIndex);
          if (detailMap)
          {
             glEnable(GL_TEXTURE_2D);
             glBindTexture(GL_TEXTURE_2D,detailMap->getGLName());

             // has texture scale changed?
             F32 tscale = materials->getDetailMapScale(matIndex);
             if (mFabs(tscale-TSShapeInstance::smRenderData.detailTextureScale) > 0.001f)
             {
                // yes, update scale
                TSShapeInstance::smRenderData.detailTextureScale = tscale;
                glMatrixMode(GL_TEXTURE);
                MatrixF scaleMat;
                scaleMat.identity();
                for (S32 i=0; i<15; i++)
                   ((F32*)scaleMat)[i] *= tscale;
                dglLoadMatrix(&scaleMat);
                glMatrixMode(GL_MODELVIEW);
             }
          }
          else
             glDisable(GL_TEXTURE_2D);

          // set default texture unit...
          glActiveTextureARB(GL_TEXTURE0_ARB + TSShapeInstance::smRenderData.baseTE);
       }

       // translucent materials shouldn't get cloak shifting
       // 1: pushed -> pushed
       // 2: pushed -> not pushed
       // 3: not pushed -> pushed
       // 4: not pushed -> not pushed
       if (TSShapeInstance::smRenderData.textureMatrixPushed)
       {
          if (TSShapeInstance::smRenderData.useOverride && bareFlags & TSMaterialList::Translucent)
          {
             // Leave it alone
          }
          else
          {
             // Pop it off
             glMatrixMode(GL_TEXTURE);
             glPopMatrix();
             glMatrixMode(GL_MODELVIEW);
             TSShapeInstance::smRenderData.textureMatrixPushed = false;
          }
       }
       else
       {
          if (TSShapeInstance::smRenderData.useOverride && bareFlags & TSMaterialList::Translucent)
          {
             // Push it up
             glMatrixMode(GL_TEXTURE);
             glPushMatrix();
             glLoadIdentity();
             glMatrixMode(GL_MODELVIEW);
             TSShapeInstance::smRenderData.textureMatrixPushed = true;
          }
          else
          {
             // leave it alone
          }
       }

       // handle environment map
       if (flags & TSMaterialList::NeverEnvMap)
          TSShapeInstance::smRenderData.vertexAlpha.emap = 1.0f;
       else
          TSShapeInstance::smRenderData.vertexAlpha.emap = TSShapeInstance::smRenderData.environmentMapAlpha * materials->getReflectionAmount(matIndex);
       // handle vertex alpha
       if (TSShapeInstance::smRenderData.vertexAlpha.set())
       {
          Point4F v(1,1,1,TSShapeInstance::smRenderData.vertexAlpha.current);
          glColor4fv(v);
          glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE,v);
       }

       // set up fade
       if( overrideFadeVal < 1.0f && dglDoesSupportTextureEnvCombine() )
       {
          S32 & emapTE     = TSShapeInstance::smRenderData.environmentMapTE;
          S32 & baseTE     = TSShapeInstance::smRenderData.baseTE;

          if( TSShapeInstance::smRenderData.environmentMapMethod == TSShapeInstance::ENVIRONMENT_MAP_MULTI_1 )
          {
             glActiveTextureARB(GL_TEXTURE0_ARB + emapTE);
             glDisable(GL_TEXTURE_2D);
          }

          glActiveTextureARB(GL_TEXTURE0_ARB + baseTE);

          glEnable( GL_BLEND );

          if( TSShapeInstance::smRenderData.materialFlags & TSMaterialList::Translucent )
          {
             glBlendFunc( GL_SRC_ALPHA, GL_ONE );
          }
          else
          {
             glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
          }


          ColorF curColor;
          glGetFloatv( GL_FOG_COLOR, (GLfloat*)&curColor );
          curColor.alpha = overrideFadeVal;
          glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, curColor);

          glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE_EXT);
          glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_ALPHA_EXT,GL_REPLACE);
          glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_ALPHA_EXT,GL_CONSTANT_EXT);
          glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND0_ALPHA_EXT,GL_SRC_ALPHA);


       }
    */
}

void TSMesh::setFade(F32 fadeValue)
{
    mVisibility = fadeValue;

    TSShapeInstance::smRenderData.vertexAlpha.vis = fadeValue;
    TSShapeInstance::smRenderData.fadeSet = true;
    /*
       TSShapeInstance::smRenderData.vertexAlpha.vis = fadeValue;
       if (TSShapeInstance::smRenderData.vertexAlpha.set())
       {
          Point4F v(1,1,1,TSShapeInstance::smRenderData.vertexAlpha.current);
          glColor4fv(v);
          glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE,v);
       }
       TSShapeInstance::smRenderData.fadeSet = true;
    */
}

void TSMesh::clearFade()
{
    setFade(1.0f);
    //   setFade(1.0f);
       TSShapeInstance::smRenderData.fadeSet = false;
}

//-----------------------------------------------------
// TSMesh render environment map (2-pass) methods
//-----------------------------------------------------

void TSMesh::renderEnvironmentMap(S32 frame, S32 matFrame, TSMaterialList* materials)
{
    /*
       matFrame;

       // most gl states assumed to be all set up...
       // if we're here, then we're drawing environment map in two passes...

       S32 firstVert  = vertsPerFrame * frame;

       // set up vertex arrays -- already enabled in TSShapeInstance::render
       glVertexPointer(3,GL_FLOAT,0,&verts[firstVert]);
       glNormalPointer(GL_FLOAT,0,&norms[firstVert]);

       // lock...
       bool lockArrays = dglDoesSupportCompiledVertexArray();
       if (lockArrays)
          glLockArraysEXT(0,vertsPerFrame);

       S32 matIndex = -1;

       for (S32 i=0; i<primitives.size(); i++)
       {
          TSDrawPrimitive & draw = primitives[i];
          AssertFatal(draw.matIndex & TSDrawPrimitive::Indexed,"TSMesh::render: rendering of non-indexed meshes no longer supported");

          if ( (matIndex ^ draw.matIndex) & (TSDrawPrimitive::MaterialMask|TSDrawPrimitive::NoMaterial))
          {
             // material change
             matIndex = draw.matIndex & (TSDrawPrimitive::MaterialMask|TSDrawPrimitive::NoMaterial);

             if (matIndex & TSDrawPrimitive::NoMaterial)
                // no material...
                continue;
             else
             {
                if (materials->getFlags(matIndex) & TSMaterialList::NeverEnvMap)
                   continue;
                TSShapeInstance::smRenderData.vertexAlpha.emap =
                   TSShapeInstance::smRenderData.environmentMapAlpha * materials->getReflectionAmount(matIndex);
                glBindTexture(GL_TEXTURE_2D, materials->getReflectionMap(matIndex)->getGLName());
                if (TSShapeInstance::smRenderData.vertexAlpha.set())
                {
                   Point4F v(1,1,1,TSShapeInstance::smRenderData.vertexAlpha.current);
                   glColor4fv(v);
                   glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE,v);
                }
             }
          }
          glDrawElements(getDrawType(draw.matIndex>>30),draw.numElements,GL_UNSIGNED_SHORT,&indices[draw.start]);
       }

       // unlock...
       if (lockArrays)
          glUnlockArraysEXT();
    */
}

void TSMesh::initEnvironmentMapMaterials()
{
    /*
       // set up gl environment

       // TE0
       glActiveTextureARB(GL_TEXTURE0_ARB);

       glEnable(GL_TEXTURE_2D);
       glEnable(GL_CULL_FACE);
       glEnable(GL_BLEND);
       glDepthMask(GL_TRUE);
       glFrontFace(GL_CW);
       glEnableClientState(GL_VERTEX_ARRAY);
       glEnableClientState(GL_NORMAL_ARRAY);
       glEnableClientState(GL_TEXTURE_COORD_ARRAY);
       glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
       glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);

       // TE1
       glActiveTextureARB(GL_TEXTURE1_ARB);

       glEnable(GL_TEXTURE_2D);
       glTexGeni(GL_S,GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
       glTexGeni(GL_T,GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
       glEnable(GL_TEXTURE_GEN_S);
       glEnable(GL_TEXTURE_GEN_T);
       glBindTexture(GL_TEXTURE_2D, TSShapeInstance::smRenderData.environmentMapGLName);
       glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE); // should leave alpha alone since emap has no alpha

       // TE0 again
       glActiveTextureARB(GL_TEXTURE0_ARB);

       TSShapeInstance::smRenderData.vertexAlpha.init();
       TSShapeInstance::smRenderData.vertexAlpha.emap = TSShapeInstance::smRenderData.environmentMapAlpha;
       TSShapeInstance::smRenderData.vertexAlpha.always = TSShapeInstance::smRenderData.alwaysAlphaValue;
       TSShapeInstance::smRenderData.vertexAlpha.set();
       glColor4fv(Point4F(1.0f,1.0f,1.0f,TSShapeInstance::smRenderData.vertexAlpha.current));
    */
}

void TSMesh::resetEnvironmentMapMaterials()
{
    /*
       // restore texture environmnet 0
       glDisable(GL_TEXTURE_2D);
       glDisable(GL_BLEND);
       glDepthMask(GL_TRUE);
       glDisable(GL_CULL_FACE);
       glDisableClientState(GL_VERTEX_ARRAY);
       glDisableClientState(GL_NORMAL_ARRAY);
       glDisableClientState(GL_TEXTURE_COORD_ARRAY);

       // restore texture environment 1
       glActiveTextureARB(GL_TEXTURE1_ARB);
       glDisable(GL_TEXTURE_GEN_S);
       glDisable(GL_TEXTURE_GEN_T);
       glDisable(GL_TEXTURE_2D);

       glActiveTextureARB(GL_TEXTURE0_ARB);
    */
}

//-----------------------------------------------------
// TSMesh render detail map (2 pass) methods
//-----------------------------------------------------

void TSMesh::renderDetailMap(S32 frame, S32 matFrame, TSMaterialList* materials)
{
    /*
       // most gl states assumed to be all set up...
       // if we're here, then we're drawing detail map in two passes...

       S32 firstVert  = vertsPerFrame * frame;
       S32 firstTVert = vertsPerFrame * matFrame;

       // set up vertex arrays -- already enabled in TSShapeInstance::render
       glVertexPointer(3,GL_FLOAT,0,&verts[firstVert]);
       glNormalPointer(GL_FLOAT,0,&norms[firstVert]);
       glTexCoordPointer(2,GL_FLOAT,0,&tverts[firstTVert]);

       // lock...
       bool lockArrays = dglDoesSupportCompiledVertexArray();
       if (lockArrays)
          glLockArraysEXT(0,vertsPerFrame);

       S32 matIndex = -1;

       for (S32 i=0; i<primitives.size(); i++)
       {
          TSDrawPrimitive & draw = primitives[i];
          AssertFatal(draw.matIndex & TSDrawPrimitive::Indexed,"TSMesh::render: rendering of non-indexed meshes no longer supported");

          if ( (matIndex ^ draw.matIndex) & (TSDrawPrimitive::MaterialMask|TSDrawPrimitive::NoMaterial))
          {
             // material change
             matIndex = draw.matIndex & (TSDrawPrimitive::MaterialMask|TSDrawPrimitive::NoMaterial);

             if (matIndex & TSDrawPrimitive::NoMaterial)
                // no material...
                continue;
             else
             {
                TSShapeInstance::smRenderData.detailMapTE = 0; // borrow this variable...use as a bool to flag that we have a dmap
                TextureHandle * detailMap = materials->getDetailMap(matIndex);
                if (detailMap)
                   glBindTexture(GL_TEXTURE_2D,detailMap->getGLName());
                else
                   continue;
                TSShapeInstance::smRenderData.detailMapTE = 1;
                F32 tscale = materials->getDetailMapScale(matIndex);
                if (mFabs(tscale-TSShapeInstance::smRenderData.detailTextureScale) > 0.001f)
                {
                   // yes, update scale
                   TSShapeInstance::smRenderData.detailTextureScale = tscale;
                   glMatrixMode(GL_TEXTURE);
                   MatrixF scaleMat;
                   scaleMat.identity();
                   for (S32 i=0; i<15; i++)
                      ((F32*)scaleMat)[i] *= tscale;
                   dglLoadMatrix(&scaleMat);
                   glMatrixMode(GL_MODELVIEW);
                }
             }
          }
          if (TSShapeInstance::smRenderData.detailMapTE)
             glDrawElements(getDrawType(draw.matIndex>>30),draw.numElements,GL_UNSIGNED_SHORT,&indices[draw.start]);
       }

       // unlock...
       if (lockArrays)
          glUnlockArraysEXT();
    */
}

void TSMesh::initDetailMapMaterials()
{
    /*
       TSShapeInstance::smRenderData.detailTextureScale = 1.0f;

       glEnable(GL_TEXTURE_2D);
       glEnable(GL_BLEND);
       glEnable(GL_CULL_FACE);
       glFrontFace(GL_CW);
       glDepthMask(GL_TRUE);
       glBlendFunc(GL_DST_COLOR,GL_SRC_COLOR);
       glEnableClientState(GL_VERTEX_ARRAY);
       glEnableClientState(GL_NORMAL_ARRAY);
       glEnableClientState(GL_TEXTURE_COORD_ARRAY);

       // set texture environment color and fragment color
       // so that c=1-k/2 and f=k/2, where k is 1-detailMapAlpha
       // result is that distance from 0.5 is reduced as
       // detailMapAlpha goes to 0...
       F32 k = 0.5f * (1.0f - TSShapeInstance::smRenderData.detailMapAlpha);
       Point4F TEColor(1.0f-k,1.0f-k,1.0f-k,1.0f);
       glTexEnvfv(GL_TEXTURE_ENV,GL_TEXTURE_ENV_COLOR,TEColor);
       Point4F fColor(k,k,k,1.0f);
       glColor4fv(fColor);
       glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_BLEND);

       glDisable(GL_LIGHTING);
    */
}

void TSMesh::resetDetailMapMaterials()
{
    /*
       glEnable(GL_LIGHTING);

       if (mFabs(TSShapeInstance::smRenderData.detailTextureScale-1.0f)>0.001f)
       {
          glMatrixMode(GL_TEXTURE);
          glLoadIdentity();
          glMatrixMode(GL_MODELVIEW);
       }

       glDisable(GL_BLEND);
       glDisable(GL_TEXTURE_2D);
       glDepthMask(GL_TRUE);
       glDisable(GL_CULL_FACE);
       glDisableClientState(GL_VERTEX_ARRAY);
       glDisableClientState(GL_NORMAL_ARRAY);
       glDisableClientState(GL_TEXTURE_COORD_ARRAY);

       glColor4fv(Point4F(1,1,1,1));
       glTexEnvfv(GL_TEXTURE_ENV,GL_TEXTURE_ENV_COLOR,Point4F(0,0,0,0));
       glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
    */
}

//-----------------------------------------------------
// TSMesh renderShadow
//-----------------------------------------------------

Vector<Point2I> gShadowVerts(__FILE__, __LINE__);

void shadowTransform(const MatrixF& mat, Point3F* v, Point3F* vend, S32 max)
{
    const F32* m = (const F32*)mat;
    F32 ax = m[0];
    F32 ay = m[1];
    F32 az = m[2];
    F32 at = m[3];
    F32 bx = m[8];
    F32 by = m[9];
    F32 bz = m[10];
    F32 bt = m[11];

    Point2I* dest = gShadowVerts.address();
    S32 val;

    while (v < vend)
    {
        val = (S32)(ax * v->x + ay * v->y + az * v->z + at);
        dest->x = val < 0 ? 0 : (val > max ? max : val);

        val = (S32)(bx * v->x + by * v->y + bz * v->z + bt);
        dest->y = val < 0 ? 0 : (val > max ? max : val);

        dest++; v++;
    }
}

void TSMesh::renderShadow(S32 frame, const MatrixF& mat, S32 dim, U32* bits, TSMaterialList* materials)
{
    /*
       if (!vertsPerFrame || !primitives.size())
          return;

       if (!(primitives[0].matIndex & TSDrawPrimitive::NoMaterial) && (TSMaterialList::Translucent & materials->getFlags(primitives[0].matIndex & TSDrawPrimitive::MaterialMask)))
          // if no material...it would be nice to just exit...but may have some real polys back there...
          return;

       dglNPatchEnd();

       AssertFatal(primitives[0].matIndex & TSDrawPrimitive::Indexed,"TSMesh::renderShadow: indexed polys only");

       gShadowVerts.setSize(vertsPerFrame);
       Point3F * vstart = &verts[frame*vertsPerFrame];
       Point3F * vend   = vstart + vertsPerFrame;

       // result placed into gShadowVerts
       shadowTransform(mat,vstart,vend,dim-1);

       // pick correct bit render routine...we assume all strips or all triangles
       if ( (primitives[0].matIndex & TSDrawPrimitive::TypeMask) == TSDrawPrimitive::Strip)
          BitRender::render_strips((U8*)primitives.address(),primitives.size(),sizeof(TSDrawPrimitive),indices.address(),gShadowVerts.address(),dim,bits);
       else if ( (primitives[0].matIndex & TSDrawPrimitive::TypeMask) == TSDrawPrimitive::Triangles)
          BitRender::render_tris((U8*)primitives.address(),primitives.size(),sizeof(TSDrawPrimitive),indices.address(),gShadowVerts.address(),dim,bits);
       else
          AssertFatal(0,"TSMesh::renderShadow: strips or triangles only...how'd you get in here.");
    */
}


//-----------------------------------------------------
// TSMesh render fog
//-----------------------------------------------------
void TSMesh::renderFog(S32 frame, TSMaterialList* materials)
{
    /*
       if (getFlags(Billboard))
       {
          if (getFlags(BillboardZAxis))
             forceFaceCameraZAxis();
          else
             forceFaceCamera();
       }

       S32 firstVert  = vertsPerFrame * frame;

       // set up vertex arrays -- already enabled in TSShapeInstance::render
       glVertexPointer(3,GL_FLOAT,0,&verts[firstVert]);

       // lock...
       bool lockArrays = dglDoesSupportCompiledVertexArray();
       if (lockArrays)
          glLockArraysEXT(0,vertsPerFrame);

       for (S32 i=0; i<primitives.size(); i++)
       {
          TSDrawPrimitive & draw = primitives[i];
          if (primitives[i].matIndex & TSDrawPrimitive::NoMaterial ||
              materials->getFlags(primitives[i].matIndex & TSDrawPrimitive::MaterialMask) & (TSMaterialList::Translucent | TSMaterialList::Additive))
             continue;

          glDrawElements(getDrawType(draw.matIndex>>30),draw.numElements,GL_UNSIGNED_SHORT,&indices[draw.start]);
       }

       // unlock...
       if (lockArrays)
          glUnlockArraysEXT();
    */
}

//-----------------------------------------------------
// TSMesh collision methods
//-----------------------------------------------------

bool TSMesh::buildPolyList(S32 frame, AbstractPolyList* polyList, U32& surfaceKey)
{
    S32 firstVert = vertsPerFrame * frame, i, base;

    // add the verts...
    if (vertsPerFrame)
    {
        base = polyList->addPoint(verts[firstVert]);
        for (i = 1; i < vertsPerFrame; i++)
            polyList->addPoint(verts[i + firstVert]);
    }

    // add the polys...
    for (i = 0; i < primitives.size(); i++)
    {
        TSDrawPrimitive& draw = primitives[i];
        U32 start = draw.start;

        AssertFatal(draw.matIndex & TSDrawPrimitive::Indexed, "TSMesh::buildPolyList (1)");

        U32 material = draw.matIndex & TSDrawPrimitive::MaterialMask;

        // gonna depend on what kind of primitive it is...
        if ((draw.matIndex & TSDrawPrimitive::TypeMask) == TSDrawPrimitive::Triangles)
        {
            for (S32 j = 0; j < draw.numElements; )
            {
                U32 idx0 = base + indices[start + j + 0];
                U32 idx1 = base + indices[start + j + 1];
                U32 idx2 = base + indices[start + j + 2];
                polyList->begin(material, surfaceKey++);
                polyList->vertex(idx0);
                polyList->vertex(idx1);
                polyList->vertex(idx2);
                polyList->plane(idx0, idx1, idx2);
                polyList->end();
                j += 3;
            }
        }
        else
        {
            AssertFatal((draw.matIndex & TSDrawPrimitive::TypeMask) == TSDrawPrimitive::Strip, "TSMesh::buildPolyList (2)");

            U32 idx0 = base + indices[start + 0];
            U32 idx1;
            U32 idx2 = base + indices[start + 1];
            U32* nextIdx = &idx1;
            for (S32 j = 2; j < draw.numElements; j++)
            {
                *nextIdx = idx2;
                //            nextIdx = (j%2)==0 ? &idx0 : &idx1;
                nextIdx = (U32*)((dsize_t)nextIdx ^ (dsize_t)&idx0 ^ (dsize_t)&idx1);
                idx2 = base + indices[start + j];
                if (idx0 == idx1 || idx0 == idx2 || idx1 == idx2)
                    continue;

                polyList->begin(material, surfaceKey++);
                polyList->vertex(idx0);
                polyList->vertex(idx1);
                polyList->vertex(idx2);
                polyList->plane(idx0, idx1, idx2);
                polyList->end();
            }
        }
    }
    return true;
}

bool TSMesh::getFeatures(S32 frame, const MatrixF& mat, const VectorF& /*n*/, ConvexFeature* cf, U32& /*surfaceKey*/)
{
    // DMM NOTE! Do not change without talking to Dave Moore.  ShapeBase assumes that
    //  this will return ALL information from the mesh.
    S32 firstVert = vertsPerFrame * frame;
    S32 i;
    S32 base = cf->mVertexList.size();

    for (i = 0; i < vertsPerFrame; i++) {
        cf->mVertexList.increment();
        mat.mulP(verts[firstVert + i], &cf->mVertexList.last());
    }

    // add the polys...
    for (i = 0; i < primitives.size(); i++)
    {
        TSDrawPrimitive& draw = primitives[i];
        U32 start = draw.start;

        AssertFatal(draw.matIndex & TSDrawPrimitive::Indexed, "TSMesh::buildPolyList (1)");

        U32 material = draw.matIndex & TSDrawPrimitive::MaterialMask;

        // gonna depend on what kind of primitive it is...
        if ((draw.matIndex & TSDrawPrimitive::TypeMask) == TSDrawPrimitive::Triangles)
        {
            for (S32 j = 0; j < draw.numElements; j += 3)
            {
                PlaneF plane(cf->mVertexList[base + indices[start + j + 0]],
                    cf->mVertexList[base + indices[start + j + 1]],
                    cf->mVertexList[base + indices[start + j + 2]]);

                cf->mFaceList.increment();
                cf->mFaceList.last().normal = plane;

                cf->mFaceList.last().vertex[0] = base + indices[start + j + 0];
                cf->mFaceList.last().vertex[1] = base + indices[start + j + 1];
                cf->mFaceList.last().vertex[2] = base + indices[start + j + 2];

                for (U32 l = 0; l < 3; l++) {
                    U32 newEdge0, newEdge1;
                    U32 zero = base + indices[start + j + l];
                    U32 one = base + indices[start + j + ((l + 1) % 3)];
                    newEdge0 = getMin(zero, one);
                    newEdge1 = getMax(zero, one);
                    bool found = false;
                    for (S32 k = 0; k < cf->mEdgeList.size(); k++) {
                        if (cf->mEdgeList[k].vertex[0] == newEdge0 &&
                            cf->mEdgeList[k].vertex[1] == newEdge1) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        cf->mEdgeList.increment();
                        cf->mEdgeList.last().vertex[0] = newEdge0;
                        cf->mEdgeList.last().vertex[1] = newEdge1;
                    }
                }
            }
        }
        else
        {
            AssertFatal((draw.matIndex & TSDrawPrimitive::TypeMask) == TSDrawPrimitive::Strip, "TSMesh::buildPolyList (2)");

            U32 idx0 = base + indices[start + 0];
            U32 idx1;
            U32 idx2 = base + indices[start + 1];
            U32* nextIdx = &idx1;
            for (S32 j = 2; j < draw.numElements; j++)
            {
                *nextIdx = idx2;
                nextIdx = (U32*)((dsize_t)nextIdx ^ (dsize_t)&idx0 ^ (dsize_t)&idx1);
                idx2 = base + indices[start + j];
                if (idx0 == idx1 || idx0 == idx2 || idx1 == idx2)
                    continue;

                PlaneF plane(cf->mVertexList[idx0],
                    cf->mVertexList[idx1],
                    cf->mVertexList[idx2]);

                cf->mFaceList.increment();
                cf->mFaceList.last().normal = plane;

                cf->mFaceList.last().vertex[0] = idx0;
                cf->mFaceList.last().vertex[1] = idx1;
                cf->mFaceList.last().vertex[2] = idx2;

                U32 newEdge0, newEdge1;
                newEdge0 = getMin(idx0, idx1);
                newEdge1 = getMax(idx0, idx1);
                bool found = false;
                S32 k;
                for (k = 0; k < cf->mEdgeList.size(); k++) {
                    if (cf->mEdgeList[k].vertex[0] == newEdge0 &&
                        cf->mEdgeList[k].vertex[1] == newEdge1) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    cf->mEdgeList.increment();
                    cf->mEdgeList.last().vertex[0] = newEdge0;
                    cf->mEdgeList.last().vertex[1] = newEdge1;
                }

                newEdge0 = getMin(idx1, idx2);
                newEdge1 = getMax(idx1, idx2);
                found = false;
                for (k = 0; k < cf->mEdgeList.size(); k++) {
                    if (cf->mEdgeList[k].vertex[0] == newEdge0 &&
                        cf->mEdgeList[k].vertex[1] == newEdge1) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    cf->mEdgeList.increment();
                    cf->mEdgeList.last().vertex[0] = newEdge0;
                    cf->mEdgeList.last().vertex[1] = newEdge1;
                }

                newEdge0 = getMin(idx0, idx2);
                newEdge1 = getMax(idx0, idx2);
                found = false;
                for (k = 0; k < cf->mEdgeList.size(); k++) {
                    if (cf->mEdgeList[k].vertex[0] == newEdge0 &&
                        cf->mEdgeList[k].vertex[1] == newEdge1) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    cf->mEdgeList.increment();
                    cf->mEdgeList.last().vertex[0] = newEdge0;
                    cf->mEdgeList.last().vertex[1] = newEdge1;
                }
            }
        }
    }

    return false;
}


void TSMesh::support(S32 frame, const Point3F& v, F32* currMaxDP, Point3F* currSupport)
{
    if (vertsPerFrame == 0)
        return;

    U32 waterMark = FrameAllocator::getWaterMark();
    F32* pDots = (F32*)FrameAllocator::alloc(sizeof(F32) * vertsPerFrame);

    S32 firstVert = vertsPerFrame * frame;
    m_point3F_bulk_dot(&v.x,
        &verts[firstVert].x,
        vertsPerFrame,
        sizeof(Point3F),
        pDots);

    F32 localdp = *currMaxDP;
    S32 index = -1;

    for (S32 i = 0; i < vertsPerFrame; i++)
    {
        if (pDots[i] > localdp)
        {
            localdp = pDots[i];
            index = i;
        }
    }

    FrameAllocator::setWaterMark(waterMark);

    if (index != -1)
    {
        *currMaxDP = localdp;
        *currSupport = verts[index + firstVert];
    }
}

bool TSMesh::castRay(S32 frame, const Point3F& start, const Point3F& end, RayInfo* rayInfo)
{
    if (planeNormals.empty())
        // if haven't done it yet...
        buildConvexHull();

    // Keep track of startTime and endTime.  They start out at just under 0 and just over 1, respectively.
    // As we check against each plane, prune start and end times back to represent current intersection of
    // line with all the planes (or rather with all the half-spaces defined by the planes).
    // But, instead of explicitly keeping track of startTime and endTime, keep track as numerator and denominator
    // so that we can avoid as many divisions as possible.

    //   F32 startTime = -0.01f;
    F32 startNum = -0.01f;
    F32 startDen = 1.00f;
    //   F32 endTime   = 1.01f;
    F32 endNum = 1.01f;
    F32 endDen = 1.00f;

    S32 curPlane = 0;
    U32 curMaterial = 0;
    bool found = false;

    // the following block of code is an optimization...
    // it isn't necessary if the longer version of the main loop is used
    bool tmpFound;
    S32 tmpPlane;
    F32 sgn = -1.0f;
    F32* pnum = &startNum;
    F32* pden = &startDen;
    S32* pplane = &curPlane;
    bool* pfound = &found;

    S32 startPlane = frame * planesPerFrame;
    for (S32 i = startPlane; i < startPlane + planesPerFrame; i++)
    {
        // if start & end outside, no collision
        // if start & end inside, continue
        // if start outside, end inside, or visa versa, find intersection of line with plane
        //    then update intersection of line with hull (using startTime and endTime)
        F32 dot1 = mDot(planeNormals[i], start) - planeConstants[i];
        F32 dot2 = mDot(planeNormals[i], end) - planeConstants[i];
        if (dot1 * dot2 > 0.0f)
        {
            // same side of the plane...which side -- dot==0 considered inside
            if (dot1 > 0.0f)
                // start and end outside of this plane, no collision
                return false;
            // start and end inside plane, continue
            continue;
        }

        AssertFatal(dot1 / (dot1 - dot2) >= 0.0f && dot1 / (dot1 - dot2) <= 1.0f, "TSMesh::castRay (1)");

        // find intersection (time) with this plane...
        // F32 time = dot1 / (dot1-dot2);
        F32 num = mFabs(dot1);
        F32 den = mFabs(dot1 - dot2);

        // the following block of code is an optimized version...
        // this can be commented out and the following block of code used instead
        // if debugging a problem in this code, that should probably be done
        // if you want to see how this works, look at the following block of code,
        // not this one...
        // Note that this does not get optimized appropriately...it is included this way
        // as an idea for future optimization.
        if (sgn * dot1 >= 0)
        {
            sgn *= -1.0f;
            pnum = (F32*)((dsize_t)pnum ^ (dsize_t)&endNum ^ (dsize_t)&startNum);
            pden = (F32*)((dsize_t)pden ^ (dsize_t)&endDen ^ (dsize_t)&startDen);
            pplane = (S32*)((dsize_t)pplane ^ (dsize_t)&tmpPlane ^ (dsize_t)&curPlane);
            pfound = (bool*)((dsize_t)pfound ^ (dsize_t)&tmpFound ^ (dsize_t)&found);
        }
        bool noCollision = num * endDen * sgn < endNum* den* sgn&& num* startDen* sgn < startNum* den* sgn;
        if (num * *pden * sgn < *pnum * den * sgn && !noCollision)
        {
            *pnum = num;
            *pden = den;
            *pplane = i;
            *pfound = true;
        }
        else if (noCollision)
            return false;

        //      if (dot1<=0.0f)
        //      {
        //         // start is inside plane, end is outside...chop off end
        //         if (num*endDen<endNum*den) // if (time<endTime)
        //         {
        //            if (num*startDen<startNum*den) //if (time<startTime)
        //               // no intersection of line and hull
        //               return false;
        //            // endTime = time;
        //            endNum = num;
        //            endDen = den;
        //         }
        //         // else, no need to do anything, just continue (we've been more inside than this)
        //      }
        //      else // dot2<=0.0f
        //      {
        //         // end is inside poly, start is outside...chop off start
        //         AssertFatal(dot2<=0.0f,"TSMesh::castRay (2)");
        //         if (num*startDen>startNum*den) // if (time>startTime)
        //        {
        //            if (num*endDen>endNum*den) //if (time>endTime)
        //               // no intersection of line and hull
        //               return false;
        //            // startTime   = time;
        //            startNum = num;
        //            startDen = den;
        //            curPlane    = i;
        //            curMaterial = planeMaterials[i-startPlane];
        //            found = true;
        //         }
        //         // else, no need to do anything, just continue (we've been more inside than this)
        //      }
    }

    // setup rayInfo
    if (found && rayInfo)
    {
        rayInfo->t = (F32)startNum / (F32)startDen; // finally divide...
        rayInfo->normal = planeNormals[curPlane];
        rayInfo->material = curMaterial;
        return true;
    }
    else if (found)
        return true;

    // only way to get here is if start is inside hull...
    // we could return null and just plug in garbage for the material and normal...
    return false;
}

bool TSMesh::addToHull(U32 idx0, U32 idx1, U32 idx2)
{
    Point3F normal;
    mCross(verts[idx2] - verts[idx0], verts[idx1] - verts[idx0], &normal);
    if (mDot(normal, normal) < 0.001f)
    {
        mCross(verts[idx0] - verts[idx1], verts[idx2] - verts[idx1], &normal);
        if (mDot(normal, normal) < 0.001f)
        {
            mCross(verts[idx1] - verts[idx2], verts[idx0] - verts[idx2], &normal);
            if (mDot(normal, normal) < 0.001f)
                return false;
        }
    }
    normal.normalize();
    F32 k = mDot(normal, verts[idx0]);
    for (S32 i = 0; i < planeNormals.size(); i++)
    {
        if (mDot(planeNormals[i], normal) > 0.99f && mFabs(k - planeConstants[i]) < 0.01f)
            // this is a repeat...
            return false;
    }
    // new plane, add it to the list...
    planeNormals.push_back(normal);
    planeConstants.push_back(k);
    return true;
}

bool TSMesh::buildConvexHull()
{
    // already done, return without error
    if (planeNormals.size())
        return true;

    bool error = false;

    // should probably only have 1 frame, but just in case...
    planesPerFrame = 0;
    S32 frame, i, j;
    for (frame = 0; frame < numFrames; frame++)
    {
        S32 firstVert = vertsPerFrame * frame;
        S32 firstPlane = planeNormals.size();
        for (i = 0; i < primitives.size(); i++)
        {
            TSDrawPrimitive& draw = primitives[i];
            U32 start = draw.start;

            AssertFatal(draw.matIndex & TSDrawPrimitive::Indexed, "TSMesh::buildConvexHull (1)");

            // gonna depend on what kind of primitive it is...
            U32 material = draw.matIndex & TSDrawPrimitive::MaterialMask;
            if ((draw.matIndex & TSDrawPrimitive::TypeMask) == TSDrawPrimitive::Triangles)
            {
                for (j = 0; j < draw.numElements; j += 3)
                    if (addToHull(indices[start + j + 0] + firstVert, indices[start + j + 1] + firstVert, indices[start + j + 2] + firstVert) && frame == 0)
                        planeMaterials.push_back(draw.matIndex & TSDrawPrimitive::MaterialMask);
            }
            else
            {
                AssertFatal((draw.matIndex & TSDrawPrimitive::Strip) == TSDrawPrimitive::Strip, "TSMesh::buildConvexHull (2)");

                U32 idx0 = indices[start + 0] + firstVert;
                U32 idx1;
                U32 idx2 = indices[start + 1] + firstVert;
                U32* nextIdx = &idx1;
                for (j = 2; j < draw.numElements; j++)
                {
                    *nextIdx = idx2;
                    //               nextIdx = (j%2)==0 ? &idx0 : &idx1;
                    nextIdx = (U32*)((dsize_t)nextIdx ^ (dsize_t)&idx0 ^ (dsize_t)&idx1);
                    idx2 = indices[start + j] + firstVert;
                    if (addToHull(idx0, idx1, idx2) && frame == 0)
                        planeMaterials.push_back(draw.matIndex & TSDrawPrimitive::MaterialMask);
                }
            }
        }
        // make sure all the verts on this frame are inside all the planes
        for (i = 0; i < vertsPerFrame; i++)
            for (j = firstPlane; j < planeNormals.size(); j++)
                if (mDot(verts[firstVert + i], planeNormals[j]) - planeConstants[j] < 0.01) // .01 == a little slack
                    error = true;
        if (frame == 0)
            planesPerFrame = planeNormals.size();

        if ((frame + 1) * planesPerFrame != planeNormals.size())
        {
            // eek, not all frames have same number of planes...
            while ((frame + 1) * planesPerFrame > planeNormals.size())
            {
                // we're short, duplicate last plane till we match
                U32 sz = planeNormals.size();
                planeNormals.increment();
                planeNormals.last() = planeNormals[sz - 1];
                planeConstants.increment();
                planeConstants.last() = planeConstants[sz - 1];
            }
            while ((frame + 1) * planesPerFrame < planeNormals.size())
            {
                // harsh -- last frame has more than other frames
                // duplicate last plane in each frame
                for (S32 k = frame - 1; k >= 0; k--)
                {
                    planeNormals.insert(k * planesPerFrame + planesPerFrame);
                    planeNormals[k * planesPerFrame + planesPerFrame] = planeNormals[k * planesPerFrame + planesPerFrame - 1];
                    planeConstants.insert(k * planesPerFrame + planesPerFrame);
                    planeConstants[k * planesPerFrame + planesPerFrame] = planeConstants[k * planesPerFrame + planesPerFrame - 1];
                    if (k == 0)
                    {
                        planeMaterials.increment();
                        planeMaterials.last() = planeMaterials[planeMaterials.size() - 2];
                    }
                }
                planesPerFrame++;
            }
        }
        AssertFatal((frame + 1) * planesPerFrame == planeNormals.size(), "TSMesh::buildConvexHull (3)");
    }
    return !error;
}

//-----------------------------------------------------
// TSMesh bounds methods
//-----------------------------------------------------

void TSMesh::computeBounds()
{
    MatrixF mat(true);
    computeBounds(mat, mBounds, -1, &mCenter, &mRadius);
}

void TSMesh::computeBounds(MatrixF& transform, Box3F& bounds, S32 frame, Point3F* center, F32* radius)
{
    if (frame < 0)
        computeBounds(verts.address(), verts.size(), transform, bounds, center, radius);
    else
        computeBounds(verts.address() + frame * vertsPerFrame, vertsPerFrame, transform, bounds, center, radius);
}

void TSMesh::computeBounds(Point3F* v, S32 numVerts, MatrixF& transform, Box3F& bounds, Point3F* center, F32* radius)
{
    if (!numVerts)
    {
        bounds.min.set(0, 0, 0);
        bounds.max.set(0, 0, 0);
        if (center)
            center->set(0, 0, 0);
        if (radius)
            *radius = 0;
        return;
    }

    S32 i;
    Point3F p;
    transform.mulP(*v, &bounds.min);
    bounds.max = bounds.min;
    for (i = 0; i < numVerts; i++)
    {
        transform.mulP(v[i], &p);
        bounds.max.setMax(p);
        bounds.min.setMin(p);
    }
    Point3F c;
    if (!center)
        center = &c;
    center->x = 0.5f * (bounds.min.x + bounds.max.x);
    center->y = 0.5f * (bounds.min.y + bounds.max.y);
    center->z = 0.5f * (bounds.min.z + bounds.max.z);
    if (radius)
    {
        *radius = 0.0f;
        for (i = 0; i < numVerts; i++)
        {
            transform.mulP(v[i], &p);
            p -= *center;
            *radius = getMax(*radius, mDot(p, p));
        }
        *radius = mSqrt(*radius);
    }
}

//-----------------------------------------------------

S32 TSMesh::getNumPolys()
{
    S32 count = 0;
    for (S32 i = 0; i < primitives.size(); i++)
    {
        if ((primitives[i].matIndex & TSDrawPrimitive::TypeMask) == TSDrawPrimitive::Triangles)
            count += primitives[i].numElements / 3;
        else
            count += primitives[i].numElements - 2;
    }
    return count;
}

//-----------------------------------------------------
// TSMesh destructor
//-----------------------------------------------------

TSMesh::~TSMesh()
{
    if (mOptTree) {
        delete mOptTree;
        mOptTree = NULL;
    }
    if (mOpMeshInterface) {
        delete mOpMeshInterface;
        mOpMeshInterface = NULL;
    }
    //if (mOpTris) {
    //    delete[] mOpTris;
    //    mOpTris = NULL;
    //}
    //if (mOpPoints) {
    //    delete[] mOpPoints;
    //    mOpPoints = NULL;
    //}
}

//-----------------------------------------------------
// TSSkinMesh methods
//-----------------------------------------------------
GFXVertexBufferHandle<MeshVertex>& TSSkinMesh::getVertexBuffer()
{
    // make sure we're playing with a full deck here...
    AssertFatal(TSShapeInstance::smRenderData.currentObjectInstance &&
        TSShapeInstance::smRenderData.currentObjectInstance->getMesh(TSShapeInstance::smRenderData.detailLevel) == this,
        "doh, current mesh object instance is not this mesh");

    // use the vertex buffer from the object instance
    return TSShapeInstance::smRenderData.currentObjectInstance->mVB;
}

Vector<MatrixF> gBoneTransforms;
Vector<Point3F> gSkinVerts;
Vector<Point3F> gSkinNorms;

void TSSkinMesh::updateSkin()
{
    if (smGlowPass || smRefractPass)
    {
        return;
    }

    PROFILE_START(UpdateSkin);

    // set arrays
    gBoneTransforms.setSize(nodeIndex.size());
#if defined(TORQUE_MAX_LIB)
    verts.setSize(initialVerts.size());
    norms.setSize(initialVerts.size());
#else
    if (encodedNorms.size())
    {
        // we co-opt responsibility for updating encoded normals from mesh
        gNormalStore.setSize(vertsPerFrame);
        for (S32 i = 0; i < vertsPerFrame; i++)
            gNormalStore[i] = decodeNormal(encodedNorms[i]);
        initialNorms.set(gNormalStore.address(), vertsPerFrame);
    }
    gSkinVerts.setSize(initialVerts.size());
    gSkinNorms.setSize(initialNorms.size());
    verts.set(gSkinVerts.address(), gSkinVerts.size());
    norms.set(gSkinNorms.address(), gSkinNorms.size());
#endif

    //if (!initialTangents.size())
    //{
    //    dMemcpy(verts.address(), initialVerts.address(), sizeof(Point3F) * verts.size());
    //    dMemcpy(norms.address(), initialNorms.address(), sizeof(Point3F) * norms.size());
    //    createTangents();
    //}

    // set up bone transforms
    S32 i;
    for (i = 0; i < nodeIndex.size(); i++)
    {
        S32 node = nodeIndex[i];
        gBoneTransforms[i].mul(TSShapeInstance::ObjectInstance::smTransforms[node], initialTransforms[i]);
    }

    // multiply verts and normals by boneTransforms
    S32 prevIndex = -1;
    for (i = 0; i < vertexIndex.size(); i++)
    {
        Point3F v0, n0, t0;
        S32 vIndex = vertexIndex[i];
        MatrixF& deltaTransform = gBoneTransforms[boneIndex[i]];
        deltaTransform.mulP(initialVerts[vIndex], &v0);
        deltaTransform.mulV(initialNorms[vIndex], &n0);
        deltaTransform.mulV(initialTangents[vIndex], &t0);
        v0 *= weight[i];
        n0 *= weight[i];
        t0 *= weight[i];
        Point3F& v = verts[vIndex];
        Point3F& n = norms[vIndex];
        Point3F& t = (Point3F&)tangents[vIndex];
        // this should be implemented as a series of conditional moves to avoid the branch
        if (vIndex != prevIndex)
        {
            v.x = 0.0f;
            v.y = 0.0f;
            v.z = 0.0f;
            n.x = 0.0f;
            n.y = 0.0f;
            n.z = 0.0f;
            t.x = 0.0f;
            t.y = 0.0f;
            t.z = 0.0f;
        }
        v += v0;
        n += n0;
        t += t0;
        prevIndex = vIndex;
    }

    // normalize normals...
    for (i = 0; i < norms.size(); i++)
    {
        // gotta do a check now since shared verts between meshes
        // may result in an unused vert in the list...
        Point3F& n = norms[i];
        F32 len2 = mDot(n, n);
        if (len2 > 0.01f)
            norms[i] *= 1.0f / mSqrt(len2);

        // also normalize tangent matrix
        ((Point3F&)tangents[i]).normalize();
    }

    createVBIB();

    PROFILE_END();
}

void TSSkinMesh::render(S32 frame, S32 matFrame, TSMaterialList* materials)
{
    // update verts and normals...
    updateSkin();

    // render...
    Parent::render(frame, matFrame, materials);
}

void TSSkinMesh::renderShadow(S32 frame, const MatrixF& mat, S32 dim, U32* bits, TSMaterialList* materials)
{
    // update verts and normals...
    updateSkin();

    // render...
    Parent::renderShadow(frame, mat, dim, bits, materials);
}


bool TSSkinMesh::buildPolyList(S32 frame, AbstractPolyList* polyList, U32& surfaceKey)
{
    // update verts and normals...
    updateSkin();

    // render...
    return Parent::buildPolyList(frame, polyList, surfaceKey);
}

bool TSSkinMesh::castRay(S32 frame, const Point3F& start, const Point3F& end, RayInfo* rayInfo)
{
    frame, start, end, rayInfo;
    return false;
}

bool TSSkinMesh::buildConvexHull()
{
    return false; // no error, but we don't do anything either...
}

void TSSkinMesh::computeBounds(MatrixF& transform, Box3F& bounds, S32 frame, Point3F* center, F32* radius)
{
    frame;
    TSMesh::computeBounds(initialVerts.address(), initialVerts.size(), transform, bounds, center, radius);
}

//-----------------------------------------------------
// encoded normals
//-----------------------------------------------------

const Point3F TSMesh::smU8ToNormalTable[] =
{
      Point3F(0.565061f, -0.270644f, -0.779396f),
      Point3F(-0.309804f, -0.731114f, 0.607860f),
      Point3F(-0.867412f, 0.472957f, 0.154619f),
      Point3F(-0.757488f, 0.498188f, -0.421925f),
      Point3F(0.306834f, -0.915340f, 0.260778f),
      Point3F(0.098754f, 0.639153f, -0.762713f),
      Point3F(0.713706f, -0.558862f, -0.422252f),
      Point3F(-0.890431f, -0.407603f, -0.202466f),
      Point3F(0.848050f, -0.487612f, -0.207475f),
      Point3F(-0.232226f, 0.776855f, 0.585293f),
      Point3F(-0.940195f, 0.304490f, -0.152706f),
      Point3F(0.602019f, -0.491878f, -0.628991f),
      Point3F(-0.096835f, -0.494354f, -0.863850f),
      Point3F(0.026630f, -0.323659f, -0.945799f),
      Point3F(0.019208f, 0.909386f, 0.415510f),
      Point3F(0.854440f, 0.491730f, 0.167731f),
      Point3F(-0.418835f, 0.866521f, -0.271512f),
      Point3F(0.465024f, 0.409667f, 0.784809f),
      Point3F(-0.674391f, -0.691087f, -0.259992f),
      Point3F(0.303858f, -0.869270f, -0.389922f),
      Point3F(0.991333f, 0.090061f, -0.095640f),
      Point3F(-0.275924f, -0.369550f, 0.887298f),
      Point3F(0.426545f, -0.465962f, 0.775202f),
      Point3F(-0.482741f, -0.873278f, -0.065920f),
      Point3F(0.063616f, 0.932012f, -0.356800f),
      Point3F(0.624786f, -0.061315f, 0.778385f),
      Point3F(-0.530300f, 0.416850f, 0.738253f),
      Point3F(0.312144f, -0.757028f, -0.573999f),
      Point3F(0.399288f, -0.587091f, -0.704197f),
      Point3F(-0.132698f, 0.482877f, 0.865576f),
      Point3F(0.950966f, 0.306530f, 0.041268f),
      Point3F(-0.015923f, -0.144300f, 0.989406f),
      Point3F(-0.407522f, -0.854193f, 0.322925f),
      Point3F(-0.932398f, 0.220464f, 0.286408f),
      Point3F(0.477509f, 0.876580f, 0.059936f),
      Point3F(0.337133f, 0.932606f, -0.128796f),
      Point3F(-0.638117f, 0.199338f, 0.743687f),
      Point3F(-0.677454f, 0.445349f, 0.585423f),
      Point3F(-0.446715f, 0.889059f, -0.100099f),
      Point3F(-0.410024f, 0.909168f, 0.072759f),
      Point3F(0.708462f, 0.702103f, -0.071641f),
      Point3F(-0.048801f, -0.903683f, -0.425411f),
      Point3F(-0.513681f, -0.646901f, 0.563606f),
      Point3F(-0.080022f, 0.000676f, -0.996793f),
      Point3F(0.066966f, -0.991150f, -0.114615f),
      Point3F(-0.245220f, 0.639318f, -0.728793f),
      Point3F(0.250978f, 0.855979f, 0.452006f),
      Point3F(-0.123547f, 0.982443f, -0.139791f),
      Point3F(-0.794825f, 0.030254f, -0.606084f),
      Point3F(-0.772905f, 0.547941f, 0.319967f),
      Point3F(0.916347f, 0.369614f, -0.153928f),
      Point3F(-0.388203f, 0.105395f, 0.915527f),
      Point3F(-0.700468f, -0.709334f, 0.078677f),
      Point3F(-0.816193f, 0.390455f, 0.425880f),
      Point3F(-0.043007f, 0.769222f, -0.637533f),
      Point3F(0.911444f, 0.113150f, 0.395560f),
      Point3F(0.845801f, 0.156091f, -0.510153f),
      Point3F(0.829801f, -0.029340f, 0.557287f),
      Point3F(0.259529f, 0.416263f, 0.871418f),
      Point3F(0.231128f, -0.845982f, 0.480515f),
      Point3F(-0.626203f, -0.646168f, 0.436277f),
      Point3F(-0.197047f, -0.065791f, 0.978184f),
      Point3F(-0.255692f, -0.637488f, -0.726794f),
      Point3F(0.530662f, -0.844385f, -0.073567f),
      Point3F(-0.779887f, 0.617067f, -0.104899f),
      Point3F(0.739908f, 0.113984f, 0.662982f),
      Point3F(-0.218801f, 0.930194f, -0.294729f),
      Point3F(-0.374231f, 0.818666f, 0.435589f),
      Point3F(-0.720250f, -0.028285f, 0.693137f),
      Point3F(0.075389f, 0.415049f, 0.906670f),
      Point3F(-0.539724f, -0.106620f, 0.835063f),
      Point3F(-0.452612f, -0.754669f, -0.474991f),
      Point3F(0.682822f, 0.581234f, -0.442629f),
      Point3F(0.002435f, -0.618462f, -0.785811f),
      Point3F(-0.397631f, 0.110766f, -0.910835f),
      Point3F(0.133935f, -0.985438f, 0.104754f),
      Point3F(0.759098f, -0.608004f, 0.232595f),
      Point3F(-0.825239f, -0.256087f, 0.503388f),
      Point3F(0.101693f, -0.565568f, 0.818408f),
      Point3F(0.386377f, 0.793546f, -0.470104f),
      Point3F(-0.520516f, -0.840690f, 0.149346f),
      Point3F(-0.784549f, -0.479672f, 0.392935f),
      Point3F(-0.325322f, -0.927581f, -0.183735f),
      Point3F(-0.069294f, -0.428541f, 0.900861f),
      Point3F(0.993354f, -0.115023f, -0.004288f),
      Point3F(-0.123896f, -0.700568f, 0.702747f),
      Point3F(-0.438031f, -0.120880f, -0.890795f),
      Point3F(0.063314f, 0.813233f, 0.578484f),
      Point3F(0.322045f, 0.889086f, -0.325289f),
      Point3F(-0.133521f, 0.875063f, -0.465228f),
      Point3F(0.637155f, 0.564814f, 0.524422f),
      Point3F(0.260092f, -0.669353f, 0.695930f),
      Point3F(0.953195f, 0.040485f, -0.299634f),
      Point3F(-0.840665f, -0.076509f, 0.536124f),
      Point3F(-0.971350f, 0.202093f, 0.125047f),
      Point3F(-0.804307f, -0.396312f, -0.442749f),
      Point3F(-0.936746f, 0.069572f, 0.343027f),
      Point3F(0.426545f, -0.465962f, 0.775202f),
      Point3F(0.794542f, -0.227450f, 0.563000f),
      Point3F(-0.892172f, 0.091169f, -0.442399f),
      Point3F(-0.312654f, 0.541264f, 0.780564f),
      Point3F(0.590603f, -0.735618f, -0.331743f),
      Point3F(-0.098040f, -0.986713f, 0.129558f),
      Point3F(0.569646f, 0.283078f, -0.771603f),
      Point3F(0.431051f, -0.407385f, -0.805129f),
      Point3F(-0.162087f, -0.938749f, -0.304104f),
      Point3F(0.241533f, -0.359509f, 0.901341f),
      Point3F(-0.576191f, 0.614939f, 0.538380f),
      Point3F(-0.025110f, 0.085740f, 0.996001f),
      Point3F(-0.352693f, -0.198168f, 0.914515f),
      Point3F(-0.604577f, 0.700711f, 0.378802f),
      Point3F(0.465024f, 0.409667f, 0.784809f),
      Point3F(-0.254684f, -0.030474f, -0.966544f),
      Point3F(-0.604789f, 0.791809f, 0.085259f),
      Point3F(-0.705147f, -0.399298f, 0.585943f),
      Point3F(0.185691f, 0.017236f, -0.982457f),
      Point3F(0.044588f, 0.973094f, 0.226052f),
      Point3F(-0.405463f, 0.642367f, 0.650357f),
      Point3F(-0.563959f, 0.599136f, -0.568319f),
      Point3F(0.367162f, -0.072253f, -0.927347f),
      Point3F(0.960429f, -0.213570f, -0.178783f),
      Point3F(-0.192629f, 0.906005f, 0.376893f),
      Point3F(-0.199718f, -0.359865f, -0.911378f),
      Point3F(0.485072f, 0.121233f, -0.866030f),
      Point3F(0.467163f, -0.874294f, 0.131792f),
      Point3F(-0.638953f, -0.716603f, 0.279677f),
      Point3F(-0.622710f, 0.047813f, -0.780990f),
      Point3F(0.828724f, -0.054433f, -0.557004f),
      Point3F(0.130241f, 0.991080f, 0.028245f),
      Point3F(0.310995f, -0.950076f, -0.025242f),
      Point3F(0.818118f, 0.275336f, 0.504850f),
      Point3F(0.676328f, 0.387023f, 0.626733f),
      Point3F(-0.100433f, 0.495114f, -0.863004f),
      Point3F(-0.949609f, -0.240681f, -0.200786f),
      Point3F(-0.102610f, 0.261831f, -0.959644f),
      Point3F(-0.845732f, -0.493136f, 0.203850f),
      Point3F(0.672617f, -0.738838f, 0.041290f),
      Point3F(0.380465f, 0.875938f, 0.296613f),
      Point3F(-0.811223f, 0.262027f, -0.522742f),
      Point3F(-0.074423f, -0.775670f, -0.626736f),
      Point3F(-0.286499f, 0.755850f, -0.588735f),
      Point3F(0.291182f, -0.276189f, -0.915933f),
      Point3F(-0.638117f, 0.199338f, 0.743687f),
      Point3F(0.439922f, -0.864433f, -0.243359f),
      Point3F(0.177649f, 0.206919f, 0.962094f),
      Point3F(0.277107f, 0.948521f, 0.153361f),
      Point3F(0.507629f, 0.661918f, -0.551523f),
      Point3F(-0.503110f, -0.579308f, -0.641313f),
      Point3F(0.600522f, 0.736495f, -0.311364f),
      Point3F(-0.691096f, -0.715301f, -0.103592f),
      Point3F(-0.041083f, -0.858497f, 0.511171f),
      Point3F(0.207773f, -0.480062f, -0.852274f),
      Point3F(0.795719f, 0.464614f, 0.388543f),
      Point3F(-0.100433f, 0.495114f, -0.863004f),
      Point3F(0.703249f, 0.065157f, -0.707951f),
      Point3F(-0.324171f, -0.941112f, 0.096024f),
      Point3F(-0.134933f, -0.940212f, 0.312722f),
      Point3F(-0.438240f, 0.752088f, -0.492249f),
      Point3F(0.964762f, -0.198855f, 0.172311f),
      Point3F(-0.831799f, 0.196807f, 0.519015f),
      Point3F(-0.508008f, 0.819902f, 0.263986f),
      Point3F(0.471075f, -0.001146f, 0.882092f),
      Point3F(0.919512f, 0.246162f, -0.306435f),
      Point3F(-0.960050f, 0.279828f, -0.001187f),
      Point3F(0.110232f, -0.847535f, -0.519165f),
      Point3F(0.208229f, 0.697360f, 0.685806f),
      Point3F(-0.199680f, -0.560621f, 0.803637f),
      Point3F(0.170135f, -0.679985f, -0.713214f),
      Point3F(0.758371f, -0.494907f, 0.424195f),
      Point3F(0.077734f, -0.755978f, 0.649965f),
      Point3F(0.612831f, -0.672475f, 0.414987f),
      Point3F(0.142776f, 0.836698f, -0.528726f),
      Point3F(-0.765185f, 0.635778f, 0.101382f),
      Point3F(0.669873f, -0.419737f, 0.612447f),
      Point3F(0.593549f, 0.194879f, 0.780847f),
      Point3F(0.646930f, 0.752173f, 0.125368f),
      Point3F(0.837721f, 0.545266f, -0.030127f),
      Point3F(0.541505f, 0.768070f, 0.341820f),
      Point3F(0.760679f, -0.365715f, -0.536301f),
      Point3F(0.381516f, 0.640377f, 0.666605f),
      Point3F(0.565794f, -0.072415f, -0.821361f),
      Point3F(-0.466072f, -0.401588f, 0.788356f),
      Point3F(0.987146f, 0.096290f, 0.127560f),
      Point3F(0.509709f, -0.688886f, -0.515396f),
      Point3F(-0.135132f, -0.988046f, -0.074192f),
      Point3F(0.600499f, 0.476471f, -0.642166f),
      Point3F(-0.732326f, -0.275320f, -0.622815f),
      Point3F(-0.881141f, -0.470404f, 0.048078f),
      Point3F(0.051548f, 0.601042f, 0.797553f),
      Point3F(0.402027f, -0.763183f, 0.505891f),
      Point3F(0.404233f, -0.208288f, 0.890624f),
      Point3F(-0.311793f, 0.343843f, 0.885752f),
      Point3F(0.098132f, -0.937014f, 0.335223f),
      Point3F(0.537158f, 0.830585f, -0.146936f),
      Point3F(0.725277f, 0.298172f, -0.620538f),
      Point3F(-0.882025f, 0.342976f, -0.323110f),
      Point3F(-0.668829f, 0.424296f, -0.610443f),
      Point3F(-0.408835f, -0.476442f, -0.778368f),
      Point3F(0.809472f, 0.397249f, -0.432375f),
      Point3F(-0.909184f, -0.205938f, -0.361903f),
      Point3F(0.866930f, -0.347934f, -0.356895f),
      Point3F(0.911660f, -0.141281f, -0.385897f),
      Point3F(-0.431404f, -0.844074f, -0.318480f),
      Point3F(-0.950593f, -0.073496f, 0.301614f),
      Point3F(-0.719716f, 0.626915f, -0.298305f),
      Point3F(-0.779887f, 0.617067f, -0.104899f),
      Point3F(-0.475899f, -0.542630f, 0.692151f),
      Point3F(0.081952f, -0.157248f, -0.984153f),
      Point3F(0.923990f, -0.381662f, -0.024025f),
      Point3F(-0.957998f, 0.120979f, -0.260008f),
      Point3F(0.306601f, 0.227975f, -0.924134f),
      Point3F(-0.141244f, 0.989182f, 0.039601f),
      Point3F(0.077097f, 0.186288f, -0.979466f),
      Point3F(-0.630407f, -0.259801f, 0.731499f),
      Point3F(0.718150f, 0.637408f, 0.279233f),
      Point3F(0.340946f, 0.110494f, 0.933567f),
      Point3F(-0.396671f, 0.503020f, -0.767869f),
      Point3F(0.636943f, -0.245005f, 0.730942f),
      Point3F(-0.849605f, -0.518660f, -0.095724f),
      Point3F(-0.388203f, 0.105395f, 0.915527f),
      Point3F(-0.280671f, -0.776541f, -0.564099f),
      Point3F(-0.601680f, 0.215451f, -0.769131f),
      Point3F(-0.660112f, -0.632371f, -0.405412f),
      Point3F(0.921096f, 0.284072f, 0.266242f),
      Point3F(0.074850f, -0.300846f, 0.950731f),
      Point3F(0.943952f, -0.067062f, 0.323198f),
      Point3F(-0.917838f, -0.254589f, 0.304561f),
      Point3F(0.889843f, -0.409008f, 0.202219f),
      Point3F(-0.565849f, 0.753721f, -0.334246f),
      Point3F(0.791460f, 0.555918f, -0.254060f),
      Point3F(0.261936f, 0.703590f, -0.660568f),
      Point3F(-0.234406f, 0.952084f, 0.196444f),
      Point3F(0.111205f, 0.979492f, -0.168014f),
      Point3F(-0.869844f, -0.109095f, -0.481113f),
      Point3F(-0.337728f, -0.269701f, -0.901777f),
      Point3F(0.366793f, 0.408875f, -0.835634f),
      Point3F(-0.098749f, 0.261316f, 0.960189f),
      Point3F(-0.272379f, -0.847100f, 0.456324f),
      Point3F(-0.319506f, 0.287444f, -0.902935f),
      Point3F(0.873383f, -0.294109f, 0.388203f),
      Point3F(-0.088950f, 0.710450f, 0.698104f),
      Point3F(0.551238f, -0.786552f, 0.278340f),
      Point3F(0.724436f, -0.663575f, -0.186712f),
      Point3F(0.529741f, -0.606539f, 0.592861f),
      Point3F(-0.949743f, -0.282514f, 0.134809f),
      Point3F(0.155047f, 0.419442f, -0.894443f),
      Point3F(-0.562653f, -0.329139f, -0.758346f),
      Point3F(0.816407f, -0.576953f, 0.024576f),
      Point3F(0.178550f, -0.950242f, -0.255266f),
      Point3F(0.479571f, 0.706691f, 0.520192f),
      Point3F(0.391687f, 0.559884f, -0.730145f),
      Point3F(0.724872f, -0.205570f, -0.657496f),
      Point3F(-0.663196f, -0.517587f, -0.540624f),
      Point3F(-0.660054f, -0.122486f, -0.741165f),
      Point3F(-0.531989f, 0.374711f, -0.759328f),
      Point3F(0.194979f, -0.059120f, 0.979024f)
};

U8 TSMesh::encodeNormal(const Point3F& normal)
{
    U8 bestIndex = 0;
    F32 bestDot = -10E30f;
    for (U32 i = 0; i < 256; i++)
    {
        F32 dot = mDot(normal, smU8ToNormalTable[i]);
        if (dot > bestDot)
        {
            bestIndex = i;
            bestDot = dot;
        }
    }
    return bestIndex;
}

//-----------------------------------------------------
// TSMesh assemble from/ dissemble to memory buffer
//-----------------------------------------------------

#define alloc TSShape::alloc

TSMesh* TSMesh::assembleMesh(U32 meshType, bool skip)
{
    static TSMesh tempStandardMesh;
    static TSSkinMesh tempSkinMesh;
    static TSDecalMesh tempDecalMesh;
    static TSSortedMesh tempSortedMesh;

    bool justSize = skip || !alloc.allocShape32(0); // if this returns NULL, we're just sizing memory block

    // a little funny business because we pretend decals are derived from meshes
    S32* ret = NULL;
    TSMesh* mesh = NULL;
    TSDecalMesh* decal = NULL;

    if (justSize)
    {
        switch (meshType)
        {
        case StandardMeshType:
        {
            ret = (S32*)&tempStandardMesh;
            mesh = &tempStandardMesh;
            alloc.allocShape32(sizeof(TSMesh) >> 2);
            break;
        }
        case SkinMeshType:
        {
            ret = (S32*)&tempSkinMesh;
            mesh = &tempSkinMesh;
            alloc.allocShape32(sizeof(TSSkinMesh) >> 2);
            break;
        }
        case DecalMeshType:
        {
            ret = (S32*)&tempDecalMesh;
            decal = &tempDecalMesh;
            alloc.allocShape32(sizeof(TSDecalMesh) >> 2);
            break;
        }
        case SortedMeshType:
        {
            ret = (S32*)&tempSortedMesh;
            mesh = &tempSortedMesh;
            alloc.allocShape32(sizeof(TSSortedMesh) >> 2);
            break;
        }
        }
    }
    else
    {
        switch (meshType)
        {
        case StandardMeshType:
        {
            ret = alloc.allocShape32(sizeof(TSMesh) >> 2);
            constructInPlace((TSMesh*)ret);
            mesh = (TSMesh*)ret;
            break;
        }
        case SkinMeshType:
        {
            ret = alloc.allocShape32(sizeof(TSSkinMesh) >> 2);
            constructInPlace((TSSkinMesh*)ret);
            mesh = (TSSkinMesh*)ret;
            break;
        }
        case DecalMeshType:
        {
            ret = alloc.allocShape32(sizeof(TSDecalMesh) >> 2);
            constructInPlace((TSDecalMesh*)ret);
            decal = (TSDecalMesh*)ret;
            break;
        }
        case SortedMeshType:
        {
            ret = alloc.allocShape32(sizeof(TSSortedMesh) >> 2);
            constructInPlace((TSSortedMesh*)ret);
            mesh = (TSSortedMesh*)ret;
            break;
        }
        }
    }

    alloc.setSkipMode(skip);

    if (mesh)
        mesh->assemble(skip);
    if (decal)
        decal->assemble(skip);

    alloc.setSkipMode(false);

    return (TSMesh*)ret;
}

void TSMesh::convertToTris(S16* primitiveDataIn, S32* primitiveMatIn, S16* indicesIn,
    S32 numPrimIn, S32& numPrimOut, S32& numIndicesOut,
    S32* primitivesOut, S16* indicesOut)
{
    S32 prevMaterial = -99999;
    TSDrawPrimitive* newDraw = NULL;
    numPrimOut = 0;
    numIndicesOut = 0;
    for (S32 i = 0; i < numPrimIn; i++)
    {
        S32 newMat = primitiveMatIn[i];
        newMat &= ~TSDrawPrimitive::TypeMask;
        if (newMat != prevMaterial)
        {
            if (primitivesOut)
            {
                newDraw = (TSDrawPrimitive*)&primitivesOut[numPrimOut * 2];
                newDraw->start = numIndicesOut;
                newDraw->numElements = 0;
                newDraw->matIndex = newMat | TSDrawPrimitive::Triangles;
            }
            numPrimOut++;
            prevMaterial = newMat;
        }
        U16 start = primitiveDataIn[i * 2];
        U16 numElements = primitiveDataIn[i * 2 + 1];

        // gonna depend on what kind of primitive it is...
        if ((primitiveMatIn[i] & TSDrawPrimitive::TypeMask) == TSDrawPrimitive::Triangles)
        {
            for (S32 j = 0; j < numElements; j += 3)
            {
                if (indicesOut)
                {
                    indicesOut[numIndicesOut + 0] = indicesIn[start + j + 0];
                    indicesOut[numIndicesOut + 1] = indicesIn[start + j + 1];
                    indicesOut[numIndicesOut + 2] = indicesIn[start + j + 2];
                }
                if (newDraw)
                    newDraw->numElements += 3;
                numIndicesOut += 3;
            }
        }
        else
        {
            U32 idx0 = indicesIn[start + 0];
            U32 idx1;
            U32 idx2 = indicesIn[start + 1];
            U32* nextIdx = &idx1;
            for (S32 j = 2; j < numElements; j++)
            {
                *nextIdx = idx2;
                nextIdx = (U32*)((dsize_t)nextIdx ^ (dsize_t)&idx0 ^ (dsize_t)&idx1);
                idx2 = indicesIn[start + j];
                if (idx0 == idx1 || idx1 == idx2 || idx2 == idx0)
                    continue;
                if (indicesOut)
                {
                    indicesOut[numIndicesOut + 0] = idx0;
                    indicesOut[numIndicesOut + 1] = idx1;
                    indicesOut[numIndicesOut + 2] = idx2;
                }
                if (newDraw)
                    newDraw->numElements += 3;
                numIndicesOut += 3;
            }
        }
    }
}

void unwindStrip(S16* indices, S32 numElements, Vector<S16>& triIndices)
{
    U32 idx0 = indices[0];
    U32 idx1;
    U32 idx2 = indices[1];
    U32* nextIdx = &idx1;
    for (S32 j = 2; j < numElements; j++)
    {
        *nextIdx = idx2;
        nextIdx = (U32*)((dsize_t)nextIdx ^ (dsize_t)&idx0 ^ (dsize_t)&idx1);
        idx2 = indices[j];
        if (idx0 == idx1 || idx1 == idx2 || idx2 == idx0)
            continue;
        triIndices.push_back(idx0);
        triIndices.push_back(idx1);
        triIndices.push_back(idx2);
    }
}

void TSMesh::convertToSingleStrip(S16* primitiveDataIn, S32* primitiveMatIn, S16* indicesIn, S32 numPrimIn,
    S32& numPrimOut, S32& numIndicesOut,
    S32* primitivesOut, S16* indicesOut)
{
    S32 prevMaterial = -99999;
    TSDrawPrimitive* newDraw = NULL;
    TSDrawPrimitive* newTris = NULL;
    Vector<S16> triIndices;
    S32 curDrawOut = 0;
    numPrimOut = 0;
    numIndicesOut = 0;
    for (S32 i = 0; i < numPrimIn; i++)
    {
        S32 newMat = primitiveMatIn[i];
        if (newMat != prevMaterial)
        {
            // before adding the new primitive, transfer triangle indices
            if (triIndices.size())
            {
                if (newTris && indicesOut)
                {
                    newTris->start = numIndicesOut;
                    newTris->numElements = triIndices.size();
                    dMemcpy(&indicesOut[numIndicesOut], triIndices.address(), triIndices.size() * sizeof(U16));
                }
                numIndicesOut += triIndices.size();
                triIndices.clear();
                newTris = NULL;
            }

            if (primitivesOut)
            {
                newDraw = (TSDrawPrimitive*)&primitivesOut[numPrimOut * 2];
                newDraw->start = numIndicesOut;
                newDraw->numElements = 0;
                newDraw->matIndex = newMat;
            }
            numPrimOut++;
            curDrawOut = 0;
            prevMaterial = newMat;
        }
        U16 start = primitiveDataIn[i * 2];
        U16 numElements = primitiveDataIn[i * 2 + 1];

        // gonna depend on what kind of primitive it is...
        // from above we know it's the same kind as the one we're building...
        if ((primitiveMatIn[i] & TSDrawPrimitive::TypeMask) == TSDrawPrimitive::Triangles)
        {
            // triangles primitive...add to it
            for (S32 j = 0; j < numElements; j += 3)
            {
                if (indicesOut)
                {
                    indicesOut[numIndicesOut + 0] = indicesIn[start + j + 0];
                    indicesOut[numIndicesOut + 1] = indicesIn[start + j + 1];
                    indicesOut[numIndicesOut + 2] = indicesIn[start + j + 2];
                }
                if (newDraw)
                    newDraw->numElements += 3;
                numIndicesOut += 3;
            }
        }
        else
        {
            // strip primitive...
            // if numElements less than smSmallestStripSize, add to triangles...
            if (numElements < smMinStripSize + 2)
            {
                // put triangle indices aside until material changes...
                if (triIndices.empty())
                {
                    // set up for new triangle primitive and add it if we are copying data right now
                    if (primitivesOut)
                    {
                        newTris = (TSDrawPrimitive*)&primitivesOut[numPrimOut * 2];
                        newTris->matIndex = newMat;
                        newTris->matIndex &= ~(TSDrawPrimitive::Triangles | TSDrawPrimitive::Strip);
                        newTris->matIndex |= TSDrawPrimitive::Triangles;
                    }
                    numPrimOut++;
                }
                unwindStrip(indicesIn + start, numElements, triIndices);
            }
            else
            {
                // strip primitive...add to it
                if (indicesOut)
                {
                    if (curDrawOut & 1)
                    {
                        indicesOut[numIndicesOut + 0] = indicesOut[numIndicesOut - 1];
                        indicesOut[numIndicesOut + 1] = indicesOut[numIndicesOut - 1];
                        indicesOut[numIndicesOut + 2] = indicesIn[start];
                        dMemcpy(indicesOut + numIndicesOut + 3, indicesIn + start, 2 * numElements);
                    }
                    else if (curDrawOut)
                    {
                        indicesOut[numIndicesOut + 0] = indicesOut[numIndicesOut - 1];
                        indicesOut[numIndicesOut + 1] = indicesIn[start];
                        dMemcpy(indicesOut + numIndicesOut + 2, indicesIn + start, 2 * numElements);
                    }
                    else
                        dMemcpy(indicesOut + numIndicesOut, indicesIn + start, 2 * numElements);
                }
                S32 added = numElements;
                added += curDrawOut ? (curDrawOut & 1 ? 3 : 2) : 0;
                if (newDraw)
                    newDraw->numElements += added;
                numIndicesOut += added;
                curDrawOut += added;
            }
        }
    }
    // spit out tris before leaving
    // before adding the new primitive, transfer triangle indices
    if (triIndices.size())
    {
        if (newTris && indicesOut)
        {
            newTris->start = numIndicesOut;
            newTris->numElements = triIndices.size();
            dMemcpy(&indicesOut[numIndicesOut], triIndices.address(), triIndices.size() * sizeof(U16));
        }
        numIndicesOut += triIndices.size();
        triIndices.clear();
        newTris = NULL;
    }
}

// this method does none of the converting that the above methods do, except that small strips are converted
// to triangle lists...
void TSMesh::leaveAsMultipleStrips(S16* primitiveDataIn, S32* primitiveMatIn, S16* indicesIn, S32 numPrimIn,
    S32& numPrimOut, S32& numIndicesOut,
    S32* primitivesOut, S16* indicesOut)
{
    S32 prevMaterial = -99999;
    TSDrawPrimitive* newDraw = NULL;
    Vector<S16> triIndices;
    numPrimOut = 0;
    numIndicesOut = 0;
    for (S32 i = 0; i < numPrimIn; i++)
    {
        S32 newMat = primitiveMatIn[i];

        U16 start = primitiveDataIn[i * 2];
        U16 numElements = primitiveDataIn[i * 2 + 1];

        if (newMat != prevMaterial && triIndices.size())
        {
            // material just changed and we have triangles lying around
            // add primitive and indices for triangles and clear triIndices
            if (indicesOut)
            {
                TSDrawPrimitive* newTris = (TSDrawPrimitive*)&primitivesOut[numPrimOut * 2];
                newTris->matIndex = prevMaterial;
                newTris->matIndex &= ~(TSDrawPrimitive::Triangles | TSDrawPrimitive::Strip);
                newTris->matIndex |= TSDrawPrimitive::Triangles;
                newTris->start = numIndicesOut;
                newTris->numElements = triIndices.size();
                dMemcpy(&indicesOut[numIndicesOut], triIndices.address(), triIndices.size() * sizeof(U16));
            }
            numPrimOut++;
            numIndicesOut += triIndices.size();
            triIndices.clear();
        }

        // this is a little convoluted because this code was adapted from convertToSingleStrip
        // but we will need a new primitive only if it is a triangle primitive coming in
        // or we have more elements than the min strip size...
        if ((primitiveMatIn[i] & TSDrawPrimitive::TypeMask) == TSDrawPrimitive::Triangles || numElements >= smMinStripSize + 2)
        {
            if (primitivesOut)
            {
                newDraw = (TSDrawPrimitive*)&primitivesOut[numPrimOut * 2];
                newDraw->start = numIndicesOut;
                newDraw->numElements = 0;
                newDraw->matIndex = newMat;
            }
            numPrimOut++;
        }
        prevMaterial = newMat;

        // gonna depend on what kind of primitive it is...
        // from above we know it's the same kind as the one we're building...
        if ((primitiveMatIn[i] & TSDrawPrimitive::TypeMask) == TSDrawPrimitive::Triangles)
        {
            // triangles primitive...add to it
            for (S32 j = 0; j < numElements; j += 3)
            {
                if (indicesOut)
                {
                    indicesOut[numIndicesOut + 0] = indicesIn[start + j + 0];
                    indicesOut[numIndicesOut + 1] = indicesIn[start + j + 1];
                    indicesOut[numIndicesOut + 2] = indicesIn[start + j + 2];
                }
                if (newDraw)
                    newDraw->numElements += 3;
                numIndicesOut += 3;
            }
        }
        else
        {
            // strip primitive...
            // if numElements less than smSmallestStripSize, add to triangles...
            if (numElements < smMinStripSize + 2)
                // put triangle indices aside until material changes...
                unwindStrip(indicesIn + start, numElements, triIndices);
            else
            {
                // strip primitive...add to it
                if (indicesOut)
                    dMemcpy(indicesOut + numIndicesOut, indicesIn + start, 2 * numElements);
                if (newDraw)
                    newDraw->numElements = numElements;
                numIndicesOut += numElements;
            }
        }
    }
    // spit out tris before leaving
    if (triIndices.size())
    {
        // material just changed and we have triangles lying around
        // add primitive and indices for triangles and clear triIndices
        if (indicesOut)
        {
            TSDrawPrimitive* newTris = (TSDrawPrimitive*)&primitivesOut[numPrimOut * 2];
            newTris->matIndex = prevMaterial;
            newTris->matIndex &= ~(TSDrawPrimitive::Triangles | TSDrawPrimitive::Strip);
            newTris->matIndex |= TSDrawPrimitive::Triangles;
            newTris->start = numIndicesOut;
            newTris->numElements = triIndices.size();
            dMemcpy(&indicesOut[numIndicesOut], triIndices.address(), triIndices.size() * sizeof(U16));
        }
        numPrimOut++;
        numIndicesOut += triIndices.size();
        triIndices.clear();
    }
}

// This method retrieves data that is shared (or possibly shared) between different meshes.
// This adds an extra step to the copying of data from the memory buffer to the shape data buffer.
// If we have no parentMesh, then we either return a pointer to the data in the memory buffer
// (in the case that we skip this mesh) or copy the data into the shape data buffer and return
// that pointer (in the case that we don't skip this mesh).
// If we do have a parent mesh, then we return a pointer to the data in the shape buffer,
// copying the data in there ourselves if our parent didn't already do it (i.e., if it was skipped).
S32* TSMesh::getSharedData32(S32 parentMesh, S32 size, S32** source, bool skip)
{
    S32* ptr;
    if (parentMesh < 0)
        ptr = skip ? alloc.getPointer32(size) : alloc.copyToShape32(size);
    else
    {
        ptr = source[parentMesh];
        // if we skipped the previous mesh (and we're not skipping this one) then
        // we still need to copy points into the shape...
        if (!smDataCopied[parentMesh] && !skip)
        {
            S32* tmp = ptr;
            ptr = alloc.allocShape32(size);
            if (ptr && tmp)
                dMemcpy(ptr, tmp, size * sizeof(S32));
        }
    }
    return ptr;
}

S8* TSMesh::getSharedData8(S32 parentMesh, S32 size, S8** source, bool skip)
{
    S8* ptr;
    if (parentMesh < 0)
        ptr = skip ? alloc.getPointer8(size) : alloc.copyToShape8(size);
    else
    {
        ptr = source[parentMesh];
        // if we skipped the previous mesh (and we're not skipping this one) then
        // we still need to copy points into the shape...
        if (!smDataCopied[parentMesh] && !skip)
        {
            S8* tmp = ptr;
            ptr = alloc.allocShape8(size);
            if (ptr && tmp)
                dMemcpy(ptr, tmp, size * sizeof(S32));
        }
    }
    return ptr;
}

//-----------------------------------------------------------------------------
// Create vertex and index buffers
//-----------------------------------------------------------------------------
void TSMesh::createVBIB()
{
    if (!verts.size() || !&verts[0])
        return;

    if (!GFXDevice::devicePresent())
    {
        return;
    }

    //mDynamic = false;

    PROFILE_START(CreateVBIB);


    MeshVertex* tempVerts = new MeshVertex[verts.size()];

    // fill in basic info
    for (U32 i = 0; i < verts.size(); i++)
    {
        tempVerts[i].point = verts[i];
        tempVerts[i].texCoord = tverts[i];
        tempVerts[i].normal = norms[i];
    }

    // expensive operation - optimize
    fillTextureSpaceInfo(tempVerts);

    // copy to video mem

    mVB.set(GFX, verts.size(), mDynamic ? GFXBufferTypeVolatile : GFXBufferTypeStatic);
    MeshVertex* vbVerts = mVB.lock();

    dMemcpy(vbVerts, tempVerts, sizeof(MeshVertex) * verts.size());

    mVB.unlock();

    delete[] tempVerts;

    // go through and create PrimitiveInfo array
    Vector <GFXPrimitive> piArray;
    for (S32 i = 0; i < primitives.size(); i++)
    {
        GFXPrimitive pInfo;

        TSDrawPrimitive& draw = primitives[i];

        GFXPrimitiveType drawType = getDrawType(draw.matIndex >> 30);


        switch (drawType)
        {
        case GFXTriangleList:
            pInfo.type = drawType;
            pInfo.numPrimitives = draw.numElements / 3;
            pInfo.startIndex = draw.start;
            pInfo.minIndex = 0;
            pInfo.numVertices = verts.size();
            break;

        case GFXTriangleStrip:
        case GFXTriangleFan:
            pInfo.type = drawType;
            pInfo.numPrimitives = draw.numElements - 2;
            pInfo.startIndex = draw.start;
            pInfo.minIndex = 0;
            pInfo.numVertices = verts.size();
            break;


        default:
            AssertFatal(false, "WTF?!");
        }

        piArray.push_back(pInfo);
    }

    U16* ibIndices;
    GFXPrimitive* piInput;
    mPB.set(GFX, indices.size(), piArray.size(), mDynamic ? GFXBufferTypeVolatile : GFXBufferTypeStatic);
    mPB.lock(&ibIndices, &piInput);

    dMemcpy(ibIndices, indices.address(), indices.size() * sizeof(U16));
    dMemcpy(piInput, piArray.address(), piArray.size() * sizeof(GFXPrimitive));

    mPB.unlock();

    PROFILE_END();
}


void TSMesh::assemble(bool skip)
{
    alloc.checkGuard();

    numFrames = alloc.get32();
    numMatFrames = alloc.get32();
    parentMesh = alloc.get32();
    alloc.get32((S32*)&mBounds, 6);
    alloc.get32((S32*)&mCenter, 3);
    mRadius = (F32)alloc.get32();

    S32 numVerts = alloc.get32();
    S32* ptr32 = getSharedData32(parentMesh, 3 * numVerts, (S32**)smVertsList.address(), skip);
    verts.set((Point3F*)ptr32, numVerts);

    S32 numTVerts = alloc.get32();
    ptr32 = getSharedData32(parentMesh, 2 * numTVerts, (S32**)smTVertsList.address(), skip);
    tverts.set((Point2F*)ptr32, numTVerts);

    S8* ptr8;
    if (TSShape::smReadVersion > 21 && TSMesh::smUseEncodedNormals)
    {
        // we have encoded normals and we want to use them...

        if (parentMesh < 0)
            alloc.getPointer32(numVerts * 3); // advance past norms, don't use
        norms.set(NULL, 0);

        ptr8 = getSharedData8(parentMesh, numVerts, (S8**)smEncodedNormsList.address(), skip);
        encodedNorms.set(ptr8, numVerts);
    }
    else if (TSShape::smReadVersion > 21)
    {
        // we have encoded normals but we don't want to use them...

        ptr32 = getSharedData32(parentMesh, 3 * numVerts, (S32**)smNormsList.address(), skip);
        norms.set((Point3F*)ptr32, numVerts);

        if (parentMesh < 0)
            alloc.getPointer8(numVerts); // advance past encoded normls, don't use
        encodedNorms.set(NULL, 0);
    }
    else
    {
        // no encoded normals...

        ptr32 = getSharedData32(parentMesh, 3 * numVerts, (S32**)smNormsList.address(), skip);
        norms.set((Point3F*)ptr32, numVerts);
        encodedNorms.set(NULL, 0);
    }

    // copy the primitives and indices...how we do this depends on what
    // form we want them in when copied...just get pointers to data for now
    S32 szPrim = alloc.get32();
    S16* prim16 = alloc.getPointer16(szPrim * 2);
    S32* prim32 = alloc.getPointer32(szPrim);
    S32 szInd = alloc.get32();
    S16* ind16 = alloc.getPointer16(szInd);

    // count then copy...
    S32 cpyPrim = szPrim, cpyInd = szInd;
    if (smUseTriangles)
        convertToTris(prim16, prim32, ind16, szPrim, cpyPrim, cpyInd, NULL, NULL);
    else if (smUseOneStrip)
        convertToSingleStrip(prim16, prim32, ind16, szPrim, cpyPrim, cpyInd, NULL, NULL);
    else
        leaveAsMultipleStrips(prim16, prim32, ind16, szPrim, cpyPrim, cpyInd, NULL, NULL);
    ptr32 = alloc.allocShape32(2 * cpyPrim);
    S16* ptr16 = alloc.allocShape16(cpyInd);
    alloc.align32();
    S32 chkPrim = szPrim, chkInd = szInd;
    if (smUseTriangles)
        convertToTris(prim16, prim32, ind16, szPrim, chkPrim, chkInd, ptr32, ptr16);
    else if (smUseOneStrip)
        convertToSingleStrip(prim16, prim32, ind16, szPrim, chkPrim, chkInd, ptr32, ptr16);
    else
        leaveAsMultipleStrips(prim16, prim32, ind16, szPrim, chkPrim, chkInd, ptr32, ptr16);
    AssertFatal(chkPrim == cpyPrim && chkInd == cpyInd, "TSMesh::primitive conversion");
    primitives.set(ptr32, cpyPrim);
    indices.set(ptr16, cpyInd);

    S32 sz = alloc.get32();
    ptr16 = alloc.copyToShape16(sz);
    alloc.align32();
    mergeIndices.set(ptr16, sz);

    vertsPerFrame = alloc.get32();
    U32 flags = (U32)alloc.get32();
    if (encodedNorms.size())
        flags |= UseEncodedNormals;
    setFlags(flags);

    alloc.checkGuard();

    if (alloc.allocShape32(0))// && TSShape::smReadVersion < 19)
        // only do this if we copied the data...
        computeBounds();

    if (alloc.allocShape32(0) && meshType != SkinMeshType)
    {
        createVBIB();
    }
}

void TSMesh::disassemble()
{
    alloc.setGuard();

    alloc.set32(numFrames);
    alloc.set32(numMatFrames);
    alloc.set32(parentMesh);
    alloc.copyToBuffer32((S32*)&mBounds, 6);
    alloc.copyToBuffer32((S32*)&mCenter, 3);
    alloc.set32((S32)mRadius);

    // verts...
    alloc.set32(verts.size());
    if (parentMesh < 0)
        // if no parent mesh, then save off our verts
        alloc.copyToBuffer32((S32*)verts.address(), 3 * verts.size());

    // tverts...
    alloc.set32(tverts.size());
    if (parentMesh < 0)
        // if no parent mesh, then save off our tverts
        alloc.copyToBuffer32((S32*)tverts.address(), 2 * tverts.size());

    // norms...
    if (parentMesh < 0)
        // if no parent mesh, then save off our norms
        alloc.copyToBuffer32((S32*)norms.address(), 3 * norms.size()); // norms.size()==verts.size() or error...

     // encoded norms...
    if (parentMesh < 0)
    {
        // if no parent mesh, compute encoded normals and copy over
        for (S32 i = 0; i < norms.size(); i++)
        {
            U8 normIdx = encodedNorms.size() ? encodedNorms[i] : encodeNormal(norms[i]);
            alloc.copyToBuffer8((S8*)&normIdx, 1);
        }
    }

    // primitives...
    alloc.set32(primitives.size());
    for (S32 i = 0; i < primitives.size(); i++)
    {
        alloc.copyToBuffer16((S16*)&primitives[i], 2);
        alloc.copyToBuffer32(((S32*)&primitives[i]) + 1, 1);
    }

    // indices...
    alloc.set32(indices.size());
    alloc.copyToBuffer16((S16*)indices.address(), indices.size());

    // merge indices...
    alloc.set32(mergeIndices.size());
    alloc.copyToBuffer16((S16*)mergeIndices.address(), mergeIndices.size());

    // small stuff...
    alloc.set32(vertsPerFrame);
    alloc.set32(getFlags());

    alloc.setGuard();
}

//-----------------------------------------------------------------------------
// TSSkinMesh assemble from/ dissemble to memory buffer
//-----------------------------------------------------------------------------
void TSSkinMesh::assemble(bool skip)
{
    // avoid a crash on computeBounds...
    initialVerts.set(NULL, 0);

    TSMesh::assemble(skip);

    S32 sz = alloc.get32();
    S32 numVerts = sz;
    S32* ptr32 = getSharedData32(parentMesh, 3 * numVerts, (S32**)smVertsList.address(), skip);
    initialVerts.set((Point3F*)ptr32, sz);

    S8* ptr8;
    if (TSShape::smReadVersion > 21 && TSMesh::smUseEncodedNormals)
    {
        // we have encoded normals and we want to use them...
        if (parentMesh < 0)
            alloc.getPointer32(numVerts * 3); // advance past norms, don't use
        initialNorms.set(NULL, 0);

        ptr8 = getSharedData8(parentMesh, numVerts, (S8**)smEncodedNormsList.address(), skip);
        encodedNorms.set(ptr8, numVerts);
        // Note: we don't set the encoded normals flag because we handle them in updateSkin and
        //       hide the fact that we are using them from base class (TSMesh)
    }
    else if (TSShape::smReadVersion > 21)
    {
        // we have encoded normals but we don't want to use them...
        ptr32 = getSharedData32(parentMesh, 3 * numVerts, (S32**)smNormsList.address(), skip);
        initialNorms.set((Point3F*)ptr32, numVerts);

        if (parentMesh < 0)
            alloc.getPointer8(numVerts); // advance past encoded normls, don't use
        encodedNorms.set(NULL, 0);
    }
    else
    {
        // no encoded normals...
        ptr32 = getSharedData32(parentMesh, 3 * numVerts, (S32**)smNormsList.address(), skip);
        initialNorms.set((Point3F*)ptr32, numVerts);
        encodedNorms.set(NULL, 0);
    }

    sz = alloc.get32();
    ptr32 = getSharedData32(parentMesh, 16 * sz, (S32**)smInitTransformList.address(), skip);
    initialTransforms.set(ptr32, sz);

    sz = alloc.get32();
    ptr32 = getSharedData32(parentMesh, sz, (S32**)smVertexIndexList.address(), skip);
    vertexIndex.set(ptr32, sz);

    ptr32 = getSharedData32(parentMesh, sz, (S32**)smBoneIndexList.address(), skip);
    boneIndex.set(ptr32, sz);

    ptr32 = getSharedData32(parentMesh, sz, (S32**)smWeightList.address(), skip);
    weight.set((F32*)ptr32, sz);

    sz = alloc.get32();
    ptr32 = getSharedData32(parentMesh, sz, (S32**)smNodeIndexList.address(), skip);
    nodeIndex.set(ptr32, sz);

    alloc.checkGuard();

    if (alloc.allocShape32(0))// && TSShape::smReadVersion < 19)
        // only do this if we copied the data...
        TSMesh::computeBounds();
}

//-----------------------------------------------------------------------------
// disassemble
//-----------------------------------------------------------------------------
void TSSkinMesh::disassemble()
{
    TSMesh::disassemble();

    alloc.set32(initialVerts.size());
    // if we have no parent mesh, then save off our verts & norms
    if (parentMesh < 0)
    {
        alloc.copyToBuffer32((S32*)initialVerts.address(), 3 * initialVerts.size());

        // no longer do this here...let tsmesh handle this
        alloc.copyToBuffer32((S32*)initialNorms.address(), 3 * initialNorms.size());

        // if no parent mesh, compute encoded normals and copy over
        for (S32 i = 0; i < initialNorms.size(); i++)
        {
            U8 normIdx = encodedNorms.size() ? encodedNorms[i] : encodeNormal(initialNorms[i]);
            alloc.copyToBuffer8((S8*)&normIdx, 1);
        }
    }

    alloc.set32(initialTransforms.size());
    if (parentMesh < 0)
        alloc.copyToBuffer32((S32*)initialTransforms.address(), initialTransforms.size() * 16);

    alloc.set32(vertexIndex.size());
    if (parentMesh < 0)
    {
        alloc.copyToBuffer32((S32*)vertexIndex.address(), vertexIndex.size());

        alloc.copyToBuffer32((S32*)boneIndex.address(), boneIndex.size());

        alloc.copyToBuffer32((S32*)weight.address(), weight.size());
    }

    alloc.set32(nodeIndex.size());
    if (parentMesh < 0)
        alloc.copyToBuffer32((S32*)nodeIndex.address(), nodeIndex.size());

    alloc.setGuard();
}


//-----------------------------------------------------------------------------
// createTextureSpaceMatrix
//-----------------------------------------------------------------------------
#define SMALL_FLOAT (1e-12)
void TSMesh::createTextureSpaceMatrix(MeshVertex* v0, MeshVertex* v1, MeshVertex* v2)
{
    Point3F edge1;
    Point3F edge2;
    Point3F cp;
 

    // x, s, t
    edge1.set(v1->point.x - v0->point.x, v1->texCoord.x - v0->texCoord.x, v1->texCoord.y - v0->texCoord.y);
    edge2.set(v2->point.x - v0->point.x, v2->texCoord.x - v0->texCoord.x, v2->texCoord.y - v0->texCoord.y);

    float fVar1 = v1->point.x - v0->point.x;
    float fVar2 = v2->point.x - v0->point.x;
    float fVar4 = v1->texCoord.x - v0->texCoord.x;
    float fVar5 = v2->texCoord.x - v0->texCoord.x;
    float fVar6 = v1->texCoord.y - v0->texCoord.y;
    float fVar7 = v2->texCoord.y - v0->texCoord.y;
    float fVar3 = fVar7 * fVar4 - fVar5 * fVar6;
    if (1e-12 < fabs(fVar3)) {
        fVar3 = 1.0 / fVar3;
        fVar4 = -(fVar3 * (fVar5 * fVar1 - fVar4 * fVar2));
        fVar1 = -(fVar3 * (fVar6 * fVar2 - fVar7 * fVar1));
        v0->T.x = fVar1;
        v1->T.x = fVar1;
        v2->T.x = fVar1;
        v0->B.x = fVar4;
        v1->B.x = fVar4;
        v2->B.x = fVar4;
    }
    fVar1 = v1->texCoord.x - v0->texCoord.x;
    fVar2 = v2->texCoord.x - v0->texCoord.x;
    fVar3 = v1->texCoord.y - v0->texCoord.y;
    fVar6 = v1->point.y - v0->point.y;
    fVar4 = v2->texCoord.y - v0->texCoord.y;
    fVar7 = v2->point.y - v0->point.y;
    fVar5 = fVar4 * fVar1 - fVar2 * fVar3;
    if (1e-12 < fabs(fVar5)) {
        fVar5 = 1.0 / fVar5;
        fVar3 = -(fVar5 * (fVar3 * fVar7 - fVar4 * fVar6));
        fVar1 = -(fVar5 * (fVar2 * fVar6 - fVar1 * fVar7));
        v0->B.y = fVar1;
        v1->B.y = fVar1;
        v2->B.y = fVar1;
        v0->T.y = fVar3;
        v1->T.y = fVar3;
        v2->T.y = fVar3;
    }
    fVar1 = v1->texCoord.x - v0->texCoord.x;
    fVar2 = v2->texCoord.x - v0->texCoord.x;
    fVar3 = v1->texCoord.y - v0->texCoord.y;
    fVar5 = v1->point.z - v0->point.z;
    fVar4 = v2->texCoord.y - v0->texCoord.y;
    fVar6 = v2->point.z - v0->point.z;
    fVar7 = fVar4 * fVar1 - fVar2 * fVar3;
    if (1e-12 < fabs(fVar7)) {
        fVar7 = 1.0 / fVar7;
        fVar3 = -(fVar7 * (fVar3 * fVar6 - fVar4 * fVar5));
        fVar1 = -(fVar7 * (fVar2 * fVar5 - fVar1 * fVar6));
        v0->B.z = fVar1;
        v1->B.z = fVar1;
        v2->B.z = fVar1;
        v0->T.z = fVar3;
        v1->T.z = fVar3;
        v2->T.z = fVar3;
    }

    // v0
    v0->T.normalizeSafe();
    v0->B.normalizeSafe();
    mCross(v0->T, v0->B, &v0->N);
    if (mDot(v0->N, v0->normal) < 0.0)
    {
        v0->N = -v0->N;
    }

    // v1
    v1->T.normalizeSafe();
    v1->B.normalizeSafe();
    mCross(v1->T, v1->B, &v1->N);
    if (mDot(v1->N, v1->normal) < 0.0)
    {
        v1->N = -v1->N;
    }

    // v2
    v2->T.normalizeSafe();
    v2->B.normalizeSafe();
    mCross(v2->T, v2->B, &v2->N);
    if (mDot(v2->N, v2->normal) < 0.0)
    {
        v2->N = -v2->N;
    }

}

//-----------------------------------------------------------------------------
// Fills in texture space matrix portion of each vertex - for bumpmapping
//-----------------------------------------------------------------------------
void TSMesh::fillTextureSpaceInfo(MeshVertex* vertArray)
{
    for (S32 i = 0; i < primitives.size(); i++)
    {
        TSDrawPrimitive& draw = primitives[i];
        GFXPrimitiveType drawType = getDrawType(draw.matIndex >> 30);


        U32 numPrims = 0;
        U32 p1Index = 0;
        U32 p2Index = 0;

        U16* baseIdx = &indices[draw.start];

        switch (drawType)
        {
        case GFXTriangleList:
        {
            for (U32 j = 0; j < draw.numElements; j += 3)
            {
                createTextureSpaceMatrix(&vertArray[baseIdx[j]], &vertArray[baseIdx[j + 1]], &vertArray[baseIdx[j + 2]]);
            }
            break;
        }

        case GFXTriangleStrip:
        {
            p1Index = baseIdx[0];
            p2Index = baseIdx[1];
            for (U32 j = 2; j < draw.numElements; j++)
            {
                createTextureSpaceMatrix(&vertArray[p1Index], &vertArray[p2Index], &vertArray[baseIdx[j]]);
                p1Index = p2Index;
                p2Index = baseIdx[j];
            }
            break;
        }
        case GFXTriangleFan:
        {
            p1Index = baseIdx[0];
            p2Index = baseIdx[1];
            for (U32 j = 2; j < draw.numElements; j++)
            {
                createTextureSpaceMatrix(&vertArray[p1Index], &vertArray[p2Index], &vertArray[baseIdx[j]]);
                p2Index = baseIdx[j];
            }
            break;
        }

        default:
            AssertFatal(false, "WTF?!");
        }

    }

}

//-----------------------------------------------------------------------------
// find tangent vector
//-----------------------------------------------------------------------------
inline void TSMesh::findTangent(U32 index1,
    U32 index2,
    U32 index3,
    Point3F* tan0,
    Point3F* tan1,
    const Vector<Point3F>& _verts)
{
    const Point3F& v1 = _verts[index1];
    const Point3F& v2 = _verts[index2];
    const Point3F& v3 = _verts[index3];

    const Point2F& w1 = tverts[index1];
    const Point2F& w2 = tverts[index2];
    const Point2F& w3 = tverts[index3];

    F32 x1 = v2.x - v1.x;
    F32 x2 = v3.x - v1.x;
    F32 y1 = v2.y - v1.y;
    F32 y2 = v3.y - v1.y;
    F32 z1 = v2.z - v1.z;
    F32 z2 = v3.z - v1.z;

    F32 s1 = w2.x - w1.x;
    F32 s2 = w3.x - w1.x;
    F32 t1 = w2.y - w1.y;
    F32 t2 = w3.y - w1.y;

    F32 denom = (s1 * t2 - s2 * t1);

    if (mFabs(denom) < 0.0001f)
    {
        // handle degenerate triangles from strips
        if (denom < 0) denom = -0.0001f;
        else denom = 0.0001f;
    }
    F32 r = 1.0f / denom;

    Point3F sdir((t2 * x1 - t1 * x2) * r,
        (t2 * y1 - t1 * y2) * r,
        (t2 * z1 - t1 * z2) * r);

    Point3F tdir((s1 * x2 - s2 * x1) * r,
        (s1 * y2 - s2 * y1) * r,
        (s1 * z2 - s2 * z1) * r);


    tan0[index1] += sdir;
    tan1[index1] += tdir;

    tan0[index2] += sdir;
    tan1[index2] += tdir;

    tan0[index3] += sdir;
    tan1[index3] += tdir;
}

//-----------------------------------------------------------------------------
// create array of tangent vectors
//-----------------------------------------------------------------------------
void TSMesh::createTangents(const Vector<Point3F>& _verts, const Vector<Point3F>& _norms)
{
    if (_verts.size() == 0) // can only be done in editable mode
        return;

    U32 numVerts = _verts.size();
    U32 numNorms = _norms.size();
    if (numVerts <= 0 || numNorms <= 0)
        return;

    if (numVerts != numNorms)
        return;

    Vector<Point3F> tan0;
    tan0.setSize(numVerts * 2);

    Point3F* tan1 = tan0.address() + numVerts;
    dMemset(tan0.address(), 0, sizeof(Point3F) * 2 * numVerts);


    U32   numPrimatives = primitives.size();

    for (S32 i = 0; i < numPrimatives; i++)
    {
        const TSDrawPrimitive& draw = primitives[i];
        GFXPrimitiveType drawType = getDrawType(draw.matIndex >> 30);

        U32 p1Index = 0;
        U32 p2Index = 0;

        U16* baseIdx = &indices[draw.start];

        const U32 numElements = (U32)draw.numElements;

        switch (drawType)
        {
        case GFXTriangleList:
        {
            for (U32 j = 0; j < numElements; j += 3)
                findTangent(baseIdx[j], baseIdx[j + 1], baseIdx[j + 2], tan0.address(), tan1, _verts);
            break;
        }

        case GFXTriangleStrip:
        {
            p1Index = baseIdx[0];
            p2Index = baseIdx[1];
            for (U32 j = 2; j < numElements; j++)
            {
                findTangent(p1Index, p2Index, baseIdx[j], tan0.address(), tan1, _verts);
                p1Index = p2Index;
                p2Index = baseIdx[j];
            }
            break;
        }

        default:
            AssertFatal(false, "TSMesh::createTangents: unknown primitive type!");
        }
    }

    tangents.setSize(numVerts);

    // fill out final info from accumulated basis data
    for (U32 i = 0; i < numVerts; i++)
    {
        const Point3F& n = _norms[i];
        const Point3F& t = tan0[i];
        const Point3F& b = tan1[i];

        Point3F tempPt = t - n * mDot(n, t);
        tempPt.normalize();
        tangents[i] = tempPt;

        Point3F cp;
        mCross(n, t, &cp);

        tangents[i].w = (mDot(cp, b) < 0.0f) ? -1.0f : 1.0f;
    }
}