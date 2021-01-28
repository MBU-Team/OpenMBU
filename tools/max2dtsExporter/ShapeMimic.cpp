//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "max2dtsExporter/ShapeMimic.h"
#include "max2dtsExporter/SceneEnum.h"
#include "max2dtsExporter/maxUtil.h"
#include "max2dtsExporter/Sequence.h"
#include "max2dtsExporter/stripper.h"
#include "max2dtsExporter/translucentSort.h"
#include "max2dtsExporter/skinHelper.h"

#pragma pack(push,8)
#include <istdplug.h>
#include <iparamm2.h>
#include <modstack.h>
#pragma pack(pop)

// The ShapeMimic tries to hold court in both the max world and
// the Torque three-space world.  It holds a shape tree isomorphic
// to what the shape will look like when exported, but maintains
// links to max objects and delays computing certain things
// until the tsshape is finally created in generateShape().

#define printDump SceneEnumProc::printDump

#define QUICK_STRIP
//#define NV_STRIP

// simple struct to bind scale factors together (TSScale uses a QuatF)
struct ScaleStruct
{
   Quat16 mRotate;
   Point3F mScale;
};

Vector<Quat16*> nodeRotCache;
Vector<Point3F*> nodeTransCache;
Vector<ScaleStruct*> nodeScaleCache;

Vector<INode*> cutNodes;
Vector<INode*> cutNodesParents;

bool cullOffTile = true;

// briefly mentioned in MaxSDK docs, not in any include files...
#define PB_CLIPU 0
#define PB_CLIPV 1
#define PB_CLIPW 2
#define PB_CLIPH 3
#define PB_APPLY 6
#define PB_CROP_PLACE 7

#define HACK_PLANE_CLASS_ID Class_ID(136257020,2002153317)

F32 sameVertTOL = 0.00005f;
F32 sameTVertTOL = 0.00005f;

F32 mSq(F32 a) { return a*a; }

//--------------------------------------------
// Decal data structure and enum
struct DecalInfo
{
   INode * decalNode;
   Bitmap * filter;
   S32 tsMat;

   U32 decalType;
   Point3F pos;
   Point3F x;
   Point3F y;
   Point3F z;

   F32 maxAngle;
   F32 minCos;

   Vector<Point3> verts;
   Vector<Point3> tverts;
   Vector<TSDrawPrimitive> faces;
   Vector<U16> indices;

   Vector<Point3F> n;
   Vector<F32> k;

} gDecalInfo;

enum
{
   SphereDecal   = 0,
   CylinderDecal = 1,
   BoxDecal      = 2,
   PlaneDecal    = 3
};

struct MipmapMethod
{
   S32 noMipmap;
   S32 noMipmapTranslucent;
   S32 zapBorder;
} gMipmapMethod;

S32 gPolyCount;
F32 gMinVolPerPoly;
F32 gMaxVolPerPoly;
Vector<S32> t2AutoDetailSizes;
Vector<F32> t2AutoDetailPercents;

S32 findClosestMatch(Point3F & p, Vector<Point3F> & points)
{
   F32 closestValue = 10E30f;
   S32 closestIndex = -1;
   for (S32 i=0; i<points.size(); i++)
   {
      Point3F delta = p-points[i];
      if (mDot(delta,delta)<=closestValue)
      {
         closestIndex=i;
         closestValue = mDot(delta,delta);
      }
   }
   return closestIndex;
}

//--------------------------------------------
// Constructor/destructor
ShapeMimic::ShapeMimic()
{
    errorStr = NULL;
}

ShapeMimic::~ShapeMimic()
{
   S32 i;

   dFree(errorStr);

   for (i=0;i<objectList.size();i++)
      delete objectList[i];

   for (i=0;i<decalObjectList.size();i++)
      delete decalObjectList[i];

   for (i=0;i<skins.size();i++)
      delete skins[i];

   for (i=0;i<multiResList.size();i++)
   {
      // restore multiRes meshes to original state
      Animatable * obj = (Animatable*)multiResList[i]->multiResNode->GetObjectRef();
      for (S32 j=0; j<obj->NumSubs(); j++)
      {
         if (!dStrcmp(obj->SubAnimName(j),"MultiRes"))
         {
            IParamBlock * paramBlock = (IParamBlock*)obj->SubAnim(j)->SubAnim(0);

            Interval range = multiResList[i]->multiResNode->GetTimeRange(TIMERANGE_ALL|TIMERANGE_CHILDNODES|TIMERANGE_CHILDANIMS);
            paramBlock->SetValue(0,DEFAULT_TIME,multiResList[i]->totalVerts);
            break;
         }
      }
      delete multiResList[i];
   }

   for (i=0;i<subtrees.size();i++)
   {
      S32 j;
      for (j=0;j<subtrees[i]->detailNames.size();j++)
         dFree((char*) subtrees[i]->detailNames[j]);

      NodeMimic * node = subtrees[i]->start.child;
      nodes.clear();
      while (node)
      {
         nodes.push_back(node);
         node = findNextNode(node);
      }

      for (j=0;j<nodes.size();j++)
         delete nodes[j];

      delete subtrees[i];
   }

   for (i=0;i<materials.size();i++)
      dFree(materials[i]);

   for (i=0;i<iflList.size();i++)
      delete iflList[i];

   for (i=0; i<nodeRotCache.size(); i++)
      delete [] nodeRotCache[i];
   nodeRotCache.clear();

   for (i=0; i<nodeTransCache.size(); i++)
      delete [] nodeTransCache[i];
   nodeTransCache.clear();

   for (i=0; i<nodeScaleCache.size(); i++)
      delete [] nodeScaleCache[i];
   nodeScaleCache.clear();
}

//--------------------------------------------
// error handling:
bool ShapeMimic::isError()
{
    return errorStr != NULL;
}

void ShapeMimic::setExportError(const char * str)
{
   errorStr = errorStr ? errorStr : dStrdup(str);
}

const char * ShapeMimic::getError()
{
    return errorStr;
}

//--------------------------------------------
// some utilities

S32 __cdecl compareTSDetails( void const *e1, void const *e2 )
{
   const TSShape::Detail * d1 = (const TSShape::Detail*)e1;
   const TSShape::Detail * d2 = (const TSShape::Detail*)e2;

   if (d1->size > d2->size)
      return -1;
   if (d2->size > d1->size)
      return 1;

   return 0;
}

void sortTSDetails(Vector<TSShape::Detail> & details)
{
    dQsort(details.address(),details.size(),sizeof(TSShape::Detail),compareTSDetails);
}

S32 __cdecl compareObjectMimics( void const *e1, void const *e2 )
{
   const ObjectMimic * om1 = *(const ObjectMimic**)e1;
   const ObjectMimic * om2 = *(const ObjectMimic**)e2;

   if (om1->subtreeNum < om2->subtreeNum)
      return -1;
   else if (om1->priority < om2->priority)
      return -1;
   else if (om2->subtreeNum < om1->subtreeNum)
      return 1;
   else if (om2->priority < om1->priority)
      return 1;
   else
      return 0;
}

void sortObjectList(Vector<ObjectMimic*> & olist)
{
    dQsort(olist.address(),olist.size(),sizeof(ObjectMimic*),compareObjectMimics);
}


S32 __cdecl compareDecalObjectMimics( void const *e1, void const *e2 )
{
   const DecalObjectMimic * dom1 = *(const DecalObjectMimic**)e1;
   const DecalObjectMimic * dom2 = *(const DecalObjectMimic**)e2;

   if (dom1->subtreeNum < dom2->subtreeNum)
      return -1;
   else if (dom1->priority < dom2->priority)
      return -1;
   else if (dom2->subtreeNum < dom1->subtreeNum)
      return 1;
   else if (dom2->priority < dom1->priority)
      return 1;
   else
      return 0;
}

void sortDecalObjectList(Vector<DecalObjectMimic*> & dlist)
{
   dQsort(dlist.address(),dlist.size(),sizeof(DecalObjectMimic*),compareDecalObjectMimics);
}

S32 __cdecl compareFaces( void const *e1, void const *e2 )
{
   const TSDrawPrimitive * face1 = (const TSDrawPrimitive*)e1;
   const TSDrawPrimitive * face2 = (const TSDrawPrimitive*)e2;

   if (face1->matIndex < face2->matIndex)
      return -1;
   else if (face2->matIndex < face1->matIndex)
      return 1;
   else return 0;
}

void sortFaceList(Vector<TSDrawPrimitive> & faces)
{
    dQsort(faces.address(),faces.size(),sizeof(TSDrawPrimitive),compareFaces);
}

//--------------------------------------------
// set up objects for sorting
void ShapeMimic::setObjectPriorities(Vector<ObjectMimic*> & objectList)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   S32 i,j;
   for (i=0; i<objectList.size(); i++)
   {
      ObjectMimic * om = objectList[i];

      // assign priority as follows:
      // 1. if it has translucency, set the highest 2 bits so it will occur after all non-translucent items
      //    but if it is also a sortObject, only set the highest bit so that it will occur before other
      //    translucent objects (in case we are writing depth values).
      // 2. if it has exactly 1 material set next bit and put material index into next 14 bits
      //    so all objects that use exactly that material occur consecutively
      // 3. put tsnodeIndex into final 16 bits so objects hanging off same node will be consecutive,
      //    all else (1&2) being equal
      om->priority = (U32) om->tsNodeIndex;

      // use highest detail mesh we can find...
      for (j=0;j<om->numDetails;j++)
         if (om->details[j].mesh)
            break;
      if (j==om->numDetails)
         // not sure what this would mean, but don't want to crash
         continue;

      // if bone object, set priority to 0
      if (om->isBone)
      {
         om->priority = 0;
         continue;
      }

      INode * mesh = om->details[j].mesh->pNode;
      if (!mesh)
         // not sure what this would mean, but don't want to crash
         continue;

      bool delTri;
      TriObject * tri = getTriObject(mesh,DEFAULT_TIME,-1,delTri);
      Mesh & maxMesh = tri->mesh;

      bool hasTranslucent = false;
      bool hasRepeat = false;
      bool isSortObject = om->details[j].mesh->sortedObject;
      S32 matIndex = -1;
      for (j=0; j<maxMesh.getNumFaces(); j++)
      {
         CropInfo cropInfo; // throw-away...
         S32 mi = addMaterial(mesh,maxMesh.faces[j].getMatID(),&cropInfo); // we'll end up adding again later...

         // if already encountered an error, then
         // we'll just go through the motions
         if (isError()) return;

         if (mi==matIndex)
            continue;
         if (matIndex!=-1)
            hasRepeat=true;
         matIndex = mi;
         if (!(matIndex & TSDrawPrimitive::NoMaterial) && (materialFlags[matIndex] & TSMaterialList::Translucent))
            hasTranslucent = true;
      }
      if (delTri)
         delete tri;

      if (hasTranslucent && !isSortObject)
         om->priority |= 3 << 30;
      else if (hasTranslucent && isSortObject)
         om->priority |= 2 << 30;
      if (!hasRepeat)
      {
         om->priority |= 1 << 29;
         om->priority |= (matIndex & 0x0FFF) << 16;
      }
   }
}

//--------------------------------------------
// set up decals for sorting
void ShapeMimic::setDecalObjectPriorities(Vector<DecalObjectMimic*> & decals)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   S32 i;
   for (i=0; i<decals.size(); i++)
   {
      decals[i]->priority = 0; // they'll all be translucent, so don't worry about it
      decals[i]->subtreeNum = decals[i]->targetObject->subtreeNum;
   }
}

//--------------------------------------------
// a fleet of utility functions used to
// generate animation
//--------------------------------------------
struct ExporterSequenceData
{
    S32   cyclic;
    S32   blend;
    S32   blendReferenceTime;
    S32   priority;
    S32   addPadFrame;
    S32   useFrameRate;
    F32   frameRate;
    S32   numFrames;
    S32   ignoreGround;
    S32   useGroundFrameRate;
    F32   groundFrameRate;
    S32   groundNumFrames;
    S32   enableMorph;
    S32   enableVis;
    S32   enableTransform;
    S32   enableUniformScale;
    S32   enableArbitraryScale;
    S32   enableTVert;
    S32   enableIFL;
    S32   enableDecal;
    S32   enableDecalFrame;
    S32   forceMorph;
    S32   forceVis;
    S32   forceTransform;
    S32   forceScale;
    S32   forceTVert;
    S32   forceDecal;
    S32   overrideDuration;
    F32   overriddenDuration;

};

void setupOldSequence(IParamBlock * pblock, Interval range, ExporterSequenceData * data)
{
   // if we got this far with an old sequence, then it's allowed
   // but we need to figure out what values to use
   S32 oneShot;
   S32 forceVis;
   S32 visOnly;

   pblock->GetValue(PB_OLD_SEQ_ONESHOT,  range.Start(), oneShot, FOREVER);
   pblock->GetValue(PB_OLD_SEQ_FORCEVIS, range.Start(), forceVis, FOREVER );
   pblock->GetValue(PB_OLD_SEQ_VISONLY,  range.Start(), visOnly, FOREVER );

   data->cyclic              = !oneShot;
   data->blend               = false;
   data->blendReferenceTime  = 0;
   data->priority            = getIntSequenceDefault(PB_SEQ_DEFAULT_PRIORITY);
   data->addPadFrame         = !getBoolSequenceDefault(PB_SEQ_LAST_FIRST_FRAME_SAME);
   data->useFrameRate        = getBoolSequenceDefault(PB_SEQ_USE_FRAME_RATE);
   data->frameRate           = getFloatSequenceDefault(PB_SEQ_FRAME_RATE);
   data->numFrames           = getIntSequenceDefault(PB_SEQ_NUM_FRAMES);
   data->ignoreGround        = getBoolSequenceDefault(PB_SEQ_IGNORE_GROUND_TRANSFORM);
   data->useGroundFrameRate  = getBoolSequenceDefault(PB_SEQ_USE_GROUND_FRAME_RATE);
   data->groundFrameRate     = getFloatSequenceDefault(PB_SEQ_GROUND_FRAME_RATE);
   data->groundNumFrames     = getIntSequenceDefault(PB_SEQ_NUM_GROUND_FRAMES);

   data->forceMorph = false;
   data->forceVis = forceVis;
   data->forceTransform = false;
   data->forceScale = false;
   data->forceTVert = false;
   data->forceDecal = false;
   data->enableMorph = true;
   data->enableVis = true;
   data->enableTransform = true;
   data->enableUniformScale = false;
   data->enableArbitraryScale = false;
   data->enableIFL = true;
   data->enableTVert = true;
   data->enableDecal = false;
   data->enableDecalFrame = false;

   if (visOnly)
   {
      data->enableMorph = false;
      data->enableTransform = false;
      data->enableIFL = false;
      data->enableTVert = false;
   }

   data->overrideDuration = false;
   data->overriddenDuration = 1.0f;
}

void setupSequence(IParamBlock * pblock, Interval range, ExporterSequenceData * data)
{
   S32 noPad;

   pblock->GetValue(PB_SEQ_CYCLIC,                 range.Start(), data->cyclic, FOREVER);
   pblock->GetValue(PB_SEQ_BLEND,                  range.Start(), data->blend, FOREVER);
   data->blendReferenceTime = 0;
   Control * control = pblock->GetController(PB_SEQ_BLEND_REFERENCE_TIME);
   IKeyControl * ikc = control ? GetKeyControlInterface(control) : NULL;
   if (ikc && ikc->GetNumKeys()>=1)
   {
      // find first keyframe...time will determine reference position of blend sequence
      ILinFloatKey key;
      char buffer[128]; // just in case some other key type gets in there make sure nothing gets trashed...
      ikc->GetKey(0,&key);
      data->blendReferenceTime = key.time;
   }

   pblock->GetValue(PB_SEQ_DEFAULT_PRIORITY,       range.Start(), data->priority, FOREVER);

   pblock->GetValue(PB_SEQ_LAST_FIRST_FRAME_SAME,  range.Start(), noPad, FOREVER);
   data->addPadFrame = !noPad;

   pblock->GetValue(PB_SEQ_USE_FRAME_RATE,         range.Start(), data->useFrameRate, FOREVER);
   pblock->GetValue(PB_SEQ_FRAME_RATE,             range.Start(), data->frameRate, FOREVER);
   pblock->GetValue(PB_SEQ_NUM_FRAMES,             range.Start(), data->numFrames, FOREVER);

   pblock->GetValue(PB_SEQ_IGNORE_GROUND_TRANSFORM,range.Start(), data->ignoreGround, FOREVER);
   pblock->GetValue(PB_SEQ_USE_GROUND_FRAME_RATE,  range.Start(), data->useGroundFrameRate, FOREVER);
   pblock->GetValue(PB_SEQ_GROUND_FRAME_RATE,      range.Start(), data->groundFrameRate, FOREVER);
   pblock->GetValue(PB_SEQ_NUM_GROUND_FRAMES,      range.Start(), data->groundNumFrames, FOREVER);

   pblock->GetValue(PB_SEQ_ENABLE_MORPH_ANIMATION, range.Start(), data->enableMorph, FOREVER);
   pblock->GetValue(PB_SEQ_ENABLE_VIS_ANIMATION,   range.Start(), data->enableVis, FOREVER);
   pblock->GetValue(PB_SEQ_ENABLE_TRANSFORM_ANIMATION, range.Start(), data->enableTransform, FOREVER);
   pblock->GetValue(PB_SEQ_ENABLE_UNIFORM_SCALE_ANIMATION, range.Start(), data->enableUniformScale, FOREVER);
   pblock->GetValue(PB_SEQ_ENABLE_ARBITRARY_SCALE_ANIMATION, range.Start(), data->enableArbitraryScale, FOREVER);
   pblock->GetValue(PB_SEQ_ENABLE_TEXTURE_ANIMATION, range.Start(), data->enableTVert, FOREVER);
   pblock->GetValue(PB_SEQ_ENABLE_IFL_ANIMATION,   range.Start(), data->enableIFL, FOREVER);
   pblock->GetValue(PB_SEQ_ENABLE_DECAL_ANIMATION, range.Start(), data->enableDecal, FOREVER);
   pblock->GetValue(PB_SEQ_ENABLE_DECAL_FRAME_ANIMATION, range.Start(), data->enableDecalFrame, FOREVER);

   pblock->GetValue(PB_SEQ_FORCE_MORPH_ANIMATION,  range.Start(), data->forceMorph, FOREVER);
   pblock->GetValue(PB_SEQ_FORCE_VIS_ANIMATION,    range.Start(), data->forceVis, FOREVER);
   pblock->GetValue(PB_SEQ_FORCE_TRANSFORM_ANIMATION, range.Start(), data->forceTransform, FOREVER);
   pblock->GetValue(PB_SEQ_FORCE_SCALE_ANIMATION, range.Start(), data->forceScale, FOREVER);
   pblock->GetValue(PB_SEQ_FORCE_TEXTURE_ANIMATION, range.Start(), data->forceTVert, FOREVER);
   pblock->GetValue(PB_SEQ_FORCE_DECAL_ANIMATION,   range.Start(), data->forceDecal, FOREVER);

   pblock->GetValue(PB_SEQ_OVERRIDE_DURATION, range.Start(), data->overrideDuration, FOREVER);
   pblock->GetValue(PB_SEQ_DURATION, range.Start(), data->overriddenDuration, FOREVER);
}

void getSequenceTiming(ExporterSequenceData & data,
                       F32 * start, F32 * end, F32 * duration,
                       F32 * delta, S32 * numFrames)
{
    // following 4 cases set up duration, delta, and numFrames
    // duration goes into sequence while delta and numFrames
    // are used to generate keyframes...
    // we generate keyframes at:
    // start + i * delta, where i: 0 to numFrames-1
    if (data.cyclic && data.useFrameRate)
    {
        if (data.addPadFrame)
            // go one frame beyond the end
            *end += 1.0f / maxFrameRate; // as in 3ds max
        *duration = *end - *start + 0.000001f; // add a little for rounding purposes
        *delta = 1.0f / data.frameRate;
        *numFrames = (S32) (*duration / *delta);
        *delta = *duration / (F32)*numFrames;
    }
    else if (data.cyclic && !data.useFrameRate)
    {
        // ignore frame rate, use numFrames instead
        // we ignore 'addPadFrame' variable, because
        // it would not be useful at all
        *numFrames = data.numFrames;
        *duration = *end - *start + 0.000001f; // add a little for rounding purposes
        *delta = *duration / (F32) *numFrames;
    }
    else if (!data.cyclic && data.useFrameRate)
    {
        *duration = *end - *start + 0.000001f; // add a little for rounding purposes
        *delta = 1.0f / data.frameRate;
        *numFrames = (S32) (*duration / *delta);
        *delta = *duration / (F32) *numFrames;
        // add one more frame at the end of one-shot (not cyclic) sequence
        (*numFrames)++;
    }
    else // (!data.cyclic && !data.useFrameRate)
    {
        *duration = *end - *start + 0.000001f; // add a little for rounding purposes
        *numFrames = data.numFrames;
        *delta = *duration / (F32)(data.numFrames-1);
    }
}

void getGroundSequenceTiming(ExporterSequenceData & data, F32 groundDuration,
                             F32 *groundDelta, S32 *groundNumFrames)
{
    if (data.ignoreGround)
        return;

    if (data.useGroundFrameRate)
    {
        *groundDelta = groundDuration / (F32) data.groundFrameRate;
        *groundNumFrames = (S32)(groundDuration / *groundDelta);
        *groundDelta = groundDuration / (F32) *groundNumFrames;
        *groundNumFrames++;
    }
    else
    {
        *groundNumFrames = data.groundNumFrames;
        *groundDelta = groundDuration / (F32)(*groundNumFrames-1);
    }
}

//--------------------------------------------
// set global parameters
void ShapeMimic::setGlobals(INode * pNode, S32 polyCount, F32 minVolPerPoly, F32 maxVolPerPoly)
{
   if (!pNode->GetUserPropBool("MIPMAP::NO_MIPMAP",gMipmapMethod.noMipmap))
      gMipmapMethod.noMipmap = false;
   if (!pNode->GetUserPropBool("MIPMAP::NO_MIPMAP_TRANSLUCENT",gMipmapMethod.noMipmapTranslucent))
      gMipmapMethod.noMipmapTranslucent = false;
   if (!pNode->GetUserPropBool("MIPMAP::BLACK_BORDER",gMipmapMethod.zapBorder))
      gMipmapMethod.zapBorder = true;
   gPolyCount = polyCount;
   gMinVolPerPoly = minVolPerPoly;
   gMaxVolPerPoly = maxVolPerPoly;
}

//--------------------------------------------
// set up bounds node
void ShapeMimic::addBounds(INode * pNode)
{
    // if already encountered an error, then
    // we'll just go through the motions
    if (isError()) return;

    boundsNode = pNode;
}

void ShapeMimic::applyAutoDetail(Vector<S32> & validDetails, Vector<const char *> & detailNames, Vector<INode*> & detailNodes)
{
   if (t2AutoDetail<51)
   {
      // this number determines number of polys per new detail level...anything less than 50
      // would probably be way excessive, but we'll just make sure user didn't type in "1" for "true"
      // a little bit
      setExportError("t2AutoDetail parameter must be 0 or greater than 50");
      return;
   }

   if (subtrees.size()>1)
   {
      setExportError("Too many subtrees:  t2Autodetail only works with single subtree objects");
      return;
   }

   S32 numDetails = (gPolyCount+t2AutoDetail*2)/t2AutoDetail;
   S32 lastVisibleDetail = -1;
   for (S32 i=0; i<validDetails.size(); i++)
      if (validDetails[i]>=0)
         lastVisibleDetail = i;
   S32 numVisibleDetails = lastVisibleDetail+1;

   while (numVisibleDetails<numDetails)
   {
      S32 maxDiff = 0;
      S32 idx = -1;
      for (S32 i=0; i<numVisibleDetails; i++)
      {
         S32 diff = i+1<numVisibleDetails ? validDetails[i]-validDetails[i+1] : validDetails[i];
         if (diff>maxDiff)
         {
            maxDiff = diff;
            idx = i;
         }
      }
      if (idx<0)
      {
         validDetails.push_back(128);
         detailNames.push_back(dStrdup("detail128"));
         detailNodes.push_back(boundsNode); // no detail nodes...just put something legal in here (eek)
      }
      else
      {
         validDetails.insert(idx+1);
         detailNames.insert(idx+1);
         detailNodes.insert(idx+1);
      if (idx+2<validDetails.size())
      {
         validDetails[idx+1] = (validDetails[idx] + validDetails[idx+2])/2;
         detailNames[idx+1] = dStrdup(avar("detail%i",validDetails[idx+1]));
         detailNodes[idx+1] = detailNodes[idx];
      }
      else
      {
         validDetails[idx+1] = (validDetails[idx])/2;
         detailNames[idx+1] = dStrdup(avar("detail%i",validDetails[idx+1]));
         detailNodes[idx+1] = detailNodes[idx];
      }
      }
      numVisibleDetails++;
   }

   t2AutoDetailSizes = validDetails;
   for (S32 ii=0; ii<t2AutoDetailSizes.size(); ii++)
   {
      if (t2AutoDetailSizes[ii]<0)
      {
         t2AutoDetailSizes.erase(ii);
         ii--;
      }
   }

   t2AutoDetailPercents.setSize(t2AutoDetailSizes.size());
   for (S32 j=0; j<t2AutoDetailPercents.size(); j++)
   {
      S32 count;
      if (j==0)
         count=gPolyCount;
      else if (j==t2AutoDetailPercents.size()-1)
         count = 16;
      else if (j==t2AutoDetailPercents.size()-2)
         count = 40;
      else if (j==t2AutoDetailPercents.size()-3)
         count = 80;
      else
      // when j=0, count=gPolyCount, when j==t2ADP.size()-3, count=80...
      count = 80 + (t2AutoDetailPercents.size()-3-j)*(gPolyCount-80)/(t2AutoDetailPercents.size()-3);

      t2AutoDetailPercents[j]=(F32)count/(F32)gPolyCount;
      if (j==0)
         // be sure...
         t2AutoDetailPercents[j]=1.0f;
   }
}

void ShapeMimic::fixupT2AutoDetail()
{
   for (S32 i=0; i<objectList.size(); i++)
   {
      ObjectMimic * om = objectList[i];
      if (om->numDetails==0 || om->details[0].size<0)
         continue;
      for (S32 j=0; j<t2AutoDetailSizes.size(); j++)
      {
         if (j>=om->numDetails || om->details[j].size!=t2AutoDetailSizes[j])
         {
            if (j==0)
            {
               break;
//               setExportError("Assertion failed");
//               return;
            }
            INode * node = om->details[j-1].mesh->pNode;
            S32 stopNumber;
            if (node->GetUserPropInt("MULTIRES::STOP",stopNumber))
            {
               if (j>=stopNumber)
                  // oops, we explicitly limited number of details...
                  break;
            }
            S32 pos;
            getObject(node,dStrdup(om->name),t2AutoDetailSizes[j],&pos);
         }
      }
   }
}

//--------------------------------------------
// add a subtree
void ShapeMimic::addSubtree(INode * pNode)
{
    // if already encountered an error, then
    // we'll just go through the motions
    if (isError()) return;

    subtrees.increment();
    Subtree * pSubtree = subtrees.last() = new Subtree;
    Vector<S32> & validDetails = pSubtree->validDetails;
    Vector<const char*> & detailNames = pSubtree->detailNames;
    Vector<INode*> & detailNodes = pSubtree->detailNodes;
    const char * pname = pNode->GetName();

    // we need to create a dummy node for branches to hang off of
    // it will correspond to pNode...but won't be exported
    pSubtree->start.maxNode = pNode;
    pSubtree->start.parent  = NULL;
    pSubtree->start.child   = NULL;
    pSubtree->start.sibling = NULL;

    // first go through the top level and parse
    // into detail markers and shape branches
    Vector<INode*> branches;
    S32 i;
    for (i=0; i<pNode->NumberOfChildren(); i++)
    {
        INode * child = pNode->GetChildNode(i);

        // we'll deal with these separately...
        if (SceneEnumProc::isBounds(child))
            continue;

        if (SceneEnumProc::isCamera(child))
            continue;

        if (child->NumberOfChildren()==0)
        {
            S32 size;
            char * dname = chopTrailingNumber(child->GetName(),size);
            if (dStrcmp(dname,child->GetName()))
            {
               dFree(dname);
               dname = dStrdup(child->GetName()); // use full name, with size
               validDetails.push_back(size);
               detailNames.push_back(dname);
               detailNodes.push_back(child);
               printDump(PDPass2,avar("Adding detail named \"%s\" of size %i to subtree \"%s\".\r\n", dname, size, pname));
            }
            else
            {
               printDump(PDPass2,avar("Ignoring node named \"%s\" off subtree \"%s\" because no trailing number.\r\n", dname,pname));
               dFree(dname);
            }
        }
        else
            branches.push_back(child);
    }

    if (validDetails.empty() && !allowEmptySubtrees)
    {
        setExportError(avar("Subtree \"%s\" has no detail levels -- this error can be turned off",pNode->GetName()));
        return;
    }
    else if (branches.empty() && !allowEmptySubtrees)
    {
        setExportError(avar("Subtree \"%s\" has nothing in it -- this error can be turned off",pNode->GetName()));
        return;
    }

    // the detail list
    // need to keep names and sizes in synch...we don't already
    // have a struct set up to do this the easy way, so do a
    // bubble sort
    S32 j;
    for (i=0; i<(S32)validDetails.size()-1; i++)
    {
        for (j=i+1; j<validDetails.size(); j++)
        {
            if (validDetails[j]>validDetails[i])
            {
                S32 tmpInt;
                const char * tmpCh;
                tmpInt = validDetails[i];
                tmpCh = detailNames[i];
                validDetails[i] = validDetails[j];
                detailNames[i] = detailNames[j];
                validDetails[j] = tmpInt;
                detailNames[j] = tmpCh;
            }
        }
    }

    // if we are doing t2AutoDetail, then this is where we need to re-configure our details
    if (t2AutoDetail>1)
      applyAutoDetail(validDetails,detailNames,detailNodes);

    if (validDetails.empty() || branches.empty())
    {
        // nothing here, but if we made it this far it isn't an error
        delete pSubtree;
        subtrees.decrement();
        return;
    }

    addNode(&pSubtree->start,pNode,validDetails,false);
    for (i=0; i<branches.size(); i++)
        addNode(pSubtree->start.child,branches[i],validDetails,true);

    // everything needs to be rooted to the bounds node...
    pSubtree->start.maxNode = boundsNode;
}

//--------------------------------------------
// add a node
void ShapeMimic::addNode(NodeMimic * mimicParent,
                         INode * maxChild,
                         Vector<S32> & validDetails,
                         bool recurseChildren)
{
    // if already encountered an error, then
    // we'll just go through the motions
    if (isError()) return;

    // if it's the bounds node or a camera, don't do anything
    if (SceneEnumProc::isBounds(maxChild) || SceneEnumProc::isCamera(maxChild) || SceneEnumProc::isDecal(maxChild))
        return;

    printDump(PDPass2,avar("Adding node \"%s\" with parent \"%s\" to subtree rooted on max-node \"%s\".\r\n",
        maxChild->GetName(), mimicParent->maxNode->GetName(),subtrees.last()->start.maxNode->GetName()));

    NodeMimic * mimicChild = new NodeMimic;
    mimicChild->maxNode = maxChild;
    mimicChild->parent  = mimicParent;
    mimicChild->child   = NULL;
    mimicChild->sibling = NULL;
    if (mimicParent->child)
    {
        // put us at the end of parent's child list
        NodeMimic * sib = mimicParent->child;
        while (sib->sibling)
            sib = sib->sibling;
        sib->sibling = mimicChild;
    }
    else
        // so far, an only child
        mimicParent->child = mimicChild;

    if (hasMesh(mimicChild->maxNode) && !SceneEnumProc::isDummy(mimicChild->maxNode))
    {
        printDump(PDPass2,"Attaching object to node.\r\n");
        mimicChild->objects.push_back(addObject(mimicChild->maxNode,&validDetails));
    }

    // now mimic the children of maxChild...
    if (recurseChildren)
    {
        S32 i;
        for (i=0; i<maxChild->NumberOfChildren(); i++)
            addNode(mimicChild,maxChild->GetChildNode(i),validDetails,true);
    }
}

//--------------------------------------------
// get object from object list -- create if not present
ObjectMimic * ShapeMimic::getObject(INode * pNode, char * name, S32 size, S32 * detailNum, F32 multiResPercent, bool matchFull, bool isBone, bool isSkin)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return NULL;

   S32 i;

   // if this is a billboard object, detect that now and adjust name accordingly
   // Note: billboard objects are named BB::* (the BB:: is dropped from here on).
   bool billboard = pNode ? SceneEnumProc::isBillboard(pNode) : false;
   bool sortedObject = pNode ? SceneEnumProc::isSortedObject(pNode) : false;

   // if the name ends in : then ignore the : (and when we search this object, we compare to full name instead of name)
   char * colon = NULL;
   if (dStrlen(name)>1 && name[dStrlen(name)-1]==':')
   {
      colon  = name+dStrlen(name)-1;
      *colon = '\0';
   }

   // if we're in the shape tree, find out here (alternative is that we are unlinked and a detail level of a mesh)
   bool inTree = pNode ? !pNode->GetParentNode()->IsRootNode() : false;

   // if we're in the tree, we may need the full name...
   const char * fullName = NULL;
   if (inTree)
   {
      fullName = pNode->GetName();
      SceneEnumProc::tweakName(&fullName);  // get's rid of "BB::" prefix, for example
   }

   // look for name in list of objects
   for (i=0; i<objectList.size(); i++)
   {
      if (isBone!=objectList[i]->isBone || isSkin!=objectList[i]->isSkin)
         continue;
      if (matchFull && inTree && objectList[i]->fullName && !dStricmp(fullName,objectList[i]->fullName))
         break;
      if ( colon && objectList[i]->fullName && !dStricmp(name,objectList[i]->fullName))
         break;
      if (!colon && !dStricmp(name,objectList[i]->name))
         break;
   }

   // add an entry if needed
   if (i==objectList.size())
   {
      objectList.increment();
      objectList.last() = new ObjectMimic;
      objectList.last()->name = name;
      // note:  not straight forwared ... full name is the full name of the object
      // on the tree.  If inTree, add that now...if "colon" is not null, use "name" for
      // full name.
      if (inTree)
         objectList.last()->fullName = dStrdup(fullName);
      else if (colon)
         objectList.last()->fullName = dStrdup(name);
      else
         objectList.last()->fullName = NULL;
      objectList.last()->numDetails = 0;
      objectList.last()->pValidDetails = NULL;
      objectList.last()->subtreeNum = -1;
      objectList.last()->maxParent = NULL;
      objectList.last()->maxTSParent = NULL;
      objectList.last()->tsObject = NULL;
      objectList.last()->tsNodeIndex = -1;
      objectList.last()->isBone = isBone;
      objectList.last()->isSkin = isSkin;
      printDump(PDPass2,avar("Adding object named \"%s\".\r\n",name));
   }
   else
      dFree(name); // don't need duplicate name

   ObjectMimic * om = objectList[i];
   if (om->isBone)
   {
      printDump(PDPass2,"Object is bone\r\n");
      om->maxParent=om->maxTSParent=pNode;
      return om;
   }

   // enter data
   S32 dl = om->numDetails++;
   if (om->numDetails>ObjectMimic::MaxDetails)
   {
      setExportError(avar("Assertion failed:  too many details for mesh %s.",name));
      return NULL;
   }

   printDump(PDPass2,avar("Adding mesh of size %i to object \"%s\".\r\n", size,om->name));

   // keep meshes sorted by size
   S32 j;
   for (j=dl; j>=0; j--)
   {
      if (j==0 || size<om->details[j-1].size)
      {
         if (j<dl && om->details[j].size==size)
         {
            setExportError(avar("Found two meshes named \"%s\" of size %i.\r\n",om->name,size));
            // avoid crash...
            *detailNum = j;
            om->details[j].size = size;
            om->details[j].mesh = NULL;
            return NULL;
         }

         // this is where we go:
         // larger than all that follow us
         // smaller than all that precede us
         om->details[j].size = size;
         om->details[j].mesh = new MeshMimic(pNode);
         om->details[j].mesh->billboard = billboard;
         om->details[j].mesh->sortedObject = sortedObject;
         om->details[j].mesh->multiResPercent = multiResPercent;
         *detailNum = j;
         break; // all in order
      }
      else
         // not here, move j-1 up to make room
         om->details[j] = om->details[j-1];
   }

   return om;
}

//--------------------------------------------
// get multi-res mimic
MultiResMimic * ShapeMimic::getMultiRes(INode * pNode)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return NULL;

   for (S32 i=0; i<multiResList.size(); i++)
   {
      if (multiResList[i]->pNode==pNode)
         return multiResList[i];
   }
   return NULL;
}

void grabMeshData(INode * multiResNode, Vector<Face> & faces, Vector<Point3F> & verts)
{
   bool delTri;
   TriObject * tri = getTriObject(multiResNode,DEFAULT_TIME,-1,delTri);
   Mesh & mesh = tri->mesh;

   faces.clear();
   verts.clear();

   S32 j;
   faces.setSize(mesh.getNumFaces());
   verts.setSize(mesh.getNumVerts());
   for (j=0; j<mesh.getNumFaces(); j++)
      faces[j] = mesh.faces[j];
   for (j=0; j<mesh.getNumVerts(); j++)
   {
      verts[j].x = mesh.getVert(j).x;
      verts[j].y = mesh.getVert(j).y;
      verts[j].z = mesh.getVert(j).z;
   }

   if (delTri)
      delete tri;
}

void findEdgeHard(Vector<Face> & hiMesh, Vector<Face> & loMesh, Vector<Point3F> & verts, S32 & fromA, S32 & toB)
{
   S32 i,j,numVert = fromA;
   toB = -1; // initialize to illegal value...if remains -1 on exit, then we shouldn't need it later

   // we'll have to do this the hard way...look for someone that has changed
   for (i=0; i<loMesh.size(); i++)
   {
      if (i>=hiMesh.size())
         continue;
      if (hiMesh[i].v[0]==fromA)
         toB = loMesh[i].v[0];
      else if (hiMesh[i].v[1]==fromA)
         toB = loMesh[i].v[1];
      else if (hiMesh[i].v[2]==fromA)
         toB = loMesh[i].v[2];
      else
         continue;
      return;
   }

   // if we get here, then the only face/faces that contain vert "fromA" have been
   // removed from loMesh...search hiMesh for a face using "fromA" and choose the
   // closest vert on the face as the merge vert...
   for (i=0; i<hiMesh.size(); i++)
   {
      for (j=0;j<3;j++)
         if (hiMesh[i].v[j]==fromA)
            // bingo
            break;
      if (j==3)
         continue;
      Point3F delta1 = verts[(S32)hiMesh[i].v[j]] - verts[(S32)hiMesh[i].v[(j+1)%3]];
      Point3F delta2 = verts[(S32)hiMesh[i].v[j]] - verts[(S32)hiMesh[i].v[(j+2)%3]];
      if (mDot(delta1,delta1)<mDot(delta2,delta2))
         toB=hiMesh[i].v[(j+1)%3];
      else
         toB=hiMesh[i].v[(j+2)%3];
      return;
   }
}

bool findEdgeEasy(Vector<Face> & faces, S32 & fromA, S32 & toB)
{
   toB = -1; // initialize to illegal value...if remains -1 on exit, then we shouldn't need it later

   // now find toB...
   S32 vertA1 = -1, vertB1 = -1;
   S32 faceNum = faces.size()-1;

   if (faceNum<1)
      return false;

   if (faces[faceNum].v[0]==fromA)
   {
      vertA1 = faces[faceNum].v[1];
      vertB1 = faces[faceNum].v[2];
   }
   else if (faces[faceNum].v[1]==fromA)
   {
      vertA1 = faces[faceNum].v[0];
      vertB1 = faces[faceNum].v[2];
   }
   else if (faces[faceNum].v[2]==fromA)
   {
      vertA1 = faces[faceNum].v[0];
      vertB1 = faces[faceNum].v[1];
   }

   faceNum--;
   S32 vertA2 = -1, vertB2 = -1;
   if (faces[faceNum].v[0]==fromA)
   {
      vertA2 = faces[faceNum].v[1];
      vertB2 = faces[faceNum].v[2];
   }
   else if (faces[faceNum].v[1]==fromA)
   {
      vertA2 = faces[faceNum].v[0];
      vertB2 = faces[faceNum].v[2];
   }
   else if (faces[faceNum].v[2]==fromA)
   {
      vertA2 = faces[faceNum].v[0];
      vertB2 = faces[faceNum].v[1];
   }
   if (vertA1!=vertA2 && vertA1!=vertB2 && vertB1!=vertA2 && vertB1!=vertB2)
      // doh! no shared edge
      return false;
   else if ( (vertA1==vertA2 && vertB1==vertB2) || (vertA1==vertB2 && vertB1==vertA2) )
      // doh! same triangle, different facing
      return false;
   if (vertA1==vertA2 || vertA1==vertB2)
      toB = vertA1;
   else if (vertB1==vertA2 || vertB1==vertB2)
      toB = vertB1;
   if (toB>=0)
      return true;
   return false;
}

//--------------------------------------------
// get multi-res mimic
void ShapeMimic::addMultiRes(INode * pNode, INode * multiResNode)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   if (getMultiRes(pNode))
      return;

   S32 i;
   multiResList.increment();
   multiResList.last() = new MultiResMimic(pNode,multiResNode);
   multiResList.last()->totalVerts = 0;
   // look up total verts (we assume we start at max...allows artist to dial down to start with)
   Animatable * obj = (Animatable*)multiResList.last()->multiResNode->GetObjectRef();
   IParamBlock * paramBlock = NULL;
   Interval range = multiResList.last()->multiResNode->GetTimeRange(TIMERANGE_ALL|TIMERANGE_CHILDNODES|TIMERANGE_CHILDANIMS);
   for (i=0; i<obj->NumSubs(); i++)
   {
      if (!dStrcmp(obj->SubAnimName(i),"MultiRes"))
      {
         paramBlock = (IParamBlock*)obj->SubAnim(i)->SubAnim(0);

         paramBlock->GetValue(0,DEFAULT_TIME,multiResList.last()->totalVerts,range);
         break;
      }
   }

   // since first time we've since this multi res object, record off vertex merge information
   if (paramBlock)
      recordMergeOrder(paramBlock,multiResList.last());
}

void ShapeMimic::recordMergeOrder(IParamBlock * paramBlock, MultiResMimic * multiResMimic)
{
   struct MeshData
   {
      Vector<Face> faces;
      Vector<Point3F> verts;
   } currentMesh, loMesh;

   // we will assume that mesh starts with this many verts...
   S32 startNumVerts = multiResMimic->totalVerts;
   paramBlock->SetValue(0,DEFAULT_TIME,startNumVerts);
   grabMeshData(multiResMimic->multiResNode,currentMesh.faces,currentMesh.verts);

   S32 numVerts = startNumVerts;
   if (numVerts!=currentMesh.verts.size())
      setExportError(avar("Multires vertex mis-match on mesh \"%s\" -- hit generate on modifier",multiResMimic->multiResNode->GetName()));

   while (numVerts)
   {
      bool hardWay = false;
      S32 fromA, toB;
      fromA = numVerts-1;
      findEdgeEasy(currentMesh.faces,fromA,toB);
      if (toB<0)
      {
         paramBlock->SetValue(0,DEFAULT_TIME,numVerts-1);
         grabMeshData(multiResMimic->multiResNode,loMesh.faces,loMesh.verts);
         findEdgeHard(currentMesh.faces,loMesh.faces,currentMesh.verts,fromA,toB);
         hardWay = true;
      }

      if (toB<0)
         // merge to ourself...not sure that this can happen, but if it does, it'll be handled ok
         toB = fromA;

      multiResMimic->mergeFrom.push_back(currentMesh.verts[fromA]);
      multiResMimic->mergeTo.push_back(currentMesh.verts[toB]);

      // now collapse the edge from hiMesh
      collapseEdge(currentMesh.faces,fromA,toB);

      // The following check can't be done because we sometimes merge in a different
      // order than the plugin does.  This is a speed consideration.  We assume that

      // if the Nth vert is being removed and the top two faces are N i j & N i k
      // that N is merged with i.  This isn't always the case, but usually is.
      /*
      if (hardWay)
      {
         // let's take this opportunity to make sure that the
         // collapsed mesh is the same we would get using
         // the modifier (i.e., loMesh and hiMesh should have
         // same topology on the verts...we don't care about
         // tverts since our edge collapse ignores them for now).
         if (loMesh.faces.size()!=currentMesh.faces.size())
         {
            setExportError("Assertion failed during edge collapse.");
            return;
         }
         for (S32 j=0; j<loMesh.faces.size(); j++)
         {
            if (loMesh.faces[j].v[0]!=currentMesh.faces[j].v[0] ||
                loMesh.faces[j].v[1]!=currentMesh.faces[j].v[1] ||
                loMesh.faces[j].v[2]!=currentMesh.faces[j].v[2])
            {
               setExportError("Assertion failed during edge collapse.");
               return;
            }
         }
      }
      */

      numVerts--;
   }

   // restore to original state...
   paramBlock->SetValue(0,DEFAULT_TIME,startNumVerts);
}

void ShapeMimic::collapseEdge(Vector<Face> & faces, S32 fromA, S32 toB)
{
   for (S32 i=0; i<faces.size(); i++)
   {
      Face & face = faces[i];
      for (S32 v=0; v<3; v++)
         if (face.v[v]==fromA)
            face.v[v]=toB;
      if (face.v[0]==face.v[1] || face.v[1]==face.v[2] || face.v[2]==face.v[0])
      {
         faces.erase(i);
         i--;
      }
   }
}

void doT2AutoDetailPercents(INode * pNode, Vector<F32> & percents, F32 minPercent)
{
   Animatable * obj = (Animatable*)pNode->GetObjectRef();
   IParamBlock * paramBlock = NULL;
   Interval range = pNode->GetTimeRange(TIMERANGE_ALL|TIMERANGE_CHILDNODES|TIMERANGE_CHILDANIMS);
   S32 i;
   S32 totalVerts = 0;
   for (i=0; i<obj->NumSubs(); i++)
   {
      if (!dStrcmp(obj->SubAnimName(i),"MultiRes"))
      {
         paramBlock = (IParamBlock*)obj->SubAnim(i)->SubAnim(0);

         paramBlock->GetValue(0,DEFAULT_TIME,totalVerts,range);
         break;
      }
   }
   F32 totalVertsF = totalVerts;
   S32 numNeeded = percents.size();
   for (i=0; i<numNeeded; i++)
   {
      if (i==0)
         percents[i]=1.0f;
      else
      {
         F32 k  = ((F32)i)/(F32)(numNeeded-1);
/*
         F32 k2 = 1.0f - (1.0f-k)*(1.0f-k);
         // use k to interpolate between k and k2
         k = (1.0f-k) * k + k * k2;
*/
         percents[i] = ((1.0f-k)*totalVertsF + k*3.0f)/totalVertsF;
         if (percents[i]<minPercent)
            percents[i]=minPercent;
      }
   }
}

//--------------------------------------------
// get multi-res info from a node...
void ShapeMimic::getMultiResData(INode * pNode, Vector<S32> & multiResSize, Vector<F32> & multiResPercent)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   TSTR buffer;
   if (!pNode->GetUserPropString("MULTIRES::SIZES",buffer))
      return;

   if (t2AutoDetail>1)
   {
      multiResSize = t2AutoDetailSizes;
      multiResPercent.setSize(multiResSize.size());
      F32 minPercent;
      if (!pNode->GetUserPropFloat("MULTIRES::MIN",minPercent))
         minPercent=0.0f;
      doT2AutoDetailPercents(pNode,multiResPercent,minPercent);
      S32 stopNumber;
      if (pNode->GetUserPropInt("MULTIRES::STOP",stopNumber))
      {
         if (stopNumber<0)
            stopNumber=0;
         else if (stopNumber>multiResSize.size())
            stopNumber=multiResSize.size();
         multiResSize.setSize(stopNumber);
         multiResPercent.setSize(stopNumber);
      }
   }
   else
   {
      // extract sizes from buffer
      char * pos = buffer;
      while (*pos && (*pos=='=' || *pos==' '))
         pos++;
      while (pos && *pos)
      {
         while (*pos && *pos==' ')
            pos++;
         char * nextPos = dStrchr(pos,',');
         if (!nextPos)
            nextPos = pos + dStrlen(pos);
         char entry[128];
         S32 i;
         for (i=0; pos+i<nextPos; i++)
            entry[i]=pos[i];
         entry[i]='\0';
         multiResSize.push_back(dAtoi(entry));
         pos = *nextPos ? nextPos+1 : nextPos;
         while (*pos && *pos==' ')
            pos++;
      }

      if (pNode->GetUserPropString("MULTIRES::DETAILS",buffer))
      {
         char * pos = buffer;
         while (*pos && (*pos=='=' || *pos==' '))
            pos++;
         while (pos && *pos)
         {
            while (*pos && *pos==' ')
               pos++;
            char * nextPos = dStrchr(pos,',');
            if (!nextPos)
               nextPos = pos + dStrlen(pos);
            char entry[128];
            S32 i;
            for (i=0; pos+i<nextPos; i++)
               entry[i]=pos[i];
            entry[i]='\0';
            multiResPercent.push_back(dAtof(entry));
            pos = *nextPos ? nextPos+1 : nextPos;
            while (*pos && *pos==' ')
               pos++;
         }
     }
  }

  if (multiResSize.size() && !multiResPercent.size())
  {
      F32 p = 1.0f;
      F32 step = 1.0f/(F32)multiResSize.size();
      for (S32 i=0; i<multiResSize.size(); i++, p -= step)
         multiResPercent.push_back(p);
  }

   // make sure percent's are in the right order...sort if they aren't
   for (S32 i=0; i<multiResPercent.size(); i++)
   {
      for (S32 j=i+1; j<multiResPercent.size(); j++)
      {
         if (multiResPercent[i]<multiResPercent[j])
         {
            F32 tmp = multiResPercent[i];
            multiResPercent[i]=multiResPercent[j];
            multiResPercent[j]=tmp;
         }
      }
   }

   if (multiResSize.size() != multiResPercent.size())
   {
      setExportError(avar("Multi-res size list has more or less entries than details list on object \"%s\"",pNode->GetName()));
      return;
   }
}

//--------------------------------------------
// add a mesh -- detect MultiRes
// if multi res, then add one mesh per detail
ObjectMimic * ShapeMimic::addObject(INode * pNode, Vector<S32> * pValidDetails)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return NULL;

   ObjectMimic * om;

   // detect MultiRes...
   Vector<S32> multiResSize;
   Vector<F32> multiResPercent;
   getMultiResData(pNode,multiResSize,multiResPercent);

   if (multiResSize.size())
   {
      addMultiRes(pNode,pNode);
      for (S32 i=0; i<multiResSize.size(); i++)
         // om will be the same for each object
         om = addObject(pNode,pValidDetails,true,multiResSize[i],multiResPercent[i]);
   }
   else
     om = addObject(pNode,pValidDetails,false);

   // does this object have any decals?
   for (S32 i=0; i<pNode->NumberOfChildren(); i++)
     if (SceneEnumProc::isDecal(pNode->GetChildNode(i)))
        addDecalObject(pNode->GetChildNode(i),om);

   return om;
}

//--------------------------------------------
// add a mesh -- add MultiRes info
ObjectMimic * ShapeMimic::addObject(INode * pNode, Vector<S32> * pValidDetails, bool multiRes, S32 multiResSize, F32 multiResPercent)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return NULL;

   ObjectMimic * om;
   S32 size;
   S32 detailPos;
   const char * name = pNode->GetName();
   SceneEnumProc::tweakName(&name); // any re-mapping should be done here

   // separate object name from detail size for current mesh
   char * objectName = chopTrailingNumber(name,size);

   // artist can set detail level in the user properties if they want...
   S32 tmp;
   if (multiResSize>=0)
      size = multiResSize;
   else if (pNode->GetUserPropInt("Detail",tmp))
      size = tmp;

   om = getObject(pNode,objectName,size,&detailPos,multiResPercent);
   if (!om)
      return NULL;

   // set up the table of allowed detail levels
   // this will be checked when we start adding meshes
   // to the shape...start by making sure it hasn't
   // been set up already or if it has that at least
   // it points to the same place...
   if (om->pValidDetails && pValidDetails && pValidDetails!=om->pValidDetails)
   {
      setExportError(avar("Mesh \"%s\" occurs in two different places on the shape.",om->name));
      return NULL;
   }
   if (pValidDetails)
   {
      // set valid detail levels...
      om->pValidDetails = pValidDetails;
      om->inTreeNode = pNode;
      // we now know what subtree we belong in -- unless error
      if (om->subtreeNum>=0 && om->subtreeNum != subtrees.size()-1)
      {
         setExportError(avar("Mesh \"%s\" occurs in two different subtrees on the shape.",om->name));
         return NULL;
      }
      om->subtreeNum = subtrees.size() - 1;

      computeObjectOffset(pNode,om->objectOffset);

      // print out object offsets?
      if (dumpMask & PDObjectOffsets)
      {
         AffineParts parts;
         decomp_affine(om->objectOffset,&parts);
         printDump(PDObjectOffsets,     "Object offset transform:\r\n");
         printDump(PDObjectOffsets,avar("    scale:            x=%3.5f, y=%3.5f, z=%3.5f\r\n",parts.k.x,parts.k.y,parts.k.z));
         printDump(PDObjectOffsets,avar("    stretch rotation: x=%3.5f, y=%3.5f, z=%3.5f, w=%3.5f\r\n",parts.u.x,parts.u.y,parts.u.z,parts.u.w));
         printDump(PDObjectOffsets,avar("    translation:      x=%3.5f, y=%3.5f, z=%3.5f\r\n",parts.t.x,parts.t.y,parts.t.z));
         printDump(PDObjectOffsets,avar("    actual rotation:  x=%3.5f, y=%3.5f, z=%3.5f, w=%3.5f\r\n",parts.q.x,parts.q.y,parts.q.z,parts.q.w));
         if (parts.f<0)
            printDump(PDObjectOffsets,     "    ---determinant negative---\r\n");
      }
   }

   // if we were passed validDetails then we are on the shape (a hack for a check?)
   // in this case, fill in the parent of the object
   if (pValidDetails)
   {
      om->maxParent = pNode;
      om->maxTSParent = pNode; // this may change later...
   }

   return om;
}

//--------------------------------------------
// add decal object
void ShapeMimic::addDecalObject(INode * pNode, ObjectMimic * om)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   decalObjectList.increment();
   decalObjectList.last() = new DecalObjectMimic;
   DecalObjectMimic * dom = decalObjectList.last();
   dom->numDetails = 0;
   dom->targetObject = om;
   dom->decalNode = pNode;
};

//--------------------------------------------
// add bone object
ObjectMimic * ShapeMimic::addBoneObject(INode * pNode, S32 subtreeNum)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return NULL;

   S32 i;

   const char * name = pNode->GetName();
   S32 len = dStrlen(name)+20;
   char * boneName = new char[len];
   dSprintf(boneName,len,"Bone::%s:",name);

   S32 detailPos;
   ObjectMimic * om = getObject(pNode,boneName,0,&detailPos,-1,false,true,false);

   return om;
}

//--------------------------------------------
// add skin object -- different than above
// version of this method.  Above, we add
// an object to make sure bone doesn't get
// deleted.  Here we want to add an object
// that will actually go into shape and allow
// us to sort skins with other objects
MeshMimic * ShapeMimic::addSkinObject(SkinMimic * skinMimic)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return NULL;

   S32 i, size;

   // first, separate object name from detail size for current mesh
   const char * name = skinMimic->skinNode->GetName();
   char * objectName = chopTrailingNumber(name,size);

   S32 detailPos;

   ObjectMimic * om = getObject(skinMimic->skinNode,objectName,skinMimic->detailSize,&detailPos,-1,true,false,true);
   if (isError() || om==NULL) return NULL; // detailPos might not be valid...
   om->details[detailPos].mesh->skinMimic = skinMimic;

   om->subtreeNum = 0; // we'll assume skins are on the first detail...at least for now
   om->pValidDetails = &subtrees[om->subtreeNum]->validDetails;
   om->maxParent = om->maxTSParent = NULL;

   return om->details[detailPos].mesh;
}

//--------------------------------------------
// add skin...detect multi res
void ShapeMimic::addSkin(INode * pNode)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   // detect MultiRes...
   Vector<S32> multiResSize;
   Vector<F32> multiResPercent;
   const char * nodeName = pNode->GetName();
   for (S32 i=0; i<pNode->NumberOfChildren(); i++)
   {
      const char * childName = pNode->GetChildNode(i)->GetName();
      if (dStrnicmp(childName,"MultiRes::",dStrlen("MultiRes::")))
         continue;
      if (dStricmp(childName+dStrlen("MultiRes::"),nodeName))
         continue;
      getMultiResData(pNode->GetChildNode(i),multiResSize,multiResPercent);
      if (multiResSize.size())
      {
         addMultiRes(pNode,pNode->GetChildNode(i));
         break;
      }
   }

   // add skin helper modifier...
   Object * obj = pNode->GetObjectRef();
   IDerivedObject * dobj = (IDerivedObject*)CreateWSDerivedObject(pNode->GetObjectRef());
   SkinHelper * skinHelper = (SkinHelper*)CreateInstance(GetSkinHelperDesc()->SuperClassID(),GetSkinHelperDesc()->ClassID());
   dobj->AddModifier(skinHelper);
   pNode->SetObjectRef(dobj);

   if (multiResSize.size())
   {
      for (S32 i=0; i<multiResSize.size(); i++)
         addSkin(pNode,multiResSize[i],multiResPercent[i]);
   }
   else
      addSkin(pNode,-1,-1);

   // done with helper...remove it now...
   dobj->DeleteModifier(); // this'll be our skin helper

   // following copied from AVCUtil.cpp...is needed to get rid of bar in modifier list
   if (dobj->NumModifiers() == 0 && !dobj->TestAFlag(A_DERIVEDOBJ_DONTDELETE))
   {
      obj = dobj->GetObjRef();
      obj->TransferReferences(dobj);
      dobj->SetAFlag(A_LOCK_TARGET);
      dobj->NotifyDependents(FOREVER,0,REFMSG_SUBANIM_STRUCTURE_CHANGED);
      obj->NotifyDependents(FOREVER,0,REFMSG_SUBANIM_STRUCTURE_CHANGED);
      dobj->ClearAFlag(A_LOCK_TARGET);
      dobj->MaybeAutoDelete();
   }
}

//--------------------------------------------
// add skin...will create 1 object per bone...handle multi res
void ShapeMimic::addSkin(INode * pNode, S32 multiResSize, F32 multiResPercent)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   S32 i,j,k;

   skins.push_back(new SkinMimic);
   SkinMimic * skinMimic = skins.last();
   skinMimic->skinNode = pNode;
   skinMimic->multiResPercent = multiResPercent;
   if (multiResSize<0)
      dFree(chopTrailingNumber(pNode->GetName(),skinMimic->detailSize)); // want size, not name
   else
      skinMimic->detailSize = multiResSize;

   ISkin * skin;
   ISkinContextData * skinData;
   findSkinData(pNode,&skin,&skinData);
   if (!skin || !skinData)
   {
      setExportError("Assertion failed -- skin modifier was here a moment ago :(");
      return;
   }

   // get bones
   S32 numBones = skin->GetNumBones();
   skinMimic->bones.setSize(numBones);
   for (i=0; i<numBones; i++)
   {
      skinMimic->bones[i] = skin->GetBone(i);
      printDump(PDPass2,avar("Adding skin object from skin \"%s\" to bone \"%s\" (%i).\r\n",pNode->GetName(),skinMimic->bones[i]->GetName(),i));
   }

   // if no bones...don't add anything
   if (skinMimic->bones.empty())
   {
      delete skins.last();
      skins.decrement();
      return;
   }

   bool delTri;
   TriObject * tri = NULL;

   // get skin mesh
   tri = getTriObject(pNode,DEFAULT_TIME,-1,delTri);

   // get vertex weights from alternate tv channels
   S32 numPoints = tri->mesh.getNumVerts();
   if (tri->mesh.getNumMaps()<2+((1+numBones)>>1))
   {
      setExportError("Assertion failed on skin object");
      return;
   }
   skinMimic->weights.setSize(numBones);
   for (i=0; i<skinMimic->weights.size(); i++)
   {
      skinMimic->weights[i] = new SkinMimic::WeightList;
      skinMimic->weights[i]->setSize(numPoints);
      for (j=0; j<numPoints; j++)
         (*skinMimic->weights[i])[j]=0.0f;
   }

   for (j=0; j<numBones; j++)
   {
      printDump(-1,avar("Adding weights for bone %i (\"%s\")\r\n",j,skinMimic->bones[j]->GetName()));
      Vector<bool> gotWeight;
      gotWeight.setSize(tri->mesh.numVerts);
      for (i=0; i<gotWeight.size(); i++)
         gotWeight[i]=false;
      for (i=0; i<tri->mesh.numFaces; i++)
      {
         S32 ch = 2+(j>>1);
         Face & face = tri->mesh.faces[i];
         TVFace & tvFace = tri->mesh.mapFaces(ch)[i];

         for (S32 count=0; count<3; count++)
         {
            S32 idx = face.v[count];
            if (!gotWeight[idx])
            {
               UVVert tv = tri->mesh.mapVerts(ch)[tvFace.t[count]];
               F32 w = (j&1) ? tv.y : tv.x;
               (*skinMimic->weights[j])[idx] = w;
               if (w>0.01f)
                  printDump(-1,avar("   Vertex %i, weight %f\r\n",idx,w));
               gotWeight[idx]=true;
            }
         }
      }
   }

   if (delTri)
      delete tri;

   // for some reason, skin object likes to duplicate bones...delete dups here
   for (i=0; i<skinMimic->bones.size(); i++)
   {
      for (j=i+1; j<skinMimic->bones.size(); j++)
      {
         if (skinMimic->bones[i]==skinMimic->bones[j])
         {
            // delete weight data for this bone
            printDump(PDPass2,avar("Deleting duplicate skin object  \"%s\" -- stupid max.\r\n",skinMimic->bones[j]->GetName()));
            // transfer weights...to first instance
            for (k=0; k<skinMimic->weights[i]->size(); k++)
               (*skinMimic->weights[i])[k] += (*skinMimic->weights[j])[k];
            delete skinMimic->weights[j];
            skinMimic->weights.erase(j);
            skinMimic->bones.erase(j);
            j--;
         }
      }
   }

   // limit number of bones per vertex and apply weight threshhold
   for (i=0;i<skinMimic->weights[0]->size();i++)
   {
      F32 ** hi  = new F32 * [weightsPerVertex];
      for (k=0; k<weightsPerVertex; k++)
         hi[k]  = NULL;

      for (j=0; j<skinMimic->bones.size(); j++)
      {
         F32 & w = (*skinMimic->weights[j])[i];
         for (k=0; k<weightsPerVertex && (!hi[k] || *hi[k]<w); k++);
         k--;

         if (k<0 || w<weightThreshhold)
         {
            w=0.0f;
            continue;
         }

         S32 pos = k;

         // zero out least significant saved weight
         if (hi[0])
            *hi[0] = 0.0f;
         // shift all the weights below new one down a notch
         for (k=0;k<pos;k++)
            hi[k]  = hi[k+1];
         // add our new weight
         hi[pos] = &w;
      }
      F32 sum = 0;
      for (k=0;k<weightsPerVertex;k++)
         if (hi[k])
            sum += *hi[k];
      if (sum>weightThreshhold)
         for (k=0;k<weightsPerVertex;k++)
            if (hi[k])
               *hi[k] /= sum;

      delete [] hi;
   }

   // some bones may no longer have any weight...if so, throw them out
   for (i=0;i<skinMimic->bones.size();i++)
   {
      F32 sum = 0.0f;
      for (j=0;j<skinMimic->weights[i]->size();j++)
         sum += (*skinMimic->weights[i])[j];
      if (sum<weightThreshhold)
      {
         // delete weight data for this bone
         printDump(PDPass2,avar("Deleting skin object  \"%s\" with no weight.\r\n",skinMimic->bones[i]->GetName()));
         delete skinMimic->weights[i];
         skinMimic->weights.erase(i);
         skinMimic->bones.erase(i);
         i--;
      }
   }

   MeshMimic * meshMimic = addSkinObject(skinMimic); // goes into object list without node...

   // get the mesh from the skin node or the multi-res node
   if (multiResPercent<0.0f)
      tri = getTriObject(pNode,DEFAULT_TIME,-1,delTri);
   else
   {
      MultiResMimic * mrm = getMultiRes(pNode);
      remapWeights(skinMimic->weights,mrm->pNode,mrm->multiResNode);
      S32 multiResVerts = getMultiResVerts(pNode,multiResPercent);
      tri = getTriObject(mrm->multiResNode,DEFAULT_TIME,multiResVerts,delTri);
   }
   Mesh & maxMesh = tri->mesh;

   // get offset matrix
   Matrix3 toBounds = boundsNode->GetNodeTM(DEFAULT_TIME);
   zapScale(toBounds);
   toBounds = Inverse(toBounds);
   Matrix3 fromObj = multiResPercent<0.0f ?
      pNode->GetObjTMAfterWSM(DEFAULT_TIME) :
      getMultiRes(pNode)->multiResNode->GetObjTMAfterWSM(DEFAULT_TIME);
   fromObj *= toBounds;
   meshMimic->objectOffset = fromObj;

   // generate the faces of the mesh -- will be transfered to objects on subtrees later (as ts objects are generated)
   printDump(PDPass2,avar("Generating faces for skin \"%s\".\r\n",pNode->GetName()));

   generateFaces(pNode,
                 maxMesh,
                 meshMimic->objectOffset,
                 skinMimic->faces,
                 skinMimic->normals,
                 skinMimic->verts,
                 skinMimic->tverts,
                 skinMimic->indices,
                 skinMimic->smoothingGroups,
                 &skinMimic->vertId);
   meshMimic->numVerts = maxMesh.getNumVerts();

   // iterate through the subtrees looking for bones...when we find them, add a skin object
   Subtree * pSubtree;
   for (i=0; i<subtrees.size(); i++)
   {
      pSubtree = subtrees[i];
      NodeMimic * mimicNode = pSubtree->start.child;
      while (mimicNode)
      {
         if (mimicNode==&pSubtree->start)
         {
            // this should just never happen...
            setExportError("Assertion failed:  Illegal condition.");
            return;
         }

         // a bone?
         for (j=0; j<skinMimic->bones.size(); j++)
         {
            if (skinMimic->bones[j] == mimicNode->maxNode)
            {
               ObjectMimic * obj = addBoneObject(skinMimic->bones[j],i);
               if (!obj)
                  return;
               for (k=0;k<mimicNode->objects.size();k++)
                  if (mimicNode->objects[k]==obj)
                     break;
               if (k==mimicNode->objects.size())
                  mimicNode->objects.push_back(obj);
            }
         }

         // figure out where to go next
         mimicNode = findNextNode(mimicNode);
      }
   }

// What to do about these???

//skin->GetBoneInitTM(skin->GetBone(i), boneInitTM))
//skin->GetSkinInitTM(pNode, skinInitTM)

}

void ShapeMimic::copyWeightsToVerts(SkinMimic * skinMimic)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   // on input, weights are stored in a bone x vertId matrix
   // on input, weights will be stored in a bone x vert index matrix
   // difference between vertId and vert Index is that the former
   // signifies the order of the vert in max while the latter corresponds
   // to the order in our vert list

   // if we have weights, copy them and remap as we add verts
   S32 i,j;
   Vector<SkinMimic::WeightList*> oldWeights = skinMimic->weights;
   for (i=0; i<oldWeights.size(); i++)
      skinMimic->weights[i] = new SkinMimic::WeightList;

   for (i=0; i<oldWeights.size(); i++)
      for (j=0; j<skinMimic->vertId.size(); j++)
         skinMimic->weights[i]->push_back((*(oldWeights[i]))[skinMimic->vertId[j]]);

   for (i=0; i<oldWeights.size(); i++)
         delete oldWeights[i];
}

//--------------------------------------------
// Map weights from skin vertices to multires vertices
void ShapeMimic::remapWeights(Vector<SkinMimic::WeightList*> & weights, INode * skinNode, INode * multiResNode)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   Vector<SkinMimic::WeightList*> newWeights;
   S32 i,j;

   bool delTriSkin, delTriMR;
   TriObject * triMR, * triSkin = getTriObject(skinNode,DEFAULT_TIME,-1,delTriSkin);

   S32 multiResVerts = getMultiResVerts(skinNode,100.0f);
   triMR = getTriObject(multiResNode,DEFAULT_TIME,multiResVerts,delTriMR);

   newWeights.setSize(weights.size());
   for (i=0; i<newWeights.size(); i++)
      newWeights[i] = new SkinMimic::WeightList;

   Matrix3 skinT  = skinNode->GetObjTMAfterWSM(DEFAULT_TIME);
   Matrix3 multiT = multiResNode->GetObjTMAfterWSM(DEFAULT_TIME);

   for (i=0; i<triMR->mesh.getNumVerts(); i++)
   {
      // find vert in triSkin that each vert in triMR is closest to
      F32 closest = 10E10f;
      S32 idx = -1;
      for (j=0; j<triSkin->mesh.getNumVerts(); j++)
      {
         Point3F skinVert = Point3ToPoint3F(triSkin->mesh.verts[j] * skinT,skinVert);
         Point3F multiVert = Point3ToPoint3F(triMR->mesh.verts[i] * multiT,multiVert);
         Point3F delta = skinVert - multiVert;
         F32 d = delta.x*delta.x + delta.y*delta.y + delta.z*delta.z;
         if (d<closest)
         {
            closest = d;
            idx = j;
         }
      }
      if (idx>=0)
      {
         for (j=0; j<newWeights.size(); j++)
            newWeights[j]->push_back((*weights[j])[idx]);
      }
   }

   for (i=0; i<weights.size(); i++)
      delete weights[i];
   weights = newWeights; // copies pointers...which are still valid on exit

   if (delTriSkin)
      delete triSkin;
   if (delTriMR)
      delete triMR;
}

//--------------------------------------------
// add a sequence
void ShapeMimic::addSequence(INode * pNode)
{
    // if already encountered an error, then
    // we'll just go through the motions
    if (isError()) return;

    // just push it
    sequences.push_back(pNode);
}

//--------------------------------------------
// add a material
S32 ShapeMimic::addMaterial(INode * pNode, S32 materialIndex, CropInfo * cropInfo)
{
   // start out with no auxilary maps...reflection map will point back at us if none found
   S32 reflectionMap = -1;
   S32 bumpMap = -1;
   S32 detailMap = -1;
   F32 detailScale = 1.0f;
   F32 emapAmount = 1.0f;

   U32 flags = 0;

   // until proven otherwise...
   cropInfo->hasTVerts = false;
   cropInfo->crop = false;
   cropInfo->place = false;
   cropInfo->twoSided = false;

   // do some hocus pocus to get the material....

   Mtl * mtl = pNode->GetMtl();
   if( !mtl )
      return TSDrawPrimitive::NoMaterial;

   if( mtl->ClassID() == Class_ID(MULTI_CLASS_ID,0) )
   {
      MultiMtl * multiMtl = (MultiMtl*)mtl;
      if (multiMtl->NumSubMtls()==0)
         return TSDrawPrimitive::NoMaterial;
      materialIndex %= multiMtl->NumSubMtls();

      mtl = multiMtl->GetSubMtl( materialIndex );
   }

   if( mtl->ClassID() != Class_ID(DMTL_CLASS_ID,0) )
   {
      setExportError(avar("Unexpected material type on node \"%s\".",pNode->GetName()));
      return TSDrawPrimitive::NoMaterial;
   }

   StdMat * stdMat = (StdMat*)mtl;

   // we now have a standard material...this guy has a number of maps
   // the diffuse map corresponds to the texture
   // the reflection map is used for environment mapping (normally this will be in the alpha of
   //    the texture, but under some circumstances -- translucency -- you want a separate map for it)
   // the material also has a bump map and a detail map (we look for this in the ambient map...
   //    ambient because detail maps add ambiance... :)

   // are we two sided?
   cropInfo->twoSided = enableTwoSidedMaterials && stdMat->GetTwoSided();

   // get diffuse map...

   if (stdMat->GetSubTexmap(ID_DI) == NULL || !stdMat->MapEnabled(ID_DI))
      return TSDrawPrimitive::NoMaterial;

   if (stdMat->GetSubTexmap(ID_DI)->ClassID() != Class_ID(BMTEX_CLASS_ID,0))
   {
      setExportError(avar("Diffuse channel on node \"%s\" has a non-bitmap texture map.",pNode->GetName()));
      return TSDrawPrimitive::NoMaterial;
   }

   BitmapTex * diffuse = (BitmapTex*)stdMat->GetSubTexmap(ID_DI);
   cropInfo->hasTVerts = true;
   const char * materialName = getBaseTextureName(diffuse->GetMapName());

   // get reflection map...

   BitmapTex * reflection = NULL;
   if (stdMat->GetSubTexmap(ID_RL) && stdMat->MapEnabled(ID_RL))
   {
      if (stdMat->GetSubTexmap(ID_RL)->ClassID() != Class_ID(BMTEX_CLASS_ID,0))
      {
         setExportError(avar("Reflection channel on node \"%s\" has a non-bitmap texture map.",pNode->GetName()));
         return TSDrawPrimitive::NoMaterial;
      }
      reflection = (BitmapTex*)stdMat->GetSubTexmap(ID_RL);

      // reflection map will be alpha channel of diffuse map...
      // make sure they have the same base name...
      const char * materialName2 = getBaseTextureName(reflection->GetMapName());
      const char * end1 = dStrrchr(materialName ,'.');
      const char * end2 = dStrrchr(materialName2,'.');
      const char * s1 = materialName;
      const char * s2 = materialName2;
      while (s1!=end1 && s2!=end2 && *s1==*s2)
      {
         s1++; s2++;
      }
      if (s1!=end1 || s2!=end2)
         // reflection map and diffuse map different...
         reflectionMap = findMaterial(materialName2,reflection->GetMapName(),TSMaterialList::ReflectanceMapOnly,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF);
   }

   // get bump map...

   if (stdMat->MapEnabled(ID_BU) && stdMat->GetSubTexmap(ID_BU))
   {
      if (stdMat->GetSubTexmap(ID_BU)->ClassID() != Class_ID(BMTEX_CLASS_ID,0))
      {
         setExportError(avar("Bump map channel on node \"%s\" has a non-bitmap texture map.",pNode->GetName()));
         return TSDrawPrimitive::NoMaterial;
      }

      BitmapTex * bump = (BitmapTex*)stdMat->GetSubTexmap(ID_BU);
      const char * bumpName = getBaseTextureName(bump->GetMapName());
      bumpMap = findMaterial(bumpName,bump->GetMapName(),TSMaterialList::BumpMapOnly,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF);
   }

   // get detail map...

   if (stdMat->MapEnabled(ID_AM) && stdMat->GetSubTexmap(ID_AM))
   {
      if (stdMat->GetSubTexmap(ID_AM)->ClassID() != Class_ID(BMTEX_CLASS_ID,0))
      {
         setExportError(avar("Detail map channel (ambient channel) on node \"%s\" has a non-bitmap texture map.",pNode->GetName()));
         return TSDrawPrimitive::NoMaterial;
      }

      BitmapTex * detail = (BitmapTex*)stdMat->GetSubTexmap(ID_AM);
      const char * detailName = getBaseTextureName(detail->GetMapName());
      detailMap = findMaterial(detailName,detail->GetMapName(),TSMaterialList::DetailMapOnly,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF);
      detailScale = detail->GetUVGen()->GetUScl(0);
      if (mFabs(detailScale-detail->GetUVGen()->GetVScl(0)) > 0.01f)
      {
         setExportError(avar("U-scale must match V-scale on detail texture (found in ambient channel) on texture %s",detailName));
         return TSDrawPrimitive::NoMaterial;
      }
   }

   // set up crop info
   diffuse->GetUVGen()->GetUVTransform(cropInfo->uvTransform);
   cropInfo->uWrap = diffuse->GetUVGen()->GetTextureTiling() & U_WRAP;
   cropInfo->vWrap = diffuse->GetUVGen()->GetTextureTiling() & V_WRAP;
   IParamBlock2 * pblock = diffuse->GetParamBlock(0);
   Interval interval;
   S32 apply;
   pblock->GetValue(PB_APPLY,DEFAULT_TIME,apply,interval);
   if (apply)
   {
      S32 cropOrPlace;
      pblock->GetValue(PB_CROP_PLACE,DEFAULT_TIME,cropOrPlace,interval);
      if (cropOrPlace==0)
         cropInfo->crop = true;
      else
      {
         cropInfo->place = true;
         // don't wrap if texture placed...
         cropInfo->uWrap = cropInfo->vWrap = false;
      }
      pblock->GetValue(PB_CLIPU,DEFAULT_TIME,cropInfo->uOffset,interval);
      pblock->GetValue(PB_CLIPV,DEFAULT_TIME,cropInfo->vOffset,interval);
      pblock->GetValue(PB_CLIPW,DEFAULT_TIME,cropInfo->uWidth,interval);
      pblock->GetValue(PB_CLIPH,DEFAULT_TIME,cropInfo->vHeight,interval);
      if (cropInfo->crop)
      {
         cropInfo->cropLeft = cropInfo->uOffset>0.001f;
         cropInfo->cropRight = cropInfo->uOffset+cropInfo->uWidth<0.999f;
         cropInfo->cropTop = cropInfo->vOffset>0.001f;
         cropInfo->cropBottom = cropInfo->vOffset+cropInfo->vHeight<0.999f;
         if (cropInfo->uWrap && (cropInfo->cropLeft != cropInfo->cropRight))
            cropInfo->cropLeft = cropInfo->cropRight = true;
         if (cropInfo->vWrap && (cropInfo->cropTop != cropInfo->cropBottom))
            cropInfo->cropTop = cropInfo->cropBottom = true;
      }
   }

   // set up texture flags
   if (stdMat->MapEnabled(ID_OP)) // note:  translucent if opacity channel is enabled
   {
      flags |= TSMaterialList::Translucent;
      if (stdMat->GetTransparencyType() == TRANSP_ADDITIVE)
         flags |= TSMaterialList::Additive;
      else if (stdMat->GetTransparencyType() == TRANSP_SUBTRACTIVE)
         flags |= TSMaterialList::Subtractive;
   }
   if (!stdMat->MapEnabled(ID_RL))
      // only environment map if reflectance check box is set (but no material necessary)
      flags |= TSMaterialList::NeverEnvMap;
   emapAmount = stdMat->GetTexmapAmt(ID_RL,DEFAULT_TIME);

   if (cropInfo->uWrap)
      flags |= TSMaterialList::S_Wrap;
   if (cropInfo->vWrap)
      flags |= TSMaterialList::T_Wrap;
   if ( stdMat->GetSelfIllum(0) > 0.99f)
      flags |= TSMaterialList::SelfIlluminating;

   return findMaterial(materialName,diffuse->GetMapName(),flags,reflectionMap,bumpMap,detailMap,detailScale,emapAmount);
}

S32 ShapeMimic::findMaterial(const char * name, const char * fullName, U32 flags, S32 reflectionMap, S32 bumpMap, S32 detailMap, F32 detailScale, F32 emapAmount)
{
   // return the index to this material if we already have it saved off
   // otherwise create a new entry...
   // material considered new if anything is different (name, flags, or any of the maps)
   // exception #1 is if the incoming item is an auxiliary item (a refelctionMap, bumpMap,
   //    or detail map) in that case, it will ignore everything but the name...
   // exception #2 is if the incoming item is an ifl (in which case it is added to back)

   bool auxiliary = flags & TSMaterialList::AuxiliaryMap;
   if (dStrstr(name,".ifl") || dStrstr(name,".IFL"))
      flags |= TSMaterialList::IflMaterial;

   if (gMipmapMethod.noMipmap)
      flags |= TSMaterialList::NoMipMap;
   if (gMipmapMethod.noMipmapTranslucent && flags & TSMaterialList::Translucent)
      flags |= TSMaterialList::NoMipMap;
   if (flags & TSMaterialList::Translucent && !(flags & TSMaterialList::S_Wrap|TSMaterialList::T_Wrap) && gMipmapMethod.zapBorder)
      // material is translucent and doesn't wrap -- we're clearing the borders on the mipmaps of such materials
      flags |= TSMaterialList::MipMap_ZeroBorder;

   // we don't care about extension (unless we're an ifl
   const char * dot = dStrrchr(name,'.');
   S32 len = dot ? dot-name : dStrlen(name);

   for (S32 i=0; i<materials.size(); i++)
   {
      // first check name
      if (dStrlen(materials[i]) && (dStrnicmp(materials[i],name,len) || dStrlen(materials[i])!=len))
         continue;

      // good enough for auxiliary
      if (auxiliary)
      {
         if (materialFlags[i] & TSMaterialList::AuxiliaryMap)
            // if we're using an auxiliary map for two purpose, let it be known...
            materialFlags[i] |= (flags & TSMaterialList::AuxiliaryMap);
         return i;
      }

      // a reflection map of -1 gets mapped to i...
      if (reflectionMap!=-1 && materialReflectionMaps[i]!=(U32)reflectionMap)
         continue;

      // check the rest
      if (materialFlags[i]==flags && materialBumpMaps[i]==bumpMap && materialDetailMaps[i]==detailMap &&
          mFabs(materialDetailScales[i]-detailScale)<0.01f && mFabs(materialEmapAmounts[i]-emapAmount)<0.01f)
         return i;
   }

   // check to see if material is an ifl
   if (dStrstr(name,".ifl") || dStrstr(name,".IFL"))
   {
      iflList.push_back(new IflMimic);
      iflList.last()->fileName = dStrdup(name);
      iflList.last()->fullFileName = dStrdup(fullName);
      iflList.last()->materialSlot = materials.size();
   }

   appendMaterialList(name,flags,reflectionMap,bumpMap,detailMap,detailScale,emapAmount);
   return materials.size()-1;
}

void ShapeMimic::appendMaterialList(const char * name, U32 flags, S32 reflectionMap, S32 bumpMap, S32 detailMap, F32 detailScale, F32 emapAmount)
{
   // always have a reflection map, even if it's ourself
   if (reflectionMap==-1 && !(flags&TSMaterialList::AuxiliaryMap))
      reflectionMap = materialReflectionMaps.size();

   S32 len = dStrlen(name);
   const char * dot = dStrrchr(name,'.');
   if (dot)
      len = dot-name;

   // new name...
   char * matName = (char*)dMalloc(len+1);
   dStrncpy(matName,name,len);
   matName[len]='\0';

   // new one -- save the texture map material and return new index
   materials.push_back(matName);
   materialFlags.push_back(flags);
   materialReflectionMaps.push_back(reflectionMap);
   materialBumpMaps.push_back(bumpMap);
   materialDetailMaps.push_back(detailMap);
   materialDetailScales.push_back(detailScale);
   materialEmapAmounts.push_back(emapAmount);
}

//--------------------------------------------
// adjust texture name -- remove base texture path
const char * ShapeMimic::getBaseTextureName(const char * name)
{
   const char * retName;
   if (baseTexturePath[0]=='.')
   {
      retName = dStrrchr(name,'\\');
      if (!retName++)
         retName = name;
   }
   else
   {
      retName = dStrstr(name,baseTexturePath);
      if (!retName)
      {
         retName = name;
         setExportError(avar("Material \"%s\" must be in a subdirectory of \"%s\" -- or you can change the base texture path",name,baseTexturePath));
      }
      else
         retName += dStrlen(baseTexturePath);
   }

   return retName;
}

//--------------------------------------------
// add a name to the shape
S32 ShapeMimic::addName(const char * name, TSShape * pShape)
{
    SceneEnumProc::tweakName(&name); // chops off beginning only

    S32 ret = pShape->findName(name);
    if (ret<0)
    {
        ret = pShape->names.size();
        pShape->names.increment();
        char * newName = dStrdup(name);
        // if name terminated with CR, get rid of it
        while (newName && dStrlen(newName)>0 && newName[dStrlen(newName)-1]==(char)13)
           newName[dStrlen(newName)-1] = '\0';
        pShape->names.last() = newName;
    }
    return ret;
}

//--------------------------------------------
// walk through mimic node structure depth first
NodeMimic * ShapeMimic::findNextNode(NodeMimic * cur)
{
    if (cur->child)
        return cur->child;
    if (cur->sibling)
        return cur->sibling;
    while (cur->parent)
    {
        if (cur->parent->sibling)
            return cur->parent->sibling;
        cur = cur->parent;
    }
    // done...
    return NULL;
}

//--------------------------------------------
// clear lists for next time around
void ShapeMimic::clearCollapseTransforms()
{
   cutNodes.clear();
   cutNodesParents.clear();
}

//--------------------------------------------
// prune shape of unneeded nodes
void ShapeMimic::collapseTransforms()
{
    // if already encountered an error, then
    // we'll just go through the motions
    if (isError()) return;

    printDump(PDPass3,"\r\nThird pass:  Collapsing unneeded nodes...\r\n\r\n");

    Subtree * pSubtree;
    for (S32 i=0; i<subtrees.size(); i++)
    {
        pSubtree = subtrees[i];
        NodeMimic * mimicNode = pSubtree->start.child;
        while (mimicNode)
        {
           if (mimicNode==&pSubtree->start)
           {
               // this should just never happen...
               setExportError("Assertion failed:  Illegal condition.");
               return;
           }

           // figure out where to go next now
           // because we might end up cutting
           // out the current node
           NodeMimic * nextNode = findNextNode(mimicNode);

           // cut?
           if (cut(mimicNode))
           {
               printDump(PDPass3,avar("Removing node \"%s\"\r\n",mimicNode->maxNode->GetName()));
               snip(mimicNode);
           }

           mimicNode = nextNode;
        }
    }
}

// perform string compare with wildcards (in s2 only) and case insensitivity
bool stringEqual(const char * s1, const char * s2)
{
    if (*s1=='\0' && *s2=='\0')
        return true;

    if (*s2=='*')
    {
        if (stringEqual(s1,s2+1))
            return true;
        if (*s1=='\0')
            return false;
        return stringEqual(s1+1,s2);
    }
    if (toupper(*s1)==toupper(*s2))
        return stringEqual(s1+1,s2+1);
    return false;
}

bool ShapeMimic::cut(NodeMimic * mimicNode)
{
    const char * name = mimicNode->maxNode->GetName();

    S32 i;

    // search always export list
    for (i=0;i<SceneEnumProc::alwaysExport.size(); i++)
        if (stringEqual(name,SceneEnumProc::alwaysExport[i]))
            return false;

    // search never export list
    for (i=0;i<SceneEnumProc::neverExport.size(); i++)
        if (stringEqual(name,SceneEnumProc::neverExport[i]))
            return true;

    // if transform collapse is false, only collapse explicitly named nodes (in neverExport list)
    if (!transformCollapse)
      return false;

    // not in either list -- cut if no object and not dummy
    return (mimicNode->objects.empty() && !SceneEnumProc::isDummy(mimicNode->maxNode));
}

bool ShapeMimic::neverAnimateNode(NodeMimic * mimicNode)
{
    const char * name = mimicNode->maxNode->GetName();

    // search always export list
    for (S32 i=0;i<SceneEnumProc::neverAnimate.size(); i++)
        if (stringEqual(name,SceneEnumProc::neverAnimate[i]))
            return true;
    return false;
}

void ShapeMimic::snip(NodeMimic * nodeMimic)
{
    // if nodeMimic has a mesh, we want to make sure there is no animation between it
    // and it's parent...can't do that yet (no sequence) so add to a list and check later
    if (!nodeMimic->objects.empty())
    {
        cutNodes.push_back(nodeMimic->maxNode);
        cutNodesParents.push_back(nodeMimic->parent->maxNode);
    }

    // get rid of nodeMimic, bring children up to nodeMimic level
    // they will sit between nodeMimic's siblings

    // go to parent, find me
    // point parent->child or prev-sibling->sibling pointer to my child
    NodeMimic * parent = nodeMimic->parent;
    NodeMimic * n;
    if (parent->child == nodeMimic)
        parent->child = nodeMimic->child ? nodeMimic->child : nodeMimic->sibling;
    else
    {
        n = parent->child;
        while (n->sibling != nodeMimic)
            n = n->sibling;
        n->sibling = nodeMimic->child ? nodeMimic->child : nodeMimic->sibling;
    }

    // point my children's parent pointer to my parent
    n = nodeMimic->child;
    if (n)
    {
       n->parent = nodeMimic->parent;
       while (n->sibling)
       {
           n = n->sibling;
           n->parent = nodeMimic->parent;
       }
       // point my last child's sibling point to my sibling pointer
       n->sibling = nodeMimic->sibling;
    }

    // attach my objects to my parent
    for (S32 i=0; i<nodeMimic->objects.size(); i++)
    {
        parent->objects.push_back(nodeMimic->objects[i]);
        parent->objects.last()->maxTSParent = parent->maxNode; // until notified o.w.
    }

    // delete me
    delete nodeMimic;
}

//--------------------------------------------
// initialize a shape after it's generated
// mostly just let shape do it...but some
// things are done pre-export that aren't
// done other times
void ShapeMimic::initShape(TSShape * pShape)
{
   S32 numSubShapes = pShape->subShapeFirstNode.size();

   // compute subShapeNumNodes,
   pShape->subShapeNumNodes.setSize(numSubShapes);
   S32 i,j,prev = pShape->nodes.size();
   for (i=numSubShapes-1; i>=0; i--)
   {
      pShape->subShapeNumNodes[i] = prev - pShape->subShapeFirstNode[i];
      prev = pShape->subShapeFirstNode[i];
   }

   // compute subShapeNumObjects
   pShape->subShapeNumObjects.setSize(numSubShapes);
   prev = pShape->objects.size();
   for (i=numSubShapes-1; i>=0; i--)
   {
      pShape->subShapeNumObjects[i] = prev - pShape->subShapeFirstObject[i];
      prev = pShape->subShapeFirstObject[i];
   }

   // compute subShapeNumDecals
   pShape->subShapeNumDecals.setSize(numSubShapes);
   prev = pShape->decals.size();
   for (i=numSubShapes-1; i>=0; i--)
   {
      pShape->subShapeNumDecals[i] = prev - pShape->subShapeFirstDecal[i];
      prev = pShape->subShapeFirstDecal[i];
   }

   // find the smallest renderable detail level
   pShape->mSmallestVisibleSize = 0;
   pShape->mSmallestVisibleDL   = 0;
   for (i=0; i<pShape->details.size(); i++)
   {
      if (pShape->details[i].size>=0.0f)
      {
         pShape->mSmallestVisibleSize = pShape->details[i].size;
         pShape->mSmallestVisibleDL   = i;
      }
      pShape->details[i].maxError     = -1.0f;
      pShape->details[i].averageError = -1.0f;
   }

   for (i=0; i<pShape->objects.size(); i++)
   {
      for (S32 j=0;j<pShape->objects[i].numMeshes;j++)
         if (pShape->meshes[pShape->objects[i].startMeshIndex+j])
            pShape->meshes[pShape->objects[i].startMeshIndex+j]->computeBounds();
   }

   // catch decals that don't do anything
   for (i=0; i<pShape->decals.size(); i++)
   {
      for (j=0; j<pShape->decals[i].numMeshes; j++)
      {
         S32 idx = pShape->decals[i].startMeshIndex+j;
         if (pShape->meshes[idx] && !((TSDecalMesh*)pShape->meshes[idx])->texgenS.size())
         {
            // empty...get rid of it
            delete (TSDecalMesh*)pShape->meshes[idx];
            pShape->meshes[idx] = NULL;
         }
      }
   }

   // copmpute detail errors...
   for (i=0; i<pShape->details.size(); i++)
   {
      TSShape::Detail & detail = pShape->details[i];
      Vector<TSMesh*> & meshes = pShape->meshes;
      F32 totalMaxDist = 0.0f, curMaxMax = 0.0f;
      S32 count = 0;
      for (j=0; j<pShape->objects.size(); j++)
      {
         // get highest mesh and get current mesh
         TSMesh * hiMesh = NULL, * curMesh = NULL;
         TSShape::Object & obj = pShape->objects[j];
         for (S32 k=0; k<obj.numMeshes; k++)
       {
            if (meshes[obj.startMeshIndex+k])
            {
               hiMesh = meshes[obj.startMeshIndex+k];
               break;
            }
         }
         if (obj.numMeshes > detail.objectDetailNum)
           curMesh = meshes[obj.startMeshIndex + detail.objectDetailNum];
         if (!curMesh || !hiMesh)
           continue;

         F32 total;
         S32 cnt;
         F32 maxDist = findMaxDistance(curMesh,hiMesh,total,cnt);
         totalMaxDist += total;
         count += cnt;
         if (maxDist > curMaxMax)
            curMaxMax = maxDist;
      }
      F32 avgDist = count ? totalMaxDist / (F32)count : 0.0f;
      detail.averageError = avgDist;
      detail.maxError = curMaxMax;
   }

   pShape->init();
}

//
void ShapeMimic::destroyShape(TSShape * pShape)
{
   if (!pShape)
      return;

   // delete the meshes ourselves since we newed them and shape
   // assumes they were constructed in place...

   // before deleting meshes, we have to delete decals and get them out
   // of the mesh list...this is a legacy issue from when decals were meshes
   S32 i;
   for (i=0; i<pShape->decals.size(); i++)
   {
      for (S32 j=0; j<pShape->decals[i].numMeshes; j++)
      {
         delete (TSDecalMesh*)pShape->meshes[pShape->decals[i].startMeshIndex+j];
         pShape->meshes[pShape->decals[i].startMeshIndex+j]=NULL;
      }
   }

   // everything left over here is a legit mesh
   for (i=0; i<pShape->meshes.size(); i++)
   {
      delete pShape->meshes[i];
      pShape->meshes[i] = NULL;
   }

   delete pShape;
}

//--------------------------------------------
// generate a shape
TSShape * ShapeMimic::generateShape()
{
    // if already encountered an error, then
    // we'll just go through the motions
    if (isError()) return NULL;

    // this may be our second time around, make sure
    // certain variables and lists are initialized:
    nodes.clear();

    // before going any further, do a little fix-up for auto-detail generation
    fixupT2AutoDetail();

    // no frills construction
    TSShape * pShape = new TSShape;
    pShape->mExporterVersion = DTS_EXPORTER_CURRENT_VERSION;

    // step one:    generate bounds
    generateBounds(pShape);

    // step two:    generate detail levels
    //              sort subTrees according to dl
    generateDetails(pShape);

    // step three:  generate subTrees (tree structure
    //              without objects connected)
    generateSubtrees(pShape);

    // step four:   generate objects -- hook up to nodes
    generateObjects(pShape);

    // if already encountered an error, then
    // we'll just go through the motions
    if (isError()) return NULL;

    // step fourB:   generate decals
    generateDecals(pShape);

    // if already encountered an error, then
    // we'll just go through the motions
    if (isError()) return NULL;

    // at this point, we have a tsshape with all the details,
    // nodes, and objects set up.  We have also set up the
    // subShapeFirstNode and subShapeFirstObject vectors and
    // added a bunch of names.  In addition, the meshIndexList
    // is set up and the meshes list is set up.

    // step five:   set default states (including mesh data)
    generateDefaultStates(pShape);

    // if already encountered an error, then
    // we'll just go through the motions
    if (isError()) return NULL;

    // step six:    generate ifl materials
    generateIflMaterials(pShape);

    // if already encountered an error, then
    // we'll just go through the motions
    if (isError()) return NULL;

    // step seven:  animation
    if (enableSequences)
        generateSequences(pShape);

    // if already encountered an error, then
    // we'll just go through the motions
    if (isError()) return NULL;

    // step eight:  generate material list
    generateMaterialList(pShape);

    // if already encountered an error, then
    // we'll just go through the motions
    if (isError()) return NULL;

    // step eight:  generate the skins
    generateSkins(pShape);

    // if already encountered an error, then
    // we'll just go through the motions
    if (isError()) return NULL;

    // step nine:   optimize the meshes (but only if exporting them)
    if (SceneEnumProc::exportType == 'w')
       optimizeMeshes(pShape);

    // if already encountered an error, then
    // we'll just go through the motions
    if (isError()) return NULL;

    // step ten:    convert sortObjects
    convertSortObjects(pShape);

    if (isError()) return NULL;

    // what else?
    initShape(pShape);

    return pShape;
}

//--------------------------------------------
// generate bounds -- called by generateShape
void ShapeMimic::generateBounds(TSShape * pShape)
{
    // if already encountered an error, then
    // we'll just go through the motions
    if (isError()) return;

    bool delTri;
    TriObject * tri = getTriObject(boundsNode,DEFAULT_TIME,-1,delTri);
    Mesh & maxMesh = tri->mesh;

    // get object offset
    Matrix3 objectOffset(true);
    Point3 pos = boundsNode->GetObjOffsetPos();
    objectOffset.PreTranslate(pos);
    Quat quat = boundsNode->GetObjOffsetRot();
    PreRotateMatrix(objectOffset,quat);
    ScaleValue objectScale = boundsNode->GetObjOffsetScale();
    ApplyScaling(objectOffset,objectScale);

    // find min and max verts
    S32 i;
    Point3F minVert = Point3ToPoint3F(maxMesh.verts[0] * objectOffset,minVert);
    Point3F maxVert = minVert;
    for (i=1; i<maxMesh.numVerts; i++)
    {
        Point3F v = Point3ToPoint3F(maxMesh.verts[i] * objectOffset,v);
        minVert.setMin(v);
        maxVert.setMax(v);
    }

    // now set up shape bounds parameters
    pShape->center = (minVert + maxVert) * 0.5f;
    pShape->bounds.min = minVert;
    pShape->bounds.max = maxVert;

    // find the smallest radius that includes bounds...
    // if artist uses box as bounds we could just use
    // boundsBox to figure this out -- but the following
    // allows them to use a bounding sphere and sometimes
    // get a smaller radius...
    F32 maxRadius2 = -1.0f;
    F32 radius2;
    for (i=0; i<maxMesh.numVerts; i++)
    {
        Point3F v2 = Point3ToPoint3F(maxMesh.verts[i] * objectOffset,v2);
        Point3F radial3 = v2 - pShape->center;
        radius2 = mDot(radial3,radial3);
        if (radius2 > maxRadius2)
            maxRadius2 = radius2;
    }
    pShape->radius = mSqrt(maxRadius2);

    // find the smallest z-axis aligned bounding tube...
    maxRadius2 = -1.0f;
    for (i=0; i<maxMesh.numVerts; i++)
    {
        Point2F radial2;
        Point3 maxV = maxMesh.verts[i] * objectOffset;
        radial2.x = maxV.x - pShape->center.x;
        radial2.y = maxV.y - pShape->center.y;
        radius2 = radial2.x * radial2.x + radial2.y * radial2.y;
        if (radius2 > maxRadius2)
            maxRadius2 = radius2;
    }
    pShape->tubeRadius = mSqrt(maxRadius2);

    if (delTri)
        delete tri;
}

//--------------------------------------------
// generate details -- called by generateShape
void ShapeMimic::generateDetails(TSShape * pShape)
{
    // if already encountered an error, then
    // we'll just go through the motions
    if (isError()) return;

    // if nothing to export...
    if (subtrees.empty())
    {
        setExportError("No details to export");
        return;
    }

    // each subTree has a set of detail levels
    // just plug them in and sort the list

    S32 i,j;
    for (i=0; i<subtrees.size(); i++)
    {
        Subtree * pSubtree = subtrees[i];
        for (j=0; j<pSubtree->validDetails.size(); j++)
        {
            pShape->details.increment();
            TSShape::Detail & detail = pShape->details.last();
            detail.subShapeNum = i;
            detail.objectDetailNum = j;
            detail.size = (F32) pSubtree->validDetails[j];
            detail.nameIndex = addName(pSubtree->detailNames[j],pShape);
            if (!dStrnicmp(pSubtree->detailNames[j],"BB::",4))
            {
               // this is a billboard detail, this works a little differently...
               detail.subShapeNum = -1;

               // determine properties...
               S32 numEquatorSteps;
               S32 numPolarSteps;
               F32 polarAngle;
               S32 dl;
               S32 dim;
               S32 includePoles;
               INode * pNode = pSubtree->detailNodes[j];
               if (!pNode->GetUserPropInt("BB::EQUATOR_STEPS",numEquatorSteps))
                  numEquatorSteps = 4;
               if (!pNode->GetUserPropInt("BB::POLAR_STEPS",numPolarSteps))
                  numPolarSteps = 0;
               if (!pNode->GetUserPropFloat("BB::POLAR_ANGLE",polarAngle))
                  polarAngle = M_PI/(F32)(((numPolarSteps>>1)<<1)+5);
               if (!pNode->GetUserPropInt("BB::DL",dl))
                  dl = 0;
               if (!pNode->GetUserPropInt("BB::DIM",dim))
                  dim = 64;
               if (!pNode->GetUserPropBool("BB::INCLUDE_POLES",includePoles))
                  includePoles = true;

               // set properties...
               U32 props = 0;
               props |= numEquatorSteps;
               props |= (numPolarSteps>>1) << 7;
               props |= ((S32)(64.0f * polarAngle/(M_PI*0.5f))) << 13;
               props |= dl<<19;
               props |= dim<<23;
               props |= includePoles ? 1<<31 : 0;
               detail.objectDetailNum = props;
            }
        }
    }

    // sort detail levels based on projection size
    sortTSDetails(pShape->details);

    // optionally, check to make sure detail trees
    // are not crossed -- that is, that all details
    // of a particular subtree are consecutive...
    // not really problem for the code, but usually
    // indicates a shape error
    if (!allowCrossedDetails)
    {
        Vector<S32> checkers;
        S32 last = -999;
        for (i=0; i<pShape->details.size(); i++)
        {
            TSShape::Detail & det = pShape->details[i];
            if (det.subShapeNum==last)
                continue;
            // new block -- make sure we haven't seen this yet
            for (j=0; j<checkers.size(); j++)
                if (checkers[j]==det.subShapeNum)
                {
                    setExportError("Crossed detail levels -- this error can be turned off.");
                    return;
                }
            checkers.push_back(det.subShapeNum);
            last = det.subShapeNum;
        }
    }
}


//--------------------------------------------
// generate sub trees -- called by generateShape
void ShapeMimic::generateSubtrees(TSShape * pShape)
{
    // if already encountered an error, then
    // we'll just go through the motions
    if (isError()) return;

    // this should already have been caught, but...
    if (subtrees.empty())
    {
        setExportError("No details to export");
        return;
    }

    // generate a set of nodes for each subtree
    S32 i;
    for (i=0; i<subtrees.size(); i++)
    {
        Subtree * pSubtree = subtrees[i];
        pSubtree->start.number = -1; // translates to NULL...
                                     // this means branches will have no parent
        NodeMimic * curNode = pSubtree->start.child;

        // mark the beginning of the subshape
        pShape->subShapeFirstNode.increment();
        pShape->subShapeFirstNode.last() = pShape->nodes.size();

        // traverse depth first
        while (curNode)
        {
            curNode->number = pShape->nodes.size();

            // add node to tsshape
            pShape->nodes.increment();
            TSShape::Node & tsnode = pShape->nodes.last();

            tsnode.nameIndex = addName(curNode->maxNode->GetName(),pShape);
            tsnode.parentIndex = curNode->parent->number; // special case: start->number = -1 -> NULL

            // set up ShapeMimic::Object with the right ts node index
            for (S32 j=0;j<curNode->objects.size(); j++)
                curNode->objects[j]->tsNodeIndex = curNode->number;

            // add NodeMimic to nodes list to keep track of order of nodes in shape
            nodes.push_back(curNode);

            // figure out where to go next
            curNode = findNextNode(curNode);
        }
    }
}

//--------------------------------------------
// generate objects -- called by generateShape
void ShapeMimic::generateObjects(TSShape * pShape)
{
    // if already encountered an error, then
    // we'll just go through the motions
    if (isError()) return;

    S32 i,j;

    // index into shapes meshes
    S32 nextMeshIndex = 0;

    // set object priority -- requires running through all the faces of
    // all the meshes and extracting the materials...
    setObjectPriorities(objectList);

    // sort the objectList by subTree and priority
    sortObjectList(objectList);

    // if already encountered an error, then
    // we'll just go through the motions
    if (isError()) return;

    // initialize array that indexes first object in subshape
    for (i=0; i<subtrees.size(); i++)
    {
        pShape->subShapeFirstObject.increment();
        pShape->subShapeFirstObject.last() = -1;
    }

    // go through mesh list and add objects as we go
    for (i=0; i<objectList.size(); i++)
    {
        // if already encountered an error, then
        // we'll just go through the motions
        if (isError()) return;

        ObjectMimic * pObject = objectList[i];
        if (!allowUnusedMeshes && !pObject->pValidDetails)
        {
            setExportError(avar("Mesh \"%s\" not hooked up to shape -- this error can be turned off"));
            return;
        }

        // we may have cut out our actual parent...if so, we need to update object offset
        if (pObject->maxParent != pObject->maxTSParent)
        {
            // if a bone...
            if (pObject->isBone)
            {
               // trying to cut out a bone node ... not allowed
               setExportError(avar("Cannot collapse node \"%s\" because it is a bone.",pObject->maxParent->GetName()));
               return;
            }

            // compute the transform from the unscaled maxParent transform to the
            // unscaled maxTSParent transform...
            // -- maxParent is the max node the object hangs off of in max.
            // -- maxTSParent is the max node the object will hang off of when
            //    exported to the ts shape (or said a longer way, the max node that
            //    corresponds to the ts node that the object will hang off of).
            // -- unscaled transforms are used because that is what get's stored in
            //    the ts shapes (the original objectOffset also was to the unscaled
            //    maxParent transform).

            Matrix3 m1,m2;
            m1 = pObject->maxParent->GetNodeTM(DEFAULT_TIME);
            zapScale(m1);
            m2 = pObject->maxTSParent->GetNodeTM(DEFAULT_TIME);
            zapScale(m2);
            m2 = Inverse(m2);
            m1 *= m2;
            pObject->objectOffset *= m1;

            // print out revised object offsets?
            if (dumpMask & PDObjectOffsets)
            {
               AffineParts parts;
               decomp_affine(pObject->objectOffset,&parts);
               printDump(PDObjectOffsets,avar("Revising object offset transform for object \"%s\":\r\n",pObject->maxParent->GetName()));
               printDump(PDObjectOffsets,avar("    scale:            x=%3.5f, y=%3.5f, z=%3.5f\r\n",parts.k.x,parts.k.y,parts.k.z));
               printDump(PDObjectOffsets,avar("    stretch rotation: x=%3.5f, y=%3.5f, z=%3.5f, w=%3.5f\r\n",parts.u.x,parts.u.y,parts.u.z,parts.u.w));
               printDump(PDObjectOffsets,avar("    translation:      x=%3.5f, y=%3.5f, z=%3.5f\r\n",parts.t.x,parts.t.y,parts.t.z));
               printDump(PDObjectOffsets,avar("    actual rotation:  x=%3.5f, y=%3.5f, z=%3.5f, w=%3.5f\r\n",parts.q.x,parts.q.y,parts.q.z,parts.q.w));
               if (parts.f<0)
                   printDump(PDObjectOffsets,     "    ---determinant negative---\r\n");
           }
        }

        // if object not in shape, skip it
        if (!pObject->pValidDetails || pObject->isBone)
        {
            // don't need it, don't want it
            delete pObject;
            objectList.erase(i);
            i--;
            continue;
        }

        Vector<S32> * pValidDetails = pObject->pValidDetails;
        pShape->objects.increment();
        TSShape::Object & tsobj = pShape->objects.last();
        tsobj.nameIndex = addName(pObject->name,pShape);
        tsobj.numMeshes = pValidDetails->size();
        tsobj.startMeshIndex = pShape->meshes.size();
        tsobj.nodeIndex = pObject->tsNodeIndex;

        // is this the first object for this subshape...
        if (pShape->subShapeFirstObject[pObject->subtreeNum] == -1)
            pShape->subShapeFirstObject[pObject->subtreeNum] = pShape->objects.size()-1;

        S32 k,prevk = -1;
        for (j=0; j<pObject->numDetails; j++)
        {
            for (k=0; k<pValidDetails->size(); k++)
                if ((*pValidDetails)[k]==pObject->details[j].size)
                    break;
            if (k==pValidDetails->size() && !allowUnusedMeshes)
            {
                // ooh, this mesh is an invalid detail size
                setExportError(avar("Mesh \"%s\" was found with invalid detail (%i)",pObject->name,pObject->details[j].size));
                return;
            }

            // if this is an invalid detail size get rid of it here
            if (k==pValidDetails->size())
            {
               delete pObject->details[j].mesh;
               for (k=j;k+1<pObject->numDetails;k++)
                  pObject->details[k]=pObject->details[k+1];
               pObject->numDetails--;
               j--;
               continue;
            }

            // add NULL meshes for all the unused detail levels
            for (S32 l=prevk+1; l<k; l++)
            {
                pShape->meshes.increment();
                pShape->meshes.last() = NULL; // no mesh
            }
            prevk=k;

            // fill in some data for later use
            pObject->details[j].mesh->meshNum = pShape->meshes.size();
            if (pObject->details[j].mesh->skinMimic)
               pObject->details[j].mesh->skinMimic->meshNum = pShape->meshes.size();

            // now hook up this mesh...
            pShape->meshes.increment();
            if (pObject->details[j].mesh->sortedObject)
               pShape->meshes.last() = new TSSortedMesh;
            else if (pObject->details[j].mesh->skinMimic)
               pShape->meshes.last() = new TSSkinMesh;
            else
               pShape->meshes.last() = new TSMesh;
            pShape->meshes.last()->numFrames = 0;
            pShape->meshes.last()->numMatFrames = 0;
            if (pObject->details[j].mesh->billboard)
            {
               pShape->meshes.last()->setFlags(TSMesh::Billboard);
               if (SceneEnumProc::isBillboardZAxis(pObject->details[j].mesh->pNode))
                  pShape->meshes.last()->setFlags(TSMesh::BillboardZAxis);
            }
            pObject->details[j].mesh->tsMesh = pShape->meshes.last();
        }
        // may have rid ourselves of all the meshes above...
        // if so, delete this object and continue
        if (pObject->numDetails==0)
        {
            delete pObject;
            objectList.erase(i);
            pShape->objects.decrement();
            i--;
            continue;
        }
        // for any remaining null meshes, decrement object count
        for (j=prevk+1; j<pValidDetails->size(); j++)
            tsobj.numMeshes--;
    }

    // some subtrees may not have objects on them...
    // make sure subShapeFirstObject array is valid
    S32 prev = pShape->objects.size();
    for (i=subtrees.size()-1; i>=0; i--)
    {
        if (pShape->subShapeFirstObject[i] == -1)
            pShape->subShapeFirstObject[i] = prev;
        prev = pShape->subShapeFirstObject[i];
    }

    // now that the address of all the tsobjects are
    // set (since no longer incrementing objects vector)
    // set up pointers back to these objects from the
    // ShapeMimic versions...
    S32 tsObjIndex = 0;
    for (i=0; i<objectList.size(); i++)
    {
        ObjectMimic * obj = objectList[i];

        // if object not in shape, skip it as we did above
        if (!obj->pValidDetails)
            obj->tsObject = NULL;
        else
        {
            obj->tsObjectIndex = tsObjIndex;
            obj->tsObject = &pShape->objects[tsObjIndex++];
        }
    }
}

//--------------------------------------------
// generate decals -- called by generateShape
void ShapeMimic::generateDecals(TSShape * pShape)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   // We have already called generateObjects, so all objects
   // and meshes in objectList are in the shape.

   // put decal objects in order
   setDecalObjectPriorities(decalObjectList);
   sortDecalObjectList(decalObjectList);

   S32 i,j;

   // initialize array that indexes first object in subshape
   for (i=0; i<subtrees.size(); i++)
   {
      pShape->subShapeFirstDecal.increment();
      pShape->subShapeFirstDecal.last() = -1;
   }

   // loop through decal objects and generate decalObjects and decalMeshes on shape
   for (i=0; i<decalObjectList.size(); i++)
   {
      DecalObjectMimic * dom = decalObjectList[i];

      S32 size;
      char * name = chopTrailingNumber(dom->decalNode->GetName()+7,size);

      // add decal object to shape
      pShape->decals.increment();
      TSShape::Decal & decal = pShape->decals.last();
      decal.nameIndex = addName(name,pShape);
      decal.objectIndex = dom->targetObject->tsObjectIndex;
      decal.numMeshes = dom->targetObject->numDetails;
      decal.startMeshIndex = pShape->meshes.size();

      dom->numDetails = decal.numMeshes;

      // is this the first decal for this subshape...
      if (pShape->subShapeFirstDecal[dom->subtreeNum] == -1)
         pShape->subShapeFirstDecal[dom->subtreeNum] = pShape->decals.size()-1;

      for (j=0; j<decal.numMeshes; j++)
      {
         MeshMimic * meshMimic = dom->targetObject->details[j].mesh;
         if (size>0 && dom->targetObject->details[j].size < size)
            meshMimic = NULL; // don't want a decal on this detail level
         DecalMeshMimic * dmm = meshMimic ? new DecalMeshMimic(meshMimic) : NULL;
         dom->details[j].decalMesh = dmm;
         if (!dmm)
         {
            pShape->meshes.push_back(NULL);
            continue;
         }

         // add decalMesh to shape
         pShape->meshes.push_back((TSMesh*)new TSDecalMesh);
         dmm->tsMesh = (TSDecalMesh*)pShape->meshes.last();
      }

      dFree(name);
   }

   // some subtrees may not have decals on them...
   // make sure subShapeFirstDecal array is valid
   S32 prev = pShape->decals.size();
   for (i=subtrees.size()-1; i>=0; i--)
   {
      if (pShape->subShapeFirstDecal[i] == -1)
         pShape->subShapeFirstDecal[i] = prev;
      prev = pShape->subShapeFirstDecal[i];
   }

   // point DecalMimic's to the tsDecalObject (do it now rather than earlier since
   // objects could move in memory before now)
   S32 tsDecalIndex = 0;
   for (i=0; i<decalObjectList.size(); i++)
   {
      DecalObjectMimic * decal = decalObjectList[i];
      decal->tsDecal = &pShape->decals[tsDecalIndex++];
   }
}

//--------------------------------------------
// Set time to zero and generate default states
void ShapeMimic::generateDefaultStates(TSShape * pShape)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   U32 i;

   printDump(PDObjectStates,"\r\nAdd default object states...\r\n\r\n");

   // visit all the objects in order
   for (i=0; i<objectList.size(); i++)
   {
      ObjectMimic * obj = objectList[i];

      // if object not in shape, skip it
      if (!obj->pValidDetails)
         continue;

      generateObjectState(obj,DEFAULT_TIME,pShape,true,true);

      generateMergeIndices(obj);
   }

   printDump(PDNodeStates,"\r\nAdd default node states...\r\n\r\n");

   // iterate through the nodes
   for (i=0; i<nodes.size(); i++)
   {
      NodeMimic * curNode = nodes[i];
      Quat16 rot;
      Point3F trans;
      Quat16 srot; // won't matter
      Point3F scale; // should be uniform
      generateNodeTransform(curNode,DEFAULT_TIME,pShape,false,0,rot,trans,srot,scale);
      addNodeRotation(curNode,DEFAULT_TIME,pShape,false,rot,true);
      addNodeTranslation(curNode,DEFAULT_TIME,pShape,false,trans,true);
      if (mFabs(scale.x*scale.x-1.0f)>0.01f || mFabs(scale.y*scale.y-1.0f)>0.01f || mFabs(scale.z*scale.z-1.0f)>0.01f)
      {
         setExportError("Assertion failed: scale on default transform");
         return;
      }
   }

   if (decalObjectList.size())
   {
      printDump(PDNodeStates,"\r\nAdd default decal states...\r\n\r\n");

      // iterate through the decals
      for (i=0; i<decalObjectList.size(); i++)
      {
         DecalObjectMimic * dom = decalObjectList[i];
         generateDecalState(dom,DEFAULT_TIME,pShape,false);
      }
   }
}

//--------------------------------------------
// generate an object state at specific time
void ShapeMimic::generateObjectState(ObjectMimic * om, S32 time, TSShape * pShape, bool addFrame, bool addMatFrame)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   printDump(PDObjectStates,avar("Adding object state to %i detail level(s) of mesh \"%s\".\r\n",om->numDetails,om->name));
   if (addFrame)
      printDump(PDObjectStates,"Adding frame.\r\n");

   pShape->objectStates.increment();
   TSShape::ObjectState & os = pShape->objectStates.last();

   os.frameIndex = 0;
   os.matFrameIndex = 0;
   os.vis = om->maxParent ? getVisValue(om->maxParent,time) : 1.0f; // might be NULL if we're a skin
   if (os.vis < 0.0f)
      os.vis = 0.0f;
   else if (os.vis > 1.0f)
      os.vis = 1.0f;

   if (os.vis < 0.01f || os.vis > 0.99f)
      printDump(PDObjectStateDetails,avar("Object is%svisible.\r\n",os.vis>0.5f ? " " : " not "));
   else
      printDump(PDObjectStateDetails,avar("Object visibility = %5.3f out of 1.\r\n",os.vis));

   if (addFrame || addMatFrame)
   {
      generateFrame(om,time,addFrame,addMatFrame);

      // must have highest detail level...
      if (!om->details[0].mesh->tsMesh)
      {
         setExportError(avar("Missing highest detail level on mesh \"%s\".",om->name));
         return;
      }

      // set the frame number for the object state
      os.frameIndex = om->details[0].mesh->tsMesh->numFrames - 1;
      os.matFrameIndex = om->details[0].mesh->tsMesh->numMatFrames - 1;
      if (os.frameIndex<0)
         os.frameIndex=0;
      if (os.matFrameIndex<0)
         os.matFrameIndex=0;
   }

   // all added, add separator to dump file...
   printDump(PDObjectStates|PDObjectStateDetails,"---------------------------------\r\n");
}

//--------------------------------------------
// generate a mesh frame at specific time -- put it into the shape
void ShapeMimic::generateFrame(ObjectMimic * om, S32 time, bool addFrame, bool addMatFrame)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   if (om->isBone)
   {
      setExportError("Assertion failed: bone should no longer be on node");
      return;
   }
   if (om->isSkin)
      // don't generate frame
      return;

   S32 i,start,dl;
   for (dl=0; dl<om->numDetails; dl++)
   {
      TSMesh * tsMesh  = om->details[dl].mesh->tsMesh;
      INode * meshNode = om->details[dl].mesh->pNode;
      F32 multiResPercent = om->details[dl].mesh->multiResPercent;

      // compute object offset -- if we're the node in the tree then use object offset,
      // otherwise, put ourselves in that nodes space (at time 0)
      // NOTE: VERY important to do this before getting the mesh...o.w. morph
      //       animation doesn't work (weird...)
      Matrix3 objectOffset;
      if (om->inTreeNode==meshNode)
         objectOffset = om->objectOffset;
      else
      {
         Matrix3 mat = om->inTreeNode->GetNodeTM(DEFAULT_TIME);
         zapScale(mat);
         mat = Inverse(mat);

         objectOffset = meshNode->GetObjTMAfterWSM(DEFAULT_TIME);
         objectOffset *= mat;

         // check to see if in tree version's parent was deleted
         // if so, adjust objectOffset -- see generateObjects for
         // similar code and an explanation
         if (om->maxParent != om->maxTSParent)
         {
            Matrix3 m1,m2;
            m1 = om->maxParent->GetNodeTM(DEFAULT_TIME);
            zapScale(m1);
            m2 = om->maxTSParent->GetNodeTM(DEFAULT_TIME);
            zapScale(m2);
            m2 = Inverse(m2);
            m1 *= m2;
            objectOffset *= m1;
         }
      }

      bool delTri;
      S32 multiResVerts = getMultiResVerts(meshNode,multiResPercent);
      TriObject * tri = getTriObject(meshNode,time,multiResVerts,delTri);
      Mesh & maxMesh = tri->mesh;

      Vector<TSDrawPrimitive> faces;
      Vector<Point3F> normals;
      Vector<Point3> verts;
      Vector<Point3> tverts;
      Vector<U16> indices;
      Vector<U32> smooth;
      Vector<U32> vertId;

      generateFaces(meshNode,maxMesh,objectOffset,faces,normals,verts,tverts,indices,smooth,&vertId);

      if (tsMesh->numFrames==0)
      {
         // first frame, copy faces into mesh
         tsMesh->primitives     = faces;
         tsMesh->indices        = indices;
         tsMesh->vertsPerFrame  =  verts.size();
         om->details[dl].mesh->numVerts = maxMesh.getNumVerts();
         om->details[dl].mesh->smoothingGroups = smooth;
         om->details[dl].mesh->vertId = vertId;
         om->details[dl].mesh->objectOffset = objectOffset; // keep around for generateObjects for convenience
      }
      else
      {
         // not first frame, make sure topology of this frame is same as first frame
         // NOTE:  if multi-frame then can't be multi-res...
         bool error = false;
         if (vertId.size()==om->details[dl].mesh->vertId.size())
         {
            for (i=0;i<vertId.size();i++)
               if (vertId[i]!=om->details[dl].mesh->vertId[i])
                  break;
            if (i!=vertId.size())
               error=true;
         }
         else
            error=true;
         for (i=0; i<faces.size(); i++)
         {
            if (tsMesh->primitives.size() != faces.size())
               // only need to check this once really, but code simpler this way
               break;
            if (tsMesh->indices.size() != indices.size())
               // different number of indices/verts -- should hit prior break in that case...
               break;
            // make sure tsMesh->face[i] is same as face[i]
            if (tsMesh->primitives[i].start!=faces[i].start ||
                tsMesh->primitives[i].matIndex!=faces[i].matIndex)
               break;
         }
         if (i!=faces.size() || error)
         {
            setExportError(avar("Mesh topology is animated on mesh \"%s\".",meshNode->GetName()));
            return;
         }
      }

      if (addFrame)
      {
         // copy normals...
         start = tsMesh->norms.size();
         tsMesh->norms.setSize(start+normals.size());
         for (i=0; i<normals.size(); i++)
            tsMesh->norms[i+start] = normals[i];
         // copy verts...
         start = tsMesh->verts.size();
         tsMesh->verts.setSize(start+verts.size());
         for (i=0; i<verts.size(); i++)
            Point3ToPoint3F(verts[i],tsMesh->verts[i+start]);

         tsMesh->numFrames++;
      }

      if (addMatFrame)
      {
         // copy tverts...
         start = tsMesh->tverts.size();
         tsMesh->tverts.setSize(start+tverts.size());
         for (i=0; i<tverts.size(); i++)
         {
            tsMesh->tverts[i+start].x = tverts[i].x;
            tsMesh->tverts[i+start].y = tverts[i].y;
         }

         tsMesh->numMatFrames++;
      }

      if (delTri)
         delete tri;
   }
}

//--------------------------------------------
// generate merge indices (after generating default state
void ShapeMimic::generateMergeIndices(ObjectMimic * om)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   if (om->isBone)
   {
      setExportError("Assertion failed: bone should no longer be on node");
      return;
   }
   if (om->isSkin)
      // don't generate frame
      return;

   S32 i,k,dl;
   for (dl=0; dl<om->numDetails; dl++)
   {
      if (!om->details[dl].mesh)
         continue;

      TSMesh * tsMesh  = om->details[dl].mesh->tsMesh;

      // how many verts in the next smallest version of us
      S32 numChildVerts = 0;
      k = 1;
      while (dl+k<om->numDetails)
      {
         if (om->details[dl+k].mesh)
         {
            numChildVerts = om->details[dl+k].mesh->numVerts;
            break;
         }
         k++;
      }

      findMergeIndices(om->details[dl].mesh,
                       om->details[dl].mesh->objectOffset,
                       tsMesh->primitives,
                       tsMesh->verts,
                       tsMesh->norms,
                       tsMesh->tverts,
                       tsMesh->indices,
                       tsMesh->mergeIndices,
                       om->details[dl].mesh->smoothingGroups,
                       om->details[dl].mesh->vertId,
                       numChildVerts);
      tsMesh->vertsPerFrame = tsMesh->verts.size();
   }
}

//--------------------------------------------
// generate list of mesh faces -- called for each frame of
// mesh animation and checked against prior versions...
void ShapeMimic::generateFaces(INode * meshNode, Mesh & maxMesh, Matrix3 & objectOffset,
                               Vector<TSDrawPrimitive> & faces, Vector<Point3F> & normals,
                               Vector<Point3> & verts, Vector<Point3> & tverts, Vector<U16> & indices,
                               Vector<U32> & smooth, Vector<U32> * vertId)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   // are we mirrored?
   AffineParts parts;
   decomp_affine(objectOffset,&parts);
   bool mirror = parts.f < 0.0f;

   S32 i,j;
   Vector<CropInfo> cropInfoList;
   Vector<U32> vertIdStore;

   // start lists empty
   verts.clear();
   tverts.clear();
   normals.clear();
   indices.clear();
   smooth.clear();
   if (!vertId)
      vertId = &vertIdStore;
   vertId->clear();

   // start out with faces and crop data allocated
   faces.setSize(maxMesh.getNumFaces());
   cropInfoList.setSize(maxMesh.getNumFaces());

   // if no faces, exit here without error
   if (!maxMesh.getNumFaces())
      return;

   // get faces, points & materials
   for (i=0; i<faces.size();i++)
   {
      Face & maxFace = maxMesh.faces[i];
      TVFace & maxTVFace = maxMesh.mapFaces(1)[i]; // 1 == default map
      TSDrawPrimitive & tsFace = faces[i];

      CropInfo & cropInfo = cropInfoList[i];

      // set faces material index
      tsFace.matIndex = addMaterial(meshNode,maxFace.getMatID(),&cropInfo);
      tsFace.start = indices.size();
      tsFace.numElements = 3;
      tsFace.matIndex |= TSDrawPrimitive::Triangles|TSDrawPrimitive::Indexed;

      // set vertex indices
      S32 idx0 = maxFace.v[0];
      S32 idx1 = maxFace.v[2]; // switch the order to be CW
      S32 idx2 = maxFace.v[1]; // switch the order to be CW
      if (mirror)
      {
         S32 tmp = idx1;
         idx1 = idx2;
         idx2 = tmp;
      }
      verts.push_back(maxMesh.verts[idx0] * objectOffset);
      verts.push_back(maxMesh.verts[idx1] * objectOffset);
      verts.push_back(maxMesh.verts[idx2] * objectOffset);

      // add smoothing group for vertices
      smooth.push_back(maxFace.smGroup);
      smooth.push_back(maxFace.smGroup);
      smooth.push_back(maxFace.smGroup);

      // add vertex index to uniquely identify verts as max sees them
      // this will allow us to never collapse a vert that max doesn't collapse
      if (vertId)
      {
         vertId->push_back(idx0);
         vertId->push_back(idx1);
         vertId->push_back(idx2);
      }

      // set texture vertex indices
      if (cropInfo.hasTVerts)
      {
         // if no tverts...get out now
         if (!maxMesh.tVerts)
         {
            setExportError(avar("No texture verts on mesh \"%s\"",meshNode->GetName()));
            return;
         }

         idx0 = maxTVFace.getTVert(0);
         idx1 = maxTVFace.getTVert(2); // switch the order to be CW
         idx2 = maxTVFace.getTVert(1); // switch the order to be CW
         if (mirror)
         {
            S32 tmp = idx1;
            idx1 = idx2;
            idx2 = tmp;
         }
         tverts.push_back(maxMesh.mapVerts(1)[idx0] * cropInfo.uvTransform);
         tverts.push_back(maxMesh.mapVerts(1)[idx1] * cropInfo.uvTransform);
         tverts.push_back(maxMesh.mapVerts(1)[idx2] * cropInfo.uvTransform);
      }
      else
      {
         tverts.push_back(Point3(0,0,0));
         tverts.push_back(Point3(0,0,0));
         tverts.push_back(Point3(0,0,0));
      }

      // now add indices...this is easy right now...later we'll mess this up
      indices.push_back(indices.size());
      indices.push_back(indices.size());
      indices.push_back(indices.size());
   }

   // duplicate 2-sided faces
   S32 sz = faces.size();
   for (i=0; i<sz; i++)
   {
      if (!cropInfoList[i].twoSided)
         continue;

      faces.increment();
      TSDrawPrimitive & frontSide = faces[i];
      TSDrawPrimitive & backSide = faces.last();
      backSide.matIndex = frontSide.matIndex;
      backSide.numElements = frontSide.numElements;
      backSide.start = indices.size();

      // add vert indices (but flip order since we're the other side)
      U32 idx0 = indices[frontSide.start+0];
      U32 idx1 = indices[frontSide.start+1];
      U32 idx2 = indices[frontSide.start+2];
      indices.push_back(idx0);
      indices.push_back(idx2);
      indices.push_back(idx1);

      cropInfoList.increment();
      cropInfoList.last() = cropInfoList[i];
   }

   bool anySplitting = false;

   printDump(PDObjectStates,avar("%i faces, %i verts, %i tverts before cropping textures and joining verts\r\n",
      faces.size(),verts.size(),tverts.size()));

   // flip y (v) coord on all texture coords -- you'd think this should be done
   // after cropping ... well it shouldn't be, trust me.
   for (i=0; i<tverts.size(); i++)
      tverts[i].y = 1.0f - tverts[i].y;

   // the following vectors will be tweaked as we crop...new texture verts created
   // as we account for wrapping of cropped textures will have to be flipped later
   // down the line...
   Vector<bool> flipX, flipY;
   flipX.setSize(tverts.size());
   flipY.setSize(tverts.size());
   for (i=0;i<tverts.size();i++)
      flipX[i] = flipY[i] = false;
   S32 startTV = tverts.size();

   // if we crop or place a texture, we might have to split up some polys if they go off texture
   // loop through and apply cropping/placement to faces, splitting as necessary...
   S32 last1 = -1;
   S32 last2 = -1;
   S32 last3 = -1;
   S32 last4 = -1;
   for (i=0; i<faces.size();i++)
   {
      CropInfo & cropInfo = cropInfoList[i];
      if (!cropInfo.crop)
         continue;

      // cropping ...
      Point3 & tv0 = tverts[indices[faces[i].start]];
      Point3 & tv1 = tverts[indices[faces[i].start+1]];
      Point3 & tv2 = tverts[indices[faces[i].start+2]];

      // shall we split...
      TSDrawPrimitive faceA,faceB;
      if (cropInfo.cropLeft && (tv0.x < 0.0f || tv1.x < 0.0f || tv2.x < 0.0f) && (tv0.x > 0.0f || tv1.x > 0.0f || tv2.x > 0.0f))
      {
         printDump(PDObjectStates,"Splitting face (U coord) due to cropping -- adding 2 faces.\r\n");
         if (i==last1)
         {
            // should never split the same face twice in same spot, precision error?
            setExportError("Re-splitting face -- get programmer (1)");
            return;
         }
         anySplitting = true;
         splitFaceX(faces[i],faceA,faceB,verts,tverts,indices,flipX,flipY,0.0f,smooth,vertId);
         faces.push_back(faceA);
         faces.push_back(faceB);
         CropInfo ci = cropInfo; // need to copy because cropInfo is on the list...
         cropInfoList.push_back(ci);
         cropInfoList.push_back(ci);
         last1=i;
         i--;
         continue;
      }
      if (cropInfo.cropRight && (tv0.x > 1.0f || tv1.x > 1.0f || tv2.x > 1.0f) && (tv0.x < 1.0f || tv1.x < 1.0f || tv2.x < 1.0f))
      {
         printDump(PDObjectStates,"Splitting face (U coord) due to cropping -- adding 2 faces.\r\n");
         if (i==last2)
         {
            // should never split the same face twice in same spot, precision error?
            setExportError("Re-splitting face -- get programmer (2)");
            return;
         }
         anySplitting = true;
         splitFaceX(faces[i],faceA,faceB,verts,tverts,indices,flipX,flipY,1.0f,smooth,vertId);
         faces.push_back(faceA);
         faces.push_back(faceB);
         CropInfo ci = cropInfo; // need to copy because cropInfo is on the list...
         cropInfoList.push_back(ci);
         cropInfoList.push_back(ci);
         last2=i;
         i--;
         continue;
      }
      if (cropInfo.cropTop && (tv0.y < 0.0f || tv1.y < 0.0f || tv2.y < 0.0f) && (tv0.y > 0.0f || tv1.y > 0.0f || tv2.y > 0.0f))
      {
         printDump(PDObjectStates,"Splitting face (V coord) due to cropping -- adding 2 faces.\r\n");
         if (i==last3)
         {
            // should never split the same face twice in same spot, precision error?
            setExportError("Re-splitting face -- get programmer (3)");
            return;
         }
         anySplitting = true;
         splitFaceY(faces[i],faceA,faceB,verts,tverts,indices,flipX,flipY,0.0f,smooth,vertId);
         faces.push_back(faceA);
         faces.push_back(faceB);
         CropInfo ci = cropInfo; // need to copy because cropInfo is on the list...
         cropInfoList.push_back(ci);
         cropInfoList.push_back(ci);
         last3=i;
         i--;
         continue;
      }
      if (cropInfo.cropBottom && (tv0.y > 1.0f || tv1.y > 1.0f || tv2.y > 1.0f) && (tv0.y < 1.0f || tv1.y < 1.0f || tv2.y < 1.0f))
      {
         printDump(PDObjectStates,"Splitting face (V coord) due to cropping -- adding 2 faces.\r\n");
         if (i==last4)
         {
            // should never split the same face twice in same spot, precision error?
            setExportError("Re-splitting face -- get programmer (4)");
            return;
         }
         anySplitting = true;
         splitFaceY(faces[i],faceA,faceB,verts,tverts,indices,flipX,flipY,1.0f,smooth,vertId);
         faces.push_back(faceA);
         faces.push_back(faceB);
         CropInfo ci = cropInfo; // need to copy because cropInfo is on the list...
         cropInfoList.push_back(ci);
         cropInfoList.push_back(ci);
         last4=i;
         i--;
         continue;
      }
   }

   if (anySplitting)
   {
      printDump(PDObjectStates,avar("%i faces, %i verts, %i tverts after cropping but before joining verts\r\n",
         faces.size(),verts.size(),tverts.size()));

      // flip verts?
      for (i=startTV;i<tverts.size();i++)
      {
         if (flipX[i])
            tverts[i].x = 1.0f - tverts[i].x;
         if (flipY[i])
            tverts[i].y = 1.0f - tverts[i].y;
      }
   }

   // get rid of faces that don't wrap and are off tile
   S32 offTileRemovals = 0;
   for (i=0; i<faces.size(); i++)
   {
      CropInfo & cropInfo = cropInfoList[i];
      if (!cropInfo.crop || !cullOffTile)
         continue;
      if (!cropInfo.uWrap)
      {
         S32 start = faces[i].start;
         S32 idx0 = indices[start+0];
         S32 idx1 = indices[start+1];
         S32 idx2 = indices[start+2];
         if ( (tverts[idx0].x <= 0.0f && tverts[idx1].x <= 0.0f && tverts[idx2].x <= 0.0f) ||
              (tverts[idx0].x >= 1.0f && tverts[idx1].x >= 1.0f && tverts[idx2].x >= 1.0f))
         {
            // get rid of this face
            for (j=0; j<faces.size(); j++)
               if (faces[j].start>start)
                  faces[j].start -= 3;
            faces.erase(i);
            cropInfoList.erase(i);
            indices.erase(start);
            indices.erase(start);
            indices.erase(start);
            i--;
            offTileRemovals++;
            continue;
         }
      }
      if (!cropInfo.vWrap)
      {
         S32 start = faces[i].start;
         S32 idx0 = indices[start+0];
         S32 idx1 = indices[start+1];
         S32 idx2 = indices[start+2];
         if ( (tverts[idx0].y <= 0.0f && tverts[idx1].y <= 0.0f && tverts[idx2].y <= 0.0f) ||
              (tverts[idx0].y >= 1.0f && tverts[idx1].y >= 1.0f && tverts[idx2].y >= 1.0f))
         {
            // get rid of this face
            for (j=0; j<faces.size(); j++)
               if (faces[j].start>start)
                  faces[j].start -= 3;
            faces.erase(i);
            cropInfoList.erase(i);
            indices.erase(start);
            indices.erase(start);
            indices.erase(start);
            i--;
            offTileRemovals++;
            continue;
         }
      }
   }
   if (offTileRemovals)
   {
      // we removed some faces that were off-tile...remove unused verts too
      bool * vertUsed = new bool[verts.size()];
      for (i=0; i<verts.size(); i++)
         vertUsed[i]=true;
      for (i=0; i<indices.size(); i++)
      {
         if (indices[i]<0||indices[i]>=verts.size())
         {
            setExportError("Assertion failed");
            return;
         }
         vertUsed[indices[i]] = true;
      }
      for (i=verts.size()-1; i>=0; i--)
      {
         if (!vertUsed[i])
         {
            for (j=0; j<indices.size(); j++)
               if (indices[j]>i)
                  indices[j]--;
            verts.erase(i);
            tverts.erase(i);
            smooth.erase(i);
            if (vertId)
               vertId->erase(i);
         }
      }
      delete [] vertUsed;

      printDump(PDObjectStates,avar("Removing off-tile faces, left with %i faces and %i verts\r\n",faces.size(),verts.size()));
   }

   // now do the actual cropping/placing
   Vector<bool> done;
   done.setSize(tverts.size());
   for (i=0;i<done.size();i++)
      done[i]=false;
   for (i=0;i<faces.size();i++)
   {
      CropInfo & cropInfo = cropInfoList[i];
      if (!cropInfo.crop && !cropInfo.place)
         continue;

      S32 idx0 = indices[faces[i].start+0];
      S32 idx1 = indices[faces[i].start+1];
      S32 idx2 = indices[faces[i].start+2];

      if (!done[idx0])
         handleCropAndPlace(tverts[idx0],cropInfo);
      if (!done[idx1])
         handleCropAndPlace(tverts[idx1],cropInfo);
      if (!done[idx2])
         handleCropAndPlace(tverts[idx2],cropInfo);

      done[idx0] = done[idx1] = done[idx2] = true;
   }

   // generate normals
   computeNormals(faces,indices,verts,normals,smooth,verts.size(),1);
   // now sort by material index
   sortFaceList(faces);
}

S32 ShapeMimic::getMultiResVerts(INode * meshNode, F32 multiResPercent)
{
   MultiResMimic * multiResMimic = getMultiRes(meshNode);
   if (!multiResMimic || multiResPercent<0.0f)
      return -1;

   return multiResMimic->totalVerts * multiResPercent;
}

//--------------------------------------------
// generate a node state at specific time -- put state into shape
void ShapeMimic::addNodeRotation(NodeMimic * curNode, S32 time, TSShape * pShape, bool blend, Quat16 & rot, bool defaultVal)
{
    // if already encountered an error, then
    // we'll just go through the motions
    if (isError()) return;

    printDump(PDNodeStates,avar("Adding%snode rotation at time %i for node \"%s\".\r\n",
        blend ? " blend " : " ", time, curNode->maxNode->GetName()));

    if (!defaultVal)
    {
       pShape->nodeRotations.increment();
       pShape->nodeRotations.last() = rot;
    }
    else
    {
       pShape->defaultRotations.increment();
       pShape->defaultRotations.last() = rot;
    }
    QuatF q;
    rot.getQuatF(&q);
    printDump(PDNodeStateDetails,avar("  rotation:     x=%3.5f, y=%3.5f, z=%3.5f, w=%3.5f\r\n",q.x,q.y,q.z,q.w));

    // all added, add separator to dump file...
    printDump(PDNodeStates|PDNodeStateDetails,"---------------------------------\r\n");
}

void ShapeMimic::addNodeTranslation(NodeMimic * curNode, S32 time, TSShape * pShape, bool blend, Point3F & trans, bool defaultVal)
{
    // if already encountered an error, then
    // we'll just go through the motions
    if (isError()) return;

    printDump(PDNodeStates,avar("Adding%snode translation at time %i for node \"%s\".\r\n",
        blend ? " blend " : " ", time, curNode->maxNode->GetName()));

    if (!defaultVal)
    {
       pShape->nodeTranslations.increment();
       pShape->nodeTranslations.last() = trans;
    }
    else
    {
       pShape->defaultTranslations.increment();
       pShape->defaultTranslations.last() = trans;
    }

    printDump(PDNodeStateDetails,avar("  translation:     x=%3.5f, y=%3.5f, z=%3.5f\r\n",trans.x,trans.y,trans.z));

    // all added, add separator to dump file...
    printDump(PDNodeStates|PDNodeStateDetails,"---------------------------------\r\n");
}

void ShapeMimic::addNodeUniformScale(NodeMimic * curNode, S32 time, TSShape * pShape, bool blend, F32 scale)
{
    // if already encountered an error, then
    // we'll just go through the motions
    if (isError()) return;

    printDump(PDNodeStates,avar("Adding%snode scale at time %i for node \"%s\".\r\n",
        blend ? " blend " : " ", time, curNode->maxNode->GetName()));

    pShape->nodeUniformScales.increment();
    pShape->nodeUniformScales.last() = scale;
    printDump(PDNodeStateDetails,avar("  uniform scale:     %3.5f\r\n",scale));

    // all added, add separator to dump file...
    printDump(PDNodeStates|PDNodeStateDetails,"---------------------------------\r\n");
}

void ShapeMimic::addNodeAlignedScale(NodeMimic * curNode, S32 time, TSShape * pShape, bool blend, Point3F & scale)
{
    // if already encountered an error, then
    // we'll just go through the motions
    if (isError()) return;

    printDump(PDNodeStates,avar("Adding%snode scale at time %i for node \"%s\".\r\n",
        blend ? " blend " : " ", time, curNode->maxNode->GetName()));

    pShape->nodeAlignedScales.increment();
    pShape->nodeAlignedScales.last() = scale;
    printDump(PDNodeStateDetails,avar("  aligned scale:     x=%3.5f, y=%3.5f, z=%3.5f\r\n",scale.x,scale.y,scale.z));

    // all added, add separator to dump file...
    printDump(PDNodeStates|PDNodeStateDetails,"---------------------------------\r\n");
}

void ShapeMimic::addNodeArbitraryScale(NodeMimic * curNode, S32 time, TSShape * pShape, bool blend, Quat16 & qrot, Point3F & scale)
{
    // if already encountered an error, then
    // we'll just go through the motions
    if (isError()) return;

    printDump(PDNodeStates,avar("Adding%snode scale at time %i for node \"%s\".\r\n",
        blend ? " blend " : " ", time, curNode->maxNode->GetName()));

    pShape->nodeArbitraryScaleRots.increment();
    pShape->nodeArbitraryScaleFactors.increment();
    pShape->nodeArbitraryScaleRots.last() = qrot;
    pShape->nodeArbitraryScaleFactors.last() = scale;
    QuatF q;
    qrot.getQuatF(&q);
    printDump(PDNodeStateDetails,avar("  arbitrary scale rot:     x=%3.5f, y=%3.5f, z=%3.5f, w=%3.5f\r\n",q.x,q.y,q.z,q.w));
    printDump(PDNodeStateDetails,avar("  arbitrary scale factor:  x=%3.5f, y=%3.5f, z=%3.5f\r\n",scale.x,scale.y,scale.z));

    // all added, add separator to dump file...
    printDump(PDNodeStates|PDNodeStateDetails,"---------------------------------\r\n");
}

//--------------------------------------------
// generate a node transform at specific time
void ShapeMimic::generateNodeTransform(NodeMimic * curNode,S32 time,TSShape * pShape,
                                       bool blend, S32 blendReferenceTime,
                                       Quat16 & rot, Point3F & trans, Quat16 & qrot, Point3F & scale)
{
    // if already encountered an error, then
    // we'll just go through the motions
    if (isError()) return;

    if (blend)
        getBlendNodeTransform(curNode->maxNode,curNode->parent->maxNode,curNode->child0,curNode->parent0,time,blendReferenceTime,rot,trans,qrot,scale);
    else
        getLocalNodeTransform(curNode->maxNode,curNode->parent->maxNode,curNode->child0,curNode->parent0,time,rot,trans,qrot,scale);
}

//--------------------------------------------
// generate ifl materials -- called by generateShape
void ShapeMimic::generateIflMaterials(TSShape * pShape)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   // if none to make...
   if (iflList.empty())
      return;

   printDump(PDSequences,avar("\r\nAdding %i ifl materials...\r\n\r\n",iflList.size()));

   S32 i;
   for (i=0; i<iflList.size(); i++)
   {
      pShape->iflMaterials.increment();
      TSShape::IflMaterial & iflMaterial = pShape->iflMaterials.last();

      iflMaterial.nameIndex = addName(iflList[i]->fileName,pShape);
      iflMaterial.materialSlot = iflList[i]->materialSlot;

      printDump(PDSequences,avar("Adding ifl material \"%s\".\r\n",iflList[i]->fileName));
   }
}

//--------------------------------------------
// generate sequences -- called by generateShape
void ShapeMimic::generateSequences(TSShape * pShape)
{
    // if already encountered an error, then
    // we'll just go through the motions
    if (isError()) return;

    printDump(PDSequences,avar("\r\nAdding %i sequences...\r\n\r\n",sequences.size()));

    for (S32 i=0; i<sequences.size(); i++)
    {
        INode * pNode = sequences[i];
        const char * name = pNode->GetName();
        printDump(PDSequences,avar("Adding sequence %i named \"%s\"\r\n",i,name));

        pShape->sequences.increment();
        TSShape::Sequence & seq = pShape->sequences.last();
        constructInPlace(&seq);

        // we'll need some tools from max...
        Object *obj = pNode->GetObjectRef();
        Interval range = pNode->GetTimeRange(TIMERANGE_ALL          |
                                             TIMERANGE_CHILDNODES   |
                                             TIMERANGE_CHILDANIMS   );
        IParamBlock *pblock = (IParamBlock *)obj->GetReference(0);
        Control * control = pblock->GetController(PB_SEQ_BEGIN_END);
        IKeyControl * ikc = GetKeyControlInterface(control);
        if (ikc && ikc->GetNumKeys()>=2)
        {
           IBezFloatKey key; // hopefully the biggest key user can choose...otherwise we might crash...stupid max.
           ikc->GetKey(0,&key);
           range.SetStart(key.time);
           range.SetEnd(key.time);
           for (S32 j=1; j<ikc->GetNumKeys(); j++)
           {
             ikc->GetKey(j,&key);
             if (key.time > range.End())
               range.SetEnd(key.time);
             else if (key.time < range.Start())
               range.SetStart(key.time);
           }
        }

        // we know 'obj' is either a sequence object for new exporter or
        // a sequence object for the old exporter...find out which
        bool oldSequence = obj->ClassID() == OLD_SEQUENCE_CLASS_ID;

        ExporterSequenceData seqData;

        if (oldSequence)
            setupOldSequence(pblock,range,&seqData);
        else
            setupSequence(pblock,range,&seqData);

        // get the range of this sequence...
        F32 start     = TicksToSec(range.Start());
        F32 end       = TicksToSec(range.End());
        F32 duration;
        F32 delta;
        S32 numFrames;

        getSequenceTiming(seqData,&start,&end,&duration,&delta,&numFrames);

        // fill in some sequence data
        seq.flags = 0;
        if (seqData.cyclic)
         seq.flags |= TSShape::Cyclic;
        if (seqData.blend)
         seq.flags |= TSShape::Blend;
        seq.priority = seqData.priority;
        seq.nameIndex = addName(name,pShape);
        seq.numKeyframes = numFrames;
        seq.duration = seqData.overrideDuration ? seqData.overriddenDuration : duration;
        seq.toolBegin = start;

        // determine which nodes/objects are controlled by this sequence
        S32 rotCount, transCount, uniformScaleCount, alignedScaleCount, arbitraryScaleCount;
        setNodeMembership(pShape,range,seqData, rotCount,transCount,uniformScaleCount,alignedScaleCount,arbitraryScaleCount);
        S32 objectCount = setObjectMembership(seq,range,seqData);
        S32 iflCount = setIflMembership(seq,range,seqData);
        S32 decalCount = setDecalMembership(seq,range,seqData);

        S32 scaleCount = getMax(uniformScaleCount,getMax(alignedScaleCount,arbitraryScaleCount));
        S32 nodeCount = getMax(rotCount,getMax(transCount,scaleCount));
        const char * scaleType;
        if (arbitraryScaleCount)
           scaleType=" (arbitary scale)";
        else if (alignedScaleCount)
           scaleType=" (aligned scale)";
        else if (uniformScaleCount)
           scaleType=" (uniform scale)";
        else
           scaleType="";

        if (isError()) return;

        // supply some dump information
        if (!seqData.cyclic)
            printDump(PDSequences,"One-shot sequence.  ");
        if (seqData.blend)
            printDump(PDSequences,"Blend sequence.  ");
        printDump(PDSequences,avar("Enabled animation: %c%c%c%c%c%c%c%c%c, Forced animation: %c%c%c, Default priority = %i.\r\n",
                  seqData.enableMorph      ? 'M' : ' ',
                  seqData.enableVis        ? 'V' : ' ',
                  seqData.enableTransform  ? 'T' : ' ',
                  seqData.enableUniformScale  ? 'U' : ' ',
                  seqData.enableArbitraryScale ? 'A' : ' ',
                  seqData.enableIFL        ? 'I' : ' ',
                  seqData.enableDecal      ? 'D' : ' ',
                  seqData.enableDecalFrame ? 'F' : ' ',
                  seqData.forceMorph       ? 'M' : ' ',
                  seqData.forceVis         ? 'V' : ' ',
                  seqData.forceTransform   ? 'T' : ' ',
                  seqData.forceScale       ? 'S' : ' ',
                  seqData.priority));
        if (seqData.ignoreGround)
            printDump(PDSequences,"Ignoring ground transform.\r\n");
        printDump(PDSequences,avar("Duration = %3.5f, secPerFrame = %3.5f, # frames = %i\r\n",duration,delta,numFrames));
        printDump(PDSequences,avar("Sequence includes %i nodes, %i objects, %i decals, and %i ifl materials\r\n",nodeCount,objectCount,decalCount,iflCount));
        printDump(PDSequences,avar("  %i rotations, %i translations, %i scales%s\r\n\r\n",rotCount,transCount,scaleCount,scaleType));

        // these methods generate the animation data...each of
        // them operates on the most recently created sequence
        generateNodeAnimation(pShape,range,seqData);
        generateObjectAnimation(pShape,range,seqData);
        generateDecalAnimation(pShape,range,seqData);
        generateGroundAnimation(pShape,range,seqData);
        generateFrameTriggers(pShape,range,seqData,pblock);

        if (testCutNodes(range,seqData))
            return;
    }
}

//--------------------------------------------
// setup object membership-- called by generateSequences
S32 ShapeMimic::setObjectMembership(TSShape::Sequence & seq,
                                    Interval & range,
                                    ExporterSequenceData & seqData)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return 0;

   // clear out all object membership...
   seq.visMatters.clearAll();
   seq.frameMatters.clearAll();
   seq.matFrameMatters.clearAll();

   // force anything?
   if (seqData.forceVis)
      seq.visMatters.setAll(objectList.size());
   if (seqData.forceMorph)
      seq.frameMatters.setAll(objectList.size());
   if (seqData.forceTVert)
      seq.matFrameMatters.setAll(objectList.size());

   // don't do any work we don't have to...
   bool doVis = seqData.enableVis && !seqData.forceVis;
   bool doMorph = seqData.enableMorph && !seqData.forceMorph;
   bool doTVert = seqData.enableTVert && !seqData.forceTVert;

   // add objects to the subsets...
   for (S32 i=0; i<objectList.size(); i++)
   {
      bool error = false;

      // the node to perform membership tests on
      INode * testNode = objectList[i]->maxParent;

      if (objectList[i]->isSkin)
      {
         // in case force was set
         seq.frameMatters.clear(i);
         seq.matFrameMatters.clear(i);
         testNode=NULL;
         for (S32 dl=0; dl<objectList[i]->numDetails; dl++)
            if (objectList[i]->details[dl].mesh)
               testNode = objectList[i]->details[dl].mesh->skinMimic->skinNode;
         if (!testNode)
            continue;
      }

      bool visDiffersFromDefault =
           (mFabs(getVisValue(testNode,DEFAULT_TIME)-getVisValue(testNode,range.Start()))>0.01f);

      if (doVis && (animatesVis(testNode,range,error) || visDiffersFromDefault))
         seq.visMatters.set(i);

      if (doTVert && animatesMatFrame(testNode,range,error))
         seq.matFrameMatters.set(i);

      if (objectList[i]->isSkin)
         continue;

      if (doMorph && animatesFrame(testNode,range,error))
         seq.frameMatters.set(i);

      if (error)
      {
         setExportError("Assertion failed:  Error checking for object animation.");
         break;
      }
   }

   // how many objects are in the set?
   S32 objectCount=0;
   for (S32 j=0; j<objectList.size(); j++)
      if (seq.frameMatters.test(j) || seq.matFrameMatters.test(j) || seq.visMatters.test(j))
         objectCount++;

   return objectCount;
}

//--------------------------------------------
// setup decal membership-- called by generateSequences
S32 ShapeMimic::setDecalMembership(TSShape::Sequence & seq,
                                   Interval & range,
                                   ExporterSequenceData & seqData)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return 0;

   // decide object membership
   seq.decalMatters.clearAll();

   // don't do any work we don't have to...
   if (!seqData.enableDecal)
      return 0;

   // force anything?
   if (seqData.forceDecal)
   {
      seq.decalMatters.setAll(decalObjectList.size());
      return decalObjectList.size();
   }

   // do the work
   S32 count = 0;
   for (S32 i=0; i<decalObjectList.size(); i++)
   {
      for (S32 time = range.Start(); time<=range.End(); time++)
         if (isVis(decalObjectList[i]->decalNode,time))
         {
            // if decal visible at all during the sequence then it is a member...
            seq.decalMatters.set(i);
            count++;
            break;
         }
   }

   return count;
}

//--------------------------------------------
// setup node membership-- called by generateSequences
void ShapeMimic::setNodeMembership(TSShape * pShape,
                                   Interval & range,
                                   ExporterSequenceData & seqData,
                                   S32 & rotCount, S32 & transCount,
                                   S32 & uniformScaleCount, S32 & alignedScaleCount, S32 & arbitraryScaleCount)
{
   // if already encountered an error, then
   // we'll just go through the motions
   rotCount = transCount = uniformScaleCount = alignedScaleCount = arbitraryScaleCount = 0;
   if (isError()) return;

   // get the sequence we're working on ... always working
   // on the last sequence created
   TSShape::Sequence & seq = pShape->sequences.last();

   // decide node membership
   seq.rotationMatters.clearAll();
   seq.translationMatters.clearAll();
   seq.scaleMatters.clearAll();

   if (!seqData.enableTransform && !seqData.enableUniformScale && !seqData.enableArbitraryScale)
      // not animating transforms, so no nodes are members
      return;

   // get the range of this sequence...
   F32 start     = TicksToSec(range.Start());
   F32 end       = TicksToSec(range.End());
   F32 duration;
   F32 delta;
   S32 numFrames;

   // we re-compute this here rather than pass in the data for
   // readability sake only
   getSequenceTiming(seqData,&start,&end,&duration,&delta,&numFrames);

   // this shouldn't be allowed, but check anyway...
   if (numFrames<2)
      return;

   S32 i, frame;

   // clear out the transform caches and set it up for this sequence
   for (i=0; i<nodeRotCache.size(); i++)
      delete [] nodeRotCache[i];
   for (i=0; i<nodeTransCache.size(); i++)
      delete [] nodeTransCache[i];
   for (i=0; i<nodeScaleCache.size(); i++)
      delete [] nodeScaleCache[i];
   nodeRotCache.setSize(nodes.size());
   for (i=0;i<nodes.size();i++)
      nodeRotCache[i] = new Quat16[numFrames];
   nodeTransCache.setSize(nodes.size());
   for (i=0;i<nodes.size();i++)
      nodeTransCache[i] = new Point3F[numFrames];
   nodeScaleCache.setSize(nodes.size());
   for (i=0;i<nodes.size();i++)
      nodeScaleCache[i] = new ScaleStruct[numFrames];

   // get all the node transforms for every frame
   F32 time = start;
   for (frame = 0; frame<numFrames; frame++)
   {
      S32 maxTime = SecToTicks(time); // as in 3d studio max...
      // may go just a tad over/under due to round-off...correct that here
      if (maxTime>range.End())
         maxTime = range.End();
      else if (maxTime<range.Start())
         maxTime = range.Start();

      for (i=0;i<nodes.size();i++)
         generateNodeTransform(nodes[i],maxTime,pShape,seqData.blend,seqData.blendReferenceTime,
                               nodeRotCache[i][frame],nodeTransCache[i][frame],
                               nodeScaleCache[i][frame].mRotate,nodeScaleCache[i][frame].mScale);
      time += delta;
   }

   // test to see if max changes the transform over the interval
   // in order to decide whether to animate the transform in 3space
   // we don't use max's mechanism for doing this because it tests
   // whether world space transform changes, and we care about local
   // transform -- we handle scale a little different too...we have
   // no scale on the default transforms

   rotCount = setRotationMembership(pShape,seq,seqData,numFrames);
   transCount = setTranslationMembership(pShape,seq,seqData,numFrames);
   if (animatesArbitraryScale(seqData,numFrames))
      arbitraryScaleCount = setArbitraryScaleMembership(pShape,seq,seqData,numFrames);
   else if (animatesAlignedScale(seqData,numFrames))
      alignedScaleCount = setAlignedScaleMembership(pShape,seq,seqData,numFrames);
   else
      uniformScaleCount = setUniformScaleMembership(pShape,seq,seqData,numFrames);

   if (arbitraryScaleCount)
      seq.flags |= TSShape::ArbitraryScale;
   else if (alignedScaleCount)
      seq.flags |= TSShape::AlignedScale;
   else if (uniformScaleCount)
      seq.flags |= TSShape::UniformScale;
}

S32 ShapeMimic::setRotationMembership(TSShape * pShape, TSSequence & seq, ExporterSequenceData & seqData, S32 numFrames)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return 0;

   S32 nodeCount = 0;

   if (seqData.forceTransform)
   {
      // all transforms are forced to be members
      seq.rotationMatters.setAll(nodes.size());
      for (S32 i=0; i<nodes.size(); i++)
      {
         if (neverAnimateNode(nodes[i]))
            seq.rotationMatters.clear(i);
         else
            nodeCount++;
      }
      return nodeCount;
   }
   else if (!seqData.enableTransform)
      return 0;

   for (S32 i=0; i<nodes.size(); i++)
   {
      if (neverAnimateNode(nodes[i]))
         continue;

      // first rotation
      Quat16 * firstRot = &nodeRotCache[i][0];
      Quat16 * prevRot = firstRot;
      Quat16 & defaultRot = pShape->defaultRotations[i];

      if (!(*firstRot==defaultRot))
      {
         seq.rotationMatters.set(i);
         nodeCount++;
         continue;
      }

      for (S32 frame=1; frame<numFrames; frame++)
      {
         Quat16 * curRot = &nodeRotCache[i][frame];
         if (!(*curRot==*prevRot) || !(*curRot==*firstRot))
         {
            seq.rotationMatters.set(i);
            nodeCount++;
            break;
         }
         prevRot = curRot;
      }
   }

   return nodeCount;
}

S32 ShapeMimic::setTranslationMembership(TSShape * pShape, TSSequence & seq, ExporterSequenceData & seqData, S32 numFrames)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return 0;

   S32 nodeCount = 0;

   if (seqData.forceTransform)
   {
      // all transforms are forced to be members
      seq.translationMatters.setAll(nodes.size());
      for (S32 i=0; i<nodes.size(); i++)
      {
         if (neverAnimateNode(nodes[i]))
            seq.translationMatters.clear(i);
         else
            nodeCount++;
      }
      return nodeCount;
   }
   else if (!seqData.enableTransform)
      return 0;

   for (S32 i=0; i<nodes.size(); i++)
   {
      if (neverAnimateNode(nodes[i]))
         continue;

      // first rotation
      Point3F * firstTrans = &nodeTransCache[i][0];
      Point3F * prevTrans = firstTrans;
      Point3F & defaultTrans = pShape->defaultTranslations[i];

      Point3F delta = *firstTrans-defaultTrans;
      if (mFabs(delta.x)>0.0001f || mFabs(delta.y)>0.0001f || mFabs(delta.z)>0.0001f)
      {
         seq.translationMatters.set(i);
         nodeCount++;
         continue;
      }

      for (S32 frame=1; frame<numFrames; frame++)
      {
         Point3F * curTrans = &nodeTransCache[i][frame];
         Point3F delta1 = *curTrans-*prevTrans;
         Point3F delta2 = *curTrans-*firstTrans;
         if (mFabs(delta1.x)>0.0001f || mFabs(delta1.y)>0.0001f || mFabs(delta1.z)>0.0001f ||
             mFabs(delta2.x)>0.0001f || mFabs(delta2.y)>0.0001f || mFabs(delta2.z)>0.0001f)
         {
            seq.translationMatters.set(i);
            nodeCount++;
            break;
         }
         prevTrans = curTrans;
      }
   }

   return nodeCount;
}

S32 ShapeMimic::setUniformScaleMembership(TSShape * pShape, TSSequence & seq, ExporterSequenceData & seqData, S32 numFrames)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return 0;

   S32 nodeCount = 0;

   if (seqData.forceScale)
   {
      // all transforms are forced to be members
      seq.scaleMatters.setAll(nodes.size());
      for (S32 i=0; i<nodes.size(); i++)
      {
         if (neverAnimateNode(nodes[i]))
            seq.scaleMatters.clear(i);
         else
            nodeCount++;
      }
      return nodeCount;
   }
   else if (!seqData.enableUniformScale)
      return 0;

   for (S32 i=0; i<nodes.size(); i++)
   {
      if (neverAnimateNode(nodes[i]))
         continue;

      // first rotation
      Point3F a = nodeScaleCache[i][0].mScale;
      F32 firstScale = (a.x+a.y+a.z)/3.0f;
      F32 prevScale = firstScale;

      if (mFabs(firstScale-1.0f)>0.001f)
      {
         seq.scaleMatters.set(i);
         nodeCount++;
         continue;
      }

      for (S32 frame=1; frame<numFrames; frame++)
      {
         Point3F a = nodeScaleCache[i][frame].mScale;
         F32 curScale = (a.x+a.y+a.z)/3.0f;
         if (mFabs(curScale-prevScale)>0.001f)
         {
            seq.scaleMatters.set(i);
            nodeCount++;
            break;
         }
         prevScale = curScale;
      }
   }

   return nodeCount;
}

S32 ShapeMimic::setAlignedScaleMembership(TSShape * pShape, TSSequence & seq, ExporterSequenceData & seqData, S32 numFrames)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return 0;

   S32 nodeCount = 0;

   if (seqData.forceScale)
   {
      // all transforms are forced to be members
      seq.scaleMatters.setAll(nodes.size());
      for (S32 i=0; i<nodes.size(); i++)
      {
         if (neverAnimateNode(nodes[i]))
            seq.scaleMatters.clear(i);
         else
            nodeCount++;
      }
      return nodeCount;
   }
   else if (!seqData.enableArbitraryScale)
      return 0;

   for (S32 i=0; i<nodes.size(); i++)
   {
      if (neverAnimateNode(nodes[i]))
         continue;

      // first rotation
      Point3F * firstScale = &nodeScaleCache[i][0].mScale;
      Point3F * prevScale = firstScale;

      if (mFabs(firstScale->x-1.0f)>0.0001f || mFabs(firstScale->y-1.0f)>0.0001f || mFabs(firstScale->z-1.0f)>0.0001f)
      {
         seq.scaleMatters.set(i);
         nodeCount++;
         continue;
      }

      for (S32 frame=1; frame<numFrames; frame++)
      {
         Point3F * curScale = &nodeScaleCache[i][frame].mScale;
         Point3F delta1 = *curScale-*prevScale;
         Point3F delta2 = *curScale-*firstScale;
         if (mFabs(delta1.x)>0.0001f || mFabs(delta1.y)>0.0001f || mFabs(delta1.z)>0.0001f ||
             mFabs(delta2.x)>0.0001f || mFabs(delta2.y)>0.0001f || mFabs(delta2.z)>0.0001f)
         {
            seq.scaleMatters.set(i);
            nodeCount++;
            break;
         }
         prevScale = curScale;
      }
   }

   return nodeCount;
}

S32 ShapeMimic::setArbitraryScaleMembership(TSShape * pShape, TSSequence & seq, ExporterSequenceData & seqData, S32 numFrames)
{
   // for determining membership, we just care if scale factor is animated...
   return setAlignedScaleMembership(pShape,seq,seqData,numFrames);
}

bool ShapeMimic::animatesAlignedScale(ExporterSequenceData & seqData,S32 numFrames)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return false;

   if (!seqData.enableArbitraryScale)
      return false;

   for (S32 i=0; i<nodes.size(); i++)
   {
      if (neverAnimateNode(nodes[i]))
         continue;

      for (S32 frame=0; frame<numFrames; frame++)
      {
         Point3F delta = nodeScaleCache[i][frame].mScale - Point3F(1,1,1);
         if ((mFabs(delta.x)>0.001f || mFabs(delta.y)>0.001f || mFabs(delta.z)>0.001f) &&
              mFabs(delta.x-delta.y)>0.001f && mFabs(delta.y-delta.z)>0.001f && mFabs(delta.z-delta.x)>0.001f)
            // we not only animate scale, but we do it non-uniformly
            return true;
      }
   }

   return false;
}

bool ShapeMimic::animatesArbitraryScale(ExporterSequenceData & seqData, S32 numFrames)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return false;

   if (!seqData.enableArbitraryScale)
      return false;

   for (S32 i=0; i<nodes.size(); i++)
   {
      if (neverAnimateNode(nodes[i]))
         continue;

      Quat16 idQuat16;
      idQuat16.set(QuatF(0,0,0,1));
      for (S32 frame=0; frame<numFrames; frame++)
      {
         Quat16 curRot = nodeScaleCache[i][frame].mRotate;
         Point3F curScale = nodeScaleCache[i][frame].mScale;
         if (curRot==idQuat16)
            // scale factor is aligned, not arbitrary
            continue;
         Point3F delta = curScale - Point3F(1,1,1);
         if (mFabs(delta.x)<0.001f && mFabs(delta.y)<0.001f && mFabs(delta.z)<0.001f)
            // no scale
            continue;
         // we have scale and it isn't aligned...
         return true;
      }
   }

   return false;
}


//--------------------------------------------
// setup ifl membership-- called by generateSequences
S32 ShapeMimic::setIflMembership(TSShape::Sequence & seq,
                                 Interval & range,
                                 ExporterSequenceData & seqData)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return 0;

   range,seqData;

   S32 i,iflCount = 0;

   FileStream fs;
   char buffer[512];
   char name[256];

   // get start and end frame
   U32 startFrame = (U32) ( 0.01f + TicksToSec(range.Start()) * 30.0f ); // 30 fps
   U32 endFrame   = (U32) ( 0.01f + TicksToSec(  range.End()) * 30.0f ); // 30 fps

   // decide object membership
   if (seqData.enableIFL)
   {
      for (i=0; i<iflList.size(); i++)
      {
         // does ifl animate any materials during our range?
         U32 totalTime = 0, duration;
         fs.open(iflList[i]->fullFileName,FileStream::Read);
         if (fs.getStatus() != Stream::Ok)
         {
            setExportError(avar("Error reading ifl file \"%s\"",iflList[i]->fullFileName));
            fs.close();
            return 0;
         }
         Vector<S32> durations;
         bool countMe = false;
         while (fs.getStatus() == Stream::Ok)
         {
            fs.readLine((U8*)buffer,512);
            if (fs.getStatus() == Stream::Ok || fs.getStatus() == Stream::EOS)
            {
               char * pos = buffer;
               while (*pos)
               {
                  if (*pos=='\t')
                     *pos=' ';
                  pos++;
               }
               pos = buffer;
               // strip off preceding spaces...
               while (*pos && *pos==' ')
                  pos++;
               char * pos2 = dStrchr(pos,' ');
               duration = 1; // if nothing provided...
               if (pos2)
               {
                  // look for a supplied duration
                  pos = pos2+1;
                  while (*pos && *pos==' ')
                     pos++;
                  if (*pos)
                     duration = dAtoi(pos);
                  if (duration==0)
                     duration = 1;
               }
               durations.push_back(duration);
               totalTime += duration;
               if (totalTime>startFrame && totalTime<=endFrame)
               {
                  // changing material during this sequence...
                  countMe = true;
                  break;
               }
            }
         }
         fs.close();
         // file closed, but if we aren't through w/ sequence, loop ifl
         S32 loop=0;
         while (!countMe && totalTime<endFrame)
         {
            totalTime += durations[loop++ % durations.size()];
            if (totalTime>startFrame && totalTime<=endFrame)
               countMe=true;

         }
         if (countMe)
         {
            // changing material during this sequence...
            iflCount++;
            seq.iflMatters.set(i);
         }
      }
   }
   else
   {
      seq.iflMatters.clearAll();
      iflCount = 0;
   }

   // at some point we may want to have a way to more selectively animate ifl materials

   return iflCount;
}

//--------------------------------------------
// determine whether nodes with meshes which
// were cut during collapse phase have illegal
// animation
bool ShapeMimic::testCutNodes(Interval & range,
                              ExporterSequenceData & seqData)
{
    // if already encountered an error, then
    // we'll just go through the motions
    if (isError()) return true;

    // get the range of this sequence...
    F32 start     = TicksToSec(range.Start());
    F32 end       = TicksToSec(range.End());
    F32 duration;
    F32 delta;
    S32 numFrames;

    // we re-compute this here rather than pass in the data for
    // readability sake only
    getSequenceTiming(seqData,&start,&end,&duration,&delta,&numFrames);

    // this shouldn't be allowed, but check anyway...
    if (numFrames<2)
        return false;

    S32 i, frame;

    Vector<Quat16> rotTrans;
    Vector<Point3F> transTrans;
    Vector<ScaleStruct> scaleTrans;
    Vector<AffineParts> child0;
    Vector<AffineParts> parent0;
    rotTrans.setSize(numFrames);
    transTrans.setSize(numFrames);
    scaleTrans.setSize(numFrames);
    child0.setSize(cutNodes.size());
    parent0.setSize(cutNodesParents.size());

    Quat16 tmpRot;
    Point3F tmpTrans;
    Quat16 tmpScaleRot;
    Point3F tmpScaleTrans;
    for (i=0;i<cutNodes.size();i++)
      getLocalNodeTransform(cutNodes[i],cutNodesParents[i],child0[i],parent0[i],DEFAULT_TIME,tmpRot,tmpTrans,tmpScaleRot,tmpScaleTrans);

    // get the transform for each frame
    for (i=0;i<cutNodes.size();i++)
    {
        // get all the node transforms for every frame
        F32 time = start;
        for (frame = 0; frame<numFrames; frame++)
        {
            S32 maxTime = SecToTicks(time); // as in 3d studio max...
            // may go just a tad over/under due to round-off...correct that here
            if (maxTime>range.End())
                maxTime = range.End();
            else if (maxTime<range.Start())
                maxTime = range.Start();

            getLocalNodeTransform(cutNodes[i],cutNodesParents[i],child0[i],parent0[i],time,rotTrans[frame],transTrans[frame],scaleTrans[frame].mRotate,scaleTrans[frame].mScale);

            time += delta;
        }

        // we now have all numFrames transforms...check to see if they change
        Quat16 * firstRot = &rotTrans[0];
        Quat16 * prevRot = firstRot;

        Point3F * firstTrans = &transTrans[0];
        Point3F * prevTrans = firstTrans;

        Quat16 * firstScaleRot = &scaleTrans[0].mRotate;
        Quat16 * prevScaleRot = firstScaleRot;

        Point3F * firstScale = &scaleTrans[0].mScale;
        Point3F * prevScale = firstScale;

        for (frame=0; frame<numFrames; frame++)
        {
            Quat16 * curRot = &rotTrans[frame];
            Point3F * curTrans = &transTrans[frame];
            Quat16 * curScaleRot = &scaleTrans[frame].mRotate;
            Point3F * curScale = &scaleTrans[frame].mScale;
            Point3F delta = *curTrans-*prevTrans;
            Point3F deltaScale = *curScale-*prevScale;
            bool idScale1 = mFabs(curScale->x - 1.0f) < 0.01f && mFabs(curScale->y - 1.0f) < 0.01f && mFabs(curScale->z - 1.0f) < 0.01f;
            bool idScale2 = mFabs(prevScale->x - 1.0f) < 0.01f && mFabs(prevScale->y - 1.0f) < 0.01f && mFabs(prevScale->z - 1.0f) < 0.01f;
            bool pureScaleDiff = mFabs(deltaScale.x)>0.01f || mFabs(deltaScale.y)>0.01f || mFabs(deltaScale.z)>0.01f;
            bool isScaled = pureScaleDiff || ((!idScale1 || !idScale2) && !(*curScale==*prevScale));
            bool isTrans = mFabs(delta.x)>animationDelta || mFabs(delta.y)>animationDelta || mFabs(delta.z)>animationDelta;
            bool isRot = !(*curRot==*prevRot);
            if (isRot || isTrans || isScaled)
            {
                // going to report error -- add extra information to the dump file:
                printDump(PDAlways,"\r\n----------------------------------------------\r\n");
                printDump(PDAlways,avar("\r\nIllegal transform animiation detected between collapsed node \"%s\" and \"%s\".\r\n",
                    cutNodes[i]->GetName(),cutNodesParents[i]->GetName()));
                printDump(PDAlways,"Transform dump:\r\n\r\n");
                Point3F maxT(0,0,0);
                Point4F maxQ(0,0,0,0);
                Point3F startT = *firstTrans;
                QuatF startQ;
                startQ = firstRot->getQuatF(&startQ);

                for (S32 f=0; f<numFrames; f++)
                {
                    Point3F t = transTrans[f];
                    QuatF q;
                    q = rotTrans[f].getQuatF(&q);
                    printDump(PDAlways,avar("  translation:  x=%3.5f, y=%3.5f, z=%3.5f\r\n",t.x,t.y,t.z));
                    printDump(PDAlways,avar("  rotation:     x=%3.5f, y=%3.5f, z=%3.5f, w=%3.5f\r\n",q.x,q.y,q.z,q.w));
                    printDump(PDAlways,"---------------------------------\r\n");

                    F32 diff;

                    // get maximum difference for the dump file
                    diff = mFabs(t.x-startT.x);
                    if (diff > maxT.x)
                        maxT.x = diff;

                    diff = mFabs(t.y-startT.y);
                    if (diff > maxT.y)
                        maxT.y = diff;

                    diff = mFabs(t.z-startT.z);
                    if (diff > maxT.z)
                        maxT.z = diff;

                    diff = mFabs(q.x-startQ.x);
                    if (diff > maxQ.x)
                        maxQ.x = diff;

                    diff = mFabs(q.y-startQ.y);
                    if (diff > maxQ.y)
                        maxQ.y = diff;

                    diff = mFabs(q.z-startQ.z);
                    if (diff > maxQ.z)
                        maxQ.z = diff;

                    diff = mFabs(q.w-startQ.w);
                    if (diff > maxQ.w)
                        maxQ.w = diff;
                }
                printDump(PDAlways,"Maximum deviation:\r\n");
                printDump(PDAlways,avar("  translation:  x=%3.5f, y=%3.5f, z=%3.5f\r\n",maxT.x,maxT.y,maxT.z));
                printDump(PDAlways,avar("  rotation:     x=%3.5f, y=%3.5f, z=%3.5f, w=%3.5f\r\n",maxQ.x,maxQ.y,maxQ.z,maxQ.w));
                printDump(PDAlways,"        Scale may have animated too.\r\n");
                printDump(PDAlways,"---------------------------------\r\n");

                setExportError(avar("Illegal transform animation detected between collapsed node \"%s\" and \"%s\".",
                    cutNodes[i]->GetName(),cutNodesParents[i]->GetName()));
                return true;
            }
            prevTrans = curTrans;
            prevScaleRot = curScaleRot;
            prevScale = curScale;
        }
    }
    return false;
}

//--------------------------------------------
// generate ground transform animation -- called by generateSequences
void ShapeMimic::generateGroundAnimation(TSShape * pShape,
                                         Interval & range,
                                         ExporterSequenceData & seqData)
{
    // if already encountered an error, then
    // we'll just go through the motions
    if (isError()) return;

    // get the sequence we're working on ... always working
    // on the last sequence created
    TSShape::Sequence & seq = pShape->sequences.last();

    seq.firstGroundFrame = pShape->groundTranslations.size();
    seq.numGroundFrames = 0; // will change if we have ground transform

    if (seqData.ignoreGround)
        // nothing more to do
        return;

    // does this sequence animate the bounds node, if so, add ground transform
    Interval test = range;
    S32 midpoint = (range.Start() + range.End()) / 2;
    boundsNode->GetNodeTM(midpoint,&test);
    if ( test.Start()==range.Start() && test.End()==range.End() )
        // no ground animation
        return;

    // at this point we know that we do animate bounds node,
    // so we do have ground animation...

    F32 groundDelta;
    S32 groundNumFrames;
    F32 groundDuration = TicksToSec(range.End() - range.Start());

    getGroundSequenceTiming(seqData,groundDuration,&groundDelta,&groundNumFrames);

    seq.flags |= TSShape::MakePath;
    seq.numGroundFrames = groundNumFrames-1; // we only really add this many frames

    printDump(PDSequences,
        avar("\r\nAdding %i ground transform frames at %3.5f sec per frame intervals.\r\n",
             groundNumFrames,groundDelta));

    // frame at start isn't added since it would just be identity anyway...
    F32 time = TicksToSec(range.Start()) + groundDelta;
    for (S32 i=0; i<groundNumFrames-1; i++)
    {
        S32 maxTime = SecToTicks(time); // as in 3d studio max...

        // may go just a tad over/under due to round-off...correct that here
        if (maxTime>range.End())
            maxTime = range.End();
        else if (maxTime<range.Start())
            maxTime = range.Start();

        pShape->groundTranslations.increment();
        pShape->groundRotations.increment();
        Quat16 & rot = pShape->groundRotations.last();
        Point3F & trans = pShape->groundTranslations.last();
        Quat16 srot;   // ignored on ground transform
        Point3F scale; // ignored on ground transform
        getDeltaTransform(boundsNode,range.Start(),maxTime,rot,trans,srot,scale);

        time += groundDelta;
    }
}

//--------------------------------------------
// generate node animation -- called by generateSequences
void ShapeMimic::generateNodeAnimation(TSShape * pShape,
                                       Interval & range,
                                       ExporterSequenceData & seqData)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   // get the sequence we're working on ... always working
   // on the last sequence created
   TSShape::Sequence & seq = pShape->sequences.last();

   // get the range of this sequence...
   F32 start     = TicksToSec(range.Start());
   F32 end       = TicksToSec(range.End());
   F32 duration;
   F32 delta;
   S32 numFrames;

   // we re-compute this here rather than pass in the data for
   // readability sake only
   getSequenceTiming(seqData,&start,&end,&duration,&delta,&numFrames);

   // add the states -- add all the states for each node in a row
   // Note: this is new since 9-15-00...used to be that all states
   //       for each keyframe were consecutive.
   seq.baseRotation = pShape->nodeRotations.size();
   seq.baseTranslation = pShape->nodeTranslations.size();
   seq.baseScale = seq.animatesArbitraryScale() ? pShape->nodeArbitraryScaleFactors.size() :
      seq.animatesAlignedScale() ? pShape->nodeAlignedScales.size() : pShape->nodeUniformScales.size();
   for (S32 i=0; i<nodes.size(); i++)
   {
      F32 time = start;
      for (S32 frame = 0; frame<numFrames; frame++, time += delta)
      {
         S32 maxTime = SecToTicks(time); // as in 3d studio max...
         // may go just a tad over/under due to round-off...correct that here
         if (maxTime>range.End())
            maxTime = range.End();
         else if (maxTime<range.Start())
            maxTime = range.Start();
         if (seq.rotationMatters.test(i))
            addNodeRotation(nodes[i],maxTime,pShape,seqData.blend,nodeRotCache[i][frame]);
         if (seq.translationMatters.test(i))
            addNodeTranslation(nodes[i],maxTime,pShape,seqData.blend,nodeTransCache[i][frame]);
         if (seq.scaleMatters.test(i))
         {
            Quat16 & rot = nodeScaleCache[i][frame].mRotate;
            Point3F scale = nodeScaleCache[i][frame].mScale;
            if (seq.animatesArbitraryScale())
               addNodeArbitraryScale(nodes[i],maxTime,pShape,seqData.blend,rot,scale);
            else if (seq.animatesAlignedScale())
               addNodeAlignedScale(nodes[i],maxTime,pShape,seqData.blend,scale);
            else
               addNodeUniformScale(nodes[i],maxTime,pShape,seqData.blend,(scale.x+scale.y+scale.z)/3.0f);
         }
      }
   }
}

//--------------------------------------------
// generate object animation -- called by generateSequences
void ShapeMimic::generateObjectAnimation(TSShape * pShape,
                                         Interval & range,
                                         ExporterSequenceData & seqData)
{
    // if already encountered an error, then
    // we'll just go through the motions
    if (isError()) return;

    // get the sequence we're working on ... always working
    // on the last sequence created
    TSShape::Sequence & seq = pShape->sequences.last();

    // get the range of this sequence...
    F32 start     = TicksToSec(range.Start());
    F32 end       = TicksToSec(range.End());
    F32 duration;
    F32 delta;
    S32 numFrames;

    // we re-compute this here rather than pass in the data for
    // readability sake only
    getSequenceTiming(seqData,&start,&end,&duration,&delta,&numFrames);

   // add the states -- add all the states for each object in a row
   // Note: this is new since 9-15-00...used to be that all states
   //       for each keyframe were consecutive.
   seq.baseObjectState = pShape->objectStates.size();
   TSIntegerSet objectMembership = seq.frameMatters;
   objectMembership.overlap(seq.matFrameMatters);
   objectMembership.overlap(seq.visMatters);
   for (S32 i=0; i<objectList.size(); i++)
   {
      if (objectMembership.test(i))
      {
         F32 time = start;
         for (S32 frame = 0; frame<numFrames; frame++, time += delta)
         {
            S32 maxTime = SecToTicks(time); // as in 3d studio max...
            // may go just a tad over/under due to round-off...correct that here
            if (maxTime>range.End())
               maxTime = range.End();
            else if (maxTime<range.Start())
               maxTime = range.Start();

            generateObjectState(objectList[i],maxTime,pShape,seq.frameMatters.test(i),seq.matFrameMatters.test(i));
         }
      }
   }
}

//--------------------------------------------
// generate decal animation -- called by generateSequences
void ShapeMimic::generateDecalAnimation(TSShape * pShape,
                                        Interval & range,
                                        ExporterSequenceData & seqData)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   // get the sequence we're working on ... always working
   // on the last sequence created
   TSShape::Sequence & seq = pShape->sequences.last();

   // get the range of this sequence...
   F32 start     = TicksToSec(range.Start());
   F32 end       = TicksToSec(range.End());
   F32 duration;
   F32 delta;
   S32 numFrames;

   // we re-compute this here rather than pass in the data for
   // readability sake only
   getSequenceTiming(seqData,&start,&end,&duration,&delta,&numFrames);

   // add the states -- add all the states for each decal in a row
   // Note: this is new since 9-15-00...used to be that all states
   //       for each keyframe were consecutive.
   seq.baseDecalState = pShape->decalStates.size();
   for (S32 i=0; i<decalObjectList.size(); i++)
   {
      if (seq.decalMatters.test(i))
      {
         F32 time = start;
         for (S32 frame = 0; frame<numFrames; frame++, time+=delta)
         {
            S32 maxTime = SecToTicks(time); // as in 3d studio max...
            // may go just a tad over/under due to round-off...correct that here
            if (maxTime>range.End())
               maxTime = range.End();
            else if (maxTime<range.Start())
               maxTime = range.Start();

            generateDecalState(decalObjectList[i],maxTime,pShape,seqData.enableDecalFrame);
         }
      }
   }
}

//--------------------------------------------
// generate a decal state at specific time
void ShapeMimic::generateDecalState(DecalObjectMimic * dom, S32 time, TSShape * pShape, bool multipleFrames)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   printDump(PDObjectStates,avar("Adding decal state to %i detail level(s) of mesh \"%s\".\r\n",dom->numDetails,dom->targetObject->name));

   pShape->decalStates.increment();
   TSShape::DecalState & ds = pShape->decalStates.last();

   // is decal visible
   if (!decalOn(dom,time))
   {
      ds.frameIndex = -1;
      return;
   }

   // must have highest detail level...
   if (!dom->details[0].decalMesh->tsMesh)
   {
      setExportError(avar("Missing highest detail level on decal mesh \"%s\" which decals \"%s\".",dom->decalNode->GetName(),dom->targetObject->name));
      return;
   }

   S32 startNumMatFrames = dom->details[0].decalMesh->tsMesh->startPrimitive.size();

   // new frame?
   if (multipleFrames || startNumMatFrames==0)
      // add frame -- may end up using old frame or being a blank frame
      ds.frameIndex = generateDecalFrame(dom,time);
   else
      // only 1 frame allowed, use that one
      ds.frameIndex = 0;

   // new frame, add to dump file...
   if (startNumMatFrames!=dom->details[0].decalMesh->tsMesh->startPrimitive.size())
      printDump(PDObjectStates,"New frame added.\r\n");

   // all added, add separator to dump file...
   printDump(PDObjectStates|PDObjectStateDetails,"---------------------------------\r\n");
}

//--------------------------------------------
// test to see if decal is on at given time
bool ShapeMimic::decalOn(DecalObjectMimic * dom, S32 time)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return false;

   return isVis(dom->decalNode,time);
}

//--------------------------------------------
// generate a decal frame at specific time -- put it into the shape
// this may be a repeat frame, or even an empty frame, so we return the frame number
S32 ShapeMimic::generateDecalFrame(DecalObjectMimic * dom, S32 time)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return -1;

   if (!prepareDecal(dom->decalNode,time))
      // some error occurred...or maybe just no decal
      return -1;

   S32 i,start, dl;
   for (dl=0; dl<dom->numDetails; dl++)
   {
      if (!dom->details[dl].decalMesh)
         continue;
      generateDecalFrame(dom,time,dl);
   }

   return findLastDecalFrameNumber(dom);
}

// return frame number for last added frame...
// -- check to see if all the details just added a previous frame (or an empty frame) and if it's
//    the same one...if so, we'll get rid of the new frame and return the old one (or empty one)
S32 ShapeMimic::findLastDecalFrameNumber(DecalObjectMimic * dom)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return -1;

   S32 dl,i;
   bool allEmpty = true;
   S32 dupFrame = -1;
   for (dl=0; dl<dom->numDetails; dl++)
   {
      if (!dom->details[dl].decalMesh)
         continue;

      TSDecalMesh * tsDecal = dom->details[dl].decalMesh->tsMesh;
      if (tsDecal->startPrimitive.last()!=tsDecal->primitives.size())
      {
         allEmpty = false;
         for (i=0; i<tsDecal->startPrimitive.size()-1; i++)
         {
            if (tsDecal->startPrimitive.last()==tsDecal->startPrimitive[i])
               break;
         }
         if (i<tsDecal->startPrimitive.size()-1)
         {
            if (dupFrame>=0 && dupFrame!=i)
            {
               // different duplicate frame...keep the frame
               dupFrame = -1;
               break;
            }
            dupFrame = i;
         }
      }
   }
   if (dupFrame>=0 || allEmpty)
   {
      // we don't need the frame we added because we already had it (or it was empty)
      for (dl=0; dl<dom->numDetails; dl++)
      {
         if (!dom->details[dl].decalMesh)
            continue;

         TSDecalMesh * tsDecal = dom->details[dl].decalMesh->tsMesh;
         if (tsDecal->startPrimitive.size())
         {
            tsDecal->startPrimitive.decrement();
            tsDecal->texgenS.decrement();
            tsDecal->texgenT.decrement();
         }
      }
   }
   if (allEmpty)
      return -1;
   if (dupFrame>=0)
      return dupFrame;
   return dom->details[0].decalMesh->tsMesh->startPrimitive.size()-1;
}

bool ShapeMimic::prepareDecal(INode * decalNode, S32 time)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return false;

   S32 matIndex = -1;
   gDecalInfo.decalNode = decalNode;
   gDecalInfo.filter = NULL;
   gDecalInfo.pos       = Point3ToPoint3F(decalNode->GetNodeTM(time).GetTrans(),gDecalInfo.pos);
   gDecalInfo.x         = Point3ToPoint3F(decalNode->GetNodeTM(time).GetRow(0),gDecalInfo.x);
   gDecalInfo.y         = Point3ToPoint3F(decalNode->GetNodeTM(time).GetRow(1),gDecalInfo.y);
   gDecalInfo.z         = Point3ToPoint3F(decalNode->GetNodeTM(time).GetRow(2),gDecalInfo.z);
   if (!decalNode->GetUserPropFloat("DECAL::MAX_ANGLE",gDecalInfo.maxAngle))
      gDecalInfo.maxAngle = 90.0f;
   gDecalInfo.minCos    = mCos( gDecalInfo.maxAngle * M_PI / 180.0f);

   if (decalNode->GetObjectRef()->FindBaseObject()->ClassID() == Class_ID(SPHERE_CLASS_ID,0))
   {
      gDecalInfo.decalType = SphereDecal;
      F32 radius;
      S32 recenter;
      Interval valid = FOREVER;
      decalNode->GetObjectRef()->FindBaseObject()->GetParamBlock()->GetValue(0,time,radius,valid);   // PB_RADIUS=0 (hard-code because can depend on object)
      decalNode->GetObjectRef()->FindBaseObject()->GetParamBlock()->GetValue(5,time,recenter,valid); // PB_RECENTER=5
      if (recenter)
      {
         gDecalInfo.z *= radius;
         gDecalInfo.pos += gDecalInfo.z;
      }
   }
   else if (decalNode->GetObjectRef()->FindBaseObject()->ClassID() == Class_ID(CYLINDER_CLASS_ID,0))
      gDecalInfo.decalType = CylinderDecal;
   else if (decalNode->GetObjectRef()->FindBaseObject()->ClassID() == Class_ID(BOXOBJ_CLASS_ID,0))
      gDecalInfo.decalType = BoxDecal;
   else if (decalNode->GetObjectRef()->FindBaseObject()->ClassID() == HACK_PLANE_CLASS_ID) // not in docs...added meself
      gDecalInfo.decalType = PlaneDecal;
   else
   {
      setExportError(avar("Unknown decal base type (%i:%i) -- should be sphere, cylinder, box, or plane.",decalNode->GetObjectRef()->FindBaseObject()->ClassID().PartA(),decalNode->GetObjectRef()->FindBaseObject()->ClassID().PartB()));
      return false;
   }

   // get the decal mesh at this time...
   bool delTri;
   U32 saveDump = dumpMask;
   dumpMask = 0;
   TriObject * tri = getTriObject(decalNode,time,-1,delTri);
   if (tri->mesh.getNumFaces()>0)
      matIndex = tri->mesh.faces[0].getMatID(); // below we'll make sure there's only one material on the mesh and we'll use this id to get the filter bitmap
   Vector<Point3F> normals;
   Vector<U32> smooth;
   cullOffTile = false; // temporarily disable this...
   generateFaces(decalNode,tri->mesh,decalNode->GetObjTMAfterWSM(time),gDecalInfo.faces,normals,gDecalInfo.verts,gDecalInfo.tverts,gDecalInfo.indices,smooth);
   cullOffTile=true; // re-enable it...
   dumpMask = saveDump;
   if (delTri)
      delete tri;

   Vector<TSDrawPrimitive> & decalFaces = gDecalInfo.faces;
   Vector<U16> & decalIndices = gDecalInfo.indices;
   Vector<Point3> & decalVerts = gDecalInfo.verts;
   Vector<Point3> & decalTVerts = gDecalInfo.tverts;
   Vector<Point3F> & n = gDecalInfo.n;
   Vector<F32> & k = gDecalInfo.k;
   S32 & tsMat = gDecalInfo.tsMat;
   S32 i;

   // make sure decal mesh only has 1 material, and get the filter bitmap if there is one
   Mtl * mtl = decalNode->GetMtl();
   if (mtl && mtl->ClassID() == Class_ID(MULTI_CLASS_ID,0))
   {
      // make sure we only have 1 material
      tsMat = -1;
      for (i=0; i<decalFaces.size(); i++)
      {
         // if this or previous material has no material, continue...
         if (decalFaces[i].matIndex & TSDrawPrimitive::NoMaterial)
            continue;
         if (tsMat>=0 && (decalFaces[i].matIndex&TSDrawPrimitive::MaterialMask)!=tsMat)
         {
            setExportError(avar("Decal \"%s\" has more than one material.  Only one material per decal is allowed.",decalNode->GetName()));
            return false;
         }
         tsMat = decalFaces[i].matIndex&TSDrawPrimitive::MaterialMask;
      }
      // we only have 1 material...
      // now get material -- don't use tsMat, use matIndex from earlier (max version of material id)
      if (matIndex==-1 || matIndex>=((MultiMtl*)mtl)->NumSubMtls())
         mtl=NULL;
      else
         mtl = (MultiMtl*)mtl->GetSubMtl(matIndex);
   }
   else if (!decalFaces.empty())
      tsMat = decalFaces[0].matIndex;

   if (!mtl)
   {
      // no material...no decal frame
      return false;
   }
   if (mtl->ClassID() != Class_ID(DMTL_CLASS_ID,0))
   {
      setExportError(avar("Unexpected material type on decal node \"%\".",decalNode->GetName()));
      return false;
   }
   StdMat * stdMat = (StdMat*)mtl;
   if (stdMat->GetSubTexmap(ID_FI) && stdMat->MapEnabled(ID_FI))
   {
      if (stdMat->GetSubTexmap(ID_FI)->ClassID() != Class_ID(BMTEX_CLASS_ID,0))
      {
         setExportError(avar("Filter channel on decal node \"%s\" has a non-bitmap texture map.",decalNode->GetName()));
         return false;
      }
      gDecalInfo.filter = ((BitmapTex*)stdMat->GetSubTexmap(ID_FI))->GetBitmap(time);
   }

   // find plane for each face on decal
   n.setSize(decalFaces.size());
   k.setSize(decalFaces.size());
   for (i=0; i<decalFaces.size(); i++)
   {
      U32 idx0 = decalIndices[decalFaces[i].start+0];
      U32 idx1 = decalIndices[decalFaces[i].start+1];
      U32 idx2 = decalIndices[decalFaces[i].start+2];
      Point3F v10 = Point3ToPoint3F(decalVerts[idx1]-decalVerts[idx0],v10);
      Point3F v20 = Point3ToPoint3F(decalVerts[idx2]-decalVerts[idx0],v20);
      Point3F v0 = Point3ToPoint3F(decalVerts[idx0],v0);
      mCross(v20,v10,&n[i]);
      n[i].normalize();
      k[i] = mDot(v0,n[i]);
   }

   return true;
}

void ShapeMimic::generateDecalFrame(DecalObjectMimic * dom, S32 time, S32 dl)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   Vector<TSDrawPrimitive> meshFaces;
   Vector<Point3F> normals;
   Vector<Point3> meshVerts;
   Vector<Point3> meshTVerts;
   Vector<U16> meshIndices;
   Vector<U32> smooth;

   // turn off dump file since the following is just getting the mesh over again...
   U32 saveDumpMask = dumpMask;
   dumpMask = 0;

   TSDecalMesh * tsDecal  = dom->details[dl].decalMesh->tsMesh;
   INode * target = dom->targetObject->details[dl].mesh->pNode;
   S32 multiResVerts = getMultiResVerts(target,dom->targetObject->details[dl].mesh->multiResPercent);
   MultiResMimic * mrm = getMultiRes(target);
   target = mrm ? mrm->multiResNode : target;

   bool delTri;
   TriObject * tri = getTriObject(target,time,multiResVerts,delTri);
   generateFaces(target,tri->mesh,target->GetObjTMAfterWSM(time),meshFaces,normals,meshVerts,meshTVerts,meshIndices,smooth);
   if (delTri)
      delete tri;

   // restore dump file
   dumpMask = saveDumpMask;

   S32 i,j;
   Vector<bool> used;
   used.setSize(meshVerts.size());

   // for each meshVert, find out where it maps to on the decal mesh
   for (i=0; i<meshVerts.size(); i++)
   {
      used[i] = false; // till proven otherwise
      meshTVerts[i] = Point3(0,0,0);
   }

   for (i=0; i<meshFaces.size(); i++)
   {
      // we assume that the verts are unshared between faces...that is, the mesh is still
      // "unoptimized" and will be optimized later down the line...if this changes, be
      // sure to cause some trouble below (see setExportError calls in this loop)...

      // do projection for each vert of this face
      U32 start = meshFaces[i].start;
      if (meshFaces[i].numElements != 3)
      {
         setExportError("Something has broken decal generation...mesh should be unoptimized at this point");
         return;
      }
      U32 idx0 = meshIndices[start+0];
      U32 idx1 = meshIndices[start+1];
      U32 idx2 = meshIndices[start+2];
      Point3F v0 = Point3ToPoint3F(meshVerts[idx0],v0);
      Point3F v1 = Point3ToPoint3F(meshVerts[idx1],v1);
      Point3F v2 = Point3ToPoint3F(meshVerts[idx2],v2);
      Point3F faceNormal;
      mCross(v2-v0,v1-v0,&faceNormal);
      if (mDot(faceNormal,faceNormal)>0.00000001f)
         faceNormal.normalize();
      else
         // face real small...don't bother with decal
         continue;

      if (used[idx0] || used[idx1] || used[idx2])
      {
         setExportError("Assertion failed when adding decal -- re-using vertex no allowed");
         return;
      }

      // pass in face normal rather than vertex normal to figure decal tverts (only used
      // for box mapping and rejection text...i.e., if face points the wrong way, don't decal)
      used[idx0] = getDecalTVert(gDecalInfo.decalType,meshVerts[idx0],faceNormal,meshTVerts[idx0]);
      used[idx1] = getDecalTVert(gDecalInfo.decalType,meshVerts[idx1],faceNormal,meshTVerts[idx1]);
      used[idx2] = getDecalTVert(gDecalInfo.decalType,meshVerts[idx2],faceNormal,meshTVerts[idx2]);
   }

   // now get rid of faces that aren't used in the decal
   for (i=0; i<meshFaces.size(); i++)
   {
      U32 start = meshFaces[i].start;
      U32 idx0 = meshIndices[start+0];
      U32 idx1 = meshIndices[start+1];
      U32 idx2 = meshIndices[start+2];

      // is this face textured?  if we don't wrap and face tverts don't intersect unit box [0..1,0..1]
      // then we don't need this face...
      bool onBmp = (materialFlags[gDecalInfo.tsMat&TSDrawPrimitive::MaterialMask] & (TSMaterialList::S_Wrap|TSMaterialList::T_Wrap)) || checkDecalFace(meshTVerts[idx0],meshTVerts[idx1],meshTVerts[idx2]);

      if (!onBmp || !used[idx0] || !used[idx1] || !used[idx2] || !checkDecalFilter(meshTVerts[idx0],meshTVerts[idx1],meshTVerts[idx2]))
      {
         // don't want this face...
         meshIndices.erase(start);
         meshIndices.erase(start);
         meshIndices.erase(start);
         for (j=0;j<meshFaces.size();j++)
            if (meshFaces[j].start>start)
               meshFaces[j].start-=3; // assume faces are still triangles at this point
         meshFaces.erase(i);
         i--;
      }
   }

   //---------------------------------------------------
   // RIGHT HERE!
   // Get rid of tverts and generate texgen info instead
   //---------------------------------------------------
   Matrix3 mat3 = dom->targetObject->maxTSParent->GetNodeTM(time);
   zapScale(mat3);
   MatrixF invMat = convertToMatrixF(mat3,invMat);
   invMat.inverse();
   Point4F texgenS, texgenT;
   findDecalTexGen(invMat,texgenS,texgenT);

   // do we match another frame?
   S32 frame;
   S32 numFrames = tsDecal->startPrimitive.size();
   for (frame=0; frame<numFrames; frame++)
   {
      S32 numPrimitives;
      if (frame+1<numFrames)
         numPrimitives = tsDecal->startPrimitive[frame+1] - tsDecal->startPrimitive[frame];
      else
         numPrimitives = tsDecal->primitives.size() - tsDecal->startPrimitive[frame];
      if (numPrimitives != meshFaces.size())
         continue; // not same as this frame
      Point4F delta;
      delta.x = texgenS.x-tsDecal->texgenS[frame].x;
      delta.y = texgenS.y-tsDecal->texgenS[frame].y;
      delta.z = texgenS.z-tsDecal->texgenS[frame].z;
      delta.w = texgenS.w-tsDecal->texgenS[frame].w;
      if (delta.x*delta.x + delta.y*delta.y + delta.z*delta.z + delta.w*delta.w>0.0001f)
         // different s coords
         continue;
      delta.x = texgenT.x-tsDecal->texgenT[frame].x;
      delta.y = texgenT.y-tsDecal->texgenT[frame].y;
      delta.z = texgenT.z-tsDecal->texgenT[frame].z;
      delta.w = texgenT.w-tsDecal->texgenT[frame].w;
      if (delta.x*delta.x + delta.y*delta.y + delta.z*delta.z + delta.w*delta.w>0.0001f)
         // different t coords
         continue;
      // if we made it this far, then we have same number of faces and identical tverts to current frame...
      // that's a repeat...
      break;
   }

   // we either match a previous frame -- in which case we add a frame but not the faces/indices
   // or we don't -- in which case we add everything
   if (frame<numFrames)
   {
      S32 prm = tsDecal->startPrimitive[frame];
      tsDecal->startPrimitive.push_back(prm);
   }
   else
   {
      tsDecal->startPrimitive.push_back(tsDecal->primitives.size());

      for (i=0; i<meshFaces.size(); i++)
      {
         tsDecal->primitives.push_back(meshFaces[i]);
         tsDecal->primitives.last().matIndex = numFrames | TSDrawPrimitive::Triangles | TSDrawPrimitive::Indexed;
         tsDecal->primitives.last().start += tsDecal->indices.size();
      }
      for (i=0; i<meshIndices.size(); i++)
         tsDecal->indices.push_back(meshIndices[i]);
      tsDecal->texgenS.push_back(texgenS);
      tsDecal->texgenT.push_back(texgenT);
      tsDecal->materialIndex = gDecalInfo.tsMat;
   }
}

void ShapeMimic::findDecalTexGen(MatrixF & invMat, Point4F & texgenS, Point4F & texgenT)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   if (gDecalInfo.faces.empty())
   {
      setExportError("Assertion failed");
      texgenS.set(0,0,0,0);
      texgenT.set(0,0,0,0);
   }

   Point3F n = gDecalInfo.n[0];
   invMat.mulV(n);

   // use first face of decal to determine texgen coords...
   S32 start = gDecalInfo.faces[0].start;
   Point3F v0 = Point3ToPoint3F(gDecalInfo.verts[gDecalInfo.indices[start+0]],v0);
   invMat.mulP(v0);
   Point3F v1 = Point3ToPoint3F(gDecalInfo.verts[gDecalInfo.indices[start+1]],v1);
   invMat.mulP(v1);
   Point3F v2 = Point3ToPoint3F(gDecalInfo.verts[gDecalInfo.indices[start+2]],v2);
   invMat.mulP(v2);

   // adjust origin to be center of face...this guarantees that we can invert matrix
   Point3F center = v0+v1+v2;
   center *= 1.0f/3.0f;
   v0 -= center;
   v1 -= center;
   v2 -= center;

   Point2F tv0,tv1,tv2;
   tv0.x = gDecalInfo.tverts[gDecalInfo.indices[start+0]].x;
   tv0.y = gDecalInfo.tverts[gDecalInfo.indices[start+0]].y;

   tv1.x = gDecalInfo.tverts[gDecalInfo.indices[start+1]].x;
   tv1.y = gDecalInfo.tverts[gDecalInfo.indices[start+1]].y;

   tv2.x = gDecalInfo.tverts[gDecalInfo.indices[start+2]].x;
   tv2.y = gDecalInfo.tverts[gDecalInfo.indices[start+2]].y;

   MatrixF mat;
   mat.identity();

   mat.setRow(0,Point4F(v0.x,v0.y,v0.z,1));
   mat.setRow(1,Point4F(v1.x,v1.y,v1.z,1));
   mat.setRow(2,Point4F(v2.x,v2.y,v2.z,1));
   mat.setRow(3,Point4F(n.x,n.y,n.z,0));
   if (!mat.fullInverse())
   {
      setExportError("Assertion failed computing decal inverse transform");
      return;
   }

   Point3F lastRow;
   mat.getRow(3,&lastRow);

   Point3F s,t;
   mat.mulV(Point3F(tv0.x,tv1.x,tv2.x),&s);
   mat.mulV(Point3F(tv0.y,tv1.y,tv2.y),&t);

   F32 ws = mDot(lastRow,Point3F(tv0.x,tv1.x,tv2.x));
   ws -= mDot(s,center); // adjust back for moving origin

   F32 wt = mDot(lastRow,Point3F(tv0.y,tv1.y,tv2.y));
   wt -= mDot(t,center); // adjust back for moving origin

   printDump(-1,avar("S decal plane (%f,%f,%f,%f)\r\n",s.x,s.y,s.z,ws));
   printDump(-1,avar("T decal plane (%f,%f,%f,%f)\r\n",t.x,t.y,t.z,wt));
   texgenS.set(s.x,s.y,s.z,ws);
   texgenT.set(t.x,t.y,t.z,wt);
}

bool ShapeMimic::getDecalTVert(U32 decalType, Point3 vert, Point3F normal, Point3 & tv)
{
   Vector<Point3> & decalVerts = gDecalInfo.verts;
   Vector<Point3> & decalTVerts = gDecalInfo.tverts;
   Vector<TSDrawPrimitive> & decalFaces = gDecalInfo.faces;
   Vector<U16> & decalIndices = gDecalInfo.indices;

   Vector<Point3F> & n = gDecalInfo.n;
   Vector<F32> & k = gDecalInfo.k;

   for (S32 i=0; i<decalFaces.size(); i++)
   {
      Point3F p;
      if (!getDecalProjPoint(decalType,vert,normal,i,p))
         continue;
      if (mFabs(mDot(p,n[i])-k[i])>0.001f)
      {
         // if not on plane, above computation has gone awry
         setExportError("Assertion failed when adding decal (1)");
         return false;
      }

      // is p in this face?
      Point3F edge;
      Point3F inVec;
      U32 idx0 = decalIndices[decalFaces[i].start+0];
      U32 idx1 = decalIndices[decalFaces[i].start+1];
      U32 idx2 = decalIndices[decalFaces[i].start+2];
      Point3F v0 = Point3ToPoint3F(decalVerts[idx0],v0);
      Point3F v1 = Point3ToPoint3F(decalVerts[idx1],v1);
      Point3F v2 = Point3ToPoint3F(decalVerts[idx2],v2);
      edge = v1-v0;
      mCross(edge,n[i],&inVec);
      if (mDot(inVec,v0)>mDot(inVec,p))
         // nope...
         continue;
      edge = v2-v1;
      mCross(edge,n[i],&inVec);
      if (mDot(inVec,v1)>mDot(inVec,p))
         // nope...
         continue;
      edge = v0-v2;
      mCross(edge,n[i],&inVec);
      if (mDot(inVec,v2)>mDot(inVec,p))
         // nope...
         continue;

      // ok, but does this face have a material
      if (decalFaces[i].matIndex & TSDrawPrimitive::NoMaterial)
         continue;

      // yep...get tvert
      interpolateTVert(v0,v1,v2,n[i],p,decalTVerts[idx0],decalTVerts[idx1],decalTVerts[idx2],tv);
      return true;
   }

   if (decalType==PlaneDecal)
      return getPlaneDecalTVSpecial(vert,normal,tv);

   return false;
}

bool ShapeMimic::getDecalProjPoint(U32 decalType, Point3 vert, Point3F & normal, U32 decalFaceIdx, Point3F & p)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return false;

   Vector<Point3F> & n = gDecalInfo.n;
   Vector<F32> & k = gDecalInfo.k;

   Point3F a,ray;
   switch (decalType)
   {
      case SphereDecal:
      {
         ray = Point3ToPoint3F(vert,ray);
         ray -= gDecalInfo.pos;
         a = gDecalInfo.pos;
         break;
      }

      case CylinderDecal:
      {
         // just like sphere decal, except project vert onto cylinder axis and use that as pos
         Point3F pt = Point3ToPoint3F(vert,pt);
         F32 t = mDot(pt,gDecalInfo.z) - mDot(gDecalInfo.pos,gDecalInfo.z);
         a  = gDecalInfo.z;
         a *= t;
         a += gDecalInfo.pos;
         ray = pt;
         ray -= a;
         break;
      }

      case BoxDecal:
      {
         if (mDot(n[decalFaceIdx],normal) < 0.5f)
            // only use closest face...
            return false;
         // we're on closest face (closest in the sense of pointing nearest to the same direction)
         // just do plane map...
         a = Point3ToPoint3F(vert,a);
         ray = n[decalFaceIdx];
         break;
      }

      case PlaneDecal:
      {
         a = Point3ToPoint3F(vert,a);
         ray = n[decalFaceIdx];
         break;
      }

      default:
         setExportError("Unknown decal base type -- should be sphere, cylinder, box, or plane.");
         return false;
   };

   // find point on plane j of line passing through "a" in direction of "ray"
   if (mDot(ray,n[decalFaceIdx]) < 0.001f)
      // parallel or behind the ray
     return false;
   if (mDot(normal,n[decalFaceIdx])<gDecalInfo.minCos)
      // facing wrong way -- or too much the wrong way, anyway
      return false;
   F32 t = (k[decalFaceIdx] - mDot(a,n[decalFaceIdx])) / mDot(ray,n[decalFaceIdx]);
   p = ray;
   p *= t;
   p += a;
   return true;
}

bool ShapeMimic::getPlaneDecalTVSpecial(Point3 & vert, const Point3F & normal, Point3 & tv)
{
   Vector<Point3> & decalVerts = gDecalInfo.verts;
   Vector<Point3> & decalTVerts = gDecalInfo.tverts;
   Vector<TSDrawPrimitive> & decalFaces = gDecalInfo.faces;
   Vector<U16> & decalIndices = gDecalInfo.indices;

   Vector<Point3F> & n = gDecalInfo.n;
   Vector<F32> & k = gDecalInfo.k;

   Point3F p = Point3ToPoint3F(vert,p);

   if (n.size() && mDot(normal,n[0])<gDecalInfo.minCos)
      // facing wrong way -- or too much the wrong way, anyway
      return false;

   // Plane is special case...if mesh projects off plane, we still generate tverts...
   // we'll assume tvert scale is uniform across plane (if assumption unwarranted, artist
   // needs to extend the  plane).  Since scale is uniform, we can just use 1st decal face...

   if (decalFaces.empty())
      return false;

   U32 idx0 = decalIndices[decalFaces[0].start+0];
   U32 idx1 = decalIndices[decalFaces[0].start+1];
   U32 idx2 = decalIndices[decalFaces[0].start+2];
   Point3F v0 = Point3ToPoint3F(decalVerts[idx0],v0);
   Point3F v1 = Point3ToPoint3F(decalVerts[idx1],v1);
   Point3F v2 = Point3ToPoint3F(decalVerts[idx2],v2);

   // find edges of triangle...assume not co-linear.
   Point3F edge1 = v1-v0;
   Point3F edge2 = v2-v0;

   // find number of steps to take along edge1 and edge2 to go from v0 to p
   F32 t1,t2;
   findNonOrthogonalComponents(edge1,edge2,p-v0,t1,t2);

   Point3 tv0 = decalTVerts[idx0];
   Point3 tv1 = decalTVerts[idx1];
   Point3 tv2 = decalTVerts[idx2];

   Point3 tvEdge1 = tv1-tv0;
   Point3 tvEdge2 = tv2-tv0;

   tv = tv0 + t1*tvEdge1 + t2*tvEdge2;

   return true;
}

void ShapeMimic::findNonOrthogonalComponents(const Point3F & v1, const Point3F & v2, const Point3F & p, F32 & t1, F32 & t2)
{
   // find t1, t2 s.t. p = v1 * t1 + v2 * t2
   // we first find q which is the intersection of the line from the origin through v1
   // and the line through p in the direction of v2
   // we assume that v1, v2, and p are all on the same plane, so N is normal to that plane,
   // L1 is vector which describes first line on that plane, and L2 is vector which describes
   // second line on that plane.
   Point3F L1, L2, N, q;
   mCross(v2,v1,&N);
   N.normalize(); // not needed, but keeps numbers smaller
   mCross(N,v1,&L1);
   mCross(N,v2,&L2);

   // vec describes q: holds the dot products of q with N, L1, and L2, respectively.
   Point3F vec(0, 0, mDot(L2,p));

   MatrixF mat;
   mat.identity();
   mat.setRow(0,N);
   mat.setRow(1,L1);
   mat.setRow(2,L2);
   mat.inverse();

   // get q
   mat.mulP(vec,&q);

   t1 = mDot(v1,q);
   t2 = mDot(v2,p-q);

   F32 d1 = 1.0f / v1.len();
   F32 d2 = 1.0f / v2.len();
   t1 *= d1*d1;
   t2 *= d2*d2;
}


void ShapeMimic::interpolateTVert(Point3F v0, Point3F v1, Point3F v2, Point3F planeNormal, Point3F & p, Point3 tv0, Point3 tv1, Point3 tv2, Point3 & tv)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   // rotate points till v1 and v2 are furthest apart
   Point3F leg1 = v1-v0;
   Point3F leg2 = v2-v0;
   Point3F c = v2-v1;
   // Note: we actually NEED to use the following variables...compiler bug causes infinite loop
   //       if we put the mDot's directly into the "while" statement.
   F32 fa = mDot(leg1,leg1);
   F32 fb = mDot(leg2,leg2);
   F32 fc = mDot(c,c);
   while (fa>fc || fb>fc)
   {
      Point3F tmp = v0;
      v0 = v1;
      v1 = v2;
      v2 = tmp;
      Point3 tmp2 = tv0;
      tv0 = tv1;
      tv1 = tv2;
      tv2 = tmp2;
      leg1 = v1-v0;
      leg2=v2-v0;
      c=v2-v1;
      fa = mDot(leg1,leg1);
      fb = mDot(leg2,leg2);
      fc = mDot(c,c);
   }

   // where does line passing through v0 and p intersect edge v1 to v2?
   Point3F edgeNormal;
   mCross(c,planeNormal,&edgeNormal);
   Point3F vec = p-v0;
   F32 t = (mDot(edgeNormal,v1) - mDot(edgeNormal,v0)) / mDot(edgeNormal,vec);
   Point3F v12 = vec;
   v12 *= t;
   v12 += v0;
   if (mFabs(mDot(edgeNormal,v1)-mDot(edgeNormal,v12)) > 0.001f)
   {
      // oops, v12 not on edge v1-v2
      setExportError("Assertion failed when adding decal (2)");
      return;
   }

   // how far up edge v1 to v2 is point v12
   F32 k = mDot(v12-v1,c) / mDot(c,c);
   if (k<0.0f || k>1.0f)
   {
      // oops, v12 is on line v1-v2 but is off the edge
      setExportError("Assertion failed when adding decal (3)");
      return;
   }

   // find texture coord for v12
   Point3 tv12;
   tv12.x = tv1.x * (1.0f - k) + tv2.x * k;
   tv12.y = tv1.y * (1.0f - k) + tv2.y * k;
   tv12.z = tv1.z * (1.0f - k) + tv2.z * k;

   // how far along edge from v0 to v12 is p
   k = mDot(v12-v0,p-v0) / mDot(v12-v0,v12-v0);
   if (k<0.0f || k>1.0f)
   {
      // oops, p is supposed to be between v0 and v12
      setExportError("Assertion failed when adding decal (4)");
      return;
   }

   // finally, find texture coord for p
   tv.x = tv0.x * (1.0f - k) + tv12.x * k;
   tv.y = tv0.y * (1.0f - k) + tv12.y * k;
   tv.z = tv0.z * (1.0f - k) + tv12.z * k;
}

bool intersectUnitSquare(Point3 & tv0, Point3  & tv1)
{
   // return true if line from tv0 to tv1 intersects any edge of unit square

   // compare with (0,0) to (0,1)
   if ( (tv0.x <= 0.0f && tv1.x >= 0.0f) || (tv0.x >= 0.0f && tv1.x <= 0.0f) )
   {
      // tv0-tv1 crosses edge
      F32 k = (0.0f - tv0.x) / (tv1.x - tv0.x);
      F32 y = tv0.y + k * (tv1.y - tv0.y);
      if (y>=0.0f && y<=1.0f)
         return true; // we intersect
   }

   // compare with (0,0) to (1,0)
   if ( (tv0.y <= 0.0f && tv1.y >= 0.0f) || (tv0.y >= 0.0f && tv1.y <= 0.0f) )
   {
      // tv0-tv1 crosses edge
      F32 k = (0.0f - tv0.y) / (tv1.y - tv0.y);
      F32 x = tv0.x + k * (tv1.x - tv0.x);
      if (x>=0.0f && x<=1.0f)
         return true; // we intersect
   }

   // compare with (1,0) to (1,1)
   if ( (tv0.x <= 1.0f && tv1.x >= 1.0f) || (tv0.x >= 1.0f && tv1.x <= 1.0f) )
   {
      // tv0-tv1 crosses edge
      F32 k = (1.0f - tv0.x) / (tv1.x - tv0.x);
      F32 y = tv0.y + k * (tv1.y - tv0.y);
      if (y>=0.0f && y<=1.0f)
         return true; // we intersect
   }

   // compare with (0,1) to (1,1)
   if ( (tv0.y <= 1.0f && tv1.y >= 1.0f) || (tv0.y >= 1.0f && tv1.y <= 1.0f) )
   {
      // tv0-tv1 crosses edge
      F32 k = (1.0f - tv0.y) / (tv1.y - tv0.y);
      F32 x = tv0.x + k * (tv1.x - tv0.x);
      if (x>=0.0f && x<=1.0f)
         return true; // we intersect
   }

   return false;
}

// returns true if face defined by v0,v1, & v2 intersects
// box [0..1,0..1]
// verts v0,v1, and v2 treated as Point2's even tho really Point3's
bool ShapeMimic::checkDecalFace(Point3 tv0, Point3 tv1, Point3 tv2)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return true;

   // face intersects box iff:
   // 1.  any of 3 verts is in box [0..1,0..1]
   // 2.  (0,0) is in face (i.e. box is in face)
   // 3.  some edge of box intersects some edge of face

   // 1.  any of 3 verts is in box [0..1,0..1]
   if (tv0.x >= 0.0f && tv0.x <= 1.0f && tv0.y >= 0.0f && tv0.y <= 1.0f)
      return true;

   // 2.  (0,0) is in face (i.e. box is in face)
   Point3F face[3];
   Point3ToPoint3F(tv0,face[0]); face[0].z = 0;
   Point3ToPoint3F(tv1,face[1]); face[1].z = 0;
   Point3ToPoint3F(tv2,face[2]); face[2].z = 0;
   if (pointInPoly(Point3F(0,0,0),Point3F(0,0,1),face,3))
      return true;

   // 3.  some edge of box intersects some edge of face
   if (intersectUnitSquare(tv0,tv1))
      return true;
   if (intersectUnitSquare(tv1,tv2))
      return true;
   if (intersectUnitSquare(tv2,tv0))
      return true;

   return false;
}

// returns true if some non-zero value is found in the filter bitmap
// within the triangle defined by v0-v1-v2 (z-coord ignored)
bool ShapeMimic::checkDecalFilter(Point3 v0, Point3 v1, Point3 v2)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return true;

   if (!gDecalInfo.filter)
      return true;

   F32 width = gDecalInfo.filter->Width();
   F32 height = gDecalInfo.filter->Height();

   // rotate until min and max y is in v0 and v1
   while ( (v2.y > v1.y && v2.y > v0.y) || (v2.y < v1.y && v2.y < v0.y))
   {
      Point3 tmp = v0;
      v0 = v1;
      v1 = v2;
      v2 = tmp;
   }
   // make v0.y top, v1.y bottom -- don't care about winding direction
   if (v0.y > v1.y)
   {
      Point3 tmp = v0;
      v0 = v1;
      v1 = tmp;
   }
   // scale v0,v1,v2 to be in the right range
   v0.x *= width;
   v1.x *= width;
   v2.x *= width;
   v0.y *= height;
   v1.y *= height;
   v2.y *= height;

   for (F32 y = v0.y; y<=v1.y; y+=1.0f)
   {
      F32 x1 = v0.x + (v1.x-v0.x) * (y-v0.y)/(v1.y-v0.y);
      F32 x2;
      if (y<v2.y)
         x2 = v0.x + (v2.x-v0.x) * (y-v0.y)/(v2.y-v0.y);
      else
         x2 = v2.x + (v1.x-v2.x) * (y-v2.y)/(v1.y-v2.y);
      if (x2<x1)
      {
         F32 tmp = x1;
         x1 = x2;
         x2 = tmp;
      }
      for (F32 x=x1; x<=x2; x+=1.0f)
      {
         BMM_Color_64 color;
         if (!gDecalInfo.filter->GetPixels(x,y,1,&color))
         {
            setExportError("Assertion failed during decal:  trouble retrieving value from filter bitmap");
            return false;
         }
         if (color.r || color.g || color.b)
            return true;
      }
   }
   // didn't find any non-zero polys...
   return false;
}

//--------------------------------------------
// generate frame triggers -- called by generateSequences
void ShapeMimic::generateFrameTriggers(TSShape * pShape,
                                       Interval & range,
                                       ExporterSequenceData & seqData,
                                       IParamBlock * pblock)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   // get the sequence we're working on ... always working
   // on the last sequence created
   TSShape::Sequence & seq = pShape->sequences.last();

   // get the range of this sequence...
   F32 start     = TicksToSec(range.Start());
   F32 end       = TicksToSec(range.End());
   F32 duration;
   F32 delta;
   S32 numFrames;

   // we re-compute this here rather than pass in the data for
   // readability sake only...even though we only want duration
   getSequenceTiming(seqData,&start,&end,&duration,&delta,&numFrames);

   // initialize triggers...
   seq.firstTrigger = pShape->triggers.size();
   seq.numTriggers  = 0;

   if (!pblock->GetController(PB_SEQ_TRIGGERS))
      return; // no triggers, rest now

   // these are the triggers that get turned off by this shape..."normally" triggers
   // aren't turned on/off, just on...if we are a sequence that does both then we
   // need to mark ourselves as such so that on/off can become off/on when sequence
   // is played in reverse...
   U32 offTriggers = 0;

   IKeyControl * kc = GetKeyControlInterface(pblock->GetController(PB_SEQ_TRIGGERS));
   if (kc)
      // triggers are already in order so this is a bit of overkill...but were going
      // to have several tracks of triggers at one point...
      addTriggersInOrder(kc,duration,pShape,offTriggers);

   for (S32 i=seq.firstTrigger; i<seq.numTriggers; i++)
   {
      if ( (pShape->triggers[i].state&TSShape::Trigger::StateMask) & offTriggers )
         // this trigger controls a state that is turned on and off...
         // make sure state is reversed if played backwards
         pShape->triggers[i].state |= TSShape::Trigger::InvertOnReverse;
   }
}

//--------------------------------------------
// used by above method
void ShapeMimic::addTriggersInOrder(IKeyControl * kc, F32 duration, TSShape * pShape, U32 & offTriggers)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   // get the sequence we're working on ... always working
   // on the last sequence created
   TSShape::Sequence & seq = pShape->sequences.last();

   for (S32 i=0; i<kc->GetNumKeys(); i++)
   {
      IBezFloatKey key;
      kc->GetKey(i,&key);

      if (key.val<-30 || key.val>30)
      {
         setExportError(avar("Sequence \"%s\" has trigger with magnitude greater than 30",pShape->getName(seq.nameIndex)));
         return;
      }
      if ((U32)key.val==0)
      {
         setExportError(avar("Sequence \"%s\" has trigger with magnitude 0",pShape->getName(seq.nameIndex)));
         return;
      }
      U32 val;
      bool on;
      if (key.val<0)
      {
         val = -key.val - 1;
         on  = false;
      }
      else
      {
         val = key.val - 1;
         on  = true;
      }

      // between 0 and 31
      if (val<0 || val > 32)
      {
         setExportError("Assertion failed when processing frame triggers -- get programmer");
         return;
      }

      S32 j;
      F32 pos = TicksToSec(key.time) / duration;
      for (j=seq.firstTrigger; j<pShape->triggers.size(); j++)
      {
         if (pos<pShape->triggers[j].pos)
            break; // put new trigger here
      }
      pShape->triggers.insert(j);
      TSShape::Trigger & trigger = pShape->triggers[j];
      trigger.pos = pos;
      if (on)
      {
         trigger.state  = 1 << (U32)val;
         trigger.state |= TSShape::Trigger::StateOn;
      }
      else
      {
         trigger.state = 1 << val;
         offTriggers |= trigger.state; // this trigger is turned off by sequence (we assume on too)...see below for consequences
      }
      seq.numTriggers++;
   }
}

//--------------------------------------------
// generate material list -- called by generate shape
void ShapeMimic::generateMaterialList(TSShape * pShape)
{
    // if already encountered an error, then
    // we'll just go through the motions
    if (isError()) return;

    pShape->materialList = new TSMaterialList(materials.size(),
                                              (const char**)materials.address(),
                                              (const U32*)materialFlags.address(),
                                              (const U32*)materialReflectionMaps.address(),
                                              (const U32*)materialBumpMaps.address(),
                                              (const U32*)materialDetailMaps.address(),
                                              (const F32*)materialDetailScales.address(),
                                              (const F32*)materialEmapAmounts.address());
}

//--------------------------------------------
// generate skins -- called by generate shape
void ShapeMimic::generateSkins(TSShape * pShape)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   // put skins in the right order...we're basically sorting by detailSize member of skinMimic
   // but we'll do it a slow way to make sure in synch with detail ordering
   {
      // Note:  using code block to scope index variables for convenience
      S32 i,j,k;
      j=0;
      for (i=0; i<pShape->details.size(); i++)
      {
         // search for skins of size details[i].size
         // place any found at j and advance j
         for (k=j; k<skins.size(); k++)
            if (skins[k]->detailSize == (S32) pShape->details[i].size)
            {
               // swap skin j and k, increment j
               SkinMimic * tmp = skins[j];
               skins[j] = skins[k];
               skins[k] = tmp;
               j++;
            }
      }
      if (j!=skins.size() && !allowUnusedMeshes)
      {
         setExportError("Unused skins were found.");
         return;
      }
      for (;j<skins.size();j++)
      {
         delete skins[j];
         skins.erase(j);
         j--;
      }
   }
   // skins are now in order of detail size and unused ones have been deleted

   for (S32 i=0; i<objectList.size(); i++)
   {
      if (!objectList[i]->isSkin)
         continue;
      // all meshes on this object should be skins...
      for (S32 meshNum=0; meshNum<objectList[i]->numDetails; meshNum++)
      {
         if (!objectList[i]->details[meshNum].mesh)
            continue;
         if (!objectList[i]->details[meshNum].mesh->skinMimic)
         {
            setExportError("Assertion failed generating skins");
            return;
         }

         // this mesh is a skin...set it up (it's already been created)
         TSSkinMesh * skinMesh = (TSSkinMesh*)objectList[i]->details[meshNum].mesh->tsMesh;

         // now find the corresponding skin
         SkinMimic * skin = objectList[i]->details[meshNum].mesh->skinMimic;
         skin->skinMesh = skinMesh;
         skinMesh->numFrames = 1;
         skinMesh->numMatFrames = 1;
         skinMesh->vertsPerFrame = skin->verts.size();

         skinMesh->primitives = skin->faces;
         skinMesh->indices = skin->indices;
         skinMesh->mergeIndices = skin->mergeIndices;
         skinMesh->initialNorms = skin->normals;

         skinMesh->tverts.setSize(skin->tverts.size());
         S32 j,k;
         for (j=0; j<skin->tverts.size(); j++)
            skinMesh->tverts[j].set(skin->tverts[j].x,skin->tverts[j].y);

         skinMesh->initialVerts.setSize(skin->verts.size());
         for (j=0; j<skin->verts.size(); j++)
            Point3ToPoint3F(skin->verts[j],skinMesh->initialVerts[j]);

         // how many verts in the next smallest version of us
         S32 numChildVerts = 0;
         k = 1;
         while (meshNum+k<objectList[i]->numDetails)
         {
            if (objectList[i]->details[meshNum+k].mesh)
            {
               numChildVerts = objectList[i]->details[meshNum+k].mesh->numVerts;
               break;
            }
            k++;
         }

         // find merge indices
         findMergeIndices(objectList[i]->details[meshNum].mesh,
                          objectList[i]->details[meshNum].mesh->objectOffset,
                          skinMesh->primitives,
                          skinMesh->initialVerts,
                          skinMesh->initialNorms,
                          skinMesh->tverts,
                          skinMesh->indices,
                          skinMesh->mergeIndices,
                          objectList[i]->details[meshNum].mesh->skinMimic->smoothingGroups,
                          objectList[i]->details[meshNum].mesh->skinMimic->vertId,
                          numChildVerts);
         skinMesh->vertsPerFrame = skinMesh->initialVerts.size();

         // up till now weights have been stored in terms of vertId...convert to vertex number now
         copyWeightsToVerts(objectList[i]->details[meshNum].mesh->skinMimic);

         // for each bone, indicate node index in shape and inverseDefaultTransform
         for (j=0; j<skin->bones.size(); j++)
         {
            // find node index
            for (k=0; k<nodes.size(); k++)
               if (nodes[k]->maxNode == skin->bones[j])
                  break;
            if (k==nodes.size())
            {
               setExportError("Error: bone missing from shape");
               return;
            }
            skinMesh->nodeIndex.push_back(k);

            Matrix3 boundsTransform = boundsNode->GetNodeTM( DEFAULT_TIME );
            zapScale(boundsTransform);
            Matrix3 invBoneTransform = skin->bones[j]->GetNodeTM( DEFAULT_TIME );
            zapScale(invBoneTransform);
            invBoneTransform = Inverse(invBoneTransform);

            Matrix3 initTransform = boundsTransform;
            initTransform *= invBoneTransform;

            MatrixF matf;
            F32 * f = (F32*)matf;
            f[0 * 4 + 0] = initTransform.GetAddr()[0][0];
            f[0 * 4 + 1] = initTransform.GetAddr()[1][0];
            f[0 * 4 + 2] = initTransform.GetAddr()[2][0];
            f[0 * 4 + 3] = initTransform.GetAddr()[3][0];

            f[1 * 4 + 0] = initTransform.GetAddr()[0][1];
            f[1 * 4 + 1] = initTransform.GetAddr()[1][1];
            f[1 * 4 + 2] = initTransform.GetAddr()[2][1];
            f[1 * 4 + 3] = initTransform.GetAddr()[3][1];

            f[2 * 4 + 0] = initTransform.GetAddr()[0][2];
            f[2 * 4 + 1] = initTransform.GetAddr()[1][2];
            f[2 * 4 + 2] = initTransform.GetAddr()[2][2];
            f[2 * 4 + 3] = initTransform.GetAddr()[3][2];

            f[3 * 4 + 0] = 0.0f;
            f[3 * 4 + 1] = 0.0f;
            f[3 * 4 + 2] = 0.0f;
            f[3 * 4 + 3] = 1.0f;

            skinMesh->initialTransforms.push_back(matf);
         }

         printDump(PDObjectStateDetails|PDPass2,avar("\r\nGenerating skin \"%s\".\r\n",skin->skinNode->GetName()));

         // push all vertex, bone, weight triples
         printDump(PDObjectStateDetails,"\r\nVertex, bone, & weight data:\r\n\r\n");
         for (j=0; j<skinMesh->initialVerts.size(); j++)
         {
            printDump(PDObjectStateDetails,avar("Vertex %i\r\n",j));
            for (k=0; k<skin->bones.size(); k++)
            {
               if ((*skin->weights[k])[j]>=weightThreshhold)
               {
                  skinMesh->vertexIndex.push_back(j);
                  skinMesh->boneIndex.push_back(k);
                  skinMesh->weight.push_back((*skin->weights[k])[j]);

                  printDump(PDObjectStateDetails,avar("  Bone %i, weight = %5.3f, name =  \"%s\"\r\n",
                     skinMesh->boneIndex.last(),skinMesh->weight.last(),skin->bones[k]->GetName()));
               }
            }
         }
      }
   }
}

//--------------------------------------------
// various routines used by generate faces

void ShapeMimic::splitFaceX(TSDrawPrimitive & face, TSDrawPrimitive & faceA, TSDrawPrimitive & faceB,
                            Vector<Point3> & verts, Vector<Point3> & tverts, Vector<U16> & indices,
                            Vector<bool> & flipX, Vector<bool> & flipY, F32 splitAt,
                            Vector<U32> & smooth, Vector<U32> * vertId)
{
   // find the odd man out (odd vertex out, really)
   S32 code = 0, rogue;
   if (tverts[indices[face.start+0]].x < splitAt)
      code |= 1;
   if (tverts[indices[face.start+1]].x < splitAt)
      code |= 2;
   if (tverts[indices[face.start+2]].x < splitAt)
      code |= 4;

   switch (code)
   {
      case 1:
      case 6: rogue = 0; break;
      case 2:
      case 5: rogue = 1; break;
      case 4:
      case 3: rogue = 2; break;
      case 0:
      case 7: setExportError("splitFaceX: not off-tile, get programmer."); return;
   }

   // idx0,1,2 used to walk around triangle clock-wise starting at odd-vertex out (rogue vertex)
   // note: a little confusing because idx0,1,2 are offsets from face.start in indices array.
   // indices[face.start+idx0], for example, is the index into vert and tvert array of the rogue vertex...
   // indices[face.start+idx1] is index for next vertex around the face clockwise...
   S32 idx0 = rogue;
   S32 idx1 = (rogue+1) % 3;
   S32 idx2 = (rogue+2) % 3;

   // the vert and tvert indices for the 3 vertices of the face
   S32 v0 = indices[face.start+idx0];
   S32 v1 = indices[face.start+idx1];
   S32 v2 = indices[face.start+idx2];

   // fill out the indices for the new faces and the split face

   indices[face.start+idx1] = verts.size();
   indices[face.start+idx2] = verts.size()+1;

   faceA.start = indices.size();
   faceA.numElements = 3;
   indices.push_back(verts.size()+2);
   indices.push_back(v1);
   indices.push_back(verts.size()+3);

   faceB.start = indices.size();
   faceB.numElements = 3;
   indices.push_back(v1);
   indices.push_back(v2);
   indices.push_back(verts.size()+3);

   // find out the actual values of the 2 new vertices we've added
   F32 k;

   k = (splitAt - tverts[v0].x) / (tverts[v1].x - tverts[v0].x);
   tverts.increment();
   tverts.last() = tverts[v1] - tverts[v0];
   tverts.last() *= k;
   tverts.last() += tverts[v0];
   tverts.last().x = splitAt; // make sure exactly this
   verts.increment();
   verts.last() = verts[v1] - verts[v0];
   verts.last() *= k;
   verts.last() += verts[v0];

   k = (splitAt - tverts[v2].x) / (tverts[v0].x - tverts[v2].x);
   tverts.increment();
   tverts.last() = tverts[v0] - tverts[v2];
   tverts.last() *= k;
   tverts.last() += tverts[v2];
   tverts.last().x = splitAt; // make sure exactly this
   verts.increment();
   verts.last() = verts[v0] - verts[v2];
   verts.last() *= k;
   verts.last() += verts[v2];

   // duplicate the new tverts for faceA and faceB
   // necessary because they will be flipped (x = 1-x)
   // from face coords
   tverts.increment();
   tverts.last() = tverts[tverts.size()-3];
   tverts.increment();
   tverts.last() = tverts[tverts.size()-3];
   verts.increment();
   verts.last() = verts[verts.size()-3];
   verts.increment();
   verts.last() = verts[verts.size()-3];

   // flip coords?  if we don't wrap we don't flip.  o.w., which verts to flip depends on
   // whether rogue face was on-tile or off-tile.
   if (!(materialFlags[face.matIndex & TSDrawPrimitive::MaterialMask] & TSMaterialList::S_Wrap))
   {
      flipX.increment();
      flipX.last() = false;
      flipX.increment();
      flipX.last() = false;
      flipX.increment();
      flipX.last() = false;
      flipX.increment();
      flipX.last() = false;
   }
   else if (tverts[v0].x >=0.0f && tverts[v0].x <= 1.0f)
   {
      flipX.increment();
      flipX.last() = false;
      flipX.increment();
      flipX.last() = false;
      flipX.increment();
      flipX.last() = true;
      flipX.increment();
      flipX.last() = true;
   }
   else
   {
      flipX.increment();
      flipX.last() = true;
      flipX.increment();
      flipX.last() = true;
      flipX.increment();
      flipX.last() = false;
      flipX.increment();
      flipX.last() = false;
   }
   flipY.increment();
   flipY.last() = false;
   flipY.increment();
   flipY.last() = false;
   flipY.increment();
   flipY.last() = false;
   flipY.increment();
   flipY.last() = false;

   // added 4 verts, all have same smoothing group...
   S32 smooth0 = smooth[v0];
   smooth.push_back(smooth0);
   smooth.push_back(smooth0);
   smooth.push_back(smooth0);
   smooth.push_back(smooth0);

   if (vertId)
   {
      S32 vid0 = (*vertId)[v0];
      vertId->push_back(vid0);
      vertId->push_back(vid0);
      vertId->push_back(vid0);
      vertId->push_back(vid0);
   }

   // these faces need to have the same material indices...
   faceA.matIndex = faceB.matIndex = face.matIndex;
}

void ShapeMimic::splitFaceY(TSDrawPrimitive & face, TSDrawPrimitive & faceA, TSDrawPrimitive & faceB,
                            Vector<Point3> & verts, Vector<Point3> & tverts, Vector<U16> & indices,
                            Vector<bool> & flipX, Vector<bool> & flipY, F32 splitAt,
                            Vector<U32> & smooth, Vector<U32> * vertId)
{
   // find the odd man out (odd vertex out, really)
   S32 code = 0, rogue;
   if (tverts[indices[face.start+0]].y < splitAt)
      code |= 1;
   if (tverts[indices[face.start+1]].y < splitAt)
      code |= 2;
   if (tverts[indices[face.start+2]].y < splitAt)
      code |= 4;

   switch (code)
   {
      case 1:
      case 6: rogue = 0; break;
      case 2:
      case 5: rogue = 1; break;
      case 4:
      case 3: rogue = 2; break;
      case 0:
      case 7: setExportError("splitFaceY: not off-tile, get programmer."); return;
   }

   // indices used to walk around triangle clock-wise starting at odd-vertex out (rogue vertex)
   // note: a little confusing because these are vertices to face.v[] and face.tv[] which are
   // arrays of 3 indices to the verts and tverts of the triangle...
   S32 idx0 = rogue;
   S32 idx1 = (rogue+1) % 3;
   S32 idx2 = (rogue+2) % 3;

   // the verts and tverts for these indices
   S32 v0 = indices[face.start+idx0];
   S32 v1 = indices[face.start+idx1];
   S32 v2 = indices[face.start+idx2];

   // fill out the indices for the new faces and the split face

   indices[face.start+idx1] = verts.size();
   indices[face.start+idx2] = verts.size() + 1;

   faceA.start = indices.size();
   faceA.numElements = 3;
   indices.push_back(verts.size()+2);
   indices.push_back(v1);
   indices.push_back(verts.size()+3);
   faceB.start = indices.size();
   faceB.numElements = 3;
   indices.push_back(v1);
   indices.push_back(v2);
   indices.push_back(verts.size()+3);

   // find out the actual values of the 2 new vertices we've added
   F32 k;

   k = (splitAt - tverts[v0].y) / (tverts[v1].y - tverts[v0].y);
   tverts.increment();
   tverts.last() = tverts[v1] - tverts[v0];
   tverts.last() *= k;
   tverts.last() += tverts[v0];
   tverts.last().y = splitAt; // make sure exactly this
   verts.increment();
   verts.last() = verts[v1] - verts[v0];
   verts.last() *= k;
   verts.last() += verts[v0];

   k = (splitAt - tverts[v2].y) / (tverts[v0].y - tverts[v2].y);
   tverts.increment();
   tverts.last() = tverts[v0] - tverts[v2];
   tverts.last() *= k;
   tverts.last() += tverts[v2];
   tverts.last().y = splitAt; // make sure exactly this
   verts.increment();
   verts.last() = verts[v0] - verts[v2];
   verts.last() *= k;
   verts.last() += verts[v2];

   // duplicate the new tverts for faceA and faceB
   // necessary because they will be flipped (y = 1-y)
   // from face coords
   tverts.increment();
   tverts.last() = tverts[tverts.size()-3];
   tverts.increment();
   tverts.last() = tverts[tverts.size()-3];
   verts.increment();
   verts.last() = verts[verts.size()-3];
   verts.increment();
   verts.last() = verts[verts.size()-3];

   // flip coords?  if we don't wrap we don't flip.  o.w., which verts to flip depends on
   // whether rogue face was on-tile or off-tile.
   if (!(materialFlags[face.matIndex & TSDrawPrimitive::MaterialMask] & TSMaterialList::T_Wrap))
   {
      flipY.increment();
      flipY.last() = false;
      flipY.increment();
      flipY.last() = false;
      flipY.increment();
      flipY.last() = false;
      flipY.increment();
      flipY.last() = false;
   }
   else if (tverts[v0].y >=0.0f && tverts[v0].y <= 1.0f)
   {
      flipY.increment();
      flipY.last() = false;
      flipY.increment();
      flipY.last() = false;
      flipY.increment();
      flipY.last() = true;
      flipY.increment();
      flipY.last() = true;
   }
   else
   {
      flipY.increment();
      flipY.last() = true;
      flipY.increment();
      flipY.last() = true;
      flipY.increment();
      flipY.last() = false;
      flipY.increment();
      flipY.last() = false;
   }

   flipX.increment();
   flipX.last() = false;
   flipX.increment();
   flipX.last() = false;
   flipX.increment();
   flipX.last() = false;
   flipX.increment();
   flipX.last() = false;

   // added 4 verts, all have same smoothing group...
   S32 smooth0 = smooth[v0];
   smooth.push_back(smooth0);
   smooth.push_back(smooth0);
   smooth.push_back(smooth0);
   smooth.push_back(smooth0);
   if (vertId)
   {
      S32 vid0 = (*vertId)[v0];
      vertId->push_back(vid0);
      vertId->push_back(vid0);
      vertId->push_back(vid0);
      vertId->push_back(vid0);
   }

   // these faces need to have the same material indices...
   faceA.matIndex = faceB.matIndex = face.matIndex;
}

void ShapeMimic::handleCropAndPlace(Point3 & tv, CropInfo & cropInfo)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   if (cropInfo.place)
   {
      tv.x = (tv.x - cropInfo.uOffset) / cropInfo.uWidth;
      tv.y = (tv.y - cropInfo.vOffset) / cropInfo.vHeight;
      return;
   }

   if (cropInfo.crop)
   {
      // if off-tile and we wrap, then adjust coords to be positive
      if (cropInfo.uWrap)
      {
         while (cropInfo.cropLeft && tv.x<0.0f) tv.x += 1.0f;
         while (cropInfo.cropRight && tv.x>1.0f) tv.x -= 1.0f;
      }
      else
      {
         if (cropInfo.cropLeft && tv.x<0.0f) tv.x = 0.0f;
         if (cropInfo.cropRight && tv.x>1.0f) tv.x = 1.0f;
      }

      if (cropInfo.vWrap)
      {
         while (cropInfo.cropTop && tv.y<0.0f) tv.y += 1.0f;
         while (cropInfo.cropBottom && tv.y>1.0f) tv.y -= 1.0f;
      }
      else
      {
         if (cropInfo.cropTop && tv.y<0.0f) tv.y = 0.0f;
         if (cropInfo.cropBottom && tv.y>1.0f) tv.y = 1.0f;
      }

      // if on-tile, perform cropping here
      // if we're off-tile, don't crop use these coords
      // Note: in Max these off-tile faces will be textured
      // according to the border of the cropped section of the bmp,
      // whereas in game engine they will be textured according
      // to greater bmp's border area...live with it
      if (tv.x>=0.0f && tv.x<=1.0f)
         tv.x = tv.x * cropInfo.uWidth + cropInfo.uOffset;
      if (tv.y>=0.0f && tv.y<=1.0f)
         tv.y = tv.y * cropInfo.vHeight + cropInfo.vOffset;
   }
}

//--------------------------------------------
// make lower detail levels use higher detail
// levels verts, when possible
// we assume lower detail levels set up already
// (but by lower detail we mean higher dl # )
void ShapeMimic::shareVerts(ObjectMimic * om, S32 dl, TSShape * pShape)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   MultiResMimic * mrm = getMultiRes(om->details[dl].mesh->pNode);

   if (!om->details[dl].mesh || !mrm)
      return;

   S32 i,j,k;
   TSMesh * mesh = om->details[dl].mesh->tsMesh;
   Vector<U32> & smooth = om->details[dl].mesh->smoothingGroups;
   Vector<U32> remap;
   remap.setSize(mesh->verts.size());
   for (i=0;i<remap.size();i++)
      remap[i]=i;
   for (i=dl+1; i<om->numDetails; i++)
   {
      if (mrm!=getMultiRes(om->details[i].mesh->pNode))
         continue;

      TSMesh * mesh2 = om->details[i].mesh->tsMesh;
      Vector<U32> & smooth2 = om->details[i].mesh->smoothingGroups;
      if (mrm->totalVerts) // will be zero if not really a multires object... (hack,hack)
      {
         // ok, detail dl is generated via same multi-res object as detail i
         // share verts...
         S32 count=0;
         for (j=0; j<mesh->verts.size(); j++)
         {
            for (k=0; k<mesh2->verts.size(); k++)
            {
               S32 frame1 = j/mesh->vertsPerFrame;
               S32 frame2 = k/mesh2->vertsPerFrame;
               if (!vertexSame(mesh->verts[j],mesh2->verts[k],mesh->tverts[j],mesh2->tverts[k],smooth[j],smooth2[k],-1,-1,NULL))
                  // different...
                  continue;
               if (frame1!=frame2)
                  // different frames...
                  continue;

               // same vert
               remap[j] = k;
               break;
            }
            if (k==mesh2->verts.size())
               remap[j] = mesh2->verts.size() + count++;
         }
      }
      else
      {
         // this is a faux multi-res node...we just want to copy verts between detail levels
         // and this is already done (since generated from same mesh).  But in order to share
         // the following code, we just need to set up remap to be identity (already done)
         // So at this point, just check to make sure we are legit (mesh and mesh2 are the same)
         if (mesh->vertsPerFrame!=mesh2->vertsPerFrame || mesh->verts.size()!=mesh2->verts.size())
         {
            setExportError("Assertion failed sharing verts");
            return;
         }
      }

      // at this point we have remapped all shared verts into mesh2
      // and we have a count of how many verts are unique to this mesh (count)
      // we've also shifted remap down to account for verts moving to mesh2
      Vector<Point3F> newVerts;
      Vector<Point2F> newTVerts;
      Vector<Point3F> newNorms;
      Vector<U32> newSmooth;
      Vector<U16> newMerge;
      newVerts = mesh2->verts;
      newTVerts = mesh2->tverts;
      newNorms = mesh2->norms;
      newSmooth = smooth2;
      newMerge.setSize(mesh2->mergeIndices.size());
      for (j=0; j<newMerge.size(); j++) // unlike other vectors, we don't share these with our child
         newMerge[j]=j;
      for (j=0; j<mesh->verts.size(); j++)
      {
         if (remap[j]>=mesh2->verts.size())
         {
            newVerts.push_back(mesh->verts[j]);
            newTVerts.push_back(mesh->tverts[j]);
            newNorms.push_back(mesh->norms[j]);
            newSmooth.push_back(smooth[j]);
            newMerge.push_back(remap[mesh->mergeIndices[j]]);
         }
         else
            // normals on this detail level over-rule normals from lower detail levels
            newNorms[remap[j]] = mesh->norms[j];
      }

      // adjust indices on mesh
      for (j=0; j<mesh->indices.size(); j++)
         mesh->indices[j] = remap[mesh->indices[j]];

      // fixup remap on mesh
      for (j=0; j<om->details[dl].mesh->remap.size(); j++)
      {
         U32 & idx = om->details[dl].mesh->remap[j];
         idx = remap[idx];
      }

      // now equate verts on mesh and mesh2
      mesh->verts = newVerts;
      mesh->tverts = newTVerts;
      mesh->norms = newNorms;
      mesh->mergeIndices = newMerge;
      mesh->vertsPerFrame = mesh->verts.size() / mesh->numFrames; // keep up to date...
      smooth = newSmooth;
/*
      for (j=dl+1; j<om->numDetails; j++)
      {
         if (om->details[j].mesh->tsMesh && getMultiRes(om->details[dl].mesh->pNode)==getMultiRes(om->details[j].mesh->pNode))
         {
            // who the hell are we?  Find first mesh in shapes lists of meshes
            S32 meshIdx =0;
            while (meshIdx < pShape->meshes.size() && pShape->meshes[meshIdx]!=om->details[dl].mesh->tsMesh)
               meshIdx++;
            if (meshIdx==pShape->meshes.size())
            {
               setExportError("Assertion failed");
               return;
            }
            om->details[j].mesh->tsMesh->parentMesh = meshIdx;
         }
      }
*/
      // the above commented out code makes the highest detail the parent mesh of all lower detail version
      // the code below creates a chain of inheritance instead
      // who the hell are we?  Find first mesh in shapes lists of meshes
      S32 meshIdx =0;
      while (meshIdx < pShape->meshes.size() && pShape->meshes[meshIdx]!=om->details[dl].mesh->tsMesh)
         meshIdx++;
      om->details[i].mesh->tsMesh->parentMesh = meshIdx;
      if (meshIdx==pShape->meshes.size())
      {
         setExportError("Assertion failed");
         return;
      }

      break;
   }
}

void ShapeMimic::shareVerts(SkinMimic * sm, Vector<U32> & originalRemap, TSShape * pShape)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   MultiResMimic * mrm = getMultiRes(sm->skinNode);

   if (!sm->skinMesh || !mrm)
      return;

   S32 i,j,k;
   TSSkinMesh * mesh = sm->skinMesh;
   Vector<U32> & smooth = sm->smoothingGroups;
   Vector<U32> remap;
   remap.setSize(mesh->verts.size());
   for (i=0;i<remap.size();i++)
      remap[i]=i;

   // find next lowest detail level (next lowest in detail, next highest in number)
   SkinMimic * sm2 = NULL;
   for (i=0; i<skins.size(); i++)
   {
      if (skins[i]->detailSize < sm->detailSize && skins[i]->skinNode && mrm==getMultiRes(skins[i]->skinNode))
      {
         sm2 = skins[i];
         sm2->skinMesh->parentMesh = sm->meshNum;
         break;
      }
   }
   if (!sm2)
      return;

   // ok, sm and sm2 are generated via same multi-res object
   // share verts...
   TSSkinMesh * mesh2 = sm2->skinMesh;
   Vector<U32> & smooth2 = sm2->smoothingGroups;
   S32 count=0;
   for (j=0; j<mesh->verts.size(); j++)
   {
      for (k=0; k<mesh2->initialVerts.size(); k++)
      {
         if (!vertexSame(mesh->verts[j],mesh2->initialVerts[k],mesh->tverts[j],mesh2->tverts[k],smooth[j],smooth2[k],-1,-1,NULL))
            // different...
            continue;

         if (sm->vertId[j]!=sm2->vertId[k])
            // different vert ids...
            continue;

         // same vert
         remap[j] = k;
         break;
      }
      if (k==mesh2->initialVerts.size())
         remap[j] = mesh2->initialVerts.size() + count++;
   }

   // at this point we have remapped all shared verts into mesh2
   // and we have a count of how many verts are unique to this mesh (count)
   // we've also shifted remap down to account for verts moving to mesh2
   Vector<Point3F> newVerts;
   Vector<Point2F> newTVerts;
   Vector<Point3F> newNorms;
   Vector<U32> newSmooth;
   Vector<U32> newVertId;
   Vector<U16> newMerge;
   newVerts = mesh2->initialVerts;
   newTVerts = mesh2->tverts;
   newNorms = mesh2->initialNorms;
   newSmooth = smooth2;
   newVertId = sm2->vertId;
   newMerge.setSize(mesh2->mergeIndices.size());
   for (j=0; j<newMerge.size(); j++) // unlike other vectors, we don't share these with our child
      newMerge[j]=j;
   for (j=0; j<mesh->verts.size(); j++)
   {
      if (remap[j]>=mesh2->initialVerts.size())
      {
         newVerts.push_back(mesh->verts[j]);
         newTVerts.push_back(mesh->tverts[j]);
         newNorms.push_back(mesh->norms[j]);
         newMerge.push_back(remap[mesh->mergeIndices[j]]);
         newSmooth.push_back(smooth[j]);
         newVertId.push_back(sm->vertId[j]);
      }
      else
         // normals on this detail level over-rule normals from lower detail levels
         newNorms[remap[j]] = mesh->norms[j];
   }

   // adjust indices on mesh
   for (j=0; j<mesh->indices.size(); j++)
      mesh->indices[j] = remap[mesh->indices[j]];

   // fix up originalRemap
   for (i=0; i<originalRemap.size(); i++)
      originalRemap[i] = remap[originalRemap[i]];

   // now equate verts on mesh and mesh2
   mesh->verts = newVerts;
   mesh->tverts = newTVerts;
   mesh->norms = newNorms;
   mesh->mergeIndices = newMerge;
   mesh->vertsPerFrame = mesh->verts.size();
   smooth = newSmooth;
   sm->vertId = newVertId;
}

void ShapeMimic::shareBones(SkinMimic * sm, TSShape * pShape)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   MultiResMimic * mrm = getMultiRes(sm->skinNode);

   if (!sm->skinMesh || !mrm)
      return;

   S32 i,j,k;
   TSSkinMesh * mesh = sm->skinMesh;

   // find next lowest detail level (next lowest in detail, next highest in number)
   SkinMimic * sm2 = NULL;
   for (i=0; i<skins.size(); i++)
   {
      if (skins[i]->detailSize < sm->detailSize && skins[i]->skinNode && mrm==getMultiRes(skins[i]->skinNode))
      {
         if (skins[i]->skinMesh->parentMesh != sm->meshNum)
         {
            // this was established in shareVerts...should still be true
            setExportError("Assertion failed");
            return;
         }
         sm2 = skins[i];
         break;
      }
   }
   if (!sm2)
      return;

   // ok, sm and sm2 are generated via same multi-res object
   TSSkinMesh * mesh2 = sm2->skinMesh;

   // make sure mesh2 isn't adding any bones...
   if (mesh2->nodeIndex.size() >mesh->nodeIndex.size())
   {
         setExportError("Assertion failed:  Lower skin detail level has more bones than higher one does!!!");
         return;
   }
   for (k=0; k<mesh2->nodeIndex.size(); k++)
   {
      for (j=0; j<mesh->nodeIndex.size(); j++)
      {
         if (mesh->nodeIndex[j]==mesh2->nodeIndex[k])
            break;
      }
      if (j==mesh->nodeIndex.size())
      {
         setExportError("Assertion failed:  Lower skin detail level is adding bone!!!");
         return;
      }
   }

   // remap nodeIndex so that mesh2's nodeIndex array is first part of mesh1's
   S32 count=0;
   Vector<U32> remap;
   remap.setSize(mesh->nodeIndex.size());
   for (j=0; j<mesh->nodeIndex.size(); j++)
   {
      for (k=0; k<mesh2->nodeIndex.size(); k++)
      {
         if (mesh->nodeIndex[j]!=mesh2->nodeIndex[k])
            // different...
            continue;
         // same node
         remap[j] = k;
         break;
      }
      if (k==mesh2->nodeIndex.size())
         remap[j] = mesh2->nodeIndex.size() + count++;
   }

   // at this point, we have constructed remap so that node in nodeIndex vector
   // of mesh that also occur in child mesh will occur first (and in the order
   // it occurs in child mesh).  Other nodes occur afterwards.
   // Let's do some remapping:
   Vector<S32> newNodeIndex;
   newNodeIndex.setSize(mesh->nodeIndex.size());
   Vector<MatrixF> newIMats;
   newIMats.setSize(mesh->initialTransforms.size());
   for (j=0;j<remap.size();j++)
   {
      newNodeIndex[remap[j]]=mesh->nodeIndex[j];
      newIMats[remap[j]]=mesh->initialTransforms[j];
   }
   mesh->nodeIndex = newNodeIndex;
   mesh->initialTransforms = newIMats;
   // remap boneIndex (which are indices into nodeIndex list, which are indices into shape transform list)
   for (j=0; j<mesh->boneIndex.size(); j++)
      mesh->boneIndex[j] = remap[mesh->boneIndex[j]];

   // we don't necessarily have weighting for all vertices, because lower detail level
   // may have added a vertex (because of tvert/vert combination issues in max versus openGL and
   // every other graphics standard).  Look for such missing vertex,bone,weight combos and add them
   // here
   for (j=0; j<mesh->vertexIndex.size() && j<mesh2->vertexIndex.size(); j++)
   {
      if (mesh->vertexIndex[j]==mesh2->vertexIndex[j])
         // all is cool, keep going
         continue;
      while (j<mesh->vertexIndex.size() && j<mesh2->vertexIndex.size() && mesh->vertexIndex[j]!=mesh2->vertexIndex[j])
      {
         mesh->vertexIndex.insert(j);
         mesh->vertexIndex[j]=mesh2->vertexIndex[j];
         mesh->boneIndex.insert(j);
         mesh->boneIndex[j]=mesh2->boneIndex[j];
         mesh->weight.insert(j);
         mesh->weight[j]=mesh2->weight[j];
         j++;
      }
   }
}

//--------------------------------------------
// find which verts merge with which verts
void ShapeMimic::findMergeIndices(MeshMimic * meshMimic,
                                  Matrix3 & objectOffset3,
                                  const Vector<TSDrawPrimitive> & faces,
                                  Vector<Point3F> & verts,
                                  Vector<Point3F> & norms,
                                  Vector<Point2F> & tverts,
                                  const Vector<U16> & indices,
                                  Vector<U16> & mergeIndices,
                                  Vector<U32> & smooth,
                                  Vector<U32> & vertId,
                                  S32 numChildVerts)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   MultiResMimic * mrm = getMultiRes(meshMimic->pNode);
   if (!mrm || mrm->totalVerts==0)
      return;

   // we want to manipulate these but not change incoming data (other vectors will be changed)
   Vector<TSDrawPrimitive> workFaces = faces;
   Vector<U16> workIndices = indices;

   S32 i,j,v;

   mergeIndices.setSize(verts.size());
   for (i=0;i<mergeIndices.size();i++)
      mergeIndices[i]=i;

   Vector<S32> mergeOrder;
   Vector<S32> mergeToId;

   // merge from and mergeTo vectors are in object space before applying object offset
   // apply object offset to those points
   Vector<Point3F> mergeFrom, mergeTo;
   mergeFrom.setSize(mrm->mergeFrom.size());
   mergeTo.setSize(mrm->mergeTo.size());
   MatrixF objectOffset;
   convertToMatrixF(objectOffset3,objectOffset);
   for (i=0; i<mrm->mergeFrom.size(); i++)
   {
      objectOffset.mulP(mrm->mergeFrom[i],&mergeFrom[i]);
      objectOffset.mulP(mrm->mergeTo[i],&mergeTo[i]);
   }
   mrm=NULL; // don't want to use anything in this for the rest of this method...crash if we accidentally do

   // determine merge order of each vert
   // this also uniquely id's each vert location
   for (i=0; i<verts.size(); i++)
   {
      // find point to merge this vertex to
      S32 idx = findClosestMatch(verts[i],mergeFrom);
      if (idx<0 || mDot(verts[i]-mergeFrom[idx],verts[i]-mergeFrom[idx])>0.0001f)
      {
         setExportError(avar("Assertion error during vertex merge on mesh \"%s\"",meshMimic->pNode->GetName()));
         return;
      }
      mergeOrder.push_back(idx);
   }
   // do same thing to mergeTo list...the point
   // of this is to give each target a unique id
   for (i=0; i<mergeTo.size(); i++)
   {
      S32 idx = findClosestMatch(mergeTo[i],mergeFrom);
      if (idx<0 || mDot(mergeTo[i]-mergeFrom[idx],mergeTo[i]-mergeFrom[idx])>0.0001f || idx<i)
      {
         setExportError("Assertion error during vertex merge");
         return;
      }
      if (idx < mergeToId.size())
      {
         setExportError("Assertion error during vertex merge");
         return;
      }
      mergeToId.push_back(idx);
   }
   // check to make sure mergeFrom has no dups
   for (i=0; i<mergeTo.size(); i++)
   {
      if (i!=findClosestMatch(mergeFrom[i],mergeFrom))
      {
         setExportError(avar("Assertion error during vertex merge: must weld verts on mesh \"%s\".",meshMimic->pNode->GetName()));
         return;
      }
   }

   // we now have easy access to the location in space
   // that each vertex merges to and the order of merge.
   // but what happens to the other attributes depends
   // on the topology of the mesh.  The key is that we
   // want to maintain continuity across faces and also
   // to maintain dis-continuous borders.  To achieve this
   // we follow the rules below.  For these rules we consider
   // each instance of a vertex as a unique vertex.  So even
   // if two faces have a vertex with the same location,
   // tvert and material, they will be considered different
   // vertices.  (Note: this also turns out to be convenient
   // here because we have not merged equivalent vertices
   // so there are actually no vertices shared between faces).
   // The rules:
   // 1. if a vertex is to merge to a location occupied
   //   by one of the other vertices of the face, then
   //   the other attributes of the vertex will interpolate
   //   to the values of the other vertex.
   // 2. if a vertex is to merge to a location occupied
   //   by a vertex of another face, and that face has
   //   a vertex with the same location, tvert, material, etc,
   //   as our first vertex, then the vertex parameters
   //   will once again interpolate to the values found on
   //   the vertex in the target location.
   // 3. if neither of the above are true for any face,
   //   then the other parameters will not change as the
   //   position interpolates to the target position.

   // merge verts in order, adding mergeIndices and new vertex targets as we go
   if (numChildVerts==0)
      numChildVerts=mergeFrom.size();
   for (i=0; i<mergeFrom.size()-numChildVerts; i++)
   {
      // keep track of next vert index before we start this round -- see below
      S32 startVerts = verts.size();

      // go through faces and find all verts that need to be merged this round
      for (j=0; j<workFaces.size(); j++)
      {
         TSDrawPrimitive & face = workFaces[j];
         if ((face.matIndex & TSDrawPrimitive::TypeMask) != TSDrawPrimitive::Triangles)
         {
            setExportError("Assertion failed during vertex merge");
            return;
         }
         // find vertex that needs to be merged this round
         for (v=0; v<3; v++)
         {
            // merge to a vert on this face?
            if (mergeOrder[workIndices[face.start+v]]==i)
               // bingo
               addMergeTargetAndMerge(workFaces,j,v,workIndices,mergeIndices,verts,tverts,norms,smooth,vertId,mergeOrder,mergeToId,mergeTo);
         }
      }
      // now that we've merged all the verts to their targets, update indices to reflect new location
      // -- repeat the above in same order so that we can use "startVerts" to figure out new indices
      for (j=0; j<workFaces.size(); j++)
      {
         TSDrawPrimitive & face = workFaces[j];
         if ((face.matIndex & TSDrawPrimitive::TypeMask) != TSDrawPrimitive::Triangles)
         {
            setExportError("Assertion failed during vertex merge");
            return;
         }
         // find vertex that needs to be merged this round
         for (v=0; v<3; v++)
         {
            // merge to a vert on this face?
            if (mergeOrder[workIndices[face.start+v]]==i)
               workIndices[face.start+v]=startVerts++;
         }
      }

   }

   // check to make sure we always merge to the right place
   for (i=0; i<verts.size(); i++)
   {
      if (mergeIndices[i]==i)
         // this is ok...if a face is terminated, we don't worry about the isolated verts...
         continue;
      S32 idx = findClosestMatch(verts[i],mergeFrom);
      F32 dot = mDot(verts[mergeIndices[i]]-mergeTo[idx],verts[mergeIndices[i]]-mergeTo[idx]);
      if (dot>0.001f)
      {
         setExportError("Assertion failed");
         return;
      }
   }

   // slightly different form of the above...here we make sure if one
   // point merges someplace, another equivalent point does too
   for (i=0; i<mergeOrder.size(); i++)
   {
      for (j=i+1; j<mergeOrder.size(); j++)
         if (mergeOrder[i]==mergeOrder[j])
         {
            Point3F delta = verts[mergeIndices[i]]-verts[mergeIndices[j]];
            if (mDot(delta,delta)>0.0001f)
            {
               setExportError("Assertion failed");
               return;
            }
         }
   }
}

void ShapeMimic::addMergeTargetAndMerge(Vector<TSDrawPrimitive> & faces, S32 & faceNum, S32 & v,
                                        Vector<U16> & indices, Vector<U16> & mergeIndices,
                                        Vector<Point3F> & verts, Vector<Point2F> & tverts, Vector<Point3F> & norms,
                                        Vector<U32> & smooth, Vector<U32> & vertId,
                                        Vector<S32> & mergeOrder, Vector<S32> & mergeToId, Vector<Point3F> & mergeTarget)
{
   // get id for vertex target
   TSDrawPrimitive & face = faces[faceNum];
   S32 start = face.start;
   S32 vertMergeOrder = mergeOrder[indices[start+v]];
   S32 targetId = mergeToId[vertMergeOrder];
   if (targetId<vertMergeOrder)
      setExportError("Assertion error");

   Point3F vertTarget;
   Point3F normTarget;
   Point2F tvertTarget;
   U32 vid;
   S32 idx,sm;
   S32 i,j,v2;

   // this is where the new vert will go, no matter how we get the info
   mergeIndices[indices[face.start+v]] = verts.size();
   mergeIndices.push_back(mergeIndices.size());
   mergeOrder.push_back(targetId);

   // search this face for the target location
   bool found = false;
   for (i=1; i<3; i++)
   {
      if (targetId == mergeOrder[indices[start + ((v+i)%3)]])
      {
         // merge to vertex on same face
         idx = indices[start+((v+i)%3)];
         vertTarget = verts[idx];
         tvertTarget = tverts[idx];
         normTarget = norms[idx];
         vid = vertId[idx];
         sm = smooth[idx];
         verts.push_back(vertTarget);
         tverts.push_back(tvertTarget);
         norms.push_back(normTarget);
         vertId.push_back(vid);
         smooth.push_back(sm);
         {
            F32 dot = mDot(verts.last()-mergeTarget[vertMergeOrder],verts.last()-mergeTarget[vertMergeOrder]);
            if (dot>0.001f)
               setExportError("Assertion failed");
         }
         return;
      }
   }

   // more complicated...try to find another face with a vert like us
   for (i=0; i<faces.size(); i++)
   {
      if (i==faceNum)
         continue;
      TSDrawPrimitive & face2 = faces[i];
      if (face.matIndex != face2.matIndex)
         continue;
      for (v2=0; v2<3; v2++)
      {
         if (mergeOrder[indices[face2.start+v2]]!=vertMergeOrder)
            continue;
         if (vertId[indices[face.start+v]]!=vertId[indices[face2.start+v2]])
            // different vertId
            continue;
         for (j=1; j<3; j++)
         {
            if (targetId == mergeOrder[indices[face2.start + ((v2+j)%3)]])
               break;
         }
         if (j==3)
            // wrong target
            continue;
         Point2F delta = tverts[indices[face.start+v]]-tverts[indices[face2.start+v2]];
         if (mFabs(delta.x)>sameTVertTOL || mFabs(delta.y)>sameTVertTOL)
         {
            // wrong tvert
            continue;
         }

         // merge to vertex on this face
         idx = indices[face2.start+((v2+j)%3)];
         vertTarget = verts[idx];
         tvertTarget = tverts[idx];
         normTarget = norms[idx];
         vid = vertId[idx];
         sm = smooth[idx];
         verts.push_back(vertTarget);
         tverts.push_back(tvertTarget);
         norms.push_back(normTarget);
         vertId.push_back(vid);
         smooth.push_back(sm);
         {
            F32 dot = mDot(verts.last()-mergeTarget[vertMergeOrder],verts.last()-mergeTarget[vertMergeOrder]);
            if (dot>0.001f)
               setExportError("Assertion failed");
         }
         return;
      }
   }

   // merge to tvert of original vert but location of target
   vertTarget = mergeTarget[vertMergeOrder]; // only this is different than ourself
   // must go to special lengths to get the right vert id...
   idx = findClosestMatch(vertTarget,verts);
   vid = vertId[idx];
   idx = indices[face.start+v]; // ourself
   tvertTarget = tverts[idx]; // ourself
   normTarget = norms[idx]; // ourself
   sm = smooth[idx];
   verts.push_back(vertTarget);
   tverts.push_back(tvertTarget);
   norms.push_back(normTarget);
   smooth.push_back(sm);
   vertId.push_back(vid);
   {
      F32 dot = mDot(verts.last()-mergeTarget[vertMergeOrder],verts.last()-mergeTarget[vertMergeOrder]);
      if (dot>0.001f)
         setExportError("Assertion failed");
   }
}

//--------------------------------------------
// optimize meshes -- called by generate shape
void ShapeMimic::optimizeMeshes(TSShape * pShape)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   S32 i,j,k;

   printDump(PDObjectStateDetails,"\r\nOptimizing meshes...\r\n");

   // go through meshes and optimize each one...
   for (i=0; i<objectList.size(); i++)
   {
      ObjectMimic * om = objectList[i];
      if (om->isSkin)
         // save skins for later...
         continue;
      for (j=om->numDetails-1; j>=0; j--)
      {
         if (!om->details[j].mesh)
            continue;

         printDump(PDObjectStateDetails,avar("\r\nOptimizing mesh \"%s\" detail level %i.\r\n",om->name,om->details[j].size));

         TSMesh * mesh = om->details[j].mesh->tsMesh;
         Vector<U32> & smooth = om->details[j].mesh->smoothingGroups;
         Vector<U32> & remap = om->details[j].mesh->remap;

         // collapse vertices
         collapseVertices(mesh,smooth,remap);

         // need to sprinkle these here and there to avoid crashes...
         if (isError()) return;

         // now that verts are collapsed, delete any trivial facees
         for (S32 k=0; k<mesh->primitives.size(); k++)
         {
            TSDrawPrimitive & face = mesh->primitives[k];
            U32 start = face.start;
            U32 idx0 = mesh->indices[start + 0];
            U32 idx1 = mesh->indices[start + 1];
            U32 idx2 = mesh->indices[start + 2];
            if (idx0==idx1 || idx1==idx2 || idx0==idx2)
            {
               mesh->indices.erase(face.start);
               mesh->indices.erase(face.start);
               mesh->indices.erase(face.start);
               for (S32 l=0; l<mesh->primitives.size(); l++)
                  if (mesh->primitives[l].start >= start)
                     mesh->primitives[l].start -= 3;
               mesh->primitives.erase(k);
               k--;
            }
         }

         // make detail levels share verts when possible
         shareVerts(om,j,pShape);

         //
         if (om->details[j].mesh->sortedObject)
            continue;

         // strip
         stripify(mesh->primitives,mesh->indices);

         // need to sprinkle these here and there to avoid crashes...
         if (isError()) return;
      }
   }

   // need to sprinkle these here and there to avoid crashes...
   if (isError()) return;

   // optimize decals...
   for (i=0; i<decalObjectList.size(); i++)
   {
      DecalObjectMimic * dom = decalObjectList[i];
      for (j=0; j<dom->numDetails; j++)
      {
         if (!dom->details[j].decalMesh)
            continue;

         printDump(PDObjectStateDetails,avar("\r\nOptimizing decal \"%s\" on mesh \"%s\" on detail #%i.\r\n",dom->decalNode->GetName(),dom->targetObject->name,j));

         TSDecalMesh * decalMesh = dom->details[j].decalMesh->tsMesh;
         Vector<U32> & remap = dom->targetObject->details[j].mesh->remap; // old vert idx --> new vert idx

         for (k=0; k<decalMesh->indices.size(); k++)
            decalMesh->indices[k] = remap[decalMesh->indices[k]];

         // strip
         stripify(decalMesh->primitives,decalMesh->indices);

         // adjust startPrimitive array
         // at this point the material index of the decalMesh primitives actually refers to frame number
         // use this to update decalMesh->startPrimitive array (and put matIndex back to what it should
         // be while we're at it -- although it doesn't really matter, isn't used)
         for (k=0; k<decalMesh->startPrimitive.size(); k++)
            decalMesh->startPrimitive[k] = 0;
         for (k=0; k<decalMesh->primitives.size(); k++)
         {
            if (decalMesh->startPrimitive[ decalMesh->primitives[k].matIndex&TSDrawPrimitive::MaterialMask ] == -1)
               decalMesh->startPrimitive[ decalMesh->primitives[k].matIndex&TSDrawPrimitive::MaterialMask ] = k;
            decalMesh->primitives[k].matIndex &= ~TSDrawPrimitive::MaterialMask;
            decalMesh->primitives[k].matIndex |= decalMesh->materialIndex;
         }
      }
   }

   // need to sprinkle these here and there to avoid crashes...
   if (isError()) return;

   // optimize skins...
   Vector<U32> remap;
   for (i=skins.size()-1; i>=0; i--)
   {
      SkinMimic * skin = skins[i];
      TSSkinMesh * skinMesh = skin->skinMesh;
      Vector<U32> & smooth = skin->smoothingGroups;
      Vector<U32> * vertId = &skin->vertId;

      // first make sure we have no missing verts...
      for (j=1; j<skinMesh->vertexIndex.size(); j++)
      {
         if (skinMesh->vertexIndex[j]-skinMesh->vertexIndex[j-1]>1)
         {
            setExportError(avar("Vertex %i missing weight on skin \"%s\"",skinMesh->vertexIndex[j]+1,skin->skinNode->GetName()));
            return;
         }
      }

      // start optimizing this skin...
      printDump(PDObjectStateDetails,avar("\r\nOptimizing skin mesh \"%s\" detail level %i.\r\n",skin->skinNode->GetName(),skin->detailSize));

      // set up skinMesh for optimizing (copy initial values into run-time slot)
      skinMesh->verts = skinMesh->initialVerts;
      skinMesh->norms = skinMesh->initialNorms;

      // we don't respect smoothing groups on skins
      for (j=0;j<smooth.size();j++)
         smooth[j]=0;

      // collapse vertices
      collapseVertices(skinMesh,smooth,remap,vertId);

      // share verts...
      shareVerts(skin,remap,pShape);

      // copy verts and normals back to initial arrays and clear out run-time versions
      skinMesh->initialVerts = skinMesh->verts;
      skinMesh->initialNorms = skinMesh->norms;
      skinMesh->verts.clear();
      skinMesh->norms.clear();

      // remap some information
      for (j=0; j<skinMesh->vertexIndex.size(); j++)
         skinMesh->vertexIndex[j] = remap[skinMesh->vertexIndex[j]];
      for (j=(S32)skinMesh->vertexIndex.size()-1; j>0; j--)
      {
         for (k=0; k<j; k++)
         {
            if (skinMesh->vertexIndex[k]==skinMesh->vertexIndex[j] && skinMesh->boneIndex[k]==skinMesh->boneIndex[j])
            {
               if (mFabs(skinMesh->weight[j]-skinMesh->weight[k])>0.01f)
               {
                  setExportError("Assertion failed when collapsing vertices on skin (1)");
                  return;
               }

               // vertex and bone index for kth and jth tuple match...merge them
               skinMesh->weight.erase(j);
               skinMesh->vertexIndex.erase(j);
               skinMesh->boneIndex.erase(j);
               break; // out of k loop
            }
         }
      }

      // re-sort the vertexIndex, boneIndex, weight lists by vertex and bone, respectively...
      for (j=0; j<(S32)skinMesh->vertexIndex.size()-1; j++)
      {
         for (k=j+1; k<skinMesh->vertexIndex.size(); k++)
         {
            if ((skinMesh->vertexIndex[k]<skinMesh->vertexIndex[j]) || (skinMesh->vertexIndex[k]==skinMesh->vertexIndex[j] && skinMesh->boneIndex[k]<skinMesh->boneIndex[j]))
            {
               // swap
               S32 tmp = skinMesh->vertexIndex[k];
               skinMesh->vertexIndex[k] = skinMesh->vertexIndex[j];
               skinMesh->vertexIndex[j] = tmp;
               tmp = skinMesh->boneIndex[k];
               skinMesh->boneIndex[k] = skinMesh->boneIndex[j];
               skinMesh->boneIndex[j] = tmp;
               F32 tmp2 = skinMesh->weight[k];
               skinMesh->weight[k] = skinMesh->weight[j];
               skinMesh->weight[j] = tmp2;
            }
         }
      }

      // share bones...
      shareBones(skin,pShape);

      // strip
      stripify(skinMesh->primitives,skinMesh->indices);
   }

   // convert merge indices into the form we need:
   // 1. only include merge indices for verts not in our child detail
   // 2. make sure mergeIndex points to vert in our child detail
   //    (keep following the trail till we get there)
   fixupMergeIndices(pShape);
}

void ShapeMimic::fixupMergeIndices(TSShape * pShape)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   S32 i,j,k;

   Vector<bool> hasChild;
   hasChild.setSize(pShape->meshes.size());
   for (i=0;i<hasChild.size();i++)
      hasChild[i]=false; // till we find the child

   // convert merge indices into the form we need:
   // 1. only include merge indices for verts not in our child detail
   // 2. make sure mergeIndex points to vert in our child detail
   //    (keep following the trail till we get there)
   for (i=0; i<objectList.size(); i++)
   {
      ObjectMimic * om = objectList[i];
      for (j=0; j<om->numDetails; j++)
      {
         if (!om->details[j].mesh)
            continue;

         TSMesh * childMesh = om->isSkin ? om->details[j].mesh->skinMimic->skinMesh : om->details[j].mesh->tsMesh;
         if (childMesh->parentMesh<0)
            // we're here to work on parents, not children
            continue;
         TSMesh * parentMesh = pShape->meshes[childMesh->parentMesh];
         S32 numParentVerts = om->isSkin ? ((TSSkinMesh*)parentMesh)->initialVerts.size() : parentMesh->verts.size();
         hasChild[childMesh->parentMesh] = true;
         if (numParentVerts!=parentMesh->mergeIndices.size() && parentMesh->mergeIndices.size()!=0)
         {
            setExportError("Assertion failed during vertex merge");
            return;
         }

         // first, make sure each vert in parent mesh that isn't in child mesh has a
         // mergeIndex that maps there (follow the chain till we get to chid mesh)
         S32 numChildVerts = om->isSkin ? ((TSSkinMesh*)childMesh)->initialVerts.size() : childMesh->verts.size();
         if (numChildVerts==numParentVerts)
            // no merge needed
            parentMesh->mergeIndices.clear();
         else
         {
            // quick check to make sure merge is still ok
            //testMerge(pShape,om,j,0);

            for (k=numChildVerts; k<numParentVerts; k++)
            {
               #define self(a,b) a[a[b]]
               while (parentMesh->mergeIndices[k] >= numChildVerts &&
                      parentMesh->mergeIndices[k]!=self(parentMesh->mergeIndices,k))
                  parentMesh->mergeIndices[k] = self(parentMesh->mergeIndices,k);
            }
            // now move mergeIndices down so that we only keep mergeIndices for those verts not found in child
            dMemmove(&parentMesh->mergeIndices[0],&parentMesh->mergeIndices[numChildVerts],(parentMesh->mergeIndices.size()-numChildVerts)*sizeof(parentMesh->mergeIndices[0]));
            parentMesh->mergeIndices.setSize(numParentVerts-numChildVerts);

            // one more check -- search the verts unique to the parent...if any of them are at the same
            // position as the child, make sure that they merge to themself
            Vector<Point3F> & childVerts  = om->isSkin ? ((TSSkinMesh*)childMesh)->initialVerts : childMesh->verts;
            Vector<Point3F> & parentVerts = om->isSkin ? ((TSSkinMesh*)parentMesh)->initialVerts : parentMesh->verts;
            for (k=childVerts.size(); k<parentVerts.size(); k++)
            {
               for (S32 l=0; l<childVerts.size(); l++)
               {
                  Point3F delta = parentVerts[k]-childVerts[l];
                  if (mDot(delta,delta)<10E-20f)
                  {
                     // merge to ourself?
                     if (parentMesh->mergeIndices[k-childVerts.size()] != k)
                     {
                        // merging to different vert...bad
                        setExportError("Assertion failed during vertex merge");
                        return;
                     }
                  }
               }
            }

            // yet one more check -- make sure all verts that merge someplace either merge
            // to themselv or to a legit destination
            testMerge(pShape,om,j,numChildVerts);
         }
      }
   }

   // make sure childless meshes have no mergeIndices
   for (i=0; i<objectList.size(); i++)
   {
      ObjectMimic * om = objectList[i];
      for (j=0; j<om->numDetails; j++)
      {
         if (!om->details[j].mesh)
            continue;

         TSMesh * mesh = om->isSkin ? om->details[j].mesh->skinMimic->skinMesh : om->details[j].mesh->tsMesh;
         if (!hasChild[om->details[j].mesh->meshNum])
            pShape->meshes[om->details[j].mesh->meshNum]->mergeIndices.clear();
      }
   }
}

void ShapeMimic::testMerge(TSShape * pShape, ObjectMimic * om, S32 dl, S32 mergeRemoved)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   MeshMimic * mm = om->details[dl].mesh;
   TSMesh * childMesh = om->isSkin ? mm->skinMimic->skinMesh : mm->tsMesh;
   TSMesh * parentMesh = childMesh;
   if (childMesh->parentMesh>=0)
      parentMesh = pShape->meshes[childMesh->parentMesh];
   else
      // ok, forget the child, just check out the parent
      childMesh = NULL;
   Vector<Point3F> emptyList;
   Vector<Point3F> & childVerts  = childMesh ? (om->isSkin ? ((TSSkinMesh*)childMesh)->initialVerts : childMesh->verts) : emptyList;
   Vector<Point3F> & parentVerts = om->isSkin ? ((TSSkinMesh*)parentMesh)->initialVerts : parentMesh->verts;
   if (mergeRemoved + parentMesh->mergeIndices.size() != parentVerts.size())
   {
      setExportError("Assertion failed");
      return;
   }
   Vector<Point3F> mergeFrom, mergeTo;
   {
      MultiResMimic * mrm = getMultiRes(mm->pNode);

      // merge from and mergeTo vectors are in object space before applying object offset
      // apply object offset to those points
      mergeFrom.setSize(mrm->mergeFrom.size());
      mergeTo.setSize(mrm->mergeTo.size());
      Matrix3 objectOffset3 = mm->objectOffset;
      MatrixF objectOffset;
      convertToMatrixF(objectOffset3,objectOffset);
      for (S32 k=0; k<mrm->mergeFrom.size(); k++)
      {
         objectOffset.mulP(mrm->mergeFrom[k],&mergeFrom[k]);
         objectOffset.mulP(mrm->mergeTo[k],&mergeTo[k]);
      }
      mrm=NULL; // don't want to use anything in this for the rest of this method...crash if we accidentally do
   }
   for (S32 k=0; k<parentMesh->mergeIndices.size(); k++)
   {
      Point3F vert = parentVerts[k+mergeRemoved];

      for (S32 i=childVerts.size(); i<parentVerts.size(); i++)
      {
         Point3F delta1 = parentVerts[i]-vert;
         if (mDot(delta1,delta1)<0.00000001f)
         {
            // same location, check to make sure it's the same merge
            Point3F delta2 = parentVerts[parentMesh->mergeIndices[k]] - parentVerts[parentMesh->mergeIndices[i-mergeRemoved]];
            if (mDot(delta2,delta2)>0.0001f)
            {
               setExportError("Assertion failed");
               return;
            }
         }
      }

      if (parentMesh->mergeIndices[k]==k+mergeRemoved)
         // ourself
         continue;

      while (1)
      {
         S32 idx = findClosestMatch(vert,mergeFrom);
         vert = mergeTo[idx];
         Point3F delta = vert - parentVerts[parentMesh->mergeIndices[k]];
         if (mDot(delta,delta)<0.001f)
            // ok
            break;
         S32 v;
         for (v=childVerts.size(); v<parentVerts.size(); v++)
         {
            Point3F delta = vert-parentVerts[v];
            if (mDot(delta,delta)<0.001f)
               // ok, found it
               break;
         }
         if (v==parentVerts.size())
         {
            // made it to child mesh without finding where we merged
            setExportError("Assertion failed");
            return;
         }
      }
   }
}

bool ShapeMimic::vertexSame(Point3F & v1, Point3F & v2, Point2F & tv1, Point2F & tv2, U32 smooth1, U32 smooth2, U32 idx1, U32 idx2, Vector<U32> * vertId)
{
   if (mFabs(v1.x-v2.x)>sameVertTOL || mFabs(v1.y-v2.y)>sameVertTOL || mFabs(v1.z-v2.z)>sameVertTOL ||
       mFabs(tv1.x-tv2.x)>sameTVertTOL || mFabs(tv1.y-tv2.y)>sameTVertTOL || smooth1 != smooth2)
   {
      return false;
   }

   if (!vertId || (*vertId)[idx1]==(*vertId)[idx2])
      return true;

   // At this point, we know that the vert has the same tvert and vert coords, but that max thinks of
   // it as a different vertex...for non-skin meshes we don't care, but we won't get _this_ far in that case
   // (we'd have returned true because vertId==NULL for non-skin meshes).

   // At some point, we may want some way to determine whether we can delete this vertex if we get this far...
   // e.g., if both verts have same set of weights...

   return false;
}

void ShapeMimic::collapseVertices(TSMesh * mesh, Vector<U32> & smooth, Vector<U32> & remap, Vector<U32> * vertId)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   if (mesh->verts.size() != mesh->norms.size())
   {
      setExportError("Assertion failed when collapsing vertices (2)");
      return;
   }

   printDump(PDObjectStateDetails,avar("%i verts before joining verts\r\n",mesh->verts.size()));

   S32 i,j;

   // set up remap
   remap.setSize(mesh->vertsPerFrame);
   for (i=0; i<remap.size(); i++)
      remap[i]=i;

   for (i=(S32)mesh->vertsPerFrame-1; i>0; i--)
   {
      // try to find a vertex earlier in the list that matches this one
      for (j=i-1; j>=0; j--)
      {
         //------------------------------------------
         // same location, tvert, smoothing group, vert id (if passed)?
         if (!vertexSame(mesh->verts[i],mesh->verts[j],mesh->tverts[i],mesh->tverts[j],smooth[i],smooth[j],i,j,vertId))
            continue;

         //------------------------------------------
         // ok, but are we the same for all frames and matFrames too?
         S32 k,l;
         for (k=1; k<mesh->numFrames; k++)
         {
            S32 startVert = k * mesh->vertsPerFrame;
            for (l=1; l<mesh->numMatFrames; l++)
            {
               S32 startTVert = l * mesh->vertsPerFrame;
               if (!vertexSame(mesh->verts[i+startVert],mesh->verts[j+startVert],mesh->tverts[i+startTVert],mesh->tverts[j+startTVert],smooth[i],smooth[j],i,j,vertId))
                  break;
            }
            if (l!=mesh->numMatFrames)
               break;
         }
         if (k!=mesh->numFrames)
            // not same throughout
            continue;

         //------------------------------------------
         // ok, but do we have the same merge pattern?
         if (!mesh->mergeIndices.empty())
         {
            U16 checkI=i;
            U16 checkJ=j;
            while (checkI!=checkJ && mesh->mergeIndices[checkI]!=checkI && mesh->mergeIndices[checkJ]!=checkJ)
            {
               checkI=mesh->mergeIndices[checkI];
               checkJ=mesh->mergeIndices[checkJ];
//             if (!vertexSame(mesh->verts[checkI],mesh->verts[checkJ],mesh->tverts[checkI],mesh->tverts[checkJ],smooth[checkI],smooth[checkI],checkI,checkJ,vertId))
               if (!vertexSame(mesh->verts[checkI],mesh->verts[checkJ],mesh->tverts[checkI],mesh->tverts[checkJ],-1,-1,checkI,checkJ,vertId))
                  break;
            }
            // ok, either checkI and checkJ converged, or they are both stationary
            // either way, just check to see if they reached an equivalent vertex
//          if (!vertexSame(mesh->verts[checkI],mesh->verts[checkJ],mesh->tverts[checkI],mesh->tverts[checkJ],smooth[checkI],smooth[checkI],checkI,checkJ,vertId))
            if (!vertexSame(mesh->verts[checkI],mesh->verts[checkJ],mesh->tverts[checkI],mesh->tverts[checkJ],-1,-1,checkI,checkJ,vertId))
               // different vertices
               continue;
         }

         //------------------------------------------
         // alright, vertex i and j are the same...get rid of vertex i (i>j)
         if (i<=j)
         {
            setExportError("Assertion failed when collapsing vertex (3)");
            return;
         }
         for (k=0; k<mesh->indices.size(); k++)
         {
            if (mesh->indices[k] == i)
               mesh->indices[k] = j;
            else if (mesh->indices[k]>i)
               mesh->indices[k]--;
         }
         for (k=0; k<mesh->mergeIndices.size(); k++)
         {
            if (mesh->mergeIndices[k] == i)
               mesh->mergeIndices[k] = j;
            else if (mesh->mergeIndices[k]>i)
               mesh->mergeIndices[k]--;
         }
         if (!mesh->mergeIndices.empty())
            mesh->mergeIndices.erase(i);

         for (k=mesh->numFrames-1; k>=0; k--)
         {
            S32 startVert = mesh->vertsPerFrame * k;
            mesh->verts.erase(i + startVert);
            mesh->norms.erase(i + startVert);
         }
         for (k=mesh->numMatFrames-1; k>=0; k--)
         {
            S32 startTVert = mesh->vertsPerFrame * k;
            mesh->tverts.erase(i + startTVert);
         }
         if (vertId)
            vertId->erase(i);
         smooth.erase(i);
         mesh->vertsPerFrame--;

         // update remap -- we're getting rid of vertex i and replacing it with vertex j
         // any vertex currently mapped to i should be replaced by j, but we know all verts
         // before i are there original selves (since i-loop is going backwards) so we can start
         // at i...also shift indices greater than i
         for (k=i;k<remap.size();k++)
            if (remap[k]==i)
               remap[k]=j;
            else if (remap[k]>i)
               remap[k]--;

         break; // out of j loop
      }
   }

   // re-generate normals since vertex sharing has changed...
   computeNormals(mesh->primitives,mesh->indices,mesh->verts,mesh->norms,smooth,mesh->vertsPerFrame,mesh->numFrames,&mesh->mergeIndices);

   printDump(PDObjectStateDetails,avar("%i verts after joining verts\r\n",mesh->verts.size()));

   if (mesh->verts.size() * mesh->numMatFrames != mesh->tverts.size() * mesh->numFrames)
      setExportError("ShapeMimic::collapseVertices (3)");
   else if (mesh->verts.size() != mesh->norms.size())
      setExportError("ShapeMimic::collapseVertices (4)");
}


//----------------------------------------------------------------------------
// compute normals, account for smoothing groups and vertices that are same
// location but different vertex anyway (two verts w/ same smoothing group
// and same location should get same normal)
void ShapeMimic::computeNormals(Vector<TSDrawPrimitive> & faces, Vector<U16> & indices, Vector<Point3F> & verts, Vector<Point3F> & norms, Vector<U32> & smooth, S32 vertsPerFrame, S32 numFrames, Vector<U16> * mergeIndices)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   if (vertsPerFrame * numFrames != verts.size() || vertsPerFrame!=smooth.size())
   {
      setExportError("Assertion failed:  vertex number mismatch");
      return;
   }
   if (mergeIndices && !mergeIndices->empty() && mergeIndices->size() != vertsPerFrame)
   {
      setExportError("Assertion failed:  vertex number mismatch");
      return;
   }

   S32 i,j;
   Vector<S32> counts;
   counts.setSize(verts.size());
   norms.setSize(verts.size());
   for (i=0; i<verts.size(); i++)
   {
      counts[i]=0;
      norms[i].set(0.0f,0.0f,0.0f);
   }
   for (S32 frameNum = 0; frameNum<numFrames; frameNum++)
   {
      S32 startVert = frameNum * vertsPerFrame;
      for (i=0; i<faces.size(); i++)
      {
         TSDrawPrimitive & tsFace = faces[i];
         if ((tsFace.matIndex & TSDrawPrimitive::TypeMask) != TSDrawPrimitive::Triangles)
         {
            setExportError("Assertion error while computing normals");
            return;
         }
         // find the normal to this face
         S32 idx0 = indices[tsFace.start+0];
         S32 idx1 = indices[tsFace.start+1];
         S32 idx2 = indices[tsFace.start+2];
         Point3F v0 = verts[startVert+idx0];
         Point3F v1 = verts[startVert+idx1];
         Point3F v2 = verts[startVert+idx2];

         Point3F n,v20,v10;
         v20 = v2-v0;
         if (mDot(v20,v20)>0.0000001f)
            v20.normalize();
         v10 = v1-v0;
         if (mDot(v10,v10)>0.0000001f)
            v10.normalize();
         mCross(v20,v10,&n);
         if (mDot(n,n) > 0.0000001f)
         {
            n.normalize();
            for (S32 j=0; j<vertsPerFrame; j++)
            {
               Point3F vj = verts[startVert+j];
               if (mFabs(v0.x-vj.x) < sameVertTOL && mFabs(v0.y-vj.y) < sameVertTOL && mFabs(v0.z-vj.z) < sameVertTOL && (smooth[idx0]&smooth[j] || smooth[idx0]==smooth[j]))
               {
                  norms[startVert+j] += n;
                  counts[startVert+j]++;
               }
               if (mFabs(v1.x-vj.x) < sameVertTOL && mFabs(v1.y-vj.y) < sameVertTOL && mFabs(v1.z-vj.z) < sameVertTOL && (smooth[idx1]&smooth[j] || smooth[idx1]==smooth[j]))
               {
                  norms[startVert+j] += n;
                  counts[startVert+j]++;
               }
               if (mFabs(v2.x-vj.x) < sameVertTOL && mFabs(v2.y-vj.y) < sameVertTOL && mFabs(v2.z-vj.z) < sameVertTOL && (smooth[idx2]&smooth[j] || smooth[idx2]==smooth[j]))
               {
                  norms[startVert+j] += n;
                  counts[startVert+j]++;
               }
            }
         }
      }
   }
   // now average normals...
   for (i=0; i<norms.size(); i++)
   {
      if (counts[i] && mDot(norms[i],norms[i])>0.0000001f)
         norms[i].normalize();
   }
   // for verts w/o a normal, search for someone with same vert location and smoothing group
   for (i=0; i<counts.size(); i++)
   {
      if (!counts[i])
      {
         for (j=0; j<verts.size(); j++)
         {
            if (!counts[j])
               continue;
            Point3F delta = verts[i]-verts[j];
            if (mFabs(delta.x)>sameVertTOL || mFabs(delta.y)>sameVertTOL || mFabs(delta.z)>sameVertTOL)
               continue;
            if (smooth[i]==smooth[j])// || (smooth[i]&smooth[j])!=0)
            {
               norms[i]=norms[j];
               counts[i]++;
               break;
            }
         }
      }
   }
   // if above didn't work, try anyone in same location
   for (i=0; i<counts.size(); i++)
   {
      if (!counts[i])
      {
         // search for someone with same vert location and smoothing group
         for (j=0; j<verts.size(); j++)
         {
            if (!counts[j])
               continue;
            Point3F delta = verts[i]-verts[j];
            if (mFabs(delta.x)>sameVertTOL || mFabs(delta.y)>sameVertTOL || mFabs(delta.z)>sameVertTOL)
               continue;
            norms[i]=norms[j];
            counts[i]++;
            break;
         }
      }
   }
   // just in case, set any normal still without a value to 0,0,1
   for (i=0; i<counts.size(); i++)
      if (!counts[i])
         norms[i].set(0,0,1);
}

void ShapeMimic::computeNormals(Vector<TSDrawPrimitive> & faces, Vector<U16> & indices, Vector<Point3> & verts, Vector<Point3F> & norms, Vector<U32> & smooth, S32 vertsPerFrame, S32 numFrames)
{
   computeNormals(faces,indices, (Vector<Point3F>&)verts,norms,smooth,vertsPerFrame,numFrames, NULL);
}

extern void nvStripWrap(Vector<TSDrawPrimitive> &, Vector<U16> &, S32, S32);

void ShapeMimic::stripify(Vector<TSDrawPrimitive> & primitives, Vector<U16> & indices)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;


   if (primitives.empty() || indices.empty())
      // shouldn't really have empty meshes...but no harm, no foul (we would, however, cause
      // problems in the stripper with empty meshes).
      return;

   S32 i;
   Vector<TSDrawPrimitive> faces = primitives;
   Vector<U16> faceIndices = indices;

   // in:  primitives better just be faces and better use indexes
   for (i=0; i<primitives.size(); i++)
   {
      if (primitives[i].matIndex == -1)
      {
         setExportError("Assertion failed when sripping -- negative material index");
         return;
      }
      if ( (primitives[i].matIndex & ~(TSDrawPrimitive::NoMaterial^TSDrawPrimitive::MaterialMask)) != (TSDrawPrimitive::Triangles|TSDrawPrimitive::Indexed) || primitives[i].numElements!=3)
      {
         setExportError("Assertion failed when stripifying (1)");
         return;
      }
   }

   printDump(PDObjectStateDetails,avar("%i faces before stripping\r\n",faces.size()));

   // do a quick and dirty pass at getting strips -- sometimes proves better
   // Full pass is based on the Hoppes algorithm for optimizing (i.e., minimizing)
   // cache misses (presented at Siggraph 99).  The quick and dirty scheme is the
   // same without look ahead simulation (still has vertex cache considerations).
   Stripper quickAndDirty(primitives,indices);
   quickAndDirty.setLimitStripLength(false);
   quickAndDirty.makeStrips();
   U32 qdMisses = quickAndDirty.getCacheMisses();

#ifdef QUICK_STRIP

   quickAndDirty.getStrips(primitives,indices);
   if (dumpMask & PDObjectStateDetails)
   {
      printDump(PDObjectStateDetails,avar("Using %s stripping method.\r\n","quick and dirty"));
      float len = 0.0f;
      S32 hi = -1;
      S32 lo = -1;
      for (i=0; i<primitives.size(); i++)
      {
         len += primitives[i].numElements;
         if (primitives[i].numElements > hi)
            hi = primitives[i].numElements;
         if (lo==-1 || primitives[i].numElements < lo)
            lo = primitives[i].numElements;
      }
      S32 reversals = len - (faces.size() + primitives.size() * 2); // no. of times we needed to reverse order of face by sending extra vert
      if (!primitives.empty())
         len *= 1.0f / (F32)primitives.size();
      printDump(PDObjectStateDetails,avar("%i strips with average length %3.2f (range %i to %i) and %i reversals\r\n",primitives.size(),len,lo,hi,reversals));
      printDump(PDObjectStateDetails,avar("Results in %i cache misses\r\n",qdMisses));
   }

#elif defined(NV_STRIP)

   nvStripWrap(primitives,indices,16,1);

   if (dumpMask & PDObjectStateDetails)
   {
      printDump(PDObjectStateDetails,avar("Using %s stripping method.\r\n","NVidia"));
      float len = 0.0f;
      S32 hi = -1;
      S32 lo = -1;
      for (i=0; i<primitives.size(); i++)
      {
         len += primitives[i].numElements;
         if (primitives[i].numElements > hi)
            hi = primitives[i].numElements;
         if (lo==-1 || primitives[i].numElements < lo)
            lo = primitives[i].numElements;
      }
      S32 reversals = len - (faces.size() + primitives.size() * 2); // no. of times we needed to reverse order of face by sending extra vert
      if (!primitives.empty())
         len *= 1.0f / (F32)primitives.size();
      printDump(PDObjectStateDetails,avar("%i strips with average length %3.2f (range %i to %i) and %i reversals\r\n",primitives.size(),len,lo,hi,reversals));
   }

#else

   Stripper stripper(primitives,indices);
   stripper.makeStrips();
   U32 misses = stripper.getCacheMisses();

   bool useHoppes = misses<qdMisses;
   if (useHoppes)
      stripper.getStrips(primitives,indices);
   else
      quickAndDirty.getStrips(primitives,indices);

   if (stripper.isError())
   {
      setExportError(stripper.getError());
      return;
   }

   if (dumpMask & PDObjectStateDetails)
   {
      printDump(PDObjectStateDetails,avar("Using %s stripping method.\r\n",useHoppes ? "look ahead simulation" : "quick and dirty"));
      float len = 0.0f;
      S32 hi = -1;
      S32 lo = -1;
      for (i=0; i<primitives.size(); i++)
      {
         len += primitives[i].numElements;
         if (primitives[i].numElements > hi)
            hi = primitives[i].numElements;
         if (lo==-1 || primitives[i].numElements < lo)
            lo = primitives[i].numElements;
      }
      S32 reversals = len - (faces.size() + primitives.size() * 2); // no. of times we needed to reverse order of face by sending extra vert
      if (!primitives.empty())
         len *= 1.0f / (F32)primitives.size();
      printDump(PDObjectStateDetails,avar("%i strips with average length %3.2f (range %i to %i) and %i reversals\r\n",primitives.size(),len,lo,hi,reversals));
      if (useHoppes)
         printDump(PDObjectStateDetails,avar("Results in %i cache misses versus %i with quick and dirty scheme\r\n",misses,qdMisses));
      else
         printDump(PDObjectStateDetails,avar("Results in %i cache misses versus %i with look ahead simulation\r\n",qdMisses,misses));
   }
#endif
}

void ShapeMimic::convertSortObjects(TSShape * pShape)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   // go through meshes and convert sortObjects when we find them
   for (S32 i=0; i<objectList.size(); i++)
   {
      ObjectMimic * om = objectList[i];
      for (S32 j=0; j<om->numDetails; j++)
      {
         if (!om->details[j].mesh || !om->details[j].mesh->sortedObject)
            continue;

         TSSortedMesh * sortMesh = (TSSortedMesh*)om->details[j].mesh->tsMesh;

         // get sort data from user properties...
         INode * pNode = om->details[j].mesh->pNode;
         S32 numBigFaces, maxDepth;
         if (!pNode->GetUserPropInt("SORT::NUM_BIG_FACES",numBigFaces))
            numBigFaces = 0; // default value...
         if (!pNode->GetUserPropInt("SORT::MAX_DEPTH",maxDepth))
            maxDepth = 2;
         S32 tmp;
         if (!pNode->GetUserPropBool("SORT::WRITE_Z",tmp))
            tmp = 0;
         sortMesh->alwaysWriteDepth = tmp;
         if (!pNode->GetUserPropBool("SORT::Z_LAYER_UP",tmp))
            tmp = false;
         bool zLayerUp = tmp;
         if (!pNode->GetUserPropBool("SORT::Z_LAYER_DOWN",tmp))
            tmp = false;
         bool zLayerDown = tmp;

         if (zLayerUp && zLayerDown)
         {
            setExportError("Cannot use both Z_LAYER_UP and Z_LAYER_DOWN -- make up your mind.");
            return;
         }

         if (sortMesh->primitives.size() > MAX_TS_SET_SIZE)
         {
            setExportError("Too many faces on sort object:  up MAX_TS_SET_SIZE and recompile exporter");
            return;
         }

         TranslucentSort::generateSortedMesh(sortMesh,numBigFaces,maxDepth,zLayerUp,zLayerDown);
         S32 saveNumFrames = sortMesh->numFrames;
         sortMesh->vertsPerFrame = sortMesh->verts.size();
         Vector<U32> remap;
         Vector<U32> smooth;
         smooth.setSize(sortMesh->verts.size());
         for (S32 k=0; k<smooth.size(); k++)
            smooth[k]=0;
         collapseVertices(sortMesh,smooth,remap);
         sortMesh->numFrames = saveNumFrames;
         sortMesh->vertsPerFrame = 0; // not used
      }
   }
}

F32 distFromPoly(Point3F & v0, Point3F & v1, Point3F & v2, Point3F v3)
{
   // find distance of v0 from poly v1,v2,v3
   // don't care about v1,v2,v3 winding

   // check verts
   F32 d, dist;
   dist = mSqrt(mDot(v0-v1,v0-v1));
   d = mSqrt(mDot(v0-v2,v0-v2));
   if (d<dist)
      dist = d;
   d = mSqrt(mDot(v0-v3,v0-v3));
   if (d<dist)
      dist = d;

   Point3F v12=v1-v2;
   Point3F v23=v2-v3;
   Point3F v31=v3-v1;
   Point3F n;
   mCross(v12,v23,&n);
   if (mDot(n,n)>0.0000001f)
      n.normalize();
   else
      return dist;

   bool inTri=false;
   F32 inTriDist;
   Point3F varray[3];
   varray[0]=v1;
   varray[1]=v2;
   varray[2]=v3;
   if (pointInPoly(v0,n,varray,3))
   {
      // this should be it...but we'll keep going anyway
      dist = mFabs(mDot(n,v0-v1));
      inTri=true;
      inTriDist = dist;
   }

   v12.normalize();
   v23.normalize();
   v31.normalize();

   // find closest point along edge v12
   F32 t = mDot(v0-v2,v12);
   if (t>0 && t<1)
   {
      Point3F test = v12;
      test *= t;
      test += v2;
      d = mSqrt(mDot(v0-test,v0-test));
      if (d<dist)
         dist=d;
   }

   // find closest point along edge v23
   t = mDot(v0-v3,v23);
   if (t>0 && t<1)
   {
      Point3F test = v23;
      test *= t;
      test += v3;
      d = mSqrt(mDot(v0-test,v0-test));
      if (d<dist)
         dist=d;
   }

   // find closest point along edge v31
   t = mDot(v0-v1,v31);
   if (t>0 && t<1)
   {
      Point3F test = v31;
      test *= t;
      test += v1;
      d = mSqrt(mDot(v0-test,v0-test));
      if (d<dist)
         dist=d;
   }

   if (inTri && dist<inTriDist-0.01f)
   {
      AssertFatal(1,"Doh");
      dist=inTriDist;
   }

   return dist;
}

F32 ShapeMimic::findMaxDistance(TSMesh * loMesh, TSMesh * hiMesh, F32 & total, S32 & count)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return 0.0f;

   // for each vertex of hiMesh, find distance to loMesh
   // return the max of these distances (i.e., distance of farthest
   // hiMesh vertex from loMesh).

   Vector<Point3F> & loVerts = loMesh->getMeshType()==TSMesh::SkinMeshType ? ((TSSkinMesh*)loMesh)->initialVerts : loMesh->verts;
   Vector<Point3F> & hiVerts = hiMesh->getMeshType()==TSMesh::SkinMeshType ? ((TSSkinMesh*)hiMesh)->initialVerts : hiMesh->verts;

   F32 maxDist = 0.0f;
   total = 0;
   count = 0;
   // loop through primitives so that stray verts don't mess us up...
   for (S32 ii=0; ii<hiMesh->primitives.size(); ii++)
   {
      TSDrawPrimitive & hiDraw = hiMesh->primitives[ii];
      for (S32 i=0; i<hiDraw.numElements; i++)
      {
         Point3F v0 = hiVerts[hiMesh->indices[hiDraw.start+i]];
         F32 closestDist = 10E30f;
         bool foundSomething = false;
         for (S32 j=0; j<loMesh->primitives.size(); j++)
         {
            TSDrawPrimitive & draw = loMesh->primitives[j];
            Point3F v1;
            Point3F v2;
            Point3F v3;
            S32 numElements = 0;
            while (numElements<draw.numElements)
            {
               if ((draw.matIndex & TSDrawPrimitive::TypeMask) == TSDrawPrimitive::Triangles || numElements==0)
               {
                  v1 = loVerts[loMesh->indices[draw.start + numElements + 0]];
                  v2 = loVerts[loMesh->indices[draw.start + numElements + 1]];
                  v3 = loVerts[loMesh->indices[draw.start + numElements + 2]];
                  numElements += 3;
               }
               else
               {
                  // ignore winding
                  v1 = v2;
                  v2 = v3;
                  v3 = loVerts[loMesh->indices[draw.start+numElements]];
                  numElements++;
               }
               foundSomething=true;

               // find dist of v0 from v1,v2,v3
               F32 dist = distFromPoly(v0,v1,v2,v3);
               if (dist<closestDist)
                  closestDist = dist;
            }
         }
         if (foundSomething)
         {
            if (closestDist>maxDist)
               maxDist = closestDist;
            total += closestDist;
            count++;
         }
      }
   }
   return maxDist;
}
