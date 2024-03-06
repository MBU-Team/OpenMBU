//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "ts/tsShapeInstance.h"
#include "sim/sceneObject.h"

#include "Opcode.h"
#include "Ice/IceAABB.h"
#include "Ice/IcePoint.h"
#include "OPC_AABBTree.h"
#include "OPC_AABBCollider.h"

static bool gOpcodeInitialized = false;

//-------------------------------------------------------------------------------------
// Collision methods
//-------------------------------------------------------------------------------------

bool TSShapeInstance::buildPolyList(AbstractPolyList* polyList, S32 dl)
{
    // if dl==-1, nothing to do
    if (dl == -1)
        return false;

    AssertFatal(dl >= 0 && dl < mShape->details.size(), "TSShapeInstance::buildPolyList");

    // get subshape and object detail
    const TSDetail* detail = &mShape->details[dl];
    S32 ss = detail->subShapeNum;
    S32 od = detail->objectDetailNum;

    // set up static data
    setStatics(dl);

    // nothing emitted yet...
    bool emitted = false;
    U32 surfaceKey = 0;

    S32 start = mShape->subShapeFirstObject[ss];
    S32 end = mShape->subShapeNumObjects[ss] + start;
    if (start < end)
    {
        MatrixF initialMat;
        Point3F initialScale;
        polyList->getTransform(&initialMat, &initialScale);

        // set up for first object's node
        MatrixF mat;
        MatrixF scaleMat(true);
        F32* p = scaleMat;
        p[0] = initialScale.x;
        p[5] = initialScale.y;
        p[10] = initialScale.z;
        MatrixF* previousMat = mMeshObjects[start].getTransform();
        mat.mul(initialMat, scaleMat);
        mat.mul(*previousMat);
        polyList->setTransform(&mat, Point3F(1, 1, 1));

        // run through objects and collide
        for (S32 i = start; i < end; i++)
        {
            MeshObjectInstance* mesh = &mMeshObjects[i];

            if (od >= mesh->object->numMeshes)
                continue;

            if (mesh->getTransform() != previousMat)
            {
                // different node from before, set up for this node
                previousMat = mesh->getTransform();

                if (previousMat != NULL)
                {
                    mat.mul(initialMat, scaleMat);
                    mat.mul(*previousMat);
                    polyList->setTransform(&mat, Point3F(1, 1, 1));
                }
            }
            // collide...
            emitted |= mesh->buildPolyList(od, polyList, surfaceKey);
        }

        // restore original transform...
        polyList->setTransform(&initialMat, initialScale);
    }

    clearStatics();

    return emitted;
}

bool TSShapeInstance::getFeatures(const MatrixF& mat, const Point3F& n, ConvexFeature* cf, S32 dl)
{
    // if dl==-1, nothing to do
    if (dl == -1)
        return false;

    AssertFatal(dl >= 0 && dl < mShape->details.size(), "TSShapeInstance::buildPolyList");

    // get subshape and object detail
    const TSDetail* detail = &mShape->details[dl];
    S32 ss = detail->subShapeNum;
    S32 od = detail->objectDetailNum;

    // set up static data
    setStatics(dl);

    // nothing emitted yet...
    bool emitted = false;
    U32 surfaceKey = 0;

    S32 start = mShape->subShapeFirstObject[ss];
    S32 end = mShape->subShapeNumObjects[ss] + start;
    if (start < end)
    {
        MatrixF final;
        MatrixF* previousMat = mMeshObjects[start].getTransform();
        final.mul(mat, *previousMat);

        // run through objects and collide
        for (S32 i = start; i < end; i++)
        {
            MeshObjectInstance* mesh = &mMeshObjects[i];

            if (od >= mesh->object->numMeshes)
                continue;

            if (mesh->getTransform() != previousMat)
            {
                previousMat = mesh->getTransform();
                final.mul(mat, *previousMat);
            }
            emitted |= mesh->getFeatures(od, final, n, cf, surfaceKey);
        }
    }

    clearStatics();

    return emitted;
}

bool TSShapeInstance::castRay(const Point3F& a, const Point3F& b, RayInfo* rayInfo, S32 dl)
{
    // if dl==-1, nothing to do
    if (dl == -1)
        return false;

    AssertFatal(dl >= 0 && dl < mShape->details.size(), "TSShapeInstance::buildPolyList");

    // get subshape and object detail
    const TSDetail* detail = &mShape->details[dl];
    S32 ss = detail->subShapeNum;
    S32 od = detail->objectDetailNum;

    // set up static data
    setStatics(dl);

    S32 start = mShape->subShapeFirstObject[ss];
    S32 end = mShape->subShapeNumObjects[ss] + start;
    RayInfo saveRay;
    saveRay.t = 1.0f;
    const MatrixF* saveMat = NULL;
    bool found = false;
    if (start < end)
    {
        Point3F ta, tb;

        // set up for first object's node
        MatrixF mat;
        MatrixF* previousMat = mMeshObjects[start].getTransform();
        mat = *previousMat;
        mat.inverse();
        mat.mulP(a, &ta);
        mat.mulP(b, &tb);

        // run through objects and collide
        for (S32 i = start; i < end; i++)
        {
            MeshObjectInstance* mesh = &mMeshObjects[i];

            if (od >= mesh->object->numMeshes)
                continue;

            if (mesh->getTransform() != previousMat)
            {
                // different node from before, set up for this node
                previousMat = mesh->getTransform();

                if (previousMat != NULL)
                {
                    mat = *previousMat;
                    mat.inverse();
                    mat.mulP(a, &ta);
                    mat.mulP(b, &tb);
                }
            }
            // collide...
            if (mesh->castRay(od, ta, tb, rayInfo))
            {
                if (!rayInfo)
                {
                    clearStatics();
                    return true;
                }
                if (rayInfo->t <= saveRay.t)
                {
                    saveRay = *rayInfo;
                    saveMat = previousMat;
                }
                found = true;
            }
        }
    }

    // collide with any skins for this detail level...
    // TODO: if ever...

    // finalize the deal...
    if (found)
    {
        *rayInfo = saveRay;
        saveMat->mulV(rayInfo->normal);
        rayInfo->point = b - a;
        rayInfo->point *= rayInfo->t;
        rayInfo->point += a;
    }
    clearStatics();
    return found;
}

Point3F TSShapeInstance::support(const Point3F& v, S32 dl)
{
    // if dl==-1, nothing to do
    AssertFatal(dl != -1, "Error, should never try to collide with a non-existant detail level!");
    AssertFatal(dl >= 0 && dl < mShape->details.size(), "TSShapeInstance::support");

    // get subshape and object detail
    const TSDetail* detail = &mShape->details[dl];
    S32 ss = detail->subShapeNum;
    S32 od = detail->objectDetailNum;

    // set up static data
    setStatics(dl);

    S32 start = mShape->subShapeFirstObject[ss];
    S32 end = mShape->subShapeNumObjects[ss] + start;
    const MatrixF* saveMat = NULL;
    bool found = false;

    F32     currMaxDP = -1e9f;
    Point3F currSupport = Point3F(0, 0, 0);
    MatrixF* previousMat = NULL;
    MatrixF mat;
    if (start < end)
    {
        Point3F va;

        // set up for first object's node
        previousMat = mMeshObjects[start].getTransform();
        mat = *previousMat;
        mat.inverse();

        // run through objects and collide
        for (S32 i = start; i < end; i++)
        {
            MeshObjectInstance* mesh = &mMeshObjects[i];

            if (od >= mesh->object->numMeshes)
                continue;

            TSMesh* physMesh = mesh->getMesh(od);
            if (physMesh && mesh->visible > 0.01f)
            {
                // collide...
                F32 saveMDP = currMaxDP;

                if (mesh->getTransform() != previousMat)
                {
                    // different node from before, set up for this node
                    previousMat = mesh->getTransform();
                    mat = *previousMat;
                    mat.inverse();
                }
                mat.mulV(v, &va);
                physMesh->support(mesh->frame, va, &currMaxDP, &currSupport);
            }
        }
    }

    clearStatics();

    if (currMaxDP != -1e9f)
    {
        previousMat->mulP(currSupport);
        return currSupport;
    }
    else
    {
        return Point3F(0, 0, 0);
    }
}

void TSShapeInstance::computeBounds(S32 dl, Box3F& bounds)
{
    // if dl==-1, nothing to do
    if (dl == -1)
        return;

    AssertFatal(dl >= 0 && dl < mShape->details.size(), "TSShapeInstance::computeBounds");

    // get subshape and object detail
    const TSDetail* detail = &mShape->details[dl];
    S32 ss = detail->subShapeNum;
    S32 od = detail->objectDetailNum;

    // set up static data
    setStatics(dl);

    S32 start = mShape->subShapeFirstObject[ss];
    S32 end = mShape->subShapeNumObjects[ss] + start;

    // run through objects and updating bounds as we go
    bounds.min.set(10E30f, 10E30f, 10E30f);
    bounds.max.set(-10E30f, -10E30f, -10E30f);
    Box3F box;
    for (S32 i = start; i < end; i++)
    {
        MeshObjectInstance* mesh = &mMeshObjects[i];

        if (od >= mesh->object->numMeshes)
            continue;

        if (mesh->getMesh(od))
        {
            mesh->getMesh(od)->computeBounds(*mesh->getTransform(), box);
            bounds.min.setMin(box.min);
            bounds.max.setMax(box.max);
        }
    }

    clearStatics();
}

//-------------------------------------------------------------------------------------
// Object (MeshObjectInstance & PluginObjectInstance) collision methods
//-------------------------------------------------------------------------------------

bool TSShapeInstance::ObjectInstance::buildPolyList(S32 objectDetail, AbstractPolyList* polyList, U32& surfaceKey)
{
    objectDetail, polyList, surfaceKey;
    AssertFatal(0, "TSShapeInstance::ObjectInstance::buildPolyList:  no default method.");
    return false;
}

bool TSShapeInstance::ObjectInstance::getFeatures(S32 objectDetail, const MatrixF& mat, const Point3F& n, ConvexFeature* cf, U32& surfaceKey)
{
    objectDetail, mat, n, cf, surfaceKey;
    AssertFatal(0, "TSShapeInstance::ObjectInstance::buildPolyList:  no default method.");
    return false;
}

void TSShapeInstance::ObjectInstance::support(S32, const Point3F&, F32*, Point3F*)
{
    AssertFatal(0, "TSShapeInstance::ObjectInstance::supprt:  no default method.");
}

bool TSShapeInstance::ObjectInstance::castRay(S32 objectDetail, const Point3F& start, const Point3F& end, RayInfo* rayInfo)
{
    objectDetail, start, end, rayInfo;
    AssertFatal(0, "TSShapeInstance::ObjectInstance::castRay:  no default method.");
    return false;
}

bool TSShapeInstance::MeshObjectInstance::buildPolyList(S32 objectDetail, AbstractPolyList* polyList, U32& surfaceKey)
{
    TSMesh* mesh = getMesh(objectDetail);
    if (mesh && visible > 0.01f)
        return mesh->buildPolyList(frame, polyList, surfaceKey);
    return false;
}

bool TSShapeInstance::MeshObjectInstance::getFeatures(S32 objectDetail, const MatrixF& mat, const Point3F& n, ConvexFeature* cf, U32& surfaceKey)
{
    TSMesh* mesh = getMesh(objectDetail);
    if (mesh && visible > 0.01f)
        return mesh->getFeatures(frame, mat, n, cf, surfaceKey);
    return false;
}

void TSShapeInstance::MeshObjectInstance::support(S32 objectDetail, const Point3F& v, F32* currMaxDP, Point3F* currSupport)
{
    TSMesh* mesh = getMesh(objectDetail);
    if (mesh && visible > 0.01f)
        mesh->support(frame, v, currMaxDP, currSupport);
}


bool TSShapeInstance::MeshObjectInstance::castRay(S32 objectDetail, const Point3F& start, const Point3F& end, RayInfo* rayInfo)
{
    TSMesh* mesh = getMesh(objectDetail);
    if (mesh && visible > 0.01f)
        return mesh->castRay(frame, start, end, rayInfo);
    return false;
}

bool TSShapeInstance::ObjectInstance::castRayOpcode(S32 objectDetail, const Point3F& start, const Point3F& end, RayInfo* rayInfo, TSMaterialList* materials)
{
    return false;
}

bool TSShapeInstance::ObjectInstance::buildPolyListOpcode(S32 objectDetail, AbstractPolyList* polyList, U32& surfaceKey, TSMaterialList* materials)
{
    return false;
}

bool TSShapeInstance::MeshObjectInstance::castRayOpcode(S32 objectDetail, const Point3F& start, const Point3F& end, RayInfo* info, TSMaterialList* materials)
{
    TSMesh* mesh = getMesh(objectDetail);
    if (mesh && visible > 0.01f)
        return mesh->castRayOpcode(start, end, info, materials);
    return false;
}

bool TSShapeInstance::MeshObjectInstance::buildPolyListOpcode(S32 objectDetail, AbstractPolyList* polyList, const Box3F& box, TSMaterialList* materials, U32& surfaceKey)
{
    TSMesh* mesh = getMesh(objectDetail);
    if (mesh && visible > 0.01f && box.isOverlapped(mesh->getBounds()))
        return mesh->buildPolyListOpcode(frame, polyList, box, materials, surfaceKey);
    return false;
}

bool TSShapeInstance::buildPolyListOpcode(S32 dl, AbstractPolyList* polyList, const Box3F& box)
{
    PROFILE_SCOPE(TSShapeInstance_buildPolyListOpcode_MeshObjInst);

    if (dl == -1)
        return false;

    AssertFatal(dl >= 0 && dl < mShape->details.size(), "TSShapeInstance::buildPolyListOpcode");

    // get subshape and object detail
    const TSDetail* detail = &mShape->details[dl];
    S32 ss = detail->subShapeNum;
    S32 od = detail->objectDetailNum;

    // set up static data
    setStatics(dl);

    // nothing emitted yet...
    bool emitted = false;
    U32 surfaceKey = 0;

    S32 start = mShape->subShapeFirstObject[ss];
    S32 end = mShape->subShapeNumObjects[ss] + start;
    if (start < end)
    {
        MatrixF initialMat;
        Point3F initialScale;
        polyList->getTransform(&initialMat, &initialScale);

        // set up for first object's node
        MatrixF mat;
        MatrixF scaleMat(true);
        F32* p = scaleMat;
        p[0] = initialScale.x;
        p[5] = initialScale.y;
        p[10] = initialScale.z;
        MatrixF* previousMat = mMeshObjects[start].getTransform();
        mat.mul(initialMat, scaleMat);
        mat.mul(*previousMat);
        if (m_matF_determinant((const F32*)&mat) != 0.0) {
            polyList->setTransform(&mat, Point3F(1, 1, 1));

            // Update our bounding box...
            Box3F localBox = box;
            MatrixF otherMat = mat;
            otherMat.inverse();
            otherMat.mul(localBox);

            // run through objects and collide
            for (S32 i = start; i < end; i++)
            {
                MeshObjectInstance* mesh = &mMeshObjects[i];

                if (od >= mesh->object->numMeshes)
                    continue;

                if (mesh->getTransform() != previousMat)
                {
                    // different node from before, set up for this node
                    previousMat = mesh->getTransform();

                    if (previousMat != NULL)
                    {
                        mat.mul(initialMat, scaleMat);
                        mat.mul(*previousMat);
                        polyList->setTransform(&mat, Point3F(1, 1, 1));

                        // Update our bounding box...
                        otherMat = mat;
                        otherMat.inverse();
                        localBox = box;
                        otherMat.mul(localBox);
                    }
                }

                // collide...
                emitted |= mesh->buildPolyListOpcode(od, polyList, localBox, mMaterialList, surfaceKey);
            }

            // restore original transform...
            polyList->setTransform(&initialMat, initialScale);
        }
    }

    clearStatics();

    return emitted;
}

bool TSShapeInstance::castRayOpcode(S32 dl, const Point3F& startPos, const Point3F& endPos, RayInfo* info)
{
    // if dl==-1, nothing to do
    if (dl == -1)
        return false;

    AssertFatal(dl >= 0 && dl < mShape->details.size(), "TSShapeInstance::castRayOpcode");

    info->t = 100.f;

    // get subshape and object detail
    const TSDetail* detail = &mShape->details[dl];
    S32 ss = detail->subShapeNum;
    S32 od = detail->objectDetailNum;

    // set up static data
    setStatics(dl);

    // nothing emitted yet...
    bool emitted = false;

    const MatrixF* saveMat = NULL;
    S32 start = mShape->subShapeFirstObject[ss];
    S32 end = mShape->subShapeNumObjects[ss] + start;
    if (start < end)
    {
        MatrixF mat;
        const MatrixF* previousMat = mMeshObjects[start].getTransform();
        mat = *previousMat;
        mat.inverse();
        Point3F localStart, localEnd;
        mat.mulP(startPos, &localStart);
        mat.mulP(endPos, &localEnd);

        // run through objects and collide
        for (S32 i = start; i < end; i++)
        {
            MeshObjectInstance *mesh = &mMeshObjects[i];

            if (od >= mesh->object->numMeshes)
                continue;

            if (mesh->getTransform() != previousMat)
            {
                // different node from before, set up for this node
                previousMat = mesh->getTransform();

                if (previousMat != NULL)
                {
                    mat = *previousMat;
                    mat.inverse();
                    mat.mulP(startPos, &localStart);
                    mat.mulP(endPos, &localEnd);
                }
            }

            // collide...
            if (mesh->castRayOpcode(od, localStart, localEnd, info, mMaterialList))
            {
                saveMat = previousMat;
                emitted = true;
            }
        }
    }

    if (emitted)
    {
        saveMat->mulV(info->normal);
        info->point = endPos - startPos;
        info->point *= info->t;
        info->point += startPos;
    }

    return emitted;
}

bool TSMesh::buildPolyListOpcode(const S32 od, AbstractPolyList* polyList, const Box3F& nodeBox, TSMaterialList* materials, U32& surfaceKey)
{
    PROFILE_SCOPE(TSMesh_buildPolyListOpcode);

    Opcode::AABBCollider opCollider;
    Opcode::AABBCache opCache;

    IceMaths::AABB opBox;
    opBox.SetMinMax(Point(nodeBox.min.x, nodeBox.min.y, nodeBox.min.z),
        Point(nodeBox.max.x, nodeBox.max.y, nodeBox.max.z));

    Opcode::CollisionAABB opCBox(opBox);

    // opCollider.SetTemporalCoherence(true);
    opCollider.SetPrimitiveTests(true);
    opCollider.SetFirstContact(false);

    if (!opCollider.Collide(opCache, opCBox, *mOptTree))
        return false;

    U32 count = opCollider.GetNbTouchedPrimitives();
    const udword* idx = opCollider.GetTouchedPrimitives();

    Opcode::VertexPointers vp;
    U32 plIdx[3];
    S32 j;
    Point3F tmp;
    const IceMaths::Point** verts;
    const	Opcode::MeshInterface* mi = mOptTree->GetMeshInterface();

    for (S32 i = 0; i < count; i++)
    {
        // Get the triangle...
        mi->GetTriangle(vp, idx[i]);
        verts = vp.Vertex;

        F32 ax = verts[1]->x - verts[0]->x;
        F32 ay = verts[1]->y - verts[0]->y;
        F32 az = verts[1]->z - verts[0]->z;
        F32 bx = verts[2]->x - verts[0]->x;
        F32 by = verts[2]->y - verts[0]->y;
        F32 bz = verts[2]->z - verts[0]->z;
        F32 x = ay * bz - az * by;
        F32 y = az * bx - ax * bz;
        F32 z = ax * by - ay * bx;
        F32 squared = x * x + y * y + z * z;

        if (squared == 0) continue; // Don't do invalid planes or else we crash

        // And register it in the polylist...
        polyList->begin(vp.MatIdx, surfaceKey++);

        for (j = 2; j > -1; j--)
        {
            tmp.set(verts[j]->x, verts[j]->y, verts[j]->z);
            plIdx[j] = polyList->addPoint(tmp);
            polyList->vertex(plIdx[j]);
        }

        polyList->plane(plIdx[0], plIdx[2], plIdx[1]);
        polyList->end();
    }
    

    // TODO: Add a polyList->getCount() so we can see if we
    // got clipped polys and didn't really emit anything.
    return count > 0;
}

void TSMesh::prepOpcodeCollision()
{
    // Make sure opcode is loaded!
    if (!gOpcodeInitialized)
    {
        Opcode::InitOpcode();
        gOpcodeInitialized = true;
    }

    // Don't re init if we already have something...
    if (mOptTree)
        return;


    // Ok, first set up a MeshInterface
    Opcode::MeshInterface* mi = new Opcode::MeshInterface();
    mOpMeshInterface = mi;

    // Figure out how many triangles we have...
    U32 triCount = 0;
    const U32 base = 0;
    for (U32 i = 0; i < primitives.size(); i++)
    {
        TSDrawPrimitive& draw = primitives[i];
        const U32 start = draw.start;

        AssertFatal(draw.matIndex & TSDrawPrimitive::Indexed, "TSMesh::buildPolyList (1)");

        // gonna depend on what kind of primitive it is...
        if ((draw.matIndex & TSDrawPrimitive::TypeMask) == TSDrawPrimitive::Triangles)
            triCount += draw.numElements / 3;
        else
        {
            // Have to walk the tristrip to get a count... may have degenerates
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

                triCount++;
            }
        }
    }


    // Just do the first trilist for now.
    mi->SetNbVertices(verts.size());
    mi->SetNbTriangles(triCount);

    // Stuff everything into appropriate arrays.
    IceMaths::IndexedTriangle* its = new IceMaths::IndexedTriangle[mi->GetNbTriangles()], * curIts = its;
    IceMaths::Point* pts = new IceMaths::Point[mi->GetNbVertices()];

    mOpTris = its;
    mOpPoints = pts;

    // add the polys...
    for (U32 i = 0; i < primitives.size(); i++)
    {
        TSDrawPrimitive& draw = primitives[i];
        const U32 start = draw.start;

        AssertFatal(draw.matIndex & TSDrawPrimitive::Indexed, "TSMesh::buildPolyList (1)");

        const U32 matIndex = draw.matIndex & TSDrawPrimitive::MaterialMask;

        // gonna depend on what kind of primitive it is...
        if ((draw.matIndex & TSDrawPrimitive::TypeMask) == TSDrawPrimitive::Triangles)
        {
            for (S32 j = 0; j < draw.numElements; )
            {
                curIts->mVRef[2] = base + indices[start + j + 0];
                curIts->mVRef[1] = base + indices[start + j + 1];
                curIts->mVRef[0] = base + indices[start + j + 2];
                curIts->mMatIdx = matIndex;

                Box3F triBox(0, 0, 0, 0, 0, 0);
                triBox.extend(verts[base + indices[start + j + 0]]);
                triBox.extend(verts[base + indices[start + j + 1]]);
                triBox.extend(verts[base + indices[start + j + 2]]);

                curIts++;

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

                curIts->mVRef[2] = idx0;
                curIts->mVRef[1] = idx1;
                curIts->mVRef[0] = idx2;
                curIts->mMatIdx = matIndex;
                curIts++;
            }
        }
    }

    AssertFatal((curIts - its) == mi->GetNbTriangles(), "Triangle count mismatch!");

    for (S32 i = 0; i < mi->GetNbVertices(); i++)
            pts[i].Set(verts[i].x, verts[i].y, verts[i].z);

    mi->SetPointers(its, pts);

    // Ok, we've got a mesh interface populated, now let's build a thingy to collide against.
    mOptTree = new Opcode::Model();

    Opcode::OPCODECREATE opcc;

    opcc.mCanRemap = false;
    opcc.mIMesh = mi;
    opcc.mKeepOriginal = true;
    opcc.mNoLeaf = false;
    opcc.mQuantized = false;
    opcc.mSettings.mLimit = 1;

    mOptTree->Build(opcc);
}

bool TSMesh::castRayOpcode(const Point3F& s, const Point3F& e, RayInfo* info, TSMaterialList* materials)
{
    Opcode::RayCollider ray;
    Opcode::CollisionFaces cfs;

    IceMaths::Point dir(e.x - s.x, e.y - s.y, e.z - s.z);
    const F32 rayLen = dir.Magnitude();
    IceMaths::Ray vec(Point(s.x, s.y, s.z), dir.Normalize());

    ray.SetDestination(&cfs);
    ray.SetFirstContact(false);
    ray.SetClosestHit(true);
    ray.SetPrimitiveTests(true);
    ray.SetCulling(true);
    ray.SetMaxDist(rayLen);

    AssertFatal(ray.ValidateSettings() == NULL, "invalid ray settings");

    // Do collision.
    bool safety = ray.Collide(vec, *mOptTree);
    AssertFatal(safety, "TSMesh::castRayOpcode - no good ray collide!");

    // If no hit, just skip out.
    if (cfs.GetNbFaces() == 0)
        return false;

    // Got a hit!
    AssertFatal(cfs.GetNbFaces() == 1, "bad");
    const Opcode::CollisionFace& face = cfs.GetFaces()[0];

    // If the cast was successful let's check if the t value is less than what we had
    // and toggle the collision boolean
    // Stupid t... i prefer coffee
    const F32 t = face.mDistance / rayLen;

    if (t < 0.0f || t > 1.0f)
        return false;

    if (t <= info->t)
    {
        info->t = t;

        // Calculate the normal.
        Opcode::VertexPointers vp;
        mOptTree->GetMeshInterface()->GetTriangle(vp, face.mFaceID);

        if (materials && vp.MatIdx >= 0 && vp.MatIdx < materials->getMaterialCount())
            info->material = vp.MatIdx; //materials->getMaterialInst(vp.MatIdx);

        // Get the two edges.
        const IceMaths::Point baseVert = *vp.Vertex[0];
        const IceMaths::Point a = *vp.Vertex[1] - baseVert;
        const IceMaths::Point b = *vp.Vertex[2] - baseVert;

        IceMaths::Point n;
        n.Cross(a, b);
        n.Normalize();

        info->normal.set(n.x, n.y, n.z);
        return true;
    }

    return false;
}
