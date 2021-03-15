//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "gui/core/guiControl.h"
#include "gui/core/guiTSControl.h"
#include "console/consoleTypes.h"
#include "sceneGraph/sceneGraph.h"
#include "game/shapeBase.h"
#include "game/gameConnection.h"

#ifdef MB_ULTRA
#include "game/item.h"
#include "game/marble/marble.h"
#endif

//----------------------------------------------------------------------------
/// Displays name & damage above shape objects.
///
/// This control displays the name and damage value of all named
/// ShapeBase objects on the client.  The name and damage of objects
/// within the control's display area are overlayed above the object.
///
/// This GUI control must be a child of a TSControl, and a server connection
/// and control object must be present.
///
/// This is a stand-alone control and relies only on the standard base GuiControl.
class GuiShapeNameHud : public GuiControl {
    typedef GuiControl Parent;

    // field data
    ColorF   mFillColor;
    ColorF   mFrameColor;
    ColorF   mTextColor;

    F32      mVerticalOffset;
    F32      mDistanceFade;
    bool     mShowFrame;
    bool     mShowFill;

protected:
    void drawName(Point2I offset, const char* buf, F32 opacity);
#ifdef MB_ULTRA
    void renderArrow(ShapeBase* theObject, Point3F shapePos);
#endif

public:
    GuiShapeNameHud();

    // GuiControl
    virtual void onRender(Point2I offset, const RectI& updateRect);

    static void initPersistFields();
    DECLARE_CONOBJECT(GuiShapeNameHud);
};


//-----------------------------------------------------------------------------

IMPLEMENT_CONOBJECT(GuiShapeNameHud);

/// Default distance for object's information to be displayed.
static const F32 cDefaultVisibleDistance = 500.0f;

GuiShapeNameHud::GuiShapeNameHud()
{
    mFillColor.set(0.25, 0.25, 0.25, 0.25);
    mFrameColor.set(0, 1, 0, 1);
    mTextColor.set(0, 1, 0, 1);
    mShowFrame = mShowFill = true;
    mVerticalOffset = 0.5;
    mDistanceFade = 0.1;
}

void GuiShapeNameHud::initPersistFields()
{
    Parent::initPersistFields();
    addGroup("Colors");
    addField("fillColor", TypeColorF, Offset(mFillColor, GuiShapeNameHud));
    addField("frameColor", TypeColorF, Offset(mFrameColor, GuiShapeNameHud));
    addField("textColor", TypeColorF, Offset(mTextColor, GuiShapeNameHud));
    endGroup("Colors");

    addGroup("Misc");
    addField("showFill", TypeBool, Offset(mShowFill, GuiShapeNameHud));
    addField("showFrame", TypeBool, Offset(mShowFrame, GuiShapeNameHud));
    addField("verticalOffset", TypeF32, Offset(mVerticalOffset, GuiShapeNameHud));
    addField("distanceFade", TypeF32, Offset(mDistanceFade, GuiShapeNameHud));
    endGroup("Misc");
}


//----------------------------------------------------------------------------
/// Core rendering method for this control.
///
/// This method scans through all the current client ShapeBase objects.
/// If one is named, it displays the name and damage information for it.
///
/// Information is offset from the center of the object's bounding box,
/// unless the object is a PlayerObjectType, in which case the eye point
/// is used.
///
/// @param   updateRect   Extents of control.
void GuiShapeNameHud::onRender(Point2I, const RectI& updateRect)
{
    // Background fill first
    if (mShowFill)
        GFX->drawRectFill(updateRect, mFillColor);

    // Must be in a TS Control
    GuiTSCtrl* parent = dynamic_cast<GuiTSCtrl*>(getParent());
    if (!parent) return;

    // Must have a connection and control object
    GameConnection* conn = GameConnection::getConnectionToServer();
    if (!conn) return;
    ShapeBase* control = conn->getControlObject();
    if (!control) return;

    // Get control camera info
    MatrixF cam;
    Point3F camPos;
    VectorF camDir;
    conn->getControlCameraTransform(0, &cam);
    cam.getColumn(3, &camPos);
    cam.getColumn(1, &camDir);

    F32 camFov;
    conn->getControlCameraFov(&camFov);
    camFov = mDegToRad(camFov) / 2;

    // Visible distance info & name fading
    F32 visDistance = getCurrentClientSceneGraph()->getVisibleDistance();
    F32 visDistanceSqr = visDistance * visDistance;
    F32 fadeDistance = visDistance * mDistanceFade;

    // Collision info. We're going to be running LOS tests and we
    // don't want to collide with the control object.
    static U32 losMask = AtlasObjectType | TerrainObjectType | InteriorObjectType | ShapeBaseObjectType;
    control->disableCollision();

#ifdef MB_ULTRA
    S32 hudItemCount = 0;
#endif

    // All ghosted objects are added to the server connection group,
    // so we can find all the shape base objects by iterating through
    // our current connection.
    for (SimSetIterator itr(conn); *itr; ++itr) {
        if ((*itr)->getType() & ShapeBaseObjectType) {
            ShapeBase* shape = static_cast<ShapeBase*>(*itr);
#ifdef MB_ULTRA
            Item* item = dynamic_cast<Item*>(shape);
            if (item && ((ItemData*)(item->getDataBlock()))->addToHUDRadar)
            {
                Box3F itemBox = item->getRenderWorldBox();
                Point3F itemPos = (itemBox.min + itemBox.max) * 0.5f;
                
                renderArrow(item, itemPos);
                hudItemCount++;
            }
#endif
            if (shape != control && shape->getShapeName()) {

                // Target pos to test, if it's a player run the LOS to his eye
                // point, otherwise we'll grab the generic box center.
                Point3F shapePos;
                if (shape->getType() & PlayerObjectType) {
                    MatrixF eye;

                    // Use the render eye transform, otherwise we'll see jittering
                    shape->getRenderEyeTransform(&eye);
                    eye.getColumn(3, &shapePos);
                }
                else {
                    // Use the render transform instead of the box center
                    // otherwise it'll jitter.
                    MatrixF srtMat = shape->getRenderTransform();
                    srtMat.getColumn(3, &shapePos);
                }
                VectorF shapeDir = shapePos - camPos;

                // Test to see if it's in range
                F32 shapeDist = shapeDir.lenSquared();
                if (shapeDist == 0 || shapeDist > visDistanceSqr)
                    continue;
                shapeDist = mSqrt(shapeDist);

                // Test to see if it's within our viewcone, this test doesn't
                // actually match the viewport very well, should consider
                // projection and box test.
                shapeDir.normalize();
                F32 dot = mDot(shapeDir, camDir);
                if (dot < camFov)
                    continue;

                // Test to see if it's behind something, and we want to
                // ignore anything it's mounted on when we run the LOS.
                RayInfo info;
                shape->disableCollision();
                ShapeBase* mount = shape->getObjectMount();
                if (mount)
                    mount->disableCollision();
                bool los = !getCurrentClientContainer()->castRay(camPos, shapePos, losMask, &info);
                shape->enableCollision();
                if (mount)
                    mount->enableCollision();
                if (!los)
                    continue;

                // Project the shape pos into screen space and calculate
                // the distance opacity used to fade the labels into the
                // distance.
                Point3F projPnt;
                shapePos.z += mVerticalOffset;
                if (!parent->project(shapePos, &projPnt))
                    continue;
                F32 opacity = (shapeDist < fadeDistance) ? 1.0 :
                    1.0 - (shapeDist - fadeDistance) / (visDistance - fadeDistance);

                // Render the shape's name
                drawName(Point2I((S32)projPnt.x, (S32)projPnt.y), shape->getShapeName(), opacity);
            }
        }
    }

#ifdef MB_ULTRA
    if (hudItemCount == 0 && !Marble::smEndPad.isNull())
    {
        Marble* marble = dynamic_cast<Marble*>(control);
        if (marble && (marble->getMode() & Marble::StoppingMode) == 0)
        {
            Box3F padBox = Marble::smEndPad->getRenderWorldBox();
            Point3F padPos = (padBox.min + padBox.max) * 0.5f;

            renderArrow(Marble::smEndPad, padPos);
        }
    }
#endif

    // Restore control object collision
    control->enableCollision();

    // Border last
    if (mShowFrame)
        GFX->drawRect(updateRect, mFrameColor);
}


//----------------------------------------------------------------------------
/// Render object names.
///
/// Helper function for GuiShapeNameHud::onRender
///
/// @param   offset  Screen coordinates to render name label. (Text is centered
///                  horizontally about this location, with bottom of text at
///                  specified y position.)
/// @param   name    String name to display.
/// @param   opacity Opacity of name (a fraction).
void GuiShapeNameHud::drawName(Point2I offset, const char* name, F32 opacity)
{
    // Center the name
    offset.x -= mProfile->mFont->getStrWidth((const UTF8*)name) / 2;
    offset.y -= mProfile->mFont->getHeight();

    // Deal with opacity and draw.
    mTextColor.alpha = opacity;
    GFX->setBitmapModulation(mTextColor);
    GFX->drawText(mProfile->mFont, offset, name);
    GFX->clearBitmapModulation();
}

#ifdef MB_ULTRA
void GuiShapeNameHud::renderArrow(ShapeBase* theObject, Point3F shapePos)
{
    // TODO: Implement GuiShapeNameHud::renderArrow
}
#endif
