//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "terrain/waterBlock.h"
#include "sceneGraph/sceneState.h"
#include "sceneGraph/sceneGraph.h"
#include "materials/matInstance.h"
#include "core/bitStream.h"
#include "math/mathIO.h"
#include "console/consoleTypes.h"
#include "gui/core/guiTSControl.h"
#include "gfx/primBuilder.h"
#include "../../game/shaders/shdrConsts.h"
#include "renderInstance/renderInstMgr.h"

#define BLEND_TEX_SIZE 256
#define V_SHADER_PARAM_OFFSET 50

IMPLEMENT_CO_NETOBJECT_V1(WaterBlock);


//*****************************************************************************
// WaterBlock
//*****************************************************************************
WaterBlock::WaterBlock()
{
    mGridElementSize = 100.0;
    mObjScale.set(100.0, 100.0, 10.0);

    mFullReflect = true;

    mNetFlags.set(Ghostable | ScopeAlways);
    mTypeMask = WaterObjectType;

    mObjBox.min.set(-0.5, -0.5, -0.5);
    mObjBox.max.set(0.5, 0.5, 0.5);

    mElapsedTime = 0.0;
    mReflectTexSize = 512;
    mBaseColor.set(11, 26, 44, 255);
    mUnderwaterColor.set(25, 51, 76, 153);

    for (U32 i = 0; i < MAX_WAVES; i++)
    {
        mWaveDir[i].set(0.0, 0.0);
        mWaveSpeed[i] = 0.0;
        mWaveTexScale[i].set(0.0, 0.0);
    }

    dMemset(mSurfMatName, 0, sizeof(mSurfMatName));
    dMemset(mMaterial, 0, sizeof(mMaterial));

    mClarity = 0.15;
    mFresnelBias = 0.12;
    mFresnelPower = 6.0;

    mRenderUpdateCount = 0;
    mReflectUpdateCount = 0;
    mReflectUpdateTicks = 0;

    mRenderFogMesh = true;
}

//-----------------------------------------------------------------------------
// onAdd
//-----------------------------------------------------------------------------
bool WaterBlock::onAdd()
{
    if (!Parent::onAdd())
    {
        return false;
    }

    mPrevScale = mObjScale;


    if (isClientObject() || gSPMode)
    {
        // Load in various Material definitions
        for (U32 i = 0; i < NUM_MAT_TYPES; i++)
        {
            SceneGraphData sgData;
            sgData.setDefaultLights();

            GFXVertexPC* vert = NULL;
            GFXVertexFlags flags = (GFXVertexFlags)getGFXVertFlags(vert);

            if( dStrlen(mSurfMatName[i]) > 0 )
            {
                mMaterial[i] = new MatInstance(mSurfMatName[i]);
                mMaterial[i]->init(sgData, flags);
            }
            if (!mMaterial[i])
            {
                Con::warnf("Invalid Material name: %s: for WaterBlock ps2.0+ surface", mSurfMatName[i]);
            }
        }


        if (GFX->getPixelShaderVersion() >= 1.4 && mFullReflect)
        {
            // add to reflective set of objects (sets up updateReflection() callback)
            SimSet* reflectSet = dynamic_cast<SimSet*>(Sim::findObject("reflectiveSet"));
            reflectSet->addObject((SimObject*)this);
            mReflectPlane.setupTex(mReflectTexSize);
        }
        else
        {
            setupRadialVBIB();

            // Create render target for watery bump on 1.1 card implementation
            mBumpTex.set(BLEND_TEX_SIZE, BLEND_TEX_SIZE, GFXFormatR8G8B8A8, &GFXDefaultRenderTargetProfile, 1);
        }
    }


    resetWorldBox();
    addToScene();

    return true;
}

//-----------------------------------------------------------------------------
// onRemove
//-----------------------------------------------------------------------------
void WaterBlock::onRemove()
{
    clearVertBuffers();

    removeFromScene();
    Parent::onRemove();
}

//-----------------------------------------------------------------------------
// packUpdate
//-----------------------------------------------------------------------------
U32 WaterBlock::packUpdate(NetConnection* con, U32 mask, BitStream* stream)
{
    U32 retMask = Parent::packUpdate(con, mask, stream);

    stream->write(mGridElementSize);

    if (stream->writeFlag(mask & (UpdateMask | InitialUpdateMask)))
    {
        // This is set to allow the user to modify the size of the water dynamically
        // in the editor
        mathWrite(*stream, mObjScale);
        stream->writeAffineTransform(mObjToWorld);
        stream->writeFlag(mFullReflect);
        stream->writeFlag(mRenderFogMesh);

        stream->write(mReflectTexSize);
        stream->write(mBaseColor);
        stream->write(mUnderwaterColor);
        stream->write(mClarity);
        stream->write(mFresnelBias);
        stream->write(mFresnelPower);

        for (U32 i = 0; i < MAX_WAVES; i++)
        {
            stream->write(mWaveSpeed[i]);
            mathWrite(*stream, mWaveDir[i]);
            mathWrite(*stream, mWaveTexScale[i]);
        }
    }

    // InitialUpdateMask is not explicitly set b/c the first call to this
    // function has a 0xffffff mask - thus InitialUpdateMask is set the first time
    if (stream->writeFlag(!(mask & InitialUpdateMask)))
        return retMask;




    for (U32 i = 0; i < NUM_MAT_TYPES; i++)
    {
        stream->writeString(mSurfMatName[i]);
    }


    return retMask;
}

//-----------------------------------------------------------------------------
// unpackUpdate
//-----------------------------------------------------------------------------
void WaterBlock::unpackUpdate(NetConnection* con, BitStream* stream)
{
    Parent::unpackUpdate(con, stream);

    stream->read(&mGridElementSize);

    if (stream->readFlag())
    {
        mathRead(*stream, &mObjScale);
        stream->readAffineTransform(&mObjToWorld);
        mFullReflect = stream->readFlag();
        mRenderFogMesh = stream->readFlag();

        stream->read(&mReflectTexSize);
        stream->read(&mBaseColor);
        stream->read(&mUnderwaterColor);
        stream->read(&mClarity);
        stream->read(&mFresnelBias);
        stream->read(&mFresnelPower);

        for (U32 i = 0; i < MAX_WAVES; i++)
        {
            stream->read(&mWaveSpeed[i]);
            mathRead(*stream, &mWaveDir[i]);
            mathRead(*stream, &mWaveTexScale[i]);
        }

        setupVBIB();
    }

    if (stream->readFlag())
        return;



    for (U32 i = 0; i < NUM_MAT_TYPES; i++)
    {
        mSurfMatName[i] = stream->readSTString();
    }


}

//-----------------------------------------------------------------------------
// Setup vertex and index buffers
//-----------------------------------------------------------------------------
void WaterBlock::setupVBIB()
{
    clearVertBuffers();

    const U32 maxIndexedVerts = 65536; // max number of indexed verts with U16 size indices

    if (mObjScale.x < mGridElementSize ||
        mObjScale.y < mGridElementSize)
    {
        AssertISV(false, "WaterBlock: Scale must be larger than gridElementSize");
    }

    mWidth = mFloor(mObjScale.x / mGridElementSize) + 1;
    mHeight = mFloor(mObjScale.y / mGridElementSize) + 1;

    // figure out how many blocks are needed and their size
    U32 maxBlockRows = maxIndexedVerts / mWidth;
    U32 rowOffset = 0;

    while ((rowOffset + 1) < mHeight)
    {
        U32 numRows = mHeight - rowOffset;
        if (numRows == 1) numRows++;
        if (numRows > maxBlockRows)
        {
            numRows = maxBlockRows;
        }

        setupVertexBlock(mWidth, numRows, rowOffset);
        setupPrimitiveBlock(mWidth, numRows);

        rowOffset += numRows - 1;
    }

}

//-----------------------------------------------------------------------------
// Set up a block of vertices - the width is always the width of the entire
// waterBlock, so this is a block of full rows.
//-----------------------------------------------------------------------------
void WaterBlock::setupVertexBlock(U32 width, U32 height, U32 rowOffset)
{
    U32 numVerts = width * height;

    GFXVertexP* verts = new GFXVertexP[numVerts];

    U32 index = 0;
    for (U32 i = 0; i < height; i++)
    {
        for (U32 j = 0; j < width; j++, index++)
        {
            GFXVertexP* vert = &verts[index];
            vert->point.x = (-mObjScale.x / 2.0) + mGridElementSize * j;
            vert->point.y = (-mObjScale.y / 2.0) + mGridElementSize * (i + rowOffset);
            vert->point.z = 0.0;
        }
    }

    // copy to vertex buffer
    GFXVertexBufferHandle <GFXVertexP>* vertBuff = new GFXVertexBufferHandle <GFXVertexP>;

    vertBuff->set(GFX, numVerts, GFXBufferTypeStatic);
    GFXVertexP* vbVerts = vertBuff->lock();
    dMemcpy(vbVerts, verts, sizeof(GFXVertexP) * numVerts);
    vertBuff->unlock();
    mVertBuffList.push_back(vertBuff);


    delete[] verts;

}

//-----------------------------------------------------------------------------
// Set up a block of indices to match the block of vertices. The width is 
// always the width of the entire waterBlock, so this is a block of full rows.
//-----------------------------------------------------------------------------
void WaterBlock::setupPrimitiveBlock(U32 width, U32 height)
{
    // setup vertex / primitive buffers
    U32 numIndices = (width - 1) * (height - 1) * 6.0;
    U16* indices = new U16[numIndices];
    U32 numVerts = width * height;

    // This uses indexed triangle lists instead of strips, but it shouldn't be
    // significantly slower if the indices cache well.

    // Rough diagram of the index order
    //   0----2----+ ...
    //   |  / |    |
    //   |/   |    |
    //   1----3----+ ...
    //   |    |    |
    //   |    |    |
    //   +----+----+ ...

    U32 index = 0;
    for (U32 i = 0; i < (height - 1); i++)
    {
        for (U32 j = 0; j < (width - 1); j++, index += 6)
        {
            // Process one quad at a time.  Note it will re-use the same indices from
            // previous quad, thus optimizing vert cache.  Cache will run out at
            // end of each row with this implementation however.
            indices[index + 0] = (i)*mWidth + j;         // 0
            indices[index + 1] = (i + 1) * mWidth + j;       // 1
            indices[index + 2] = i * mWidth + j + 1;        // 2
            indices[index + 3] = (i + 1) * mWidth + j;       // 1
            indices[index + 4] = (i + 1) * mWidth + j + 1;     // 3
            indices[index + 5] = i * mWidth + j + 1;        // 2
        }

    }

    GFXPrimitiveBufferHandle* indexBuff = new GFXPrimitiveBufferHandle;

    GFXPrimitive pInfo;
    pInfo.type = GFXTriangleList;
    pInfo.numPrimitives = numIndices / 3;
    pInfo.startIndex = 0;
    pInfo.minIndex = 0;
    pInfo.numVertices = numVerts;

    U16* ibIndices;
    GFXPrimitive* piInput;
    indexBuff->set(GFX, numIndices, 1, GFXBufferTypeStatic);
    indexBuff->lock(&ibIndices, &piInput);
    dMemcpy(ibIndices, indices, numIndices * sizeof(U16));
    dMemcpy(piInput, &pInfo, sizeof(GFXPrimitive));
    indexBuff->unlock();
    mPrimBuffList.push_back(indexBuff);


    delete[] indices;
}


//-----------------------------------------------------------------------------
// prepRenderImage
//-----------------------------------------------------------------------------
bool WaterBlock::prepRenderImage(SceneState* state,
    const U32   stateKey,
    const U32,
    const bool)
{
    if (isLastState(state, stateKey))
        return false;

    setLastState(state, stateKey);

    // This should be sufficient for most objects that don't manage zones, and
    //  don't need to return a specialized RenderImage...
    if (state->isObjectRendered(this))
    {
        RenderInst* ri = gRenderInstManager.allocInst();
        ri->obj = this;
        ri->state = state;
        ri->type = RenderInstManager::RIT_Water;
        gRenderInstManager.addInst(ri);

        mRenderUpdateCount++;
    }

    return false;
}

//------------------------------------------------------------------------------
// Setup scenegraph data structure for materials
//------------------------------------------------------------------------------
SceneGraphData WaterBlock::setupSceneGraphInfo(SceneState* state)
{
    SceneGraphData sgData;

    // grab the sun data from the light manager
    Vector<LightInfo*> lights;
    const LightInfo* sunlight = getCurrentClientSceneGraph()->getLightManager()->sgGetSpecialLight(LightManager::sgSunLightType);
    VectorF sunVector = sunlight->mDirection;

    // set the sun data into scenegraph data
    sgData.light.mDirection.set(sunVector);
    sgData.light.mPos.set(sunVector * -10000.0);
    sgData.light.mAmbient = sunlight->mAmbient;

    // fill in camera position relative to water
    sgData.camPos = state->getCameraPosition();

    // fill in water's transform
    sgData.objTrans = getRenderTransform();

    // fog
    sgData.useFog = true;
    sgData.fogTex = getCurrentClientSceneGraph()->getFogTexture();
    sgData.fogHeightOffset = getCurrentClientSceneGraph()->getFogHeightOffset();
    sgData.fogInvHeightRange = getCurrentClientSceneGraph()->getFogInvHeightRange();
    sgData.visDist = getCurrentClientSceneGraph()->getVisibleDistanceMod();

    // misc
    sgData.backBuffTex = GFX->getSfxBackBuffer();
    sgData.reflectTex = mReflectPlane.getTex();
    sgData.miscTex = mBumpTex;


    return sgData;
}

//-----------------------------------------------------------------------------
// set shader parameters
//-----------------------------------------------------------------------------
void WaterBlock::setShaderParams()
{
    mElapsedTime = (F32)Platform::getVirtualMilliseconds() / 1000.0f; // uggh, should multiply by timescale (it's in main.cpp)

    F32 reg[4];

    // set vertex shader constants
    //-----------------------------------
    for (U32 i = 0; i < MAX_WAVES; i++)
    {
        dMemcpy(reg, mWaveDir[i], sizeof(Point2F));

        GFX->setVertexShaderConstF(V_SHADER_PARAM_OFFSET + i, (float*)reg, 1);
    }

    for (U32 i = 0; i < MAX_WAVES; i++)
    {
        reg[i] = mWaveSpeed[i] * mElapsedTime;
    }

    GFX->setVertexShaderConstF(V_SHADER_PARAM_OFFSET + 4, (float*)reg, 1);


    for (U32 i = 0; i < MAX_WAVES; i++)
    {
        Point2F texScale = mWaveTexScale[i];
        if (texScale.x > 0.0)
        {
            texScale.x = 1.0 / texScale.x;
        }
        if (texScale.y > 0.0)
        {
            texScale.y = 1.0 / texScale.y;
        }

        GFX->setVertexShaderConstF(V_SHADER_PARAM_OFFSET + 5 + i, (float*)&texScale, 1);
    }

    reg[0] = mReflectTexSize;
    GFX->setVertexShaderConstF(V_SHADER_PARAM_OFFSET + 9, (float*)reg, 1);


    // set pixel shader constants
    //-----------------------------------
    reg[0] = mBaseColor.red / 255.0;
    reg[1] = mBaseColor.green / 255.0;
    reg[2] = mBaseColor.blue / 255.0;
    reg[3] = mBaseColor.alpha / 255.0;
    GFX->setPixelShaderConstF(PC_USERDEF1, (float*)reg, 1);

    reg[0] = mClarity; // clarity
    reg[1] = mFresnelBias; // fresnel bias
    reg[2] = mFresnelPower;  // fresnel power
    GFX->setPixelShaderConstF(10, (float*)reg, 1);

}

//-----------------------------------------------------------------------------
// renderObject
//-----------------------------------------------------------------------------
void WaterBlock::renderObject(SceneState* state, RenderInst* ri)
{
    if (getCurrentClientSceneGraph()->isReflectPass())
    {
        return;
    }

    for (U32 i = 0; i < 4; i++)
    {
        GFX->setTextureStageMagFilter(i, GFXTextureFilterLinear);
        GFX->setTextureStageMinFilter(i, GFXTextureFilterLinear);
        GFX->setTextureStageMipFilter(i, GFXTextureFilterLinear);
    }


    GFX->setBaseRenderState();
    GFX->setCullMode(GFXCullNone);

    SceneGraphData sgData = setupSceneGraphInfo(state);
    const Point3F& camPos = state->getCameraPosition();


    if (GFX->getPixelShaderVersion() < 2.0 || !mFullReflect && mRadialVertBuff)
    {
        animBumpTex(state);
        render1_1(sgData, camPos);
    }
    else
    {
        render2_0(sgData, camPos);
    }

    // restore states
    GFX->setBaseRenderState();

}

//-----------------------------------------------------------------------------
// render water for 1.1 pixel shader cards
//-----------------------------------------------------------------------------
void WaterBlock::render1_1(SceneGraphData& sgData, const Point3F& camPosition)
{
    GFX->setTextureStageAddressModeU(0, GFXAddressWrap);
    GFX->setTextureStageAddressModeV(0, GFXAddressWrap);
    GFX->setTextureStageAddressModeU(1, GFXAddressClamp);  // reflection
    GFX->setTextureStageAddressModeV(1, GFXAddressClamp);
    GFX->setTextureStageAddressModeU(2, GFXAddressClamp);  // refraction
    GFX->setTextureStageAddressModeV(2, GFXAddressClamp);
    GFX->setTextureStageAddressModeU(3, GFXAddressClamp);  // fog
    GFX->setTextureStageAddressModeV(3, GFXAddressClamp);


    // setup proj/world transform
    GFX->pushWorldMatrix();
    MatrixF world = GFX->getWorldMatrix();
    world.mul(getRenderTransform());
    GFX->setWorldMatrix(world);

    MatrixF proj = GFX->getProjectionMatrix();
    proj.mul(world);
    proj.transpose();
    GFX->setVertexShaderConstF(0, (float*)&proj, 4);


    // update plane
    PlaneF plane;
    Point3F norm;
    getRenderTransform().getColumn(2, &norm);
    norm.normalize();
    plane.set(getRenderPosition(), norm);
    mReflectPlane.setPlane(plane);



    // set the material
    MatInstance* mat = NULL;
    if (!mFullReflect)
    {
        // This is here because the 1_1 pass is run by 2.0 cards when full reflect is off
        mat = mMaterial[NO_REFLECT];
    }
    else
    {
        mat = mMaterial[BASE_PASS];
    }

    bool underWater = false;
    if (isUnderwater(camPosition))
    {
        underWater = true;
        mat = mMaterial[UNDERWATER_PASS];
        //mat = static_cast<CustomMaterial*>(Sim::findObject(mSurfMatName[UNDERWATER_PASS]));
        GFX->setTextureStageAddressModeU(3, GFXAddressWrap);
        GFX->setTextureStageAddressModeV(3, GFXAddressWrap);

        if (GFX->getPixelShaderVersion() >= 2.0)
        {
            setShaderParams();
        }
    }


    // draw once to stencil
    if (mRenderFogMesh)
    {
        GFX->setStencilEnable(true);
        GFX->setStencilRef(2);
        GFX->setStencilFunc(GFXCmpAlways);
        GFX->setStencilPassOp(GFXStencilOpReplace);
    }

    // render the grid geometry
    if (mat)
    {
        while (mat->setupPass(sgData))
        {
            for (U32 i = 0; i < mVertBuffList.size(); i++)
            {
                // set vert/prim buffer
                GFX->setVertexBuffer(*mVertBuffList[i]);
                GFXPrimitiveBuffer* primBuff = *mPrimBuffList[i];
                GFX->setPrimitiveBuffer(primBuff);
                GFX->drawPrimitives();
            }

        }
    }

    if (mRenderFogMesh)
    {
        GFX->setStencilEnable(true);
        GFX->setStencilPassOp(GFXStencilOpZero);
        GFX->setStencilFunc(GFXCmpEqual);
        GFX->setZEnable(false);

        // render radial geometry
        if (!underWater)
        {
            GFX->popWorldMatrix();
            GFX->pushWorldMatrix();
            MatrixF world = GFX->getWorldMatrix();

            MatrixF matTrans = getRenderTransform();
            Point3F pos = plane.project(camPosition);
            matTrans.setPosition(pos);
            world.mul(matTrans);
            GFX->setWorldMatrix(world);
            sgData.objTrans = matTrans;


            MatrixF proj = GFX->getProjectionMatrix();
            proj.mul(world);
            proj.transpose();
            GFX->setVertexShaderConstF(0, (float*)&proj, 4);

            GFX->setVertexBuffer(mRadialVertBuff);
            GFXPrimitiveBuffer* primBuff = mRadialPrimBuff;
            GFX->setPrimitiveBuffer(primBuff);

            mat = mMaterial[FOG_PASS];

            if (mat)
            {
                while (mat->setupPass(sgData))
                {
                    GFX->setAlphaBlendEnable(true);
                    GFX->drawPrimitives();
                }
            }

        }
    }



    GFX->popWorldMatrix();
    GFX->setStencilEnable(false);
    GFX->setZEnable(true);


    if (underWater)
    {
        drawUnderwaterFilter();
    }

    GFX->setAlphaBlendEnable(false);
}

//-----------------------------------------------------------------------------
// render water for 2.0+ pixel shader cards
//-----------------------------------------------------------------------------
void WaterBlock::render2_0(SceneGraphData& sgData, const Point3F& camPosition)
{
    GFX->setTextureStageAddressModeU(0, GFXAddressWrap);
    GFX->setTextureStageAddressModeV(0, GFXAddressWrap);
    GFX->setTextureStageAddressModeU(1, GFXAddressClamp);  // reflection
    GFX->setTextureStageAddressModeV(1, GFXAddressClamp);
    GFX->setTextureStageAddressModeU(2, GFXAddressClamp);  // refraction
    GFX->setTextureStageAddressModeV(2, GFXAddressClamp);
    GFX->setTextureStageAddressModeU(3, GFXAddressClamp);  // fog
    GFX->setTextureStageAddressModeV(3, GFXAddressClamp);


    // setup proj/world transform
    GFX->pushWorldMatrix();
    MatrixF world = GFX->getWorldMatrix();
    world.mul(getRenderTransform());
    GFX->setWorldMatrix(world);

    MatrixF proj = GFX->getProjectionMatrix();
    proj.mul(world);
    proj.transpose();
    GFX->setVertexShaderConstF(0, (float*)&proj, 4);

    setShaderParams();


    // set the material
    MatInstance* mat = mMaterial[BASE_PASS];

    bool underwater = false;
    if (isUnderwater(camPosition))
    {
        underwater = true;
        //mat = static_cast<CustomMaterial*>(Sim::findObject(mSurfMatName[UNDERWATER_PASS]));
        mat = mMaterial[UNDERWATER_PASS];
    }


    // render the geometry
    if (mat)
    {
        while (mat->setupPass(sgData))
        {
            for (U32 i = 0; i < mVertBuffList.size(); i++)
            {
                // set vert/prim buffer
                GFX->setVertexBuffer(*mVertBuffList[i]);
                GFXPrimitiveBuffer* primBuff = *mPrimBuffList[i];
                GFX->setPrimitiveBuffer(primBuff);
                GFX->drawPrimitives();
            }
        }
    }


    GFX->popWorldMatrix();

    if (underwater)
    {
        drawUnderwaterFilter();
    }
}

//-----------------------------------------------------------------------------
// initPersistFields
//-----------------------------------------------------------------------------
void WaterBlock::initPersistFields()
{
    Parent::initPersistFields();

    addGroup("WaveData");
    addField("waveDir", TypePoint2F, Offset(mWaveDir, WaterBlock), MAX_WAVES);
    addField("waveSpeed", TypeF32, Offset(mWaveSpeed, WaterBlock), MAX_WAVES);
    addField("waveTexScale", TypePoint2F, Offset(mWaveTexScale, WaterBlock), MAX_WAVES);
    endGroup("WaveData");

    addGroup("Misc");
    addField("reflectTexSize", TypeS32, Offset(mReflectTexSize, WaterBlock));
    addField("baseColor", TypeColorI, Offset(mBaseColor, WaterBlock));
    addField("underwaterColor", TypeColorI, Offset(mUnderwaterColor, WaterBlock));
    addField("gridSize", TypeF32, Offset(mGridElementSize, WaterBlock));
    endGroup("Misc");

    addField("surfMaterial", TypeString, Offset(mSurfMatName, WaterBlock), NUM_MAT_TYPES);
    addField("fullReflect", TypeBool, Offset(mFullReflect, WaterBlock));
    addField("clarity", TypeF32, Offset(mClarity, WaterBlock));
    addField("fresnelBias", TypeF32, Offset(mFresnelBias, WaterBlock));
    addField("fresnelPower", TypeF32, Offset(mFresnelPower, WaterBlock));

    addField("renderFogMesh", TypeBool, Offset(mRenderFogMesh, WaterBlock));
}


//-----------------------------------------------------------------------------
// Update planar reflection
//-----------------------------------------------------------------------------
void WaterBlock::updateReflection()
{
    PROFILE_START(WaterBlock_updateReflection);

    // Simple method of insuring reflections aren't rendered unless
    // the waterBlock is actually visible
    if (mRenderUpdateCount > mReflectUpdateCount)
    {
        mReflectUpdateCount = mRenderUpdateCount;
    }
    else
    {
        PROFILE_END();
        return;
    }

    // Update water reflection no more than specified interval - causes some
    // artifacts if you whip the camera around a lot, but definitely can
    // improve speed.
    U32 curTime = Platform::getVirtualMilliseconds();
    if (curTime - mReflectUpdateTicks < 16)
    {
        PROFILE_END();
        return;
    }

    // grab camera transform from tsCtrl
    GuiTSCtrl* tsCtrl;
    tsCtrl = dynamic_cast<GuiTSCtrl*>(Sim::findObject("PlayGui"));
    CameraQuery query;
    if (!tsCtrl->processCameraQuery(&query))
    {
        PROFILE_END();
        return;
    }

    mReflectUpdateTicks = curTime;

    // store current matrices
    GFX->pushWorldMatrix();
    MatrixF proj = GFX->getProjectionMatrix();


    // set up projection - must match that of main camera
    Point2I resolution = gClientSceneGraph->getDisplayTargetResolution();
    GFX->setFrustum(mRadToDeg(query.fov),
        F32(resolution.x) / F32(resolution.y),
        query.nearPlane, query.farPlane);

    // store "normal" camera position before changing over to reflected position
    MatrixF camTrans = query.cameraMatrix;
    getCurrentClientSceneGraph()->mNormCamPos = camTrans.getPosition();

    // update plane
    PlaneF plane;
    Point3F norm;
    getRenderTransform().getColumn(2, &norm);
    norm.normalize();
    plane.set(getRenderPosition(), norm);
    mReflectPlane.setPlane(plane);

    // set world mat from new camera view
    MatrixF camReflectTrans = mReflectPlane.getCameraReflection(camTrans);
    camReflectTrans.inverse();
    GFX->setWorldMatrix(camReflectTrans);

    // set new projection matrix
    getCurrentClientSceneGraph()->setNonClipProjection((MatrixF&)GFX->getProjectionMatrix());
    MatrixF clipProj = mReflectPlane.getFrustumClipProj(camReflectTrans);
    GFX->setProjectionMatrix(clipProj);


    // render a frame
    getCurrentClientSceneGraph()->setReflectPass(true);

    GFX->pushActiveRenderSurfaces();
    GFX->setActiveRenderSurface(mReflectPlane.getTex());
    GFX->setZEnable(true);
    GFX->clear(GFXClearZBuffer | GFXClearStencil | GFXClearTarget, ColorI(64, 64, 64), 1.0f, 0);
    U32 objTypeFlag = -1;
    getCurrentClientSceneGraph()->renderScene(objTypeFlag);
    GFX->popActiveRenderSurfaces();

    // cleanup
    GFX->clear(GFXClearZBuffer | GFXClearStencil, ColorI(255, 0, 255), 1.0f, 0);
    getCurrentClientSceneGraph()->setReflectPass(false);
    GFX->popWorldMatrix();
    GFX->setProjectionMatrix(proj);

    PROFILE_END();

}

//-----------------------------------------------------------------------------
// Draw translucent filter over screen when underwater
//-----------------------------------------------------------------------------
void WaterBlock::drawUnderwaterFilter()
{
    // set up camera transforms
    MatrixF proj = GFX->getProjectionMatrix();
    MatrixF newMat(true);
    GFX->setProjectionMatrix(newMat);
    GFX->pushWorldMatrix();
    GFX->setWorldMatrix(newMat);


    // set up render states
    GFX->disableShaders();
    GFX->setAlphaBlendEnable(true);
    GFX->setSrcBlend(GFXBlendSrcAlpha);
    GFX->setDestBlend(GFXBlendInvSrcAlpha);
    GFX->setTextureStageColorOp(0, GFXTOPDisable);
    GFX->setZEnable(false);

    // draw quad
    GFXVideoMode mode = GFX->getVideoMode();
    F32 copyOffsetX = 1.0 / mode.resolution.x;
    F32 copyOffsetY = 1.0 / mode.resolution.y;

    PrimBuild::color(mUnderwaterColor);
    PrimBuild::begin(GFXTriangleFan, 4);
    PrimBuild::texCoord2f(0.0, 1.0);
    PrimBuild::vertex3f(-1.0 - copyOffsetX, -1.0 + copyOffsetY, 0.0);

    PrimBuild::texCoord2f(0.0, 0.0);
    PrimBuild::vertex3f(-1.0 - copyOffsetX, 1.0 + copyOffsetY, 0.0);

    PrimBuild::texCoord2f(1.0, 0.0);
    PrimBuild::vertex3f(1.0 - copyOffsetX, 1.0 + copyOffsetY, 0.0);

    PrimBuild::texCoord2f(1.0, 1.0);
    PrimBuild::vertex3f(1.0 - copyOffsetX, -1.0 + copyOffsetY, 0.0);
    PrimBuild::end();


    // reset states / transforms
    GFX->setZEnable(true);
    GFX->setAlphaBlendEnable(false);
    GFX->setProjectionMatrix(proj);
    GFX->popWorldMatrix();

}

//-----------------------------------------------------------------------------
// Animate the bump texture - for 1.1 cards
//-----------------------------------------------------------------------------
void WaterBlock::animBumpTex(SceneState* state)
{
    GFX->setTextureStageAddressModeU(0, GFXAddressWrap);
    GFX->setTextureStageAddressModeV(0, GFXAddressWrap);
    GFX->setTextureStageAddressModeU(1, GFXAddressWrap);
    GFX->setTextureStageAddressModeV(1, GFXAddressWrap);

    // set ortho projection matrix
    MatrixF proj = GFX->getProjectionMatrix();
    MatrixF newMat(true);
    GFX->setProjectionMatrix(newMat);
    GFX->pushWorldMatrix();
    GFX->setWorldMatrix(newMat);
    GFX->setVertexShaderConstF(0, (float*)&newMat, 4);

    // set up blend shader - pass in wave params
    MatInstance* mat = mMaterial[BLEND];

    // send time info to shader
    mElapsedTime = (F32)Platform::getVirtualMilliseconds() / 1000.0f; // uggh, should multiply by timescale (it's in main.cpp)
    float timeScale = mElapsedTime * 0.15;
    GFX->setVertexShaderConstF(54, (float*)&timeScale, 1);


    GFX->setZEnable(false);
    GFX->pushActiveRenderSurfaces();
    GFX->setActiveRenderSurface(mBumpTex, 0);

    SceneGraphData sgData = setupSceneGraphInfo(state); // hrmm, eliminate this?
    F32 copyOffsetX = 1.0 / BLEND_TEX_SIZE;
    F32 copyOffsetY = 1.0 / BLEND_TEX_SIZE;

    // render out new bump texture
    while (mat->setupPass(sgData))
    {
        PrimBuild::begin(GFXTriangleFan, 4);
        PrimBuild::texCoord2f(0.0, 1.0);
        PrimBuild::vertex3f(-1.0 - copyOffsetX, -1.0 + copyOffsetY, 0.0);

        PrimBuild::texCoord2f(0.0, 0.0);
        PrimBuild::vertex3f(-1.0 - copyOffsetX, 1.0 + copyOffsetY, 0.0);

        PrimBuild::texCoord2f(1.0, 0.0);
        PrimBuild::vertex3f(1.0 - copyOffsetX, 1.0 + copyOffsetY, 0.0);

        PrimBuild::texCoord2f(1.0, 1.0);
        PrimBuild::vertex3f(1.0 - copyOffsetX, -1.0 + copyOffsetY, 0.0);
        PrimBuild::end();
    }

    GFX->popActiveRenderSurfaces();


    // done
    GFX->setZEnable(true);
    GFX->popWorldMatrix();
    GFX->setProjectionMatrix(proj);
}

//-----------------------------------------------------------------------------
// Set up the vertex/index buffers for a radial ( concentric rings ) mesh
//-----------------------------------------------------------------------------
void WaterBlock::setupRadialVBIB()
{
    Vector< GFXVertexP > verts;
    Vector< U16 > indices;

    // create verts
    U32 numRadPoints = 15;
    U32 numSections = 30;

    GFXVertexP vert;
    vert.point.set(0.0, 0.0, 0.0);
    verts.push_back(vert);

    F32 radius = 1.0;

    // set up rings
    for (U32 i = 0; i < numSections; i++)
    {
        for (U32 j = 0; j < numRadPoints; j++)
        {
            F32 angle = F32(j) / F32(numRadPoints) * M_2PI;
            vert.point.set(radius * mSin(angle), radius * mCos(angle), 0.0);
            verts.push_back(vert);
        }
        radius *= 1.3;
    }

    // set up indices for innermost circle
    for (U32 i = 0; i < numRadPoints; i++)
    {
        U16 index = 0;
        indices.push_back(index);
        index = i + 1;
        indices.push_back(index);
        index = ((i + 1) % numRadPoints) + 1;
        indices.push_back(index);
    }

    // set up indices for concentric rings around the center
    for (U32 i = 0; i < numSections - 1; i++)
    {
        for (U32 j = 0; j < numRadPoints; j++)
        {
            U16 pts[4];
            pts[0] = 1 + i * numRadPoints + j;
            pts[1] = 1 + (i + 1) * numRadPoints + j;
            pts[2] = 1 + i * numRadPoints + (j + 1) % numRadPoints;
            pts[3] = 1 + (i + 1) * numRadPoints + (j + 1) % numRadPoints;

            indices.push_back(pts[0]);
            indices.push_back(pts[1]);
            indices.push_back(pts[2]);

            indices.push_back(pts[1]);
            indices.push_back(pts[3]);
            indices.push_back(pts[2]);
        }
    }


    // copy to vertex buffer
    mRadialVertBuff.set(GFX, verts.size(), GFXBufferTypeStatic);
    GFXVertexP* vbVerts = mRadialVertBuff.lock();
    dMemcpy(vbVerts, verts.address(), sizeof(GFXVertexP) * verts.size());
    mRadialVertBuff.unlock();

    // copy to index buffer
    GFXPrimitive pInfo;
    pInfo.type = GFXTriangleList;
    pInfo.numPrimitives = indices.size() / 3;
    pInfo.startIndex = 0;
    pInfo.minIndex = 0;
    pInfo.numVertices = verts.size();

    U16* ibIndices;
    GFXPrimitive* piInput;
    mRadialPrimBuff.set(GFX, indices.size(), 1, GFXBufferTypeStatic);
    mRadialPrimBuff.lock(&ibIndices, &piInput);
    dMemcpy(ibIndices, indices.address(), indices.size() * sizeof(U16));
    dMemcpy(piInput, &pInfo, sizeof(GFXPrimitive));
    mRadialPrimBuff.unlock();


}

//-----------------------------------------------------------------------------
// Returns true if specified point is under the water plane and contained in
// the water's bounding box.
//-----------------------------------------------------------------------------
bool WaterBlock::isUnderwater(const Point3F& pnt)
{
    // update plane
    if (isServerObject())
    {
        PlaneF plane;
        Point3F norm;
        getTransform().getColumn(2, &norm);
        norm.normalize();
        plane.set(getPosition(), norm);
        mReflectPlane.setPlane(plane);
    }

    if (mReflectPlane.getPlane().distToPlane(pnt) < -0.005)
    {
        if (mWorldBox.isContained(pnt))
        {
            return true;
        }
    }

    return false;
}

//-----------------------------------------------------------------------------
// Clear vertex and primitive buffers
//-----------------------------------------------------------------------------
void WaterBlock::clearVertBuffers()
{
    for (U32 i = 0; i < mVertBuffList.size(); i++)
    {
        delete mVertBuffList[i];
    }
    mVertBuffList.clear();

    for (U32 i = 0; i < mPrimBuffList.size(); i++)
    {
        delete mPrimBuffList[i];
    }
    mPrimBuffList.clear();
}

//-----------------------------------------------------------------------------
// Inspect post apply
//-----------------------------------------------------------------------------
void WaterBlock::inspectPostApply()
{
    Parent::inspectPostApply();

    setMaskBits(UpdateMask);
}

//-----------------------------------------------------------------------------
// Set up projection matrix for multipass technique with different geometry.
// It basically just pushes the near plane out.  This should work across 
// fixed-function and shader geometry.
//-----------------------------------------------------------------------------
/*
void WaterBlock::setMultiPassProjection()
{
   F32 nearPlane, farPlane;
   F32 left, right, bottom, top;
   GFX->getFrustum( &left, &right, &bottom, &top, &nearPlane, &farPlane );

   F32 FOV = GameGetCameraFov();
   Point2I size = GFX->getVideoMode().resolution;

//   GFX->setFrustum( FOV, size.x/F32(size.y), nearPlane + 0.010, farPlane + 10.0 );

// note - will have to re-calc left, right, top, bottom if the above technique
// doesn't work through portals
//   GFX->setFrustum( left, right, bottom, top, nearPlane + 0.001, farPlane );


}
*/

bool WaterBlock::castRay(const Point3F& start, const Point3F& end, RayInfo* info)
{
    // Simply look for the hit on the water plane
    // and ignore any future issues with waves, etc.
    const Point3F norm(0, 0, 1);
    PlaneF plane(Point3F(0, 0, 0), norm);

    F32 hit = plane.intersect(start, end);
    if (hit < 0.0f || hit > 1.0f)
        return false;

    info->t = hit;
    info->object = this;
    info->point = start + ((end - start) * hit);
    info->normal = norm;

    return true;
}

