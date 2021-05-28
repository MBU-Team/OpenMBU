//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "editor/editor.h"
#include "console/console.h"
#include "console/consoleInternal.h"
#include "gui/controls/guiTextListCtrl.h"
#include "platform/event.h"
#include "game/shapeBase.h"
#include "game/gameConnection.h"

bool gEditingMission = false;

//------------------------------------------------------------------------------
// Class EditManager
//------------------------------------------------------------------------------

IMPLEMENT_CONOBJECT(EditManager);

EditManager::EditManager()
{
    for (U32 i = 0; i < 10; i++)
        mBookmarks[i] = MatrixF(true);
}

EditManager::~EditManager()
{
}

//------------------------------------------------------------------------------

bool EditManager::onWake()
{
    if (!Parent::onWake())
        return(false);

    for (SimGroupIterator itr(Sim::getRootGroup()); *itr; ++itr)
        (*itr)->onEditorEnable();

    gEditingMission = true;

    return(true);
}

void EditManager::onSleep()
{
    for (SimGroupIterator itr(Sim::getRootGroup()); *itr; ++itr)
        (*itr)->onEditorDisable();

    gEditingMission = false;
    Parent::onSleep();
}

//------------------------------------------------------------------------------

bool EditManager::onAdd()
{
    if (!Parent::onAdd())
        return(false);

    // hook the namespace
    const char* name = getName();
    if (name && name[0] && getClassRep())
    {
        Namespace* parent = getClassRep()->getNameSpace();
        Con::linkNamespaces(parent->mName, name);
        mNameSpace = Con::lookupNamespace(name);
    }

    return(true);
}

//------------------------------------------------------------------------------

static GameBase* getControlObj()
{
    GameConnection* connection = GameConnection::getLocalClientConnection();
    ShapeBase* control = 0;
    if (connection)
        control = connection->getControlObject();
    return(control);
}

ConsoleMethod(EditManager, setBookmark, void, 3, 3, "(int slot)")
{
    S32 val = dAtoi(argv[2]);
    if (val < 0 || val > 9)
        return;

    GameBase* control = getControlObj();
    if (control)
        object->mBookmarks[val] = control->getTransform();
}

ConsoleMethod(EditManager, gotoBookmark, void, 3, 3, "(int slot)")
{
    S32 val = dAtoi(argv[2]);
    if (val < 0 || val > 9)
        return;

    GameBase* control = getControlObj();
    if (control)
        control->setTransform(object->mBookmarks[val]);
}