//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "sceneGraph/sceneLighting.h"
#include "interior/interiorInstance.h"
#include "interior/interiorRes.h"
#include "interior/interior.h"
#include "gfx/gBitmap.h"
#include "math/mPlane.h"
#include "sceneGraph/sceneGraph.h"
#include "core/fileStream.h"
#include "console/consoleTypes.h"
#include "gui/core/guiCanvas.h"
#include "core/zipSubStream.h"
#include "game/gameConnection.h"

namespace {

    static void findObjectsCallback(SceneObject* obj, void* val)
    {
        Vector<SceneObject*>* list = (Vector<SceneObject*>*)val;
        list->push_back(obj);
    }

    static const Point3F BoxNormals[] =
    {
       Point3F(1, 0, 0),
       Point3F(-1, 0, 0),
       Point3F(0, 1, 0),
       Point3F(0,-1, 0),
       Point3F(0, 0, 1),
       Point3F(0, 0,-1)
    };

    static U32 BoxVerts[][4] = {
       {7,6,4,5},     // +x
       {0,2,3,1},     // -x
       {7,3,2,6},     // +y
       {0,1,5,4},     // -y
       {7,5,1,3},     // +z
       {0,4,6,2}      // -z
    };

    static U32 BoxSharedEdgeMask[][6] = {
       {0, 0, 1, 4, 8, 2},
       {0, 0, 2, 8, 4, 1},
       {8, 2, 0, 0, 1, 4},
       {4, 1, 0, 0, 2, 8},
       {1, 4, 8, 2, 0, 0},
       {2, 8, 4, 1, 0, 0}
    };

    static U32 TerrainSquareIndices[][3] = {
       {2, 1, 0},  // 45
       {3, 2, 0},
       {3, 1, 0},  // 135
       {3, 2, 1}
    };

    static Point3F BoxPnts[] = {
       Point3F(0,0,0),
       Point3F(0,0,1),
       Point3F(0,1,0),
       Point3F(0,1,1),
       Point3F(1,0,0),
       Point3F(1,0,1),
       Point3F(1,1,0),
       Point3F(1,1,1)
    };

    SceneLighting* gLighting = 0;
    F32               gPlaneNormThresh = 0.999;
    F32               gPlaneDistThresh = 0.001;
    F32               gParellelVectorThresh = 0.01;
    bool              gTerminateLighting = false;
    F32               gLightingProgress = 0.f;
    const char* gCompleteCallback = 0;
    U32               gConnectionMissionCRC = 0xffffffff;
}


//------------------------------------------------------------------------------
class SceneLightingProcessEvent : public SimEvent
{
private:
    U32      mLightIndex;
    S32      mObjectIndex;

public:
    SceneLightingProcessEvent(U32 lightIndex, S32 objectIndex)
    {
        mLightIndex = lightIndex;        // size(): end of lighting
        mObjectIndex = objectIndex;      // -1: preLight, size(): next light
    }

    void process(SimObject* object)
    {
        AssertFatal(object, "SceneLightingProcessEvent:: null event object!");
        if (object)
            static_cast<SceneLighting*>(object)->processEvent(mLightIndex, mObjectIndex);
    };
};

//------------------------------------------------------------------------------
bool SceneLighting::smUseVertexLighting = false;

SceneLighting::SceneLighting()
{
    mStartTime = 0;
    mFileName[0] = 0;
    smUseVertexLighting = Interior::smUseVertexLighting;

    static bool initialized = false;
    if (!initialized)
    {
        Con::addVariable("SceneLighting::terminateLighting", TypeBool, &gTerminateLighting);
        Con::addVariable("SceneLighting::lightingProgress", TypeF32, &gLightingProgress);
        initialized = true;
    }
}

SceneLighting::~SceneLighting()
{
    gLighting = 0;
    gLightingProgress = 0.f;

    ObjectProxy** proxyItr;
    for (proxyItr = mSceneObjects.begin(); proxyItr != mSceneObjects.end(); proxyItr++)
        delete* proxyItr;
}

void SceneLighting::completed(bool success)
{
    // process the cached lighting files
    processCache();

    if (success)
    {
        AssertFatal(smUseVertexLighting == Interior::smUseVertexLighting, "SceneLighting::completed: vertex lighting state changed during scene light");

        // cannot do anything if vertex state has changed (since we only load in what is needed)
        if (smUseVertexLighting == Interior::smUseVertexLighting)
        {
            if (!smUseVertexLighting)
            {
                gInteriorLMManager.downloadGLTextures();
                gInteriorLMManager.destroyBitmaps();
            }
            else
                gInteriorLMManager.destroyTextures();
        }
    }

    if (gCompleteCallback && gCompleteCallback[0])
        Con::executef(1, gCompleteCallback);
}

//------------------------------------------------------------------------------
// Static access method: there can be only one SceneLighting object
bool SceneLighting::lightScene(const char* callback, BitSet32 flags)
{
    if (gLighting)
    {
        Con::errorf(ConsoleLogEntry::General, "SceneLighting:: forcing restart of lighting!");
        gLighting->deleteObject();
        gLighting = 0;
    }

    SceneLighting* lighting = new SceneLighting;

    // register the object
    if (!lighting->registerObject())
    {
        AssertFatal(0, "SceneLighting:: Unable to register SceneLighting object!");
        Con::errorf(ConsoleLogEntry::General, "SceneLighting:: Unable to register SceneLighting object!");
        delete lighting;
        return(false);
    }

    // could have interior resources but no instances (hey, got this far didnt we...)
    GameConnection* con = dynamic_cast<GameConnection*>(GameConnection::getConnectionToServer());
    if (!con)
    {
        Con::errorf(ConsoleLogEntry::General, "SceneLighting:: no game connection");
        return(false);
    }
    con->addObject(lighting);

    // set the globals
    gLighting = lighting;
    gTerminateLighting = false;
    gLightingProgress = 0.f;
    gCompleteCallback = callback;
    gConnectionMissionCRC = con->getMissionCRC();


    if (!lighting->light(flags))
    {
        lighting->completed(true);
        lighting->deleteObject();
        return(false);
    }
    return(true);
}

bool SceneLighting::isLighting()
{
    return(bool(gLighting));
}

//------------------------------------------------------------------------------
// Class SceneLighting::PersistInfo
//------------------------------------------------------------------------------
U32 SceneLighting::PersistInfo::smFileVersion = 0x10;

SceneLighting::PersistInfo::~PersistInfo()
{
    for (U32 i = 0; i < mChunks.size(); i++)
        delete mChunks[i];
}

//------------------------------------------------------------------------------
bool SceneLighting::PersistInfo::read(Stream& stream)
{
    U32 version;
    if (!stream.read(&version) || version != smFileVersion)
        return(false);

    U32 numChunks;
    if (!stream.read(&numChunks))
        return(false);

    if (numChunks == 0)
        return(false);

    // read in all the chunks
    for (U32 i = 0; i < numChunks; i++)
    {
        U32 chunkType;
        if (!stream.read(&chunkType))
            return(false);

        // MissionChunk must be first chunk
        if (i == 0 && chunkType != PersistChunk::MissionChunkType)
            return(false);
        if (i != 0 && chunkType == PersistChunk::MissionChunkType)
            return(false);

        // create the chunk
        switch (chunkType)
        {
            case PersistChunk::MissionChunkType:
                mChunks.push_back(new SceneLighting::PersistInfo::MissionChunk);
                break;

            case PersistChunk::InteriorChunkType:
                mChunks.push_back(new SceneLighting::PersistInfo::InteriorChunk);
                break;

            case PersistChunk::TerrainChunkType:
                mChunks.push_back(new SceneLighting::PersistInfo::TerrainChunk);
                break;

            default:
                return(false);
                break;
        }

        // load the chunk info
        if (!mChunks[i]->read(stream))
            return(false);
    }

    return(true);
}

bool SceneLighting::PersistInfo::write(Stream& stream)
{
    if (!stream.write(smFileVersion))
        return(false);

    if (!stream.write((U32)mChunks.size()))
        return(false);

    for (U32 i = 0; i < mChunks.size(); i++)
    {
        if (!stream.write(mChunks[i]->mChunkType))
            return(false);
        if (!mChunks[i]->write(stream))
            return(false);
    }

    return(true);
}

//------------------------------------------------------------------------------
// Class SceneLighting::PersistInfo::PersistChunk
//------------------------------------------------------------------------------
bool SceneLighting::PersistInfo::PersistChunk::read(Stream& stream)
{
    if (!stream.read(&mChunkCRC))
        return(false);
    return(true);
}

bool SceneLighting::PersistInfo::PersistChunk::write(Stream& stream)
{
    if (!stream.write(mChunkCRC))
        return(false);
    return(true);
}

//------------------------------------------------------------------------------
// Class SceneLighting::PersistInfo::MissionChunk
//------------------------------------------------------------------------------
SceneLighting::PersistInfo::MissionChunk::MissionChunk()
{
    mChunkType = PersistChunk::MissionChunkType;
}

//------------------------------------------------------------------------------
// Class SceneLighting::PersistInfo::InteriorChunk
//------------------------------------------------------------------------------
SceneLighting::PersistInfo::InteriorChunk::InteriorChunk()
{
    mChunkType = PersistChunk::InteriorChunkType;
}

SceneLighting::PersistInfo::InteriorChunk::~InteriorChunk()
{
    for (U32 i = 0; i < mLightmaps.size(); i++)
        delete mLightmaps[i];
}

//------------------------------------------------------------------------------
// - always read in vertex lighting, lightmaps may not be needed
bool SceneLighting::PersistInfo::InteriorChunk::read(Stream& stream)
{
    if (!Parent::read(stream))
        return(false);

    U32 size;
    U32 i;

    // lightmaps->vertex-info
    if (!SceneLighting::smUseVertexLighting)
    {
        // size of this minichunk
        if (!stream.read(&size))
            return(false);

        // lightmaps
        stream.read(&size);
        mDetailLightmapCount.setSize(size);
        for (i = 0; i < size; i++)
            if (!stream.read(&mDetailLightmapCount[i]))
                return(false);

        stream.read(&size);
        mDetailLightmapIndices.setSize(size);
        for (i = 0; i < size; i++)
            if (!stream.read(&mDetailLightmapIndices[i]))
                return(false);

        if (!stream.read(&size))
            return(false);
        mLightmaps.setSize(size);

        for (i = 0; i < size; i++)
        {
            mLightmaps[i] = new GBitmap;
            if (!mLightmaps[i]->readPNG(stream))
                return(false);
        }
    }
    else
    {
        // step past the lightmaps
        if (!stream.read(&size))
            return(false);
        if (!stream.setPosition(stream.getPosition() + size))
            return(false);
    }

    // size of the vertex lighting: need to reset stream position after zipStream reading
    U32 zipStreamEnd;
    if (!stream.read(&zipStreamEnd))
        return(false);
    zipStreamEnd += stream.getPosition();

    /*
       // vertex lighting
       ZipSubRStream zipStream;
       if(!zipStream.attachStream(&stream))
          return(false);

       if(!zipStream.read(&size))
          return(false);
       mHasAlarmState = bool(size);

       if(!zipStream.read(&size))
          return(false);
       mDetailVertexCount.setSize(size);
       for(i = 0; i < size; i++)
          if(!zipStream.read(&mDetailVertexCount[i]))
             return(false);

       size = 0;
       for(i = 0; i < mDetailVertexCount.size(); i++)
          size += mDetailVertexCount[i];

       mVertexColorsNormal.setSize(size);

       if(mHasAlarmState)
          mVertexColorsAlarm.setSize(size);

       U32 curPos = 0;
       for(i = 0; i < mDetailVertexCount.size(); i++)
       {
          U32 count = mDetailVertexCount[i];
          for(U32 j = 0; j < count; j++)
             if(!zipStream.read(&mVertexColorsNormal[curPos + j]))
                return(false);

          // read in the alarm info
          if(mHasAlarmState)
          {
             // same?
             if(!zipStream.read(&size))
                return(false);
             if(bool(size))
                dMemcpy(&mVertexColorsAlarm[curPos], &mVertexColorsNormal[curPos], count * sizeof(ColorI));
             else
             {
                for(U32 j = 0; j < count; j++)
                   if(!zipStream.read(&mVertexColorsAlarm[curPos + j]))
                      return(false);
             }
          }

          curPos += count;
       }

       zipStream.detachStream();
    */

    // since there is no resizeFilterStream the zipStream happily reads
    // off the end of the compressed block... reset the position
    stream.setPosition(zipStreamEnd);

    return(true);
}

bool SceneLighting::PersistInfo::InteriorChunk::write(Stream& stream)
{
    if (!Parent::write(stream))
        return(false);

    // lightmaps
    U32 startPos = stream.getPosition();
    if (!stream.write(U32(0)))
        return(false);

    U32 i;
    if (!stream.write(U32(mDetailLightmapCount.size())))
        return(false);
    for (i = 0; i < mDetailLightmapCount.size(); i++)
        if (!stream.write(mDetailLightmapCount[i]))
            return(false);

    if (!stream.write(U32(mDetailLightmapIndices.size())))
        return(false);
    for (i = 0; i < mDetailLightmapIndices.size(); i++)
        if (!stream.write(mDetailLightmapIndices[i]))
            return(false);

    if (!stream.write(U32(mLightmaps.size())))
        return(false);
    for (i = 0; i < mLightmaps.size(); i++)
    {
        AssertFatal(mLightmaps[i], "SceneLighting::SceneLighting::PersistInfo::InteriorChunk::Write: Invalid bitmap!");
        if (!mLightmaps[i]->writePNG(stream))
            return(false);
    }

    // write out the lightmap portions size
    U32 endPos = stream.getPosition();
    if (!stream.setPosition(startPos))
        return(false);

    // don't include the offset in the size
    if (!stream.write(U32(endPos - startPos - sizeof(U32))))
        return(false);
    if (!stream.setPosition(endPos))
        return(false);


    // vertex lighting: needs the size of the vertex info because the
    // zip stream may read off the end of the chunk
    startPos = stream.getPosition();
    if (!stream.write(U32(0)))
        return(false);

    // write out the vertex lighting portions size
    endPos = stream.getPosition();
    if (!stream.setPosition(startPos))
        return(false);

    // don't include the offset in the size
    if (!stream.write(U32(endPos - startPos - sizeof(U32))))
        return(false);
    if (!stream.setPosition(endPos))
        return(false);

    return(true);
}

//------------------------------------------------------------------------------
// Class SceneLighting::PersistInfo::TerrainChunk
//------------------------------------------------------------------------------
SceneLighting::PersistInfo::TerrainChunk::TerrainChunk()
{
    mChunkType = PersistChunk::TerrainChunkType;
    mLightmap = NULL;
}

SceneLighting::PersistInfo::TerrainChunk::~TerrainChunk()
{
    if (mLightmap)
        delete mLightmap;
}

//------------------------------------------------------------------------------

bool SceneLighting::PersistInfo::TerrainChunk::read(Stream& stream)
{
    if (!Parent::read(stream))
        return(false);

    mLightmap = new GBitmap();
    return mLightmap->readPNG(stream);
}

bool SceneLighting::PersistInfo::TerrainChunk::write(Stream& stream)
{
    if (!Parent::write(stream))
        return(false);

    if (!mLightmap)
        return(false);

    if (!mLightmap->writePNG(stream))
        return(false);

    // Debug dump...
 /*
    FileStream *f = new FileStream();
    if(f->open("terrlight.png", FileStream::Write))
    {
       mLightmap->writePNG(*f);
       f->close();
    }
 */

    return(true);
}

//------------------------------------------------------------------------------
bool SceneLighting::verifyMissionInfo(SceneLighting::PersistInfo::PersistChunk* chunk)
{
    SceneLighting::PersistInfo::MissionChunk* info = dynamic_cast<SceneLighting::PersistInfo::MissionChunk*>(chunk);
    if (!info)
        return(false);

    SceneLighting::PersistInfo::MissionChunk curInfo;
    if (!getMissionInfo(&curInfo))
        return(false);

    return(curInfo.mChunkCRC == info->mChunkCRC);
}

bool SceneLighting::getMissionInfo(SceneLighting::PersistInfo::PersistChunk* chunk)
{
    SceneLighting::PersistInfo::MissionChunk* info = dynamic_cast<SceneLighting::PersistInfo::MissionChunk*>(chunk);
    if (!info)
        return(false);

    info->mChunkCRC = gConnectionMissionCRC ^ PersistInfo::smFileVersion;
    return(true);
}

//------------------------------------------------------------------------------
bool SceneLighting::loadPersistInfo(const char* fileName)
{
    // open the file
    Stream* stream = 0;
    stream = ResourceManager->openStream(fileName);
    if (!stream)
        return(false);

    PersistInfo persistInfo;
    bool success = persistInfo.read(*stream);
    delete stream;
    if (!success)
        return(false);

    // verify the mission chunk
    if (!verifyMissionInfo(persistInfo.mChunks[0]))
        return(false);

    if (mSceneObjects.size() != (persistInfo.mChunks.size() - 1))
        return(false);

    Vector<SceneLighting::PersistInfo::PersistChunk*> chunks;

    // ensure that the scene objects are in the same order as the chunks
    //  - different instances will depend on this
    U32 i;
    for (i = 0; i < mSceneObjects.size(); i++)
    {
        // 0th chunk is the mission chunk
        U32 chunkIdx = i + 1;
        if (chunkIdx >= persistInfo.mChunks.size())
            return(false);

        if (!mSceneObjects[i]->isValidChunk(persistInfo.mChunks[chunkIdx]))
            return(false);
        chunks.push_back(persistInfo.mChunks[chunkIdx]);
    }

    // get the objects to load in the persisted chunks
    for (i = 0; i < mSceneObjects.size(); i++)
        if (!mSceneObjects[i]->setPersistInfo(chunks[i]))
            return(false);

    return(true);
}

bool SceneLighting::savePersistInfo(const char* fileName)
{
    // open the file
    FileStream file;
    if (!ResourceManager->openFileForWrite(file, fileName))
        return(false);

    PersistInfo persistInfo;

    // add in the mission chunk
    persistInfo.mChunks.push_back(new SceneLighting::PersistInfo::MissionChunk);

    // get the mission info, will return false when there are 0 lights
    if (!getMissionInfo(persistInfo.mChunks[0]))
        return(false);

    // get all the persist chunks
    for (U32 i = 0; i < mSceneObjects.size(); i++)
    {
        if (isInterior(mSceneObjects[i]->mObj))
            persistInfo.mChunks.push_back(new SceneLighting::PersistInfo::InteriorChunk);
        else if (isTerrain(mSceneObjects[i]->mObj))
            persistInfo.mChunks.push_back(new SceneLighting::PersistInfo::TerrainChunk);
        else
            return(false);

        if (!mSceneObjects[i]->getPersistInfo(persistInfo.mChunks.last()))
            return(false);
    }

    if (!persistInfo.write(file))
        return(false);

    file.close();

    // open/close the stream to get the fileSize calculated on the resource object
    ResourceManager->closeStream(ResourceManager->openStream(fileName));
    return(true);
}

//------------------------------------------------------------------------------
// SceneLighting
//------------------------------------------------------------------------------

void SceneLighting::addInterior(ShadowVolumeBSP* shadowVolume, InteriorProxy& interior, LightInfo* light, S32 level)
{
    if (light->mType != LightInfo::Vector)
        return;

    ColorF ambient = light->mAmbient;

    bool shadowedTree = true;

    // check if just getting shadow detail
    if (level == SHADOW_DETAIL)
    {
        shadowedTree = false;
        level = interior->mInteriorRes->getNumDetailLevels() - 1;
    }

    Interior* detail = interior->mInteriorRes->getDetailLevel(level);
    bool hasAlarm = detail->hasAlarmState();

    // make sure surfaces do not get processed more than once
    BitVector surfaceProcessed;
    surfaceProcessed.setSize(detail->mSurfaces.size());
    surfaceProcessed.clear();

    // go through the zones of the interior and grab outside visible surfaces
    for (U32 i = 0; i < detail->getNumZones(); i++)
    {
        Interior::Zone& zone = detail->mZones[i];
        for (U32 j = 0; j < zone.surfaceCount; j++)
        {
            U32 surfaceIndex = detail->mZoneSurfaces[zone.surfaceStart + j];

            // dont reprocess a surface
            if (surfaceProcessed.test(surfaceIndex))
                continue;
            surfaceProcessed.set(surfaceIndex);

            Interior::Surface& surface = detail->mSurfaces[surfaceIndex];

            // outside visible?
            if (!(surface.surfaceFlags & Interior::SurfaceOutsideVisible))
                continue;

            // good surface?
            PlaneF plane = detail->getPlane(surface.planeIndex);
            if (Interior::planeIsFlipped(surface.planeIndex))
                plane.neg();

            // project the plane
            PlaneF projPlane;
            mTransformPlane(interior->getTransform(), interior->getScale(), plane, &projPlane);

            // fill with ambient? (need to do here, because surface will not be
            // added to the SVBSP tree)
            F32 dot = mDot(projPlane, light->mDirection);
            if (dot > -gParellelVectorThresh && !(GFX->getPixelShaderVersion() > 0.0))
            {
                if (shadowedTree)
                {
                    // alarm lighting
                    GFXTexHandle normHandle = gInteriorLMManager.duplicateBaseLightmap(detail->getLMHandle(), interior->getLMHandle(), detail->getNormalLMapIndex(surfaceIndex));
                    GFXTexHandle alarmHandle;

                    GBitmap* normLightmap = normHandle->getBitmap();
                    GBitmap* alarmLightmap = 0;

                    // check if they share the lightmap
                    if (hasAlarm)
                    {
                        if (detail->getNormalLMapIndex(surfaceIndex) != detail->getAlarmLMapIndex(surfaceIndex))
                        {
                            alarmHandle = gInteriorLMManager.duplicateBaseLightmap(detail->getLMHandle(), interior->getLMHandle(), detail->getAlarmLMapIndex(surfaceIndex));
                            alarmLightmap = alarmHandle->getBitmap();
                        }
                    }

                    // attemp to light normal and alarm lighting
                    for (U32 c = 0; c < 2; c++)
                    {
                        GBitmap* lightmap = (c == 0) ? normLightmap : alarmLightmap;
                        if (!lightmap)
                            continue;

                        // fill it
                        for (U32 y = 0; y < surface.mapSizeY; y++)
                        {
                            ColorI color = light->mAmbient;
                            U8* pBits = lightmap->getAddress(surface.mapOffsetX, surface.mapOffsetY + y);
                            for (U32 x = 0; x < surface.mapSizeX; x++)
                            {
#ifdef SET_COLORS
                                * pBits++ = color.red;
                                *pBits++ = color.green;
                                *pBits++ = color.blue;
#else

                                // the previous *pBit++ = ... code is broken.
                                U32 _r = static_cast<U32>(color.red) + static_cast<U32>(*pBits);
                                *pBits = (_r <= 255) ? _r : 255;
                                pBits++;

                                U32 _g = static_cast<U32>(color.green) + static_cast<U32>(*pBits);
                                *pBits = (_g <= 255) ? _g : 255;
                                pBits++;

                                U32 _b = static_cast<U32>(color.blue) + static_cast<U32>(*pBits);
                                *pBits = (_b <= 255) ? _b : 255;
                                pBits++;

#endif
                            }
                        }
                    }
                }
                continue;
            }

            ShadowVolumeBSP::SVPoly* poly = buildInteriorPoly(shadowVolume, interior, detail,
                surfaceIndex, light, shadowedTree);

            // insert it into the SVBSP tree
            shadowVolume->insertPoly(poly);
        }
    }
}

//------------------------------------------------------------------------------
ShadowVolumeBSP::SVPoly* SceneLighting::buildInteriorPoly(ShadowVolumeBSP* shadowVolumeBSP,
    InteriorProxy& interior, Interior* detail, U32 surfaceIndex, LightInfo* light,
    bool createSurfaceInfo)
{
    // transform and add the points...
    const MatrixF& transform = interior->getTransform();
    const VectorF& scale = interior->getScale();

    const Interior::Surface& surface = detail->mSurfaces[surfaceIndex];

    ShadowVolumeBSP::SVPoly* poly = shadowVolumeBSP->createPoly();

    poly->mWindingCount = surface.windingCount;

    // project these points
    for (U32 j = 0; j < poly->mWindingCount; j++)
    {
        Point3F iPnt = detail->mPoints[detail->mWindings[surface.windingStart + j]].point;
        Point3F tPnt;
        iPnt.convolve(scale);
        transform.mulP(iPnt, &tPnt);
        poly->mWinding[j] = tPnt;
    }

    // convert from fan
    U32 tmpIndices[ShadowVolumeBSP::SVPoly::MaxWinding];
    Point3F fanIndices[ShadowVolumeBSP::SVPoly::MaxWinding];

    tmpIndices[0] = 0;

    U32 idx = 1;
    U32 i;
    for (i = 1; i < poly->mWindingCount; i += 2)
        tmpIndices[idx++] = i;
    for (i = ((poly->mWindingCount - 1) & (~0x1)); i > 0; i -= 2)
        tmpIndices[idx++] = i;

    idx = 0;
    for (i = 0; i < poly->mWindingCount; i++)
        if (surface.fanMask & (1 << i))
            fanIndices[idx++] = poly->mWinding[tmpIndices[i]];

    // set the data
    poly->mWindingCount = idx;
    for (i = 0; i < poly->mWindingCount; i++)
        poly->mWinding[i] = fanIndices[i];

    // flip the plane - shadow volumes face inwards
    PlaneF plane = detail->getPlane(surface.planeIndex);
    if (!Interior::planeIsFlipped(surface.planeIndex))
        plane.neg();

    // transform the plane
    mTransformPlane(transform, scale, plane, &poly->mPlane);
    shadowVolumeBSP->buildPolyVolume(poly, light);

    // do surface info?
    if (createSurfaceInfo)
    {
        ShadowVolumeBSP::SurfaceInfo* surfaceInfo = new ShadowVolumeBSP::SurfaceInfo;
        shadowVolumeBSP->mSurfaces.push_back(surfaceInfo);

        // fill it
        surfaceInfo->mSurfaceIndex = surfaceIndex;
        surfaceInfo->mShadowVolume = shadowVolumeBSP->getShadowVolume(poly->mShadowVolume);

        // POLY and POLY node gets it too
        ShadowVolumeBSP::SVNode* traverse = shadowVolumeBSP->getShadowVolume(poly->mShadowVolume);
        while (traverse->mFront)
        {
            traverse->mSurfaceInfo = surfaceInfo;
            traverse = traverse->mFront;
        }

        // get some info from the poly node
        poly->mSurfaceInfo = traverse->mSurfaceInfo = surfaceInfo;
        surfaceInfo->mPlaneIndex = traverse->mPlaneIndex;
    }

    return(poly);
}

//--------------------------------------------------------------------------
static S32 QSORT_CALLBACK compareS32(const void* a, const void* b)
{
    return(*((S32*)a) - *((S32*)b));
}

U32 SceneLighting::calcMissionCRC()
{
    // all the objects + mission chunk
    Vector<U32> crc;

    // grab the object crcs
    for (U32 i = 0; i < mSceneObjects.size(); i++)
        crc.push_back(mSceneObjects[i]->mChunkCRC);

    // grab the missions crc
    SceneLighting::PersistInfo::MissionChunk curInfo;
    getMissionInfo(&curInfo);
    crc.push_back(curInfo.mChunkCRC);

    // sort them (order may not have been preserved)
    dQsort(crc.address(), crc.size(), sizeof(U32), compareS32);

    return(calculateCRC(crc.address(), sizeof(U32) * crc.size(), 0xffffffff));
}

bool SceneLighting::light(BitSet32 flags)
{
    if (!gClientSceneGraph)
        return(false);

    mStartTime = Platform::getRealMilliseconds();

    // register static lights
    LightManager* lManager = gClientSceneGraph->getLightManager();
    lManager->registerLights(true);

    // grab all the lights
    mLights.clear();
    lManager->getLights(mLights);


    if (!mLights.size())
        return(false);

    // get all the objects and create proxy's for them
    Vector<SceneObject*>   objects;
    gClientContainer.findObjects(InteriorObjectType | TerrainObjectType, findObjectsCallback, &objects);

    for (SceneObject** itr = objects.begin(); itr != objects.end(); itr++)
    {
        ObjectProxy* proxy;
        if (isInterior(*itr))
            proxy = new InteriorProxy(*itr);
        else if (isTerrain(*itr))
            proxy = new TerrainProxy(*itr);
        else
        {
            AssertFatal(0, "SceneLighting:: invalid object returned from container search");
            continue;
        }

        if (!proxy->calcValidation())
        {
            Con::errorf(ConsoleLogEntry::General, "Failed to calculate validation info for object.  Skipped.");
            delete proxy;
            continue;
        }

        if (!proxy->loadResources())
        {
            Con::errorf(ConsoleLogEntry::General, "Failed to load resources for object.  Skipped.");
            delete proxy;
            continue;
        }

        mSceneObjects.push_back(proxy);
    }

    if (!mSceneObjects.size())
        return(false);

    // grab the missions crc
    U32 missionCRC = calcMissionCRC();

    // remove the '.mis' extension from the mission name
    char misName[256];
    dSprintf(misName, sizeof(misName), "%s", Con::getVariable("$Client::MissionFile"));
    char* dot = dStrstr(misName, (char*)".mis");
    if (dot)
        *dot = '\0';

    // get the mission name
    dSprintf(mFileName, sizeof(mFileName), "%s_%x.ml", misName, missionCRC);
    if (!ResourceManager->isValidWriteFileName(mFileName))
    {
        Con::warnf("Invalid filename '%s'.  Failed to light mission.", mFileName);
        return(false);
    }

    // check for some persisted data, check if being forced..
    if (!flags.test(ForceAlways | ForceWritable))
    {
        if (loadPersistInfo(mFileName))
        {
            Con::printf(" Successfully loaded mission lighting file: '%s'", mFileName);

            // touch this file...
            if (!dFileTouch(mFileName))
                Con::warnf("  Failed to touch file '%s'.  File may be read only.", mFileName);

            return(false);
        }

        // texture manager must have lighting complete now
        if (flags.test(LoadOnly))
        {
            Con::errorf(ConsoleLogEntry::General, "Failed to load mission lighting!");
            return(false);
        }
    }

    // don't light if file is read-only?
    if (!flags.test(ForceAlways))
    {
        FileStream fileStream;
        if (!ResourceManager->openFileForWrite(fileStream, mFileName))
        {
            Con::errorf(ConsoleLogEntry::General, "SceneLighting::Light: Failed to light mission.  File '%s' cannot be written to.", mFileName);
            return(false);
        }
    }

    // initialize the objects for lighting
    for (ObjectProxy** proxyItr = mSceneObjects.begin(); proxyItr != mSceneObjects.end(); proxyItr++)
        (*proxyItr)->init();

    // get things started
    Sim::postEvent(this, new SceneLightingProcessEvent(0, -1), Sim::getTargetTime() + 1);
    return(true);
}

//------------------------------------------------------------------------------
void SceneLighting::processEvent(U32 light, S32 object)
{
    // cancel lighting?
    if (gTerminateLighting)
    {
        completed(false);
        deleteObject();
        return;
    }

    ObjectProxy** proxyItr;

    // last object?
    if (object == mLitObjects.size())
    {
        for (proxyItr = mLitObjects.begin(); proxyItr != mLitObjects.end(); proxyItr++)
        {
            if (!(*proxyItr)->getObject())
            {
                AssertFatal(0, "SceneLighting:: missing sceneobject on light end");
                continue;
            }
            (*proxyItr)->postLight(light == (mLights.size() - 1));
        }
        mLitObjects.clear();

        Canvas->paint();
        Sim::postEvent(this, new SceneLightingProcessEvent(light + 1, -1), Sim::getTargetTime() + 1);
    }
    else
    {
        // done lighting?
        if (light == mLights.size())
        {
            Con::printf(" Scene lit in %3.3f seconds", (Platform::getRealMilliseconds() - mStartTime) / 1000.f);

            // save out the lighting?
            if (Con::getBoolVariable("$pref::sceneLighting::cacheLighting", true))
            {
                if (!savePersistInfo(mFileName))
                    Con::errorf(ConsoleLogEntry::General, "SceneLighting::light: unable to persist lighting!");
                else
                    Con::printf(" Successfully saved mission lighting file: '%s'", mFileName);
            }

            // wrap things up...
            completed(true);
            deleteObject();
        }
        else
        {
            // start of this light?
            if (object == -1)
            {
                for (proxyItr = mSceneObjects.begin(); proxyItr != mSceneObjects.end(); proxyItr++)
                {
                    if (!(*proxyItr)->getObject())
                    {
                        AssertFatal(0, "SceneLighting:: missing sceneobject on light start");
                        Con::errorf(ConsoleLogEntry::General, "SceneLighting:: missing sceneobject on light start");
                        continue;
                    }
                    if ((*proxyItr)->preLight(mLights[light]))
                        mLitObjects.push_back(*proxyItr);
                }
            }
            else
            {
                if (mLitObjects[object]->getObject())
                {
                    gLightingProgress = (F32(light) / F32(mLights.size())) + ((F32(object + 1) / F32(mLitObjects.size())) / F32(mLights.size()));
                    mLitObjects[object]->light(mLights[light]);
                }
                else
                {
                    AssertFatal(0, "SceneLighting:: missing sceneobject on light update");
                    Con::errorf(ConsoleLogEntry::General, "SceneLighting:: missing sceneobject on light update");
                }
            }

            Canvas->paint();
            Sim::postEvent(this, new SceneLightingProcessEvent(light, object + 1), Sim::getTargetTime() + 1);
        }
    }
}

//------------------------------------------------------------------------------

struct CacheEntry {
    ResourceObject* mFileObject;
    const char* mFileName;

    CacheEntry() {
        mFileObject = 0;
        mFileName = 0;
    };
};

// object list sort methods: want list in reverse
static int QSORT_CALLBACK minSizeSort(const void* p1, const void* p2)
{
    const CacheEntry* entry1 = (const CacheEntry*)p1;
    const CacheEntry* entry2 = (const CacheEntry*)p2;

    return(entry2->mFileObject->fileSize - entry1->mFileObject->fileSize);
}

static int QSORT_CALLBACK maxSizeSort(const void* p1, const void* p2)
{
    const CacheEntry* entry1 = (const CacheEntry*)p1;
    const CacheEntry* entry2 = (const CacheEntry*)p2;

    return(entry1->mFileObject->fileSize - entry2->mFileObject->fileSize);
}

static int QSORT_CALLBACK lastCreatedSort(const void* p1, const void* p2)
{
    const CacheEntry* entry1 = (const CacheEntry*)p1;
    const CacheEntry* entry2 = (const CacheEntry*)p2;

    FileTime create[2];
    FileTime modify;

    bool ret[2];

    ret[0] = Platform::getFileTimes(entry1->mFileName, &create[0], &modify);
    ret[1] = Platform::getFileTimes(entry2->mFileName, &create[1], &modify);

    // check return values
    if (!ret[0] && !ret[1])
        return(0);
    if (!ret[0])
        return(1);
    if (!ret[1])
        return(-1);

    return(Platform::compareFileTimes(create[1], create[0]));
}

static int QSORT_CALLBACK lastModifiedSort(const void* p1, const void* p2)
{
    const CacheEntry* entry1 = (const CacheEntry*)p1;
    const CacheEntry* entry2 = (const CacheEntry*)p2;

    FileTime create;
    FileTime modify[2];

    bool ret[2];

    ret[0] = Platform::getFileTimes(entry1->mFileName, &create, &modify[0]);
    ret[1] = Platform::getFileTimes(entry2->mFileName, &create, &modify[1]);

    // check return values
    if (!ret[0] && !ret[1])
        return(0);
    if (!ret[0])
        return(1);
    if (!ret[1])
        return(-1);

    return(Platform::compareFileTimes(modify[1], modify[0]));
}

void SceneLighting::processCache()
{
    // get size in kb
    S32 quota = Con::getIntVariable("$pref::sceneLighting::cacheSize", -1);

    Vector<CacheEntry> files;

    ResourceObject* match = 0;
    const char* name;

    S32 curCacheSize = 0;
    match = ResourceManager->findMatch("*.ml", &name, 0);
    while (match)
    {
        if (match->flags & ResourceObject::File)
        {
            // dont allow the current file to be removed...
            if (!dStrstr(name, mFileName))
            {
                CacheEntry entry;
                entry.mFileObject = match;

                // get out of vfs...
                char fileName[1024];
                dSprintf(fileName, sizeof(fileName), "%s/%s", match->path, match->name);

                entry.mFileName = StringTable->insert(fileName);
                files.push_back(entry);
            }
            else
                curCacheSize += match->fileSize;
        }

        match = ResourceManager->findMatch("*.ml", &name, match);
    }

    // remove old files
    for (S32 i = files.size() - 1; i >= 0; i--)
    {
        char buf[1024];
        dSprintf(buf, sizeof(buf), "%s/%s", files[i].mFileObject->path, files[i].mFileObject->name);

        Stream* stream = ResourceManager->openStream(buf);
        if (!stream)
            continue;

        // read in the version
        U32 version;
        bool ok = (stream->read(&version) && (version == SceneLighting::PersistInfo::smFileVersion));
        ResourceManager->closeStream(stream);

        // ok?
        if (ok)
            continue;

        // delete the file
        ResourceManager->freeResource(files[i].mFileObject);

        // no sneaky names
        if (!dStrstr(files[i].mFileName, ".."))
        {
            Con::warnf("Removing old lighting file '%s'.", files[i].mFileName);
            dFileDelete(files[i].mFileName);
        }

        files.pop_back();
    }

    // no size restriction?
    if (quota == -1 || !files.size())
        return;

    for (U32 i = 0; i < files.size(); i++)
        curCacheSize += files[i].mFileObject->fileSize;

    // need to remove?
    if (quota > (curCacheSize >> 10))
        return;

    // sort the entries by the correct method
    const char* purgeMethod = Con::getVariable("$pref::sceneLighting::purgeMethod");
    if (!purgeMethod)
        purgeMethod = "";

    // determine the method (default to least recently used)
    if (!dStricmp(purgeMethod, "minSize"))
        dQsort(files.address(), files.size(), sizeof(CacheEntry), minSizeSort);
    else if (!dStricmp(purgeMethod, "maxSize"))
        dQsort(files.address(), files.size(), sizeof(CacheEntry), maxSizeSort);
    else if (!dStricmp(purgeMethod, "lastCreated"))
        dQsort(files.address(), files.size(), sizeof(CacheEntry), lastCreatedSort);
    else
        dQsort(files.address(), files.size(), sizeof(CacheEntry), lastModifiedSort);

    // go through and remove the best candidate first (sorted reverse)
    while (((curCacheSize >> 10) > quota) && files.size())
    {
        curCacheSize -= files.last().mFileObject->fileSize;
        ResourceManager->freeResource(files.last().mFileObject);

        // no sneaky names
        if (!dStrstr(files.last().mFileName, ".."))
        {
            Con::warnf("Removing lighting file '%s'.", files.last().mFileName);
            dFileDelete(files.last().mFileName);
        }

        files.pop_back();
    }
}

//------------------------------------------------------------------------------
// Class SceneLighting::ObjectProxy:
//------------------------------------------------------------------------------
bool SceneLighting::ObjectProxy::calcValidation()
{
    mChunkCRC = getResourceCRC();
    if (!mChunkCRC)
        return(false);

    return(true);
}

bool SceneLighting::ObjectProxy::isValidChunk(PersistInfo::PersistChunk* chunk)
{
    return(chunk->mChunkCRC == mChunkCRC);
}

bool SceneLighting::ObjectProxy::getPersistInfo(PersistInfo::PersistChunk* chunk)
{
    chunk->mChunkCRC = mChunkCRC;
    return(true);
}

bool SceneLighting::ObjectProxy::setPersistInfo(PersistInfo::PersistChunk* chunk)
{
    mChunkCRC = chunk->mChunkCRC;
    return(true);
}

//------------------------------------------------------------------------------
// Class SceneLighting::InteriorProxy:
//------------------------------------------------------------------------------
SceneLighting::InteriorProxy::InteriorProxy(SceneObject* obj) :
    Parent(obj)
{
    mBoxShadowBSP = 0;
}

SceneLighting::InteriorProxy::~InteriorProxy()
{
    delete mBoxShadowBSP;
}

bool SceneLighting::InteriorProxy::loadResources()
{
    InteriorInstance* interior = getObject();
    if (!interior)
        return(false);

    Resource<InteriorResource>& interiorRes = interior->getResource();
    if (!bool(interiorRes))
        return(false);

    return(true);
}

void SceneLighting::InteriorProxy::init()
{
    InteriorInstance* interior = getObject();
    if (!interior)
        return;
}

bool SceneLighting::InteriorProxy::preLight(LightInfo* light)
{
    // create shadow volume of the bounding box of this object
    InteriorInstance* interior = getObject();
    if (!interior)
        return(false);

    if (light->mType != LightInfo::Vector)
        return(false);

    // reset
    mLitBoxSurfaces.clear();

    const Box3F& objBox = interior->getObjBox();
    const MatrixF& objTransform = interior->getTransform();
    const VectorF& objScale = interior->getScale();

    // grab the surfaces which form the shadow volume
    U32 numPlanes = 0;
    PlaneF testPlanes[3];
    U32 planeIndices[3];

    // grab the bounding planes which face the light
    U32 i;
    for (i = 0; (i < 6) && (numPlanes < 3); i++)
    {
        PlaneF plane;
        plane.x = BoxNormals[i].x;
        plane.y = BoxNormals[i].y;
        plane.z = BoxNormals[i].z;

        if (i & 1)
            plane.d = (((const float*)objBox.min)[(i - 1) >> 1]);
        else
            plane.d = -(((const float*)objBox.max)[i >> 1]);

        // project
        mTransformPlane(objTransform, objScale, plane, &testPlanes[numPlanes]);

        planeIndices[numPlanes] = i;

        if (mDot(testPlanes[numPlanes], light->mDirection) < gParellelVectorThresh)
            numPlanes++;
    }
    AssertFatal(numPlanes, "SceneLighting::InteriorProxy::preLight: no planes found");

    // project the points
    Point3F projPnts[8];
    for (i = 0; i < 8; i++)
    {
        Point3F pnt;
        pnt.set(BoxPnts[i].x ? objBox.max.x : objBox.min.x,
            BoxPnts[i].y ? objBox.max.y : objBox.min.y,
            BoxPnts[i].z ? objBox.max.z : objBox.min.z);

        // scale it
        pnt.convolve(objScale);
        objTransform.mulP(pnt, &projPnts[i]);
    }

    mBoxShadowBSP = new ShadowVolumeBSP;

    // insert the shadow volumes for the surfaces
    for (i = 0; i < numPlanes; i++)
    {
        ShadowVolumeBSP::SVPoly* poly = mBoxShadowBSP->createPoly();
        poly->mWindingCount = 4;

        U32 j;
        for (j = 0; j < 4; j++)
            poly->mWinding[j] = projPnts[BoxVerts[planeIndices[i]][j]];

        testPlanes[i].neg();
        poly->mPlane = testPlanes[i];

        mBoxShadowBSP->buildPolyVolume(poly, light);
        mLitBoxSurfaces.push_back(mBoxShadowBSP->copyPoly(poly));
        mBoxShadowBSP->insertPoly(poly);

        // create the opposite planes for testing against terrain
        Point3F pnts[3];
        for (j = 0; j < 3; j++)
            pnts[j] = projPnts[BoxVerts[planeIndices[i] ^ 1][j]];
        PlaneF plane(pnts[2], pnts[1], pnts[0]);
        mOppositeBoxPlanes.push_back(plane);
    }

    // grab the unique planes for terrain checks
    for (i = 0; i < numPlanes; i++)
    {
        U32 mask = 0;
        for (U32 j = 0; j < numPlanes; j++)
            mask |= BoxSharedEdgeMask[planeIndices[i]][planeIndices[j]];

        ShadowVolumeBSP::SVNode* traverse = mBoxShadowBSP->getShadowVolume(mLitBoxSurfaces[i]->mShadowVolume);
        while (traverse->mFront)
        {
            if (!(mask & 1))
                mTerrainTestPlanes.push_back(mBoxShadowBSP->getPlane(traverse->mPlaneIndex));

            mask >>= 1;
            traverse = traverse->mFront;
        }
    }

    // there will be 2 duplicate node planes if there were only 2 planes lit
    if (numPlanes == 2)
    {
        for (S32 i = 0; i < mTerrainTestPlanes.size(); i++)
            for (U32 j = 0; j < mTerrainTestPlanes.size(); j++)
            {
                if (i == j)
                    continue;

                if ((mDot(mTerrainTestPlanes[i], mTerrainTestPlanes[j]) > gPlaneNormThresh) &&
                    (mFabs(mTerrainTestPlanes[i].d - mTerrainTestPlanes[j].d) < gPlaneDistThresh))
                {
                    mTerrainTestPlanes.erase(i);
                    i--;
                    break;
                }
            }
    }

    return(true);
}

bool SceneLighting::InteriorProxy::isShadowedBy(InteriorProxy* test)
{
    // add if overlapping world box
    if ((*this)->getWorldBox().isOverlapped((*test)->getWorldBox()))
        return(true);

    // test the box shadow volume
    for (U32 i = 0; i < mLitBoxSurfaces.size(); i++)
    {
        ShadowVolumeBSP::SVPoly* poly = mBoxShadowBSP->copyPoly(mLitBoxSurfaces[i]);
        if (test->mBoxShadowBSP->testPoly(poly))
            return(true);
    }

    return(false);
}

void SceneLighting::InteriorProxy::light(LightInfo* light)
{
    InteriorInstance* interior = getObject();
    if (!interior)
        return;

    ColorF ambient = light->mAmbient;

    S32 time = Platform::getRealMilliseconds();

    // create own shadow volume
    ShadowVolumeBSP shadowVolume;

    // add the other objects lit surfaces into shadow volume
    for (ObjectProxy** itr = gLighting->mLitObjects.begin(); itr != gLighting->mLitObjects.end(); itr++)
    {
        if (!(*itr)->getObject())
            continue;

        if (gLighting->isInterior((*itr)->mObj))
        {
            if (*itr == this)
                continue;

            if (isShadowedBy(static_cast<InteriorProxy*>(*itr)))
                gLighting->addInterior(&shadowVolume, *static_cast<InteriorProxy*>(*itr), light, SceneLighting::SHADOW_DETAIL);
        }

        // insert the terrain squares
        if (gLighting->isTerrain((*itr)->mObj))
        {
            TerrainProxy* terrain = static_cast<TerrainProxy*>(*itr);

            Vector<PlaneF> clipPlanes;
            clipPlanes = mTerrainTestPlanes;
            for (U32 i = 0; i < mOppositeBoxPlanes.size(); i++)
                clipPlanes.push_back(mOppositeBoxPlanes[i]);

            Vector<U16> shadowList;
            if (terrain->getShadowedSquares(clipPlanes, shadowList))
            {
                TerrainBlock* block = static_cast<TerrainBlock*>((*itr)->getObject());
                Point3F offset;
                block->getTransform().getColumn(3, &offset);

                F32 squareSize = block->getSquareSize();

                for (U32 j = 0; j < shadowList.size(); j++)
                {
                    Point2I pos(shadowList[j] & TerrainBlock::BlockMask, shadowList[j] >> TerrainBlock::BlockShift);
                    Point2F wPos(pos.x * squareSize + offset.x,
                        pos.y * squareSize + offset.y);

                    Point3F pnts[4];
                    pnts[0].set(wPos.x, wPos.y, fixedToFloat(block->getHeight(pos.x, pos.y)));
                    pnts[1].set(wPos.x + squareSize, wPos.y, fixedToFloat(block->getHeight(pos.x + 1, pos.y)));
                    pnts[2].set(wPos.x + squareSize, wPos.y + squareSize, fixedToFloat(block->getHeight(pos.x + 1, pos.y + 1)));
                    pnts[3].set(wPos.x, wPos.y + squareSize, fixedToFloat(block->getHeight(pos.x, pos.y + 1)));

                    GridSquare* gs = block->findSquare(0, pos);

                    U32 squareIdx = (gs->flags & GridSquare::Split45) ? 0 : 2;

                    for (U32 k = squareIdx; k < (squareIdx + 2); k++)
                    {
                        // face plane inwards
                        PlaneF plane(pnts[TerrainSquareIndices[k][2]],
                            pnts[TerrainSquareIndices[k][1]],
                            pnts[TerrainSquareIndices[k][0]]);

                        if (mDot(plane, light->mDirection) > gParellelVectorThresh)
                        {
                            ShadowVolumeBSP::SVPoly* poly = shadowVolume.createPoly();
                            poly->mWindingCount = 3;

                            poly->mWinding[0] = pnts[TerrainSquareIndices[k][0]];
                            poly->mWinding[1] = pnts[TerrainSquareIndices[k][1]];
                            poly->mWinding[2] = pnts[TerrainSquareIndices[k][2]];
                            poly->mPlane = plane;

                            // create the shadow volume for this and insert
                            shadowVolume.buildPolyVolume(poly, light);
                            shadowVolume.insertPoly(poly);
                        }
                    }
                }
            }
        }
    }

    // light all details
    for (U32 i = 0; i < interior->getResource()->getNumDetailLevels(); i++)
    {
        // clear lightmaps
        Interior* detail = interior->getResource()->getDetailLevel(i);
        gInteriorLMManager.clearLightmaps(detail->getLMHandle(), interior->getLMHandle());

        // clear out the last inserted interior
        shadowVolume.removeLastInterior();

        bool hasAlarm = detail->hasAlarmState();

        gLighting->addInterior(&shadowVolume, *this, light, i);

        for (U32 j = 0; j < shadowVolume.mSurfaces.size(); j++)
        {
            ShadowVolumeBSP::SurfaceInfo* surfaceInfo = shadowVolume.mSurfaces[j];

            U32 surfaceIndex = surfaceInfo->mSurfaceIndex;

            const Interior::Surface& surface = detail->getSurface(surfaceIndex);

            // alarm lighting
            GFXTexHandle normHandle = gInteriorLMManager.duplicateBaseLightmap(detail->getLMHandle(), interior->getLMHandle(), detail->getNormalLMapIndex(surfaceIndex));
            GFXTexHandle alarmHandle;

            GBitmap* normLightmap = normHandle->getBitmap();
            GBitmap* alarmLightmap = 0;

            // check if the lightmaps are shared
            if (hasAlarm)
            {
                if (detail->getNormalLMapIndex(surfaceIndex) != detail->getAlarmLMapIndex(surfaceIndex))
                {
                    alarmHandle = gInteriorLMManager.duplicateBaseLightmap(detail->getLMHandle(), interior->getLMHandle(), detail->getAlarmLMapIndex(surfaceIndex));
                    alarmLightmap = alarmHandle->getBitmap();
                }
            }

            // points right way?
            PlaneF plane = detail->getPlane(surface.planeIndex);
            if (Interior::planeIsFlipped(surface.planeIndex))
                plane.neg();

            const MatrixF& transform = interior->getTransform();
            const Point3F& scale = interior->getScale();

            //
            PlaneF projPlane;
            mTransformPlane(transform, scale, plane, &projPlane);

            F32 dot = mDot(projPlane, -light->mDirection);

            // cancel out lambert dot product and ambient lighting on hardware
            // with pixel shaders
            if (GFX->getPixelShaderVersion() > 0.0)
            {
                dot = 1.0;
                ambient.set(0.0, 0.0, 0.0);
            }

            // shadowed?
            if (!surfaceInfo->mShadowed.size())
            {
                // calc the color and convert to U8 rep
                ColorF tmp = (light->mColor * dot) + ambient;
                tmp.clamp();
                ColorI color = tmp;

                // attempt to light both the normal and the alarm states
                for (U32 c = 0; c < 2; c++)
                {
                    GBitmap* lightmap = (c == 0) ? normLightmap : alarmLightmap;
                    if (!lightmap)
                        continue;

                    // fill it
                    for (U32 y = 0; y < surface.mapSizeY; y++)
                    {
                        U8* pBits = lightmap->getAddress(surface.mapOffsetX, surface.mapOffsetY + y);
                        for (U32 x = 0; x < surface.mapSizeX; x++)
                        {
#ifdef SET_COLORS
                            * pBits++ = color.red;
                            *pBits++ = color.green;
                            *pBits++ = color.blue;
#else
                            U32 _r = static_cast<U32>(color.red) + static_cast<U32>(*pBits);
                            *pBits = (_r <= 255) ? _r : 255;
                            pBits++;

                            U32 _g = static_cast<U32>(color.green) + static_cast<U32>(*pBits);
                            *pBits = (_g <= 255) ? _g : 255;
                            pBits++;

                            U32 _b = static_cast<U32>(color.blue) + static_cast<U32>(*pBits);
                            *pBits = (_b <= 255) ? _b : 255;
                            pBits++;
#endif
                        }
                    }
                }

                continue;
            }

            // get the lmagGen...
            const Interior::TexGenPlanes& lmTexGenEQ = detail->getLMTexGenEQ(surfaceIndex);

            const F32* const lGenX = lmTexGenEQ.planeX;
            const F32* const lGenY = lmTexGenEQ.planeY;

            AssertFatal((lGenX[0] * lGenX[1] == 0.f) &&
                (lGenX[0] * lGenX[2] == 0.f) &&
                (lGenX[1] * lGenX[2] == 0.f), "Bad lmTexGen!");
            AssertFatal((lGenY[0] * lGenY[1] == 0.f) &&
                (lGenY[0] * lGenY[2] == 0.f) &&
                (lGenY[1] * lGenY[2] == 0.f), "Bad lmTexGen!");

            // get the axis index for the texgens (could be swapped)
            S32 si;
            S32 ti;
            S32 axis = -1;

            //
            if (lGenX[0] == 0.f && lGenY[0] == 0.f)          // YZ
            {
                axis = 0;
                if (lGenX[1] == 0.f) { // swapped?
                    si = 2;
                    ti = 1;
                }
                else {
                    si = 1;
                    ti = 2;
                }
            }
            else if (lGenX[1] == 0.f && lGenY[1] == 0.f)     // XZ
            {
                axis = 1;
                if (lGenX[0] == 0.f) { // swapped?
                    si = 2;
                    ti = 0;
                }
                else {
                    si = 0;
                    ti = 2;
                }
            }
            else if (lGenX[2] == 0.f && lGenY[2] == 0.f)     // XY
            {
                axis = 2;
                if (lGenX[0] == 0.f) { // swapped?
                    si = 1;
                    ti = 0;
                }
                else {
                    si = 0;
                    ti = 1;
                }
            }
            AssertFatal(!(axis == -1), "SceneLighting::lightInterior: bad TexGen!");

            const F32* pNormal = ((const F32*)plane);

            Point3F start;
            F32* pStart = ((F32*)start);

            F32 lumelScale = 1.0 / (lGenX[si] * normLightmap->getWidth());

            // get the start point on the lightmap
            pStart[si] = (((surface.mapOffsetX * lumelScale) / (1.0 / lGenX[si])) - lGenX[3]) / lGenX[si];
            pStart[ti] = (((surface.mapOffsetY * lumelScale) / (1.0 / lGenY[ti])) - lGenY[3]) / lGenY[ti];
            pStart[axis] = ((pNormal[si] * pStart[si]) + (pNormal[ti] * pStart[ti]) + plane.d) / -pNormal[axis];

            start.convolve(scale);
            transform.mulP(start);

            // get the s/t vecs oriented on the surface
            Point3F sVec;
            Point3F tVec;

            F32* pSVec = ((F32*)sVec);
            F32* pTVec = ((F32*)tVec);

            F32 angle;
            Point3F planeNormal;

            // s
            pSVec[si] = 1.f;
            pSVec[ti] = 0.f;

            planeNormal = plane;
            ((F32*)planeNormal)[ti] = 0.f;
            planeNormal.normalize();

            angle = mAcos(mClampF(((F32*)planeNormal)[axis], -1.f, 1.f));
            pSVec[axis] = (((F32*)planeNormal)[si] < 0.f) ? mTan(angle) : -mTan(angle);

            // t
            pTVec[ti] = 1.f;
            pTVec[si] = 0.f;

            planeNormal = plane;
            ((F32*)planeNormal)[si] = 0.f;
            planeNormal.normalize();

            angle = mAcos(mClampF(((F32*)planeNormal)[axis], -1.f, 1.f));
            pTVec[axis] = (((F32*)planeNormal)[ti] < 0.f) ? mTan(angle) : -mTan(angle);

            // scale the vectors

            sVec *= lumelScale;
            tVec *= lumelScale;

            // project vecs
            transform.mulV(sVec);
            sVec.convolve(scale);

            transform.mulV(tVec);
            tVec.convolve(scale);

            Point3F& curPos = start;
            Point3F sRun = sVec * surface.mapSizeX;

            // get the lexel area
            Point3F cross;
            mCross(sVec, tVec, &cross);
            F32 maxLexelArea = cross.len();

            const PlaneF& surfacePlane = shadowVolume.getPlane(surfaceInfo->mPlaneIndex);

            // get the world coordinate for each lexel
            for (U32 y = 0; y < surface.mapSizeY; y++)
            {
                U8* normBits = normLightmap->getAddress(surface.mapOffsetX, surface.mapOffsetY + y);
                U8* alarmBits = alarmLightmap ? alarmLightmap->getAddress(surface.mapOffsetX, surface.mapOffsetY + y) : 0;

                for (U32 x = 0; x < surface.mapSizeX; x++)
                {
                    ShadowVolumeBSP::SVPoly* poly = shadowVolume.createPoly();
                    poly->mPlane = surfacePlane;
                    poly->mWindingCount = 4;

                    // set the poly indices
                    poly->mWinding[0] = curPos;
                    poly->mWinding[1] = curPos + sVec;
                    poly->mWinding[2] = curPos + sVec + tVec;
                    poly->mWinding[3] = curPos + tVec;

                    //               // insert poly which has been clipped to own shadow volume
                    //               ShadowVolumeBSP::SVPoly * store = 0;
                    //               shadowVolume.clipToSelf(surfaceInfo->mShadowVolume, &store, poly);
                    //
                    //               if(!store)
                    //                  continue;
                    //
                    //               F32 lexelArea = shadowVolume.getPolySurfaceArea(store);
                    //               F32 area = shadowVolume.getLitSurfaceArea(store, surfaceInfo);

                    F32 area = shadowVolume.getLitSurfaceArea(poly, surfaceInfo);
                    F32 shadowScale = mClampF(area / maxLexelArea, 0.f, 1.f);

                    // get the color into U8
                    ColorF tmp = (light->mColor * dot * shadowScale) + ambient;
                    tmp.clamp();
                    ColorI color = tmp;

                    // attempt to light both normal and alarm lightmaps
                    for (U32 c = 0; c < 2; c++)
                    {
                        U8*& pBits = (c == 0) ? normBits : alarmBits;
                        if (!pBits)
                            continue;

#ifdef SET_COLORS
                        * pBits++ = color.red;
                        *pBits++ = color.green;
                        *pBits++ = color.blue;
#else
                        U32 _r = static_cast<U32>(color.red) + static_cast<U32>(*pBits);
                        *pBits = (_r <= 255) ? _r : 255;
                        pBits++;

                        U32 _g = static_cast<U32>(color.green) + static_cast<U32>(*pBits);
                        *pBits = (_g <= 255) ? _g : 255;
                        pBits++;

                        U32 _b = static_cast<U32>(color.blue) + static_cast<U32>(*pBits);
                        *pBits = (_b <= 255) ? _b : 255;
                        pBits++;
#endif
                    }

                    curPos += sVec;
                }

                curPos -= sRun;
                curPos += tVec;
            }
        }
    }

    Con::printf("    = interior lit in %3.3f seconds", (Platform::getRealMilliseconds() - time) / 1000.f);
}

void SceneLighting::InteriorProxy::postLight(bool lastLight)
{
    delete mBoxShadowBSP;
    mBoxShadowBSP = 0;

    InteriorInstance* interior = getObject();
    if (!interior)
        return;
}

//------------------------------------------------------------------------------
U32 SceneLighting::InteriorProxy::getResourceCRC()
{
    InteriorInstance* interior = getObject();
    if (!interior)
        return(0);
    return(interior->getCRC());
}

//------------------------------------------------------------------------------
bool SceneLighting::InteriorProxy::setPersistInfo(SceneLighting::PersistInfo::PersistChunk* info)
{

    if (!Parent::setPersistInfo(info))
        return(false);

    SceneLighting::PersistInfo::InteriorChunk* chunk = dynamic_cast<SceneLighting::PersistInfo::InteriorChunk*>(info);
    AssertFatal(chunk, "SceneLighting::InteriorProxy::setPersistInfo: invalid info chunk!");

    InteriorInstance* interior = getObject();
    if (!interior)
        return(false);

    U32 numDetails = interior->getNumDetailLevels();

    // check the lighting method
    AssertFatal(SceneLighting::smUseVertexLighting == Interior::smUseVertexLighting, "SceneLighting::InteriorProxy::setPersistInfo: invalid vertex lighting state");
    if (SceneLighting::smUseVertexLighting != Interior::smUseVertexLighting)
        return(false);

    /*
       // process the vertex lighting...
       if(chunk->mDetailVertexCount.size() != numDetails)
          return(false);

       AssertFatal(chunk->mVertexColorsNormal.size(), "SceneLighting::InteriorProxy::setPersistInfo: invalid chunk info");
       AssertFatal(!chunk->mHasAlarmState || chunk->mVertexColorsAlarm.size(), "SceneLighting::InteriorProxy::setPersistInfo: invalid chunk info");
       AssertFatal(!(chunk->mHasAlarmState ^ interior->getDetailLevel(0)->hasAlarmState()), "SceneLighting::InteriorProxy::setPersistInfo: invalid chunk info");


       U32 curPos = 0;
       for(U32 i = 0; i < numDetails; i++)
       {
          U32 count = chunk->mDetailVertexCount[i];
          Vector<ColorI>* normal = interior->getVertexColorsNormal(i);
          Vector<ColorI>* alarm  = interior->getVertexColorsAlarm(i);
          AssertFatal(normal != NULL && alarm != NULL, "Error, bad vectors returned!");

          normal->setSize(count);
          dMemcpy(normal->address(), &chunk->mVertexColorsNormal[curPos], count * sizeof(ColorI));

          if(chunk->mHasAlarmState)
          {
             alarm->setSize(count);
             dMemcpy(alarm->address(), &chunk->mVertexColorsAlarm[curPos], count * sizeof(ColorI));
          }

          curPos += count;
       }
    */
    // need lightmaps?
    if (!SceneLighting::smUseVertexLighting)
    {
        if (chunk->mDetailLightmapCount.size() != numDetails)
            return(false);

        LM_HANDLE instanceHandle = interior->getLMHandle();
        U32 idx = 0;

        for (U32 i = 0; i < numDetails; i++)
        {
            Interior* detail = interior->getDetailLevel(i);

            LM_HANDLE interiorHandle = detail->getLMHandle();
            Vector<GFXTexHandle>& baseHandles = gInteriorLMManager.getHandles(interiorHandle, 0);

            if (chunk->mDetailLightmapCount[i] > baseHandles.size())
                return(false);

            for (U32 j = 0; j < chunk->mDetailLightmapCount[i]; j++)
            {
                U32 baseIndex = chunk->mDetailLightmapIndices[idx];
                if (baseIndex >= baseHandles.size())
                    return(false);

                AssertFatal(chunk->mLightmaps[idx], "SceneLighting::InteriorProxy::setPersistInfo: bunk bitmap!");
                if (chunk->mLightmaps[idx]->getWidth() != baseHandles[baseIndex]->getWidth() ||
                    chunk->mLightmaps[idx]->getHeight() != baseHandles[baseIndex]->getHeight())
                    return(false);

                GFXTexHandle tHandle = gInteriorLMManager.duplicateBaseLightmap(interiorHandle, instanceHandle, baseIndex);

                // create the diff bitmap
                U8* pDiff = chunk->mLightmaps[idx]->getAddress(0, 0);
                U8* pBase = baseHandles[baseIndex]->getBitmap()->getAddress(0, 0);
                U8* pDest = tHandle->getBitmap()->getAddress(0, 0);

                Point2I extent(tHandle->getWidth(), tHandle->getHeight());
                for (U32 y = 0; y < extent.y; y++)
                    for (U32 x = 0; x < extent.x; x++)
                    {
                        *pDest++ = *pBase++ + *pDiff++;
                        *pDest++ = *pBase++ + *pDiff++;
                        *pDest++ = *pBase++ + *pDiff++;
                    }

                idx++;
            }
        }
    }

    return(true);
}

bool SceneLighting::InteriorProxy::getPersistInfo(SceneLighting::PersistInfo::PersistChunk* info)
{
    if (!Parent::getPersistInfo(info))
        return(false);

    SceneLighting::PersistInfo::InteriorChunk* chunk = dynamic_cast<SceneLighting::PersistInfo::InteriorChunk*>(info);
    AssertFatal(chunk, "SceneLighting::InteriorProxy::getPersistInfo: invalid info chunk!");

    InteriorInstance* interior = getObject();
    if (!interior)
        return(false);

    LM_HANDLE instanceHandle = interior->getLMHandle();

    AssertFatal(!chunk->mDetailLightmapCount.size(), "SceneLighting::InteriorProxy::getPersistInfo: invalid array!");
    AssertFatal(!chunk->mDetailLightmapIndices.size(), "SceneLighting::InteriorProxy::getPersistInfo: invalid array!");
    AssertFatal(!chunk->mLightmaps.size(), "SceneLighting::InteriorProxy::getPersistInfo: invalid array!");

    U32 numDetails = interior->getNumDetailLevels();
    U32 i;
    for (i = 0; i < numDetails; i++)
    {
        Interior* detail = interior->getDetailLevel(i);
        LM_HANDLE interiorHandle = detail->getLMHandle();

        Vector<GFXTexHandle>& baseHandles = gInteriorLMManager.getHandles(interiorHandle, 0);
        Vector<GFXTexHandle>& instanceHandles = gInteriorLMManager.getHandles(interiorHandle, instanceHandle);

        U32 litCount = 0;

        // walk all the instance lightmaps and grab diff lighting from them
        for (U32 j = 0; j < instanceHandles.size(); j++)
        {
            if (!instanceHandles[j])
                continue;

            litCount++;
            chunk->mDetailLightmapIndices.push_back(j);

            GBitmap* baseBitmap = baseHandles[j]->getBitmap();
            GBitmap* instanceBitmap = instanceHandles[j]->getBitmap();

            Point2I extent(baseBitmap->getWidth(), baseBitmap->getHeight());

            GBitmap* diffLightmap = new GBitmap(extent.x, extent.y, false);

            U8* pBase = baseBitmap->getAddress(0, 0);
            U8* pInstance = instanceBitmap->getAddress(0, 0);
            U8* pDest = diffLightmap->getAddress(0, 0);

            // fill the diff lightmap
            for (U32 y = 0; y < extent.y; y++)
                for (U32 x = 0; x < extent.x; x++)
                {
                    *pDest++ = *pInstance++ - *pBase++;
                    *pDest++ = *pInstance++ - *pBase++;
                    *pDest++ = *pInstance++ - *pBase++;
                }

            chunk->mLightmaps.push_back(diffLightmap);
        }

        chunk->mDetailLightmapCount.push_back(litCount);
    }

    // process the vertex lighting...
    AssertFatal(!chunk->mDetailVertexCount.size(), "SceneLighting::InteriorProxy::getPersistInfo: invalid chunk info");
    AssertFatal(!chunk->mVertexColorsNormal.size(), "SceneLighting::InteriorProxy::getPersistInfo: invalid chunk info");
    AssertFatal(!chunk->mVertexColorsAlarm.size(), "SceneLighting::InteriorProxy::getPersistInfo: invalid chunk info");

    chunk->mHasAlarmState = interior->getDetailLevel(0)->hasAlarmState();
    chunk->mDetailVertexCount.setSize(numDetails);

    U32 size = 0;
    for (i = 0; i < numDetails; i++)
    {
        Interior* detail = interior->getDetailLevel(i);

        U32 count = detail->getWindingCount();
        chunk->mDetailVertexCount[i] = count;
        size += count;
    }

    /*
       chunk->mVertexColorsNormal.setSize(size);
       if(chunk->mHasAlarmState)
          chunk->mVertexColorsAlarm.setSize(size);

       U32 curPos = 0;
       for(i = 0; i < numDetails; i++)
       {
          Vector<ColorI>* normal = interior->getVertexColorsNormal(i);
          Vector<ColorI>* alarm  = interior->getVertexColorsAlarm(i);
          AssertFatal(normal != NULL && alarm != NULL, "Error, no normal or alarm vertex colors!");

          U32 count = chunk->mDetailVertexCount[i];
          dMemcpy(&chunk->mVertexColorsNormal[curPos], normal->address(), count * sizeof(ColorI));

          if(chunk->mHasAlarmState)
             dMemcpy(&chunk->mVertexColorsAlarm[curPos], alarm->address(), count * sizeof(ColorI));

          curPos += count;
       }
    */

    return(true);
}

//-------------------------------------------------------------------------------
// Class SceneLighting::TerrainProxy:
//-------------------------------------------------------------------------------
SceneLighting::TerrainProxy::TerrainProxy(SceneObject* obj) :
    Parent(obj)
{
    mLightmap = 0;
}

SceneLighting::TerrainProxy::~TerrainProxy()
{
    delete[] mLightmap;
}

//-------------------------------------------------------------------------------
void SceneLighting::TerrainProxy::init()
{
    mLightmap = new ColorF[TerrainBlock::LightmapSize * TerrainBlock::LightmapSize];
    dMemset(mLightmap, 0, TerrainBlock::LightmapSize * TerrainBlock::LightmapSize * sizeof(ColorF));
    mShadowMask.setSize(TerrainBlock::BlockSize * TerrainBlock::BlockSize);
}

bool SceneLighting::TerrainProxy::preLight(LightInfo* light)
{
    if (!bool(mObj))
        return(false);

    if (light->mType != LightInfo::Vector)
        return(false);

    mShadowMask.clear();
    return(true);
}

inline ColorF getValue(S32 row, S32 column, ColorF* lmap)
{
    while (row < 0)
        row += TerrainBlock::LightmapSize;
    row = row % TerrainBlock::LightmapSize;

    while (column < 0)
        column += TerrainBlock::LightmapSize;
    column = column % TerrainBlock::LightmapSize;

    U32 offset = row * TerrainBlock::LightmapSize + column;

    return lmap[offset];
}

void SceneLighting::TerrainProxy::light(LightInfo* light)
{
    TerrainBlock* terrain = getObject();
    if (!terrain)
        return;

    AssertFatal(light->mType == LightInfo::Vector, "wrong light type");

    S32 time = Platform::getRealMilliseconds();

    // reset
    mShadowVolume = new ShadowVolumeBSP;

    // build interior shadow volume
    for (ObjectProxy** itr = gLighting->mLitObjects.begin(); itr != gLighting->mLitObjects.end(); itr++)
    {
        if (!gLighting->isInterior((*itr)->mObj))
            continue;

        if (markInteriorShadow(static_cast<InteriorProxy*>(*itr)))
            gLighting->addInterior(mShadowVolume, *static_cast<InteriorProxy*>(*itr), light, SceneLighting::SHADOW_DETAIL);
    }

    lightVector(light);

    // set the lightmap...
    if (terrain->lightMap) delete terrain->lightMap;
    terrain->lightMap = new GBitmap(TerrainBlock::LightmapSize, TerrainBlock::LightmapSize, 0, GFXFormatR8G8B8);

    // Blur...
    F32 kernel[3][3] = { 1, 2, 1,
                         2, 4 - 1, 2,
                         1, 2, 1 };

    F32 modifier = 1;
    F32 divisor = 0;


    for (U32 i = 0; i < 3; i++)
    {
        for (U32 j = 0; j < 3; j++)
        {
            if (i == 1 && j == 1)
            {
                kernel[i][j] = 1 + kernel[i][j] * modifier;
            }
            else
            {
                kernel[i][j] = kernel[i][j] * modifier;
            }

            divisor += kernel[i][j];
        }
    }

    F32 edgeDivisor = divisor - (kernel[0][0] + kernel[0][1] + kernel[0][2]);
    F32 doubleEdgeDivisor = kernel[1][1] + kernel[1][2] + kernel[2][1] + kernel[2][2];

    for (U32 i = 0; i < TerrainBlock::LightmapSize; i++)
    {
        for (U32 j = 0; j < TerrainBlock::LightmapSize; j++)
        {

            ColorF val;
            val = getValue(i - 1, j - 1, mLightmap) * kernel[0][0];
            val += getValue(i - 1, j, mLightmap) * kernel[0][1];
            val += getValue(i - 1, j + 1, mLightmap) * kernel[0][2];
            val += getValue(i, j - 1, mLightmap) * kernel[1][0];
            val += getValue(i, j, mLightmap) * kernel[1][1];
            val += getValue(i, j + 1, mLightmap) * kernel[1][2];
            val += getValue(i + 1, j - 1, mLightmap) * kernel[2][0];
            val += getValue(i + 1, j, mLightmap) * kernel[2][1];
            val += getValue(i + 1, j + 1, mLightmap) * kernel[2][2];


            U32 edge = 0;

            if (j == 0 || j == TerrainBlock::LightmapSize - 1)
            {
                edge++;
            }
            if (i == 0 || i == TerrainBlock::LightmapSize - 1)
            {
                edge++;
            }

            if (!edge)
            {
                val = val / divisor;
            }
            else
            {
                val = mLightmap[(i)*TerrainBlock::LightmapSize + (j)];
            }

            // clamp values
            mLightmap[(i * TerrainBlock::LightmapSize) + j] = val;
        }
    }

    // And stuff it into the texture...
    U8* lPtr = terrain->lightMap->getAddress(0, 0);
    for (U32 i = 0; i < (TerrainBlock::LightmapSize * TerrainBlock::LightmapSize); i++)
    {
        lPtr[i * 3 + 0] = mLightmap[i].red * 256;
        lPtr[i * 3 + 1] = mLightmap[i].green * 256;
        lPtr[i * 3 + 2] = mLightmap[i].blue * 256;
    }

    terrain->buildMaterialMap();

    delete mShadowVolume;

    Con::printf("    = terrain lit in %3.3f seconds", (Platform::getRealMilliseconds() - time) / 1000.f);
}

//------------------------------------------------------------------------------
S32 SceneLighting::TerrainProxy::testSquare(const Point3F& min, const Point3F& max, S32 mask, F32 expand, const Vector<PlaneF>& clipPlanes)
{
    expand = 0;
    S32 retMask = 0;
    Point3F minPoint, maxPoint;
    for (S32 i = 0; i < clipPlanes.size(); i++)
    {
        if (mask & (1 << i))
        {
            if (clipPlanes[i].x > 0)
            {
                maxPoint.x = max.x;
                minPoint.x = min.x;
            }
            else
            {
                maxPoint.x = min.x;
                minPoint.x = max.x;
            }
            if (clipPlanes[i].y > 0)
            {
                maxPoint.y = max.y;
                minPoint.y = min.y;
            }
            else
            {
                maxPoint.y = min.y;
                minPoint.y = max.y;
            }
            if (clipPlanes[i].z > 0)
            {
                maxPoint.z = max.z;
                minPoint.z = min.z;
            }
            else
            {
                maxPoint.z = min.z;
                minPoint.z = max.z;
            }
            F32 maxDot = mDot(maxPoint, clipPlanes[i]);
            F32 minDot = mDot(minPoint, clipPlanes[i]);
            F32 planeD = clipPlanes[i].d;
            if (maxDot <= -(planeD + expand))
                return(U16(-1));
            if (minDot <= -planeD)
                retMask |= (1 << i);
        }
    }
    return(retMask);
}

bool SceneLighting::TerrainProxy::getShadowedSquares(const Vector<PlaneF>& clipPlanes, Vector<U16>& shadowList)
{
    TerrainBlock* terrain = getObject();
    if (!terrain)
        return(false);

    SquareStackNode stack[TerrainBlock::BlockShift * 4];

    stack[0].mLevel = TerrainBlock::BlockShift;
    stack[0].mClipFlags = 0xff;
    stack[0].mPos.set(0, 0);

    U32 stackSize = 1;

    Point3F blockPos;
    terrain->getTransform().getColumn(3, &blockPos);
    S32 squareSize = terrain->getSquareSize();

    bool marked = false;

    // push through all the levels of the quadtree
    while (stackSize)
    {
        SquareStackNode* node = &stack[stackSize - 1];

        S32 clipFlags = node->mClipFlags;
        Point2I pos = node->mPos;
        GridSquare* sq = terrain->findSquare(node->mLevel, pos);

        Point3F minPoint, maxPoint;
        minPoint.set(squareSize * pos.x + blockPos.x,
            squareSize * pos.y + blockPos.y,
            fixedToFloat(sq->minHeight));
        maxPoint.set(minPoint.x + (squareSize << node->mLevel),
            minPoint.y + (squareSize << node->mLevel),
            fixedToFloat(sq->maxHeight));

        // test the square against the current level
        if (clipFlags)
        {
            clipFlags = testSquare(minPoint, maxPoint, clipFlags, squareSize, clipPlanes);
            if (clipFlags == U16(-1))
            {
                stackSize--;
                continue;
            }
        }

        // shadowed?
        if (node->mLevel == 0)
        {
            marked = true;
            shadowList.push_back(pos.x + (pos.y << TerrainBlock::BlockShift));
            stackSize--;
            continue;
        }

        // setup the next level of squares
        U8 nextLevel = node->mLevel - 1;
        S32 squareHalfSize = 1 << nextLevel;

        for (U32 i = 0; i < 4; i++)
        {
            node[i].mLevel = nextLevel;
            node[i].mClipFlags = clipFlags;
        }

        node[3].mPos = pos;
        node[2].mPos.set(pos.x + squareHalfSize, pos.y);
        node[1].mPos.set(pos.x, pos.y + squareHalfSize);
        node[0].mPos.set(pos.x + squareHalfSize, pos.y + squareHalfSize);

        stackSize += 3;
    }

    return(marked);
}

bool SceneLighting::TerrainProxy::markInteriorShadow(InteriorProxy* proxy)
{
    U32 i;
    // setup the clip planes: add the test and the lit planes
    Vector<PlaneF> clipPlanes;
    clipPlanes = proxy->mTerrainTestPlanes;
    for (i = 0; i < proxy->mLitBoxSurfaces.size(); i++)
        clipPlanes.push_back(proxy->mLitBoxSurfaces[i]->mPlane);

    Vector<U16> shadowList;
    if (!getShadowedSquares(clipPlanes, shadowList))
        return(false);

    // set the correct bit
    for (i = 0; i < shadowList.size(); i++)
        mShadowMask.set(shadowList[i]);

    return(true);
}

//-------------------------------------------------------------------------------
// BUGS: does not work with x or y directions of 0
//    : light dir of 0.1, 0.3, -0.8 causes strange behavior
void SceneLighting::TerrainProxy::lightVector(LightInfo* light)
{
    TerrainBlock* terrain = getObject();
    if (!terrain)
        return;

    Point3F lightDir = light->mDirection;
    lightDir.normalize();

    ColorF& ambient = light->mAmbient;
    ColorF& lightColor = light->mColor;

    if (lightDir.x == 0 && lightDir.y == 0)
        return;

    S32 generateLevel = Con::getIntVariable("$pref::sceneLighting::terrainGenerateLevel", 4);
    generateLevel = mClamp(generateLevel, 0, 4);

    bool allowLexelSplits = Con::getBoolVariable("$pref::sceneLighting::terrainAllowLexelSplits", false);

    U32 generateDim = TerrainBlock::LightmapSize << generateLevel;
    U32 generateShift = TerrainBlock::LightmapShift + generateLevel;
    U32 generateMask = generateDim - 1;

    F32 zStep;
    F32 frac;

    Point2I blockColStep;
    Point2I blockRowStep;
    Point2I blockFirstPos;
    Point2I lmFirstPos;

    F32 terrainDim = F32(terrain->getSquareSize()) * F32(TerrainBlock::BlockSize);
    F32 stepSize = F32(terrain->getSquareSize()) / F32(generateDim / TerrainBlock::BlockSize);

    if (mFabs(lightDir.x) >= mFabs(lightDir.y))
    {
        if (lightDir.x > 0)
        {
            zStep = lightDir.z / lightDir.x;
            frac = lightDir.y / lightDir.x;

            blockColStep.set(1, 0);
            blockRowStep.set(0, 1);
            blockFirstPos.set(0, 0);
            lmFirstPos.set(0, 0);
        }
        else
        {
            zStep = -lightDir.z / lightDir.x;
            frac = -lightDir.y / lightDir.x;

            blockColStep.set(-1, 0);
            blockRowStep.set(0, 1);
            blockFirstPos.set(255, 0);
            lmFirstPos.set(TerrainBlock::LightmapSize - 1, 0);
        }
    }
    else
    {
        if (lightDir.y > 0)
        {
            zStep = lightDir.z / lightDir.y;
            frac = lightDir.x / lightDir.y;

            blockColStep.set(0, 1);
            blockRowStep.set(1, 0);
            blockFirstPos.set(0, 0);
            lmFirstPos.set(0, 0);
        }
        else
        {
            zStep = -lightDir.z / lightDir.y;
            frac = -lightDir.x / lightDir.y;

            blockColStep.set(0, -1);
            blockRowStep.set(1, 0);
            blockFirstPos.set(0, 255);
            lmFirstPos.set(0, TerrainBlock::LightmapSize - 1);
        }
    }
    zStep *= stepSize;

    F32* heightArray = new F32[generateDim];

    S32 fracStep = -1;
    if (frac < 0)
    {
        fracStep = 1;
        frac = -frac;
    }

    F32* nextHeightArray = new F32[generateDim];
    F32 oneMinusFrac = 1 - frac;

    U32 blockShift = generateShift - TerrainBlock::BlockShift;
    U32 lightmapShift = generateShift - TerrainBlock::LightmapShift;

    U32 blockStep = 1 << blockShift;
    U32 blockMask = blockStep - 1;
    U32 lightmapStep = 1 << lightmapShift;
    U32 lightmapNormalOffset = lightmapStep >> 1;
    U32 lightmapMask = lightmapStep - 1;

    Point2I bp = blockFirstPos;
    F32 terrainHeights[2][TerrainBlock::BlockSize];
    U32 i;

    F32* pTerrainHeights = static_cast<F32*>(terrainHeights[0]);
    F32* pNextTerrainHeights = static_cast<F32*>(terrainHeights[1]);

    // get first set of heights
    for (i = 0; i < TerrainBlock::BlockSize; i++) {
        pTerrainHeights[i] = fixedToFloat(terrain->getHeight(bp.x, bp.y));
        bp += blockRowStep;
    }

    // get second set of heights
    bp = blockFirstPos + blockColStep;
    for (i = 0; i < TerrainBlock::BlockSize; i++) {
        pNextTerrainHeights[i] = fixedToFloat(terrain->getHeight(bp.x, bp.y));
        bp += blockRowStep;
    }

    F32 heightStep = 1.f / blockStep;

    F32 terrainZRowStep[TerrainBlock::BlockSize];
    F32 nextTerrainZRowStep[TerrainBlock::BlockSize];
    F32 terrainZColStep[TerrainBlock::BlockSize];

    // fill in the row steps
    for (i = 0; i < TerrainBlock::BlockSize; i++)
    {
        terrainZRowStep[i] = (pTerrainHeights[(i + 1) & TerrainBlock::BlockMask] - pTerrainHeights[i]) * heightStep;
        nextTerrainZRowStep[i] = (pNextTerrainHeights[(i + 1) & TerrainBlock::BlockMask] - pNextTerrainHeights[i]) * heightStep;
        terrainZColStep[i] = (pNextTerrainHeights[i] - pTerrainHeights[i]) * heightStep;
    }

    // get first row of process heights
    for (i = 0; i < generateDim; i++)
    {
        U32 bi = i >> blockShift;
        heightArray[i] = pTerrainHeights[bi] + (i & blockMask) * terrainZRowStep[bi];
    }

    bp = blockFirstPos;
    if (generateDim == TerrainBlock::BlockSize)
        bp += blockColStep;

    // generate the initial run
    U32 x, y;
    for (x = 1; x < generateDim; x++)
    {
        U32 xmask = x & blockMask;

        // generate new height step rows?
        if (!xmask)
        {
            F32* tmp = pTerrainHeights;
            pTerrainHeights = pNextTerrainHeights;
            pNextTerrainHeights = tmp;

            bp += blockColStep;

            Point2I bwalk = bp;
            for (i = 0; i < TerrainBlock::BlockSize; i++, bwalk += blockRowStep)
                pNextTerrainHeights[i] = fixedToFloat(terrain->getHeight(bwalk.x, bwalk.y));

            // fill in the row steps
            for (i = 0; i < TerrainBlock::BlockSize; i++)
            {
                terrainZRowStep[i] = (pTerrainHeights[(i + 1) & TerrainBlock::BlockMask] - pTerrainHeights[i]) * heightStep;
                nextTerrainZRowStep[i] = (pNextTerrainHeights[(i + 1) & TerrainBlock::BlockMask] - pNextTerrainHeights[i]) * heightStep;
                terrainZColStep[i] = (pNextTerrainHeights[i] - pTerrainHeights[i]) * heightStep;
            }
        }

        Point2I bwalk = bp - blockRowStep;
        for (y = 0; y < generateDim; y++)
        {
            U32 ymask = y & blockMask;
            if (!ymask)
                bwalk += blockRowStep;

            U32 bi = y >> blockShift;
            U32 binext = (bi + 1) & TerrainBlock::BlockMask;

            F32 height;

            // 135?
            if ((bwalk.x ^ bwalk.y) & 1)
            {
                U32 xsub = blockStep - xmask;
                if (xsub > ymask) // bottom
                    height = pTerrainHeights[bi] + xmask * terrainZColStep[bi] +
                    ymask * terrainZRowStep[bi];
                else // top
                    height = pNextTerrainHeights[bi] - xmask * terrainZColStep[binext] +
                    ymask * nextTerrainZRowStep[bi];
            }
            else
            {
                if (xmask > ymask) // bottom
                    height = pTerrainHeights[bi] + xmask * terrainZColStep[bi] +
                    ymask * nextTerrainZRowStep[bi];
                else // top
                    height = pTerrainHeights[bi] + xmask * terrainZColStep[binext] +
                    ymask * terrainZRowStep[bi];
            }

            F32 intHeight = heightArray[y] * oneMinusFrac + heightArray[(y + fracStep) & generateMask] * frac + zStep;
            nextHeightArray[y] = getMax(height, intHeight);
        }

        // swap the height rows
        F32* tmp = heightArray;
        heightArray = nextHeightArray;
        nextHeightArray = tmp;
    }

    F32 squareSize = terrain->getSquareSize();
    F32 lexelDim = squareSize * F32(TerrainBlock::BlockSize) / F32(TerrainBlock::LightmapSize);

    // calculate normal runs
    Point3F normals[2][TerrainBlock::BlockSize];

    Point3F* pNormals = static_cast<Point3F*>(normals[0]);
    Point3F* pNextNormals = static_cast<Point3F*>(normals[1]);

    // calculate the normal lookup table
    F32* normTable = new F32[blockStep * blockStep * 4];

    Point2F corners[4] = {
       Point2F(0.f, 0.f),
       Point2F(1.f, 0.f),
       Point2F(1.f, 1.f),
       Point2F(0.f, 1.f)
    };

    U32 idx = 0;
    F32 step = 1.f / blockStep;
    Point2F pos(0.f, 0.f);

    // fill it
    for (x = 0; x < blockStep; x++, pos.x += step, pos.y = 0.f) {
        for (y = 0; y < blockStep; y++, pos.y += step) {
            for (i = 0; i < 4; i++, idx++)
                normTable[idx] = 1.f - getMin(Point2F(pos - corners[i]).len(), 1.f);
        }
    }

    // fill first column
    bp = blockFirstPos;
    for (x = 0; x < TerrainBlock::BlockSize; x++)
    {
        pNextTerrainHeights[x] = fixedToFloat(terrain->getHeight(bp.x, bp.y));
        Point2F pos(bp.x * squareSize, bp.y * squareSize);
        terrain->getNormal(pos, &pNextNormals[x]);
        bp += blockRowStep;
    }

    // get swapped on first pass
    pTerrainHeights = static_cast<F32*>(terrainHeights[1]);
    pNextTerrainHeights = static_cast<F32*>(terrainHeights[0]);

    // get the world offset of the terrain
    const MatrixF& transform = terrain->getTransform();
    Point3F worldOffset;
    transform.getColumn(3, &worldOffset);

    F32 ratio = F32(1 << lightmapShift);
    F32 ratioSquared = ratio * ratio;
    F32 inverseRatioSquared = 1.f / ratioSquared;

    F32 lightScale[TerrainBlock::LightmapSize];

    // walk it...
    bp = blockFirstPos - blockColStep;
    for (x = 0; x < generateDim; x++)
    {
        U32 xmask = x & blockMask;
        U32 lxmask = x & lightmapMask;

        // generate new runs?
        if (!xmask)
        {
            bp += blockColStep;

            // do the normals
            Point3F* temp = pNormals;
            pNormals = pNextNormals;
            pNextNormals = temp;

            // fill the row
            Point2I bwalk = bp + blockColStep;
            for (i = 0; i < TerrainBlock::BlockSize; i++)
            {
                Point2F pos(bwalk.x * squareSize, bwalk.y * squareSize);
                terrain->getNormal(pos, &pNextNormals[i]);
                bwalk += blockRowStep;
            }

            // do the heights
            F32* tmp = pTerrainHeights;
            pTerrainHeights = pNextTerrainHeights;
            pNextTerrainHeights = tmp;

            bwalk = bp + blockColStep;
            for (i = 0; i < TerrainBlock::BlockSize; i++, bwalk += blockRowStep)
                pNextTerrainHeights[i] = fixedToFloat(terrain->getHeight(bwalk.x, bwalk.y));

            // fill in the row steps
            for (i = 0; i < TerrainBlock::BlockSize; i++)
            {
                terrainZRowStep[i] = (pTerrainHeights[(i + 1) & TerrainBlock::BlockMask] - pTerrainHeights[i]) * heightStep;
                nextTerrainZRowStep[i] = (pNextTerrainHeights[(i + 1) & TerrainBlock::BlockMask] - pNextTerrainHeights[i]) * heightStep;
                terrainZColStep[i] = (pNextTerrainHeights[i] - pTerrainHeights[i]) * heightStep;
            }
        }

        // reset the light scale table
        if (!lxmask)
            for (i = 0; i < TerrainBlock::LightmapSize; i++)
                lightScale[i] = 1.f;

        Point2I bwalk = bp - blockRowStep;
        for (y = 0; y < generateDim; y++)
        {
            U32 lymask = y & lightmapMask;
            U32 ymask = y & blockMask;
            if (!ymask)
                bwalk += blockRowStep;

            U32 bi = y >> blockShift;
            U32 binext = (bi + 1) & TerrainBlock::BlockMask;

            F32 height;
            F32 xstep, ystep;

            // 135?
            if ((bwalk.x ^ bwalk.y) & 1)
            {
                U32 xsub = blockStep - xmask;
                if (xsub > ymask) // bottom
                {
                    xstep = terrainZColStep[bi];
                    ystep = terrainZRowStep[bi];
                    height = pTerrainHeights[bi] + xmask * xstep + ymask * ystep;
                }
                else // top
                {
                    xstep = -terrainZColStep[binext];
                    ystep = nextTerrainZRowStep[bi];
                    height = pNextTerrainHeights[bi] + xsub * xstep + ymask * ystep;
                }
            }
            else
            {
                if (xmask > ymask) // bottom
                {
                    xstep = terrainZColStep[bi];
                    ystep = nextTerrainZRowStep[bi];
                    height = pTerrainHeights[bi] + xmask * xstep + ymask * ystep;
                }
                else // top
                {
                    xstep = terrainZColStep[binext];
                    ystep = terrainZRowStep[bi];
                    height = pTerrainHeights[bi] + xmask * xstep + ymask * ystep;
                }
            }

            F32 intHeight = heightArray[y] * oneMinusFrac + heightArray[(y + fracStep) & generateMask] * frac + zStep;

            U32 lsi = y >> lightmapShift;
            /* ColorF & col = mLightmap[(x >> lightmapShift) + (lsi << TerrainBlock::LightmapShift)]; */
            Point2I lmPos = lmFirstPos + blockColStep * (x >> lightmapShift) + blockRowStep * lsi;
            ColorF& col = mLightmap[lmPos.x + (lmPos.y << TerrainBlock::LightmapShift)];

            // lexel shaded by an interior?
            Point2I terrPos = lmPos;
            terrPos.x >>= TerrainBlock::LightmapShift - TerrainBlock::BlockShift;
            terrPos.y >>= TerrainBlock::LightmapShift - TerrainBlock::BlockShift;

            if (!lxmask && !lymask && mShadowMask.test(terrPos.x + (terrPos.y << TerrainBlock::BlockShift)))
            {
                U32 idx = (xmask + lightmapNormalOffset + ((ymask + lightmapNormalOffset) << blockShift)) << 2;

                // get the normal for this lexel
                Point3F normal = pNormals[bi] * normTable[idx++];
                normal += pNormals[binext] * normTable[idx++];
                normal += pNextNormals[binext] * normTable[idx++];
                normal += pNextNormals[bi] * normTable[idx];
                normal.normalize();

                nextHeightArray[y] = height;
                F32 colorScale = -mDot(normal, lightDir);

                if (colorScale > 0.f)
                {
                    // split lexels which cross the square split?
                    if (allowLexelSplits)
                    {
                        // jff:todo
                    }
                    else
                    {
                        Point2F wPos((lmPos.x) * lexelDim + worldOffset.x,
                            (lmPos.y) * lexelDim + worldOffset.y);

                        F32 xh = xstep * ratio;
                        F32 yh = ystep * ratio;

                        ShadowVolumeBSP::SVPoly* poly = mShadowVolume->createPoly();
                        poly->mWindingCount = 4;
                        poly->mWinding[0].set(wPos.x, wPos.y, height);
                        poly->mWinding[1].set(wPos.x + lexelDim, wPos.y, height + xh);
                        poly->mWinding[2].set(wPos.x + lexelDim, wPos.y + lexelDim, height + xh + yh);
                        poly->mWinding[3].set(wPos.x, wPos.y + lexelDim, height + yh);
                        poly->mPlane.set(poly->mWinding[0], poly->mWinding[1], poly->mWinding[2]);

                        F32 lexelSize = mShadowVolume->getPolySurfaceArea(poly);
                        F32 intensity = mShadowVolume->getClippedSurfaceArea(poly) / lexelSize;
                        lightScale[lsi] = mClampF(intensity, 0.f, 1.f);
                    }
                }
                else
                    lightScale[lsi] = 0.f;
            }

            // non shadowed?
            if (height >= intHeight)
            {
                U32 idx = (xmask + (ymask << blockShift)) << 2;

                Point3F normal = pNormals[bi] * normTable[idx++];
                normal += pNormals[binext] * normTable[idx++];
                normal += pNextNormals[binext] * normTable[idx++];
                normal += pNextNormals[bi] * normTable[idx];
                normal.normalize();

                nextHeightArray[y] = height;
                F32 colorScale = -mDot(normal, lightDir);

                if (colorScale > 0.f)
                    col = ambient + lightColor * colorScale * lightScale[lsi];
                else
                    col = ambient;
            }
            else
            {
                nextHeightArray[y] = intHeight;
                col = ambient;
            }
        }

        F32* tmp = heightArray;
        heightArray = nextHeightArray;
        nextHeightArray = tmp;
    }

    // set the proper color
    for (i = 0; i < TerrainBlock::LightmapSize * TerrainBlock::LightmapSize; i++)
    {
        //mLightmap[i] *= inverseRatioSquared;
        mLightmap[i] /= TERRAIN_OVERRANGE;
        mLightmap[i].clamp();
    }

    delete[] normTable;
    delete[] heightArray;
    delete[] nextHeightArray;
}

//--------------------------------------------------------------------------
U32 SceneLighting::TerrainProxy::getResourceCRC()
{
    TerrainBlock* terrain = getObject();
    if (!terrain)
        return(0);
    return(terrain->getCRC());
}

//--------------------------------------------------------------------------
bool SceneLighting::TerrainProxy::setPersistInfo(SceneLighting::PersistInfo::PersistChunk* info)
{
    if (!Parent::setPersistInfo(info))
        return(false);

    SceneLighting::PersistInfo::TerrainChunk* chunk = dynamic_cast<SceneLighting::PersistInfo::TerrainChunk*>(info);
    AssertFatal(chunk, "SceneLighting::TerrainProxy::setPersistInfo: invalid info chunk!");

    TerrainBlock* terrain = getObject();
    if (!terrain || !terrain->lightMap)
        return(false);

    if (terrain->lightMap) delete terrain->lightMap;

    terrain->lightMap = new GBitmap(*chunk->mLightmap);

    terrain->buildMaterialMap();
    return(true);
}

bool SceneLighting::TerrainProxy::getPersistInfo(SceneLighting::PersistInfo::PersistChunk* info)
{
    if (!Parent::getPersistInfo(info))
        return(false);

    SceneLighting::PersistInfo::TerrainChunk* chunk = dynamic_cast<SceneLighting::PersistInfo::TerrainChunk*>(info);
    AssertFatal(chunk, "SceneLighting::TerrainProxy::getPersistInfo: invalid info chunk!");

    TerrainBlock* terrain = getObject();
    if (!terrain || !terrain->lightMap)
        return(false);

    if (chunk->mLightmap) delete chunk->mLightmap;

    chunk->mLightmap = new GBitmap(*terrain->lightMap);

    return(true);
}
