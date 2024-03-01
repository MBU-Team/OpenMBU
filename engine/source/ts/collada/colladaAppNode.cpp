//-----------------------------------------------------------------------------
// Collada-2-DTS
// Copyright (C) 2008 GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifdef _MSC_VER
#pragma warning(disable : 4706)  // disable warning about assignment within conditional
#endif

#include "ts/loader/appSequence.h"
#include "ts/collada/colladaExtensions.h"
#include "ts/collada/colladaAppNode.h"
#include "ts/collada/colladaAppMesh.h"


ColladaAppNode::ColladaAppNode(const domNode* node, ColladaAppNode* parent)
      : p_domNode(node), appParent(parent), nodeExt(new ColladaExtension_node(node)),
      lastNodeTransformTime(-1), defaultTransformValid(false), invertMeshes(false)
{
   mName = dStrdup(_GetNameOrId(node));
   mParentName = dStrdup(parent ? parent->getName() : "ROOT");

   // Extract user properties from the <node> extension as "name=value" pairs
   const char *delims = "=\r\n";
   char* properties = dStrdup(nodeExt->user_properties);
   char* token = dStrtok(properties, delims);
   do {
      // Trim leading whitespace from the name string
      const char* name = token;
      while (name && dIsspace(*name))
         name++;

      // Get the value string
      if ( (token = dStrtok(NULL, delims)) )
         mProps.insert(StringTable->insert(name), dAtof(token));
   } while ( (token = dStrtok(NULL, delims)) );

   dFree( properties );

   // Create vector of transform elements
   for (int iChild = 0; iChild < node->getContents().getCount(); iChild++) {
      switch (node->getContents()[iChild]->getElementType()) {
         case COLLADA_TYPE::TRANSLATE:
         case COLLADA_TYPE::ROTATE:
         case COLLADA_TYPE::SCALE:
         case COLLADA_TYPE::SKEW:
         case COLLADA_TYPE::MATRIX:
         case COLLADA_TYPE::LOOKAT:
            nodeTransforms.increment();
            nodeTransforms.last().element = node->getContents()[iChild];
            break;
      }
   }
}

// Get all child nodes
void ColladaAppNode::buildChildList()
{
   // Process children: collect <node> and <instance_node> elements
   for (int iChild = 0; iChild < p_domNode->getContents().getCount(); iChild++) {

      daeElement* child = p_domNode->getContents()[iChild];
      switch (child->getElementType()) {

         case COLLADA_TYPE::NODE:
         {
            domNode* node = daeSafeCast<domNode>(child);
            mChildNodes.push_back(new ColladaAppNode(node, this));
            break;
         }

         case COLLADA_TYPE::INSTANCE_NODE:
         {
            domInstance_node* instanceNode = daeSafeCast<domInstance_node>(child);
            domNode* node = daeSafeCast<domNode>(instanceNode->getUrl().getElement());
            mChildNodes.push_back(new ColladaAppNode(node, this));
            break;
         }
      }
   }
}

// Get all geometry attached to this node
void ColladaAppNode::buildMeshList()
{
   // Process children: collect <instance_geometry> and <instance_controller> elements
   for (int iChild = 0; iChild < p_domNode->getContents().getCount(); iChild++) {

      daeElement* child = p_domNode->getContents()[iChild];
      switch (child->getElementType()) {

         case COLLADA_TYPE::INSTANCE_GEOMETRY:
            mMeshes.push_back(new ColladaAppMesh(daeSafeCast<domInstance_geometry>(child), this));
            break;

         case COLLADA_TYPE::INSTANCE_CONTROLLER:
            mMeshes.push_back(new ColladaAppMesh(daeSafeCast<domInstance_controller>(child), this));
            break;
      }
   }
}

bool ColladaAppNode::animatesTransform(const AppSequence* appSeq)
{
   // Check if any of this node's transform elements are animated during the
   // sequence interval
   for (int iTxfm = 0; iTxfm < nodeTransforms.size(); iTxfm++) {
      if (nodeTransforms[iTxfm].isAnimated(appSeq->getStart(), appSeq->getEnd()))
         return true;
   }
   return false;
}

/// Get the world transform of the node at the specified time
MatrixF ColladaAppNode::getNodeTransform(F32 time)
{
   // Avoid re-computing the transform if possible
   if (defaultTransformValid && time == TSShapeLoader::DefaultTime)
   {
      return defaultNodeTransform;
   }
   else if (time != lastNodeTransformTime)
   {
      lastNodeTransform = getTransform(time);

      // Check for inverted node coordinate spaces => can happen when modelers
      // use the 'mirror' tool in their 3d app. Shows up as negative <scale>
      // transforms in the collada model.
      if (m_matF_determinant(lastNodeTransform) < 0.0f)
      {
         // Mark this node as inverted so we can mirror mesh geometry, then
         // de-invert the transform matrix
         invertMeshes = true;
         lastNodeTransform.scale(Point3F(1, 1, -1));
      }

      // Cache the transform for subsequent calls
      lastNodeTransformTime = time;
      if (time == TSShapeLoader::DefaultTime)
      {
         defaultTransformValid = true;
         defaultNodeTransform = lastNodeTransform;
      }
   }

   return lastNodeTransform;
}

MatrixF ColladaAppNode::getTransform(F32 time)
{
   MatrixF transform;

   if (appParent) {
      // Get parent node's transform
      transform = appParent->getTransform(time);
   }
   else {
      // no parent (ie. root level) => scale by global shape <unit>
      transform.identity();
      transform.scale(Point3F(ColladaUtils::getOptions().unit, ColladaUtils::getOptions().unit, ColladaUtils::getOptions().unit));
      ColladaUtils::convertTransform(transform);
   }

   // Multiply by local node transform elements
   for (int iTxfm = 0; iTxfm < nodeTransforms.size(); iTxfm++) {

      MatrixF mat(true);

      // Convert the transform element to a MatrixF
      switch (nodeTransforms[iTxfm].element->getElementType()) {
         case COLLADA_TYPE::TRANSLATE: mat = vecToMatrixF<domTranslate>(nodeTransforms[iTxfm].getValue(time));  break;
         case COLLADA_TYPE::SCALE:     mat = vecToMatrixF<domScale>(nodeTransforms[iTxfm].getValue(time));      break;
         case COLLADA_TYPE::ROTATE:    mat = vecToMatrixF<domRotate>(nodeTransforms[iTxfm].getValue(time));     break;
         case COLLADA_TYPE::MATRIX:    mat = vecToMatrixF<domMatrix>(nodeTransforms[iTxfm].getValue(time));     break;
         case COLLADA_TYPE::SKEW:      mat = vecToMatrixF<domSkew>(nodeTransforms[iTxfm].getValue(time));       break;
         case COLLADA_TYPE::LOOKAT:    mat = vecToMatrixF<domLookat>(nodeTransforms[iTxfm].getValue(time));     break;
      }

      // Remove node scaling (but keep reflections) if desired
      if (ColladaUtils::getOptions().ignoreNodeScale)
      {
         Point3F invScale = mat.getScale();
         invScale.x = invScale.x ? (1.0f / invScale.x) : 0;
         invScale.y = invScale.y ? (1.0f / invScale.y) : 0;
         invScale.z = invScale.z ? (1.0f / invScale.z) : 0;
         mat.scale(invScale);
      }

      // Post multiply the animated transform
      transform.mul(mat);
   }

   return transform;
}
