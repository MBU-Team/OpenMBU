//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "console/console.h"
#include "terrain/terrData.h"
#include "editor/terrain/terraformer.h"
#include "gui/editor/guiFilterCtrl.h"
#include "editor/editor.h"
#include "platform/event.h"
#include "game/gameConnection.h"

#include "core/fileStream.h"



inline F32 lerp(F32 t, F32 a, F32 b)
{
    return a + t * (b - a);
}


inline F32 curve(F32 t)
{
    return t * t * (3.0f - 2.0f * t);
}


F32 getAlpha(U32 x, U32 y, Heightfield* alpha)
{
    F32 xFactor = F32(x & 7) * (1.0f / 8.0f);
    F32 yFactor = F32(y & 7) * (1.0f / 8.0f);
    U32 xi = x >> 3;
    U32 yi = y >> 3;

    F32 a0 = alpha->val(xi, yi);
    F32 a1 = alpha->val(xi + 1, yi);
    F32 a2 = alpha->val(xi + 1, yi + 1);
    F32 a3 = alpha->val(xi, yi + 1);

    //   F32 ah0 = (a0 * (1.0f-xFactor)) + (a1 * xFactor);
    //   F32 ah1 = (a3 * (1.0f-xFactor)) + (a2 * xFactor);
    //
    //   F32 a = (ah0 * (1.0f-yFactor)) + (ah1 * yFactor);

       //xFactor = curve(xFactor);
       //yFactor = curve(yFactor);

    F32 ah0 = lerp(xFactor, a0, a1);
    F32 ah1 = lerp(xFactor, a3, a2);
    F32 a = lerp(yFactor, ah0, ah1);

    return (a * a);
}


GBitmap* merge(VectorPtr<Heightfield*>& alpha, VectorPtr<GBitmap*>& material)
{
    // due to memory constraints we build the the output bitmap one scan-line at a time.
    F32 sum[2048];
    GBitmap* bitmap = new GBitmap(2048, 2048, false);
    VectorPtr<Heightfield*>::iterator itrA;
    VectorPtr<GBitmap*>::iterator itrM;

    for (S32 y = 0; y < 2048; y++)
    {
        // first compute the sum of the alphas at each pixel
        S32 x;
        for (x = 0; x < 2048; x++)
        {
            sum[x] = 0.0f;
            for (itrA = alpha.begin(); itrA != alpha.end(); itrA++)
                sum[x] += getAlpha(x, y, *itrA);
        }

        // blend the pixels
        for (x = 0; x < 2048; x++)
        {
            ColorI blend(0, 0, 0, 0);
            if (sum[x] > 0.0f)
            {
                F32 fsum = sum[x];
                F32 scaleFactor = (1.0f / fsum);
                for (itrA = alpha.begin(), itrM = material.begin(); itrM != material.end(); itrM++, itrA++)
                {
                    ColorI color;
                    GBitmap* bmp = *itrM;
                    bmp->getColor(x % bmp->getWidth(), y % bmp->getHeight(), color);
                    color *= getAlpha(x, y, *itrA) * scaleFactor;
                    blend.red += color.red;
                    blend.green += color.green;
                    blend.blue += color.blue;
                }
            }
            else
            {
                GBitmap* mat = *material.begin();
                mat->getColor(x % mat->getWidth(), y % mat->getHeight(), blend);
            }
            bitmap->setColor(x, y, blend);
        }
    }
    return bitmap;
}


//--------------------------------------
bool Terraformer::setMaterials(const char* r_src, const char* materials)
{
    TerrainBlock* serverTerrBlock = dynamic_cast<TerrainBlock*>(Sim::findObject("Terrain"));
    if (!serverTerrBlock)
        return false;

    NetConnection* toServer = NetConnection::getConnectionToServer();
    NetConnection* toClient = NetConnection::getLocalClientConnection();

    S32 index = toClient->getGhostIndex(serverTerrBlock);

    TerrainBlock* clientTerrBlock = dynamic_cast<TerrainBlock*>(toServer->resolveGhost(index));
    if (!clientTerrBlock)
        return false;

    VectorPtr<Heightfield*> src;
    VectorPtr<char*> dml;
    Vector<S32> dmlIndex;

    //--------------------------------------
    // extract the source registers
    char buffer[1024];
    dStrcpy(buffer, r_src);
    char* str = dStrtok(buffer, " \0");
    while (str)
    {
        src.push_back(getRegister(dAtof(str)));
        str = dStrtok(NULL, " \0");
    }

    //--------------------------------------
    // extract the materials
    dStrcpy(buffer, materials);
    str = dStrtok(buffer, " \0");
    while (str)
    {
        S32 i;
        for (i = 0; i < dml.size(); i++)
            if (dStricmp(str, dml[i]) == 0)
                break;

        // a unique material list ?
        if (i == dml.size())
            dml.push_back(str);

        // map register to dml
        dmlIndex.push_back(i);

        str = dStrtok(NULL, " \0");
    }

    if (dml.size() > TerrainBlock::MaterialGroups)
    {
        Con::printf("maximum number of DML Material Exceeded");
        return false;
    }

    // install the new DMLs
    clientTerrBlock->setBaseMaterials(dml.size(), (const char**)dml.address());

    //--------------------------------------
    // build alpha masks for each material type

    for (S32 y = 0; y < blockSize; y++)
    {
        for (S32 x = 0; x < blockSize; x++)
        {
            // skip? (cannot skip if index is out of range...)
            F32 total = 0;
            F32 matVals[TerrainBlock::MaterialGroups];
            S32 i;

            for (i = 0; i < TerrainBlock::MaterialGroups; i++)
                matVals[i] = 0;

            for (i = 0; i < src.size(); i++)
            {
                matVals[dmlIndex[i]] += src[i]->val(x, y);
                total += src[i]->val(x, y);
            }

            if (total == 0)
            {
                matVals[0] = 1;
                total = 1;
            }

            // axe out any amount that is less than the threshold
            F32 threshold = 0.15 * total;
            for (i = 0; i < TerrainBlock::MaterialGroups; i++)
                if (matVals[i] < threshold)
                    matVals[i] = 0;

            total = 0;
            for (i = 0; i < TerrainBlock::MaterialGroups; i++)
                total += matVals[i];

            for (i = 0; i < TerrainBlock::MaterialGroups; i++)
            {
                U8* map = clientTerrBlock->getMaterialAlphaMap(i);
                map[x + (y << TerrainBlock::BlockShift)] = (U8)(255 * matVals[i] / total);
            }

            S32 material = 0;
            F32 best = 0.0f;
            for (i = 0; i < src.size(); i++)
            {
                F32 value = src[i]->val(x, y);
                if (value > best)
                {
                    material = dmlIndex[i];
                    best = value;
                }
            }
            // place the material
            *clientTerrBlock->getBaseMaterialAddress(x, y) = material;
        }
    }

    // make it so!
    clientTerrBlock->buildGridMap();
    clientTerrBlock->buildMaterialMap();

    // reload the material lists?
    if (gEditingMission)
        clientTerrBlock->refreshMaterialLists();

    //--------------------------------------------------------------------------
    // for mow steal the first bitmap out of each dml

    if (Con::getBoolVariable("$terrainTestBmp", false) == true)
    {
        VectorPtr<GBitmap*> mats;
        for (S32 i = 0; i < dmlIndex.size(); i++)
        {
            /*
                     Resource<MaterialList> mlist = ResourceManager->load(dml[dmlIndex[i]]);
                     mlist->load();
                     GBitmap *bmp = mlist->getMaterial(0).getBitmap();
                     mats.push_back(bmp);
            */
        }
        GBitmap* texture = merge(src, mats);

        FileStream stream;
        stream.open("terrain.png", FileStream::Write);
        texture->writePNG(stream);
        stream.close();
        delete texture;
    }

    return true;
}


//--------------------------------------
bool Terraformer::mergeMasks(const char* r_src, U32 r_dst)
{
    Heightfield* dst = getRegister(r_dst);
    VectorPtr<Heightfield*> src;

    // extract the source registers
    char buffer[1024];
    dStrcpy(buffer, r_src);
    char* reg = dStrtok(buffer, " \0");
    while (reg)
    {
        src.push_back(getRegister(dAtoi(reg)));
        reg = dStrtok(NULL, " \0");
    }

    // if no masks set the destination to Zero
    if (src.size() == 0)
    {
        for (S32 i = 0; i < (blockSize * blockSize); i++)
            dst->val(i) = 0.0f;
        return true;
    }

    if (src.size() == 1)
    {
        for (S32 i = 0; i < (blockSize * blockSize); i++)
            dst->val(i) = src[0]->val(i);
        return true;
    }

    // store the MAX of the masks into dst
    for (S32 i = 0; i < (blockSize * blockSize); i++)
    {
        F32 value = src[0]->val(i);
        for (S32 j = 1; j < src.size(); j++)
            value *= src[j]->val(i);
        dst->val(i) = value;
    }

    return true;
}


//--------------------------------------
bool Terraformer::maskFBm(U32 r_dst, U32 interval, F32 roughness, U32 seed, const Filter& filter, bool distort, U32 r_distort)
{
    Heightfield* dst = getRegister(r_dst);
    noise.setSeed(seed);
    noise.fBm(dst, blockSize, interval, 1.0 - roughness, 3.0f);

    scale(r_dst, r_dst, 0.0f, 1.0f);

    for (S32 i = 0; i < (blockSize * blockSize); i++)
        dst->val(i) = filter.getValue(dst->val(i));

    if (distort)
    {
        Heightfield* d = getRegister(r_distort);
        for (S32 i = 0; i < (blockSize * blockSize); i++)
            dst->val(i) *= d->val(i);
    }
    return true;
}


//--------------------------------------
bool Terraformer::maskHeight(U32 r_src, U32 r_dst, const Filter& filter, bool distort, U32 r_distort)
{
    Heightfield* src = getRegister(r_src);
    Heightfield* dst = getRegister(r_dst);

    scale(r_src, r_dst, 0.0f, 1.0f);

    for (S32 i = 0; i < (blockSize * blockSize); i++)
        dst->val(i) = filter.getValue(dst->val(i));

    if (distort)
    {
        Heightfield* d = getRegister(r_distort);
        for (S32 i = 0; i < (blockSize * blockSize); i++)
            dst->val(i) *= d->val(i);
    }
    return true;
}


//--------------------------------------
bool Terraformer::maskSlope(U32 r_src, U32 r_dst, const Filter& filter, bool distort, U32 r_distort)
{
    Heightfield* src = getRegister(r_src);
    Heightfield* dst = getRegister(r_dst);

    F32 fmin, fmax;
    getMinMax(r_src, &fmin, &fmax);
    F32 scale = worldHeight / (fmax - fmin);

    for (S32 y = 0; y < blockSize; y++)
    {
        for (S32 x = 0; x < blockSize; x++)
        {
            // for each height look at the immediate surrounding heights and find max slope
            F32 array[9];
            F32 maxDelta = 0;
            src->block(x, y, array);
            F32 height = array[4];

            for (S32 i = 0; i < 9; i++)
            {
                F32 delta = mFabs(array[i] - height);
                if ((i & 1) == 0)
                    delta *= 0.70711f;    // compensate for diagonals

                if (delta > maxDelta)
                    maxDelta = delta;
            }
            F32 slopeVal = mAtan(maxDelta * scale, worldTileSize) * (2.0f / M_PI);
            dst->val(x, y) = filter.getValue(mPow(slopeVal, 1.5f));
        }
    }

    if (distort)
    {
        Heightfield* d = getRegister(r_distort);
        for (S32 i = 0; i < (blockSize * blockSize); i++)
            dst->val(i) *= d->val(i);
    }
    return true;
}


//--------------------------------------
bool Terraformer::maskWater(U32 r_src, U32 r_dst, bool distort, U32 r_distort)
{
    Heightfield* src = getRegister(r_src);
    Heightfield* dst = getRegister(r_dst);

    scale(r_src, r_dst, 0.0f, 1.0f);

    for (S32 i = 0; i < (blockSize * blockSize); i++)
        dst->val(i) = (dst->val(i) > worldWater) ? 0.0f : 1.0f;

    if (distort)
    {
        Heightfield* d = getRegister(r_distort);
        for (S32 i = 0; i < (blockSize * blockSize); i++)
            dst->val(i) *= d->val(i);
    }
    return true;
}
