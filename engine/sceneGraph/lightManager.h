//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _LIGHTMANAGER_H_
#define _LIGHTMANAGER_H_

#ifndef _MMATH_H_
#include "math/mMath.h"
#endif
#ifndef _COLOR_H_
#include "core/color.h"
#endif
#ifndef _TVECTOR_H_
#include "core/tVector.h"
#endif
#ifndef _DATACHUNKER_H_
#include "core/dataChunker.h"
#endif

#include "console/console.h"


// until we delete this file...
#include "lightingSystem/sgLightManager.h"


/*
class LightInfo
{
   friend class LightManager;

   public:
      enum Type {
         Point    = 0,
         Spot     = 1,
         Vector   = 2,
         Ambient  = 3
      };
      Type        mType;

      Point3F     mPos;
      VectorF     mDirection; ///< For spot and vector lights, indicates the direction of the light.
      ColorF      mColor;
      ColorF      mAmbient;
      F32         mRadius;    ///< For point and spot lights, indicates the effective radius of the light.

   private:
      S32         mScore;     ///< Used internally to track importance of light.
};
typedef Vector<LightInfo*> LightInfoList;


/*
//------------------------------------------------------------------------------
class SimObject;
class SceneObject;

class LightManager
{
   public:

      enum {
         DefaultMaxLights     = 8   ///< Indicate how many lights we should display by default.
      };

      LightManager();

      void setMaxGLLights(U32);  ///< Set the maximum number of lights to use when rendering.
      void registerLights(bool lightingScene);  ///< Register lights with the lighting manager.
                                                ///
                                                ///  @param lightingScene  Set to true if we're calculating
                                                ///                        lightmaps.
      const LightInfo& getLight(U32 light) { return *mLights[light]; }
      void addLight(LightInfo *);
      void removeLight(LightInfo *);

      U32 installGLLights(const SphereF &);  /// Set up lighting for a given area.
      U32 installGLLights(const Box3F &);    /// Set up lighting for a given area.
      void uninstallGLLights();

      S32  getNumLights() const { return mLights.size(); }
      void getLights(LightInfoList& lightList) { lightList = mLights; }
      void getLights(LightInfo** lightArray)
      {
         for (S32 i = 0; i < mLights.size(); i++)
            lightArray[i] = mLights[i];
      }
      const LightInfoList & getLights() { return mLights; }

      /// @name Best Light Management
      ///
      /// It is often the case that we have more lights in the map than we have slots on our
      /// graphics card; therefore, we have these functions to determine the best lights to use
      /// in a given context.
      /// @{
      void getBestLights(const PlaneF *, const U32, const Point3F &, LightInfoList &, const U32 max = 0xffffffff);
      void getBestLights(const SphereF &, LightInfoList&, const U32 max = 0xffffffff);
      void getBestLights(const Box3F &, LightInfoList&, const U32 max = 0xffffffff);
      /// @}

      void setVectorLightsEnabled(bool enable)     {mVectorLightsEnabled = enable;}
      bool getVectorLightsEnabled()                {return(mVectorLightsEnabled);}

      void setVectorLightsAttenuation(F32 factor)  {mVectorLightsAttenuation = factor;}
      F32 getVectorLightsAttenuation()             {return(mVectorLightsAttenuation);}

      void setAmbientColor(const ColorF & col)     {mAmbientLightColor = col;}
      const ColorF & getAmbientColor()             {return(mAmbientLightColor);}

      // XA: Moved to public for access via ConsoleMethod
      void resetGL();

private:
      U32 installGLLights();
      void installGLLight(LightInfo *);

      void fillLightList(LightInfoList&, U32);
      void scoreLight(LightInfo *, const SphereF &) const;
      void scoreLight(LightInfo *, const PlaneF *, const U32, const Point3F &) const;
      static S32 QSORT_CALLBACK compareLights(const void *, const void *);

      LightInfoList           mLights;
      U32                     mMaxGLLights;
      U32                     mGLLightCount;
      bool                    mGLLightsInstalled;
      bool                    mGLInitialized;
      bool                    mVectorLightsEnabled;
      F32                     mVectorLightsAttenuation;
      ColorF                  mAmbientLightColor;
};
*/
#endif
