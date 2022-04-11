//-----------------------------------------------
// Synapse Gaming - Lighting System
// Copyright � Synapse Gaming 2003
// Written by John Kabus
//-----------------------------------------------
#include "math/mBox.h"
#include "math/mathUtils.h"
#include "sceneGraph/sceneGraph.h"
#ifdef TORQUE_TERRAIN
#include "terrain/terrData.h"
#include "atlas/runtime/atlasInstance2.h"
#endif
#include "platform/profiler.h"
#include "interior/interior.h"
#include "interior/interiorInstance.h"
#include "lightingSystem/sgLightMap.h"
#include "lightingSystem/sgLightingModel.h"

/// used to calculate the start and end points
/// for ray casting directional light.
#define SG_STATIC_LIGHT_VECTOR_DIST	100

VectorPtr<SceneObject*> sgShadowObjects::sgObjects;

void sgCalculateLightMapTransforms(InteriorInstance* intinst, const Interior::Surface& surf, MatrixF& objspace, MatrixF& tanspace)
{
    tanspace.setColumn(0, surf.T);
    tanspace.setColumn(1, surf.B);
    tanspace.setColumn(2, surf.N);
    tanspace.inverse();

    objspace = intinst->getRenderWorldTransform();
}

ColorF sgCalculateLightMapVector(Point3F& lightingnormal, MatrixF& objspace, MatrixF& tangentspace)
{
    Point3F p = lightingnormal;
    objspace.mulV(p);
    tangentspace.mulV(p);
    ColorF ret(p.x, p.y, p.z, 0);
    return ret;
}


/// used to generate light map indexes that wrap around
/// instead of exceeding the index bounds.
inline S32 sgGetIndex(S32 width, S32 height, S32 x, S32 y)
{
    if (x > (width - 1))
        x -= width;
    else if (x < 0)
        x += width;

    if (y > (height - 1))
        y -= height;
    else if (y < 0)
        y += height;

    return (y * width) + x;
}

void sgColorMap::sgFillInLighting()
{
    U32 x, y;
    U32 lastgoodx, lastgoody;
    U32 lastgoodyoffset, yoffset;

    if (LightManager::sgAllowFullLightMaps())
        return;

    U32 lmscalemask = LightManager::sgGetLightMapScale() - 1;

    for (y = 0; y < sgHeight; y++)
    {
        // do we have a good y?
        if ((y & lmscalemask) == 0)
        {
            lastgoody = y;
            lastgoodyoffset = lastgoody * sgWidth;
        }

        yoffset = y * sgWidth;

        for (x = 0; x < sgWidth; x++)
        {
            // do we have a good x?
            if ((x & lmscalemask) == 0)
            {
                lastgoodx = x;

                // only bailout if we're on a good y, otherwise
                // all of the x entries are empty...
                if ((y & lmscalemask) == 0)
                    continue;
            }

            ColorF& last = sgData[(lastgoodyoffset + lastgoodx)];
            sgData[(yoffset + x)] = last;
        }
    }
}

void sgColorMap::sgBlur()
{
    static F32 blur[3][3] = { {0.1, 0.125, 0.1}, {0.125, 0.1, 0.125}, {0.1, 0.125, 0.1} };
    //static F32 blur[3][3] = {{0.075, 0.125, 0.075}, {0.125, 0.2, 0.125}, {0.075, 0.125, 0.075}};

    ColorF* buffer = new ColorF[sgWidth * sgHeight];

    // lets get the values that we don't blur...
    dMemcpy(buffer, sgData, (sizeof(ColorF) * sgWidth * sgHeight));

    for (U32 y = 1; y < (sgHeight - 1); y++)
    {
        for (U32 x = 1; x < (sgWidth - 1); x++)
        {
            ColorF& col = buffer[((y * sgWidth) + x)];

            col = ColorF(0.0f, 0.0f, 0.0f);

            for (S32 by = -1; by < 2; by++)
            {
                for (S32 bx = -1; bx < 2; bx++)
                {
                    col += sgData[(((y + by) * sgWidth) + (x + bx))] * blur[bx + 1][by + 1];
                }
            }
        }
    }

    delete[] sgData;
    sgData = buffer;
}
/*
void sgColorMap::sgMerge(GBitmap *lightmap, U32 xoffset, U32 yoffset, bool normalblend)
{
    U32 y, x, c, frag;
    U8 *bits;

    sgFillInLighting();
    sgBlur();

    for(y=0; y<sgHeight; y++)
    {
        bits = lightmap->getAddress(xoffset, (yoffset + y));
        c = 0;

        for(x=0; x<sgWidth; x++)
        {
            ColorF &texel = sgData[((y * sgWidth) + x)];

            if(normalblend)
            {
                Point3F src(texel.red, texel.green, texel.blue);
                //Point3F dst(0, 0, 0);
                Point3F dst(bits[c+2], bits[c+1], bits[c]);
                dst *= 0.0039215f;
                dst = (dst - Point3F(0.5f, 0.5f, 0.5f)) * 2.0f;
                dst += src;
                dst.normalizeSafe();
                dst = (dst * 0.5) + Point3F(0.5f, 0.5f, 0.5f);

                bits[c++] = dst.z * 255.0f;
                bits[c++] = dst.y * 255.0f;
                bits[c++] = dst.x * 255.0f;
            }
            else
            {
                frag = bits[c] + (texel.red * 255.0f);
                if(frag > 255) frag = 255;
                bits[c++] = frag;

                frag = bits[c] + (texel.green * 255.0f);
                if(frag > 255) frag = 255;
                bits[c++] = frag;

                frag = bits[c] + (texel.blue * 255.0f);
                if(frag > 255) frag = 255;
                bits[c++] = frag;
            }
        }
    }
}
*/
/// generic ray casting using a sceneObject or an interior.
inline bool sgCastLightRay(SceneObject* object, Interior* interior, const Point3F& destination, Point3F& source, RayInfo& info)
{
    const MatrixF& transform = object->getWorldTransform();
    const VectorF& scale = object->getScale();

    Point3F start, end;

    // convert to object space...
    transform.mulP(source, &start);
    start.convolveInverse(scale);
    transform.mulP(destination, &end);
    end.convolveInverse(scale);

    if (interior)
        return interior->castRay(start, end, &info);
    return object->castRay(start, end, &info);
}


void sgObjectCallback(SceneObject* object, void* vector)
{
    VectorPtr<SceneObject*>* intersectingobjects = static_cast<VectorPtr<SceneObject*> *>(vector);
    intersectingobjects->push_back(object);
}

void sgShadowObjects::sgGetObjects(SceneObject* obj)
{
    sgObjects.clear();
    obj->getContainer()->findObjects(ShadowCasterObjectType, &sgObjectCallback, &sgObjects);
}

void sgLightMap::sgGetIntersectingObjects(const Box3F& surfacebox, const LightInfo* light)
{
    Box3F box = surfacebox;
    box.max.setMax(light->mPos);
    box.min.setMin(light->mPos);

    sgIntersectingObjects.clear();

    for (U32 i = 0; i < sgShadowObjects::sgObjects.size(); i++)
    {
        if (box.isOverlapped(sgShadowObjects::sgObjects[i]->getWorldBox()))
            sgIntersectingObjects.push_back(sgShadowObjects::sgObjects[i]);
    }
}

void sgPlanarLightMap::sgSetupLighting()
{
    // stats...
    sgStatistics::sgInteriorSurfaceIncludedCount++;


    // get tranformed points...
    U32 winding[32];
    U32 windingcount = 0;
    Vector<Point3F> pointlisti;
    Vector<Point3F> pointvect1;
    const Interior::Surface& surf = sgInteriorCurrentDetail->getSurface(sgSurfaceIndex);
    sgInteriorCurrentDetail->collisionFanFromSurface(surf, winding, &windingcount);

    sgIntersectingObjects.clear();

    pointlisti.reserve(windingcount);
    pointvect1.reserve(windingcount);

    for (U32 i = 0; i < windingcount; i++)
    {
        U32 k = (i + 1) % windingcount;

        Point3F pointi = sgInteriorCurrentDetail->getPoint(winding[i]);
        Point3F pointk = sgInteriorCurrentDetail->getPoint(winding[k]);

        pointi.convolve(sgInteriorInstance->getScale());
        sgInteriorInstance->getTransform().mulP(pointi);

        pointk.convolve(sgInteriorInstance->getScale());
        sgInteriorInstance->getTransform().mulP(pointk);

        Point3F vec1 = pointk - pointi;

        pointlisti.push_back(pointi);
        pointvect1.push_back(vec1);

        if (i == 0)
            sgSurfaceBox = Box3F(pointi, pointi);
        else
        {
            sgSurfaceBox.max.setMax(pointi);
            sgSurfaceBox.min.setMin(pointi);
        }
    }

    // loop through the texels...
    // sort by inner and outer...
    const U32 lexelmax = sgHeight * sgWidth;
    const U32 buffersize = lexelmax * sizeof(sgLexel);
    sgInnerLexels.clear();
    sgInnerLexels.reserve(lexelmax);
    sgOuterLexels.clear();
    sgOuterLexels.reserve(lexelmax);

    // this is faster than Vector[]...
    Point3F* pointlistiptr = pointlisti.address();
    Point3F* pointvect1ptr = pointvect1.address();

    bool outer;
    Point3F vec2;
    Point3F cross;
    Point3F pos = sgWorldPosition;
    Point3F run = sgLightMapSVector * sgWidth;

    bool halfsize = !LightManager::sgAllowFullLightMaps();
    U32 lmscalemask = LightManager::sgGetLightMapScale() - 1;

    for (U32 y = 0; y < sgHeight; y++)
    {
        if (halfsize && (y & lmscalemask))
        {
            pos += sgLightMapTVector;
            continue;
        }

        for (U32 x = 0; x < sgWidth; x++)
        {
            if (halfsize && (x & lmscalemask))
            {
                pos += sgLightMapSVector;
                continue;
            }

            outer = false;

            for (U32 i = 0; i < windingcount; i++)
            {
                vec2 = pos - pointlistiptr[i];
                mCross(vec2, pointvect1ptr[i], &cross);

                if (mDot(sgPlaneNormal, cross) < 0.f)
                {
                    // nope...
                    outer = true;
                    break;
                }
            }

            // find respective vector...
            if (outer)
            {
                sgOuterLexels.increment();
                sgLexel& temp = sgOuterLexels.last();
                temp.lmPos.x = x;
                temp.lmPos.y = y;
                temp.worldPos = pos;
            }
            else
            {
                sgInnerLexels.increment();
                sgLexel& temp = sgInnerLexels.last();
                temp.lmPos.x = x;
                temp.lmPos.y = y;
                temp.worldPos = pos;
            }

            pos += sgLightMapSVector;
        }

        pos -= run;
        pos += sgLightMapTVector;
    }
}

void sgPlanarLightMap::sgCalculateLighting(LightInfo* light)
{
    U32 o;
    U32 i, ii;


    // stats...
    sgStatistics::sgInteriorSurfaceIlluminationCount++;


    // setup zone info...
    bool isinzone = false;
    if (light->sgDiffuseRestrictZone || light->sgAmbientRestrictZone)
    {
        for (U32 z = 0; z < sgInteriorInstance->getNumCurrZones(); z++)
        {
            S32 zone = sgInteriorInstance->getCurrZone(z);
            if (zone > 0)
            {
                if ((zone == light->sgZone[0]) || (zone == light->sgZone[1]))
                {
                    isinzone = true;
                    break;
                }
            }
        }

        if (!isinzone)
        {
            S32 zone = sgInteriorInstance->getSurfaceZone(sgSurfaceIndex, sgInteriorCurrentDetail);

            if ((light->sgZone[0] == zone) || (light->sgZone[1] == zone))
                isinzone = true;
        }
    }

    // allow what?
    bool allowdiffuse = (!light->sgDiffuseRestrictZone) || isinzone;
    bool allowambient = (!light->sgAmbientRestrictZone) || isinzone;

    // should I bother?
    if ((!allowdiffuse) && (!allowambient))
    {
        //testing...
        S32 zone = sgInteriorInstance->getSurfaceZone(sgSurfaceIndex, sgInteriorCurrentDetail);
        Con::printf("Skipping surface in zone %d.", zone);
        return;
    }

    // calculate some static info...
    Point3F run = sgLightMapSVector * sgWidth;

    AssertFatal((sgSurfaceIndex != -1), "Member 'sgSurfaceIndex' must be populated.");
    AssertFatal((sgInteriorInstance != NULL), "Member 'sgInteriorInstance' must be populated.");
    AssertFatal((sgInteriorCurrentDetail != NULL), "Member 'sgInteriorCurrentDetail' must be populated.");

    U32 time = Platform::getRealMilliseconds();
    SceneObject* object;

    // first get lighting model...
    sgLightingModel& model = sgLightingModelManager::sgGetLightingModel(
        light->sgLightingModelName);
    model.sgSetState(light);

    // test for early out...
    if (!model.sgCanIlluminate(sgSurfaceBox))
    {
        model.sgResetState();

        // stats...
        sgStatistics::sgInteriorLexelTime += Platform::getRealMilliseconds() - time;
        return;
    }

    // this is slow, so do it after the early out...
    model.sgInitStateLM();

    // build a list of potential shadow casters...
    if (light->sgCastsShadows && LightManager::sgAllowShadows())
        sgGetIntersectingObjects(sgSurfaceBox, light);

    Vector<S32> selfshadowingsurfaces;
    ColorF diffuse = ColorF(0.0, 0.0, 0.0);
    ColorF ambient = ColorF(0.0, 0.0, 0.0);
    Point3F lightingnormal;

    MatrixF objspace;
    MatrixF tanspace;
    const Interior::Surface& surf = sgInteriorCurrentDetail->getSurface(sgSurfaceIndex);
    sgCalculateLightMapTransforms(sgInteriorInstance, surf, objspace, tanspace);


    // stats...
    sgStatistics::sgInteriorSurfaceIlluminatedCount++;
    sgStatistics::sgInteriorLexelCount += sgInnerLexels.size() + sgOuterLexels.size();


    for (i = 0; i < sgPlanarLightMap::sglpCount; i++)
    {
        // set which list...
        U32 templexelscount;
        sgLexel* templexels;
        if (i == sgPlanarLightMap::sglpInner)
        {
            templexelscount = sgInnerLexels.size();
            templexels = sgInnerLexels.address();
        }
        else
        {
            templexelscount = sgOuterLexels.size();
            templexels = sgOuterLexels.address();
        }

        for (ii = 0; ii < templexelscount; ii++)
        {
            // get the current lexel...
            const sgLexel& lexel = templexels[ii];

            // too often unset, must do these here...
            ambient = diffuse = ColorF(0.0f, 0.0f, 0.0f);
            lightingnormal = VectorF(0.0f, 0.0f, 0.0f);
            model.sgLightingLM(lexel.worldPos, sgPlaneNormal, diffuse, ambient, lightingnormal);


            const U32 x = lexel.lmPos.x;
            const U32 y = lexel.lmPos.y;
            const U32 lmindex = ((y * sgWidth) + x);

            if (allowdiffuse && ((diffuse.red > SG_MIN_LEXEL_INTENSITY) ||
                (diffuse.green > SG_MIN_LEXEL_INTENSITY) ||
                (diffuse.blue > SG_MIN_LEXEL_INTENSITY)))
            {
                // step four: check for shadows...

                bool shadowed = false;
                RayInfo info;

                if (light->sgCastsShadows && LightManager::sgAllowShadows())
                {
                    // set light pos for shadows...
                    Point3F lightpos = light->mPos;
                    if (light->mType == LightInfo::Vector)
                    {
                        lightpos = SG_STATIC_LIGHT_VECTOR_DIST * light->mDirection * -1;
                        lightpos = lexel.worldPos + lightpos;
                    }

                    // cast rays against the potential shadow casters...
                    for (o = 0; o < sgIntersectingObjects.size(); o++)
                    {
                        object = sgIntersectingObjects[o];
                        // ignore self...
                        if (object != sgInteriorInstance)
                        {
                            if (sgCastLightRay(object, NULL, lexel.worldPos, lightpos, info))
                            {
                                shadowed = true;
                                break;
                            }
                        }


                        // stats...
                        sgStatistics::sgInteriorOccluderCount++;
                    }

                    // cast against self...
                    if ((!shadowed) && (sgSelfShadowing) &&
                        (sgCastLightRay(sgInteriorInstance, sgInteriorCurrentDetail, lexel.worldPos, lightpos, info)))
                    {
                        // stats...
                        sgStatistics::sgInteriorOccluderCount++;


                        // prevent self or neighbor surface shadowing...
                        if (info.face != sgSurfaceIndex)
                        {
                            U32 adj = sgAreAdjacent(info.face, sgSurfaceIndex);
                            if (adj == sgaTrue)
                                shadowed = false;
                            else if (adj == sgaFalse)
                                shadowed = true;
                            else
                            {
                                // maybe... :)
                                if (i == sglpInner)
                                {
                                    shadowed = true;
                                    selfshadowingsurfaces.push_back(info.face);
                                }
                                else
                                {
                                    // was it shadowing the inner lexels?
                                    shadowed = false;
                                    for (U32 s = 0; s < selfshadowingsurfaces.size(); s++)
                                    {
                                        if (selfshadowingsurfaces[s] == info.face)
                                        {
                                            shadowed = true;
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                if (!shadowed)
                {
                    // step five: apply the lighting to the light map...
                    sgDirty = true;
                    sgTexels->sgData[lmindex] += diffuse;
                    sgVectors->sgData[lmindex] += sgCalculateLightMapVector(lightingnormal, objspace, tanspace);
                }
            }

            if (allowambient && ((ambient.red > 0.0f) || (ambient.green > 0.0f) || (ambient.blue > 0.0f)))
            {
                sgDirty = true;
                sgTexels->sgData[lmindex] += ambient;

                lightingnormal = sgPlaneNormal;
                sgVectors->sgData[lmindex] += sgCalculateLightMapVector(lightingnormal, objspace, tanspace);
            }
        }
    }

    model.sgResetState();


    // stats...
    sgStatistics::sgInteriorLexelTime += Platform::getRealMilliseconds() - time;
}

U32 sgPlanarLightMap::sgAreAdjacent(U32 surface1, U32 surface2)
{
    U32 i1, i2;
    Vector<U32> index1, index2;

    // skip?
    if (sgChooseSpeedOverQuality)
        return sgaFalse;

    //if we can't identify the surfaces then assume the worst...
    if ((surface1 == U32(-1)) || (surface2 == U32(-1)))
        return sgaFalse;

    // get the compare surfaces...
    const Interior::Surface& surf1 = sgInteriorCurrentDetail->getSurface(surface1);
    const Interior::Surface& surf2 = sgInteriorCurrentDetail->getSurface(surface2);

    // grab the surface's winding indexes...
    for (i1 = 0; i1 < surf1.windingCount; i1++)
        index1.push_back(sgInteriorCurrentDetail->getWinding(surf1.windingStart + i1));

    // grab the surface's winding indexes...
    for (i2 = 0; i2 < surf2.windingCount; i2++)
        index2.push_back(sgInteriorCurrentDetail->getWinding(surf2.windingStart + i2));

    // compare for common index...
    long indexcount = 0;
    for (i1 = 0; i1 < index1.size(); i1++)
    {
        for (i2 = 0; i2 < index2.size(); i2++)
        {
            if (index1[i1] == index2[i2])
            {
                indexcount++;
                if (indexcount > 1)
                    return sgaTrue;
            }
        }
    }

    if (indexcount < 1)
        return sgaFalse;

    return sgaMaybe;
}

void sgPlanarLightMap::sgMergeLighting(GBitmap* lightmap, GBitmap* normalmap, U32 xoffset, U32 yoffset)
{
    //sgTexels->sgMerge(lightmap, xoffset, yoffset, false);
    //sgVectors->sgMerge(normalmap, xoffset, yoffset, true);
//}
    sgTexels->sgFillInLighting();
    sgTexels->sgBlur();
    sgVectors->sgFillInLighting();
    sgVectors->sgBlur();

    U32 y, x, d, s, frag;
    F32 si, di;
    U8* bits;
    U8* nbits;
    ColorF texel;
    ColorF curtexel;
    Point3F nexel;
    Point3F curnexel;
    Point3F half = Point3F(0.5f, 0.5f, 0.5f);

    ColorF* texels = sgTexels->sgData;
    ColorF* vectors = sgVectors->sgData;

    for (y = 0; y < sgHeight; y++)
    {
        bits = lightmap->getAddress(xoffset, (yoffset + y));
        nbits = normalmap->getAddress(xoffset, (yoffset + y));
        d = 0;

        for (x = 0; x < sgWidth; x++)
        {
            s = ((y * sgWidth) + x);
            ColorF& texel1 = texels[s];
            //ColorF &texel2 = texels[(s + 1)];
            ColorF& p1c = vectors[s];
            //ColorF &p2c = vectors[(s + 1)];

            Point3F p1 = Point3F(p1c.red, p1c.green, p1c.blue);
            //Point3F p2 = Point3F(p2c.red, p2c.green, p2c.blue);
/*
            s += sgWidth;
            ColorF &texel3 = texels[s];
            ColorF &texel4 = texels[(s + 1)];
            ColorF &p3c = vectors[s];
            ColorF &p4c = vectors[(s + 1)];

            Point3F p3 = Point3F(p3c.red, p3c.green, p3c.blue);
            Point3F p4 = Point3F(p4c.red, p4c.green, p4c.blue);
*/
            texel = texel1;// + texel2 + texel3 + texel4;
            //texel *= 0.25;

            nexel = p1;// + p2 + p3 + p4;
            nexel.normalizeSafe();

            // get the current texel (BGR)...
            curtexel = ColorF(bits[d + 2], bits[d + 1], bits[d]);
            curtexel *= 0.0039215f;

            // get the current nexel (BGR)...
            curnexel = Point3F(nbits[d + 2], nbits[d + 1], nbits[d]);
            curnexel *= 0.0039215f;
            curnexel -= half;
            curnexel *= 2.0f;

            // adjust...
            si = texel.red + texel.green + texel.blue;
            di = curtexel.red + curtexel.green + curtexel.blue;
            di = getMax(di, 0.00001f);
            si = si / di;

            // and add...
            curtexel += texel;
            curnexel = (nexel * si) + curnexel;
            curnexel.normalizeSafe();

            // convert...
            curtexel *= 255.0f;
            curnexel = ((curnexel * 0.5f) + half) * 255.0f;

            // save (BGR)...
            bits[d] = mClamp(((U32)curtexel.blue), 0, 255);
            nbits[d++] = mClamp(((U32)curnexel.z), 0, 255);

            bits[d] = mClamp(((U32)curtexel.green), 0, 255);
            nbits[d++] = mClamp(((U32)curnexel.y), 0, 255);

            bits[d] = mClamp(((U32)curtexel.red), 0, 255);
            nbits[d++] = mClamp(((U32)curnexel.x), 0, 255);

            /*vector = ((vector * 0.5) + half);
            vector *= 255.0f;

            nbits[d] = mClamp(((U32)vector.x), 0, 255);
            nbits[d+1] = mClamp(((U32)vector.y), 0, 255);
            nbits[d+2] = mClamp(((U32)vector.z), 0, 255);

            frag = bits[d] + (texel.red * 255.0f);
            if(frag > 255) frag = 255;
            bits[d++] = frag;

            frag = bits[d] + (texel.green * 255.0f);
            if(frag > 255) frag = 255;
            bits[d++] = frag;

            frag = bits[d] + (texel.blue * 255.0f);
            if(frag > 255) frag = 255;
            bits[d++] = frag;*/
        }
    }
}

//----------------------------------------------

#ifdef TORQUE_TERRAIN
void sgTerrainLightMap::sgCalculateLighting(LightInfo* light)
{
    // setup zone info...
    bool isinzone = (light->sgZone[0] == 0) || (light->sgZone[1] == 0);

    // allow what?
    bool allowdiffuse = (!light->sgDiffuseRestrictZone) || isinzone;
    bool allowambient = (!light->sgAmbientRestrictZone) || isinzone;

    // should I bother?
    if ((!allowdiffuse) && (!allowambient))
        return;


    AssertFatal((sgTerrain), "Member 'sgTerrain' must be populated.");

    // setup constants...
    F32 terrainlength = (sgTerrain->getSquareSize() * TerrainBlock::BlockSize);
    const F32 halfterrainlength = terrainlength * 0.5;


    U32 time = Platform::getRealMilliseconds();

    Point2F s, t;
    s[0] = sgLightMapSVector[0];
    s[1] = sgLightMapSVector[1];
    t[0] = sgLightMapTVector[0];
    t[1] = sgLightMapTVector[1];
    Point2F run = t * sgWidth;

    Point2F start;
    start[0] = sgWorldPosition[0] + halfterrainlength;
    start[1] = sgWorldPosition[1] + halfterrainlength;

    sgLightingModel& model = sgLightingModelManager::sgGetLightingModel(
        light->sgLightingModelName);
    model.sgSetState(light);
    model.sgInitStateLM();

    Point2I lmindexmin, lmindexmax;
    if (light->mType == LightInfo::Vector)
    {
        lmindexmin.x = 0;
        lmindexmin.y = 0;
        lmindexmax.x = (sgWidth - 1);
        lmindexmax.y = (sgHeight - 1);
    }
    else
    {
        F32 maxrad = model.sgGetMaxRadius();
        Box3F worldbox = Box3F(light->mPos, light->mPos);
        worldbox.min -= Point3F(maxrad, maxrad, maxrad);
        worldbox.max += Point3F(maxrad, maxrad, maxrad);

        lmindexmin.x = (worldbox.min.x - sgWorldPosition.x) / s.x;
        lmindexmin.y = (worldbox.min.y - sgWorldPosition.y) / t.y;
        lmindexmax.x = (worldbox.max.x - sgWorldPosition.x) / s.x;
        lmindexmax.y = (worldbox.max.y - sgWorldPosition.y) / t.y;

        lmindexmin.x = getMax(lmindexmin.x, S32(0));
        lmindexmin.y = getMax(lmindexmin.y, S32(0));
        lmindexmax.x = getMin(lmindexmax.x, S32(sgWidth - 1));
        lmindexmax.y = getMin(lmindexmax.y, S32(sgHeight - 1));
    }

    S32 lmx, lmy, lmindex;
    Point3F lexelworldpos;
    Point3F normal;
    ColorF diffuse = ColorF(0.0, 0.0, 0.0);
    ColorF ambient = ColorF(0.0, 0.0, 0.0);
    Point3F lightingnormal = Point3F(0.0, 0.0, 0.0);

    Point2F point = ((t * lmindexmin.y) + start + (s * lmindexmin.x));

    for (lmy = lmindexmin.y; lmy < lmindexmax.y; lmy++)
    {
        for (lmx = lmindexmin.x; lmx < lmindexmax.x; lmx++)
        {
            lmindex = (lmx + (lmy * sgWidth));

            // get lexel 2D world pos...
            lexelworldpos[0] = point[0] - halfterrainlength;
            lexelworldpos[1] = point[1] - halfterrainlength;

            // use 2D terrain space pos to get the world space z and normal...
            sgTerrain->getNormalAndHeight(point, &normal, &lexelworldpos.z, false);

            // too often unset, must do these here...
            ambient = diffuse = ColorF(0.0f, 0.0f, 0.0f);
            lightingnormal = VectorF(0.0f, 0.0f, 0.0f);
            model.sgLightingLM(lexelworldpos, normal, diffuse, ambient, lightingnormal);

            if (allowdiffuse && ((diffuse.red > SG_MIN_LEXEL_INTENSITY) ||
                (diffuse.green > SG_MIN_LEXEL_INTENSITY) || (diffuse.blue > SG_MIN_LEXEL_INTENSITY)))
            {
                // step four: check for shadows...

                bool shadowed = false;
                RayInfo info;
                if (light->sgCastsShadows && LightManager::sgAllowShadows())
                {
                    // set light pos for shadows...
                    Point3F lightpos = light->mPos;
                    if (light->mType == LightInfo::Vector)
                    {
                        lightpos = SG_STATIC_LIGHT_VECTOR_DIST * light->mDirection * -1;
                        lightpos = lexelworldpos + lightpos;
                    }

                    // make texels terrain space coord into a world space coord...
                    RayInfo info;
                    if (sgTerrain->getContainer()->castRay(lightpos, (lexelworldpos + (lightingnormal * 0.5)),
                        ShadowCasterObjectType, &info))
                    {
                        shadowed = true;
                    }
                }

                if (!shadowed)
                {
                    // step five: apply the lighting to the light map...
                    sgTexels->sgData[lmindex] += diffuse;
                }
            }

            if (allowambient && ((ambient.red > 0.0f) || (ambient.green > 0.0f) || (ambient.blue > 0.0f)))
            {
                //sgTexels->sgData[lmindex] += ambient;
            }

            /*if((lmx & 0x1) == (lmy & 0x1))
                sgTexels[lmindex] += ColorF(1.0, 0.0, 0.0);
            else
                sgTexels[lmindex] += ColorF(0.0, 1.0, 0.0);*/


                // stats...
            sgStatistics::sgTerrainLexelCount++;


            point += s;
        }

        point = ((t * lmy) + start + (s * lmindexmin.x));
    }

    model.sgResetState();


    // stats...
    sgStatistics::sgTerrainLexelTime += Platform::getRealMilliseconds() - time;
}

void sgTerrainLightMap::sgMergeLighting(ColorF* lightmap)
{
    sgTexels->sgBlur();

    U32 y, x, index;
    for (y = 0; y < sgHeight; y++)
    {
        for (x = 0; x < sgWidth; x++)
        {
            index = (y * sgWidth) + x;
            ColorF& pixel = lightmap[index];
            pixel += sgTexels->sgData[index];
        }
    }
}
/*
void sgTerrainLightMap::sgMergeLighting(ColorF *lightmap)
{
    U32 y, x, index;
    for(y=0; y<sgHeight; y++)
    {
        for(x=0; x<sgWidth; x++)
        {
            index = (y * sgWidth) + x;
            ColorF pixel = sgTexels[index];

            pixel += sgTexels[sgGetIndex(sgWidth, sgHeight, x+1, y)];
            pixel += sgTexels[sgGetIndex(sgWidth, sgHeight, x+1, y+1)];
            pixel += sgTexels[sgGetIndex(sgWidth, sgHeight, x, y+1)];

            pixel *= 0.25f;

            lightmap[index] += pixel;
        }
    }
}
*/
//----------------------------------------------

void sgAtlasLightMap::sgSetupLighting()
{
    MathUtils::transformBoundingBox(sgStub->mBounds,
        sgAtlas->getRenderTransform(), sgChunkBox);

    sgAtlasSurfaceHelper.mChunk = sgStub->mChunk;
    sgAtlasSurfaceHelper.mTexBounds = sgStub->mChunk->mTCBounds;
    sgAtlasSurfaceHelper.generateLookupTables();
}

void sgAtlasLightMap::sgCalculateLighting(LightInfo* light)
{
    // setup zone info...
    bool isinzone = (light->sgZone[0] == 0) || (light->sgZone[1] == 0);

    // allow what?
    bool allowdiffuse = (!light->sgDiffuseRestrictZone) || isinzone;
    bool allowambient = (!light->sgAmbientRestrictZone) || isinzone;

    // should I bother?
    if ((!allowdiffuse) && (!allowambient))
        return;


    AssertFatal((sgAtlas), "Member 'sgAtlas' must be populated.");

    U32 time = Platform::getRealMilliseconds();
    /*
        Point2F s, t;
        s[0] = sgLightMapSVector[0];
        s[1] = sgLightMapSVector[1];
        t[0] = sgLightMapTVector[0];
        t[1] = sgLightMapTVector[1];
        Point2F run = t * sgWidth;

        Point2F start;
        start[0] = sgWorldPosition[0] + halfterrainlength;
        start[1] = sgWorldPosition[1] + halfterrainlength;
    */
    sgLightingModel& model = sgLightingModelManager::sgGetLightingModel(
        light->sgLightingModelName);
    model.sgSetState(light);

    // test for early out...
    if (!model.sgCanIlluminate(sgChunkBox))
    {
        model.sgResetState();

        // stats...
        sgStatistics::sgTerrainLexelTime += Platform::getRealMilliseconds() - time;
        return;
    }

    model.sgInitStateLM();

    /*	Point2I lmindexmin, lmindexmax;
        if(light->mType == LightInfo::Vector)
        {
            lmindexmin.x = 0;
            lmindexmin.y = 0;
            lmindexmax.x = (sgWidth - 1);
            lmindexmax.y = (sgHeight - 1);
        }
        else
        {
            F32 maxrad = model.sgGetMaxRadius();
            Box3F worldbox = Box3F(light->mPos, light->mPos);
            worldbox.min -= Point3F(maxrad, maxrad, maxrad);
            worldbox.max += Point3F(maxrad, maxrad, maxrad);

            lmindexmin.x = (worldbox.min.x - sgWorldPosition.x) / s.x;
            lmindexmin.y = (worldbox.min.y - sgWorldPosition.y) / t.y;
            lmindexmax.x = (worldbox.max.x - sgWorldPosition.x) / s.x;
            lmindexmax.y = (worldbox.max.y - sgWorldPosition.y) / t.y;

            lmindexmin.x = getMax(lmindexmin.x, S32(0));
            lmindexmin.y = getMax(lmindexmin.y, S32(0));
            lmindexmax.x = getMin(lmindexmax.x, S32(sgWidth - 1));
            lmindexmax.y = getMin(lmindexmax.y, S32(sgHeight - 1));
        }*/

    S32 lmx, lmy, lmindex;
    Point3F lexelworldpos;
    Point3F lexelworldnormal;
    Point2F lexeluv;
    ColorF diffuse = ColorF(0.0, 0.0, 0.0);
    ColorF ambient = ColorF(0.0, 0.0, 0.0);
    Point3F lightingnormal = Point3F(0.0, 0.0, 0.0);

    //Point2F point = ((t * lmindexmin.y) + start + (s * lmindexmin.x));

    //lexelworldpos = sgWorldPosition -
    //	(sgLightMapSVector * (F32(sgWidth) * 0.5)) -
    //	(sgLightMapTVector * (F32(sgHeight) * 0.5));

    //Point3F start = lexelworldpos;

    lexeluv = sgAtlasSurfaceHelper.mChunk->mTCBounds.point;
    Point2F startuv = lexeluv;
    F32 uvsoffset = 1.0f / F32(sgWidth) * sgAtlasSurfaceHelper.mChunk->mTCBounds.extent.x;
    F32 uvtoffset = 1.0f / F32(sgWidth) * sgAtlasSurfaceHelper.mChunk->mTCBounds.extent.y;

    bool halfsize = !LightManager::sgAllowFullLightMaps();
    U32 lmscalemask = LightManager::sgGetLightMapScale() - 1;

    for (lmy = 0; lmy < sgHeight; lmy++)
    {
        if (halfsize && (lmy & lmscalemask))
        {
            startuv.y += uvtoffset;
            lexeluv = startuv;
            continue;
        }

        for (lmx = 0; lmx < sgWidth; lmx++)
        {
            if (halfsize && (lmx & lmscalemask))
            {
                lexeluv.x += uvsoffset;
                continue;
            }

            lmindex = (lmx + (lmy * sgWidth));

            // get lexel 2D world pos...
            //lexelworldpos[0] = point[0] - halfterrainlength;
            //lexelworldpos[1] = point[1] - halfterrainlength;

            // use 2D terrain space pos to get the world space z and normal...
            //sgAtlas->getNormalAndHeight(lexelworldpos, normal, lexelworldpos.z, false);
            //Point3F n = normal;
            //normal.normalizeSafe();
            //normal = (normal * 0.5) + Point3F(0.5, 0.5, 0.5);
            U16 temp;
            if (!sgAtlasSurfaceHelper.lookup(lexeluv, temp, lexelworldpos, lexelworldnormal))
                continue;

            // this is not good - very slow...
            sgAtlas->getRenderTransform().mulP(lexelworldpos);
            sgAtlas->getRenderTransform().mulV(lexelworldnormal);

            // too often unset, must do these here...
            ambient = diffuse = ColorF(0.0f, 0.0f, 0.0f);
            lightingnormal = VectorF(0.0f, 0.0f, 0.0f);
            model.sgLightingLM(lexelworldpos, lexelworldnormal, diffuse, ambient, lightingnormal);

            //diffuse = light->mColor;
            //diffuse *= mDot(normal, -light->mDirection);

            //diffuse.red = normal.x;
            //diffuse.green = normal.y;
            //diffuse.blue = normal.z;

            if (allowdiffuse && ((diffuse.red > SG_MIN_LEXEL_INTENSITY) ||
                (diffuse.green > SG_MIN_LEXEL_INTENSITY) || (diffuse.blue > SG_MIN_LEXEL_INTENSITY)))
            {
                // step four: check for shadows...

                bool shadowed = false;
                RayInfo info;
                if (light->sgCastsShadows && LightManager::sgAllowShadows())
                {
                    // set light pos for shadows...
                    Point3F lightpos = light->mPos;
                    if (light->mType == LightInfo::Vector)
                    {
                        lightpos = SG_STATIC_LIGHT_VECTOR_DIST * light->mDirection * -1;
                        //lightpos = Point3F(0, 0, SG_STATIC_LIGHT_VECTOR_DIST);
                        lightpos = lexelworldpos + lightpos;
                    }

                    // make texels terrain space coord into a world space coord...
                    RayInfo info;
                    //if(sgAtlas->getContainer()->castRay(lightpos, (lexelworldpos + (lightingnormal * 0.5)),
                    if (sgAtlas->getContainer()->castRay((lexelworldpos + (lexelworldnormal * 0.1)), lightpos,
                        //Point3F lp, wp;
                        //sgAtlas->getRenderWorldTransform().mulP(lightpos, &lp);
                        //sgAtlas->getRenderWorldTransform().mulP((lexelworldpos + (lightingnormal * 2.0)), &wp);
                        //if(sgAtlas->castRay(lp, wp,
                        ShadowCasterObjectType, &info))
                        //&info))
                    {
                        shadowed = true;
                    }
                }

                if (!shadowed)
                {
                    // step five: apply the lighting to the light map...
                    sgTexels->sgData[lmindex] += diffuse;
                }
            }

            if (allowambient && ((ambient.red > 0.0f) || (ambient.green > 0.0f) || (ambient.blue > 0.0f)))
            {
                sgTexels->sgData[lmindex] += ambient;
            }

            /*if((lmx & 0x1) == (lmy & 0x1))
                sgTexels[lmindex] += ColorF(1.0, 0.0, 0.0);
            else
                sgTexels[lmindex] += ColorF(0.0, 1.0, 0.0);*/


                // stats...
            sgStatistics::sgTerrainLexelCount++;


            //point += s;
            lexeluv.x += uvsoffset;
        }

        //point = ((t * lmy) + start + (s * lmindexmin.x));
        startuv.y += uvtoffset;
        lexeluv = startuv;
    }

    model.sgResetState();


    // stats...
    sgStatistics::sgTerrainLexelTime += Platform::getRealMilliseconds() - time;
}

void sgAtlasLightMap::sgMergeLighting(GBitmap* lightmap, U32 xoffset, U32 yoffset)
{
    sgTexels->sgFillInLighting();
    sgTexels->sgBlur();

    U32 y, x, d;
    U8* bits;

    ColorF* texels = sgTexels->sgData;

    for (y = 0; y < sgHeight; y++)
    {
        bits = lightmap->getAddress(xoffset, (yoffset + y));
        d = 0;

        for (x = 0; x < sgWidth; x++)
        {
            ColorF curtexel = texels[((y * sgWidth) + x)];

            curtexel *= 255.0f;

            // save (BGRA)...
            bits[d++] = mClamp(((U32)curtexel.blue), 0, 255);
            bits[d++] = mClamp(((U32)curtexel.green), 0, 255);
            bits[d++] = mClamp(((U32)curtexel.red), 0, 255);
            d++;
        }
    }
}
#endif
