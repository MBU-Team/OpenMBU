//-----------------------------------------------------------------------------
// Torque Shader Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _inc_createLightMaps
#define _inc_createLightMaps

#ifndef _ENTITYTYPES_H_
#include "map2dif plus/entityTypes.h"
#endif
#ifndef _EDITGEOMETRY_H_
#include "map2dif plus/editGeometry.h"
#endif
#ifndef _CSGBRUSH_H_
#include "map2dif plus/csgBrush.h"
#endif
#ifndef _DATACHUNKER_H_
#include "core/dataChunker.h"
#endif
#ifndef _INTERIOR_H_
#include "interior/interior.h"
#endif

static const U32 LexelPointStoreSize = 100;
static const U32 LightingMaxWindingPoints = 36;

//------------------------------------------------------------------------------

class Lighting
{
   public:

      // 132 bytes from 396   
      struct MiniWinding
      {
         union {
            U32            mNumIndices;
            MiniWinding *  mNext;
         };
         U32      mIndices[LightingMaxWindingPoints];
      };

      class EmitterInfo
      {
         public:
            U32 mEmitter;
            U32 mShadowVolume;
            UniqueVector mShadowed;

            // info for animated lights.
            GBitmap *      mLightMap;
      };
      Chunker<EmitterInfo> mEmitterInfoChunker;
      EmitterInfo * createEmitterInfo();

      //
      class SVNode
      {
         public:
            enum Side
            {
               Front = 0,
               Back = 1,
               On = 2,
               Split = 3
            };
            enum Type
            {
               PolyPlane = 0,
               ShadowPlane = 1,
               LexelPlane = 2,
            };
      
            Type mType;
            SVNode * mFront;
            SVNode * mBack;
   
            U32            mPlaneIndex;

            MiniWinding *  mWinding;
   
            // polyPlane member info
            SVNode *       mTarget;
            EmitterInfo *  mEmitterInfo;
               
            Point3D getCenter();
            void split(SVNode ** front, SVNode ** back, U32 planeIndex);
            void insertShadowVolume(SVNode ** root);
            void insertFront(SVNode ** root);
            void insertBack(SVNode ** root);
            void insert(SVNode ** root);
            void addToList(SVNode ** list);
            void move(SVNode ** list);
            void clipToVolume(SVNode ** store, SVNode * volume);
            void clipToInverseVolume(SVNode ** store, SVNode * volume);

            // help functions
            const Point3D & getPoint(U32);
            U32 insertPoint(const Point3D &);

            // use only with lexel plane nodes
            F64 getWindingSurfaceArea();
            bool clipWindingToPlaneFront(U32 planeEQIndex);
            SVNode::Side whichSide(SVNode * node);
      };

      // have only lexel/poly nodes with windings (keeps size of 
      // shadow volume down by 128 bytes each... with possibly well
      // over 100k of them, saves alot of mem)
      MiniWinding * createWinding();
      void recycleWinding(MiniWinding *);

      Chunker<MiniWinding>    mWindingChunker;
      MiniWinding *           mWindingStore;

      //
      Chunker<SVNode> mNodeChunker;
      SVNode * mNodeRepository;
      
      SVNode * createNode(SVNode::Type type);
      void recycleNode(SVNode * node);
      
      class Surface
      {
         public:
            U32                     mPlaneIndex;
            MiniWinding             mWinding;
            U32                     mSurfaceIndex;   
            SVNode *                mPolyNode;

            U32                     mNumEmitters;
            EmitterInfo **          mEmitters;	

            EmitterInfo * getEmitterInfo(U32 emitter);
      };
      Surface * createSurface();
      Chunker<Surface> mSurfaceChunker;
      
      class Emitter
      {
         public:
            Point3D        mPos;
            ColorF         mColor;
            U32            mPoint;
            U32            mIndex;

            U32            mNumSurfaces;
            U32 *          mSurfaces;

            bool           mAnimated;
            bool           mBumpSpec;

            Emitter();
            void processBSP();
            
            virtual bool isSurfaceLit(const Surface * surface) = 0;
            virtual F64 calcIntensity(SVNode * lSurface) = 0;
      };
   
      class PointEmitter : public Emitter
      {
         public:
            F64 mFalloffs[2];

            bool isSurfaceLit(const Surface * surface);
            F64 calcIntensity(SVNode * lSurface);
      };

      class RadEmitter : public PointEmitter
      {
         public:
            Point3D  mDirection;
            F32      mIntensity;

            F64 calcIntensity( const Point3D & pnt, const Point3D &surfNormal );
      };
      
      class SpotEmitter : public Emitter
      {
         public:
            F64      mFalloffs[2];
            F64      mAngles[2];
            Point3D  mDirection;
            
            bool isSurfaceLit(const Surface * surface);
            F64 calcIntensity(SVNode * lSurface);
      };
      
      class Light
      {
         public:
            
            Light();
            ~Light();

            class LightState {
               public:
                  U32         mNumEmitters;
                  U32         mEmitterIndex;
                  F32         mDuration;

                  void fillStateData(EditGeometry::LightState * state);
            };

            U32            mNumStates;
            U32            mStateIndex;

            char *         mName;
            bool           mAnimated;
            U32            mAnimType;
            bool           alarm;

            bool getTargetPosition(const char * name, Point3D & pos);

            // each of the worldcraft light types will be processed here
            bool buildLight(BaseLightEntity * entity);

            // scripted lights
            bool buildOmniLight(BaseLightEntity * entity);
            bool buildSpotLight(BaseLightEntity * entity);

            bool buildBrushLight(BaseLightEntity * entity);

            // animated...
            bool buildStrobeLight(BaseLightEntity * entity);
            bool buildPulseLight(BaseLightEntity * entity);
            bool buildPulse2Light(BaseLightEntity * entity);
            bool buildFlickerLight(BaseLightEntity * entity);
            bool buildRunwayLight(BaseLightEntity * entity);

            bool build(BaseLightEntity * entity);
      };
      
      Vector<Light::LightState>           mLightStates;
      Vector<U32>                         mLightStateEmitterStore;

      //
      Lighting();
      ~Lighting();
      
      void grabLights(bool alarmMode);

      void convertToFan(MiniWinding &, U32);
      void copyWinding(MiniWinding &, Winding &);
      U32 constructPlane(const Point3D&, const Point3D&, const Point3D&) const;

      void grabSurfaces();
      void processSurfaces();
      void createShadowVolumes();
      void processEmitterBSPs();
      void lightSurfaces();
      void processAnimatedLights();

      //
      Vector<Light *>                     mLights;
      Vector<BaseLightEmitterEntity *>    mBaseLightEmitters;
      Vector<TargetEntity *>              mTargets;
      Vector<Emitter *>                   mEmitters;
      Vector<Surface *>                   mSurfaces;
      Vector<SVNode *>                    mShadowVolumes;
      EmitterInfo **                      mSurfaceEmitterInfos;
      U32 *                               mEmitterSurfaceIndices;
      ColorF                              mAmbientColor;

      SVNode * getShadowVolume(U32 index);
      Surface * getSurface(U32 index);
      F64 getLumelScale(Surface &surf);
      
      //
      Vector<Point3D> mLexelPoints;
      U32 mNumAmbiguousPlanes;

      const Point3D & getLexelPoint(U32 index);
      U32 insertLexelPoint(const Point3D & pnt);
      void flushLexelPoints();
};

#endif
