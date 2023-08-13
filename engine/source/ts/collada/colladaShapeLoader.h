//-----------------------------------------------------------------------------
// Collada-2-DTS
// Copyright (C) 2008 GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _COLLADA_SHAPELOADER_H_
#define _COLLADA_SHAPELOADER_H_

#ifndef _TSSHAPELOADER_H_
#include "ts/loader/tsShapeLoader.h"
#endif
#include "colladaUtils.h"

class domCOLLADA;
class domAnimation;

//-----------------------------------------------------------------------------
class ColladaShapeLoader : public TSShapeLoader
{
   friend TSShape* loadColladaShape(const Torque::Path &path);

   domCOLLADA*             root;
   Vector<AnimChannels*>   animations;       ///< Holds all animation channels for deletion after loading

   void processAnimation(const domAnimation* anim, F32& maxEndTime);

   void cleanup();

public:
   ColladaShapeLoader(domCOLLADA* _root);
   ~ColladaShapeLoader();

   void enumerateScene();
   bool ignore(const String& name);
   void computeShapeOffset();

   static bool canLoadCachedDTS(const Torque::Path& path);
   static bool checkAndMountSketchup(const Torque::Path& path, String& mountPoint, Torque::Path& daePath);
   static domCOLLADA* getDomCOLLADA(const Torque::Path& path);
};

#endif // _COLLADA_SHAPELOADER_H_
