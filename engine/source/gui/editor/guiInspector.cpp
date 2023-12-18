//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "gui/editor/guiInspector.h"
#include "gui/core/guiCanvas.h"
#include "core/frameAllocator.h"

//////////////////////////////////////////////////////////////////////////
// GuiInspector
//////////////////////////////////////////////////////////////////////////
// The GuiInspector Control houses the body of the inspector.
// It is not exposed as a conobject because it merely does the grunt work
// and is only meant to be used when housed by a scroll control.  Therefore
// the GuiInspector control is a scroll control that creates it's own 
// content.  That content being of course, the GuiInspector control.
IMPLEMENT_CONOBJECT(GuiInspector);

GuiInspector::GuiInspector()
{
    mGroups.clear();
    mTarget = NULL;
    mPadding = 1;
}

GuiInspector::~GuiInspector()
{
    clearGroups();
}

bool GuiInspector::onAdd()
{
    if (!Parent::onAdd())
        return false;

    return true;
}

//////////////////////////////////////////////////////////////////////////
// Handle Parent Sizing (We constrain ourself to our parents width)
//////////////////////////////////////////////////////////////////////////
void GuiInspector::parentResized(const Point2I& oldParentExtent, const Point2I& newParentExtent)
{
    GuiControl* parent = getParent();
    if (parent && dynamic_cast<GuiScrollCtrl*>(parent) != NULL)
    {
        GuiScrollCtrl* scroll = dynamic_cast<GuiScrollCtrl*>(parent);
        setWidth((newParentExtent.x - (scroll->scrollBarThickness() + 4)));
    }
    else
        Parent::parentResized(oldParentExtent, newParentExtent);
}

bool GuiInspector::findExistentGroup(StringTableEntry groupName)
{
    // If we have no groups, it couldn't possibly exist
    if (mGroups.empty())
        return false;

    // Attempt to find it in the group list
    Vector<GuiInspectorGroup*>::iterator i = mGroups.begin();

    for (; i != mGroups.end(); i++)
    {
        if (dStricmp((*i)->getGroupName(), groupName) == 0)
            return true;
    }

    return false;
}

void GuiInspector::clearGroups()
{
    // If we have no groups, there's nothing to clear!
    if (mGroups.empty())
        return;

    // Attempt to find it in the group list
    Vector<GuiInspectorGroup*>::iterator i = mGroups.begin();

    for (; i != mGroups.end(); i++)
        if ((*i)->isProperlyAdded())
            (*i)->deleteObject();

    mGroups.clear();
}

void GuiInspector::inspectObject(SimObject* object)
{
    GuiCanvas* guiCanvas = getRoot();
    if (!guiCanvas)
        return;

    SimObjectPtr<GuiControl> currResponder = guiCanvas->getFirstResponder();

    // If our target is the same as our current target, just update the groups.
    if (mTarget == object)
    {
        // Force the target to update itself.
        mTarget->inspectPreApply();
        mTarget->inspectPostApply();

        Vector<GuiInspectorGroup*>::iterator i = mGroups.begin();
        for (; i != mGroups.end(); i++)
            (*i)->inspectGroup();

        // Don't steal first responder
        if (!currResponder.isNull())
            guiCanvas->setFirstResponder(currResponder);

        return;
    }

    // Clear our current groups
    clearGroups();

    // Set Target
    mTarget = object;

    // Always create the 'general' group (for un-grouped fields)
    GuiInspectorGroup* general = new GuiInspectorGroup(mTarget, "General", this);
    if (general != NULL)
    {
        general->registerObject();
        mGroups.push_back(general);
        addObject(general);
    }

    // Grab this objects field list
    AbstractClassRep::FieldList& fieldList = mTarget->getModifiableFieldList();
    AbstractClassRep::FieldList::iterator itr;

    // Iterate through, identifying the groups and create necessary GuiInspectorGroups
    for (itr = fieldList.begin(); itr != fieldList.end(); itr++)
    {
        if (itr->type == AbstractClassRep::StartGroupFieldType && !findExistentGroup(itr->pGroupname))
        {
            GuiInspectorGroup* group = new GuiInspectorGroup(mTarget, itr->pGroupname, this);
            if (group != NULL)
            {
                group->registerObject();
                mGroups.push_back(group);
                addObject(group);
            }
        }
    }

    // Deal with dynamic fields
    GuiInspectorGroup* dynGroup = new GuiInspectorDynamicGroup(mTarget, "Dynamic Fields", this);
    if (dynGroup != NULL)
    {
        dynGroup->registerObject();
        mGroups.push_back(dynGroup);
        addObject(dynGroup);
    }

    // If the general group is still empty at this point, kill it.
    for (S32 i = 0; i < mGroups.size(); i++)
    {
        if (mGroups[i] == general && general->mStack->size() == 0)
        {
            mGroups.erase(i);
            general->deleteObject();
            updatePanes();
        }
    }

    // Don't steal first responder
    if (!currResponder.isNull())
        guiCanvas->setFirstResponder(currResponder);

}

ConsoleMethod(GuiInspector, inspect, void, 3, 3, "Inspect(Object)")
{
    SimObject* target = Sim::findObject(argv[2]);
    if (!target)
    {
        if (dAtoi(argv[2]) > 0)
            Con::warnf("%s::inspect(): invalid object: %s", argv[0], argv[2]);

        object->clearGroups();
        return;
    }

    object->inspectObject(target);
}


ConsoleMethod(GuiInspector, getInspectObject, const char*, 2, 2, "getInspectObject() - Returns currently inspected object")
{
    SimObject* pSimObject = object->getInspectObject();
    if (pSimObject != NULL)
        return pSimObject->getIdString();

    return "";
}

void GuiInspector::setName(StringTableEntry newName)
{
    if (mTarget == NULL)
        return;

    StringTableEntry name = StringTable->insert(newName);

    // Only assign a new name if we provide one
    mTarget->assignName(name);

}

ConsoleMethod(GuiInspector, setName, void, 3, 3, "setName(NewObjectName)")
{
    object->setName(argv[2]);
}


//////////////////////////////////////////////////////////////////////////
// GuiInspectorField
//////////////////////////////////////////////////////////////////////////
// The GuiInspectorField control is a representation of a single abstract
// field for a given ConsoleObject derived object.  It handles creation
// getting and setting of it's fields data and editing control.  
//
// Creation of custom edit controls is done through this class and is
// dependent upon the dynamic console type, which may be defined to be
// custom for different types.
//
// Note : GuiInspectorField controls must have a GuiInspectorGroup as their
//        parent.  
IMPLEMENT_CONOBJECT(GuiInspectorField);

// Caption width is in percentage of total width
S32 GuiInspectorField::smCaptionWidth = 35;

GuiInspectorField::GuiInspectorField(GuiInspectorGroup* parent, SimObjectPtr<SimObject> target, AbstractClassRep::Field* field)
{
    if (field != NULL)
        mCaption = StringTable->insert(field->pFieldname);
    else
        mCaption = StringTable->insert("");

    mParent = parent;
    mTarget = target;
    mField = field;
    mFieldArrayIndex = NULL;
    mBounds.set(0, 0, 100, 20);

}

GuiInspectorField::GuiInspectorField()
{
    mCaption = StringTable->insert("");
    mParent = NULL;
    mTarget = NULL;
    mField = NULL;
    mFieldArrayIndex = NULL;
    mBounds.set(0, 0, 100, 20);
}

GuiInspectorField::~GuiInspectorField()
{
}

//////////////////////////////////////////////////////////////////////////
// Get/Set Data Functions
//////////////////////////////////////////////////////////////////////////
void GuiInspectorField::setData(StringTableEntry data)
{
    if (mField == NULL || mTarget == NULL)
        return;

    mTarget->inspectPreApply();

    mTarget->setDataField(mField->pFieldname, mFieldArrayIndex, data);

    // Force our edit to update
    updateValue(data);

    mTarget->inspectPostApply();
}

StringTableEntry GuiInspectorField::getData()
{
    if (mField == NULL || mTarget == NULL)
        return "";

    return mTarget->getDataField(mField->pFieldname, mFieldArrayIndex);
}

void GuiInspectorField::setField(AbstractClassRep::Field* field, const char* arrayIndex)
{
    mField = field;

    if (arrayIndex != NULL)
    {
        mFieldArrayIndex = StringTable->insert(arrayIndex);

        S32 frameTempSize = dStrlen(field->pFieldname) + 32;
        FrameTemp<char> valCopy(frameTempSize);
        dSprintf((char*)valCopy, frameTempSize, "%s%s", field->pFieldname, arrayIndex);

        mCaption = StringTable->insert(valCopy);
    }
    else
        mCaption = StringTable->insert(field->pFieldname);
}


StringTableEntry GuiInspectorField::getFieldName()
{
    // Sanity
    if (mField == NULL)
        return StringTable->lookup("");

    // Array element?
    if (mFieldArrayIndex != NULL)
    {
        S32 frameTempSize = dStrlen(mField->pFieldname) + 32;
        FrameTemp<char> valCopy(frameTempSize);
        dSprintf((char*)valCopy, frameTempSize, "%s%s", mField->pFieldname, mFieldArrayIndex);

        // Return formatted element
        return StringTable->insert(valCopy);
    }

    // Plain ole field name.
    return mField->pFieldname;
};
//////////////////////////////////////////////////////////////////////////
// Overrideables for custom edit fields
//////////////////////////////////////////////////////////////////////////
GuiControl* GuiInspectorField::constructEditControl()
{
    GuiControl* retCtrl = new GuiTextEditCtrl();

    // If we couldn't construct the control, bail!
    if (retCtrl == NULL)
        return retCtrl;

    // Let's make it look pretty.
    retCtrl->setField("profile", "GuiInspectorTextEditProfile");

    // Don't forget to register ourselves
    registerEditControl(retCtrl);

    char szBuffer[512];
    dSprintf(szBuffer, 512, "%d.apply(%d.getText());", getId(), retCtrl->getId());
    retCtrl->setField("AltCommand", szBuffer);
    retCtrl->setField("Validate", szBuffer);


    return retCtrl;
}

void GuiInspectorField::registerEditControl(GuiControl* ctrl)
{
    if (!mTarget)
        return;

    char szName[512];
    dSprintf(szName, 512, "IE_%s_%d_Field", ctrl->getClassName(), mTarget->getId());

    // Register the object
    ctrl->registerObject(szName);
}

void GuiInspectorField::onRender(Point2I offset, const RectI& updateRect)
{
    RectI worldRect(offset, mBounds.extent);
    // Calculate Caption Rect
    RectI captionRect(offset, Point2I(mFloor(mBounds.extent.x * (F32)((F32)GuiInspectorField::smCaptionWidth / 100.0)), mBounds.extent.y));
    // Calculate Edit Field Rect
    RectI editFieldRect(offset + Point2I(captionRect.extent.x + 1, 0), Point2I(mBounds.extent.x - (captionRect.extent.x + 5), mBounds.extent.y));

    // Calculate Y Offset to center vertically the caption
    U32 captionYOffset = mFloor((F32)(captionRect.extent.y - mProfile->mFonts[0].mFont->getHeight()) / 2);

    renderFilledBorder(captionRect, mProfile->mBorderColor, mProfile->mFillColor);
    renderFilledBorder(editFieldRect, mProfile->mBorderColor, mProfile->mFillColor);


    RectI clipRect = GFX->getClipRect();

    if (clipRect.intersect(captionRect))
    {
        // Backup Bitmap Modulation
        ColorI currColor;
        GFX->getBitmapModulation(&currColor);

        GFX->setBitmapModulation(mProfile->mFontColor);

        GFX->setClipRect(RectI(clipRect.point, Point2I(captionRect.extent.x, clipRect.extent.y)));
        // Draw Caption ( Vertically Centered )
        GFX->drawText(mProfile->mFonts[0].mFont, Point2I(captionRect.point.x + 6, captionRect.point.y + captionYOffset), mCaption, &mProfile->mFontColor);

        GFX->setBitmapModulation(currColor);

        GFX->setClipRect(clipRect);
    }

    Parent::onRender(offset, updateRect);
}

bool GuiInspectorField::onAdd()
{
    if (!Parent::onAdd())
        return false;

    if (!mTarget)
        return false;

    mEdit = constructEditControl();

    if (mEdit == NULL)
        return false;

    // Add our edit as a child
    addObject(mEdit);

    // Calculate Caption Rect
    RectI captionRect(mBounds.point, Point2I(mFloor(mBounds.extent.x * (F32)((F32)GuiInspectorField::smCaptionWidth / 100.0)), mBounds.extent.y));

    // Calculate Edit Field Rect
    RectI editFieldRect(Point2I(captionRect.extent.x + 1, 0), Point2I(mBounds.extent.x - (captionRect.extent.x + 5), mBounds.extent.y));

    // Resize to fit properly in allotted space
    mEdit->resize(editFieldRect.point, editFieldRect.extent);

    // Prefer GuiInspectorFieldProfile 
    SimObject* profilePtr = Sim::findObject("GuiInspectorFieldProfile");
    if (profilePtr != NULL)
        setControlProfile(dynamic_cast<GuiControlProfile*>(profilePtr));

    // Force our editField to set it's value
    updateValue(getData());

    return true;
}

void GuiInspectorField::updateValue(StringTableEntry newValue)
{
    GuiTextEditCtrl* ctrl = dynamic_cast<GuiTextEditCtrl*>(mEdit);
    if (ctrl != NULL)
        ctrl->setText(newValue);
}

ConsoleMethod(GuiInspectorField, apply, void, 3, 3, "apply(newValue);")
{
    object->setData(argv[2]);
}

void GuiInspectorField::resize(const Point2I& newPosition, const Point2I& newExtent)
{
    Parent::resize(newPosition, newExtent);

    if (mEdit != NULL)
    {
        // Calculate Caption Rect
        RectI captionRect(mBounds.point, Point2I(mFloor(mBounds.extent.x * (F32)((F32)GuiInspectorField::smCaptionWidth / 100.0)), mBounds.extent.y));

        // Calculate Edit Field Rect
        RectI editFieldRect(Point2I(captionRect.extent.x + 1, 0), Point2I(mBounds.extent.x - (captionRect.extent.x + 5), mBounds.extent.y));

        mEdit->resize(editFieldRect.point, editFieldRect.extent);
    }
}

//////////////////////////////////////////////////////////////////////////
// GuiInspectorGroup
//////////////////////////////////////////////////////////////////////////
//
// The GuiInspectorGroup control is a helper control that the inspector
// makes use of which houses a collapsible pane type control for separating
// inspected objects fields into groups.  The content of the inspector is 
// made up of zero or more GuiInspectorGroup controls inside of a GuiStackControl
//
//
//
IMPLEMENT_CONOBJECT(GuiInspectorGroup);

GuiInspectorGroup::GuiInspectorGroup()
{
    mBounds.set(0, 0, 200, 20);
    mBarWidth.set(18, 18);

    mChildren.clear();

    mCaption = StringTable->insert("");
    mTarget = NULL;
    mParent = NULL;
    mIsExpanded = true;
    mIsAnimating = false;
    mCollapsing = true;
    mAnimateDestHeight = mBarWidth.y;
    mAnimateStep = 1;

    // Make sure we receive our ticks.
    setProcessTicks();
}

GuiInspectorGroup::GuiInspectorGroup(SimObjectPtr<SimObject> target, StringTableEntry groupName, SimObjectPtr<GuiInspector> parent)
{
    mBounds.set(0, 0, 200, 20);
    mBarWidth.set(18, 18);

    mChildren.clear();

    mIsExpanded = true;
    mIsAnimating = false;
    mCollapsing = true;
    mAnimateDestHeight = mBarWidth.y;
    mAnimateStep = 1;
    mChildHeight = 32;
    mCaption = StringTable->insert(groupName);
    mTarget = target;
    mParent = parent;
}

GuiInspectorGroup::~GuiInspectorGroup()
{
    if (!mChildren.empty())
    {
        Vector<GuiInspectorField*>::iterator i = mChildren.begin();
        for (; i != mChildren.end(); i++);

    }
}

//////////////////////////////////////////////////////////////////////////
// Persistence 
//////////////////////////////////////////////////////////////////////////
void GuiInspectorGroup::initPersistFields()
{
    Parent::initPersistFields();

    addField("Caption", TypeString, Offset(mCaption, GuiInspectorGroup));
}

//////////////////////////////////////////////////////////////////////////
// Scene Events
//////////////////////////////////////////////////////////////////////////
bool GuiInspectorGroup::onAdd()
{
    if (!Parent::onAdd())
        return false;

    // Create our field stack control
    mStack = new GuiStackControl();
    if (!mStack)
        return false;

    addObject(mStack);
    mStack->setField("padding", "1.0");
    mStack->resize(mBarWidth + Point2I(1, 1), mBounds.extent - (mBarWidth + Point2I(1, 1)));

    inspectGroup();

    return true;
}


//////////////////////////////////////////////////////////////////////////
// Mouse Events
//////////////////////////////////////////////////////////////////////////
void GuiInspectorGroup::onMouseDown(const GuiEvent& event)
{
    // Calculate Group Caption Rect ( Clip rect within 4 units of the outer bounds so we don't render into border )
    RectI captionRect(Point2I(mBarWidth.x, 0), Point2I(mBounds.extent.x - 4, mBarWidth.y));
    // Calculate Y Offset to center vertically the caption
    U32 captionYOffset = mFloor((F32)(captionRect.extent.y - mProfile->mFonts[0].mFont->getHeight()) / 2);
    // Calculate Expand/Collapse Button Rect
    RectI toggleRect(Point2I(captionYOffset, captionYOffset), mBarWidth - Point2I(captionYOffset * 2, captionYOffset * 2));
    toggleRect.inset(1, 1);

    if (captionRect.pointInRect(globalToLocalCoord(event.mousePoint)) && !mIsAnimating)
    {
        if (!mIsExpanded)
            animateTo(getExpandedHeight());
        else
            animateTo(mBarWidth.y);
    }
}

//////////////////////////////////////////////////////////////////////////
// Control Sizing Helpers
//////////////////////////////////////////////////////////////////////////
S32 GuiInspectorGroup::getExpandedHeight()
{
    if (mStack != NULL && mStack->getCount() != 0)
        return mStack->getHeight() + mBarWidth.y + 1;
    return mBarWidth.y;
}

void GuiInspectorGroup::resize(const Point2I& newPosition, const Point2I& newExtent)
{
    Parent::resize(newPosition, newExtent);

    if (mStack != NULL && !mIsAnimating && mIsExpanded)
        mStack->setExtent(mBounds.extent - (mBarWidth + Point2I(1, 1)));
}

//////////////////////////////////////////////////////////////////////////
// Control Sizing Animation Functions
//////////////////////////////////////////////////////////////////////////
void GuiInspectorGroup::animateTo(S32 height)
{
    // We do nothing if we're already animating
    if (mIsAnimating)
        return;

    bool collapsing = (bool)(mBounds.extent.y > height);

    // If we're already at the destination height, bail
    if (mBounds.extent.y >= height && !collapsing)
        return;

    // If we're already at the destination height, bail
    if (mBounds.extent.y <= height && collapsing)
        return;

    // Set destination height
    mAnimateDestHeight = height;

    // Set Animation Mode
    mCollapsing = collapsing;

    // Set Animation Step (Increment)
    if (collapsing)
        mAnimateStep = mFloor((F32)(mBounds.extent.y - height) / 2);
    else
        mAnimateStep = mFloor((F32)(height - mBounds.extent.y) / 2);

    // Start our animation
    mIsAnimating = true;

}

void GuiInspectorGroup::processTick()
{
    // We do nothing here if we're NOT animating
    if (!mIsAnimating)
        return;

    // Sanity check to fix non collapsing panels.
    if (mAnimateStep == 0)
        mAnimateStep = 1;

    // We're collapsing ourself down (Hiding our contents)
    if (mCollapsing)
    {
        if (mBounds.extent.y < mAnimateDestHeight)
            mBounds.extent.y = mAnimateDestHeight;
        else if ((mBounds.extent.y - mAnimateStep) < mAnimateDestHeight)
            mBounds.extent.y = mAnimateDestHeight;

        if (mBounds.extent.y == mAnimateDestHeight)
            mIsAnimating = false;
        else
            mBounds.extent.y -= mAnimateStep;

        if (!mIsAnimating)
            mIsExpanded = false;
    }
    else // We're expanding ourself (Showing our contents)
    {
        if (mBounds.extent.y > mAnimateDestHeight)
            mBounds.extent.y = mAnimateDestHeight;
        else if ((mBounds.extent.y + mAnimateStep) > mAnimateDestHeight)
            mBounds.extent.y = mAnimateDestHeight;

        if (mBounds.extent.y == mAnimateDestHeight)
            mIsAnimating = false;
        else
            mBounds.extent.y += mAnimateStep;

        if (!mIsAnimating)
            mIsExpanded = true;
    }

    GuiControl* parent = getParent();
    if (parent)
        parent->childResized(this);
}

//////////////////////////////////////////////////////////////////////////
// Control Rendering
//////////////////////////////////////////////////////////////////////////
void GuiInspectorGroup::onRender(Point2I offset, const RectI& updateRect)
{
    //////////////////////////////////////////////////////////////////////////
    // Calculate Necessary Rendering Rectangles
    //////////////////////////////////////////////////////////////////////////
    if (!mProfile || mProfile->mFonts[0].mFont.isNull())
        return;

    // Calculate actual world bounds for rendering
    RectI worldBounds(offset, mBounds.extent);
    // Calculate rendering rectangle for the groups content
    RectI contentRect(offset + mBarWidth, mBounds.extent - (mBarWidth + Point2I(1, 1)));
    // Calculate Group Caption Rect ( Clip rect within 4 units of the outer bounds so we don't render into border )
    RectI captionRect(offset + Point2I(mBarWidth.x, 0), Point2I(mBounds.extent.x - (mBarWidth.x + 4), mBarWidth.y));
    // Calculate Y Offset to center vertically the caption
    U32 captionYOffset = mFloor((F32)(captionRect.extent.y - mProfile->mFonts[0].mFont->getHeight()) / 2);
    // Calculate Expand/Collapse Button Rect
    RectI toggleRect(offset + Point2I(captionYOffset, captionYOffset), mBarWidth - Point2I(captionYOffset * 2, captionYOffset * 2));
    toggleRect.inset(1, 1);

    // Draw Border
    renderFilledBorder(worldBounds, mProfile);

    // Draw Content Background
    if (mIsExpanded || (mIsAnimating && !mCollapsing))
        GFX->drawRectFill(contentRect, ColorI(255, 255, 255));

    // Backup Bitmap Modulation
    ColorI currColor;
    GFX->getBitmapModulation(&currColor);

    GFX->setBitmapModulation(mProfile->mFontColor);
    // Draw Caption ( Vertically Centered )
    GFX->drawText(mProfile->mFonts[0].mFont, Point2I(captionRect.point.x, captionRect.point.y + captionYOffset), mCaption, &mProfile->mFontColor);

    GFX->setBitmapModulation(currColor);


    // Draw Expand/Collapse Button
    if (mIsExpanded)
        renderFilledBorder(toggleRect, mProfile->mBorderColorNA, mProfile->mFillColorNA);
    else
        renderFilledBorder(toggleRect, mProfile->mFillColorNA, mProfile->mBorderColorNA);

    Parent::onRender(offset, updateRect);
}

GuiInspectorField* GuiInspectorGroup::constructField(S32 fieldType)
{
    ConsoleBaseType* cbt = ConsoleBaseType::getType(fieldType);
    AssertFatal(cbt, "GuiInspectorGroup::constructField - could not resolve field type!");

    // Alright, is it a datablock?
    if (cbt->isDatablock())
    {
        // This is fairly straightforward to deal with.
        GuiInspectorDatablockField* dbFieldClass = new GuiInspectorDatablockField(cbt->getTypeClassName());
        if (dbFieldClass != NULL)
        {
            // return our new datablock field with correct datablock type enumeration info
            return dbFieldClass;
        }
    }

    // Nope, not a datablock. So maybe it has a valid inspector field override we can use?
    if (!cbt->getInspectorFieldType())
        // Nothing, so bail.
        return NULL;

    // Otherwise try to make it!
    ConsoleObject* co = create(cbt->getInspectorFieldType());
    GuiInspectorField* gif = dynamic_cast<GuiInspectorField*>(co);

    if (!gif)
    {
        // Wasn't appropriate type, bail.
        delete co;
        return NULL;
    }

    return gif;
}

GuiInspectorField* GuiInspectorGroup::findField(StringTableEntry fieldName)
{
    // If we don't have any field children we can't very well find one then can we?
    if (mChildren.empty())
        return NULL;

    Vector<GuiInspectorField*>::iterator i = mChildren.begin();

    for (; i != mChildren.end(); i++)
    {
        if ((*i)->getFieldName() != NULL && dStricmp((*i)->getFieldName(), fieldName) == 0)
            return (*i);
    }

    return NULL;
}

//GuiInspectorField *GuiInspectorGroup::findArrayField( StringTableEntry fieldName, S32 index )
//{
//   // If we don't have any field children we can't very well find one then can we?
//   if( mChildren.empty() )
//      return NULL;
//
//   Vector<GuiInspectorField*>::iterator i = mChildren.begin();
//
//   for( ; i != mChildren.end(); i++ )
//   {
//      if( (*i)->getFieldName() != NULL && dStricmp( (*i)->getFieldName(), fieldName ) == 0 )
//         return (*i);
//   }
//
//   return NULL;
//}


bool GuiInspectorGroup::inspectGroup()
{
    // We can't inspect a group without a target!
    if (!mTarget)
        return false;

    bool bNoGroup = false;

    // Un-grouped fields are all sorted into the 'general' group
    if (dStricmp(mCaption, "General") == 0)
        bNoGroup = true;

    AbstractClassRep::FieldList& fieldList = mTarget->getModifiableFieldList();
    AbstractClassRep::FieldList::iterator itr;

    bool bGrabItems = false;
    bool bNewItems = false;

    for (itr = fieldList.begin(); itr != fieldList.end(); itr++)
    {
        if (itr->type == AbstractClassRep::StartGroupFieldType)
        {
            // If we're dealing with general fields, always set grabItems to true (to skip them)
            if (bNoGroup == true)
                bGrabItems = true;
            else if (itr->pGroupname != NULL && dStricmp(itr->pGroupname, mCaption) == 0)
                bGrabItems = true;
            continue;
        }
        else if (itr->type == AbstractClassRep::EndGroupFieldType)
        {
            // If we're dealing with general fields, always set grabItems to false (to grab them)
            if (bNoGroup == true)
                bGrabItems = false;
            else if (itr->pGroupname != NULL && dStricmp(itr->pGroupname, mCaption) == 0)
                bGrabItems = false;
            continue;
        }

        if ((bGrabItems == true || (bNoGroup == true && bGrabItems == false)) && itr->type != AbstractClassRep::DepricatedFieldType)
        {
            if (bNoGroup == true && bGrabItems == true)
                continue;
            // This is weird, but it should work for now. - JDD
            // We are going to check to see if this item is an array
            // if so, we're going to construct a field for each array element
            if (itr->elementCount > 1)
            {
                for (S32 nI = 0; nI < itr->elementCount; nI++)
                {
                    FrameTemp<char> intToStr(64);
                    dSprintf(intToStr, 64, "%d", nI);

                    const char* val = mTarget->getDataField(itr->pFieldname, intToStr);
                    if (!val)
                        val = StringTable->lookup("");


                    // Copy Val and construct proper ValueName[nI] format 
                    //      which is "ValueName0" for index 0, etc.
                    S32 frameTempSize = dStrlen(val) + 32;
                    FrameTemp<char> valCopy(frameTempSize);
                    dSprintf((char*)valCopy, frameTempSize, "%s%d", itr->pFieldname, nI);

                    // If the field already exists, just update it
                    GuiInspectorField* field = findField(valCopy);
                    if (field != NULL)
                    {
                        field->updateValue(field->getData());
                        continue;
                    }

                    bNewItems = true;

                    field = constructField(itr->type);
                    if (field == NULL)
                    {
                        field = new GuiInspectorField(this, mTarget, itr);
                        field->setField(itr, intToStr);
                    }
                    else
                    {
                        field->setTarget(mTarget);
                        field->setParent(this);
                        field->setField(itr, intToStr);
                    }

                    field->registerObject();
                    mChildren.push_back(field);
                    mStack->addObject(field);
                }
            }
            else
            {
                // If the field already exists, just update it
                GuiInspectorField* field = findField(itr->pFieldname);
                if (field != NULL)
                {
                    field->updateValue(field->getData());
                    continue;
                }

                bNewItems = true;

                field = constructField(itr->type);
                if (field == NULL)
                    field = new GuiInspectorField(this, mTarget, itr);
                else
                {
                    field->setTarget(mTarget);
                    field->setParent(this);
                    field->setField(itr);
                }

                field->registerObject();
                mChildren.push_back(field);
                mStack->addObject(field);

            }
        }
    }

    // If we've no new items, there's no need to resize anything!
    if (bNewItems == false && !mChildren.empty())
        return true;

    if (mIsExpanded && getHeight() != getExpandedHeight())
        setHeight(getExpandedHeight());

    if (mChildren.empty() && getHeight() != mBarWidth.y)
        setHeight(mBarWidth.y);

    setUpdate();

    return true;
}

//////////////////////////////////////////////////////////////////////////
// GuiInspectorDynamicGroup - inspectGroup override
//////////////////////////////////////////////////////////////////////////
bool GuiInspectorDynamicGroup::inspectGroup()
{
    // We can't inspect a group without a target!
    if (!mTarget)
        return false;

    // Clearing the fields and recreating them will more than likely be more
    // efficient than looking up existent fields, updating them, and then iterating
    // over existent fields and making sure they still exist, if not, deleting them.
    clearFields();

    // Then populate with fields
    SimFieldDictionary* fieldDictionary = mTarget->getFieldDictionary();
    for (SimFieldDictionaryIterator ditr(fieldDictionary); *ditr; ++ditr)
    {
        SimFieldDictionary::Entry* entry = (*ditr);

        GuiInspectorField* field = new GuiInspectorDynamicField(this, mTarget, entry);
        if (field != NULL)
        {
            field->registerObject();
            mChildren.push_back(field);
            mStack->addObject(field);

        }
    }

    if (mIsExpanded && getHeight() != getExpandedHeight())
        setHeight(getExpandedHeight());

    setUpdate();

    return true;
}

void GuiInspectorDynamicGroup::clearFields()
{
    // If we have no groups, it couldn't possibly exist
    if (mChildren.empty())
        return;

    // Attempt to find it in the group list
    Vector<GuiInspectorField*>::iterator i = mChildren.begin();

    for (; i != mChildren.end(); i++)
        if ((*i)->isProperlyAdded())
            (*i)->deleteObject();

    mChildren.clear();
}

SimFieldDictionary::Entry* GuiInspectorDynamicGroup::findDynamicFieldInDictionary(StringTableEntry fieldName)
{
    if (!mTarget)
        return NULL;

    SimFieldDictionary* fieldDictionary = mTarget->getFieldDictionary();

    for (SimFieldDictionaryIterator ditr(fieldDictionary); *ditr; ++ditr)
    {
        SimFieldDictionary::Entry* entry = (*ditr);

        if (dStricmp(entry->slotName, fieldName) == 0)
            return entry;
    }

    return NULL;
}

GuiInspectorDynamicField* GuiInspectorDynamicGroup::findDynamicField(StringTableEntry fieldName)
{
    // We can't inspect a group without a target!
    if (!mTarget || mChildren.empty())
        return NULL;

    Vector<GuiInspectorField*>::iterator i = mChildren.begin();
    for (; i != mChildren.end(); i++)
    {
        if (dStricmp((*i)->getFieldName(), fieldName) == 0)
            return dynamic_cast<GuiInspectorDynamicField*>(*i);
    }

    return NULL;
}


void GuiInspectorDynamicGroup::onRender(Point2I offset, const RectI& updateRect)
{
    // Do normal rendering
    Parent::onRender(offset, updateRect);

    if (!mIsExpanded)
        return;

    // Calculate actual world bounds for rendering
    RectI worldBounds(offset, mBounds.extent);
    // Calculate rendering rectangle for the groups content
    RectI contentRect(offset + mBarWidth, mBounds.extent - (mBarWidth + Point2I(1, 1)));

    U32 textXOffset = mFloor((F32)(mBarWidth.x - mProfile->mFonts[0].mFont->getHeight()) / 2) + mProfile->mFonts[0].mFont->getHeight();
    RectI addFieldRect(offset + Point2I(textXOffset, mBarWidth.y + 4), Point2I(mBarWidth.x, mBounds.extent.y));

    // Backup Bitmap Modulation
    ColorI currColor;
    GFX->getBitmapModulation(&currColor);

    GFX->setBitmapModulation(mProfile->mFontColor);

    RectI clipRect = GFX->getClipRect();
    GFX->setClipRect(updateRect);
    // Draw Caption ( Vertically Centered )
    GFX->drawText(mProfile->mFonts[0].mFont, addFieldRect.point, "Add Field", &mProfile->mFontColor, 9, -90.f);

    GFX->setClipRect(clipRect);
    GFX->setBitmapModulation(currColor);
}

void GuiInspectorDynamicGroup::onMouseDown(const GuiEvent& event)
{
    Parent::onMouseDown(event);

    // Calculate actual world bounds for rendering
    RectI worldBounds(mBounds);
    // Calculate rendering rectangle for the groups content
    RectI contentRect(mBarWidth, mBounds.extent - (mBarWidth + Point2I(1, 1)));

    Point2I localPoint(globalToLocalCoord(event.mousePoint));

    U32 textXOffset = mFloor((F32)(mBarWidth.x - mProfile->mFonts[0].mFont->getHeight()) / 2);
    RectI addFieldRect(Point2I(textXOffset, mBarWidth.y + 4), Point2I(mBarWidth.x, mBounds.extent.y));

    if (addFieldRect.pointInRect(localPoint))
    {
        addDynamicField();
    }
}

S32 GuiInspectorDynamicGroup::getExpandedHeight()
{
    if (mStack != NULL && mStack->size() > 0)
        return mStack->getHeight() + mBarWidth.y + 1;
    else
        return mBarWidth.y + 60;
}

void GuiInspectorDynamicGroup::addDynamicField()
{
    // We can't add a field without a target
    if (!mTarget || !mStack)
    {
        Con::warnf("GuiInspectorDynamicGroup::addDynamicField - no target SimObject to add a dynamic field to.");
        return;
    }

    Con::evaluatef("%d.%s = \"Default Value\";", mTarget->getId(), "NewDynamicField");

    SimFieldDictionary::Entry* entry = findDynamicFieldInDictionary("NewDynamicField");
    if (entry == NULL)
    {
        Con::warnf("GuiInspectorDynamicGroup::addDynamicField - Unable to locate new dynamic field");
        return;
    }

    GuiInspectorDynamicField* field = findDynamicField(entry->slotName);
    if (field != NULL)
    {
        Con::warnf("GuiInspectorDynamicGroup::addDynamicField - Dynamic Field already exists with name (%s)", entry->slotName);
        return;
    }

    field = new GuiInspectorDynamicField(this, mTarget, entry);
    if (field != NULL)
    {
        field->registerObject();
        mChildren.push_back(field);
        mStack->addObject(field);
    }

    animateTo(getExpandedHeight());
}

ConsoleMethod(GuiInspectorDynamicGroup, addDynamicField, void, 2, 2, "obj.addDynamicField();")
{
    object->addDynamicField();
}

//////////////////////////////////////////////////////////////////////////
// GuiInspectorDynamicField - Child class of GuiInspectorField 
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_CONOBJECT(GuiInspectorDynamicField);

GuiInspectorDynamicField::GuiInspectorDynamicField(GuiInspectorGroup* parent, SimObjectPtr<SimObject> target, SimFieldDictionary::Entry* field)
{
    if (field != NULL)
        mCaption = StringTable->insert(field->slotName);
    else
        mCaption = StringTable->insert("");

    mParent = parent;
    mTarget = target;
    mDynField = field;
    mBounds.set(0, 0, 100, 20);
    mRenameCtrl = NULL;
}

void GuiInspectorDynamicField::setData(StringTableEntry data)
{
    if (mTarget == NULL || mDynField == NULL)
        return;

    char buf[1024];
    const char* newValue = mEdit->getScriptValue();
    dStrcpy(buf, newValue ? newValue : "");
    collapseEscape(buf);

    mTarget->getFieldDictionary()->setFieldValue(mDynField->slotName, buf);

    // Force our edit to update
    updateValue(data);

}

StringTableEntry GuiInspectorDynamicField::getData()
{
    if (mTarget == NULL || mDynField == NULL)
        return "";

    return mTarget->getFieldDictionary()->getFieldValue(mDynField->slotName);
}

void GuiInspectorDynamicField::renameField(StringTableEntry newFieldName)
{
    if (mTarget == NULL || mDynField == NULL || mParent == NULL || mEdit == NULL)
    {
        Con::warnf("GuiInspectorDynamicField::renameField - No target object or dynamic field data found!");
        return;
    }

    if (!newFieldName)
    {
        Con::warnf("GuiInspectorDynamicField::renameField - Invalid field name specified!");
        return;
    }

    // Only proceed if the name has changed
    if (dStricmp(newFieldName, getFieldName()) == 0)
        return;

    // Grab a pointer to our parent and cast it to GuiInspectorDynamicGroup
    GuiInspectorDynamicGroup* group = dynamic_cast<GuiInspectorDynamicGroup*>(mParent);

    if (group == NULL)
    {
        Con::warnf("GuiInspectorDynamicField::renameField - Unable to locate GuiInspectorDynamicGroup parent!");
        return;
    }

    // Grab our current dynamic field value
    StringTableEntry currentValue = getData();

    // Create our new field with the value of our old field and the new fields name!
    mTarget->setDataField(newFieldName, NULL, currentValue);

    // Configure our field to grab data from the new dynamic field
    SimFieldDictionary::Entry* newEntry = group->findDynamicFieldInDictionary(newFieldName);

    if (newEntry == NULL)
    {
        Con::warnf("GuiInspectorDynamicField::renameField - Unable to find new field!");
        return;
    }

    // Set our old fields data to "" (which will effectively erase the field)
    mTarget->setDataField(getFieldName(), NULL, "");

    // Assign our dynamic field pointer (where we retrieve field information from) to our new field pointer
    mDynField = newEntry;

    // Reassign the caption of the field (to match the new field name)
    // Note : we use the getFieldName accessor which, since we have changed our field pointer
    //        will grab the slotName from the new field
    mCaption = getFieldName();

    // Lastly we need to reassign our Command and AltCommand fields for our value edit control
    char szBuffer[512];
    dSprintf(szBuffer, 512, "%d.%s = %d.getText();", mTarget->getId(), getFieldName(), mEdit->getId());
    mEdit->setField("AltCommand", szBuffer);
    mEdit->setField("Validate", szBuffer);
}

ConsoleMethod(GuiInspectorDynamicField, renameField, void, 3, 3, "field.renameField(newDynamicFieldName);")
{
    object->renameField(argv[2]);
}

bool GuiInspectorDynamicField::onAdd()
{
    if (!Parent::onAdd())
        return false;

    mRenameCtrl = constructRenameControl();

    return true;
}

GuiControl* GuiInspectorDynamicField::constructRenameControl()
{
    // Create our renaming field
    GuiControl* retCtrl = new GuiTextEditCtrl();

    // If we couldn't construct the control, bail!
    if (retCtrl == NULL)
        return retCtrl;

    // Let's make it look pretty.
    retCtrl->setField("profile", "GuiInspectorTextEditProfile");

    // Don't forget to register ourselves
    char szName[512];
    dSprintf(szName, 512, "IE_%s_%d_%s_Rename", retCtrl->getClassName(), mTarget->getId(), getFieldName());
    retCtrl->registerObject(szName);


    // Our command will evaluate to :
    //
    //    if( (editCtrl).getText() !$= "" )
    //       (field).renameField((editCtrl).getText());
    //
    char szBuffer[512];
    dSprintf(szBuffer, 512, "if( %d.getText() !$= \"\" ) %d.renameField(%d.getText());", retCtrl->getId(), getId(), retCtrl->getId());
    dynamic_cast<GuiTextEditCtrl*>(retCtrl)->setText(getFieldName());
    retCtrl->setField("AltCommand", szBuffer);
    retCtrl->setField("Validate", szBuffer);

    // Calculate Caption Rect (Adjust for 16 pixel wide delete button)
    RectI captionRect(Point2I(mBounds.point.x + 16, 0), Point2I(mFloor(mBounds.extent.x * (F32)((F32)GuiInspectorField::smCaptionWidth / 100.0)) - 16, mBounds.extent.y));

    RectI deleteRect(Point2I(mBounds.point.x, 2), Point2I(16, mBounds.extent.y - 4));
    addObject(retCtrl);

    // Resize the edit control to fit in our caption rect (tricksy!)
    retCtrl->resize(captionRect.point, captionRect.extent);

    // Finally, add a delete button for this field
    GuiButtonCtrl* delButt = new GuiBitmapButtonCtrl();
    if (delButt != NULL)
    {
        dSprintf(szBuffer, 512, "%d.%s = \"\";%d.inspect(%d);", mTarget->getId(), getFieldName(), mParent->getContentCtrl()->getId(), mTarget->getId());

        delButt->setField("Bitmap", "common/gui/images/trashCan");
        delButt->setField("Text", "X");
        delButt->setField("Command", szBuffer);
        delButt->registerObject();

        delButt->resize(deleteRect.point, deleteRect.extent);

        addObject(delButt);
    }

    return retCtrl;
}

void GuiInspectorDynamicField::resize(const Point2I& newPosition, const Point2I& newExtent)
{
    Parent::resize(newPosition, newExtent);

    // If we don't have a field rename control, bail!
    if (mRenameCtrl == NULL)
        return;

    // Calculate Caption Rect
    RectI captionRect(Point2I(mBounds.point.x + 16, 0), Point2I(mFloor(mBounds.extent.x * (F32)((F32)GuiInspectorField::smCaptionWidth / 100.0)) - 16, mBounds.extent.y));

    // Resize the edit control to fit in our caption rect (tricksy!)
    mRenameCtrl->resize(captionRect.point, captionRect.extent);
}

void GuiInspectorDynamicField::onRender(Point2I offset, const RectI& updateRect)
{
    RectI worldRect(offset, mBounds.extent);
    // Calculate Caption Rect
    RectI captionRect(offset, Point2I(mFloor(mBounds.extent.x * (F32)((F32)GuiInspectorField::smCaptionWidth / 100.0)), mBounds.extent.y));
    // Calculate Edit Field Rect
    RectI editFieldRect(offset + Point2I(captionRect.extent.x + 1, 0), Point2I(mBounds.extent.x - (captionRect.extent.x + 5), mBounds.extent.y));

    // Calculate Y Offset to center vertically the caption
    U32 captionYOffset = mFloor((F32)(captionRect.extent.y - mProfile->mFonts[0].mFont->getHeight()) / 2);

    renderFilledBorder(captionRect, mProfile->mBorderColor, mProfile->mFillColor);
    renderFilledBorder(editFieldRect, mProfile->mBorderColor, mProfile->mFillColor);

    // Skip directly to GuiControl onRender (so we get child controls rendered, but no caption rendering from GuiInspectorField::onRender)
    GuiControl::onRender(offset, updateRect);
}


//////////////////////////////////////////////////////////////////////////
// GuiInspectorDatablockField 
// Field construction for datablock types
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_CONOBJECT(GuiInspectorDatablockField);

static S32 QSORT_CALLBACK stringCompare(const void* a, const void* b)
{
    StringTableEntry sa = *(StringTableEntry*)a;
    StringTableEntry sb = *(StringTableEntry*)b;
    return(dStricmp(sb, sa));
}

GuiInspectorDatablockField::GuiInspectorDatablockField(StringTableEntry className)
{
    setClassName(className);
};

void GuiInspectorDatablockField::setClassName(StringTableEntry className)
{
    // Walk the ACR list and find a matching class if any.
    AbstractClassRep* walk = AbstractClassRep::getClassList();
    while (walk)
    {
        if (!dStricmp(walk->getClassName(), className))
        {
            // Match!
            mDesiredClass = walk;
            return;
        }

        walk = walk->getNextClass();
    }

    // No dice.
    Con::warnf("GuiInspectorDatablockField::setClassName - no class '%s' found!", className);
    return;
}

GuiControl* GuiInspectorDatablockField::constructEditControl()
{
    GuiControl* retCtrl = new GuiPopUpMenuCtrl();

    // If we couldn't construct the control, bail!
    if (retCtrl == NULL)
        return retCtrl;

    GuiPopUpMenuCtrl* menu = dynamic_cast<GuiPopUpMenuCtrl*>(retCtrl);

    // Let's make it look pretty.
    retCtrl->setField("profile", "InspectorTypeEnumProfile");

    menu->setField("text", getData());

    registerEditControl(retCtrl);

    // Configure it to update our value when the popup is closed
    char szBuffer[512];
    dSprintf(szBuffer, 512, "%d.%s = %d.getText();%d.inspect(%d);", mTarget->getId(), mField->pFieldname, menu->getId(), mParent->mParent->getId(), mTarget->getId());
    menu->setField("Command", szBuffer);

    Vector<StringTableEntry> entries;

    SimDataBlockGroup* grp = Sim::getDataBlockGroup();
    for (SimDataBlockGroup::iterator i = grp->begin(); i != grp->end(); i++)
    {
        SimDataBlock* datablock = dynamic_cast<SimDataBlock*>(*i);

        // Skip non-datablocks if we somehow encounter them.
        if (!datablock)
            continue;

        // Ok, now we have to figure inheritance info.
        if (datablock && datablock->getClassRep()->isClass(mDesiredClass))
            entries.push_back(datablock->getName());
    }

    // sort the entries
    dQsort(entries.address(), entries.size(), sizeof(StringTableEntry), stringCompare);

    // add them to our enum
    for (U32 j = 0; j < entries.size(); j++)
        menu->addEntry(entries[j], 0);

    return retCtrl;
}
