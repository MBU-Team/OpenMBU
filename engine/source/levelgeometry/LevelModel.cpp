#include "LevelModel.h"
#include "LevelModelResource.h"
#include "LevelModelInstance.h"
#include "console/console.h"
#include "renderInstance/renderInstMgr.h"
#include "sceneGraph/sceneGraph.h"

const U32 LevelModel::smFileVersion = 0;
const U32 LevelModel::smIdentifier = 'XLDM'; // MDLX

LevelModel::LevelModel()
{

}

LevelModel::~LevelModel()
{

}

bool LevelModel::read(Stream &stream)
{
    AssertFatal(stream.hasCapability(Stream::StreamRead), "LevelModel::read: non-read capable stream passed");
    AssertFatal(stream.getStatus() == Stream::Ok, "LevelModel::read: Error, stream in inconsistent state");

    U32 identifier;
    stream.read(&identifier);
    if (identifier != smIdentifier)
    {
        Con::errorf("LevelModel::read: Error, invalid file format.");
        return false;
    }

    U32 fileVersion;
    stream.read(&fileVersion);
    if (fileVersion > smFileVersion)
    {
        Con::errorf("LevelModel::read - File version is too new.");
        return false;
    }

    // TODO: Read the model data from the stream.

    return stream.getStatus() == Stream::Ok;
}

bool LevelModel::write(Stream &stream) const
{
    AssertFatal(stream.hasCapability(Stream::StreamWrite), "LevelModel::write: non-write capable stream passed");
    AssertFatal(stream.getStatus() == Stream::Ok, "LevelModel::write: Error, stream in inconsistent state");

    stream.write(smIdentifier);
    stream.write(smFileVersion);

    // TODO: Write the model data to the stream.

    return stream.getStatus() == Stream::Ok;
}

bool LevelModel::prepForRendering(const char* path)
{
    if (!mVertBuff)
    {
        Vector<GFXVertexPNTTBN> verts;
        Vector<U16> indices;

        U32 startIndex = 0;
        U32 startVert = 0;


        Vector<GFXPrimitive> primInfoList;

        // TODO: Fill verts, indices, and primitives
        {
            // TEMP: random test object
            GFXVertexPNTTBN vert1;
            vert1.point.set(0, 0, 0);
            vert1.normal.set(0, 0, 1);
            verts.push_back(vert1);
            indices.push_back(verts.size() - 1);
            GFXVertexPNTTBN vert2;
            vert2.point.set(0, 1, 0);
            vert2.normal.set(0, 0, 1);
            verts.push_back(vert2);
            indices.push_back(verts.size() - 1);
            GFXVertexPNTTBN vert3;
            vert3.point.set(0, 1, 0);
            vert3.normal.set(0, 0, 1);
            verts.push_back(vert3);
            indices.push_back(verts.size() - 1);
            //vert.point = mPoints[tempIndexList[startIndex + i]].point;
            //fillVertex(vert, surface, surfaceIndex);


            //U16* subIndices = &indices[3 * pointOffset + 3 * surface.windingStart];
            //vert.T.set(mNormals[subIndices[0]]);
            //vert.N.set(mNormals[subIndices[1]]);
            //vert.B.set(mNormals[subIndices[2]]);

            //verts.push_back(vert);

            // store the index
            //indices.push_back(verts.size() - 1);

            GFXPrimitive pnfo;

            pnfo.minIndex = U32(-1);

            pnfo.numPrimitives = 1;//(indexList.size() - startIndex) / 3;
            pnfo.startIndex = startIndex;
            pnfo.numVertices = verts.size() - startVert;
            pnfo.type = GFXTriangleList;
            startIndex = 0;//indexList.size();
            startVert = verts.size();

            if (pnfo.numPrimitives > 0)
            {
                primInfoList.push_back(pnfo);
            }
        }

        // create vertex buffer
        mVertBuff.set(GFX, verts.size(), GFXBufferTypeStatic);
        GFXVertexPNTTBN *vbVerts = vbVerts = mVertBuff.lock();

        dMemcpy(vbVerts, verts.address(), verts.size() * sizeof(GFXVertexPNTTBN));

        mVertBuff.unlock();

        // create primitive buffer
        U16 *ibIndices;
        GFXPrimitive *piInput;
        mPrimBuff.set(GFX, indices.size(), primInfoList.size(), GFXBufferTypeStatic);
        mPrimBuff.lock(&ibIndices, &piInput);

        dMemcpy(ibIndices, indices.address(), indices.size() * sizeof(U16));
        dMemcpy(piInput, primInfoList.address(), primInfoList.size() * sizeof(GFXPrimitive));

        mPrimBuff.unlock();
    }

    return true;
}

void LevelModel::prepBatchRender(LevelModelInstance *lmdInst, SceneState *state)
{
    RenderInst* coreRi = gRenderInstManager.allocInst();

    // setup world matrix - for fixed function
    MatrixF world = GFX->getWorldMatrix();
    world.mul(lmdInst->getRenderTransform());
    world.scale(lmdInst->getScale());
    GFX->setWorldMatrix(world);

    // setup world matrix - for shaders
    MatrixF proj = GFX->getProjectionMatrix();
    proj.mul(world);
    proj.transpose();
    GFX->setVertexShaderConstF(0, (float*)&proj, 4);

    // set buffers
    GFX->setVertexBuffer(mVertBuff);
    GFX->setPrimitiveBuffer(mPrimBuff);

    // setup renderInst
    coreRi->worldXform = gRenderInstManager.allocXform();
    *coreRi->worldXform = proj;

    coreRi->vertBuff = &mVertBuff;
    coreRi->primBuff = &mPrimBuff;

    coreRi->objXform = gRenderInstManager.allocXform();
    *coreRi->objXform = lmdInst->getRenderTransform();

    // grab the sun data from the light manager
    const LightInfo* sunlight = getCurrentClientSceneGraph()->getLightManager()->sgGetSpecialLight(LightManager::sgSunLightType);
    VectorF sunVector = sunlight->mDirection;

    coreRi->light.mDirection.set(sunVector);
    coreRi->light.mPos.set(sunVector * -10000.0);
    coreRi->light.mAmbient = sunlight->mAmbient;
    coreRi->light.mColor = sunlight->mColor;

    coreRi->type = RenderInstManager::RIT_Mesh;
    coreRi->backBuffTex = &GFX->getSfxBackBuffer();

    MatInstance* mat = gRenderInstManager.getWarningMat();//node.matInst;
    if (mat && GFX->getPixelShaderVersion() > 0.0)
    {
        coreRi->matInst = mat;
        //coreRi->primBuffIndex = node.primInfoIndex;
        gRenderInstManager.addInst(coreRi);

    }

    GFX->setBaseRenderState();
}

Box3F LevelModel::getBoundingBox() const
{
    // TODO: Return the bounding box of the model.
    return Box3F(0, 0, 0, 1, 1, 1);
}
