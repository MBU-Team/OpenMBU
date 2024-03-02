//-----------------------------------------------------------------------------
// TS Shape Loader
// Copyright (C) 2008 GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "ts/loader/appNode.h"

AppNode::AppNode()
{
   mName = NULL;
   mParentName = NULL;
}

AppNode::~AppNode()
{
   dFree( mName );
   dFree( mParentName );

   // delete children and meshes
   for (S32 i = 0; i < mChildNodes.size(); i++)
      delete mChildNodes[i];
   for (S32 i = 0; i < mMeshes.size(); i++)
      delete mMeshes[i];
}

S32 AppNode::getNumMesh()
{
   if (mMeshes.size() == 0)
      buildMeshList();
   return mMeshes.size();
}

AppMesh* AppNode::getMesh(S32 idx)
{
   return (idx < getNumMesh() && idx >= 0) ? mMeshes[idx] : NULL;
}

S32 AppNode::getNumChildNodes()
{
   if (mChildNodes.size() == 0)
      buildChildList();
   return mChildNodes.size();
}

AppNode * AppNode::getChildNode(S32 idx)
{
   return (idx < getNumChildNodes() && idx >= 0) ? mChildNodes[idx] : NULL;
}

bool AppNode::isBillboard()
{
   return !dStrnicmp(getName(),"BB::",4) || !dStrnicmp(getName(),"BB_",3) || isBillboardZAxis();
}

bool AppNode::isBillboardZAxis()
{
   return !dStrnicmp(getName(),"BBZ::",5) || !dStrnicmp(getName(),"BBZ_",4);
}

bool AppNode::isDummy()
{
   // naming convention should work well enough...
   // ...but can override this method if one wants more
   return !dStrnicmp(getName(), "dummy", 5);
}

bool AppNode::isBounds()
{
   // naming convention should work well enough...
   // ...but can override this method if one wants more
   return !dStricmp(getName(), "bounds");
}

bool AppNode::isSequence()
{
   // naming convention should work well enough...
   // ...but can override this method if one wants more
   return !dStrnicmp(getName(), "Sequence", 8);
}

bool AppNode::isRoot()
{
   // we assume root node isn't added, so this is never true
   // but allow for possibility (by overriding this method)
   // so that isParentRoot still works.
   return false;
}
