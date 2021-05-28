//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "sceneGraph/detailManager.h"
#include "ts/tsShapeInstance.h"
#include "ts/tsPartInstance.h"

// bias towards using same detail level as previous frame
#define MatchPreviousReward 0.99f
#define NotMatchPreviousPenalty 1.01f

// this is the pref value that the user should be able to set
F32 DetailManager::smDetailScale = 1.0f;
// this should be fixed at some large upper-bound
S32 DetailManager::smMaxPolyLimit = 20000;
// this should be fixed at some small lower-bound
S32 DetailManager::smMinPolyLimit = 2000;
// this determines how much we can under-shoot poly limit when reducing
S32 DetailManager::smLimitRange = 1000;

// default detail profile -- delay detailing out 2 times...
DetailManager::DetailProfile DetailManager::smDefaultProfile = { 0, 0, 2 };

DetailManager* DetailManager::smDetailManager = NULL;

S32 DetailManager::smPolysDidRender = 0;
S32 DetailManager::smPolysTriedToRender = 0;

S32 QSORT_CALLBACK FN_CDECL compareDetailData(const void* e1, const void* e2)
{
    const DetailManager::DetailData* dd1 = *(const DetailManager::DetailData**)e1;
    const DetailManager::DetailData* dd2 = *(const DetailManager::DetailData**)e2;

    if (dd1->priority < dd2->priority)
        return -1;
    else if (dd2->priority < dd1->priority)
        return 1;
    else
        return 0;
}

//---------------------------------------------------------
DetailManager::DetailManager()
{
    mInPrepRender = false;
}

DetailManager::~DetailManager()
{
    U32 i;
    for (i = 0; i < mDetailData.size(); i++)
        delete mDetailData[i];
    mDetailData.clear();
    for (i = 0; i < mFreeDetailData.size(); i++)
        delete mFreeDetailData[i];
    mFreeDetailData.clear();
}

#ifdef TORQUE_DEBUG
void DetailManager::clean()
{
    for (S32 i = 0; i < mFreeDetailData.size(); i++)
        delete mFreeDetailData[i];
    mFreeDetailData.clear();
    if (mDetailData.size())
        Con::errorf("Detail manager still has %i detail data structures left", mDetailData.size());
    mFreeDetailData.compact();
    mDetailData.compact();
}
#endif
//---------------------------------------------------------

inline void DetailManager::bumpOne(DetailData* dd, S32 bump)
{
    dd->dl = dd->dls[bump];
    dd->intraDL = 1.0f;
    mPolyCount -= dd->bump[bump];
    for (S32 i = 0; i < MAX_BUMP; i++)
        dd->bump[i] -= dd->bump[bump];
}

void DetailManager::bumpAll(S32 bump)
{
    for (U32 i = 0; i < mDetailData.size(); i++)
        bumpOne(mDetailData[i], bump);
}

//---------------------------------------------------------
void DetailManager::begin()
{
    // enter prepRender stage
    mInPrepRender = true;

    // inc tag...
    mTag++;

    // clear poly count
    mPolyCount = 0;

    // set poly limit for this frame
    mPolyLimit = (S32)(smMinPolyLimit + smDetailScale * (smMaxPolyLimit - smMinPolyLimit));

    // clear bump count
    for (U32 i = 0; i < MAX_BUMP; i++)
        mBumpPolyCount[i] = 0;
}

void DetailManager::end()
{
    S32 i;

    // leave prepRender stage
    mInPrepRender = false;

    // update poly count for new frame
    smPolysDidRender = mPolyCount;
    smPolysTriedToRender = mPolyCount;

    // drop detailData for shapes rendered last time but not this time
    for (i = mDetailData.size() - 1; i >= 0; i--)
        if (mDetailData[i]->tag != mTag)
        {
            // not rendering this time
            mDetailData[i]->shapeInstance = NULL; // enough to disconnect it from shape instance...
            mDetailData[i]->partInstance = NULL;  // enough to disconnect it from part instance...
            mFreeDetailData.push_back(mDetailData[i]);
            mDetailData.erase(i);
        }

    // we may already be below the poly limit
    if (mPolyCount < getMax(1, mPolyLimit))
        // we accept the preferred details
        return;
    /*
       // do as many full bumps as we can
       S32 bump;
       for (bump=0; bump<MAX_BUMP; bump++)
          if (mPolyCount-mBumpPolyCount[bump]<mPolyLimit)
             break;
       if (bump)
          // bump everyone this many times...
          bumpAll(bump-1);

       // update poly count for new frame
       smPolysDidRender = mPolyCount;

       if (bump==MAX_BUMP)
          // can do no more...
          return;

       // compute priority function...sort
       for (i=0; i<mDetailData.size(); i++)
          computePriority(mDetailData[i],bump);
       dQsort(mDetailData.address(),mDetailData.size(),sizeof(DetailData*),compareDetailData);

       // keep reducing until done...
       for (i=0; i<mDetailData.size(); i++)
       {
          if (mPolyCount-mDetailData[i]->bump[bump] > mPolyLimit-smLimitRange)
          {
             bumpOne(mDetailData[i],bump);
             if (mPolyCount<mPolyLimit)
                // we're done...
                break;
          }
       }
    */
    // update poly count for new frame
    smPolysDidRender = mPolyCount;
}

//---------------------------------------------------------
void DetailManager::selectPotential(TSShapeInstance* si, F32 dist, F32 invScale, const DetailProfile* dp)
{
    AssertFatal(mInPrepRender, "DetailManager::selectPotentialDetails");

    dist *= invScale;
    S32 dl = si->selectCurrentDetail2(dist);
    if (dl < 0)
        // don't render
        return;

    DetailData* dd;
    if (!si->mData || ((DetailData*)si->mData)->shapeInstance != si)
    {
        // we weren't rendered last time
        // get new detail data and set prevDL to -2 to encode that we're fresh meat...
        si->mData = getNewDetailData();
        dd = (DetailData*)si->mData;
        dd->prevDL = -2;
    }
    else
    {
        // we were rendered last time
        dd = (DetailData*)si->mData;
        dd->prevDL = dd->dl;
    }

    dd->shapeInstance = si;
    dd->partInstance = NULL;
    dd->tag = mTag;
    dd->dl = dl;
    dd->pixelSize = -dist; //pixelRadius; BROKEN, but this'll be fine for now
    dd->intraDL = si->getCurrentIntraDetail();

    // add in poly count for preferred detail level
    S32 polyCount = si->getShape()->details[dl].polyCount;
    mPolyCount += polyCount;

    S32 countFirst = 0;
    S32 countMiddle = 0;
    S32 countLast = 0;
    for (S32 i = 0; i < MAX_BUMP; i++)
    {
        // duplicate first detail...or...
        bool dup = false;
        if (dl == 0 && countFirst < dp->skipFirst)
        {
            countFirst++;
            dup = true;
        }

        // duplicate last detail...or...
        if (dl == si->getShape()->mSmallestVisibleDL && countLast < dp->skipLast)
        {
            countLast++;
            dup = true;
        }

        // duplicate other details...
        if (countMiddle < dp->skipMiddle)
        {
            countMiddle++;
            dup = true;
        }
        else
            countMiddle = 0;

        // find the next detail...
        if (!dup)
        {
            if (dl == si->getShape()->mSmallestVisibleDL)
                dl = -1;
            else
                dl++;
        }

        dd->dls[i] = dl;
        S32 detailPolys = dl >= 0 ? si->getShape()->details[dl].polyCount : 0;
        dd->bump[i] = polyCount - detailPolys;
        mBumpPolyCount[i] += dd->bump[i];

        if (dl < 0)
        {
            for (S32 j = i + 1; j < MAX_BUMP; j++)
            {
                dd->dls[j] = -1;
                dd->bump[j] = polyCount;
                mBumpPolyCount[j] += dd->bump[j];
            }
            break;
        }
    }
}

//---------------------------------------------------------
void DetailManager::selectPotential(TSPartInstance* pi, F32 dist, F32 invScale, const DetailProfile* dp)
{
    AssertFatal(mInPrepRender, "DetailManager::selectPotentialDetails");

    dist *= invScale;
    pi->selectCurrentDetail2(dist);
    S32 dl = pi->getCurrentObjectDetail();
    if (dl < 0)
        // don't render
        return;

    DetailData* dd;
    if (!pi->mData || ((DetailData*)pi->mData)->partInstance != pi)
    {
        // we weren't rendered last time
        // get new detail data and set prevDL to -2 to encode that we're fresh meat...
        pi->mData = getNewDetailData();
        dd = (DetailData*)pi->mData;
        dd->prevDL = -2;
    }
    else
    {
        // we were rendered last time
        dd = (DetailData*)pi->mData;
        dd->prevDL = dd->dl;
    }

    dd->shapeInstance = NULL;
    dd->partInstance = pi;
    dd->tag = mTag;
    dd->dl = dl;
    dd->pixelSize = -dist;
    dd->intraDL = pi->getCurrentIntraDetail();

    // add in poly count for preferred detail level
    S32 polyCount = pi->getPolyCount(dl);
    mPolyCount += polyCount;

    S32 countFirst = 0;
    S32 countMiddle = 0;
    S32 countLast = 0;
    for (S32 i = 0; i < MAX_BUMP; i++)
    {
        // duplicate first detail...or...
        bool dup = false;
        if (dl == 0 && countFirst < dp->skipFirst)
        {
            countFirst++;
            dup = true;
        }

        // duplicate last detail...or...
        if (dl == pi->getNumDetails() - 1 && countLast < dp->skipLast)
        {
            countLast++;
            dup = true;
        }

        // duplicate other details...
        if (countMiddle < dp->skipMiddle)
        {
            countMiddle++;
            dup = true;
        }
        else
            countMiddle = 0;

        // find the next detail...
        if (!dup)
        {
            if (dl == pi->getNumDetails() - 1)
                dl = -1;
            else
                dl++;
        }

        dd->dls[i] = dl;
        S32 detailPolys = pi->getPolyCount(dl);
        dd->bump[i] = polyCount - detailPolys;
        mBumpPolyCount[i] += dd->bump[i];

        if (dl < 0)
        {
            for (S32 j = i + 1; j < MAX_BUMP; j++)
            {
                dd->dls[j] = -1;
                dd->bump[j] = polyCount;
                mBumpPolyCount[j] += dd->bump[j];
            }
            break;
        }
    }
}

//---------------------------------------------------------
bool DetailManager::selectCurrent(TSShapeInstance* si)
{


    DetailData* dd = (DetailData*)si->mData;
    if (!dd || dd->shapeInstance != si)
        // not rendering...
        return false;
    si->setCurrentDetail(dd->dl, dd->intraDL);
    return true;
}

//---------------------------------------------------------
bool DetailManager::selectCurrent(TSPartInstance* pi)
{


    DetailData* dd = (DetailData*)pi->mData;
    if (!dd || dd->partInstance != pi)
        // not rendering...
        return false;
    pi->setCurrentObjectDetail(dd->dl);
    pi->setCurrentIntraDetail(dd->intraDL);
    return true;
}

//---------------------------------------------------------
void DetailManager::computePriority(DetailData* detailData, S32 bump)
{
    S32 oldSize, newSize;
    if (detailData->shapeInstance)
    {
        // shape instance...
        if (bump > 0)
            oldSize = (S32)(detailData->dl >= 0 ? detailData->shapeInstance->getShape()->details[detailData->dl].size : 0);
        else
            oldSize = (S32)detailData->pixelSize;
        newSize = (S32)(detailData->dls[bump] >= 0 ? detailData->shapeInstance->getShape()->details[detailData->dls[bump]].size : 0);
    }
    else
    {
        // part instance...
        AssertFatal(detailData->partInstance, "DetailManager::computePriority");
        if (bump > 0)
            oldSize = (S32)detailData->partInstance->getDetailSize(detailData->dl);
        else
            oldSize = (S32)detailData->pixelSize;
        newSize = (S32)detailData->partInstance->getDetailSize(detailData->dls[bump]);
    }

    // priority weighted by both total bump and recent bump (total bump component helps break ties
    // when recent bump has the same effect).
    detailData->priority = 0.5f * (detailData->pixelSize - oldSize) + (oldSize - newSize);

    // try to be consistent between frames...as much as possible
    if (detailData->prevDL < 0)
    {
        if (detailData->prevDL == -1)
        {
            // weren't visible last time...penalize for being visible this time, reward for being invisible
            if (detailData->dls[bump] == -1)
                detailData->priority *= MatchPreviousReward;
            else if (detailData->dl != -1)
                detailData->priority *= NotMatchPreviousPenalty;
        }
    }
    else
    {
        // reward for bumping us down if we are at a higher detail than last time,
        // penalize for bumping us down if our detail is at or lower than last time
        // Note: higher detail --> smaller dl number
        if (detailData->dl > detailData->prevDL)
            detailData->priority *= MatchPreviousReward;
        else
            detailData->priority *= NotMatchPreviousPenalty;
    }
}

//---------------------------------------------------------
DetailManager::DetailData* DetailManager::getNewDetailData()
{
    DetailData* ret;
    if (mFreeDetailData.size())
    {
        ret = mFreeDetailData.last();
        mFreeDetailData.decrement();
    }
    else
        ret = new DetailData;
    mDetailData.push_back(ret);
    return ret;
}

#ifdef TORQUE_DEBUG
ConsoleFunction(cleanDetailManager, void, 1, 1, "cleanDetailManager();")
{
    DetailManager::beginPrepRender();
    DetailManager::endPrepRender();
    DetailManager::Clean();
}
#endif
