//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "sim/simPath.h"
#include "console/consoleTypes.h"
#include "sim/pathManager.h"
#include "sceneGraph/sceneState.h"
#include "math/mathIO.h"
#include "core/bitStream.h"

extern bool gEditingMission;

//--------------------------------------------------------------------------
//-------------------------------------- Console functions and cmp funcs
//
namespace {

    static ColorF cubeColors[8] = {
       ColorF(0, 0, 0),
       ColorF(1, 0, 0),
       ColorF(0, 1, 0),
       ColorF(0, 0, 1),
       ColorF(1, 1, 0),
       ColorF(1, 0, 1),
       ColorF(0, 1, 1),
       ColorF(1, 1, 1)
    };

    static Point3F cubePoints[8] = {
       Point3F(-1, -1, -1),
       Point3F(-1, -1,  1),
       Point3F(-1,  1, -1),
       Point3F(-1,  1,  1),
       Point3F(1, -1, -1),
       Point3F(1, -1,  1),
       Point3F(1,  1, -1),
       Point3F(1,  1,  1)
    };

    static U32 cubeFaces[6][4] = {
       { 0, 2, 6, 4 },
       { 0, 2, 3, 1 },
       { 0, 1, 5, 4 },
       { 3, 2, 6, 7 },
       { 7, 6, 4, 5 },
       { 3, 7, 5, 1 }
    };

    void wireCube(F32 size, Point3F pos)
    {
        /*
           glDisable(GL_CULL_FACE);

           for(int i = 0; i < 6; i++)
           {
              glBegin(GL_LINE_LOOP);
              for(int vert = 0; vert < 4; vert++)
              {
                 int idx = cubeFaces[i][vert];
                 glColor3f(cubeColors[idx].red, cubeColors[idx].green, cubeColors[idx].blue);
                 glVertex3f(cubePoints[idx].x * size + pos.x, cubePoints[idx].y * size + pos.y, cubePoints[idx].z * size + pos.z);
              }
              glEnd();
           }
        */
    }


    static Point3F wedgePoints[4] = {
       Point3F(-1, -1,  0),
       Point3F(0,  1,  0),
       Point3F(1, -1,  0),
       Point3F(0,-.75, .5),
    };

    static U32 wedgeFaces[4][3] = {
       { 0, 3, 1 },
       { 3, 1, 2 },
       { 0, 3, 2 },
       { 0, 2, 1 }
    };


    void wireWedge(F32 size, Point3F pos)
    {
        /*
           glDisable(GL_CULL_FACE);

           for(int i = 0; i < 4; i++)
           {
              glBegin(GL_LINE_LOOP);
              for(int vert = 0; vert < 3; vert++)
              {
                 int idx = wedgeFaces[i][vert];
                 glColor3f(0,1,0);
                 glVertex3f(wedgePoints[idx].x * size + pos.x, wedgePoints[idx].y * size + pos.y, wedgePoints[idx].z * size + pos.z);
              }
              glEnd();
           }
        */
    }

    ConsoleFunction(pathOnMissionLoadDone, void, 1, 1, "Load all path information from interiors.")
    {
        // Need to load subobjects for all loaded interiors...
        SimGroup* pMissionGroup = dynamic_cast<SimGroup*>(Sim::findObject("MissionGroup"));
        AssertFatal(pMissionGroup != NULL, "Error, mission done loading and no mission group?");

        U32 currStart = 0;
        U32 currEnd = 1;
        Vector<SimGroup*> groups;
        groups.push_back(pMissionGroup);

        while (true) {
            for (U32 i = currStart; i < currEnd; i++) {
                for (SimGroup::iterator itr = groups[i]->begin(); itr != groups[i]->end(); itr++) {
                    if (dynamic_cast<SimGroup*>(*itr) != NULL)
                        groups.push_back(static_cast<SimGroup*>(*itr));
                }
            }

            if (groups.size() == currEnd) {
                break;
            }
            else {
                currStart = currEnd;
                currEnd = groups.size();
            }
        }

        for (U32 i = 0; i < groups.size(); i++) {
            Path* pPath = dynamic_cast<Path*>(groups[i]);
            if (pPath)
                pPath->updatePath();
        }
    }

    S32 FN_CDECL cmpPathObject(const void* p1, const void* p2)
    {
        SimObject* o1 = *((SimObject**)p1);
        SimObject* o2 = *((SimObject**)p2);

        Marker* m1 = dynamic_cast<Marker*>(o1);
        Marker* m2 = dynamic_cast<Marker*>(o2);

        if (m1 == NULL && m2 == NULL)
            return 0;
        else if (m1 != NULL && m2 == NULL)
            return 1;
        else if (m1 == NULL && m2 != NULL)
            return -1;
        else {
            // Both markers...
            return S32(m1->mSeqNum) - S32(m2->mSeqNum);
        }
    }

} // namespace {}


//--------------------------------------------------------------------------
//-------------------------------------- Implementation
//
IMPLEMENT_CONOBJECT(Path);

Path::Path()
{
    mPathIndex = NoPathIndex;
    mIsLooping = true;
}

Path::~Path()
{
    //
}

//--------------------------------------------------------------------------
void Path::initPersistFields()
{
    Parent::initPersistFields();
    addField("isLooping", TypeBool, Offset(mIsLooping, Path));

    //
}



//--------------------------------------------------------------------------
bool Path::onAdd()
{
    if (!Parent::onAdd())
        return false;

    return true;
}


void Path::onRemove()
{
    //

    Parent::onRemove();
}



//--------------------------------------------------------------------------
/// Sort the markers objects into sequence order
void Path::sortMarkers()
{
    dQsort(objectList.address(), objectList.size(), sizeof(SimObject*), cmpPathObject);
}

void Path::updatePath()
{
    // If we need to, allocate a path index from the manager
    if (mPathIndex == NoPathIndex)
        mPathIndex = gServerPathManager->allocatePathId();

    sortMarkers();

    Vector<Point3F> positions;
    Vector<QuatF>   rotations;
    Vector<U32>     times;
    Vector<U32>     smoothingTypes;

    for (iterator itr = begin(); itr != end(); itr++)
    {
        Marker* pMarker = dynamic_cast<Marker*>(*itr);
        if (pMarker != NULL)
        {
            Point3F pos;
            pMarker->getTransform().getColumn(3, &pos);
            positions.push_back(pos);

            QuatF rot;
            rot.set(pMarker->getTransform());
            rotations.push_back(rot);

            times.push_back(pMarker->mMSToNext);
            smoothingTypes.push_back(pMarker->mSmoothingType);
        }
    }

    // DMMTODO: Looping paths.
    gServerPathManager->updatePath(mPathIndex, positions, rotations, times, smoothingTypes);
}

void Path::addObject(SimObject* obj)
{
    Parent::addObject(obj);

    if (mPathIndex != NoPathIndex) {
        // If we're already finished, and this object is a marker, then we need to
        //  update our path information...
        if (dynamic_cast<Marker*>(obj) != NULL)
            updatePath();
    }
}

void Path::removeObject(SimObject* obj)
{
    bool recalc = dynamic_cast<Marker*>(obj) != NULL;

    Parent::removeObject(obj);

    if (mPathIndex != NoPathIndex && recalc == true)
        updatePath();
}

ConsoleMethod(Path, getPathId, S32, 2, 2, "getPathId();")
{
    Path* path = static_cast<Path*>(object);
    return path->getPathIndex();
}


//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
IMPLEMENT_CO_NETOBJECT_V1(Marker);
Marker::Marker()
{
    // Not ghostable unless we're editing...
    mNetFlags.clear(Ghostable);

    mTypeMask = MarkerObjectType;

    mSeqNum = 0;
    mSmoothingType = SmoothingTypeLinear;
    mMSToNext = 1000;
    mSmoothingType = SmoothingTypeSpline;
    mKnotType = KnotTypeNormal;
}

Marker::~Marker()
{
    //
}

//--------------------------------------------------------------------------
static EnumTable::Enums markerEnums[] =
{
   { Marker::SmoothingTypeSpline , "Spline" },
   { Marker::SmoothingTypeLinear , "Linear" },
   { Marker::SmoothingTypeAccelerate , "Accelerate" },
};
static EnumTable markerSmoothingTable(3, &markerEnums[0]);

static EnumTable::Enums knotEnums[] =
{
   { Marker::KnotTypeNormal ,       "Normal" },
   { Marker::KnotTypePositionOnly,  "Position Only" },
   { Marker::KnotTypeKink,          "Kink" },
};
static EnumTable markerKnotTable(3, &knotEnums[0]);


void Marker::initPersistFields()
{
    Parent::initPersistFields();

    addField("seqNum", TypeS32, Offset(mSeqNum, Marker));
    addField("type", TypeEnum, Offset(mKnotType, Marker), 1, &markerKnotTable);
    addField("msToNext", TypeS32, Offset(mMSToNext, Marker));
    addField("smoothingType", TypeEnum, Offset(mSmoothingType, Marker), 1, &markerSmoothingTable);
    endGroup("Misc");
}

//--------------------------------------------------------------------------
bool Marker::onAdd()
{
    if (!Parent::onAdd())
        return false;

    mObjBox = Box3F(Point3F(-.25, -.25, -.25), Point3F(.25, .25, .25));
    resetWorldBox();

    if (gEditingMission)
        onEditorEnable();

    return true;
}


void Marker::onRemove()
{
    if (gEditingMission)
        onEditorDisable();

    Parent::onRemove();
}

void Marker::onGroupAdd()
{
    mSeqNum = getGroup()->size();
}


/// Enable scoping so we can see this thing on the client.
void Marker::onEditorEnable()
{
    mNetFlags.set(Ghostable);
    setScopeAlways();
    addToScene();
}

/// Disable scoping so we can see this thing on the client
void Marker::onEditorDisable()
{
    removeFromScene();
    mNetFlags.clear(Ghostable);
    clearScopeAlways();
}


/// Tell our parent that this Path has been modified
void Marker::inspectPostApply()
{
    Path* path = dynamic_cast<Path*>(getGroup());
    if (path)
        path->updatePath();
}


//--------------------------------------------------------------------------
bool Marker::prepRenderImage(SceneState* state, const U32 stateKey,
    const U32 /*startZone*/, const bool /*modifyBaseState*/)
{
    if (isLastState(state, stateKey))
        return false;
    setLastState(state, stateKey);

    // This should be sufficient for most objects that don't manage zones, and
    //  don't need to return a specialized RenderImage...
    if (state->isObjectRendered(this)) {
        /*
              SceneRenderImage* image = new SceneRenderImage;
              image->obj = this;
              state->insertRenderImage(image);
        */
    }

    return false;
}


void Marker::renderObject(SceneState* state)
{
    /*
       AssertFatal(dglIsInCanonicalState(), "Error, GL not in canonical state on entry");

       RectI viewport;
       glMatrixMode(GL_PROJECTION);
       glPushMatrix();
       dglGetViewport(&viewport);

       // Uncomment this if this is a "simple" (non-zone managing) object
       state->setupObjectProjection(this);

       glMatrixMode(GL_MODELVIEW);
       glPushMatrix();
       dglMultMatrix(&mObjToWorld);
       glScalef(mObjScale.x, mObjScale.y, mObjScale.z);

       // RENDER CODE HERE
       wireWedge(.25, Point3F(0, 0, 0));

       glMatrixMode(GL_MODELVIEW);
       glPopMatrix();

       glMatrixMode(GL_PROJECTION);
       glPopMatrix();
       glMatrixMode(GL_MODELVIEW);
       dglSetViewport(viewport);

       AssertFatal(dglIsInCanonicalState(), "Error, GL not in canonical state on exit");
    */
}


//--------------------------------------------------------------------------
U32 Marker::packUpdate(NetConnection* con, U32 mask, BitStream* stream)
{
    U32 retMask = Parent::packUpdate(con, mask, stream);

    // Note that we don't really care about efficiency here, since this is an
    //  edit-only ghost...
    stream->writeAffineTransform(mObjToWorld);

    return retMask;
}

void Marker::unpackUpdate(NetConnection* con, BitStream* stream)
{
    Parent::unpackUpdate(con, stream);

    // Transform
    MatrixF otow;
    stream->readAffineTransform(&otow);

    setTransform(otow);
}

