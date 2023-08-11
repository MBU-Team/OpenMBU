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
typedef enum domUpAxisType;

//-----------------------------------------------------------------------------
class ColladaShapeLoader : public TSShapeLoader
{
   friend TSShape* loadColladaShape(const Torque::Path &path);

   domCOLLADA*             root;
   static Point3F          unitScale;
   static domUpAxisType    upAxis;
   Vector<AnimChannels*>   animations;       ///< Holds all animation channels for deletion after loading

   void processAnimation(const domAnimation* anim, F32& maxEndTime);

   void cleanup();

public:
   ColladaShapeLoader(domCOLLADA* _root);
   ~ColladaShapeLoader();

   void enumerateScene();
   static const Point3F& getUnitScale() { return unitScale; }
   static domUpAxisType getUpAxis() { return upAxis; }
};

#endif // _COLLADA_SHAPELOADER_H_
