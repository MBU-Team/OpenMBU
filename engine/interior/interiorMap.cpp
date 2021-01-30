#include "interiorMap.h"

#include "core/bitStream.h"
#include "game/gameConnection.h"
#include "math/mathIO.h"
#include "console/consoleTypes.h"
#include "collision/concretePolyList.h"
#include "gfx/gBitmap.h"
#include "math/mPlaneTransformer.h"
#include "gfx/gfxTextureHandle.h"

#define FRONTEPSILON 0.00001

//------------------------------------------------------------------------------
IMPLEMENT_CO_NETOBJECT_V1(InteriorMap);

// Return the bounding box transformed into world space
Box3F InteriorMapConvex::getBoundingBox() const
{
    return getBoundingBox(mObject->getTransform(), mObject->getScale());
}

// Transform and scale the bounding box by the inputs
Box3F InteriorMapConvex::getBoundingBox(const MatrixF& mat, const Point3F& scale) const
{
    Box3F newBox = box;
    newBox.min.convolve(scale);
    newBox.max.convolve(scale);
    mat.mul(newBox);
    return newBox;
}

// Return the point furthest from the input vector
Point3F InteriorMapConvex::support(const VectorF& v) const
{
    Point3F ret(0.0f, 0.0f, 0.0f);
    F32 dp = 0.0f;
    F32 tp = 0.0f;

    // Loop through the points and save the furthest one
    //for (U32 i = 0; i < 8; i++)
    //{
    //	tp = mDot(v, pOwner->mPoints[i]);

    //	if (tp > dp)
    //	{
    //		dp = tp;
    //		ret = pOwner->mPoints[i];
    //	}
    //}

    return ret;
}

// This function simply checks to see if the edge already exists on the edgelist
// If it doesn't the edge gets added
void InteriorMapConvex::addEdge(U32 zero, U32 one, U32 base, ConvexFeature* cf)
{
    U32 newEdge0, newEdge1;

    // Sort the vertex indexes by magnitude (lowest number first, largest second)
    newEdge0 = getMin(zero, one) + base;
    newEdge1 = getMax(zero, one) + base;

    // Assume that it isn't found
    bool found = false;

    // Loop through the edgelist
    // Start with base so we don't search *all* of the edges if there already are some on the list
    for (U32 k = base; k < cf->mEdgeList.size(); k++)
    {
        // If we find a match flag found and break out (no need to keep searching)
        if (cf->mEdgeList[k].vertex[0] == newEdge0 && cf->mEdgeList[k].vertex[1] == newEdge1)
        {
            found = true;
            break;
        }
    }

    // If we didn't find it then add it
    // Otherwise we return without doing anything
    if (!found)
    {
        cf->mEdgeList.increment();
        cf->mEdgeList.last().vertex[0] = newEdge0;
        cf->mEdgeList.last().vertex[1] = newEdge1;
    }
};

// Return the vertices, faces, and edges of the convex
// Used by the vehicle collisions
void InteriorMapConvex::getFeatures(const MatrixF& mat, const VectorF& n, ConvexFeature* cf)
{

}


// Return list(s) of convex faces
// Used by player collisions
void InteriorMapConvex::getPolyList(AbstractPolyList* list)
{
    // Be sure to transform the list into model space
    list->setTransform(&mObject->getTransform(), mObject->getScale());
    // Set the object
    list->setObject(mObject);

    // Get the brush
    ConvexBrush* brush = pOwner->mInteriorRes->mBrushes[brushIndex];

    brush->getPolyList(list);
}

InteriorMap::InteriorMap(void)
{
    // Setup NetObject.
   // Note that we set this as a TutorialObjectType object
   // TutorialObjectType is defined in game/objectTypes.h in the SimObjectTypes enum
   // You should also notify the scripting engine of it existance in game/main.cc in initGame()
    mTypeMask = StaticObjectType | StaticRenderedObjectType | InteriorMapObjectType;
    mNetFlags.set(Ghostable);

    // Haven't loaded yet
    mLoaded = false;

    // Give it a nonexistant bounding box
    mObjBox.min.set(0, 0, 0);
    mObjBox.max.set(0, 0, 0);

    mConvexList = new Convex;
    mTexPath = NULL;
    mRenderMode = TexShaded;
    mEnableLights = true;
    mTexHandles = NULL;
}

InteriorMap::~InteriorMap(void)
{
    delete mConvexList;
    mConvexList = NULL;
}

void InteriorMap::initPersistFields()
{
    // Initialise parents' persistent fields.
    Parent::initPersistFields();

    addField("File", TypeFilename, Offset(mFileName, InteriorMap));
}

bool InteriorMap::onAdd()
{
    if (!Parent::onAdd()) return(false);

    // Set our path info
    setPath(mFileName);

    // Load resource
    mInteriorRes = ResourceManager->load(mFileName, true);
    if (bool(mInteriorRes) == false)
    {
        Con::errorf(ConsoleLogEntry::General, "Unable to load interior: %s", mFileName);
        NetConnection::setLastError("Unable to load interior: %s", mFileName);
        return false;
    }
    else
        mLoaded = true;

    // Scale our brushes
    if (mInteriorRes->mBrushScale != 1.0f)
    {
        for (U32 i = 0; i < mInteriorRes->mBrushes.size(); i++)
            mInteriorRes->mBrushes[i]->setScale(mInteriorRes->mBrushScale);
    }

    loadTextures();

    mInteriorRes->mTexGensCalced = calcTexgenDiv();

    // Set the bounds
    mObjBox.min.set(1e10, 1e10, 1e10);
    mObjBox.max.set(-1e10, -1e10, -1e10);

    for (U32 i = 0; i < mInteriorRes->mBrushes.size(); i++)
    {
        if (mInteriorRes->mBrushes[i]->mStatus == ConvexBrush::Good)
        {
            // Transform the bounding boxes
            MatrixF& mat = mInteriorRes->mBrushes[i]->mTransform;
            Point3F& scale = mInteriorRes->mBrushes[i]->mScale;

            Point3F min = mInteriorRes->mBrushes[i]->mBounds.min;
            Point3F max = mInteriorRes->mBrushes[i]->mBounds.max;

            mat.mulP(min);
            mat.mulP(max);

            min.convolveInverse(scale);
            max.convolveInverse(scale);

            mObjBox.min.setMin(min);
            mObjBox.max.setMax(max);
        }
    }

    mWhite = new GFXTexHandle("common/lighting/whiteNoAlpha", &GFXDefaultStaticDiffuseProfile);

    // Reset the World Box.
    resetWorldBox();
    // Set the Render Transform.
    setRenderTransform(mObjToWorld);

    // Add to Scene.
    addToScene();

    // Return OK.
    return true;
}

bool InteriorMap::setPath(const char* filename)
{
    // Save our filename just in case it hasn't already been done
    mFileName = StringTable->insert(filename);

    // Get the directory
    char dir[4096]; // FIXME: no hardcoded lengths
    if (dStrrchr(filename, '/'))
    {
        dStrncpy(dir, filename, (int)(dStrrchr(filename, '/') - filename + 1));
        dir[(int)(dStrrchr(filename, '/') - filename + 1)] = 0;
    }
    else if (dStrrchr(filename, '\\'))
    {
        dStrncpy(dir, filename, (int)(dStrrchr(filename, '\\') - filename + 1));
        dir[(int)(dStrrchr(filename, '\\') - filename + 1)] = 0;
    }
    else
    {
        dSprintf(dir, dStrlen(Platform::getWorkingDirectory()) + 1, "%s/\0", Platform::getWorkingDirectory());
    }

    mFilePath = StringTable->insert(dir);
    if (!mTexPath)
        mTexPath = mFilePath;

    return true;
}

bool InteriorMap::loadTextures()
{
    if (mTexHandles)
        return true;

    // Load our textures
    mTexHandles = new GFXTexHandle[mInteriorRes->mMaterials.size()];

    for (int t = 0; t < mInteriorRes->mMaterials.size(); t++)
    {
        mTexHandles[t] = NULL;

        char fullname[8192];

        // First go for standard
        dSprintf(fullname, 8192, "%s%s\0", mTexPath, mInteriorRes->mMaterials[t]);

        mTexHandles[t] = GFXTexHandle(fullname, &GFXDefaultStaticDiffuseProfile);
    }

    return true;
}

bool InteriorMap::calcTexgenDiv()
{
    // If we have already calculated our texgen scale then we don't need to do it again
    // This occurs when the client and server are on the same machine (they share Resources)
    if (mInteriorRes->mTexGensCalced)
        return true;

    if (mInteriorRes->mBrushFormat != InteriorMapResource::QuakeOld && mInteriorRes->mBrushFormat != InteriorMapResource::Valve220)
        return false;

    for (U32 i = 0; i < mInteriorRes->mMaterials.size(); i++)
    {
        GFXTextureObject* tex = mTexHandles[i];

        F32 width = 16.0f;
        F32 height = 16.0f;

        if (tex)
        {
            width = tex->getWidth();
            height = tex->getHeight();
        }

        for (U32 j = 0; j < mInteriorRes->mBrushes.size(); j++)
        {
            for (U32 k = 0; k < mInteriorRes->mBrushes[j]->mFaces.mPolyList.size(); k++)
            {
                if (i == mInteriorRes->mBrushes[j]->mFaces.mPolyList[k].material)
                {
                    mInteriorRes->mBrushes[j]->mTexInfos[k].texDiv[0] = width;
                    mInteriorRes->mBrushes[j]->mTexInfos[k].texDiv[1] = height;

                    mInteriorRes->mBrushes[j]->mTexInfos[k].texGens[0].x /= width;
                    mInteriorRes->mBrushes[j]->mTexInfos[k].texGens[0].y /= width;
                    mInteriorRes->mBrushes[j]->mTexInfos[k].texGens[0].z /= width;
                    mInteriorRes->mBrushes[j]->mTexInfos[k].texGens[0].d /= width;

                    mInteriorRes->mBrushes[j]->mTexInfos[k].texGens[1].x /= height;
                    mInteriorRes->mBrushes[j]->mTexInfos[k].texGens[1].y /= height;
                    mInteriorRes->mBrushes[j]->mTexInfos[k].texGens[1].z /= height;
                    mInteriorRes->mBrushes[j]->mTexInfos[k].texGens[1].d /= height;
                }
            }
        }
    }

    // MDFFIX: Probably should move this to its own function
    // Scale the texgens by the brushscale
    for (U32 j = 0; j < mInteriorRes->mBrushes.size(); j++)
    {
        for (U32 k = 0; k < mInteriorRes->mBrushes[j]->mFaces.mPolyList.size(); k++)
        {
            mInteriorRes->mBrushes[j]->mTexInfos[k].texGens[0] *= mInteriorRes->mBrushScale;
            mInteriorRes->mBrushes[j]->mTexInfos[k].texGens[1] *= mInteriorRes->mBrushScale;
        }
    }

    return true;
}

void InteriorMap::onRemove()
{
    mConvexList->nukeList();

    // Remove from Scene.
    removeFromScene();

    // Do Parent.
    Parent::onRemove();
}

void InteriorMap::inspectPostApply()
{
    // Set Parent.
    Parent::inspectPostApply();

    // Set Move Mask.
    setMaskBits(MoveMask);
}

//------------------------------------------------------------------------------

void InteriorMap::onEditorEnable()
{
}

//------------------------------------------------------------------------------

void InteriorMap::onEditorDisable()
{
}

bool InteriorMap::prepRenderImage(SceneState* state, const U32 stateKey, const U32 startZone,
    const bool modifyBaseZoneState)
{
    // Return if last state.
    if (isLastState(state, stateKey)) return false;
    // Set Last State.
    setLastState(state, stateKey);

    // Is Object Rendered?
    if (state->isObjectRendered(this))
    {
        /*
                // Yes, so get a SceneRenderImage.
                SceneRenderImage* image = new SceneRenderImage;
                // Populate it.
                image->obj = this;
                image->isTranslucent = false;
                image->sortType = SceneRenderImage::Normal;

                // Insert it into the scene images.
                state->insertRenderImage(image);
        */
    }

    return false;
}

void InteriorMap::renderObject(SceneState* state)
{
    /*
        // Check we are in Canonical State.
        AssertFatal(dglIsInCanonicalState(), "Error, GL not in canonical state on entry");

        // Save state.
        RectI viewport;

       if (mEnableLights && mRenderMode != BrushColors && mRenderMode != FaceColors && mRenderMode != BspPolys && mRenderMode != CollisionHulls)
       {
          ColorF oneColor(1,1,1,1);
          glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, oneColor);
          installLights();
       }

        // Save Projection Matrix so we can restore Canonical state at exit.
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();

        // Save Viewport so we can restore Canonical state at exit.
        dglGetViewport(&viewport);

        // Setup the projection to the current frustum.
        //
        // NOTE:-	You should let the SceneGraph drive the frustum as it
        //			determines portal clipping etc.
        //			It also leaves us with the MODELVIEW current.
        //
        state->setupBaseProjection();

        // Save ModelView Matrix so we can restore Canonical state at exit.
        glPushMatrix();

        // Transform by the objects' transform e.g move it.
        dglMultMatrix(&getTransform());

       glScalef(mObjScale.x, mObjScale.y, mObjScale.z);

        // I separated out the actual render function to make this a little cleaner
       if (mRenderMode != Edges && mRenderMode != BspPolys && mRenderMode != CollisionHulls)
           render();

       if (mEnableLights && mRenderMode != BrushColors && mRenderMode != FaceColors && mRenderMode != BspPolys && mRenderMode != CollisionHulls)
          uninstallLights();

        // Restore our canonical matrix state.
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        // Restore our canonical viewport state.
        dglSetViewport(viewport);

        // Check we have restored Canonical State.
        AssertFatal(dglIsInCanonicalState(), "Error, GL not in canonical state on exit");
    */
}

bool InteriorMap::render(void)
{
    /*
       gRandGen.setSeed(1978);

       //glEnable(GL_CULL_FACE);

       ColorF oneColor;

       if (mRenderMode == TexShaded || mRenderMode == TexWireframe ||
           mRenderMode == Lighting || mRenderMode == TexLighting)
       {
          glActiveTextureARB(GL_TEXTURE0_ARB);
          glEnable(GL_TEXTURE_2D);
          glBindTexture(GL_TEXTURE_2D, mWhite->getGLName());

          if(mRenderMode == Lighting || mRenderMode == TexLighting)
          {
             glActiveTextureARB(GL_TEXTURE1_ARB);
             glEnable(GL_TEXTURE_2D);
             glBindTexture(GL_TEXTURE_2D, mWhite->getGLName());

             if(mRenderMode == TexLighting)
                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
             else
                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

             glActiveTextureARB(GL_TEXTURE0_ARB);
             glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
          }
          else
          glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
       }

       S32 bound = -2;

       for (U32 i = 0; i < mInteriorRes->mBrushes.size(); i++)
       {
          if (mInteriorRes->mBrushes[i]->mType == InteriorMapResource::Portal || mInteriorRes->mBrushes[i]->mType == InteriorMapResource::Trigger)
             continue;

          if (mInteriorRes->mBrushes[i]->mStatus == ConvexBrush::Deleted)
             continue;

          if (mRenderMode == BrushColors)
          {
             //oneColor = ColorF(gRandGen.randF(0.0f, 1.0f), gRandGen.randF(0.0f, 1.0f), gRandGen.randF(0.0f, 1.0f), 1.0f);
             //glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, oneColor);
             glColor4f(gRandGen.randF(0.0f, 1.0f), gRandGen.randF(0.0f, 1.0f), gRandGen.randF(0.0f, 1.0f), 1.0f);
          }

          // Save ModelView Matrix so we can restore Canonical state at exit.
           glPushMatrix();

           // Transform by the objects' transform e.g move it.
           dglMultMatrix(&mInteriorRes->mBrushes[i]->mTransform);

          // Scale
          //glScalef(mInteriorRes->mBrushes[i]->mScale.x, mInteriorRes->mBrushes[i]->mScale.y, mInteriorRes->mBrushes[i]->mScale.z);

          for (U32 j = 0; j < mInteriorRes->mBrushes[i]->mFaces.mPolyList.size(); j++)
          {
             S32 tx = mInteriorRes->mBrushes[i]->mFaces.mPolyList[j].material;

             if (tx == -2)
                continue;

             if (mRenderMode == TexShaded || mRenderMode == TexWireframe ||
                 mRenderMode == TexLighting)
             {
                if (tx != bound)
                {
                   TextureObject* tex(mTexHandles[tx]);
                   if (tex)
                   {
                      glActiveTextureARB(GL_TEXTURE0_ARB);
                      glBindTexture(GL_TEXTURE_2D, mTexHandles[tx].getGLName());
                      bound = tx;
                   }
                   else
                   {
                      glActiveTextureARB(GL_TEXTURE0_ARB);
                      glBindTexture(GL_TEXTURE_2D, mWhite->getGLName());
                      bound = -1;
                   }
                }
             }

             if (mRenderMode == FaceColors)
             {
                //oneColor = ColorF(gRandGen.randF(0.0f, 1.0f), gRandGen.randF(0.0f, 1.0f), gRandGen.randF(0.0f, 1.0f), 1.0f);
                //glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, oneColor);
                glColor4f(gRandGen.randF(0.0f, 1.0f), gRandGen.randF(0.0f, 1.0f), gRandGen.randF(0.0f, 1.0f), 1.0f);
             }

             mInteriorRes->mBrushes[i]->renderFace(j, (mRenderMode == Lighting || mRenderMode == TexLighting));
          }

          glPopMatrix();
       }

       if (mRenderMode == TexShaded || mRenderMode == TexWireframe ||
           mRenderMode == Lighting || mRenderMode == TexLighting)
       {
          glActiveTextureARB(GL_TEXTURE1_ARB);
          glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
          glDisable(GL_TEXTURE_2D);

          glActiveTextureARB(GL_TEXTURE0_ARB);
          glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
          glDisable(GL_TEXTURE_2D);
       }

       //glDisable(GL_CULL_FACE);
    */

    return true;
}

U32 InteriorMap::packUpdate(NetConnection* con, U32 mask, BitStream* stream)
{
    // Pack Parent.
    U32 retMask = Parent::packUpdate(con, mask, stream);

    // Write fxPortal Mask Flag.
    if (stream->writeFlag(mask & MoveMask))
    {
        // Write Object Transform.
        stream->writeAffineTransform(mObjToWorld);

        // Write Object Scale.
        mathWrite(*stream, mObjScale);

        // Write the file name
        stream->writeString(mFileName);
    }

    if (stream->writeFlag(mask & RenderModeMask))
        stream->write(mRenderMode);

    // Were done ...
    return(retMask);
}

//------------------------------------------------------------------------------

void InteriorMap::unpackUpdate(NetConnection* con, BitStream* stream)
{
    // Unpack Parent.
    Parent::unpackUpdate(con, stream);

    // Read fxPortal Mask Flag.
    if (stream->readFlag())
    {
        MatrixF		ObjectMatrix;
        Point3F     scale;

        // Read Object Transform.
        stream->readAffineTransform(&ObjectMatrix);
        // Set Transform.
        setTransform(ObjectMatrix);

        // Read Object Scale
        mathRead(*stream, &scale);
        // Set Scale
        setScale(scale);

        // Reset the World Box.
        resetWorldBox();
        // Set the Render Transform.
        setRenderTransform(mObjToWorld);

        // Read the file name
        mFileName = stream->readSTString();
    }

    // New RenderMode?
    if (stream->readFlag())
        stream->read(&mRenderMode);
}

void InteriorMap::buildConvex(const Box3F& box, Convex* convex)
{
    Box3F realBox = box;
    mWorldToObj.mul(realBox);
    realBox.min.convolveInverse(mObjScale);
    realBox.max.convolveInverse(mObjScale);

    mConvexList->collectGarbage();

    for (U32 i = 0; i < mInteriorRes->mBrushes.size(); i++)
    {
        if (mInteriorRes->mBrushes[i]->mType == InteriorMapResource::Portal || mInteriorRes->mBrushes[i]->mType == InteriorMapResource::Trigger)
            continue;

        if (mInteriorRes->mBrushes[i]->mStatus == ConvexBrush::Deleted)
            continue;

        ConvexBrush* brush = mInteriorRes->mBrushes[i];

        if (realBox.isOverlapped(brush->mBounds))
        {
            // See if this brush exists in the working set already...
            Convex* cc = 0;
            CollisionWorkingList& wl = convex->getWorkingList();

            for (CollisionWorkingList* itr = wl.wLink.mNext; itr != &wl; itr = itr->wLink.mNext)
            {
                if (itr->mConvex->getType() == InteriorMapConvexType &&
                    (static_cast<InteriorMapConvex*>(itr->mConvex)->getObject() == this &&
                        static_cast<InteriorMapConvex*>(itr->mConvex)->brushIndex == i))
                {
                    cc = itr->mConvex;
                    break;
                }
            }

            if (!cc)
            {
                // Got ourselves a new convex
                InteriorMapConvex* cp = new InteriorMapConvex;
                mConvexList->registerObject(cp);
                convex->addToWorkingList(cp);
                cp->mObject = this;
                cp->pOwner = this;
                cp->brushIndex = i;
                cp->box = brush->mBounds;
            }
        }
    }
}

bool InteriorMap::castRay(const Point3F& s, const Point3F& e, RayInfo* info)
{
    // This assumes that the collision hulls are convex...otherwise it can return unpredictable results
    F32 outputFraction = 1.0f;
    VectorF outputNormal;

    // Loop through the collision hulls checking for the nearest ray collision
    // MDFFIX: Again I really should have the collision hulls sorted into a bsp
    for (U32 i = 0; i < mInteriorRes->mBrushes.size(); i++)
    {
        if (mInteriorRes->mBrushes[i]->mStatus == ConvexBrush::Deleted)
            continue;

        RayInfo test;

        if (mInteriorRes->mBrushes[i]->castRay(s, e, &test))
        {
            if (test.t < outputFraction)
            {
                outputFraction = test.t;
                outputNormal = test.normal;
            }
        }
    }

    // If we had a collision fill in the info structure and return
    if (outputFraction >= 0.0f && outputFraction < 1.0f)
    {
        info->t = outputFraction;
        info->normal = outputNormal;
        info->point.interpolate(s, e, outputFraction);
        info->face = -1;
        info->object = this;

        return true;
    }

    // Otherwise we didn't collide
    return false;
}

void InteriorMap::removeBrush(S32 brushIndex)
{
    if (brushIndex < 0 || brushIndex >= mInteriorRes->mBrushes.size())
        return;

    // Grab an iterator and point it at the beginning of the VectorPtr
    VectorPtr<ConvexBrush*>::iterator cbitr = mInteriorRes->mBrushes.begin();

    // Move the pointer up to our brush
    cbitr += brushIndex;

    // And remove it
    delete* cbitr;
    mInteriorRes->mBrushes.erase(cbitr);
}

S32 InteriorMap::getBrushIndex(U32 brushID)
{
    for (U32 i = 0; i < mInteriorRes->mBrushes.size(); i++)
    {
        if (mInteriorRes->mBrushes[i]->mID == brushID)
            return i;
    }

    return -1;
}

//****************************************************************************
// Map Texture Management
//****************************************************************************

// Returns the number of textures used in the map
S32 InteriorMap::getTextureCount()
{
    if (mInteriorRes)
        return mInteriorRes->mMaterials.size();

    return 0;
}

// Returns the name of the texture given the texture's index
const char* InteriorMap::getTextureName(S32 index)
{
    S32 count = getTextureCount();
    if (index >= 0 && index < count)
    {
        if (mInteriorRes)
            return ((const char*)mInteriorRes->getTextureName(index));
    }

    return "";
}

// Returns the texture for the map
const char* InteriorMap::getTexturePathway()
{
    return ((const char*)mTexPath);
}

// Returns the requested texture's dimensions
Point2I InteriorMap::getTextureSize(S32 index)
{
    S32 count = getTextureCount();
    if (index >= 0 && index < count)
    {
        if (mTexHandles[index])
            return Point2I(mTexHandles[index].getWidth(), mTexHandles[index].getHeight());
    }

    return Point2I(0, 0);
}
