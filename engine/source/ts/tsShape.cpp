//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "ts/tsShape.h"
#include "ts/tsLastDetail.h"
#include "core/stringTable.h"
#include "console/console.h"
#include "ts/tsShapeInstance.h"
#include "collision/convex.h"
#include <string>
#include "collada/colladaShapeLoader.h"

/// most recent version -- this is the version we write
S32 TSShape::smVersion = 24;
/// the version currently being read...valid only during a read
S32 TSShape::smReadVersion = -1;
const U32 TSShape::smMostRecentExporterVersion = DTS_EXPORTER_CURRENT_VERSION;

F32 TSShape::smAlphaOutLastDetail = -1.0f;
F32 TSShape::smAlphaInBillboard = 0.15f;
F32 TSShape::smAlphaOutBillboard = 0.1f;
F32 TSShape::smAlphaInDefault = -1.0f;
F32 TSShape::smAlphaOutDefault = -1.0f;

// don't bother even loading this many of the highest detail levels (but
// always load last renderable detail)
S32 TSShape::smNumSkipLoadDetails = 0;

bool TSShape::smInitOnRead = true;


TSShape::TSShape()
{
    materialList = NULL;
    mReadVersion = -1; // -1 means constructed from scratch (e.g., in exporter or no read yet)
    mMemoryBlock = NULL;

    mSequencesConstructed = false;

    mVertexBuffer = (U32)-1;
    mCallbackKey = (U32)-1;

    VECTOR_SET_ASSOCIATION(sequences);
    VECTOR_SET_ASSOCIATION(billboardDetails);
    VECTOR_SET_ASSOCIATION(detailCollisionAccelerators);
    VECTOR_SET_ASSOCIATION(names);
}

TSShape::~TSShape()
{
    clearDynamicData();
    delete materialList;

    S32 i;

    // before deleting meshes, we have to delete decals and get them out
    // of the mesh list...this is a legacy issue from when decals were meshes
    for (i = 0; i < decals.size(); i++)
    {
        for (S32 j = 0; j < decals[i].numMeshes; j++)
        {
            if ((TSDecalMesh*)meshes[decals[i].startMeshIndex + j])
                destructInPlace((TSDecalMesh*)meshes[decals[i].startMeshIndex + j]);
            meshes[decals[i].startMeshIndex + j] = NULL;
        }
    }

    // everything left over here is a legit mesh
    for (i = 0; i < meshes.size(); i++)
        if (meshes[i])
            destructInPlace(meshes[i]);

    for (i = 0; i < sequences.size(); i++)
        destructInPlace(&sequences[i]);

    for (i = 0; i < billboardDetails.size(); i++)
    {
        delete billboardDetails[i];
        billboardDetails[i] = NULL;
    }
    billboardDetails.clear();

    // Delete any generated accelerators
    S32 dca;
    for (dca = 0; dca < detailCollisionAccelerators.size(); dca++)
    {
        ConvexHullAccelerator* accel = detailCollisionAccelerators[dca];
        if (accel != NULL) {
            delete[] accel->vertexList;
            delete[] accel->normalList;
            for (S32 j = 0; j < accel->numVerts; j++)
                delete[] accel->emitStrings[j];
            delete[] accel->emitStrings;
            delete accel;
        }
    }
    for (dca = 0; dca < detailCollisionAccelerators.size(); dca++)
        detailCollisionAccelerators[dca] = NULL;

    delete[] mMemoryBlock;
    mMemoryBlock = NULL;

    /*
       if (mVertexBuffer != -1)
          if (dglDoesSupportVertexBuffer())
             glFreeVertexBufferEXT(mVertexBuffer);
          else
             AssertFatal(false,"Vertex buffer should have already been freed!");
       if (mCallbackKey != -1)
          TextureManager::unregisterEventCallback(mCallbackKey);
    */
}

void TSShape::clearDynamicData()
{
}

const char* TSShape::getName(S32 nameIndex) const
{
    AssertFatal(nameIndex >= 0 && nameIndex < names.size(), "TSShape::getName");
    return names[nameIndex];
}

S32 TSShape::findName(const char* name) const
{
    for (S32 i = 0; i < names.size(); i++)
        if (!dStricmp(name, names[i]))
            return i;
    return -1;
}

S32 TSShape::findNode(S32 nameIndex) const
{
    for (S32 i = 0; i < nodes.size(); i++)
        if (nodes[i].nameIndex == nameIndex)
            return i;
    return -1;
}

S32 TSShape::findObject(S32 nameIndex) const
{
    for (S32 i = 0; i < objects.size(); i++)
        if (objects[i].nameIndex == nameIndex)
            return i;
    return -1;
}

S32 TSShape::findDecal(S32 nameIndex) const
{
    for (S32 i = 0; i < decals.size(); i++)
        if (decals[i].nameIndex == nameIndex)
            return i;
    return -1;
}

S32 TSShape::findIflMaterial(S32 nameIndex) const
{
    for (S32 i = 0; i < iflMaterials.size(); i++)
        if (iflMaterials[i].nameIndex == nameIndex)
            return i;
    return -1;
}

S32 TSShape::findDetail(S32 nameIndex) const
{
    for (S32 i = 0; i < details.size(); i++)
        if (details[i].nameIndex == nameIndex)
            return i;
    return -1;
}

S32 TSShape::findSequence(S32 nameIndex) const
{
    for (S32 i = 0; i < sequences.size(); i++)
        if (sequences[i].nameIndex == nameIndex)
            return i;
    return -1;
}

bool TSShape::findMeshIndex(const char* meshName, S32& objIndex, S32& meshIndex)
{
    // Determine the object name and detail size from the mesh name
    S32 detailSize = 999;
    objIndex = findObject(dGetTrailingNumber(meshName, detailSize));
    if (objIndex < 0)
        return false;

    // Determine the subshape this object belongs to
    S32 subShapeIndex = getSubShapeForObject(objIndex);
    AssertFatal(subShapeIndex < subShapeFirstObject.size(), "Could not find subshape for object!");

    // Get the detail levels for the subshape
    Vector<S32> validDetails;
    getSubShapeDetails(subShapeIndex, validDetails);

    // Find the detail with the correct size
    for (meshIndex = 0; meshIndex < validDetails.size(); meshIndex++)
    {
        const TSShape::Detail& det = details[validDetails[meshIndex]];
        if (detailSize == det.size)
            return true;
    }

    return false;
}

TSMesh* TSShape::findMesh(const char* meshName)
{
    S32 objIndex, meshIndex;
    if (!findMeshIndex(meshName, objIndex, meshIndex))
        return 0;
    return meshes[objects[objIndex].startMeshIndex + meshIndex];
}

S32 TSShape::getSubShapeForNode(S32 nodeIndex)
{
    for (S32 i = 0; i < subShapeFirstNode.size(); i++)
    {
        S32 start = subShapeFirstNode[i];
        S32 end = start + subShapeNumNodes[i];
        if ((nodeIndex >= start) && (nodeIndex < end))
            return i;;
    }
    return -1;
}

S32 TSShape::getSubShapeForObject(S32 objIndex)
{
    for (S32 i = 0; i < subShapeFirstObject.size(); i++)
    {
        S32 start = subShapeFirstObject[i];
        S32 end = start + subShapeNumObjects[i];
        if ((objIndex >= start) && (objIndex < end))
            return i;
    }
    return -1;
}

void TSShape::getSubShapeDetails(S32 subShapeIndex, Vector<S32>& validDetails)
{
    validDetails.clear();
    for (S32 i = 0; i < details.size(); i++)
    {
        if ((details[i].subShapeNum == subShapeIndex) ||
            (details[i].subShapeNum < 0))
            validDetails.push_back(i);
    }
}

void TSShape::getNodeWorldTransform(S32 nodeIndex, MatrixF* mat) const
{
    // Calculate the world transform of the given node
    defaultRotations[nodeIndex].getQuatF().setMatrix(mat);
    mat->setPosition(defaultTranslations[nodeIndex]);

    S32 parentIndex = nodes[nodeIndex].parentIndex;
    while (parentIndex != -1)
    {
        MatrixF mat2(*mat);
        defaultRotations[parentIndex].getQuatF().setMatrix(mat);
        mat->setPosition(defaultTranslations[parentIndex]);
        mat->mul(mat2);

        parentIndex = nodes[parentIndex].parentIndex;
    }
}

void TSShape::getNodeObjects(S32 nodeIndex, Vector<S32>& nodeObjects)
{
    for (S32 i = 0; i < objects.size(); i++)
    {
        if ((nodeIndex == -1) || (objects[i].nodeIndex == nodeIndex))
            nodeObjects.push_back(i);
    }
}

void TSShape::getNodeChildren(S32 nodeIndex, Vector<S32>& nodeChildren)
{
    for (S32 i = 0; i < nodes.size(); i++)
    {
        if (nodes[i].parentIndex == nodeIndex)
            nodeChildren.push_back(i);
    }
}

void TSShape::getObjectDetails(S32 objIndex, Vector<S32>& objDetails)
{
    // Get the detail levels for this subshape
    Vector<S32> validDetails;
    getSubShapeDetails(getSubShapeForObject(objIndex), validDetails);

    // Get the non-null details for this object
    const TSShape::Object& obj = objects[objIndex];
    for (S32 i = 0; i < obj.numMeshes; i++)
    {
        if (meshes[obj.startMeshIndex + i])
            objDetails.push_back(validDetails[i]);
    }
}


void TSShape::init()
{
    clearDynamicData();

    S32 numSubShapes = subShapeFirstNode.size();
    AssertFatal(numSubShapes == subShapeFirstObject.size(), "TSShape::init");

    S32 i, j;

    // set up parent/child relationships on nodes and objects
    for (i = 0; i < nodes.size(); i++)
        nodes[i].firstObject = nodes[i].firstChild = nodes[i].nextSibling = -1;
    for (i = 0; i < nodes.size(); i++)
    {
        S32 parentIndex = nodes[i].parentIndex;
        if (parentIndex >= 0)
        {
            if (nodes[parentIndex].firstChild < 0)
                nodes[parentIndex].firstChild = i;
            else
            {
                S32 child = nodes[parentIndex].firstChild;
                while (nodes[child].nextSibling >= 0)
                    child = nodes[child].nextSibling;
                nodes[child].nextSibling = i;
            }
        }
    }
    for (i = 0; i < objects.size(); i++)
    {
        objects[i].nextSibling = -1;
        objects[i].firstDecal = -1;

        S32 nodeIndex = objects[i].nodeIndex;
        if (nodeIndex >= 0)
        {
            if (nodes[nodeIndex].firstObject < 0)
                nodes[nodeIndex].firstObject = i;
            else
            {
                S32 objectIndex = nodes[nodeIndex].firstObject;
                while (objects[objectIndex].nextSibling >= 0)
                    objectIndex = objects[objectIndex].nextSibling;
                objects[objectIndex].nextSibling = i;
            }
        }
    }
    for (i = 0; i < decals.size(); i++)
    {
        decals[i].nextSibling = -1;
        S32 objectIndex = decals[i].objectIndex;
        if (objects[objectIndex].firstDecal < 0)
            objects[objectIndex].firstDecal = i;
        else
        {
            S32 decalIndex = objects[objectIndex].firstDecal;
            while (decals[decalIndex].nextSibling >= 0)
                decalIndex = decals[decalIndex].nextSibling;
            decals[decalIndex].nextSibling = i;
        }
    }

    mFlags = 0;
    for (i = 0; i < sequences.size(); i++)
    {
        if (!sequences[i].animatesScale())
            continue;

        U32 curVal = mFlags & AnyScale;
        U32 newVal = sequences[i].flags & AnyScale;
        mFlags &= ~(AnyScale);
        mFlags |= getMax(curVal, newVal); // take the larger value (can only convert upwards)
    }

    // set up alphaIn and alphaOut vectors...
#if defined(TORQUE_MAX_LIB)
    alphaIn.setSize(details.size());
    alphaOut.setSize(details.size());
#endif
    for (i = 0; i < details.size(); i++)
    {
        if (details[i].size < 0)
        {
            // we don't care...
            alphaIn[i] = 0.0f;
            alphaOut[i] = 0.0f;
        }
        else if (i + 1 == details.size() || details[i + 1].size < 0)
        {
            alphaIn[i] = 0.0f;
            alphaOut[i] = smAlphaOutLastDetail;
        }
        else
        {
            if (details[i + 1].subShapeNum < 0)
            {
                // following detail is a billboard detail...treat special...
                alphaIn[i] = smAlphaInBillboard;
                alphaOut[i] = smAlphaOutBillboard;
            }
            else
            {
                // next detail is normal detail
                alphaIn[i] = smAlphaInDefault;
                alphaOut[i] = smAlphaOutDefault;
            }
        }
    }

    for (i = mSmallestVisibleDL - 1; i >= 0; i--)
    {
        if (i < smNumSkipLoadDetails)
        {
            // this detail level renders when pixel size
            // is larger than our cap...zap all the meshes and decals
            // associated with it and use the next detail level
            // instead...
            S32 ss = details[i].subShapeNum;
            S32 od = details[i].objectDetailNum;

            if (ss == details[i + 1].subShapeNum && od == details[i + 1].objectDetailNum)
                // doh! already done this one (init can be called multiple times on same shape due
                // to sequence importing).
                continue;
            details[i].subShapeNum = details[i + 1].subShapeNum;
            details[i].objectDetailNum = details[i + 1].objectDetailNum;
        }
    }

    for (i = 0; i < details.size(); i++)
    {
        S32 count = 0;
        S32 ss = details[i].subShapeNum;
        S32 od = details[i].objectDetailNum;
        if (ss < 0)
        {
            // billboard detail...
            count += 2;
            continue;
        }
        S32 start = subShapeFirstObject[ss];
        S32 end = start + subShapeNumObjects[ss];
        for (j = start; j < end; j++)
        {
            Object& obj = objects[j];
            if (od < obj.numMeshes)
            {
                TSMesh* mesh = meshes[obj.startMeshIndex + od];
                count += mesh ? mesh->getNumPolys() : 0;
            }
        }
        details[i].polyCount = count;
    }

    // Init the collision accelerator array.  Note that we don't compute the
    //  accelerators until the app requests them
    {
        S32 dca;
        for (dca = 0; dca < detailCollisionAccelerators.size(); dca++)
        {
            ConvexHullAccelerator* accel = detailCollisionAccelerators[dca];
            if (accel != NULL) {
                delete[] accel->vertexList;
                delete[] accel->normalList;
                for (S32 j = 0; j < accel->numVerts; j++)
                    delete[] accel->emitStrings[j];
                delete[] accel->emitStrings;
                delete accel;
            }
        }

        detailCollisionAccelerators.setSize(details.size());
        for (dca = 0; dca < detailCollisionAccelerators.size(); dca++)
            detailCollisionAccelerators[dca] = NULL;
    }

    mMergeBufferSize = 0;
    for (i = 0; i < objects.size(); i++)
    {
        TSObject* obj = &objects[i];
        S32 maxSize = 0;
        for (S32 dl = 0; dl < obj->numMeshes; dl++)
        {
            TSMesh* mesh = meshes[obj->startMeshIndex + dl];
            if (mesh)
            {
                mesh->mergeBufferStart = mMergeBufferSize;
                maxSize = getMax((S32)maxSize, (S32)mesh->mergeIndices.size());
            }
        }
        mMergeBufferSize += maxSize;
    }

    initMaterialList();
}

void TSShape::setupBillboardDetails(TSShapeInstance* shape)
{
    // set up billboard details -- only do this once, meaning that
    // if we add a sequence to the shape we don't redo the billboard
    // details...
    S32 i;
    if (billboardDetails.empty())
    {
        for (i = 0; i < details.size(); i++)
        {
            if (details[i].subShapeNum >= 0)
                continue; // not a billboard detail
            while (billboardDetails.size() <= i)
                billboardDetails.push_back(NULL);
            U32 props = details[i].objectDetailNum;
            U32 numEquatorSteps = props & 0x7F; // bits 0..6
            U32 numPolarSteps = (props >> 7) & 0x3F; // bits 7..12
            F32 polarAngle = 0.5f * M_PI_F * (1.0f / 64.0f) * (F32)((props >> 13) & 0x3F); // bits 13..18
            S32 dl = (props >> 19) & 0x0F;  // 19..22
            S32 dim = (props >> 23) & 0xFF; // 23..30
            bool includePoles = (props & 0x80000000) != 0; // bit 31

            billboardDetails[i] = new TSLastDetail(shape, numEquatorSteps, numPolarSteps, polarAngle, includePoles, dl, dim);
        }
    }
}

void TSShape::initMaterialList()
{

    S32 numSubShapes = subShapeFirstObject.size();
#if defined(TORQUE_MAX_LIB)
    subShapeFirstTranslucentObject.setSize(numSubShapes);
#endif

    S32 i, j, k;
    // for each subshape, find the first translucent object
    // also, while we're at it, set mHasTranslucency
    for (S32 ss = 0; ss < numSubShapes; ss++)
    {
        S32 start = subShapeFirstObject[ss];
        S32 end = subShapeNumObjects[ss];
        subShapeFirstTranslucentObject[ss] = end;
        for (i = start; i < end; i++)
        {
            // check to see if this object has translucency
            Object& obj = objects[i];
            for (j = 0; j < obj.numMeshes; j++)
            {
                TSMesh* mesh = meshes[obj.startMeshIndex + j];
                if (!mesh)
                    continue;
                for (k = 0; k < mesh->primitives.size(); k++)
                {
                    if (mesh->primitives[k].matIndex & TSDrawPrimitive::NoMaterial)
                        continue;
                    S32 flags = materialList->getFlags(mesh->primitives[k].matIndex & TSDrawPrimitive::MaterialMask);
                    if (flags & TSMaterialList::AuxiliaryMap)
                        continue;
                    if (flags & TSMaterialList::Translucent)
                    {
                        mFlags |= HasTranslucency;
                        subShapeFirstTranslucentObject[ss] = i;
                        break;
                    }
                }
                if (k != mesh->primitives.size())
                    break;
            }
            if (j != obj.numMeshes)
                break;
        }
        if (i != end)
            break;
    }

}

bool TSShape::preloadMaterialList()
{

    if (materialList)
        return materialList->load(MeshTexture, mSourceResource ? mSourceResource->path : "", true);

    return true;
}

bool TSShape::buildConvexHull(S32 dl) const
{
    AssertFatal(dl >= 0 && dl < details.size(), "TSShape::buildConvexHull: detail out of range");

    bool ok = true;

    const Detail& detail = details[dl];
    S32 ss = detail.subShapeNum;
    S32 od = detail.objectDetailNum;

    S32 start = subShapeFirstObject[ss];
    S32 end = subShapeNumObjects[ss];
    for (S32 i = start; i < end; i++)
    {
        TSMesh* mesh = meshes[objects[i].startMeshIndex + od];
        if (!mesh)
            continue;
        ok &= mesh->buildConvexHull();
    }
    return ok;
}

Vector<MatrixF> gTempNodeTransforms(__FILE__, __LINE__);

void TSShape::computeBounds(S32 dl, Box3F& bounds) const
{
    // if dl==-1, nothing to do
    if (dl == -1)
        return;

    AssertFatal(dl >= 0 && dl < details.size(), "TSShapeInstance::computeBounds");

    // get subshape and object detail
    const TSDetail* detail = &details[dl];
    S32 ss = detail->subShapeNum;
    S32 od = detail->objectDetailNum;

    // set up temporary storage for non-local transforms...
    S32 i;
    S32 start = subShapeFirstNode[ss];
    S32 end = subShapeNumNodes[ss] + start;
    gTempNodeTransforms.setSize(end - start);
    for (i = start; i < end; i++)
    {
        MatrixF mat;
        QuatF q;
        TSTransform::setMatrix(defaultRotations[i].getQuatF(&q), defaultTranslations[i], &mat);
        if (nodes[i].parentIndex >= 0)
            gTempNodeTransforms[i - start].mul(gTempNodeTransforms[nodes[i].parentIndex - start], mat);
        else
            gTempNodeTransforms[i - start] = mat;
    }

    // run through objects and updating bounds as we go
    bounds.min.set(10E30f, 10E30f, 10E30f);
    bounds.max.set(-10E30f, -10E30f, -10E30f);
    Box3F box;
    start = subShapeFirstObject[ss];
    end = subShapeNumObjects[ss] + start;
    for (i = start; i < end; i++)
    {
        const Object* object = &objects[i];
        TSMesh* mesh = od < object->numMeshes ? meshes[object->startMeshIndex + od] : NULL;
        if (mesh)
        {
            static MatrixF idMat(true);
            if (object->nodeIndex < 0)
                mesh->computeBounds(idMat, box);
            else
                mesh->computeBounds(gTempNodeTransforms[object->nodeIndex - start], box);
            bounds.min.setMin(box.min);
            bounds.max.setMax(box.max);
        }
    }
}

TSShapeAlloc TSShape::alloc;

#define alloc TSShape::alloc

// messy stuff: check to see if we should "skip" meshNum
// this assumes that meshes for a given object/decal are in a row
// skipDL is the lowest detail number we keep (i.e., the # of details we skip)
bool TSShape::checkSkip(S32 meshNum, S32& curObject, S32& curDecal, S32 skipDL)
{
    if (skipDL == 0)
        // easy out...
        return false;

    // skip detail level exists on this subShape
    S32 skipSS = details[skipDL].subShapeNum;

    if (curObject < objects.size())
    {
        S32 start = objects[curObject].startMeshIndex;
        if (meshNum >= start)
        {
            // we are either from this object, the next object, or a decal
            if (meshNum < start + objects[curObject].numMeshes)
            {
                // this object...
                if (subShapeFirstObject[skipSS] > curObject)
                    // haven't reached this subshape yet
                    return true;
                if (skipSS + 1 == subShapeFirstObject.size() || curObject < subShapeFirstObject[skipSS + 1])
                    // curObject is on subshape of skip detail...make sure it's after skipDL
                    return (meshNum - start < details[skipDL].objectDetailNum);
                // if we get here, then curObject ocurrs on subShape after skip detail (so keep it)
                return false;
            }
            else
                // advance object, try again
                return checkSkip(meshNum, ++curObject, curDecal, skipDL);
        }
    }

    if (curDecal < decals.size())
    {
        S32 start = decals[curDecal].startMeshIndex;
        if (meshNum >= start)
        {
            // we are either from this decal, the next decal, or error
            if (meshNum < start + decals[curDecal].numMeshes)
            {
                // this object...
                if (subShapeFirstDecal[skipSS] > curDecal)
                    // haven't reached this subshape yet
                    return true;
                if (skipSS + 1 == subShapeFirstDecal.size() || curDecal < subShapeFirstDecal[skipSS + 1])
                    // curDecal is on subshape of skip detail...make sure it's after skipDL
                    return (meshNum - start < details[skipDL].objectDetailNum);
                // if we get here, then curDecal ocurrs on subShape after skip detail (so keep it)
                return false;
            }
            else
                // advance decal, try again
                return checkSkip(meshNum, curObject, ++curDecal, skipDL);
        }
    }
    AssertFatal(0, "TSShape::checkSkip: assertion failed");
    return false;
}

void TSShape::assembleShape()
{
    S32 i, j;

    // get counts...
    S32 numNodes = alloc.get32();
    S32 numObjects = alloc.get32();
    S32 numDecals = alloc.get32();
    S32 numSubShapes = alloc.get32();
    S32 numIflMaterials = alloc.get32();
    S32 numNodeRots;
    S32 numNodeTrans;
    S32 numNodeUniformScales;
    S32 numNodeAlignedScales;
    S32 numNodeArbitraryScales;
    if (smReadVersion < 22)
    {
        numNodeRots = numNodeTrans = alloc.get32() - numNodes;
        numNodeUniformScales = numNodeAlignedScales = numNodeArbitraryScales = 0;
    }
    else
    {
        numNodeRots = alloc.get32();
        numNodeTrans = alloc.get32();
        numNodeUniformScales = alloc.get32();
        numNodeAlignedScales = alloc.get32();
        numNodeArbitraryScales = alloc.get32();
    }
    S32 numGroundFrames = 0;
    if (smReadVersion > 23)
        numGroundFrames = alloc.get32();
    S32 numObjectStates = alloc.get32();
    S32 numDecalStates = alloc.get32();
    S32 numTriggers = alloc.get32();
    S32 numDetails = alloc.get32();
    S32 numMeshes = alloc.get32();
    S32 numSkins = 0;
    if (smReadVersion < 23)
        // in later versions, skins are kept with other meshes
        numSkins = alloc.get32();
    S32 numNames = alloc.get32();
    mSmallestVisibleSize = (F32)alloc.get32();
    mSmallestVisibleDL = alloc.get32();
    S32 skipDL = getMin(mSmallestVisibleDL, smNumSkipLoadDetails);

    alloc.checkGuard();

    // get bounds...
    alloc.get32((S32*)&radius, 1);
    alloc.get32((S32*)&tubeRadius, 1);
    alloc.get32((S32*)&center, 3);
    alloc.get32((S32*)&bounds, 6);

    alloc.checkGuard();

    // copy various vectors...
    S32* ptr32 = alloc.copyToShape32(numNodes * 5);
    nodes.set(ptr32, numNodes);

    alloc.checkGuard();

    ptr32 = alloc.copyToShape32(numObjects * 6, true);
    if (!ptr32)
        ptr32 = alloc.allocShape32(numSkins * 6); // pre v23 shapes store skins and meshes separately...no longer
    else
        alloc.allocShape32(numSkins * 6);
    objects.set(ptr32, numObjects);

    alloc.checkGuard();

    ptr32 = alloc.copyToShape32(numDecals * 5, true);
    decals.set(ptr32, numDecals);

    alloc.checkGuard();

    ptr32 = alloc.copyToShape32(numIflMaterials * 5);
    iflMaterials.set(ptr32, numIflMaterials);

    alloc.checkGuard();

    ptr32 = alloc.copyToShape32(numSubShapes, true);
    subShapeFirstNode.set(ptr32, numSubShapes);
    ptr32 = alloc.copyToShape32(numSubShapes, true);
    subShapeFirstObject.set(ptr32, numSubShapes);
    ptr32 = alloc.copyToShape32(numSubShapes, true);
    subShapeFirstDecal.set(ptr32, numSubShapes);

    alloc.checkGuard();

    ptr32 = alloc.copyToShape32(numSubShapes);
    subShapeNumNodes.set(ptr32, numSubShapes);
    ptr32 = alloc.copyToShape32(numSubShapes);
    subShapeNumObjects.set(ptr32, numSubShapes);
    ptr32 = alloc.copyToShape32(numSubShapes);
    subShapeNumDecals.set(ptr32, numSubShapes);

    alloc.checkGuard();

    ptr32 = alloc.allocShape32(numSubShapes);
    subShapeFirstTranslucentObject.set(ptr32, numSubShapes);

    // get meshIndexList...older shapes only
    S32 meshIndexListSize = 0;
    S32* meshIndexList = NULL;
    if (smReadVersion < 16)
    {
        meshIndexListSize = alloc.get32();
        meshIndexList = alloc.getPointer32(meshIndexListSize);
    }

    // get default translation and rotation
    S16* ptr16 = alloc.allocShape16(0);
    for (i = 0; i < numNodes; i++)
        alloc.copyToShape16(4);
    defaultRotations.set(ptr16, numNodes);
    alloc.align32();
    ptr32 = alloc.allocShape32(0);
    for (i = 0; i < numNodes; i++)
    {
        alloc.copyToShape32(3);
        alloc.copyToShape32(sizeof(Point3F) - 12); // handle alignment issues w/ point3f
    }
    defaultTranslations.set(ptr32, numNodes);

    // get any node sequence data stored in shape
    nodeTranslations.setSize(numNodeTrans);
    for (i = 0; i < numNodeTrans; i++)
        alloc.get32((S32*)&nodeTranslations[i], 3);
    nodeRotations.setSize(numNodeRots);
    for (i = 0; i < numNodeRots; i++)
        alloc.get16((S16*)&nodeRotations[i], 4);
    alloc.align32();

    alloc.checkGuard();

    if (smReadVersion > 21)
    {
        // more node sequence data...scale
        nodeUniformScales.setSize(numNodeUniformScales);
        for (i = 0; i < numNodeUniformScales; i++)
            alloc.get32((S32*)&nodeUniformScales[i], 1);
        nodeAlignedScales.setSize(numNodeAlignedScales);
        for (i = 0; i < numNodeAlignedScales; i++)
            alloc.get32((S32*)&nodeAlignedScales[i], 3);
        nodeArbitraryScaleFactors.setSize(numNodeArbitraryScales);
        for (i = 0; i < numNodeArbitraryScales; i++)
            alloc.get32((S32*)&nodeArbitraryScaleFactors[i], 3);
        nodeArbitraryScaleRots.setSize(numNodeArbitraryScales);
        for (i = 0; i < numNodeArbitraryScales; i++)
            alloc.get16((S16*)&nodeArbitraryScaleRots[i], 4);
        alloc.align32();

        alloc.checkGuard();
    }

    // old shapes need ground transforms moved to ground arrays...but only do it once
    if (smReadVersion < 22 && alloc.allocShape32(0))
    {
        for (i = 0; i < sequences.size(); i++)
        {
            // move ground transform data to ground vectors
            Sequence& seq = sequences[i];
            S32 oldSz = groundTranslations.size();
            groundTranslations.setSize(oldSz + seq.numGroundFrames);
            groundRotations.setSize(oldSz + seq.numGroundFrames);
            for (S32 j = 0; j < seq.numGroundFrames; j++)
            {
                groundTranslations[j + oldSz] = nodeTranslations[seq.firstGroundFrame + j - numNodes];
                groundRotations[j + oldSz] = nodeRotations[seq.firstGroundFrame + j - numNodes];
            }
            seq.firstGroundFrame = oldSz;
            seq.baseTranslation -= numNodes;
            seq.baseRotation -= numNodes;
            seq.baseScale = 0; // not used on older shapes...but keep it clean
        }
    }

    // version 22 & 23 shapes accidentally had no ground transforms, and ground for
    // earlier shapes is handled just above, so...
    if (smReadVersion > 23)
    {
        groundTranslations.setSize(numGroundFrames);
        for (i = 0; i < numGroundFrames; i++)
            alloc.get32((S32*)&groundTranslations[i], 3);
        groundRotations.setSize(numGroundFrames);
        for (i = 0; i < numGroundFrames; i++)
            alloc.get16((S16*)&groundRotations[i], 4);
        alloc.align32();

        alloc.checkGuard();
    }

    // object states
    ptr32 = alloc.copyToShape32(numObjectStates * 3);
    objectStates.set(ptr32, numObjectStates);
    alloc.allocShape32(numSkins * 3); // provide buffer after objectStates for older shapes

    alloc.checkGuard();

    // decal states
    ptr32 = alloc.copyToShape32(numDecalStates);
    decalStates.set(ptr32, numDecalStates);

    alloc.checkGuard();

    // frame triggers
    ptr32 = alloc.getPointer32(numTriggers * 2);
    triggers.setSize(numTriggers);
    dMemcpy(triggers.address(), ptr32, sizeof(S32) * numTriggers * 2);

    alloc.checkGuard();

    // details
    ptr32 = alloc.copyToShape32(numDetails * 7, true);
    details.set(ptr32, numDetails);

    alloc.checkGuard();

    // about to read in the meshes...first must allocate some scratch space
    S32 scratchSize = getMax(numSkins, numMeshes);
    TSMesh::smVertsList.setSize(scratchSize);
    TSMesh::smTVertsList.setSize(scratchSize);
    TSMesh::smNormsList.setSize(scratchSize);
    TSMesh::smEncodedNormsList.setSize(scratchSize);
    TSMesh::smDataCopied.setSize(scratchSize);
    TSSkinMesh::smInitTransformList.setSize(scratchSize);
    TSSkinMesh::smVertexIndexList.setSize(scratchSize);
    TSSkinMesh::smBoneIndexList.setSize(scratchSize);
    TSSkinMesh::smWeightList.setSize(scratchSize);
    TSSkinMesh::smNodeIndexList.setSize(scratchSize);
    for (i = 0; i < numMeshes; i++)
    {
        TSMesh::smVertsList[i] = NULL;
        TSMesh::smTVertsList[i] = NULL;
        TSMesh::smNormsList[i] = NULL;
        TSMesh::smEncodedNormsList[i] = NULL;
        TSMesh::smDataCopied[i] = false;
        TSSkinMesh::smInitTransformList[i] = NULL;
        TSSkinMesh::smVertexIndexList[i] = NULL;
        TSSkinMesh::smBoneIndexList[i] = NULL;
        TSSkinMesh::smWeightList[i] = NULL;
        TSSkinMesh::smNodeIndexList[i] = NULL;
    }

    // read in the meshes (sans skins)...
    if (smReadVersion > 15)
    {
        // straight forward read one at a time
        TSMesh **ptrmesh = (TSMesh**)alloc.allocShape32((numMeshes + numSkins*numDetails) * (sizeof(TSMesh*) / 4));
        S32 curObject = 0, curDecal = 0; // for tracking skipped meshes
        for (i = 0; i < numMeshes; i++)
        {
            bool skip = checkSkip(i, curObject, curDecal, skipDL); // skip this mesh?
            S32 meshType = alloc.get32();
            TSMesh* mesh = TSMesh::assembleMesh(meshType, skip);
            if (ptrmesh)
                ptrmesh[i] = skip ? 0 : mesh;

            // fill in location of verts, tverts, and normals for detail levels
            if (mesh && meshType != TSMesh::DecalMeshType)
            {
                TSMesh::smVertsList[i] = mesh->verts.address();
                TSMesh::smTVertsList[i] = mesh->tverts.address();
                TSMesh::smNormsList[i] = mesh->norms.address();
                TSMesh::smEncodedNormsList[i] = mesh->encodedNorms.address();
                TSMesh::smDataCopied[i] = !skip; // as long as we didn't skip this mesh, the data should be in shape now
                if (meshType == TSMesh::SkinMeshType)
                {
                    TSSkinMesh* skin = (TSSkinMesh*)mesh;
                    TSMesh::smVertsList[i] = skin->initialVerts.address();
                    TSMesh::smNormsList[i] = skin->initialNorms.address();
                    TSSkinMesh::smInitTransformList[i] = skin->initialTransforms.address();
                    TSSkinMesh::smVertexIndexList[i] = skin->vertexIndex.address();
                    TSSkinMesh::smBoneIndexList[i] = skin->boneIndex.address();
                    TSSkinMesh::smWeightList[i] = skin->weight.address();
                    TSSkinMesh::smNodeIndexList[i] = skin->nodeIndex.address();
                }
            }
        }
        meshes.set(ptrmesh, numMeshes);
    }
    else
    {
        // use meshIndexList to contruct mesh list...
        TSMesh **ptrmesh = (TSMesh**)alloc.allocShape32((numMeshes + numSkins*numDetails) * (sizeof(TSMesh*) / 4));
        S32 next = 0;
        S32 curObject = 0, curDecal = 0; // for tracking skipped meshes
        for (i = 0; i < meshIndexListSize; i++)
        {
            bool skip = checkSkip(i, curObject, curDecal, skipDL);
            if (meshIndexList[i] >= 0)
            {
                AssertFatal(meshIndexList[i] == next, "TSShape::read: assertion failed on obsolete shape");
                S32 meshType = alloc.get32();
                TSMesh* mesh = TSMesh::assembleMesh(meshType, skip);
                if (ptrmesh)
                    ptrmesh[i] = skip ? 0 : mesh;
                next = meshIndexList[i] + 1;

                // fill in location of verts, tverts, and normals for detail levels
                if (mesh && meshType != TSMesh::DecalMeshType)
                {
                    TSMesh::smVertsList[i] = mesh->verts.address();
                    TSMesh::smTVertsList[i] = mesh->tverts.address();
                    TSMesh::smNormsList[i] = mesh->norms.address();
                    TSMesh::smEncodedNormsList[i] = mesh->encodedNorms.address();
                    TSMesh::smDataCopied[i] = !skip; // as long as we didn't skip this mesh, the data should be in shape now
                    if (meshType == TSMesh::SkinMeshType)
                    {
                        TSSkinMesh* skin = (TSSkinMesh*)mesh;
                        TSMesh::smVertsList[i] = skin->initialVerts.address();
                        TSMesh::smNormsList[i] = skin->initialNorms.address();
                        TSSkinMesh::smInitTransformList[i] = skin->initialTransforms.address();
                        TSSkinMesh::smVertexIndexList[i] = skin->vertexIndex.address();
                        TSSkinMesh::smBoneIndexList[i] = skin->boneIndex.address();
                        TSSkinMesh::smWeightList[i] = skin->weight.address();
                        TSSkinMesh::smNodeIndexList[i] = skin->nodeIndex.address();
                    }
                }
            }
            else if (ptrmesh)
                ptrmesh[i] = 0; // no mesh
        }
        meshes.set(ptrmesh, meshIndexListSize);
    }

    alloc.checkGuard();

    // names
    bool namesInStringTable = (StringTable != NULL);
    char* nameBufferStart = (char*)alloc.getPointer8(0);
    char* name = nameBufferStart;
    S32 nameBufferSize = 0;
    names.setSize(numNames);
    for (i = 0; i < numNames; i++)
    {
        for (j = 0; name[j]; j++)
            ;
        if (namesInStringTable)
            names[i] = StringTable->insert(name, false);
        nameBufferSize += j + 1;
        name += j + 1;
    }
    if (!namesInStringTable)
    {
        name = (char*)alloc.copyToShape8(nameBufferSize);
        if (name) // make sure we did copy (might just be getting size of buffer)
            for (i = 0; i < numNames; i++)
            {
                for (j = 0; name[j]; j++)
                    ;
                names[i] = name;
                name += j + 1;
            }
    }
    else
        alloc.getPointer8(nameBufferSize);
    alloc.align32();

    alloc.checkGuard();

    if (smReadVersion < 23)
    {
        // get detail information about skins...
        S32* detailFirstSkin = alloc.getPointer32(numDetails);
        S32* detailNumSkins = alloc.getPointer32(numDetails);

        alloc.checkGuard();

        // about to read in skins...clear out scratch space...
        if (numSkins)
        {
            TSSkinMesh::smInitTransformList.setSize(numSkins);
            TSSkinMesh::smVertexIndexList.setSize(numSkins);
            TSSkinMesh::smBoneIndexList.setSize(numSkins);
            TSSkinMesh::smWeightList.setSize(numSkins);
            TSSkinMesh::smNodeIndexList.setSize(numSkins);
        }
        for (i = 0; i < numSkins; i++)
        {
            TSMesh::smVertsList[i] = NULL;
            TSMesh::smTVertsList[i] = NULL;
            TSMesh::smNormsList[i] = NULL;
            TSMesh::smEncodedNormsList[i] = NULL;
            TSMesh::smDataCopied[i] = false;
            TSSkinMesh::smInitTransformList[i] = NULL;
            TSSkinMesh::smVertexIndexList[i] = NULL;
            TSSkinMesh::smBoneIndexList[i] = NULL;
            TSSkinMesh::smWeightList[i] = NULL;
            TSSkinMesh::smNodeIndexList[i] = NULL;
        }

        // skins
        ptr32 = alloc.allocShape32(numSkins);
        for (i = 0; i < numSkins; i++)
        {
            bool skip = i < detailFirstSkin[skipDL];
            TSSkinMesh* skin = (TSSkinMesh*)TSMesh::assembleMesh(TSMesh::SkinMeshType, skip);
            if (meshes.address())
            {
                // add pointer to skin in shapes list of meshes
                // we reserved room for this above...
                meshes.set(meshes.address(), meshes.size() + 1);
                meshes[meshes.size() - 1] = skip ? NULL : skin;
            }

            // fill in location of verts, tverts, and normals for shared detail levels
            if (skin)
            {
                TSMesh::smVertsList[i] = skin->initialVerts.address();
                TSMesh::smTVertsList[i] = skin->tverts.address();
                TSMesh::smNormsList[i] = skin->initialNorms.address();
                TSMesh::smEncodedNormsList[i] = skin->encodedNorms.address();
                TSMesh::smDataCopied[i] = !skip; // as long as we didn't skip this mesh, the data should be in shape now
                TSSkinMesh::smInitTransformList[i] = skin->initialTransforms.address();
                TSSkinMesh::smVertexIndexList[i] = skin->vertexIndex.address();
                TSSkinMesh::smBoneIndexList[i] = skin->boneIndex.address();
                TSSkinMesh::smWeightList[i] = skin->weight.address();
                TSSkinMesh::smNodeIndexList[i] = skin->nodeIndex.address();
            }
        }

        alloc.checkGuard();

        // we now have skins in mesh list...add skin objects to object list and patch things up
        fixupOldSkins(numMeshes, numSkins, numDetails, detailFirstSkin, detailNumSkins);
    }

    // allocate storage space for some arrays (filled in during Shape::init)...
    ptr32 = alloc.allocShape32(numDetails);
    alphaIn.set(ptr32, numDetails);
    ptr32 = alloc.allocShape32(numDetails);
    alphaOut.set(ptr32, numDetails);

    mPreviousMerge.setSize(numObjects);
    for (i = 0; i < numObjects; ++i)
        mPreviousMerge[i] = -1;

    mExportMerge = smReadVersion >= 23;
}

void TSShape::disassembleShape()
{
    S32 i;

    // set counts...
    S32 numNodes = alloc.set32(nodes.size());
    S32 numObjects = alloc.set32(objects.size());
    S32 numDecals = alloc.set32(decals.size());
    S32 numSubShapes = alloc.set32(subShapeFirstNode.size());
    S32 numIflMaterials = alloc.set32(iflMaterials.size());
    S32 numNodeRotations = alloc.set32(nodeRotations.size());
    S32 numNodeTranslations = alloc.set32(nodeTranslations.size());
    S32 numNodeUniformScales = alloc.set32(nodeUniformScales.size());
    S32 numNodeAlignedScales = alloc.set32(nodeAlignedScales.size());
    S32 numNodeArbitraryScales = alloc.set32(nodeArbitraryScaleFactors.size());
    S32 numGroundFrames = alloc.set32(groundTranslations.size());
    S32 numObjectStates = alloc.set32(objectStates.size());
    S32 numDecalStates = alloc.set32(decalStates.size());
    S32 numTriggers = alloc.set32(triggers.size());
    S32 numDetails = alloc.set32(details.size());
    S32 numMeshes = alloc.set32(meshes.size());
    S32 numNames = alloc.set32(names.size());
    alloc.set32((S32)mSmallestVisibleSize);
    alloc.set32(mSmallestVisibleDL);

    alloc.setGuard();

    // get bounds...
    alloc.copyToBuffer32((S32*)&radius, 1);
    alloc.copyToBuffer32((S32*)&tubeRadius, 1);
    alloc.copyToBuffer32((S32*)&center, 3);
    alloc.copyToBuffer32((S32*)&bounds, 6);

    alloc.setGuard();

    // copy various vectors...
    alloc.copyToBuffer32((S32*)nodes.address(), numNodes * 5);
    alloc.setGuard();
    alloc.copyToBuffer32((S32*)objects.address(), numObjects * 6);
    alloc.setGuard();
    alloc.copyToBuffer32((S32*)decals.address(), numDecals * 5);
    alloc.setGuard();
    alloc.copyToBuffer32((S32*)iflMaterials.address(), numIflMaterials * 5);
    alloc.setGuard();
    alloc.copyToBuffer32((S32*)subShapeFirstNode.address(), numSubShapes);
    alloc.copyToBuffer32((S32*)subShapeFirstObject.address(), numSubShapes);
    if (subShapeFirstDecal.address() == NULL) {
        subShapeFirstDecal.push_back(0);
    }
    alloc.copyToBuffer32((S32*)subShapeFirstDecal.address(), numSubShapes);
    alloc.setGuard();
    alloc.copyToBuffer32((S32*)subShapeNumNodes.address(), numSubShapes);
    alloc.copyToBuffer32((S32*)subShapeNumObjects.address(), numSubShapes);
    if (subShapeNumDecals.address() == NULL) {
        subShapeNumDecals.push_back(0);
    }
    alloc.copyToBuffer32((S32*)subShapeNumDecals.address(), numSubShapes);
    alloc.setGuard();

    // default transforms...
    alloc.copyToBuffer16((S16*)defaultRotations.address(), numNodes * 4);
    alloc.copyToBuffer32((S32*)defaultTranslations.address(), numNodes * 3);

    // animated transforms...
    alloc.copyToBuffer16((S16*)nodeRotations.address(), numNodeRotations * 4);
    alloc.copyToBuffer32((S32*)nodeTranslations.address(), numNodeTranslations * 3);

    alloc.setGuard();

    // ...with scale
    alloc.copyToBuffer32((S32*)nodeUniformScales.address(), numNodeUniformScales);
    alloc.copyToBuffer32((S32*)nodeAlignedScales.address(), numNodeAlignedScales * 3);
    alloc.copyToBuffer32((S32*)nodeArbitraryScaleFactors.address(), numNodeArbitraryScales * 3);
    alloc.copyToBuffer16((S16*)nodeArbitraryScaleRots.address(), numNodeArbitraryScales * 4);

    alloc.setGuard();

    alloc.copyToBuffer32((S32*)groundTranslations.address(), 3 * numGroundFrames);
    alloc.copyToBuffer16((S16*)groundRotations.address(), 4 * numGroundFrames);

    alloc.setGuard();

    // object states..
    alloc.copyToBuffer32((S32*)objectStates.address(), numObjectStates * 3);
    alloc.setGuard();

    // decal states...
    alloc.copyToBuffer32((S32*)decalStates.address(), numDecalStates);
    alloc.setGuard();

    // frame triggers
    alloc.copyToBuffer32((S32*)triggers.address(), numTriggers * 2);
    alloc.setGuard();

    // details
    alloc.copyToBuffer32((S32*)details.address(), numDetails * 7);
    alloc.setGuard();

    // read in the meshes (sans skins)...
    bool* isMesh = new bool[numMeshes]; // funny business because decals are pretend meshes (legacy issue)
    for (i = 0; i < numMeshes; i++)
        isMesh[i] = false;
    for (i = 0; i < objects.size(); i++)
    {
        for (S32 j = 0; j < objects[i].numMeshes; j++)
            // even if an empty mesh, it's a mesh...
            isMesh[objects[i].startMeshIndex + j] = true;
    }
    for (i = 0; i < numMeshes; i++)
    {
        TSMesh* mesh = NULL;
        TSDecalMesh* decalMesh = NULL;
        if (isMesh[i])
            mesh = meshes[i];
        else
            decalMesh = (TSDecalMesh*)meshes[i];
        alloc.set32(mesh ? mesh->getMeshType() : (decalMesh ? TSMesh::DecalMeshType : TSMesh::NullMeshType));
        if (mesh)
            mesh->disassemble();
        else if (decalMesh)
            decalMesh->disassemble();
    }
    delete[] isMesh;
    alloc.setGuard();

    // names
    for (i = 0; i < numNames; i++)
        alloc.copyToBuffer8((S8*)names[i], dStrlen(names[i]) + 1);
    alloc.setGuard();
}

//-------------------------------------------------
// write whole shape
//-------------------------------------------------
void TSShape::write(Stream* s)
{
    // write version
    s->write(smVersion | (mExporterVersion << 16));

    alloc.setWrite();
    disassembleShape();

    S32* buffer32 = alloc.getBuffer32();
    S16* buffer16 = alloc.getBuffer16();
    S8* buffer8 = alloc.getBuffer8();

    S32 size32 = alloc.getBufferSize32();
    S32 size16 = alloc.getBufferSize16();
    S32 size8 = alloc.getBufferSize8();

    // convert sizes to dwords...
    if (size16 & 1)
        size16 += 2;
    size16 >>= 1;
    if (size8 & 3)
        size8 += 4;
    size8 >>= 2;

    S32 sizeMemBuffer, start16, start8;
    sizeMemBuffer = size32 + size16 + size8;
    start16 = size32;
    start8 = start16 + size16;

    // in dwords -- write will properly endian-flip.
    s->write(sizeMemBuffer);
    s->write(start16);
    s->write(start8);

    // endian-flip the entire write buffers.
    fixEndian(buffer32, buffer16, buffer8, size32, size16, size8);

    // now write buffers
    s->write(size32 * 4, buffer32);
    s->write(size16 * 4, buffer16);
    s->write(size8 * 4, buffer8);

    // write sequences - write will properly endian-flip.
    s->write(sequences.size());
    for (S32 i = 0; i < sequences.size(); i++)
        sequences[i].write(s);

    // write material list - write will properly endian-flip.
    materialList->write(*s);

    delete[] buffer32;
    delete[] buffer16;
    delete[] buffer8;
}

//-------------------------------------------------
// read whole shape
//-------------------------------------------------

bool TSShape::read(Stream* s)
{
    // read version - read handles endian-flip
    s->read(&smReadVersion);
    mExporterVersion = smReadVersion >> 16;
    smReadVersion &= 0xFF;
    if (smReadVersion > smVersion)
    {
        // error -- don't support future versions yet :>
        Con::errorf(ConsoleLogEntry::General,
            "Error: attempt to load a version %i dts-shape, can currently only load version %i and before.",
            smReadVersion, smVersion);
        return false;
    }
    mReadVersion = smReadVersion;

    S32* memBuffer32;
    S16* memBuffer16;
    S8* memBuffer8;
    S32 count32, count16, count8;
    if (mReadVersion < 19)
    {
        Con::printf("... Shape with old version.");
        readOldShape(s, memBuffer32, memBuffer16, memBuffer8, count32, count16, count8);
    }
    else
    {
        S32 i;
        U32 sizeMemBuffer, startU16, startU8;

        // in dwords. - read handles endian-flip
        s->read(&sizeMemBuffer);
        s->read(&startU16);
        s->read(&startU8);

        if (s->getStatus() != Stream::Ok)
        {
            Con::errorf(ConsoleLogEntry::General, "Error: bad shape file.");
            return false;
        }

        S32* tmp = new S32[sizeMemBuffer];
        s->read(sizeof(S32) * sizeMemBuffer, (U8*)tmp);
        memBuffer32 = tmp;
        memBuffer16 = (S16*)(tmp + startU16);
        memBuffer8 = (S8*)(tmp + startU8);

        count32 = startU16;
        count16 = startU8 - startU16;
        count8 = sizeMemBuffer - startU8;

        // read sequences
        S32 numSequences;
        s->read(&numSequences);
        sequences.setSize(numSequences);
        for (i = 0; i < numSequences; i++)
        {
            constructInPlace(&sequences[i]);
            sequences[i].read(s);
        }

        // read material list
        delete materialList; // just in case...
        materialList = new TSMaterialList;
        materialList->read(*s);
    }

    // since we read in the buffers, we need to endian-flip their entire contents...
    fixEndian(memBuffer32, memBuffer16, memBuffer8, count32, count16, count8);

    alloc.setRead(memBuffer32, memBuffer16, memBuffer8, true);
    assembleShape(); // determine size of buffer needed
    S32 buffSize = alloc.getSize();
    alloc.doAlloc();
    mMemoryBlock = alloc.getBuffer();
    alloc.setRead(memBuffer32, memBuffer16, memBuffer8, false);
    assembleShape(); // copy to buffer
    AssertFatal(alloc.getSize() == buffSize, "TSShape::read: shape data buffer size mis-calculated");

    if (smReadVersion < 19)
    {
        delete[] memBuffer32;
        delete[] memBuffer16;
        delete[] memBuffer8;
    }
    else
        delete[] memBuffer32; // this covers all the buffers

    if (smInitOnRead)
        init();
    return true;
}

void TSShape::fixEndian(S32* buff32, S16* buff16, S8*, S32 count32, S32 count16, S32)
{
    // if endian-ness isn't the same, need to flip the buffer contents.
    if (0x12345678 != convertLEndianToHost(0x12345678))
    {
        for (S32 i = 0; i < count32; i++)
            buff32[i] = convertLEndianToHost(buff32[i]);
        for (S32 i = 0; i < count16 * 2; i++)
            buff16[i] = convertLEndianToHost(buff16[i]);
    }
}

/** Read the .ifl material file
   The .ifl file (output by max) is essentially a keyframe list for
   the animation and contains the names of the textures along
   with a duration time.  This function parses the file to and appends
   the textures to the shape's master material list.
*/
void TSShape::readIflMaterials(const char* shapePath)
{

    if (mFlags & IflInit)
        return;

    for (S32 i = 0; i < iflMaterials.size(); i++)
    {
        IflMaterial& iflMaterial = iflMaterials[i];

        iflMaterial.firstFrame = materialList->size(); // next material added will be our first frame
        iflMaterial.firstFrameOffTimeIndex = iflFrameOffTimes.size(); // next entry added will be our first
        iflMaterial.numFrames = 0; // we'll increment as we read the file

        U32 totalTime = 0;

        // Fix slashes that face the wrong way
        char* pName = (char*)getName(iflMaterial.nameIndex);
        S32 len = dStrlen(pName);
        for (S32 j = 0; j < len; j++)
            if (pName[j] == '\\')
                pName[j] = '/';

        // Open the file which should be located in the same directory
        // as the .dts shape.
        // Tg: I cut out a some backwards compatibilty code dealing
        // with the old file prefixing. If someone is having compatibilty
        // problems, you may want to check the previous version of this
        // file.

        char nameBuffer[256];
        dSprintf(nameBuffer, sizeof(nameBuffer), "%s/%s", shapePath, pName);

        Stream* s = ResourceManager->openStream(nameBuffer);
        if (!s || s->getStatus() != Stream::Ok)
        {
            iflMaterial.firstFrame = iflMaterial.materialSlot; // avoid over-runs
            if (s)
                ResourceManager->closeStream(s);
            continue;
        }

        // Parse the file, creat ifl "key" frames and append
        // texture names to the shape's material list.
        char buffer[512];
        U32 duration;
        while (s->getStatus() == Stream::Ok)
        {
            s->readLine((U8*)buffer, 512);
            if (s->getStatus() == Stream::Ok || s->getStatus() == Stream::EOS)
            {
                char* pos = buffer;
                while (*pos)
                {
                    if (*pos == '\t')
                        *pos = ' ';
                    pos++;
                }
                pos = buffer;

                // Find the name and duration start points
                while (*pos && *pos == ' ')
                    pos++;
                char* pos2 = dStrchr(pos, ' ');
                if (pos2)
                {
                    *pos2 = '\0';
                    pos2++;
                    while (*pos2 && *pos2 == ' ')
                        pos2++;
                }

                // skip line with no material
                if (!*pos)
                    continue;

                // Extract frame duration
                duration = pos2 ? dAtoi(pos2) : 1;
                if (duration == 0)
                    // just in case...
                    duration = 1;

                // Tg: I cut out a some backwards compatibilty code dealing
                // with the old file prefixing. If someone is having compatibilty
                // problems, you may want to check the previous version of this
                // file.
                // Strip off texture extension first.
                if ((pos2 = dStrchr(pos, '.')) != 0)
                    *pos2 = '\0';
                materialList->push_back(pos, TSMaterialList::IflFrame);

                //
                totalTime += duration;
                iflFrameOffTimes.push_back((1.0f / 30.0f) * (F32)totalTime); // ifl units are frames (1 frame = 1/30 sec)
                iflMaterial.numFrames++;
            }
        }

        // close the file...
        ResourceManager->closeStream(s);
    }
    initMaterialList();
    mFlags |= IflInit;

}

ResourceInstance* constructTSShape(Stream& stream)
{
    TSShape* ret = new TSShape;
    if (!ret->read(&stream))
    {
        delete ret;
        ret = NULL;
    }

    return ret;
}

//ResourceInstance* constructColladaShape(Stream& stream)
//{
//    FileStream& fs = static_cast<FileStream&>(stream);
//    loadColladaShape(stream);
//    TSShape* ret = new TSShape;
//    if (!ret->read(&stream))
//    {
//        delete ret;
//        ret = NULL;
//    }
//
//    return ret;
//}

#if 1
TSShape::ConvexHullAccelerator* TSShape::getAccelerator(S32 dl)
{
    AssertFatal(dl < details.size(), "Error, bad detail level!");
    if (dl == -1)
        return NULL;

    if (detailCollisionAccelerators[dl] == NULL)
        computeAccelerator(dl);

    AssertFatal(detailCollisionAccelerators[dl] != NULL, "This should be non-null after computing it!");
    return detailCollisionAccelerators[dl];
}


void TSShape::computeAccelerator(S32 dl)
{
    AssertFatal(dl < details.size(), "Error, bad detail level!");

    // Have we already computed this?
    if (detailCollisionAccelerators[dl] != NULL)
        return;

    // Create a bogus features list...
    ConvexFeature cf;
    MatrixF mat(true);
    Point3F n(0, 0, 1);

    const TSDetail* detail = &details[dl];
    S32 ss = detail->subShapeNum;
    S32 od = detail->objectDetailNum;

    S32 start = subShapeFirstObject[ss];
    S32 end = subShapeNumObjects[ss] + start;
    if (start < end)
    {
        // run through objects and collide
        // DMMNOTE: This assumes that the transform of the collision hulls is
        //  identity...
        U32 surfaceKey = 0;
        for (S32 i = start; i < end; i++)
        {
            const TSObject* obj = &objects[i];

            if (obj->numMeshes && od < obj->numMeshes) {
                TSMesh* mesh = meshes[obj->startMeshIndex + od];
                if (mesh)
                    mesh->getFeatures(0, mat, n, &cf, surfaceKey);
            }
        }
    }

    Vector<Point3F> fixedVerts;
    VECTOR_SET_ASSOCIATION(fixedVerts);
    S32 i;
    for (i = 0; i < cf.mVertexList.size(); i++) {
        S32 j;
        bool found = false;
        for (j = 0; j < cf.mFaceList.size(); j++) {
            if (cf.mFaceList[j].vertex[0] == i ||
                cf.mFaceList[j].vertex[1] == i ||
                cf.mFaceList[j].vertex[2] == i) {
                found = true;
                break;
            }
        }
        if (!found)
            continue;

        found = false;
        for (j = 0; j < fixedVerts.size(); j++) {
            if (fixedVerts[j] == cf.mVertexList[i]) {
                found = true;
                break;
            }
        }
        if (found == true) {
            // Ok, need to replace any references to vertex i in the facelists with
            //  a reference to vertex j in the fixed list
            for (S32 k = 0; k < cf.mFaceList.size(); k++) {
                for (S32 l = 0; l < 3; l++) {
                    if (cf.mFaceList[k].vertex[l] == i)
                        cf.mFaceList[k].vertex[l] = j;
                }
            }
        }
        else {
            for (S32 k = 0; k < cf.mFaceList.size(); k++) {
                for (S32 l = 0; l < 3; l++) {
                    if (cf.mFaceList[k].vertex[l] == i)
                        cf.mFaceList[k].vertex[l] = fixedVerts.size();
                }
            }
            fixedVerts.push_back(cf.mVertexList[i]);
        }
    }
    cf.mVertexList.setSize(0);
    cf.mVertexList = fixedVerts;

    // Ok, so now we have a vertex list.  Lets copy that out...
    ConvexHullAccelerator* accel = new ConvexHullAccelerator;
    detailCollisionAccelerators[dl] = accel;
    accel->numVerts = cf.mVertexList.size();
    accel->vertexList = new Point3F[accel->numVerts];
    dMemcpy(accel->vertexList, cf.mVertexList.address(), sizeof(Point3F) * accel->numVerts);

    accel->normalList = new PlaneF[cf.mFaceList.size()];
    for (i = 0; i < cf.mFaceList.size(); i++)
        accel->normalList[i] = cf.mFaceList[i].normal;

    accel->emitStrings = new U8 * [accel->numVerts];
    dMemset(accel->emitStrings, 0, sizeof(U8*) * accel->numVerts);

    for (i = 0; i < accel->numVerts; i++) {
        S32 j;

        Vector<U32> faces;
        VECTOR_SET_ASSOCIATION(faces);
        for (j = 0; j < cf.mFaceList.size(); j++) {
            if (cf.mFaceList[j].vertex[0] == i ||
                cf.mFaceList[j].vertex[1] == i ||
                cf.mFaceList[j].vertex[2] == i) {
                faces.push_back(j);
            }
        }
        AssertFatal(faces.size() != 0, "Huh?  Vertex unreferenced by any faces");

        // Insert all faces that didn't make the first cut, but share a plane with
        //  a face that's on the short list.
        for (j = 0; j < cf.mFaceList.size(); j++) {
            bool found = false;
            S32 k;
            for (k = 0; k < faces.size(); k++) {
                if (faces[k] == j)
                    found = true;
            }
            if (found)
                continue;

            found = false;
            for (k = 0; k < faces.size(); k++) {
                if (mDot(accel->normalList[faces[k]], accel->normalList[j]) > 0.999) {
                    found = true;
                    break;
                }
            }
            if (found)
                faces.push_back(j);
        }

        Vector<U32> vertRemaps;
        VECTOR_SET_ASSOCIATION(vertRemaps);
        for (j = 0; j < faces.size(); j++) {
            for (U32 k = 0; k < 3; k++) {
                U32 insert = cf.mFaceList[faces[j]].vertex[k];
                bool found = false;
                for (S32 l = 0; l < vertRemaps.size(); l++) {
                    if (insert == vertRemaps[l]) {
                        found = true;
                        break;
                    }
                }
                if (!found)
                    vertRemaps.push_back(insert);
            }
        }

        Vector<Point2I> edges;
        VECTOR_SET_ASSOCIATION(edges);
        for (j = 0; j < faces.size(); j++) {
            for (U32 k = 0; k < 3; k++) {
                U32 edgeStart = cf.mFaceList[faces[j]].vertex[(k + 0) % 3];
                U32 edgeEnd = cf.mFaceList[faces[j]].vertex[(k + 1) % 3];

                U32 e0 = getMin(edgeStart, edgeEnd);
                U32 e1 = getMax(edgeStart, edgeEnd);

                bool found = false;
                for (S32 l = 0; l < edges.size(); l++) {
                    if (edges[l].x == e0 && edges[l].y == e1) {
                        found = true;
                        break;
                    }
                }
                if (!found)
                    edges.push_back(Point2I(e0, e1));
            }
        }

        AssertFatal(vertRemaps.size() < 256 && faces.size() < 256 && edges.size() < 256,
            "Error, ran over the shapebase assumptions about convex hulls.");

        U32 emitStringLen = 1 + vertRemaps.size() +
            1 + (edges.size() * 2) +
            1 + (faces.size() * 4);
        accel->emitStrings[i] = new U8[emitStringLen];

        U32 currPos = 0;

        accel->emitStrings[i][currPos++] = vertRemaps.size();
        for (j = 0; j < vertRemaps.size(); j++)
            accel->emitStrings[i][currPos++] = vertRemaps[j];

        accel->emitStrings[i][currPos++] = edges.size();
        for (j = 0; j < edges.size(); j++) {
            S32 l;
            U32 old = edges[j].x;
            bool found = false;
            for (l = 0; l < vertRemaps.size(); l++) {
                if (vertRemaps[l] == old) {
                    found = true;
                    accel->emitStrings[i][currPos++] = l;
                    break;
                }
            }
            AssertFatal(found, "Error, couldn't find the remap!");

            old = edges[j].y;
            found = false;
            for (l = 0; l < vertRemaps.size(); l++) {
                if (vertRemaps[l] == old) {
                    found = true;
                    accel->emitStrings[i][currPos++] = l;
                    break;
                }
            }
            AssertFatal(found, "Error, couldn't find the remap!");
        }

        accel->emitStrings[i][currPos++] = faces.size();
        for (j = 0; j < faces.size(); j++) {
            accel->emitStrings[i][currPos++] = faces[j];
            for (U32 k = 0; k < 3; k++) {
                U32 old = cf.mFaceList[faces[j]].vertex[k];
                bool found = false;
                for (S32 l = 0; l < vertRemaps.size(); l++) {
                    if (vertRemaps[l] == old) {
                        found = true;
                        accel->emitStrings[i][currPos++] = l;
                        break;
                    }
                }
                AssertFatal(found, "Error, couldn't find the remap!");
            }
        }
        AssertFatal(currPos == emitStringLen, "Error, over/underflowed the emission string!");
    }
}
#endif

