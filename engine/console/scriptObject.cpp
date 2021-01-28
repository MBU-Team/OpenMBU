//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "console/simBase.h"
#include "console/consoleTypes.h"

//-----------------------------------------------------------------------------
// Script object placeholder
//-----------------------------------------------------------------------------

class ScriptObject : public SimObject
{
   typedef SimObject Parent;
   StringTableEntry mClassName;
   StringTableEntry mSuperClassName;
public:
   ScriptObject();
   bool onAdd();
   void onRemove();

   DECLARE_CONOBJECT(ScriptObject);

   static void initPersistFields();
};

IMPLEMENT_CONOBJECT(ScriptObject);

void ScriptObject::initPersistFields()
{
   addGroup("Classes", "Script objects have the ability to inherit and have class information.");
   addField("class", TypeString, Offset(mClassName, ScriptObject), "Class of object.");
   addField("superClass", TypeString, Offset(mSuperClassName, ScriptObject), "Superclass of object.");
   endGroup("Classes");
}

ScriptObject::ScriptObject()
{
   mClassName = "";
   mSuperClassName = "";
}

bool ScriptObject::onAdd()
{
   if (!Parent::onAdd())
      return false;

   // it's possible that all the namespace links can fail, if
   // multiple objects are named the same thing with different script
   // hierarchies.
   // linkNamespaces will now return false and echo an error message
   // rather than asserting.

   // superClassName -> ScriptObject
   StringTableEntry parent = StringTable->insert("ScriptObject");
   if(mSuperClassName[0])
   {
      if(Con::linkNamespaces(parent, mSuperClassName))
         parent = mSuperClassName;
   }

   // className -> superClassName
   if (mClassName[0])
   {
      if(Con::linkNamespaces(parent, mClassName))
         parent = mClassName;
   }

   // objectName -> className
   StringTableEntry objectName = getName();
   if (objectName && objectName[0])
   {
      if(Con::linkNamespaces(parent, objectName))
         parent = objectName;
   }

   // Store our namespace
   mNameSpace = Con::lookupNamespace(parent);

   // Call onAdd in script!
   Con::executef(this, 2, "onAdd", Con::getIntArg(getId()));
   return true;
}

void ScriptObject::onRemove()
{
   // Call onRemove in script!
   Con::executef(this, 2, "onRemove", Con::getIntArg(getId()));
   Parent::onRemove();
}

//-----------------------------------------------------------------------------
// Script group placeholder
//-----------------------------------------------------------------------------

class ScriptGroup : public SimGroup
{
   typedef SimGroup Parent;
   StringTableEntry mClassName;
   StringTableEntry mSuperClassName;
public:
   ScriptGroup();
   bool onAdd();
   void onRemove();

   DECLARE_CONOBJECT(ScriptGroup);

   static void initPersistFields();
};

IMPLEMENT_CONOBJECT(ScriptGroup);

void ScriptGroup::initPersistFields()
{
   addGroup("Classes");
   addField("class", TypeString, Offset(mClassName, ScriptGroup));
   addField("superClass", TypeString, Offset(mSuperClassName, ScriptGroup));
   endGroup("Classes");
}

ScriptGroup::ScriptGroup()
{
   mClassName = "";
   mSuperClassName = "";
}

bool ScriptGroup::onAdd()
{
   if (!Parent::onAdd())
      return false;

   // superClassName -> ScriptGroup
   StringTableEntry parent = StringTable->insert("ScriptGroup");
   if(mSuperClassName[0])
   {
      if(Con::linkNamespaces(parent, mSuperClassName))
         parent = mSuperClassName;
   }

   // className -> superClassName
   if(mClassName[0])
   {
      if(Con::linkNamespaces(parent, mClassName))
         parent = mClassName;
   }

   // objectName -> className
   StringTableEntry objectName = getName();
   if (objectName && objectName[0])
   {
      if(Con::linkNamespaces(parent, objectName))
         parent = objectName;
   }

   // Store our namespace
   mNameSpace = Con::lookupNamespace(parent);

   // Call onAdd in script!
   Con::executef(this, 2, "onAdd", Con::getIntArg(getId()));
   return true;
}

void ScriptGroup::onRemove()
{
   // Call onRemove in script!
   Con::executef(this, 2, "onRemove", Con::getIntArg(getId()));
   Parent::onRemove();
}
