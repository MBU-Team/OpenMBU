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
#include "gfx/primBuilder.h"

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

#ifdef MB_ULTRA
    Point2F mEllipseScreenFraction;
    F32 mMaxArrowAlpha;
    F32 mMaxTargetAlpha;
    F32 mFullArrowLength;
    F32 mFullArrowWidth;
    F32 mMinArrowFraction;
#endif

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
    mVerticalOffset = 0.5f;
    mDistanceFade = 0.1f;

#ifdef MB_ULTRA
    mEllipseScreenFraction.x = 0.75;
    mEllipseScreenFraction.y = 0.75;
    mMaxArrowAlpha = 0.6f;
    mMaxTargetAlpha = 0.4f;
    mFullArrowLength = 60.0f;
    mFullArrowWidth = 40.0f;
    mMinArrowFraction = 0.3f;
#endif
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

#ifdef MB_ULTRA
    addField("ellipseScreenFraction", TypePoint2F, Offset(mEllipseScreenFraction, GuiShapeNameHud));
    addField("maxArrowAlpha", TypeF32, Offset(mMaxArrowAlpha, GuiShapeNameHud));
    addField("maxTargetAlpha", TypeF32, Offset(mMaxTargetAlpha, GuiShapeNameHud));
    addField("fullArrowLength", TypeF32, Offset(mFullArrowLength, GuiShapeNameHud));
    addField("fullArrowWidth", TypeF32, Offset(mFullArrowWidth, GuiShapeNameHud));
    addField("minArrowFraction", TypeF32, Offset(mMinArrowFraction, GuiShapeNameHud));
#endif
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
    static U32 losMask = AtlasObjectType | TerrainObjectType | InteriorObjectType | ShapeBaseObjectType | StaticTSObjectType;
    control->disableCollision();

#ifdef MB_ULTRA
    S32 hudItemCount = 0;
#endif

    // All ghosted objects are added to the server connection group,
    // so we can find all the shape base objects by iterating through
    // our current connection.
    for (SimSetIterator itr(conn); *itr; ++itr) {
        if (((*itr)->getType() & ShapeBaseObjectType) != 0 && !(*itr)->isHidden()) {
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
    offset.x -= mProfile->mFonts[0].mFont->getStrWidth((const UTF8*)name) / 2;
    offset.y -= mProfile->mFonts[0].mFont->getHeight();

    // Deal with opacity and draw.
    mTextColor.alpha = opacity;
    GFX->setBitmapModulation(mTextColor);
    GFX->drawText(mProfile->mFonts[0].mFont, offset, name);
    GFX->clearBitmapModulation();
}

#ifdef MB_ULTRA
void GuiShapeNameHud::renderArrow(ShapeBase* theObject, Point3F shapePos)
{
    GuiTSCtrl* parent = dynamic_cast<GuiTSCtrl*>(getParent());
    if (!parent)
        return;

    GameConnection* con = GameConnection::getConnectionToServer();
    if (!con)
        return;

    ShapeBase* control = con->getControlObject();
    if (!control)
        return;
    
    Marble* marble = dynamic_cast<Marble*>(control);

    MatrixF gravityMat;
    if (marble)
        marble->getGravityRenderFrame().setMatrix(&gravityMat)->inverse();
    else
        gravityMat.identity();

    MatrixF cam = parent->mLastCameraQuery.cameraMatrix;
    Point3F shapeDir = shapePos - cam.getPosition();

    U32 lifetime = Platform::getVirtualMilliseconds() - theObject->mCreateTime;

    F32 distToShape = shapeDir.len();
    shapeDir.normalize();

    bool projValid = false;

    F32 fov = parent->mLastCameraQuery.fov * 0.5f;
    F32 fovy = parent->mLastCameraQuery.fovy * 0.5f;

    Point3F projPnt;
    if (parent->project(shapePos, &projPnt) && projPnt.x > -75.0f && projPnt.y > -75.0f)
    {
        if ((F32)mBounds.extent.x + 75.0f > projPnt.x && (F32)mBounds.extent.y + 75.0f > projPnt.y)
            projValid = true;
    }

    Point2F projPointOnScreen(projPnt.x, projPnt.y);
    if (projPointOnScreen.x < 0.0f)
        projPointOnScreen.x = 0.0f;
    if (projPointOnScreen.y < 0.0f)
        projPointOnScreen.y = 0.0f;

    if (mBounds.extent.x < projPointOnScreen.x)
        projPointOnScreen.x = mBounds.extent.x;
    if (mBounds.extent.y < projPointOnScreen.y)
        projPointOnScreen.y = mBounds.extent.y;

    Point3F res;
    cam.getColumn(2, &res); // Up Dir
    Point3F camCone2G = mSin(fovy) * res;
    cam.getColumn(0, &res); // Right Dir
    Point3F camCone1G = res * mSin(fov); 
    cam.getColumn(1, &res); // Forward 
    camCone1G += res;

    Point3F unk0 = camCone1G + camCone2G;
    Point3F difference = camCone1G - camCone2G;

    Point3F shapeDirGrav;
    m_matF_x_point3F(gravityMat, unk0, camCone1G); // Direction of the top right?
    m_matF_x_point3F(gravityMat, difference, camCone2G);// Direction of the bottom right?
    m_matF_x_point3F(gravityMat, shapeDir, shapeDirGrav); // Shape dir but relative to gravity dir

    Point2F cc1(mSqrt(camCone1G.x * camCone1G.x + camCone1G.y * camCone1G.y), camCone1G.z);
    Point2F cc2(mSqrt(camCone2G.x * camCone2G.x + camCone2G.y * camCone2G.y), camCone2G.z);
    Point2F sd(mSqrt(shapeDirGrav.x * shapeDirGrav.x + shapeDirGrav.y * shapeDirGrav.y), shapeDirGrav.z);
    cc1.normalize();
    cc2.normalize();
    sd.normalize();

    Point2F arrowOnScreenPos(0.0f, 0.0f);
    // If sd.y is between cc1.y and cc2.y, then
    if (cc1.y >= sd.y)
    {
        if (cc2.y <= sd.y)
        {
            // the new y is the fraction of where it is
            arrowOnScreenPos.y = mBounds.extent.y * (sd.x * cc1.y - cc1.x * sd.y) / (sd.y * (cc2.x - cc1.x) - (cc2.y - cc1.y) * sd.x);
        } else
        {
            // otherwise snap to edge
            arrowOnScreenPos.y = mBounds.extent.y;
        }
    }

    bool blink = false;
    if (lifetime < 3000)
        blink = (Platform::getVirtualMilliseconds() / 500) & 1;

    MatrixF inverseCam = cam;
    inverseCam.inverse();

    // Converting the object's position from world space to camera space.
    Point3F xfPos;
    m_matF_x_point3F(inverseCam, shapePos, xfPos);
    xfPos.z = 0.0f;
    xfPos.normalize();

    Point3F normal(0.0f, 1.0f, 0.0f);
    Point3F crossProduct = mCross(xfPos, normal);
    F32 xfPosAngle = mAsin(crossProduct.z);
    F32 forwardness = mDot(normal, xfPos);

    F32 foldAmount = 0.0f;

    bool foldArrow = false;
    if (forwardness < 0.5f)
    {
        foldArrow = true;
        foldAmount = (0.5f - forwardness) * 0.5f * 0.66f;
    }

    // Is the object behind us?
    if (forwardness < 0.0f)
    {
        // Get the angle into the correct range
        if (xfPosAngle >= 0.0f)
            xfPosAngle += M_PI_F;
        else
            xfPosAngle -= M_PI_F;
    }

    // If aSinZ is between -fov and fov, then
    if (-fov <= xfPosAngle)
    {
        if (fov >= xfPosAngle)
        {
            // the new x is the fraction of where it is but convert it from an angle to tangent
            arrowOnScreenPos.x = mBounds.extent.x * 0.5f + mTan(xfPosAngle) * (mBounds.extent.x * 0.5f) / mTan(fov);
        } else
        {
            // otherwise snap to edge
            arrowOnScreenPos.x = mBounds.extent.x;
        }
    }

    Point2F drawPoint;
    // If the point we're trying to project isn't on screen, then
    if (!projValid)
    {
        // use the calculated arrow position
        drawPoint = arrowOnScreenPos;
    } else
    {
        // otherwise we set it to the projection point
        drawPoint.set(projPnt.x, projPnt.y);

        // if we're within 75 pixels of the screen
        if (drawPoint != projPointOnScreen)
        {
            // interpolate between the 2 positions based on how far offscreen the object is
            Point2F projPointOffscreen(projPointOnScreen.x - projPnt.x, projPointOnScreen.y - projPnt.y);
            F32 distanceOffscreen = projPointOffscreen.len();

            if (distanceOffscreen <= 75.0f)
                drawPoint.interpolate(projPointOnScreen, arrowOnScreenPos, distanceOffscreen / 75.0f);
            else
                drawPoint = arrowOnScreenPos;
        }
    }

    Point2F center(mBounds.extent.x, mBounds.extent.y);
    center *= 0.5f;

    Point2F ellipse = mEllipseScreenFraction * center;
    Point2F arrowDir = drawPoint - center;

    // The portion of the radius of arrowDir relative to the ellipse
    F32 ellipseDistance = arrowDir.x * arrowDir.x / (ellipse.x * ellipse.x)
                        + arrowDir.y * arrowDir.y / (ellipse.y * ellipse.y);

    ellipseDistance = mSqrt(ellipseDistance);

    F32 arrowAlpha = mMaxArrowAlpha;
    F32 circleAlpha = 0.0f;

    if (ellipseDistance <= 1.0f)
    {
        if (ellipseDistance <= 0.7f)
        {
            arrowAlpha = 0.0f;
            circleAlpha = mMaxTargetAlpha;
        } else
        {
            arrowAlpha = (ellipseDistance - 0.7f) * 3.333333333333333f * mMaxArrowAlpha;
            circleAlpha = mMaxArrowAlpha - arrowAlpha * mMaxTargetAlpha / mMaxArrowAlpha;
        }
    } else
    {
        drawPoint = arrowDir / ellipseDistance;
        drawPoint += center;
    }
    
    arrowDir.normalize();
    
    F32 arrowScale = mClampF(1.0f - distToShape / 100.0f, mMinArrowFraction, 1.0f);

    F32 arrowWidth = mFullArrowWidth * arrowScale;
    F32 arrowLength = mFullArrowLength * arrowScale;

    Point2F arrowSideVector(arrowWidth * arrowDir.y, arrowWidth * -arrowDir.x);
    Point2F halfArrowSideVec = arrowSideVector * 0.5f;
    
    Point2F arrowForwardVec = arrowLength * arrowDir;
    Point2F arrowBack = drawPoint - arrowForwardVec;

    Point2F lowerRight = arrowBack + halfArrowSideVec;
    Point2F lowerLeft = arrowBack - halfArrowSideVec;

    F32 halfFoldWidth = 0.5f * arrowWidth * foldAmount;
    Point2F halfFoldForwardWidthVec(halfFoldWidth * arrowDir.y, -halfFoldWidth * arrowDir.x);

    F32 foldLength = foldAmount * arrowLength;
    Point2F foldForwardVec = foldLength * arrowDir;

    Point2F foldBack = drawPoint - foldForwardVec;
    Point2F foldLowerRight = foldBack + halfFoldForwardWidthVec;
    Point2F foldLowerLeft = foldBack - halfFoldForwardWidthVec;

    Point2F doubleFoldVec = arrowDir * (foldLength + foldLength);

    Point2F foldedTip = drawPoint - doubleFoldVec;

    GFX->setBaseRenderState();
    GFX->setCullMode(GFXCullNone);
    GFX->setZEnable(false);
    GFX->setLightingEnable(false);
    GFX->setAlphaBlendEnable(true);
    GFX->setSrcBlend(GFXBlendSrcAlpha);
    GFX->setDestBlend(GFXBlendInvSrcAlpha);

    GFX->setTextureStageColorOp(0, GFXTOPDisable);

    MatrixF newMat(true);
    GFX->setWorldMatrix(newMat);

    ShapeBaseData* db = (ShapeBaseData*)theObject->getDataBlock();

    ColorF refColor = db->referenceColor;
    if (blink)
    {
        refColor.red = getMax(refColor.red, 0.5f);
        refColor.green = getMax(refColor.green, 0.5f);
        refColor.blue = getMax(refColor.blue, 0.5f);

        arrowAlpha *= 1.3f;
        if (arrowAlpha > 1.0f)
            arrowAlpha = 1.0f;
        circleAlpha *= 1.3f;
        if (circleAlpha > 1.0f)
            circleAlpha = 1.0f;
    }

    if (arrowAlpha != 0.0f)
    {
        PrimBuild::begin(GFXTriangleList, 6);
        GFX->setupGenericShaders();
        PrimBuild::color4f(refColor.red, refColor.green, refColor.blue, arrowAlpha);
        if (foldArrow)
        {
            PrimBuild::vertex2f(lowerRight.x, lowerRight.y);
            PrimBuild::vertex2f(lowerLeft.x, lowerLeft.y);
            PrimBuild::vertex2f(foldLowerLeft.x, foldLowerLeft.y);
            PrimBuild::vertex2f(lowerRight.x, lowerRight.y);
            PrimBuild::vertex2f(foldLowerRight.x, foldLowerRight.y);
            PrimBuild::vertex2f(foldLowerLeft.x, foldLowerLeft.y);
        } else
        {
            PrimBuild::vertex2f(drawPoint.x, drawPoint.y);
            PrimBuild::vertex2f(lowerRight.x, lowerRight.y);
            PrimBuild::vertex2f(lowerLeft.x, lowerLeft.y);
        }

        PrimBuild::end();
        PrimBuild::begin(GFXLineList, 12);
        GFX->setupGenericShaders();
        PrimBuild::color4f(0.0f, 0.0f, 0.0f, arrowAlpha);
        if (foldArrow)
        {
            PrimBuild::vertex2f(lowerRight.x, lowerRight.y);
            PrimBuild::vertex2f(foldLowerRight.x, foldLowerRight.y);
            PrimBuild::vertex2f(foldLowerRight.x, foldLowerRight.y);
            PrimBuild::vertex2f(foldLowerLeft.x, foldLowerLeft.y);
            PrimBuild::vertex2f(foldLowerLeft.x, foldLowerLeft.y);
            PrimBuild::vertex2f(lowerLeft.x, lowerLeft.y);
            PrimBuild::vertex2f(lowerLeft.x, lowerLeft.y);
            PrimBuild::vertex2f(lowerRight.x, lowerRight.y);
            PrimBuild::vertex2f(foldLowerRight.x, foldLowerRight.y);
            PrimBuild::vertex2f(foldedTip.x, foldedTip.y);
            PrimBuild::vertex2f(foldLowerLeft.x, foldLowerLeft.y);
            PrimBuild::vertex2f(foldedTip.x, foldedTip.y);
        } else
        {
            PrimBuild::vertex2f(drawPoint.x, drawPoint.y);
            PrimBuild::vertex2f(lowerRight.x, lowerRight.y);
            PrimBuild::vertex2f(lowerRight.x, lowerRight.y);
            PrimBuild::vertex2f(lowerLeft.x, lowerLeft.y);
            PrimBuild::vertex2f(lowerLeft.x, lowerLeft.y);
            PrimBuild::vertex2f(drawPoint.x, drawPoint.y);
        }

        PrimBuild::end();
    }

    if (circleAlpha != 0.0f)
    {
        F32 arrowLen = mFullArrowLength * arrowScale * 0.4f;
        F32 halfArrowLen = arrowLen * 0.55f;

        GFX->setupGenericShaders();
        PrimBuild::color4f(refColor.red, refColor.green, refColor.blue, circleAlpha);
        
        if (arrowScale >= 0.7f)
        {
            if (arrowScale < 0.8f)
                arrowLen = ((arrowScale - 0.7f) * 10.0f) * (arrowLen - halfArrowLen) + halfArrowLen;
            PrimBuild::begin(GFXTriangleList, 12);

            Point2F topLeft = drawPoint - arrowLen;
            Point2F midTopLeft = topLeft + halfArrowLen;
            Point2F bottomRight = drawPoint + arrowLen;
            Point2F midBottomRight = bottomRight - halfArrowLen;

            // Top Left
            PrimBuild::vertex2f(midTopLeft.x, midTopLeft.y);
            PrimBuild::vertex2f(midTopLeft.x, topLeft.y);
            PrimBuild::vertex2f(topLeft.x, midTopLeft.y);

            // Top Right
            PrimBuild::vertex2f(midBottomRight.x, midTopLeft.y);
            PrimBuild::vertex2f(midBottomRight.x, topLeft.y);
            PrimBuild::vertex2f(bottomRight.x, midTopLeft.y);

            // Bottom Right
            PrimBuild::vertex2f(midBottomRight.x, midBottomRight.y);
            PrimBuild::vertex2f(midBottomRight.x, bottomRight.y);
            PrimBuild::vertex2f(bottomRight.x, midBottomRight.y);

            // Bottom Left
            PrimBuild::vertex2f(midTopLeft.x, midBottomRight.y);
            PrimBuild::vertex2f(midTopLeft.x, bottomRight.y);
            PrimBuild::vertex2f(topLeft.x, midBottomRight.y);

            PrimBuild::end();

            PrimBuild::begin(GFXLineList, 8);
            GFX->setupGenericShaders();
            PrimBuild::color4f(0.0f, 0.0f, 0.0f, circleAlpha);

            PrimBuild::vertex2f(midTopLeft.x, topLeft.y);
            PrimBuild::vertex2f(topLeft.x, midTopLeft.y);

            PrimBuild::vertex2f(midBottomRight.x, topLeft.y);
            PrimBuild::vertex2f(bottomRight.x, midTopLeft.y);

            PrimBuild::vertex2f(midBottomRight.x, bottomRight.y);
            PrimBuild::vertex2f(bottomRight.x, midBottomRight.y);

            PrimBuild::vertex2f(midTopLeft.x, bottomRight.y);
            PrimBuild::vertex2f(topLeft.x, midBottomRight.y);
        } else
        {
            PrimBuild::begin(GFXTriangleList, 6);
            Point2F halfTopLeft = drawPoint - halfArrowLen;
            Point2F halfBottomRight = drawPoint + halfArrowLen;

            // Top
            PrimBuild::vertex2f(halfTopLeft.x, drawPoint.y);
            PrimBuild::vertex2f(drawPoint.x, halfTopLeft.y);
            PrimBuild::vertex2f(halfBottomRight.x, drawPoint.y);

            // Bottom
            PrimBuild::vertex2f(halfTopLeft.x, drawPoint.y);
            PrimBuild::vertex2f(drawPoint.x, halfBottomRight.y);
            PrimBuild::vertex2f(halfBottomRight.x, drawPoint.y);

            PrimBuild::end();
            PrimBuild::begin(GFXLineList, 8);
            GFX->setupGenericShaders();
            PrimBuild::color4f(0.0f, 0.0f, 0.0f, circleAlpha);

            PrimBuild::vertex2f(halfBottomRight.x, drawPoint.y);
            PrimBuild::vertex2f(drawPoint.x, halfTopLeft.y);

            PrimBuild::vertex2f(drawPoint.x, halfTopLeft.y);
            PrimBuild::vertex2f(halfTopLeft.x, drawPoint.y);

            PrimBuild::vertex2f(halfTopLeft.x, drawPoint.y);
            PrimBuild::vertex2f(drawPoint.x, halfBottomRight.y);

            PrimBuild::vertex2f(drawPoint.x, halfBottomRight.y);
            PrimBuild::vertex2f(halfBottomRight.x, drawPoint.y);
        }

        PrimBuild::end();
    }

    GFX->setTextureStageColorOp(0, GFXTOPModulate);
    GFX->setAlphaBlendEnable(false);
}
#endif
