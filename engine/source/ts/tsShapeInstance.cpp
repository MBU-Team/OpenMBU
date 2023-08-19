//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "core/torqueConfig.h"

#include "ts/tsShapeInstance.h"
#include "ts/tsLastDetail.h"
#include "console/consoleTypes.h"
#include "ts/tsDecal.h"
#include "platform/profiler.h"
#include "core/frameAllocator.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxCanon.h"
#include "materials/sceneData.h"
#include "materials/matInstance.h"
#include "sceneGraph/sceneGraph.h"

TSShapeInstance::RenderData   TSShapeInstance::smRenderData;
MatrixF* TSShapeInstance::ObjectInstance::smTransforms = NULL;
S32                           TSShapeInstance::smMaxSnapshotScale = 2;
bool                          TSShapeInstance::smNoRenderTranslucent = false;
bool                          TSShapeInstance::smNoRenderNonTranslucent = false;
F32                           TSShapeInstance::smDetailAdjust = 1.0f;
F32                           TSShapeInstance::smScreenError = 5.0f;
bool                          TSShapeInstance::smFogExemptionOn = false;
S32                           TSShapeInstance::smNumSkipRenderDetails = 0;
bool                          TSShapeInstance::smSkipFirstFog = false;
bool                          TSShapeInstance::smSkipFog = false;

Vector<QuatF>                 TSShapeInstance::smNodeCurrentRotations(__FILE__, __LINE__);
Vector<Point3F>               TSShapeInstance::smNodeCurrentTranslations(__FILE__, __LINE__);
Vector<F32>                   TSShapeInstance::smNodeCurrentUniformScales(__FILE__, __LINE__);
Vector<Point3F>               TSShapeInstance::smNodeCurrentAlignedScales(__FILE__, __LINE__);
Vector<TSScale>               TSShapeInstance::smNodeCurrentArbitraryScales(__FILE__, __LINE__);

Vector<TSThread*>             TSShapeInstance::smRotationThreads(__FILE__, __LINE__);
Vector<TSThread*>             TSShapeInstance::smTranslationThreads(__FILE__, __LINE__);
Vector<TSThread*>             TSShapeInstance::smScaleThreads(__FILE__, __LINE__);

namespace {

    void tsShapeTextureEventCB(const U32 eventCode, void* userData)
    {
        TSShape* pShape = reinterpret_cast<TSShape*>(userData);

        /*
           if (eventCode == TextureManager::BeginZombification &&
              pShape->mVertexBuffer != -1)
           {
              // ideally we would de-register the callback here, but that would screw up the loop
              if (dglDoesSupportVertexBuffer())
                 glFreeVertexBufferEXT(pShape->mVertexBuffer);
              else
                 AssertFatal(false,"Vertex buffer should have already been freed!");
              pShape->mVertexBuffer = -1;
              for (S32 i = 0; i < pShape->objects.size(); ++i)
                 pShape->mPreviousMerge[i] = -1;
           }
        */
    }
}

//-------------------------------------------------------------------------------------
// constructors, destructors, initialization
//-------------------------------------------------------------------------------------

TSShapeInstance::TSShapeInstance(const Resource<TSShape>& shape, bool loadMaterials)
{
    shadowDirty = true;

    VECTOR_SET_ASSOCIATION(mMeshObjects);
    VECTOR_SET_ASSOCIATION(mDecalObjects);
    VECTOR_SET_ASSOCIATION(mIflMaterialInstances);
    VECTOR_SET_ASSOCIATION(mNodeTransforms);
    VECTOR_SET_ASSOCIATION(mNodeReferenceRotations);
    VECTOR_SET_ASSOCIATION(mNodeReferenceTranslations);
    VECTOR_SET_ASSOCIATION(mNodeReferenceUniformScales);
    VECTOR_SET_ASSOCIATION(mNodeReferenceScaleFactors);
    VECTOR_SET_ASSOCIATION(mNodeReferenceArbitraryScaleRots);
    VECTOR_SET_ASSOCIATION(mThreadList);
    VECTOR_SET_ASSOCIATION(mTransitionThreads);

    hShape = shape;
    mShape = hShape;
    buildInstanceData(mShape, loadMaterials);
}

TSShapeInstance::TSShapeInstance(TSShape* _shape, bool loadMaterials)
{
    shadowDirty = true;

    VECTOR_SET_ASSOCIATION(mMeshObjects);
    VECTOR_SET_ASSOCIATION(mDecalObjects);
    VECTOR_SET_ASSOCIATION(mIflMaterialInstances);
    VECTOR_SET_ASSOCIATION(mNodeTransforms);
    VECTOR_SET_ASSOCIATION(mNodeReferenceRotations);
    VECTOR_SET_ASSOCIATION(mNodeReferenceTranslations);
    VECTOR_SET_ASSOCIATION(mNodeReferenceUniformScales);
    VECTOR_SET_ASSOCIATION(mNodeReferenceScaleFactors);
    VECTOR_SET_ASSOCIATION(mNodeReferenceArbitraryScaleRots);
    VECTOR_SET_ASSOCIATION(mThreadList);
    VECTOR_SET_ASSOCIATION(mTransitionThreads);

    mShape = _shape;
    buildInstanceData(mShape, loadMaterials);
}

TSShapeInstance::~TSShapeInstance()
{
    S32 i;
    for (i = 0; i < mMeshObjects.size(); i++)
        destructInPlace(&mMeshObjects[i]);

    for (i = 0; i < mDecalObjects.size(); i++)
        destructInPlace(&mDecalObjects[i]);

    while (mThreadList.size())
        destroyThread(mThreadList.last());

    setMaterialList(NULL);

    delete[] mDirtyFlags;
}

void TSShapeInstance::init()
{
    /*
       smRenderData.fogTexture = false;
       smRenderData.fogBitmap = NULL;
       smRenderData.fogHandle = NULL;
       smRenderData.fogMapHandle = NULL;
    */
    smRenderData.renderDecals = true;

    Con::addVariable("$pref::TS::fogTexture", TypeBool, &smRenderData.fogTexture);
    Con::addVariable("$pref::TS::detailAdjust", TypeF32, &smDetailAdjust);
    Con::addVariable("$pref::TS::skipLoadDLs", TypeS32, &TSShape::smNumSkipLoadDetails);
    Con::addVariable("$pref::TS::skipRenderDLs", TypeS32, &smNumSkipRenderDetails);
    Con::addVariable("$pref::TS::skipFirstFog", TypeBool, &smSkipFirstFog);
    Con::addVariable("$pref::TS::screenError", TypeF32, &smScreenError);
}

void TSShapeInstance::destroy()
{
    //   delete smRenderData.fogHandle;
}

void TSShapeInstance::buildInstanceData(TSShape* _shape, bool loadMaterials)
{
    S32 i, dl;

    mShape = _shape;

    debrisRefCount = 0;

    mEnvironmentMapOn = false;
    mEnvironmentMapAlpha = 0.f;
    mAllowTwoPassEnvironmentMap = false;
    mAlphaIsReflectanceMap = true; // turn this off below if we find an exception
    mAllowTwoPassDetailMap = true;

    mMaxEnvironmentMapDL = 1;            // for shapes < version 23

    mMaxDetailMapDL = 0;

    // clear callback function and data
    mCallback = NULL;
    mCallbackData = 0;

    mCurrentDetailLevel = 0;
    mCurrentIntraDetailLevel = 1.0f;

    // all triggers off at start
    mTriggerStates = 0;

    //
    mAlphaAlways = false;
    mAlphaAlwaysValue = 1.0f;

    mBalloonShape = false;
    mBalloonValue = 1.0f;

    mUseOverrideTexture = false;

    // if never set, never draw fog -- do this here just in case
    smRenderData.fogOn = false;

    // material list...
    mMaterialList = NULL;
    mOwnMaterialList = false;

    //
    mData = 0;
    mScaleCurrentlyAnimated = false;

    if (loadMaterials)
    {
        setMaterialList(mShape->materialList);
    }

    // set up node data
    S32 numNodes = mShape->nodes.size();
    mNodeTransforms.setSize(numNodes);

    // add objects to trees
    S32 numObjects = mShape->objects.size();
    mMeshObjects.setSize(numObjects);
    for (i = 0; i < numObjects; i++)
    {
        const TSObject* obj = &mShape->objects[i];
        MeshObjectInstance* objInst = &mMeshObjects[i];

        // call objInst constructor
        constructInPlace(objInst);

        // hook up the object to it's node
        objInst->nodeIndex = obj->nodeIndex;

        // set up list of meshes
        if (obj->numMeshes)
            objInst->meshList = &mShape->meshes[obj->startMeshIndex];
        else
            objInst->meshList = NULL;

        objInst->object = obj;
    }

    // set up decal objects
    mDecalObjects.setSize(mShape->decals.size());
    for (i = 0; i < mShape->decals.size(); i++)
    {
        const TSShape::Decal* decal = &mShape->decals[i];
        DecalObjectInstance* decalInst = &mDecalObjects[i];

        // call constructor
        constructInPlace(decalInst);
        decalInst->decalObject = decal;

        // hook up to node
        decalInst->targetObject = &mMeshObjects[decal->objectIndex];
        decalInst->nodeIndex = decalInst->targetObject->nodeIndex;

        // set up list of decal meshes
        if (decal->numMeshes)
        {
            decalInst->decalList = (TSDecalMesh**)&mShape->meshes[decal->startMeshIndex];
            for (S32 j = 0; j < decal->numMeshes; j++)
                if (decalInst->getDecalMesh(j))
                {
                    // point the decal mesh at it's target...
                    // this is safe since meshes aren't shared between shapes
                    TSDecalMesh* decalMesh = const_cast<TSDecalMesh*>(decalInst->getDecalMesh(j));
                    decalMesh->targetMesh = decalInst->targetObject->getMesh(j);
                    if (!decalMesh->targetMesh)
                    {
                        // detecting this a little late, but we don't need this decal since it isn't doing anything
                        // should only happen on shapes exported before dtsexp 1.18
                        delete decalMesh;
                        TSDecalMesh** dm = const_cast<TSDecalMesh**>(decalInst->decalList + j);
                        *dm = NULL;
                    }
                }
        }
        else
            decalInst->decalList = NULL;
        decalInst->frame = mShape->decalStates[i].frameIndex;
    }

    // construct ifl material objects

    if (loadMaterials)
    {
        for (i = 0; i < mShape->iflMaterials.size(); i++)
        {
            mIflMaterialInstances.increment();
            mIflMaterialInstances.last().iflMaterial = &mShape->iflMaterials[i];
            mIflMaterialInstances.last().frame = -1;
        }
    }
    /*
       // check to see which dl's have detail texturing
       mMaxDetailMapDL = -1;
       if(loadMaterials)
       {
          for (dl=0; dl<mShape->details.size(); dl++)
          {
             // check meshes on this detail level...
             S32 ss = mShape->details[dl].subShapeNum;
             S32 od = mShape->details[dl].objectDetailNum;
             if (ss<0)
                continue; // this is a billboard detail level
             S32 start = mShape->subShapeFirstObject[ss];
             S32 end = mShape->subShapeNumObjects[ss] + start;
             for (i=start; i<end; i++)
             {
                TSMesh * mesh = mMeshObjects[i].getMesh(od);
                if (!mesh)
                   continue;
                for (S32 j=0; j<mesh->primitives.size(); j++)
                {
                   if (mesh->primitives[j].matIndex & TSDrawPrimitive::NoMaterial)
                      continue;
                   if (mMaterialList->getDetailMap(mesh->primitives[j].matIndex & TSDrawPrimitive::MaterialMask))
                   {
                      mesh->setFlags(TSMesh::HasDetailTexture);
                      if (dl>mMaxDetailMapDL)
                         mMaxDetailMapDL = dl;
                   }
                }
             }
          }
       }
    */

    // set up subtree data
    S32 ss = mShape->subShapeFirstNode.size(); // we have this many subtrees
    mDirtyFlags = new U32[ss];
    for (int i = 0; i < ss; i++)
        mDirtyFlags[i] = 0;

    mGroundThread = NULL;
    mCurrentDetailLevel = 0;

    animateSubtrees();

    // Construct billboards if not done already
    if (loadMaterials)
        ((TSShape*)mShape)->setupBillboardDetails(this);
}

// Initialize material instances in material list
void TSShapeInstance::initMatInstances()
{
    SceneGraphData sgData;
    sgData.useLightDir = true;
    sgData.useFog = SceneGraph::renderFog;
    GFXVertexPNT* tsVertex = NULL;

    for (U32 i = 0; i < mMaterialList->getMaterialCount(); i++)
    {
        MatInstance* matInst = mMaterialList->getMaterialInst(i);
        if (matInst)
        {
            matInst->init(sgData, (GFXVertexFlags)getGFXVertFlags(tsVertex));
        }
    }

}

void TSShapeInstance::setMaterialList(TSMaterialList* ml)
{
    // get rid of old list
    if (mOwnMaterialList)
        delete mMaterialList;
    mMaterialList = ml;
    mOwnMaterialList = false;

    if (mMaterialList && StringTable) // material lists need the string table to load...
    {
        // read ifl materials if necessary -- this is here rather than in shape because we can't open 2 files at once :(
        if (mShape->materialList == mMaterialList)
            ((TSShape*)mShape)->readIflMaterials(hShape.getFilePath());

        mMaterialList->load(MeshTexture, hShape.getFilePath(), true);
        mMaterialList->mapMaterials(hShape.getFilePath());

        initMatInstances();


        /*
              // check for reflectance map not in alpha of texture -- will require more work to emap
              for (U32 i=0; i<mMaterialList->getMaterialCount(); i++)
              {
                 if (mMaterialList->getFlags(i) & (TSMaterialList::AuxiliaryMap|TSMaterialList::NeverEnvMap))
                    continue;
                 if (!mMaterialList->reflectionInAlpha(i))
                 {
                    mAlphaIsReflectanceMap = false;
                    break; // found our exception
                 }
              }
        */
    }
}

void TSShapeInstance::cloneMaterialList()
{
    if (mOwnMaterialList)
        return;
    mMaterialList = new TSMaterialList(mMaterialList);
    mOwnMaterialList = true;

    initMatInstances();
}

static bool makeSkinPath(char* buffer, U32 bufferLength, const char* resourcePath,
    const char* oldSkin, const char* oldRoot, const char* newRoot)
{
    bool replacedRoot = true;

    dsize_t oldRootLen = 0;
    const char* rootStart = NULL;

    if (oldRoot == NULL) {
        // Not doing any replacing.
        replacedRoot = false;
    }
    else {
        // See if original name has the old root in it.
        oldRootLen = dStrlen(oldRoot);
        AssertFatal((oldRootLen + 1) < bufferLength, "makeSkinPath: Error, skin root name too long");
        dStrcpy(buffer, oldRoot);
        dStrcat(buffer, ".");
        rootStart = dStrstr(oldSkin, buffer);
        if (rootStart == NULL) {
            replacedRoot = false;
        }
    }

    // Find out how long the total pathname will be.
    const dsize_t oldLen = dStrlen(oldSkin);
    dsize_t pathLen = 0;
    if (resourcePath != NULL) {
        pathLen = dStrlen(resourcePath);
    }
    if (replacedRoot) {
        const dsize_t newRootLen = dStrlen(newRoot);
        AssertFatal((pathLen + 1 + oldLen + newRootLen - oldRootLen) < bufferLength, "makeSkinPath: Error, pathname too long");
    }
    else {
        AssertFatal((pathLen + 1 + oldLen) < bufferLength, "makeSkinPath: Error, pathname too long");
    }

    // OK, now make the pathname.

    // Start with the resource path:
    if (resourcePath != NULL) {
        dStrcpy(buffer, resourcePath);
        dStrcat(buffer, "/");
    }
    else {
        buffer[0] = '\0';
    }
    if (replacedRoot) {
        // Then the pre-root part of the old name:
        dsize_t rootStartPos = rootStart - oldSkin;
        if (rootStartPos != 0) {
            dStrncat(buffer, oldSkin, rootStartPos);
        }
        // Then the new root:
        dStrcat(buffer, newRoot);
        dStrcat(buffer, ".");
        // Then the post-root part of the old name:
        dStrcat(buffer, oldSkin + rootStartPos + oldRootLen + 1);
    }
    else {
        // Then the old name:
        dStrcat(buffer, oldSkin);
    }

    return replacedRoot;
}


void TSShapeInstance::reSkin(StringHandle& newBaseHandle)
{
#define NAME_BUFFER_LENGTH 256
    static char pathName[NAME_BUFFER_LENGTH];
    const char* defaultBaseName = "base";
    const char* newBaseName;

    if (newBaseHandle.isValidString()) {
        newBaseName = newBaseHandle.getString();
        if (newBaseName == NULL) {
            return;
        }
    }
    else {
        newBaseName = defaultBaseName;
    }

    // Make our own copy of the materials list from the resource
    // if necessary.
    if (ownMaterialList() == false) {
        cloneMaterialList();
    }

    const char* resourcePath = hShape.getFilePath();

    // Cycle through the materials.
    TSMaterialList* pMatList = getMaterialList();
    for (S32 j = 0; j < pMatList->mMaterialNames.size(); j++) {
        // Get the name of this material.
        const char* pName = pMatList->mMaterialNames[j];
        // Bail if no name.
        if (pName == NULL) {
            continue;
        }
        // Make a texture file pathname with the new root if this name
        // has the old root in it; otherwise just make a path with the
        // original name.
        bool replacedRoot = makeSkinPath(pathName, NAME_BUFFER_LENGTH, resourcePath,
            pName, defaultBaseName, newBaseName);

        if (!replacedRoot)

        {
            // If this wasn't in the desired format, set the material's
            // texture handle (since that wasn't copied over in the
            // cloning) and continue.
            pMatList->setMaterial(j, pathName);
            continue;
        }

        // OK, it is a skin texture.  Get the handle.
        // Do a sanity check; if it fails, use the original skin instead.
        if (!pMatList->setMaterial(j, pathName))
        {
            makeSkinPath(pathName, NAME_BUFFER_LENGTH, resourcePath, pName, NULL, NULL);
            pMatList->setMaterial(j, pathName);
        }
    }
}

//-------------------------------------------------------------------------------------
// Render & detail selection
//-------------------------------------------------------------------------------------
void TSShapeInstance::render(const Point3F* objectScale)
{
    if (mCurrentDetailLevel < 0)
        return;
    PROFILE_START(TSShapeInstanceRender);
    //   dglSetRenderPrimType(3);

       // alphaIn:  we start to alpha-in next detail level when intraDL > 1-alphaIn-alphaOut
       //           (finishing when intraDL = 1-alphaOut)
       // alphaOut: start to alpha-out this detail level when intraDL > 1-alphaOut
       // NOTE:
       //   intraDL is at 1 when if shape were any closer to us we'd be at dl-1,
       //   intraDL is at 0 when if shape were any farther away we'd be at dl+1
    F32 alphaOut = mShape->alphaOut[mCurrentDetailLevel];
    F32 alphaIn = mShape->alphaIn[mCurrentDetailLevel];
    F32 saveAA = mAlphaAlways ? mAlphaAlwaysValue : 1.0f;

    if (mCurrentIntraDetailLevel > alphaIn + alphaOut)
        render(mCurrentDetailLevel, mCurrentIntraDetailLevel, objectScale);
    else if (mCurrentIntraDetailLevel > alphaOut)
    {
        // draw this detail level w/ alpha=1 and next detail level w/
        // alpha=1-(intraDl-alphaOut)/alphaIn

        // first draw next detail level
        if (mCurrentDetailLevel + 1 < mShape->details.size() && mShape->details[mCurrentDetailLevel + 1].size > 0.0f)
        {
            setAlphaAlways(saveAA * (alphaIn + alphaOut - mCurrentIntraDetailLevel) / alphaIn);
            render(mCurrentDetailLevel + 1, 0.0f, objectScale);
        }

        setAlphaAlways(saveAA);
        render(mCurrentDetailLevel, mCurrentIntraDetailLevel, objectScale);
    }
    else
    {
        // draw next detail level w/ alpha=1 and this detail level w/
        // alpha = 1-intraDL/alphaOut

        // first draw next detail level
        if (mCurrentDetailLevel + 1 < mShape->details.size() && mShape->details[mCurrentDetailLevel + 1].size > 0.0f)
            render(mCurrentDetailLevel + 1, 0.0f, objectScale);

        setAlphaAlways(saveAA * mCurrentIntraDetailLevel / alphaOut);
        render(mCurrentDetailLevel, mCurrentIntraDetailLevel, objectScale);
        setAlphaAlways(saveAA);
    }
    //   dglSetRenderPrimType(0);
    PROFILE_END();
}

bool TSShapeInstance::hasTranslucency()
{
    if (!mShape->details.size())
        return false;

    const TSDetail* detail = &mShape->details[0];
    S32 ss = detail->subShapeNum;

    return mShape->subShapeFirstTranslucentObject[ss] != mShape->subShapeFirstObject[ss] + mShape->subShapeNumObjects[ss];
}

bool TSShapeInstance::hasSolid()
{
    if (!mShape->details.size())
        return false;

    const TSDetail* detail = &mShape->details[0];
    S32 ss = detail->subShapeNum;

    return mShape->subShapeFirstTranslucentObject[ss] != mShape->subShapeFirstObject[ss];
}

void TSShapeInstance::render(S32 dl, F32 intraDL, const Point3F* objectScale)
{
    // if dl==-1, nothing to do
    if (dl == -1)
        return;

    AssertFatal(dl >= 0 && dl < mShape->details.size(), "TSShapeInstance::render");

    S32 i;

    const TSDetail* detail = &mShape->details[dl];
    S32 ss = detail->subShapeNum;
    S32 od = detail->objectDetailNum;

    // set up static data
    setStatics(dl, intraDL, objectScale);

    // if we're a billboard detail, draw it and exit
    PROFILE_START(TSShapeInstanceRenderBillboards);
    if (ss < 0)
    {
        if (!smNoRenderTranslucent)
            mShape->billboardDetails[dl]->render(mAlphaAlways ? mAlphaAlwaysValue : 1.0f, smRenderData.fogOn);
        PROFILE_END();
        return;
    }
    PROFILE_END();

    PROFILE_START(TSShapeInstanceMaterials);
    // set up animating ifl materials
    for (i = 0; i < mIflMaterialInstances.size(); i++)
    {
        IflMaterialInstance* iflMaterialInstance = &mIflMaterialInstances[i];
        const TSShape::IflMaterial* iflMaterial = iflMaterialInstance->iflMaterial;
        mMaterialList->remap(iflMaterial->materialSlot, iflMaterial->firstFrame + iflMaterialInstance->frame);
    }

    // decide how to use gl resources
    setupTexturing(dl, intraDL);

    // set up gl environment for drawing mesh materials
    TSMesh::initMaterials();
    PROFILE_END();

    S32 start;
    S32 end;

    bool supportBuffers = false; //dglDoesSupportVertexBuffer();

    if (!supportBuffers || !renderMeshesX(ss, od))
    {
        // run through the meshes
        smRenderData.currentTransform = NULL;
        S32 start = smNoRenderNonTranslucent ? mShape->subShapeFirstTranslucentObject[ss] : mShape->subShapeFirstObject[ss];
        S32 end = smNoRenderTranslucent ? mShape->subShapeFirstTranslucentObject[ss] : mShape->subShapeFirstObject[ss] + mShape->subShapeNumObjects[ss];
        for (i = start; i < end; i++)
        {
            smRenderData.currentObjectInstance = &mMeshObjects[i];
            // following line is handy for debugging, to see what part of the shape that it is rendering
            //const char *name = mShape->names[mMeshObjects[i].object->nameIndex];
            mMeshObjects[i].render(od, mMaterialList);
        }
    }

    // if we have a matrix pushed, pop it now
    if (smRenderData.currentTransform)
        GFX->popWorldMatrix(); //glPopMatrix();

     // restore gl state
    TSMesh::resetMaterials();

    // render detail maps using a second pass?
    if (twoPassDetailMap())
        renderDetailMap();

    // render environment map using second pass?
    if (twoPassEnvironmentMap())
        renderEnvironmentMap();

    if (!supportBuffers || !renderDecalsX(ss, od))
    {
        start = mShape->subShapeFirstDecal[ss];
        end = mShape->subShapeNumDecals[ss] + start;
        if (smRenderData.renderDecals && !smNoRenderTranslucent && start < end)
        {
            // set up gl environment for decals...
            TSDecalMesh::initDecalMaterials();

            // render decals...
            smRenderData.currentTransform = NULL;
            for (i = start; i < end; i++)
                mDecalObjects[i].render(od, mMaterialList);

            // if we have a matrix pushed, pop it now
            if (smRenderData.currentTransform)
                GFX->popWorldMatrix(); // glPopMatrix();

             // restore gl state
            TSDecalMesh::resetDecalMaterials();
        }
    }

    // render fog if 2-passing it
    if (twoPassFog())
        renderFog();

    // restore gl state
    TSMesh::resetMaterials();

    clearStatics();
}

bool TSShapeInstance::renderMeshesX(S32 ss, S32 od)
{
    /*
       // TODO: find out why this case doesn't work
       if (smRenderData.vertexAlpha.current < 1.0)
          return false;
       PROFILE_START(TSShapeInstanceMeshes);

       S32 i,start,end,vb;

       if ((vb = mShape->mVertexBuffer) == -1)
       {
          // find out before we calc the needed buffer size if there are any free
          if (!glAvailableVertexBufferEXT())
          {
             PROFILE_END();
             return false;
          }

          GLsizei size = 0;

          start = mShape->subShapeFirstObject[0];
          end = mShape->subShapeFirstObject[0] + mShape->subShapeNumObjects[0];
          for (i = start; i < end; ++i)
             size += mMeshObjects[i].getSizeVB(size);

          mShape->mMorphable = false;
          for (i = start; i < end; ++i)
             if (mMeshObjects[i].hasMergeIndices())
             {
                mShape->mMorphable = true;
                break;
             }

          vb = mShape->mVertexBuffer = glAllocateVertexBufferEXT(size,GL_V12MTNVFMT_EXT,true);
          if (vb == -1)
          {
             PROFILE_END();
             return false;
          }
          if (mShape->mCallbackKey == -1)
             mShape->mCallbackKey = TextureManager::registerEventCallback(tsShapeTextureEventCB, mShape);

          // run through the meshes -- fill vertex buffer
          glLockVertexBufferEXT(vb,0);
          for (i = start; i < end; ++i)
             mMeshObjects[i].fillVB(vb,mMaterialList);
          glUnlockVertexBufferEXT(vb);
       }

       // run through the meshes
       start = smNoRenderNonTranslucent ? mShape->subShapeFirstTranslucentObject[ss] : mShape->subShapeFirstObject[ss];
       end   = smNoRenderTranslucent ? mShape->subShapeFirstTranslucentObject[ss] : mShape->subShapeFirstObject[ss] + mShape->subShapeNumObjects[ss];

       if (mShape->mMorphable)
       {
          PROFILE_START(TSShapeInstanceMorphVB);
          glLockVertexBufferEXT(vb,0);
          for (i = start; i < end; ++i)
             mMeshObjects[i].morphVB(vb,mShape->mPreviousMerge[i],od,mMaterialList);
          glUnlockVertexBufferEXT(vb);
          PROFILE_END();
       }

       smRenderData.currentTransform = NULL;
       PROFILE_START(TSShapeInstanceRenderVB);
       for (i = start; i < end; ++i)
          mMeshObjects[i].renderVB(vb,od,mMaterialList);
       PROFILE_END();

       PROFILE_END();
    */
    return true;
}

bool TSShapeInstance::renderDecalsX(S32 ss, S32 od)
{
    return false;

    ss, od;
    // I don't know why, but this doesn't quite work -- no time to fix
#if 0
    if (supportBuffers)
    {
        S32 i, start, end, vb;
        vb = mShape->mVertexBuffer;

        start = mShape->subShapeFirstDecal[ss];
        end = mShape->subShapeNumDecals[ss] + start;
        if (smRenderData.renderDecals && start < end)
        {
            // set up gl environment for decals...
            TSDecalMesh::initDecalMaterials();

            // render decals...
            smRenderData.currentTransform = NULL;
            for (i = start; i < end; i++)
            {
                TSDecalMesh* decal0 = mDecalObjects[i].getDecalMesh(0);

                if (!decal0)
                    continue;

                TSMesh* target0 = decal0->targetMesh;
                TSDecalMesh* decal;


                if (!target0 ||
                    mDecalObjects[i].targetObject->visible <= 0.01f ||
                    !(decal = mDecalObjects[i].getDecalMesh(od)) ||
                    mDecalObjects[i].frame < 0 ||
                    !decal->targetMesh ||
                    decal->texgenS.empty() ||
                    decal->texgenT.empty())
                    continue;

                GLuint foffset = mDecalObjects[i].frame * target0->numMatFrames * target0->vertsPerFrame;

                glSetVertexBufferEXT(vb);
                glOffsetVertexBufferEXT(vb, target0->vbOffset + foffset);
                mDecalObjects[i].render(od, mMaterialList);
            }

            // if we have a matrix pushed, pop it now
            if (smRenderData.currentTransform)
                glPopMatrix();

            // restore gl state
            TSDecalMesh::resetDecalMaterials();
        }
    }
    else
#endif
}

void TSShapeInstance::setStatics(S32 dl, F32 intraDL, const Point3F* objectScale)
{
    ObjectInstance::smTransforms = mNodeTransforms.address();
    smRenderData.objectScale = objectScale;
    smRenderData.detailLevel = dl;
    smRenderData.intraDetailLevel = intraDL;
    smRenderData.alwaysAlpha = mAlphaAlways;
    smRenderData.alwaysAlphaValue = getAlphaAlwaysValue();
    smRenderData.balloonShape = mBalloonShape;
    smRenderData.balloonValue = getBalloonValue();

    smRenderData.useOverride = mUseOverrideTexture;
    //   smRenderData.override    = mOverrideTexture;

    smRenderData.currentObjectInstance = NULL;

    S32 ss = mShape->details[dl].subShapeNum;
    S32 od = mShape->details[dl].objectDetailNum;
    TSMesh::smSaveVerts.setSize(mShape->mMergeBufferSize);
    TSMesh::smSaveTVerts.setSize(mShape->mMergeBufferSize);

    // If we have a billboard, skip the rest
    if (ss < 0)
        return;

    S32 start = smNoRenderNonTranslucent ? mShape->subShapeFirstTranslucentObject[ss] : mShape->subShapeFirstObject[ss];
    S32 end = smNoRenderTranslucent ? mShape->subShapeFirstTranslucentObject[ss] : mShape->subShapeFirstObject[ss] + mShape->subShapeNumObjects[ss];

    for (S32 i = start; i < end; i++)
    {
        TSMesh* mesh = mMeshObjects[i].getMesh(od);
        if (mesh)
            mesh->saveMergeVerts();
    }
}

void TSShapeInstance::clearStatics()
{
    ObjectInstance::smTransforms = NULL;
    //   smRenderData.override = NULL;

    smRenderData.currentObjectInstance = NULL;

    S32 ss = mShape->details[smRenderData.detailLevel].subShapeNum;
    S32 od = mShape->details[smRenderData.detailLevel].objectDetailNum;
    S32 start = smNoRenderNonTranslucent ? mShape->subShapeFirstTranslucentObject[ss] : mShape->subShapeFirstObject[ss];
    S32 end = smNoRenderTranslucent ? mShape->subShapeFirstTranslucentObject[ss] : mShape->subShapeFirstObject[ss] + mShape->subShapeNumObjects[ss];
    for (S32 i = start; i < end; i++)
    {
        TSMesh* mesh = mMeshObjects[i].getMesh(od);
        if (mesh)
            mesh->restoreMergeVerts();
    }
}

void TSShapeInstance::setupTexturing(S32 dl, F32 intraDL)
{
    /*
       // first we'll decide what maps we want
       // then we'll decide how we can implement them (1-pass or 2-pass or not at all)

       // we need to set up these variables
       S32 & emapMethod = smRenderData.environmentMapMethod;
       S32 & dmapMethod = smRenderData.detailMapMethod;
       S32 & fogMethod  = smRenderData.fogMethod;
       S32 & dmapTE     = smRenderData.detailMapTE;
       S32 & emapTE     = smRenderData.environmentMapTE;
       S32 & baseTE     = smRenderData.baseTE;
       S32 & fogTE      = smRenderData.fogTE;

       baseTE = 0; // initially assume base texture will go in first TE

       // -------------------------------------------------
       // what do we want to do?

       bool wantEMap = ((mShape->mExportMerge && dl<=mShape->mSmallestVisibleDL/2) ||
                        (!mShape->mExportMerge && dl<=mMaxEnvironmentMapDL)) &&
                       mEnvironmentMapOn && (TextureObject*)mEnvironmentMap && mEnvironmentMapAlpha>0.01f;
       bool wantDMap = dl<=mMaxDetailMapDL;
       bool wantFog  = smRenderData.fogOn && !smSkipFog;
       smRenderData.detailMapAlpha = (dl<mMaxDetailMapDL || intraDL>0.5f) ? 1.0f : 2.0f * intraDL;
       smRenderData.environmentMapAlpha = mEnvironmentMapAlpha *
                                          (
                                            (((mShape->mExportMerge && dl<mShape->mSmallestVisibleDL/2) ||
                                                (!mShape->mExportMerge && dl<mMaxEnvironmentMapDL)) ||
                                               intraDL>0.5f) ? 1.0f : 2.0f * intraDL );
       smRenderData.environmentMapGLName = wantEMap ? mEnvironmentMap.getGLName() : 0;

       // -------------------------------------------------
       // what can we do?

       if (!dglDoesSupportARBMultitexture())
       {
          // we don't support multitexturing -- early out
          emapMethod = NO_ENVIRONMENT_MAP;
          dmapMethod = (wantDMap && mAllowTwoPassDetailMap) ? DETAIL_MAP_TWO_PASS : NO_DETAIL_MAP;
          fogMethod  = wantFog ? FOG_TWO_PASS : NO_FOG;

          return;
       }

       // how many texture environments (TE's) do we have?
       GLint numTE = 1, numUsedTE = 1;
       if (dglDoesSupportARBMultitexture())
          glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB,&numTE);

       // what we do with the TE's will depend on whether the following extension is supported
       if (dglDoesSupportTextureEnvCombine())
       {
          // environment map...
          if (wantEMap)
          {
             if (mAlphaIsReflectanceMap)
             {
                emapMethod = ENVIRONMENT_MAP_MULTI_1;
                emapTE = numUsedTE;
                numUsedTE++;
             }
             else if (numUsedTE+3<=numTE)
             {
                emapMethod = ENVIRONMENT_MAP_MULTI_3;
                emapTE = numUsedTE;
                numUsedTE += 3;
             }
             else if (mAllowTwoPassEnvironmentMap)
                emapMethod = ENVIRONMENT_MAP_TWO_PASS;
             else
                emapMethod = NO_ENVIRONMENT_MAP;
          }
          else
             emapMethod = NO_ENVIRONMENT_MAP;

          // detail map...
          if (wantDMap)
          {
             if (smRenderData.detailMapAlpha>0.99f && numTE>=numUsedTE+1)
             {
                dmapMethod = DETAIL_MAP_MULTI_1;
                dmapTE = numUsedTE;
                numUsedTE++;
             }
             else if (smRenderData.detailMapAlpha<=0.9f && numTE>=numUsedTE+2)
             {
                dmapMethod = DETAIL_MAP_MULTI_2;
                dmapTE = 0;     // detail texture goes in first unit...
                baseTE++;       // so we bump this back one...
                emapTE++;       // this one gets bumped back 2...
                numUsedTE += 2; // end up using two additional units..
             }
             else
                dmapMethod = mAllowTwoPassDetailMap ? DETAIL_MAP_TWO_PASS : NO_DETAIL_MAP;
          }
          else
             dmapMethod = NO_DETAIL_MAP;

          // fog...
          if (wantFog)
          {
             // DMMUNDO!
             if (numTE>=numUsedTE+1 && emapMethod!=ENVIRONMENT_MAP_TWO_PASS)
             {
                fogMethod = smRenderData.fogMapHandle ? FOG_MULTI_1_TEXGEN : FOG_MULTI_1;
                fogTE = numUsedTE;
                numUsedTE++;
             }
             else
                fogMethod = smRenderData.fogMapHandle ? FOG_TWO_PASS_TEXGEN : FOG_TWO_PASS;
          }
          else
             fogMethod = NO_FOG;
       }
       else
       {
          // we can't single pass environment map without texture combine extension...
          wantEMap = wantEMap && mAllowTwoPassEnvironmentMap;
          emapMethod = wantEMap ? ENVIRONMENT_MAP_TWO_PASS : NO_ENVIRONMENT_MAP;
          // ditto for detail map...
          wantDMap = wantDMap && mAllowTwoPassDetailMap;
          dmapMethod = wantDMap ? DETAIL_MAP_TWO_PASS : NO_DETAIL_MAP;
          fogMethod = wantFog ? FOG_TWO_PASS : NO_FOG;
       }

       if (emapMethod == NO_ENVIRONMENT_MAP)
          smRenderData.environmentMapAlpha = 1.0f;
    */
}

void TSShapeInstance::setupFog(F32 fogAmount, const ColorF& fogColor)
{
    /*
       smRenderData.fogOn = (fogAmount > 1.0 / 64.0f);
       smRenderData.fogMapHandle = NULL;

       bool refresh = false;

       if (!smRenderData.fogBitmap)
       {
          smRenderData.fogBitmap = new GBitmap(8,8,false,GBitmap::RGBA);

          // clear the bitmap (defaults to 0xff) so if fogColor is 0,0,0
          // we will have a valid bitmap
          dMemset(smRenderData.fogBitmap->getWritableBits(), 0, 256);
       }

       if (smRenderData.fogColor.x != fogColor.red ||
           smRenderData.fogColor.y != fogColor.green ||
           smRenderData.fogColor.z != fogColor.blue)
       {
          U8 *bits = smRenderData.fogBitmap->getWritableBits();
          U8 red = U8(255*fogColor.red);
          U8 green = U8(255*fogColor.green);
          U8 blue = U8(255*fogColor.blue);

          for (U8 i = 0; i < 64; ++i)
          {
             *bits++ = red;
             *bits++ = green;
             *bits++ = blue;
             bits++;
          }
          refresh = true;
       }

       // the ATI Rage 128 needs a forthcoming driver to do do constant alpha blend
       if (smRenderData.fogTexture)
       {
          if (smRenderData.fogColor.w != fogAmount)
          {
             U8 *bits = smRenderData.fogBitmap->getWritableBits();
             U8 fog = U8(255 * fogAmount);

             for (U8 i = 0; i < 64; ++i)
             {
                bits[3] = fog;
                bits += 4;
             }
             refresh = true;
          }
       }

       if (!smRenderData.fogHandle)
          smRenderData.fogHandle = new TextureHandle("fog_texture", smRenderData.fogBitmap);
       else
          if (refresh)
             smRenderData.fogHandle->refresh();

       smRenderData.fogColor.set(fogColor.red,fogColor.green,fogColor.blue,fogAmount);
    */
}

/*
void TSShapeInstance::setupFog(F32 fogAmount, TextureHandle * fogMap, Point4F & s, Point4F & t)
{
   smRenderData.fogColor.w = fogAmount;
   smRenderData.fogOn = true;
   smRenderData.fogMapHandle = fogMap;
   smRenderData.fogTexGenS = s;
   smRenderData.fogTexGenT = t;
}
*/

bool TSShapeInstance::twoPassEnvironmentMap()
{
    return (smRenderData.environmentMapMethod == ENVIRONMENT_MAP_TWO_PASS);
}

bool TSShapeInstance::twoPassDetailMap()
{
    return (smRenderData.detailMapMethod == DETAIL_MAP_TWO_PASS);
}

bool TSShapeInstance::twoPassFog()
{
    return (smRenderData.fogMethod == FOG_TWO_PASS || smRenderData.fogMethod == FOG_TWO_PASS_TEXGEN);
}

void TSShapeInstance::renderEnvironmentMap()
{
    /*
       AssertFatal((void *)mEnvironmentMap!=NULL,"TSShapeInstance::renderEnvironmentMap (1)");
       AssertFatal(mEnvironmentMapOn,"TSShapeInstance::renderEnvironmentMap (2)");
       AssertFatal(dglDoesSupportARBMultitexture(),"TSShapeInstance::renderEnvironmentMap (3)");
       AssertFatal(smRenderData.environmentMapMethod==ENVIRONMENT_MAP_TWO_PASS,"TSShapeInstance::renderEnvironmentMap (4)");

       S32 dl = smRenderData.detailLevel;
       const TSDetail * detail = &mShape->details[dl];
       S32 ss = detail->subShapeNum;
       S32 od = detail->objectDetailNum;

       S32 start = mShape->subShapeFirstObject[ss];
       S32 end   = mShape->subShapeNumObjects[ss] + start;
       if (start>=end)
          return;

       // set up gl environment for emap...
       TSMesh::initEnvironmentMapMaterials();

       // run through objects and render
       smRenderData.currentTransform = NULL;
       for (S32 i=start; i<end; i++)
          mMeshObjects[i].renderEnvironmentMap(od,mMaterialList);

       // if we have a matrix pushed, pop it now
       if (smRenderData.currentTransform)
          glPopMatrix();

       // restore gl state
       TSMesh::resetEnvironmentMapMaterials();
    */
}

void TSShapeInstance::renderFog()
{
    /*
       AssertFatal(smRenderData.fogMethod==FOG_TWO_PASS || smRenderData.fogMethod==FOG_TWO_PASS_TEXGEN,"TSShapeInstance::renderFog");

       S32 dl = smRenderData.detailLevel;
       const TSDetail * detail = &mShape->details[dl];
       S32 ss = detail->subShapeNum;
       S32 od = detail->objectDetailNum;

       GLboolean wasLit = glIsEnabled(GL_LIGHTING);
       glDisable(GL_LIGHTING);
       glEnable(GL_BLEND);
       glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
       glEnableClientState(GL_VERTEX_ARRAY);

       if (smRenderData.fogMethod==FOG_TWO_PASS_TEXGEN)
       {
          // set up fog map
          glEnable(GL_TEXTURE_2D);
          glBindTexture(GL_TEXTURE_2D, TSShapeInstance::smRenderData.fogMapHandle->getGLName());

          // set up texgen equations
          glTexGeni(GL_S,GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
          glTexGeni(GL_T,GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
          glEnable(GL_TEXTURE_GEN_S);
          glEnable(GL_TEXTURE_GEN_T);
          glTexGenfv(GL_S,GL_OBJECT_PLANE,&TSShapeInstance::smRenderData.fogTexGenS.x);
          glTexGenfv(GL_T,GL_OBJECT_PLANE,&TSShapeInstance::smRenderData.fogTexGenT.x);
       }
       else
       {
          // just one fog color per shape...
          glColor4fv(smRenderData.fogColor);
          // texture should be disabled already...
       }

       smRenderData.currentTransform = NULL;
       S32 start = smNoRenderNonTranslucent ? mShape->subShapeFirstTranslucentObject[ss] : mShape->subShapeFirstObject[ss];
       S32 end   = smNoRenderTranslucent ? mShape->subShapeFirstTranslucentObject[ss] : mShape->subShapeFirstObject[ss] + mShape->subShapeNumObjects[ss];
       for (S32 i=start; i<end; i++)
          mMeshObjects[i].renderFog(od, mMaterialList);

       // if we have a marix pushed, pop it now
       if (smRenderData.currentTransform)
          glPopMatrix();

       // reset gl state
       glDisable(GL_BLEND);
       glDisable(GL_TEXTURE_GEN_S);
       glDisable(GL_TEXTURE_GEN_T);
       glDisable(GL_TEXTURE_2D);
       glBlendFunc(GL_ONE, GL_ZERO);
       glDisableClientState(GL_VERTEX_ARRAY);
       if (wasLit)
          glEnable(GL_LIGHTING);
    */
}

void TSShapeInstance::renderDetailMap()
{
    /*
       AssertFatal(smRenderData.detailMapMethod==DETAIL_MAP_TWO_PASS,"TSShapeInstance::renderDetailMap (1)");

       S32 dl = smRenderData.detailLevel;
       const TSDetail * detail = &mShape->details[dl];
       S32 ss = detail->subShapeNum;
       S32 od = detail->objectDetailNum;

       S32 start = mShape->subShapeFirstObject[ss];
       S32 end   = mShape->subShapeNumObjects[ss] + start;

       // set up gl environment for the detail map
       TSMesh::initDetailMapMaterials();

       // run through objects and render detail maps
       smRenderData.currentTransform = NULL;
       for (S32 i=start; i<end; i++)
          mMeshObjects[i].renderDetailMap(od,mMaterialList);

       // if we have a matrix pushed, pop it now
       if (smRenderData.currentTransform)
          glPopMatrix();

       // restore gl state
       TSMesh::resetDetailMapMaterials();
    */
}

S32 TSShapeInstance::getCurrentDetail()
{
    return mCurrentDetailLevel;
}

F32 TSShapeInstance::getCurrentIntraDetail()
{
    return mCurrentIntraDetailLevel;
}

void TSShapeInstance::setCurrentDetail(S32 dl, F32 intraDL)
{
    mCurrentDetailLevel = dl;
    mCurrentIntraDetailLevel = intraDL > 1.0f ? 1.0f : (intraDL < 0.0f ? 0.0f : intraDL);

    // restrict chosen detail level by cutoff value
    S32 cutoff = getMin(smNumSkipRenderDetails, mShape->mSmallestVisibleDL);
    if (mCurrentDetailLevel >= 0 && mCurrentDetailLevel < cutoff)
    {
        mCurrentDetailLevel = cutoff;
        mCurrentIntraDetailLevel = 1.0f;
    }
}

S32 TSShapeInstance::selectCurrentDetail(bool ignoreScale)
{
    if (mShape->mSmallestVisibleDL >= 0 && mShape->details[0].maxError >= 0)
        // use new scheme
        return selectCurrentDetailEx(ignoreScale);

    MatrixF toCam = GFX->getWorldMatrix();
    Point3F p;
    toCam.mulP(mShape->center, &p);
    F32 dist = mDot(p, p);
    F32 scale = 1.0f;

    if (!ignoreScale)
    {
        // any scale?
        Point3F x, y, z;
        toCam.getRow(0, &x);
        toCam.getRow(1, &y);
        toCam.getRow(2, &z);
        F32 scalex = mDot(x, x);
        F32 scaley = mDot(y, y);
        F32 scalez = mDot(z, z);
        scale = scalex;
        if (scaley > scale)
            scale = scaley;
        if (scalez > scale)
            scale = scalez;
    }

    dist /= scale;
    dist = mSqrt(dist);

    RectI viewport = GFX->getViewport();
    F32 pixelScale = viewport.extent.x * 1.6f / 640.0f;
    F32 pixelRadius = GFX->projectRadius(dist, mShape->radius) * pixelScale * smDetailAdjust;

    return selectCurrentDetail(pixelRadius);
}

S32 TSShapeInstance::selectCurrentDetailEx(bool ignoreScale)
{
    Point3F p;
    MatrixF toCam = GFX->getWorldMatrix();
    toCam.mulP(mShape->center, &p);
    F32 dist = mDot(p, p);
    F32 scale = 1.0f;
    if (!ignoreScale)
    {
        // any scale?
        Point3F x, y, z;
        toCam.getRow(0, &x);
        toCam.getRow(1, &y);
        toCam.getRow(2, &z);
        F32 scalex = mDot(x, x);
        F32 scaley = mDot(y, y);
        F32 scalez = mDot(z, z);
        scale = scalex;
        if (scaley > scale)
            scale = scaley;
        if (scalez > scale)
            scale = scalez;
    }
    dist /= scale;
    dist = mSqrt(dist);

    // find tolerance
    RectI viewport = GFX->getViewport();
    F32 pixelScale = viewport.extent.x * 1.6f / 640.0f;
    F32 proj = GFX->projectRadius(dist, 1.0f) * pixelScale; // pixel size of 1 meter at given distance
    if (smFogExemptionOn)
        return selectCurrentDetailEx(F32(0.001) / proj);
    else
        return selectCurrentDetailEx(smScreenError / proj);
}

S32 TSShapeInstance::selectCurrentDetail(Point3F offset, F32 invScale)
{
    F32 dist = mSqrt(mDot(offset, offset));
    dist *= invScale;
    return selectCurrentDetail2(dist);
}

S32 TSShapeInstance::selectCurrentDetail2(F32 adjustedDist)
{
    if (mShape->mSmallestVisibleDL >= 0 && mShape->details[0].maxError >= 0)
        // use new scheme
        return selectCurrentDetail2Ex(adjustedDist);

    RectI viewport = GFX->getViewport();
    F32 pixelScale = viewport.extent.x * 1.6f / 640.0f;
    F32 pixelRadius = GFX->projectRadius(adjustedDist, mShape->radius) * pixelScale;
    F32 adjustedPR = pixelRadius * smDetailAdjust;
    if (adjustedPR <= mShape->mSmallestVisibleSize)
        adjustedPR = mShape->mSmallestVisibleSize + 0.01f;

    return selectCurrentDetail(adjustedPR);
}

S32 TSShapeInstance::selectCurrentDetail2Ex(F32 adjustedDist)
{
    // find tolerance
    RectI viewport = GFX->getViewport();
    F32 pixelScale = viewport.extent.x * 1.6f / 640.0f;
    F32 proj = GFX->projectRadius(adjustedDist, 1.0f) * pixelScale; // pixel size of 1 meter at given distance
    if (smFogExemptionOn)
        return selectCurrentDetailEx(F32(0.001) / proj);
    else
        return selectCurrentDetailEx(smScreenError / proj);
}

S32 TSShapeInstance::selectCurrentDetail(F32 size)
{
    // check to see if not visible first...
    if (size <= mShape->mSmallestVisibleSize)
    {
        // don't render...
        mCurrentDetailLevel = -1;
        mCurrentIntraDetailLevel = 0.0f;
        return -1;
    }

    // same detail level as last time?
    // only search for detail level if the current one isn't the right one already
    if (mCurrentDetailLevel < 0 ||
        (mCurrentDetailLevel == 0 && size <= mShape->details[0].size) ||
        (mCurrentDetailLevel > 0 && (size <= mShape->details[mCurrentDetailLevel].size || size > mShape->details[mCurrentDetailLevel - 1].size)))
    {
        // scan shape for highest detail size smaller than us...
        // shapes details are sorted from largest to smallest...
        // a detail of size <= 0 means it isn't a renderable detail level (utility detail)
        for (S32 i = 0; i < mShape->details.size(); i++)
        {
            if (size > mShape->details[i].size)
            {
                mCurrentDetailLevel = i;
                break;
            }
            if (i + 1 >= mShape->details.size() || mShape->details[i + 1].size < 0)
            {
                // We've run out of details and haven't found anything?
                // Let's just grab this one.
                mCurrentDetailLevel = i;
                break;
            }
        }
    }

    F32 curSize = mShape->details[mCurrentDetailLevel].size;
    F32 nextSize = mCurrentDetailLevel == 0 ? 2.0f * curSize : mShape->details[mCurrentDetailLevel - 1].size;
    mCurrentIntraDetailLevel = nextSize - curSize > 0.01f ? (size - curSize) / (nextSize - curSize) : 1.0f;
    mCurrentIntraDetailLevel = mCurrentIntraDetailLevel > 1.0f ? 1.0f : (mCurrentIntraDetailLevel < 0.0f ? 0.0f : mCurrentIntraDetailLevel);

    // now restrict chosen detail level by cutoff value
    S32 cutoff = getMin(smNumSkipRenderDetails, mShape->mSmallestVisibleDL);
    if (mCurrentDetailLevel >= 0 && mCurrentDetailLevel < cutoff)
    {
        mCurrentDetailLevel = cutoff;
        mCurrentIntraDetailLevel = 1.0f;
    }

    return mCurrentDetailLevel;
}

S32 TSShapeInstance::selectCurrentDetailEx(F32 errorTOL)
{
    // note:  we use 10 time the average error as the metric...this is
    // more robust than the maxError...the factor of 10 is to put average error
    // on about the same scale as maxError.  The errorTOL is how much
    // error we are able to tolerate before going to a more detailed version of the
    // shape.  We look for a pair of details with errors bounding our errorTOL,
    // and then we select an interpolation parameter to tween betwen them.  Ok, so
    // this isn't exactly an error tolerance.  A tween value of 0 is the lower poly
    // model (higher detail number) and a value of 1 is the higher poly model (lower
    // detail number).

    // deal with degenerate case first...
    // if smallest detail corresponds to less than half tolerable error, then don't even draw
    F32 prevErr;
    if (mShape->mSmallestVisibleDL < 0)
        prevErr = 0.0f;
    else
        prevErr = 10.0f * mShape->details[mShape->mSmallestVisibleDL].averageError * 20.0f;
    if (mShape->mSmallestVisibleDL < 0 || prevErr < errorTOL)
    {
        // draw last detail
        mCurrentDetailLevel = mShape->mSmallestVisibleDL;
        mCurrentIntraDetailLevel = 0.0f;
        return mCurrentDetailLevel;
    }

    // this function is a little odd
    // the reason is that the detail numbers correspond to
    // when we stop using a given detail level...
    // we search the details from most error to least error
    // until we fit under the tolerance (errorTOL) and then
    // we use the next highest detail (higher error)
    for (S32 i = mShape->mSmallestVisibleDL; i >= 0; i--)
    {
        F32 err0 = 10.0f * mShape->details[i].averageError;
        if (err0 < errorTOL)
        {
            // ok, stop here

            // intraDL = 1 corresponds to fully this detail
            // intraDL = 0 corresponds to the next lower (higher number) detail
            mCurrentDetailLevel = i;
            mCurrentIntraDetailLevel = 1.0f - (errorTOL - err0) / (prevErr - err0);
            return mCurrentDetailLevel;
        }
        prevErr = err0;
    }

    // get here if we are drawing at DL==0
    mCurrentDetailLevel = 1;
    mCurrentIntraDetailLevel = 1.0f;
    return mCurrentDetailLevel;
}

GBitmap* TSShapeInstance::snapshot(TSShape* shape, U32 width, U32 height, bool mip, MatrixF& cameraPos, S32 dl, F32 intraDL, bool hiQuality)
{
    TSShapeInstance* shapeInstance = new TSShapeInstance(shape, true);
    shapeInstance->setCurrentDetail(dl, intraDL);
    shapeInstance->animate();
    GBitmap* bmp = shapeInstance->snapshot(width, height, mip, cameraPos, hiQuality);

    delete shapeInstance;

    return bmp;
}

GBitmap* TSShapeInstance::snapshot(U32 width, U32 height, bool mip, MatrixF& cameraPos, S32 dl, F32 intraDL, bool hiQuality)
{
    setCurrentDetail(dl, intraDL);
    animate();
    return snapshot(width, height, mip, cameraPos, hiQuality);
}

GBitmap* TSShapeInstance::snapshot_softblend(U32 width, U32 height, bool mip, MatrixF& cameraMatrix, bool hiQuality)
{
    // this version of the snapshot function renders the shape to a black texture, then to white, then reads bitmaps 
    // back for both renders and combines them, restoring the alpha and color values.  this is based on the
    // TGE implementation.  it is not fast due to the copy and software combination operations.  the generated bitmaps
    // are upside-down (which is how TGE generated them...)

    Point2I size = GFX->getVideoMode().resolution;
    U32 screenWidth = size.x;
    U32 screenHeight = size.y;
    U32 xcenter = screenWidth >> 1;
    U32 ycenter = screenHeight >> 1;

    if (screenWidth == 0 || screenHeight == 0)
        return NULL; // probably in exporter...

    PROFILE_START(TSShapeInstance_snapshot_sb);

    AssertFatal(width < screenWidth&& height < screenHeight, "TSShapeInstance::snapshot: bitmap cannot be larger than screen resolution");

    S32 scale = 1;
    if (hiQuality)
        while ((scale << 1) * width <= screenWidth && (scale << 1) * height <= screenHeight)
            scale <<= 1;
    if (scale > smMaxSnapshotScale)
        scale = smMaxSnapshotScale;

    // height and width of intermediate bitmaps
    U32 bmpWidth = width * scale;
    U32 bmpHeight = height * scale;

    PROFILE_START(TSShapeInstance_snapshot_sb_setup);

    GFX->setActiveDevice(0);
    GFX->beginScene();

    RectI saveViewport = GFX->getViewport();
    const MatrixF saveProj = GFX->getProjectionMatrix();
    GFX->pushWorldMatrix();

    // setup viewport and frustrum (do orthographic projection)
    GFX->setViewport(RectI(0, 0, bmpWidth, bmpHeight));

    // TGE snapshots images are upside down.  For compatibility with rendering code that expects this, 
    // flip the projection vertically so that the images generated are upside down.
    GFX->setOrtho(-mShape->radius, mShape->radius, mShape->radius, -mShape->radius, 1, 20.0f * mShape->radius);

    // position camera...
    Point3F y;
    cameraMatrix.getColumn(1, &y);
    y *= -10.0f * mShape->radius; // move camera back ten units * shape radius
    y += mShape->center; // translate camera so that we're looking at center of shape
    cameraMatrix.setColumn(3, y); // y is now the new position in object space, set it in the matrix
    // store the camera position for the scene state structure below
    Point3F cp = y;
    cameraMatrix.inverse(); // make it into an object -> world transformation?

    // setup scene state required for TS mesh render...this is messy and inefficient; 
    // should have a mode where most of this is done just once (and then 
    // only the camera matrix changes between snapshots).
    // note that we use getFrustum here, but we set up an ortho projection above.  
    // it doesn't seem like the scene state object pays attention to whether the projection is 
    // ortho or not.  this could become a problem if some code downstream tries to 
    // reconstruct the projection matrix using the dimensions and doesn't 
    // realize it should be ortho.  at the moment no code is doing that.
    F32 left, right, top, bottom, nearPlane, farPlane;
    GFX->getFrustum(&left, &right, &bottom, &top, &nearPlane, &farPlane);
    U32 numFogVolumes;
    FogVolume* fogVolumes;
    getCurrentClientSceneGraph()->getFogVolumes(numFogVolumes, fogVolumes);

    SceneState state(
        NULL,
        1,
        left, right,
        bottom, top,
        nearPlane,
        farPlane, //far plane
        GFX->getViewport(),
        cp,
        cameraMatrix,
        farPlane, // fog distance - aka disable fog
        farPlane, // vis distance mod
        ColorF(1.0f, 1.0f, 1.0f, 1.0f), // fog color
        numFogVolumes,// num fog volumnes
        fogVolumes, // ptr to fog volumes
        farPlane); // vis factor

     // build the fog texture
    getCurrentClientSceneGraph()->buildFogTexture(&state);

    // we'll also need to add a light to the scene if there isn't one already
    const LightInfo* light = getCurrentClientSceneGraph()->getLightManager()->sgGetSpecialLight(LightManager::sgSunLightType);

    TSMesh::setCamTrans(cameraMatrix);
    TSMesh::setSceneState(&state);

    // we don't support refraction and glow in the snapshots at this time.
    TSMesh::setGlow(false);
    TSMesh::setRefract(false);

    // TSMesh expects the world transform to contain the object's world transform.  
    // set it to identity (cameraMatrix contains all the transforming we need to do)
    GFX->setWorldMatrix(MatrixF(true));

    // set some initial states
    GFX->setCullMode(GFXCullNone);
    GFX->setLightingEnable(false);
    GFX->setZEnable(true);
    GFX->setZFunc(GFXCmpLessEqual);

    // set gfx up for render to texture
    //GFX->pushActiveRenderSurfaces();

    PROFILE_END(); // setup

    PROFILE_START(TSShapeInstance_snapshot_sb_renderblack);
    // take a snapshot of the shape with a black background...
    GFXTexHandle blackTex;
    blackTex.set(bmpWidth, bmpHeight, GFXFormatR8G8B8A8, &GFXDefaultRenderTargetProfile);
    //GFX->setActiveRenderSurface(blackTex);

    ColorI black(0, 0, 0, 0);
    GFX->clear(GFXClearZBuffer | GFXClearStencil | GFXClearTarget, black, 1.0f, 0);
    render(mCurrentDetailLevel, mCurrentIntraDetailLevel);
    PROFILE_END();

    PROFILE_START(TSShapeInstance_snapshot_sb_renderwhite);
    // take a snapshot of the shape with a white background...
    GFXTexHandle whiteTex;
    whiteTex.set(bmpWidth, bmpHeight, GFXFormatR8G8B8A8, &GFXDefaultRenderTargetProfile);
    //GFX->setActiveRenderSurface(whiteTex);

    ColorI white(255, 255, 255, 255);
    GFX->clear(GFXClearZBuffer | GFXClearStencil | GFXClearTarget, white, 1.0f, 0);
    render(mCurrentDetailLevel, mCurrentIntraDetailLevel);
    PROFILE_END();

    PROFILE_START(TSShapeInstance_snapshot_sb_unsetup);

    // done rendering, reset render states

    //GFX->popActiveRenderSurfaces();

    GFX->setZEnable(false);
    GFX->setBaseRenderState();

    GFX->setViewport(saveViewport);
    GFX->setProjectionMatrix(saveProj);
    GFX->popWorldMatrix();

    GFX->endScene();
    PROFILE_END();

    PROFILE_START(TSShapeInstance_snapshot_sb_separate);

    // copy the black render target data into a bitmap
    GBitmap* blackBmp = new GBitmap;
    blackBmp->allocateBitmap(bmpWidth, bmpHeight, false, GFXFormatR8G8B8);
    blackTex->copyToBmp(blackBmp);

    // copy the white target data into a bitmap
    GBitmap* whiteBmp = new GBitmap;
    whiteBmp->allocateBitmap(bmpWidth, bmpHeight, false, GFXFormatR8G8B8);
    whiteTex->copyToBmp(whiteBmp);

    // now separate the color and alpha channels
    GBitmap* bmp = new GBitmap;
    bmp->allocateBitmap(width, height, mip, GFXFormatR8G8B8A8);
    U8* wbmp = (U8*)whiteBmp->getBits(0);
    U8* bbmp = (U8*)blackBmp->getBits(0);
    U8* dst = (U8*)bmp->getBits(0);
    U32 i, j;

    if (hiQuality)
    {
        for (i = 0; i < height; i++)
        {
            for (j = 0; j < width; j++)
            {
                F32 alphaTally = 0.0f;
                S32 alphaIntTally = 0;
                S32 alphaCount = 0;
                F32 rTally = 0.0f;
                F32 gTally = 0.0f;
                F32 bTally = 0.0f;
                for (S32 k = 0; k < scale; k++)
                {
                    for (S32 l = 0; l < scale; l++)
                    {
                        // shape on black background is alpha * color, shape on white background is alpha * color + (1-alpha) * 255
                        // we want 255 * alpha, or 255 - (white - black)
                        U32 pos = 3 * ((i * scale + k) * bmpWidth + j * scale + l);
                        U32 alpha = 255 - (wbmp[pos + 0] - bbmp[pos + 0]);
                        alpha += 255 - (wbmp[pos + 1] - bbmp[pos + 1]);
                        alpha += 255 - (wbmp[pos + 2] - bbmp[pos + 2]);
                        F32 floatAlpha = ((F32)alpha) / (1.0f * 255.0f);
                        if (alpha != 0)
                        {
                            rTally += bbmp[pos + 0];
                            gTally += bbmp[pos + 1];
                            bTally += bbmp[pos + 2];
                            alphaCount++;
                        }
                        alphaTally += floatAlpha;
                        alphaIntTally += alpha;
                    }
                }
                F32 invAlpha = alphaTally > 0.01f ? 1.0f / alphaTally : 0.0f;
                U32 pos = 4 * (i * width + j);
                dst[pos + 0] = (U8)(rTally * invAlpha);
                dst[pos + 1] = (U8)(gTally * invAlpha);
                dst[pos + 2] = (U8)(bTally * invAlpha);
                dst[pos + 3] = (U8)(((F32)alphaIntTally) / (F32)(3 * alphaCount));
            }
        }
    }
    else
    {
        // simpler, probably faster...
        for (i = 0; i < height * width; i++)
        {
            // shape on black background is alpha * color, shape on white background is alpha * color + (1-alpha) * 255
            // we want 255 * alpha, or 255 - (white - black)
            // JMQ: or more verbosely:
            //  cB = alpha * color + (0 * (1 - alpha))
            //  cB = alpha * color
            //  cW = alpha * color + (255 * (1 - alpha))
            //  cW = cB + (255 * (1 - alpha))
            // solving for alpha
            //  cW - cB = 255 * (1 - alpha)
            //  (cW - cB)/255 = (1 - alpha)
            //  alpha = 1 - (cW - cB)/255
            // since we want alpha*255, multiply through by 255
            //  alpha * 255 = 255 - cW - cB
            U32 alpha = 255 - (wbmp[i * 3 + 0] - bbmp[i * 3 + 0]);
            alpha += 255 - (wbmp[i * 3 + 1] - bbmp[i * 3 + 1]);
            alpha += 255 - (wbmp[i * 3 + 2] - bbmp[i * 3 + 2]);

            if (alpha != 0)
            {
                F32 floatAlpha = ((F32)alpha) / (1.0f * 255.0f);
                dst[i * 4 + 0] = (U8)(bbmp[i * 3 + 0] / floatAlpha);
                dst[i * 4 + 1] = (U8)(bbmp[i * 3 + 1] / floatAlpha);
                dst[i * 4 + 2] = (U8)(bbmp[i * 3 + 2] / floatAlpha);
                dst[i * 4 + 3] = (U8)(alpha / 3);
            }
            else
            {
                dst[i * 4 + 0] = dst[i * 4 + 1] = dst[i * 4 + 2] = dst[i * 4 + 3] = 0;
            }
        }
    }
    PROFILE_END();

    PROFILE_START(TSShapeInstance_snapshot_sb_extrude);
    if (mip)
        bmp->extrudeMipLevels();
    PROFILE_END();

    delete blackBmp;
    delete whiteBmp;

    blackTex = NULL;
    whiteTex = NULL;

    PROFILE_END();

    return bmp;
}

GBitmap* TSShapeInstance::snapshot(U32 width, U32 height, bool mip, MatrixF& cameraMatrix, bool hiQuality)
{
    //GFX_Canonizer("TSShapeInstance::snapshot", __FILE__, __END__);

    GBitmap* bmp = snapshot_softblend(width, height, mip, cameraMatrix, hiQuality);
    return bmp;
}

void TSShapeInstance::renderShadow(S32 dl, const MatrixF& mat, S32 dim, U32* bits)
{
    /*
       // if dl==-1, nothing to do
       if (dl==-1)
          return;

       AssertFatal(dl>=0 && dl<mShape->details.size(),"TSShapeInstance::renderShadow");

       S32 i;

       const TSDetail * detail = &mShape->details[dl];
       S32 ss = detail->subShapeNum;
       S32 od = detail->objectDetailNum;

       // assert if we're a billboard detail
       AssertFatal(ss>=0,"TSShapeInstance::renderShadow: not with a billboard detail level");

       // set up render data
       setStatics(dl);

       // run through the meshes
       smRenderData.currentTransform = NULL;
       S32 start = mShape->subShapeFirstObject[ss];
       S32 end   = start + mShape->subShapeNumObjects[ss];
       for (i=start; i<end; i++)
          mMeshObjects[i].renderShadow(od,mat,dim,bits,mMaterialList);

       // if we have a marix pushed, pop it now
       if (smRenderData.currentTransform)
          glPopMatrix();

       clearStatics();
    */
}

//-------------------------------------------------------------------------------------
// Object (MeshObjectInstance & PluginObjectInstance) render methods
//-------------------------------------------------------------------------------------

void TSShapeInstance::ObjectInstance::render(S32, TSMaterialList*)
{
    AssertFatal(0, "TSShapeInstance::ObjectInstance::render:  no default render method.");
}

void TSShapeInstance::ObjectInstance::renderEnvironmentMap(S32, TSMaterialList*)
{
    AssertFatal(0, "TSShapeInstance::ObjectInstance::renderEnvironmentMap:  no default render method.");
}

void TSShapeInstance::ObjectInstance::renderDetailMap(S32, TSMaterialList*)
{
    AssertFatal(0, "TSShapeInstance::ObjectInstance::renderDetailMap:  no default render method.");
}

void TSShapeInstance::ObjectInstance::renderFog(S32, TSMaterialList*)
{
    AssertFatal(0, "TSShapeInstance::ObjectInstance::renderFog:  no default render method.");
}

S32 TSShapeInstance::MeshObjectInstance::getSizeVB(S32 size)
{
    TSMesh* mesh = getMesh(0);

    if (mesh && mesh->getMeshType() == TSMesh::StandardMeshType && mesh->vertsPerFrame > 0)
    {
        mesh->vbOffset = size;

        return mesh->numFrames * mesh->numMatFrames * mesh->vertsPerFrame;
    }
    else
        return 0;
}

bool TSShapeInstance::MeshObjectInstance::hasMergeIndices()
{
    TSMesh* mesh = getMesh(0);

    return (mesh && mesh->getMeshType() == TSMesh::StandardMeshType && mesh->mergeIndices.size());
}

void TSShapeInstance::MeshObjectInstance::fillVB(S32 vb, TSMaterialList* materials)
{
    TSMesh* mesh = getMesh(0);

    if (!mesh || mesh->getMeshType() != TSMesh::StandardMeshType || mesh->vertsPerFrame <= 0)
        return;

    for (S32 f = 0; f < mesh->numFrames; ++f)
        for (S32 m = 0; m < mesh->numMatFrames; ++m)
            mesh->fillVB(vb, f, m, materials);
}

void TSShapeInstance::MeshObjectInstance::morphVB(S32 vb, S32& previousMerge, S32 objectDetail, TSMaterialList* materials)
{
    /*
       if (visible > 0.01f)
       {
          TSMesh *m0 = getMesh(0);
          TSMesh *mesh = getMesh(objectDetail);

          if (m0 && mesh)
          {
             // render TSSortedMesh's standard
             if (m0->getMeshType() != TSMesh::StandardMeshType)
                return;

             GLuint foffset = (frame*m0->numMatFrames + matFrame)*m0->vertsPerFrame;
             U32 morphSize = mesh->mergeIndices.size();
             S32 merge = mesh->vertsPerFrame-morphSize;

             if (!morphSize)
                return;

             if (previousMerge != -1 && previousMerge < merge)
             {
                S32 tmp = merge;

                merge = previousMerge;
                previousMerge = tmp;
                morphSize = mesh->vertsPerFrame-merge;
             }
             else
                previousMerge = merge;

             glOffsetVertexBufferEXT(vb,m0->vbOffset + foffset + merge);
             mesh->morphVB(vb,morphSize,frame,matFrame,materials);
          }
       }
    */
}

void TSShapeInstance::MeshObjectInstance::renderVB(S32 vb, S32 objectDetail, TSMaterialList* materials)
{
    /*
       if (visible > 0.01f)
       {
          TSMesh *m0 = getMesh(0);
          TSMesh *mesh = getMesh(objectDetail);

          if (m0 && mesh)
          {
             // render TSSortedMesh's standard
             if (m0->getMeshType() != TSMesh::StandardMeshType)
             {
                render(objectDetail, materials);
                return;
             }

             if (mesh->vertsPerFrame <= 0)
                return;

             MatrixF *transform = getTransform();

             if (transform != TSShapeInstance::smRenderData.currentTransform)
             {
                if (TSShapeInstance::smRenderData.currentTransform)
                   glPopMatrix();
                if (transform)
                {
                   glPushMatrix();
                   dglMultMatrix(transform);
                }
                TSShapeInstance::smRenderData.currentTransform = transform;
             }

             GLuint foffset = (frame*m0->numMatFrames + matFrame)*m0->vertsPerFrame;

             glSetVertexBufferEXT(vb);
             glOffsetVertexBufferEXT(vb,m0->vbOffset + foffset);

             if (visible>0.99f)
             {
                if (TSShapeInstance::smRenderData.balloonShape)
                {
                   glPushMatrix();

                   F32 &bv = TSShapeInstance::smRenderData.balloonValue;

                   glScalef(bv,bv,bv);
                }
                mesh->renderVB(frame,matFrame,materials);
                if (TSShapeInstance::smRenderData.balloonShape)
                   glPopMatrix();
             }
             else
             {
                mesh->setFade(visible);
                mesh->renderVB(frame,matFrame,materials);
                mesh->clearFade();
             }
          }
       }
    */
}

Vector< MatrixF > gMatStack;

MatrixF gTemp;

void TSShapeInstance::MeshObjectInstance::render(S32 objectDetail, TSMaterialList* materials)
{
    if (visible > 0.01f)
    {
        TSMesh* mesh = getMesh(objectDetail);
        if (mesh)
        {
            MatrixF* transform = getTransform();
            //         if (transform != TSShapeInstance::smRenderData.currentTransform)
            {
                if (TSShapeInstance::smRenderData.currentTransform)
                {
                    GFX->popWorldMatrix();
                }
                if (transform)
                {
                    GFX->pushWorldMatrix();
                    GFX->multWorld(*transform);
                }
                TSShapeInstance::smRenderData.currentTransform = transform;
            }
            if (visible > 0.99f)
            {
                mesh->render(frame, matFrame, materials);
            }
            else
            {
                mesh->setFade(visible);
                mesh->render(frame, matFrame, materials);
                mesh->clearFade();
            }
        }
    }
}

void TSShapeInstance::DecalObjectInstance::render(S32 objectDetail, TSMaterialList* materials)
{
    /*
       if (targetObject->visible>0.01f)
       {
          TSDecalMesh * decalMesh = getDecalMesh(objectDetail);
          if (decalMesh && frame>=0)
          {
             MatrixF * transform = getTransform();
             if (transform != TSShapeInstance::smRenderData.currentTransform)
             {
                if (TSShapeInstance::smRenderData.currentTransform)
                   glPopMatrix();
                if (transform)
                {
                   glPushMatrix();
                   dglMultMatrix(transform);
                }
                TSShapeInstance::smRenderData.currentTransform = transform;
             }

             if (TSShapeInstance::smRenderData.balloonShape)
             {
                glPushMatrix();
                F32 & bv = TSShapeInstance::smRenderData.balloonValue;
                glScalef(bv,bv,bv);
             }
             decalMesh->render(targetObject->frame,frame,materials);
             if (TSShapeInstance::smRenderData.balloonShape)
                glPopMatrix();
          }
       }
    */
}

void TSShapeInstance::MeshObjectInstance::renderEnvironmentMap(S32 objectDetail, TSMaterialList* materials)
{
    /*
       if (visible>0.01f)
       {
          TSMesh * mesh = getMesh(objectDetail);
          if (mesh)
          {
             MatrixF * transform = getTransform();
             if (transform != TSShapeInstance::smRenderData.currentTransform)
             {
                if (TSShapeInstance::smRenderData.currentTransform)
                   glPopMatrix();
                if (transform)
                {
                   glPushMatrix();
                   dglMultMatrix(transform);
                }
                TSShapeInstance::smRenderData.currentTransform = transform;
             }

             if (TSShapeInstance::smRenderData.balloonShape)
             {
                glPushMatrix();
                F32 & bv = TSShapeInstance::smRenderData.balloonValue;
                glScalef(bv,bv,bv);
             }
             mesh->renderEnvironmentMap(frame,matFrame,materials);
             if (TSShapeInstance::smRenderData.balloonShape)
                glPopMatrix();
          }
       }
    */
}

void TSShapeInstance::MeshObjectInstance::renderDetailMap(S32 objectDetail, TSMaterialList* materials)
{
    /*
       if (visible>0.01f)
       {
          TSMesh * mesh = getMesh(objectDetail);
          if (mesh && mesh->getFlags(TSMesh::HasDetailTexture))
          {
             MatrixF * transform = getTransform();
             if (transform != TSShapeInstance::smRenderData.currentTransform)
             {
                if (TSShapeInstance::smRenderData.currentTransform)
                   glPopMatrix();
                if (transform)
                {
                   glPushMatrix();
                   dglMultMatrix(transform);
                }
                TSShapeInstance::smRenderData.currentTransform = transform;
             }

             if (TSShapeInstance::smRenderData.balloonShape)
             {
                glPushMatrix();
                F32 & bv = TSShapeInstance::smRenderData.balloonValue;
                glScalef(bv,bv,bv);
             }
             mesh->renderDetailMap(frame,matFrame,materials);
             if (TSShapeInstance::smRenderData.balloonShape)
                glPopMatrix();
          }
       }
    */
}

void TSShapeInstance::MeshObjectInstance::renderShadow(S32 objectDetail, const MatrixF& mat, S32 dim, U32* bits, TSMaterialList* materialList)
{
    /*
       if (visible>0.01f)
       {
          TSMesh * mesh = getMesh(objectDetail);
          if (mesh)
          {
             MatrixF mat2;
             MatrixF * transform = getTransform();
             if (transform)
                mat2.mul(mat,*transform);
             else
                mat2=mat;
             mesh->renderShadow(frame,mat2,dim,bits,materialList);
          }
       }
    */
}

void TSShapeInstance::MeshObjectInstance::renderFog(S32 objectDetail, TSMaterialList* materials)
{
    /*
       if (visible>0.01f)
       {
          TSMesh * mesh = getMesh(objectDetail);
          if (mesh)
          {
             MatrixF * transform = getTransform();
             if (transform != TSShapeInstance::smRenderData.currentTransform)
             {
                if (TSShapeInstance::smRenderData.currentTransform)
                   glPopMatrix();
                if (transform)
                {
                   glPushMatrix();
                   dglMultMatrix(transform);
                }
                TSShapeInstance::smRenderData.currentTransform = transform;
             }

             if (TSShapeInstance::smRenderData.balloonShape)
             {
                glPushMatrix();
                F32 & bv = TSShapeInstance::smRenderData.balloonValue;
                glScalef(bv,bv,bv);
             }
             mesh->renderFog(frame, materials);
             if (TSShapeInstance::smRenderData.balloonShape)
                glPopMatrix();
          }
       }
    */
}

void TSShapeInstance::incDebrisRefCount()
{
    ++debrisRefCount;
}

void TSShapeInstance::decDebrisRefCount()
{
    if (debrisRefCount == 0) return;
    --debrisRefCount;
}

U32 TSShapeInstance::getDebrisRefCount()
{
    return debrisRefCount;
}


U32 TSShapeInstance::getNumDetails()
{
    if (mShape)
    {
        return mShape->details.size();
    }

    return 0;
}

void TSShapeInstance::prepCollision()
{
    PROFILE_SCOPE(TSShapeInstance_PrepCollision);

    setStatics(0);
    // Iterate over all our meshes and call prepCollision on them...
    for (S32 i = 0; i < mShape->meshes.size(); i++)
    {
        if (mShape->meshes[i])
            mShape->meshes[i]->prepOpcodeCollision();
    }
    clearStatics();
}