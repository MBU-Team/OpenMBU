//-----------------------------------------------------------------------------
// Collada-2-DTS
// Copyright (C) 2008 GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _COLLADA_APPNODE_H_
#define _COLLADA_APPNODE_H_

#ifndef _TDICTIONARY_H_
#include "core/tDictionary.h"
#endif
#ifndef _APPNODE_H_
#include "ts/loader/appNode.h"
#endif
#ifndef _COLLADA_EXTENSIONS_H_
#include "ts/collada/colladaExtensions.h"
#endif

class ColladaAppNode : public AppNode
{
   typedef AppNode Parent;
   friend class ColladaAppMesh;

   MatrixF getTransform(F32 time);
   void buildMeshList();
   void buildChildList();

protected:

   const domNode*             p_domNode;        ///< Pointer to the node in the Collada DOM
   ColladaAppNode*            appParent;        ///< Parent node in Collada-space
   ColladaExtension_node*     nodeExt;          ///< node extension
   Vector<AnimatedFloatList>  nodeTransforms;   ///< Ordered vector of node transform elements (scale, translate etc)
   bool                       invertMeshes;     ///< True if this node's coordinate space is inverted (left handed)

   Map<StringTableEntry, F32> mProps;           ///< Hash of float properties (converted to int or bool as needed)

   F32                        lastNodeTransformTime;  ///< Time of the last transform lookup
   MatrixF                    lastNodeTransform;      ///< Last transform lookup
   bool                       defaultTransformValid;  ///< Flag indicating whether the defaultNodeTransform is valid
   MatrixF                    defaultNodeTransform;   ///< Transform at DefaultTime (t=0)

public:

   ColladaAppNode(const domNode* node, ColladaAppNode* parent = 0);
   virtual ~ColladaAppNode()
   {
      delete nodeExt;
      mProps.clear();
   }

   const domNode* getDomNode() const { return p_domNode; }

   //-----------------------------------------------------------------------
   const char *getName() { return mName; }
   const char *getParentName() { return mParentName; }

   bool isEqual(AppNode* node)
   {
      const ColladaAppNode* appNode = dynamic_cast<const ColladaAppNode*>(node);
      return (appNode && (appNode->p_domNode == p_domNode));
   }

   // Property look-ups: only float properties are stored, the rest are
   // converted from floats as needed
   bool getFloat(const char* propName, F32& defaultVal)
   {
      Map<StringTableEntry,F32>::Iterator itr = mProps.find(propName);
      if (itr != mProps.end())
         defaultVal = itr->value;
      return false;
   }
   bool getInt(const char* propName, S32& defaultVal)
   {
      F32 value = defaultVal;
      bool ret = getFloat(propName, value);
      defaultVal = (S32)value;
      return ret;
   }
   bool getBool(const char* propName, bool& defaultVal)
   {
      F32 value = defaultVal;
      bool ret = getFloat(propName, value);
      defaultVal = (value != 0);
      return ret;
   }

   MatrixF getNodeTransform(F32 time);
   bool animatesTransform(const AppSequence* appSeq);
   bool isParentRoot() { return (appParent == NULL); }
};

#endif // _COLLADA_APPNODE_H_
