//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "gui/core/guiControl.h"
#include "gui/controls/guiBitmapCtrl.h"
#include "console/consoleTypes.h"
#include "sceneGraph/sceneGraph.h"
#include "game/gameConnection.h"
#include "game/shapeBase.h"

//-----------------------------------------------------------------------------
/// Vary basic cross hair hud.
/// Uses the base bitmap control to render a bitmap, and decides whether
/// to draw or not depending on the current control object and it's state.
/// If there is ShapeBase object under the cross hair and it's named,
/// then a small health bar is displayed.
class GuiCrossHairHud : public GuiBitmapCtrl
{
    typedef GuiBitmapCtrl Parent;

    ColorF   mDamageFillColor;
    ColorF   mDamageFrameColor;
    Point2I  mDamageRectSize;
    Point2I  mDamageOffset;

protected:
    void drawDamage(Point2I offset, F32 damage, F32 opacity);

public:
    GuiCrossHairHud();

    void onRender(Point2I, const RectI&);
    static void initPersistFields();
    DECLARE_CONOBJECT(GuiCrossHairHud);
};

/// Valid object types for which the cross hair will render, this
/// should really all be script controlled.
static const U32 ObjectMask = PlayerObjectType | VehicleObjectType;


//-----------------------------------------------------------------------------

IMPLEMENT_CONOBJECT(GuiCrossHairHud);

GuiCrossHairHud::GuiCrossHairHud()
{
    mDamageFillColor.set(0, 1, 0, 1);
    mDamageFrameColor.set(1, 0.6, 0, 1);
    mDamageRectSize.set(50, 4);
    mDamageOffset.set(0, 32);
}

void GuiCrossHairHud::initPersistFields()
{
    Parent::initPersistFields();
    addGroup("Damage");
    addField("damageFillColor", TypeColorF, Offset(mDamageFillColor, GuiCrossHairHud));
    addField("damageFrameColor", TypeColorF, Offset(mDamageFrameColor, GuiCrossHairHud));
    addField("damageRect", TypePoint2I, Offset(mDamageRectSize, GuiCrossHairHud));
    addField("damageOffset", TypePoint2I, Offset(mDamageOffset, GuiCrossHairHud));
    endGroup("Damage");
}


//-----------------------------------------------------------------------------

void GuiCrossHairHud::onRender(Point2I offset, const RectI& updateRect)
{
    // Must have a connection and player control object
    GameConnection* conn = GameConnection::getConnectionToServer();
    if (!conn)
        return;
    ShapeBase* control = conn->getControlObject();
    if (!control || !(control->getType() & ObjectMask) || !conn->isFirstPerson())
        return;

    // Parent render.
    Parent::onRender(offset, updateRect);

    // Get control camera info
    MatrixF cam;
    Point3F camPos;
    conn->getControlCameraTransform(0, &cam);
    cam.getColumn(3, &camPos);

    // Extend the camera vector to create an endpoint for our ray
    Point3F endPos;
    cam.getColumn(1, &endPos);
    endPos *= getCurrentClientSceneGraph()->getVisibleDistance();
    endPos += camPos;

    // Collision info. We're going to be running LOS tests and we
    // don't want to collide with the control object.
    static U32 losMask = AtlasObjectType | TerrainObjectType | InteriorObjectType | ShapeBaseObjectType;
    control->disableCollision();

    RayInfo info;
    if (getCurrentClientContainer()->castRay(camPos, endPos, losMask, &info)) {
        // Hit something... but we'll only display health for named
        // ShapeBase objects.  Could mask against the object type here
        // and do a static cast if it's a ShapeBaseObjectType, but this
        // isn't a performance situation, so I'll just use dynamic_cast.
        if (ShapeBase* obj = dynamic_cast<ShapeBase*>(info.object))
            if (obj->getShapeName()) {
                offset.x = updateRect.point.x + updateRect.extent.x / 2;
                offset.y = updateRect.point.y + updateRect.extent.y / 2;
                drawDamage(offset + mDamageOffset, obj->getDamageValue(), 1);
            }
    }

    // Restore control object collision
    control->enableCollision();
}


//-----------------------------------------------------------------------------
/**
   Display a damage bar ubove the shape.
   This is a support funtion, called by onRender.
*/
void GuiCrossHairHud::drawDamage(Point2I offset, F32 damage, F32 opacity)
{
    mDamageFillColor.alpha = mDamageFrameColor.alpha = opacity;

    // Damage should be 0->1 (0 being no damage,or healthy), but
    // we'll just make sure here as we flip it.
    damage = mClampF(1 - damage, 0, 1);

    // Center the bar
    RectI rect(offset, mDamageRectSize);
    rect.point.x -= mDamageRectSize.x / 2;

    // Draw the border
    GFX->drawRect(rect, mDamageFrameColor);

    // Draw the damage % fill
    rect.point += Point2I(1, 1);
    rect.extent -= Point2I(1, 1);
    rect.extent.x = (S32)(rect.extent.x * damage);
    if (rect.extent.x == 1)
        rect.extent.x = 2;
    if (rect.extent.x > 0)
        GFX->drawRectFill(rect, mDamageFillColor);
}
