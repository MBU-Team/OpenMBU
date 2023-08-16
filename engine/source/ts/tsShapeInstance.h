//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _TSSHAPEINSTANCE_H_
#define _TSSHAPEINSTANCE_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _TSSHAPE_H_
#include "ts/tsShape.h"
#endif
#ifndef _TSINTEGERSET_H_
#include "ts/tsIntegerSet.h"
#endif
#ifndef _CONSOLE_H_
#include "console/console.h"
#endif
#ifndef _GBITMAP_H_
#include "gfx/gBitmap.h"
#endif
#ifndef _NETSTRINGTABLE_H_
#include "sim/netStringTable.h"
#endif

class RenderItem;
class TSThread;
class ConvexFeature;

//-------------------------------------------------------------------------------------
// Instance versions of shape objects
//-------------------------------------------------------------------------------------
struct TSVertex
{
    Point3F p;      ///< Position
    ColorI  color;  ///< Color
    Point2F t1;     ///< Texture coordinate 1
    Point2F t2;     ///< Texture coordinate 2
};

/// An instance of a 3space shape.
///
/// @section TSShapeInstance_intro Introduction
///
/// A 3space model represents a significant amount of data. There are multiple meshes,
/// skeleton information, as well as animation data. Some of this, like the skeletal
/// transforms, are unique for each instance of the model (as different instances are
/// likely to be in different states of animation), while most of it, like texturing
/// information and vertex data, is the same amongst all instances of the shape.
///
/// To keep this data from being replicated for every instance of a 3shape object, Torque
/// uses the ResManager to instantiate and track TSShape objects. TSShape handles reading
/// and writing 3space models, as well as keeping track of static model data, as discussed
/// above. TSShapeInstance keeps track of all instance specific data, such as the currently
/// playing sequences or the active node transforms.
///
/// TSShapeInstance contains all the functionality for 3space models, while TSShape acts as
/// a repository for common data.
///
/// @section TSShapeInstance_functionality What Does TSShapeInstance Do?
///
/// TSShapeInstance handles several areas of functionality:
///      - Collision.
///      - Rendering.
///      - Animation.
///      - Updating skeletal transforms.
///      - Ballooning (see setShapeBalloon() and getShapeBalloon())
///
/// For an excellent example of how to render a TSShape in game, see TSStatic. For examples
/// of how to procedurally animate models, look at Player::updateLookAnimation().
class TSShapeInstance
{
public:

    struct ObjectInstance;
    friend class TSThread;
    friend class TSLastDetail;
    friend class TSPartInstance;

    static void init();
    static void destroy();

    /// Base class for all renderable objects, including mesh objects and decal objects.
    ///
    /// An ObjectInstance points to the renderable items in the shape...
    struct ObjectInstance
    {
        /// this needs to be set before using an objectInstance...tells us where to
        /// look for the transforms...gets set be shape instance 'setStatics' method
        static MatrixF* smTransforms;

        S32 nodeIndex;
        /// Gets the transform of this object
        MatrixF* getTransform();
        /// @name Render Functions
        /// @{

        /// Render!  This draws the base-textured object.
        virtual void render(S32 objectDetail, TSMaterialList*);
        /// Renders the environment map
        virtual void renderEnvironmentMap(S32 objectDetail, TSMaterialList*);
        /// Renders the detail map
        virtual void renderDetailMap(S32 objectDetail, TSMaterialList*);
        /// Renders the fog texture
        virtual void renderFog(S32 objectDetail, TSMaterialList*);
        /// @}

         /// @name Collision Routines
        /// @{

        virtual bool buildPolyList(S32 objectDetail, AbstractPolyList*, U32& surfaceKey);
        virtual bool getFeatures(S32 objectDetail, const MatrixF& mat, const Point3F& n, ConvexFeature*, U32& surfaceKey);
        virtual void support(S32 od, const Point3F& v, F32* currMaxDP, Point3F* currSupport);
        /// Ray cast for collision detection
        virtual bool castRay(S32 objectDetail, const Point3F& start, const Point3F& end, RayInfo*);

        virtual bool buildPolyListOpcode(S32 objectDetail, AbstractPolyList* polyList, U32& surfaceKey, TSMaterialList* materials);
        virtual bool castRayOpcode(S32 objectDetail, const Point3F& start, const Point3F& end, RayInfo* info, TSMaterialList* materials);
        /// @}
    };

    /// These are set up by default based on shape data
    struct MeshObjectInstance : ObjectInstance
    {
        TSMesh* const* meshList; ///< one mesh per detail level... Null entries allowed.
        const TSObject* object;
        S32 frame;
        S32 matFrame;
        F32 visible;

        // skin meshes store their vertices in this buffer instead of the one from TSMesh object.
        // this is so that multiple skin meshes can be inserted into the render instance manager
        // for later rendering.  the problem this solves is that if the skin mesh vb was used,
        // the animations of that skin for all instances (i.e. players) would be linked together
        // when rendering.
        GFXVertexBufferHandle<MeshVertex> mVB;

        S32 getSizeVB(S32 size);
        bool hasMergeIndices();
        /// @name Vertex Buffer functions
        /// Fills in the mesh's vertex buffer
        /// @{
        void fillVB(S32 vb, TSMaterialList* materials);
        void morphVB(S32 vb, S32& previousMerge, S32 objectDetail, TSMaterialList* materials);
        void renderVB(S32 vb, S32 objectDetail, TSMaterialList* materials);
        /// @}

        /// @name Render Methods
        /// This just selects the right detail level (mesh) and calls mesh's render
        /// @{
        void render(S32 objectDetail, TSMaterialList*);
        void renderEnvironmentMap(S32 objectDetail, TSMaterialList*);
        void renderDetailMap(S32 objectDetail, TSMaterialList*);
        void renderShadow(S32 objectDetail, const MatrixF& mat, S32 dim, U32* bits, TSMaterialList*);
        void renderFog(S32 objectDetail, TSMaterialList*);
        /// @}

        /// Gets the mesh with specified detail level
        TSMesh* getMesh(S32 num) const { return num < object->numMeshes ? *(meshList + num) : NULL; }

        /// @name Collision Routines
        /// @{

        bool buildPolyList(S32 objectDetail, AbstractPolyList*, U32& surfaceKey);
        bool getFeatures(S32 objectDetail, const MatrixF& mat, const Point3F& n, ConvexFeature*, U32& surfaceKey);
        void support(S32 od, const Point3F& v, F32* currMaxDP, Point3F* currSupport);
        bool castRay(S32 objectDetail, const Point3F& start, const Point3F& end, RayInfo*);
        
        bool buildPolyListOpcode(S32 objectDetail, AbstractPolyList* polyList, const Box3F& box, TSMaterialList* materials, U32& surfaceKey);
        bool castRayOpcode(S32 objectDetail, const Point3F& start, const Point3F& end, RayInfo* info, TSMaterialList* materials);
        /// @}
    };

    /// Also set up based on shape data...they refer to mesh object instances
    struct DecalObjectInstance : ObjectInstance
    {
        TSDecalMesh* const* decalList;
        const MeshObjectInstance* targetObject;
        const TSShape::Decal* decalObject;

        S32 frame;

        void render(S32 objectDetail, TSMaterialList*);
        TSDecalMesh* getDecalMesh(S32 num) const { return num < decalObject->numMeshes ? *(decalList + num) : NULL; }

        // we don't do these things...
        // void renderEnvironmentMap(S32,TSMaterialList*) {}
        // void renderDecalMap(S32,TSMaterialList*) {}
        // bool getFeatures(S32 objectDetail, const MatrixF& mat, const Point3F& n, ConvexFeature*, U32 & surfaceKey);
        bool buildPolyList(S32, AbstractPolyList*, U32&) { return false; }
        bool castRay(S32, const Point3F&, const Point3F&, RayInfo*) { return false; }
    };

    /// IFL objects ... controlled by animation but also can be controlled by user
    struct IflMaterialInstance
    {
        const TSShape::IflMaterial* iflMaterial;
        S32 frame;
    };

    //-------------------------------------------------------------------------------------
    // Lists used for storage of transforms, nodes, objects, etc...
    //-------------------------------------------------------------------------------------

public:

    bool shadowDirty;

    Vector<MeshObjectInstance> mMeshObjects;
    Vector<DecalObjectInstance> mDecalObjects;
    Vector<IflMaterialInstance> mIflMaterialInstances;

    /// storage space for node transforms
    Vector<MatrixF> mNodeTransforms;

    /// @name Reference Transform Vectors
    /// unused until first transition
    /// @{
    Vector<Quat16>         mNodeReferenceRotations;
    Vector<Point3F>        mNodeReferenceTranslations;
    Vector<F32>            mNodeReferenceUniformScales;
    Vector<Point3F>        mNodeReferenceScaleFactors;
    Vector<Quat16>         mNodeReferenceArbitraryScaleRots;
    /// @}

    /// @name Workspace for Node Transforms
    /// @{
    static Vector<QuatF>   smNodeCurrentRotations;
    static Vector<Point3F> smNodeCurrentTranslations;
    static Vector<F32>     smNodeCurrentUniformScales;
    static Vector<Point3F> smNodeCurrentAlignedScales;
    static Vector<TSScale> smNodeCurrentArbitraryScales;
    /// @}

    /// @name Threads
    /// keep track of who controls what on currently animating shape
    /// @{
    static Vector<TSThread*> smRotationThreads;
    static Vector<TSThread*> smTranslationThreads;
    static Vector<TSThread*> smScaleThreads;
    /// @}

 //-------------------------------------------------------------------------------------
 // Misc.
 //-------------------------------------------------------------------------------------

protected:

    void initMatInstances();


    /// @name Ground Transform Data
    /// @{
    MatrixF mGroundTransform;
    TSThread* mGroundThread;
    /// @}

    bool mScaleCurrentlyAnimated;

    S32 mCurrentDetailLevel;

    /// 0-1, how far along from current to next (higher) detail level...
    ///
    /// 0=at this dl, 1=at higher detail level, where higher means bigger size on screen
    /// for dl=0, we use twice detail level 0's size as the size of the "next" dl
    F32 mCurrentIntraDetailLevel;

    Resource<TSShape> hShape;
    TSShape* mShape;

    TSMaterialList* mMaterialList;    ///< by default, points to hShape material list
    bool            mOwnMaterialList; ///< Does this own the material list pointer?

 //   TextureHandle  mEnvironmentMap;
    bool           mEnvironmentMapOn;
    F32            mEnvironmentMapAlpha;
    bool           mAllowTwoPassEnvironmentMap;
    bool           mAlphaIsReflectanceMap;
    bool           mAllowTwoPassDetailMap;
    S32            mMaxEnvironmentMapDL;
    S32            mMaxDetailMapDL;
    bool           mAlphaAlways;
    F32            mAlphaAlwaysValue;
    bool           mDrawFog;

    bool mBalloonShape;   ///< Is this shape ballooned?
    F32  mBalloonValue;   ///< How much is it ballooned?

    bool          mUseOverrideTexture;
    //   TextureHandle mOverrideTexture;

    U32 debrisRefCount;

    // the threads...
    Vector<TSThread*> mThreadList;
    Vector<TSThread*> mTransitionThreads;

    /// @name Transition nodes
    /// keep track of nodes that are involved in a transition
    ///
    /// @note this only tracks nodes we're transitioning from...
    /// nodes we're transitioning to are implicitly handled
    /// (i.e., we don't need to keep track of them)
    /// @{

    TSIntegerSet mTransitionRotationNodes;
    TSIntegerSet mTransitionTranslationNodes;
    TSIntegerSet mTransitionScaleNodes;
    /// @}

    /// keep track of nodes with animation restrictions put on them
    TSIntegerSet mMaskRotationNodes;
    TSIntegerSet mMaskPosXNodes;
    TSIntegerSet mMaskPosYNodes;
    TSIntegerSet mMaskPosZNodes;
    TSIntegerSet mDisableBlendNodes;
    TSIntegerSet mHandsOffNodes;        ///< Nodes that aren't animated through threads automatically
    TSIntegerSet mCallbackNodes;

    /// state variables
    U32 mTriggerStates;

    bool initGround();
    void addPath(TSThread* gt, F32 start, F32 end, MatrixF* mat = NULL);

    /// @name Render functions
    /// @{

    void setupTexturing(S32 dl, F32 intraDL);
    bool twoPassEnvironmentMap();
    bool twoPassDetailMap();
    bool twoPassFog();
    void renderEnvironmentMap();
    void renderDetailMap();
    void renderFog();
    /// @}

    /// @name DirectX render functions
    /// @{

    bool renderMeshesX(S32 ss, S32 od);
    bool renderDecalsX(S32 ss, S32 od);
    ///@}

public:

    TSShape* getShape() { return mShape; }

    /// @name Statics
    /// set non-rendering static variables used for accessing shape data
    /// @{
    void setStatics(S32 dl = 0, F32 interDL = 0.0f, const Point3F* shapeScale = NULL);
    void clearStatics();
    /// @}

    TSMaterialList* getMaterialList() { return mMaterialList; }
    void setMaterialList(TSMaterialList*); ///< we won't own the material list unless we clone it (see below)
    void cloneMaterialList(); ///< call this to own the material list -- i.e., we'll make a copy of the currently
                              ///< set material list and be responsible for deleting it...if we don't own the list,
                              ///< then the application better not delete the set list out from under us (all this
                              ///< is handled automatically when using the shape's original list).
    bool ownMaterialList() const { return mOwnMaterialList; }

    void reSkin(StringHandle& newBaseHandle);

    enum
    {
        MaskNodeRotation = 0x01,
        MaskNodePosX = 0x02,
        MaskNodePosY = 0x04,
        MaskNodePosZ = 0x08,
        MaskNodeBlend = 0x10,
        MaskNodeAll = MaskNodeRotation | MaskNodePosX | MaskNodePosY | MaskNodePosZ | MaskNodeBlend,
        MaskNodeAllButBlend = MaskNodeRotation | MaskNodePosX | MaskNodePosY | MaskNodePosZ,
        MaskNodeAllButRotation = MaskNodePosX | MaskNodePosY | MaskNodePosZ | MaskNodeBlend,
        MaskNodeAllButPosX = MaskNodeRotation | MaskNodePosY | MaskNodePosZ | MaskNodeBlend,
        MaskNodeAllButPosY = MaskNodeRotation | MaskNodePosX | MaskNodePosZ | MaskNodeBlend,
        MaskNodeAllButPosZ = MaskNodeRotation | MaskNodePosX | MaskNodePosY | MaskNodeBlend,
        MaskNodeHandsOff = 0x20, ///< meaning, don't even set to default, programmer controls it (blend still applies)
        MaskNodeCallback = 0x40  ///< meaning, get local transform via callback function (see setCallback)
                                       ///< callback data2 is node index, callback return value is pointer to local transform
                                       ///< Note: won't get this callback everytime you animate...application responsibility
                                       ///< to make sure matrix pointer continues to point to valid and updated local transform
    };
    /// @name Node Masking
    /// set node masking...
    /// @{
    void setNodeAnimationState(S32 nodeIndex, U32 animationState);
    U32  getNodeAnimationState(S32 nodeIndex);
    /// @}

    /// @name Trigger states
    /// check trigger value
    /// @{
    bool getTriggerState(U32 stateNum, bool clearState = true);
    void setTriggerState(U32 stateNum, bool on);
    void setTriggerStateBit(U32 stateBit, bool on);
    /// @}

    /// @name Callback Functions
    /// @{
    typedef void (*CallbackFunction)(TSShapeInstance*, MatrixF*, S32 node, U32 data);
    CallbackFunction mCallback;
    U32 mCallbackData;
    void setCallback(CallbackFunction cb, U32 data1) { mCallback = cb; mCallbackData = data1; }
    /// @}

    /// @name Debris Management
    /// @{
    void incDebrisRefCount();
    void decDebrisRefCount();
    U32  getDebrisRefCount();
    /// @}

    /// @name AlphaAlways
    /// AlphaAlways allows the entire model to become translucent at the same value
    /// @{
    void setAlphaAlways(F32 value) { mAlphaAlways = (value < 0.99f); mAlphaAlwaysValue = value; }
    F32  getAlphaAlwaysValue() { return mAlphaAlways ? mAlphaAlwaysValue : 1.0f; }
    bool getAlphaAlways() { return mAlphaAlways; }
    /// @}

    /// @name Ballooning
    /// Blow up the model
    /// @{
    void setShapeBalloon(F32 value) { mBalloonShape = value > 1; mBalloonValue = value; }
    F32  getBalloonValue() const { return mBalloonShape ? mBalloonValue : 1.0f; }
    /// @}

    /// @name Override Texture
    /// @{
 //   void setOverrideTexture(TextureHandle override) { mOverrideTexture = override; mUseOverrideTexture = true;  }
 //   void clearOverrideTexture()                     { mOverrideTexture = NULL;     mUseOverrideTexture = false; }
    /// @}

    enum
    {
        NO_ENVIRONMENT_MAP,        ///< don't render environment map
        ENVIRONMENT_MAP_MULTI_1,   ///< render with multi-texturing (+1 texture units), shape alpha = reflectance map
        ENVIRONMENT_MAP_MULTI_3,   ///< render with multi-texturing (+3 texture units), reflectance map separate texture
        ENVIRONMENT_MAP_TWO_PASS,  ///< render in two passes -- mAllowTwoPassEnvironmentMap must be true
                                   ///
                                   ///  @note if reflectance map is separate from shape texture then won't render unless card has 4 texture units
                                   ///         However, translucency won't work quite right if reflection map not separated -- probably ok though.
                                   ///         Bottom line:  previous 2 items probably only used for special shapes...
                                   NO_DETAIL_MAP,
                                   DETAIL_MAP_MULTI_1,
                                   DETAIL_MAP_MULTI_2,
                                   DETAIL_MAP_TWO_PASS,
                                   NO_FOG,
                                   FOG_MULTI_1,
                                   FOG_MULTI_1_TEXGEN,
                                   FOG_TWO_PASS,
                                   FOG_TWO_PASS_TEXGEN
    };
    //   const TextureHandle & getEnvironmentMap() { return mEnvironmentMap; }
    F32 getEnvironmentMapAlpha() { return mEnvironmentMapAlpha; }
    //   void setEnvironmentMap(const TextureHandle& map) { mEnvironmentMap = map; }
    void setEnvironmentMapOn(bool on, F32 alpha = 0.25f) { mEnvironmentMapOn = on; mEnvironmentMapAlpha = alpha; }

    //-------------------------------------------------------------------------------------
    // private methods for setting up and affecting animation
    //-------------------------------------------------------------------------------------

private:

    /// @name Private animation methods
    /// These are private methods for setting up and affecting animation
    /// @{

    void updateTransitions();
    void handleDefaultScale(S32 a, S32 b, TSIntegerSet& scaleBeenSet);
    void handleTransitionNodes(S32 a, S32 b);
    void handleNodeScale(S32 a, S32 b);
    void handleAnimatedScale(TSThread*, S32 a, S32 b, TSIntegerSet&);
    void handleMaskedPositionNode(TSThread*, S32 nodeIndex, S32 offset);
    void handleBlendSequence(TSThread*, S32 a, S32 b);
    void checkScaleCurrentlyAnimated();
    /// @}

 //-------------------------------------------------------------------------------------
 // animate, render, & detail control
 //-------------------------------------------------------------------------------------

public:

    struct RenderData
    {
        MatrixF* currentTransform;
        S32 detailLevel;
        F32 intraDetailLevel;
        S32 environmentMapMethod;
        S32 detailMapMethod;
        S32 detailMapTE;
        S32 environmentMapTE;
        F32 environmentMapAlpha;
        U32 environmentMapGLName;
        S32 baseTE;
        F32 detailTextureScale;
        F32 detailMapAlpha;
        bool fadeSet;
        bool lightingOn;
        bool alwaysAlpha;
        F32 alwaysAlphaValue;
        bool balloonShape;
        F32 balloonValue;
        U32 materialFlags;
        S32 materialIndex;
        const Point3F* objectScale;
        bool fogOn;
        S32 fogMethod;
        S32 fogTE;
        Point4F fogColor;
        Point4F fogTexGenS;
        Point4F fogTexGenT;
        //      TextureHandle * fogMapHandle; // used by texgen fog
        bool useOverride;
        //      TextureHandle override;
        bool textureMatrixPushed;
        bool fogTexture;
        GBitmap* fogBitmap;
        //      TextureHandle *fogHandle;
        bool renderDecals;
        struct VertexAlpha
        {
            /// @name Vertex Data
            /// Track various contributors to vertex alpha
            /// @{
            F32 vis;
            F32 emap;
            F32 fog;
            F32 always;
            /// @}

            /// current result...
            F32 current;

            void init() { current = vis = emap = fog = always = 1.0f; }
            bool set() { F32 old = current; current = vis * emap * fog * always; return (mFabs(old - current) > 0.001f); }
        } vertexAlpha;
        MeshObjectInstance* currentObjectInstance;
    };
    static RenderData smRenderData;

    /// if true, skip these objects
    static bool smNoRenderTranslucent;
    static bool smNoRenderNonTranslucent;

    /// when taking hiQuality snapshot, scale intermediate bitmaps up to this amount
    static S32 smMaxSnapshotScale;

    /// scale pixel size by this amount when selecting detail levels
    static F32 smDetailAdjust;
    /// a different error metrix used by newer shapes (screen error from hi detail)
    static F32 smScreenError;
    static bool smFogExemptionOn;

    /// never choose detail level number below this value (except if
    /// only way to get a visible detail)
    static S32 smNumSkipRenderDetails;
    static bool smSkipFirstFog;
    static bool smSkipFog;

    virtual void render(const Point3F* objectScale = NULL);
    virtual void render(S32 dl, F32 intraDL = 0.0f, const Point3F* objectScale = NULL);
    void renderShadow(S32 dl, const MatrixF& mat, S32 dim, U32* bits);
    void setupFog(F32 fogAmount, const ColorF& fogColor);
    //   void setupFog(F32 fogAmount, TextureHandle * fogMap, Point4F & s, Point4F & t);

    GBitmap* snapshot(U32 width, U32 height, bool mipmap, MatrixF& cameraMatrix, bool hiQuality);
    GBitmap* snapshot(U32 width, U32 height, bool mip, MatrixF& cameraPos, S32 dl, F32 intraDL, bool hiQuality);
    static GBitmap* snapshot(TSShape*, U32 width, U32 height, bool mipmap, MatrixF& cameraMatrix, S32 dl, F32 intraDL = 0.0f, bool hiQuality = false);

    GBitmap* snapshot_softblend(U32 width, U32 height, bool mip, MatrixF& cameraMatrix, bool hiQuality);

    void animate();
    void animate(S32 dl);
    void animateNodes(S32 ss);
    void animateVisibility(S32 ss);
    void animateFrame(S32 ss);
    void animateMatFrame(S32 ss);
    void animateDecals(S32 ss);
    void animateIfls();
    void animateSubtrees(bool forceFull = true);
    void animateNodeSubtrees(bool forceFull = true);

    bool hasTranslucency();
    bool hasSolid();

    /// @name Animation Scale
    /// Query about animated scale
    /// @{
    bool animatesScale() { return (mShape->mFlags & TSShape::AnyScale) != 0; }
    bool animatesUniformScale() { return (mShape->mFlags & TSShape::UniformScale) != 0; }
    bool animatesAlignedScale() { return (mShape->mFlags & TSShape::AlignedScale) != 0; }
    bool animatesArbitraryScale() { return (mShape->mFlags & TSShape::ArbitraryScale) != 0; }
    bool scaleCurrentlyAnimated() { return mScaleCurrentlyAnimated; }
    /// @}

    //
    bool inTransition() { return !mTransitionThreads.empty(); }

    /// @name Alpha Reflectance Mapping
    /// open up mAlphaIsReflectanceMap for custom rendering
    /// @{
    bool queryAlphaIsReflectanceMap() { return mAlphaIsReflectanceMap; }
    void setAlphaIsReflectanceMap(bool val) { mAlphaIsReflectanceMap = val; }
    /// @}

    /// @name Ground Transforms
    /// The animator of a model can make the bounding box
    /// animate along with the object.  Doing so will move the object with the bounding box.
    /// The ground transform turns the world bounding box into the post-animation bounding box
    /// when such a technique is used.  However, few models actually use this technique.
    /// @{

    void animateGround(); ///< clears previous ground transform
    MatrixF& getGroundTransform() { return mGroundTransform; }
    void deltaGround(TSThread*, F32 start, F32 end, MatrixF* mat = NULL);
    void deltaGround1(TSThread*, F32 start, F32 end, MatrixF& mat);
    /// @}

    U32 getNumDetails();
    S32 getCurrentDetail();
    F32 getCurrentIntraDetail();
    void setCurrentDetail(S32 dl, F32 intraDL = 1.0f);
    S32 selectCurrentDetail(bool ignoreScale = false);
    S32 selectCurrentDetail(Point3F offset, F32 invScale = 1.0f);
    S32 selectCurrentDetail(F32 pixelSize);
    S32 selectCurrentDetail2(F32 adjustedDist);
    /// fancy detail selection -- uses screen error
    S32 selectCurrentDetailEx(bool ignoreScale = false);
    S32 selectCurrentDetail2Ex(F32 adjustedDist);
    S32 selectCurrentDetailEx(F32 errorTOL);

    enum
    {
        TransformDirty = BIT(0),
        VisDirty = BIT(1),
        FrameDirty = BIT(2),
        MatFrameDirty = BIT(3),
        DecalDirty = BIT(4),
        IflDirty = BIT(5),
        ThreadDirty = BIT(6),
        AllDirtyMask = TransformDirty | VisDirty | FrameDirty | MatFrameDirty | DecalDirty | IflDirty | ThreadDirty
    };
    U32* mDirtyFlags;
    void setDirty(U32 dirty);
    void clearDirty(U32 dirty);

    //-------------------------------------------------------------------------------------
    // collision interface routines
    //-------------------------------------------------------------------------------------

public:

    bool buildPolyList(AbstractPolyList*, S32 dl);
    bool getFeatures(const MatrixF& mat, const Point3F& n, ConvexFeature*, S32 dl);
    bool castRay(const Point3F& start, const Point3F& end, RayInfo*, S32 dl);
    bool quickLOS(const Point3F& start, const Point3F& end, S32 dl) { return castRay(start, end, NULL, dl); }
    Point3F support(const Point3F& v, S32 dl);
    void computeBounds(S32 dl, Box3F& bounds); ///< uses current transforms to compute bounding box around a detail level
                                                ///< see like named method on shape if you want to use default transforms

    bool buildPolyListOpcode(S32 dl, AbstractPolyList* polyList, const Box3F& box);
    bool castRayOpcode(S32 objectDetail, const Point3F& start, const Point3F& end, RayInfo*);

 //-------------------------------------------------------------------------------------
 // Thread Control
 //-------------------------------------------------------------------------------------

    /// @name Thread Control
    /// Threads!  In order to animate an object, first you need to have an animation in the object.
    /// Then, you need to get the TSShape of the object:
    /// @code
    /// TSShape* shape = mShapeInstance->getShape());
    /// @endcode
    /// Next, get the sequence and store::
    /// @code
    /// S32 seq = shape->findSequence("foo"));
    /// @endcode
    /// Create a new thread (if needed):
    /// @code
    /// TSThread* thread = mShapeInstance->addThread();
    /// @endcode
    /// Finally, set the position in the sequence:
    /// @code
    /// mShapeInstance->setSequence(thread, seq, 0)
    /// @endcode
    /// @{

public:

    TSThread* addThread();                        ///< Create a new thread
    TSThread* getThread(S32 threadNumber);        ///< @note  threads can change order, best to hold
                                                   ///<        onto a thread from the start
    void destroyThread(TSThread* thread);         ///< Destroy a thread!
    U32 threadCount();                             ///< How many threads are there?

    void setSequence(TSThread*, S32 seq, F32 pos);///< Get the thread a sequence
    /// Transition to a sequence
    void transitionToSequence(TSThread*, S32 seq, F32 pos, F32 duration, bool continuePlay);
    void clearTransition(TSThread*);              ///< Stop transitions
    U32  getSequence(TSThread*);                  ///< Get the sequence of the thread

    void setBlendEnabled(TSThread*, bool blendOn);///< Set whether or not the thread will blend
    bool getBlendEnabled(TSThread*);              ///< Does this thread blend?

    F32 getTime(TSThread* thread);                ///< Get how long the thread has been playing
    F32 getPos(TSThread* thread);                 ///< Get the position in the thread

    void setTime(TSThread* thread, F32 time);     ///< Set how long into the thread to use
    void setPos(TSThread* thread, F32 pos);       ///< Set the position of the thread

    bool isInTransition(TSThread* thread);        ///< Is this thread in transition?
    F32 getTimeScale(TSThread* thread);           ///< Get the time scale of the thread
    void setTimeScale(TSThread* thread, F32);     ///< Set the time scale of the thread

    F32 getDuration(TSThread* thread);            ///< Get the duration of the thread
    F32 getScaledDuration(TSThread* thread);      ///< Get the duration of the thread with the scale factored in

    S32 getKeyframeCount(TSThread* thread);       ///< Get the number of keyframes
    S32 getKeyframeNumber(TSThread* thread);      ///< Get which keyframe the thread is on
    /// Set which keyframe the thread is on
    void setKeyframeNumber(TSThread* thread, S32 kf);

    void advanceTime(F32 delta, TSThread*); ///< advance time on a particular thread
    void advanceTime(F32 delta);             ///< advance time on all threads
    void advancePos(F32 delta, TSThread*);  ///< advance pos  on a particular thread
    void advancePos(F32 delta);              ///< advance pos  on all threads
    /// @}

 //-------------------------------------------------------------------------------------
 // constructors, destructors, initialization, io
 //-------------------------------------------------------------------------------------

    TSShapeInstance(const Resource<TSShape>& shape, bool loadMaterials = true);
    TSShapeInstance(TSShape* pShape, bool loadMaterials = true);
    ~TSShapeInstance();

    void buildInstanceData(TSShape*, bool loadMaterials);

    void dump(Stream&);
    void dumpNode(Stream&, S32 level, S32 nodeIndex, Vector<S32>& detailSizes);
    void dumpDecals(Stream&, S32 indent, MeshObjectInstance*);

    void* mData; ///< available for use by app...initialized to 0

    void prepCollision();
};

inline MatrixF* TSShapeInstance::ObjectInstance::getTransform()
{
    return nodeIndex < 0 ? NULL : smTransforms + nodeIndex;
}

//-------------------------------------------------------------------------------------
// Thread class
//-------------------------------------------------------------------------------------

/// 3space animation thread.
///
/// An animation thread:  runtime data associated with a single sequence that is
/// running (or two sequences if in transition between them).
///
/// A shape instance can have multiple threads running. When multiple threads are running,
/// which thread/sequence controls which node or object is determined based
/// on priority of the sequence.
///
/// @note all thread data and methods are private (but TSShapeInstance is a friend).
///       Users should treat thread pointers like keys -- they are used to ID
///       the thread when interfacing with the shape, but are not manipulated
///       by anything but the TSShapeInstance.  See "Thread control" methods
///       for more info on controlling threads.
class TSThread
{
    friend class TSShapeInstance;

    S32 priority;

    TSShapeInstance* mShapeInstance;  ///< Instance of the shape that this thread animates

    const TSSequence* sequence;       ///< Sequence this thread will perform
    F32 pos;

    F32 timeScale;                     ///< How fast to play through the sequence

    S32 keyNum1;                       ///< Keyframe at or before current position
    S32 keyNum2;                       ///< Keyframe at or after current position
    F32 keyPos;

    bool blendDisabled;                ///< Blend with other sequences?

    /// if in transition...
    struct TransitionData
    {
        bool inTransition;

        F32 duration;
        F32 pos;
        F32 direction;
        F32 targetScale; ///< time scale for sequence we are transitioning to (during transition only)
                         ///< this is either 1 or 0 (if 1 target sequence plays as we transition, if 0 it doesn't)
        TSIntegerSet oldRotationNodes;    ///< nodes controlled by this thread pre-transition
        TSIntegerSet oldTranslationNodes; ///< nodes controlled by this thread pre-transition
        TSIntegerSet oldScaleNodes;       ///< nodes controlled by this thread pre-transition
        U32 oldSequence; ///< sequence that was set before transition began
        F32 oldPos;      ///< position of sequence before transition began
    } transitionData;

    struct
    {
        F32 start;
        F32 end;
        S32 loop;
    } path;
    bool makePath;

    /// given a position on the thread, choose correct keyframes
    /// slight difference between one-shot and cyclic sequences -- see comments below for details
    void selectKeyframes(F32 pos, const TSSequence* seq, S32* k1, S32* k2, F32* kpos);

    void getGround(F32 p, MatrixF* pMat);

    /// @name Triggers
    /// Triggers are used to do something once a certain animation point has been reached.
    ///
    /// For example, when the player's foot animation hits the ground, a foot puff and
    /// foot print are triggered from the thread.
    ///
    /// These are called by advancePos()
    /// @{
    void animateTriggers();
    void activateTriggers(F32 a, F32 b);
    /// @}

    // methods -- accessible through shape only
    void setSequence(S32 seq, F32 pos);
    void transitionToSequence(S32 seq, F32 pos, F32 duration, bool continuePlay);

    void advanceTime(F32 delta);
    void advancePos(F32 delta);

    F32 getTime();
    F32 getPos();

    void setTime(F32);
    void setPos(F32);

    bool isInTransition();
    F32 getTimeScale();
    void setTimeScale(F32);

    F32 getDuration();
    F32 getScaledDuration();

    S32 getKeyframeCount();
    S32 getKeyframeNumber();
    void setKeyframeNumber(S32 kf);

    TSThread(TSShapeInstance*);
    TSThread() {}
public:

    S32 operator<(const TSThread&) const;
};

typedef TSShapeInstance::ObjectInstance TSObjectInstance;

#endif
