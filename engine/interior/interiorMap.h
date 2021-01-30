#ifndef _INTERIORMAP_H_
#define _INTERIORMAP_H_

#include "platform/platform.h"
#include "sim/sceneObject.h"
#include "collision/convex.h"
//#include "dgl/gTexManager.h"
#include "core/tokenizer.h"
#include "collision/convexBrush.h"
#include "collision/abstractPolyList.h"
#include "interior/interiorMapRes.h"

// Pre-define the InteriorMap class so that we can reference it in InteriorMapConvex
class InteriorMap;

// This is the convex collision implementation for InteriorMap
// Once something has collided against a InteriorMap then it creates one of
// these for the actual collisions to occur against
class InteriorMapConvex : public Convex
{
    typedef Convex Parent;
    friend class InteriorMap;  // So the "owner" object can set some properties when it creates this

public:
    // This is where you set the convex type
    // Needs to be defined in convex.h in the ConvexType enum
    InteriorMapConvex() { mType = InteriorMapConvexType; };
    ~InteriorMapConvex() {};

protected:
    Box3F box;             // The bounding box of the convex (in object space)
    InteriorMap* pOwner;   // A pointer back to the "owner" object so we can reference the vertices

    U32 nodeIndex;
    S32 brushIndex;

public:
    // Return the bounding box transformed into world space
    Box3F getBoundingBox() const;
    // Return the bounding box transformed and scaled by the input values
    Box3F getBoundingBox(const MatrixF& mat, const Point3F& scale) const;

    // This returns a list of convex faces to collide against
    void getPolyList(AbstractPolyList* list);

    // Returns the vertices, faces, and edges of our convex
    void getFeatures(const MatrixF& mat, const VectorF& n, ConvexFeature* cf);

    // This returns the furthest point from the input vector
    Point3F support(const VectorF& v) const;

    // A helper function that checks the edgelist for duplicates
    void addEdge(U32 zero, U32 one, U32 base, ConvexFeature* cf);
};

class InteriorMap : public SceneObject
{
    friend class InteriorMapConvex;  // So the "child" convex(es) can reference the vertices

public:
    typedef SceneObject		Parent;

    // Declare Console Object.
    DECLARE_CONOBJECT(InteriorMap);
protected:

    // Create and use these to specify custom events.
    //
    // NOTE:-	Using these allows you to group the changes into related
    //			events.  No need to change everything if something minor
    //			changes.  Only really important if you have *lots* of these
    //			objects at start-up or you send alot of changes whilst the
    //			game is in progress.
    //
    //			Use "setMaskBits(InteriorMapMask)" to signal.
   // - Melv May

    enum
    {
        MoveMask = (1 << 0),
        RenderModeMask = (1 << 1)
    };

    bool							mLoaded;       // Make sure we don't load this twice

public:
    enum
    {
        TexShaded = 0,
        SolidShaded,
        TexWireframe,
        SolidWireframe,
        Edges,
        BrushColors,
        FaceColors,
        BspPolys,
        CollisionHulls,
        Lighting,
        TexLighting
    };

    GFXTexHandle* mTexHandles;

    GFXTexHandle* mWhite;

    StringTableEntry              mFileName;     // The name of the level
    StringTableEntry              mFilePath;     // The path of the level
    StringTableEntry              mTexPath;      // The path to the textures

    U32                           mRenderMode;
    bool                          mEnableLights;

    Resource<InteriorMapResource> mInteriorRes;

    InteriorMapResource::Entity* getEntity(char* classname)
    {
        for (U32 i = 0; i < mInteriorRes->mEntities.size(); i++)
        {
            if (dStricmp(classname, mInteriorRes->mEntities[i]->classname) == 0)
                return mInteriorRes->mEntities[i];
        }

        return NULL;
    }

    void getEntities(char* classname, VectorPtr<InteriorMapResource::Entity*> ents)
    {
        for (U32 i = 0; i < mInteriorRes->mEntities.size(); i++)
        {
            if (dStricmp(classname, mInteriorRes->mEntities[i]->classname) == 0)
            {
                ents.increment();
                ents.last() = mInteriorRes->mEntities[i];
            }
        }
    }

protected:
    Convex* mConvexList;

    // I split the render and load functions out of renderObject() and onAdd() for clarity
    bool render(void);

public:
    // Creation and destruction
    InteriorMap(void);
    virtual ~InteriorMap(void);

    // Utility
    bool setPath(const char* filename);
    bool loadTextures();
    bool calcTexgenDiv();

    // SceneObject
   // renderObject() is the function that gets called for evey SceneObject
    void renderObject(SceneState*);

    // This function gets called to let you define a few proprties like translucency
    virtual bool prepRenderImage(SceneState*, const U32 stateKey, const U32 startZone,
        const bool modifyBaseZoneState = false);

    // SimObject
    bool onAdd();
    void onRemove();
    void onEditorEnable();
    void onEditorDisable();
    void inspectPostApply();

    // NetObject
    U32 packUpdate(NetConnection*, U32, BitStream*);
    void unpackUpdate(NetConnection*, BitStream*);

    // ConObject.
    static void initPersistFields();

    // Collision
   // castRay() returns the percentage along the line from a starting and an ending point
   // to where something collides with the object
    bool castRay(const Point3F&, const Point3F&, RayInfo*);

    // Called whenever something overlaps our bounding box
    // This is where the SceneObject can submit convexes to the working list of an object interested in collision data
    void buildConvex(const Box3F& box, Convex* convex);

    // Script access functions
    void removeBrush(S32 brushIndex);                        // Removes brush from mBrushes and deletes it from memory

    S32 getTextureCount();					                     // Returns the number of textures used in the map
    const char* getTextureName(S32 index);	                  // Returns the name of the texture given the texture's index
    const char* getTexturePathway();			                  // Returns the texture for the map
    Point2I getTextureSize(S32 index);		                  // Returns the requested texture's dimensions

    S32 getBrushIndex(U32 brushID);
};

#endif
