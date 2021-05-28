//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "console/console.h"
#include "console/consoleTypes.h"
#include "game/game.h"
#include "terrain/terrData.h"
#include "editor/guiTerrPreviewCtrl.h"

IMPLEMENT_CONOBJECT(GuiTerrPreviewCtrl);

GuiTerrPreviewCtrl::GuiTerrPreviewCtrl(void)
{
    mTerrainSize = 2048.0f;
    mRoot.set(0, 0);
    mOrigin.set(0, 0);
    mWorldScreenCenter.set(mTerrainSize * 0.5f, mTerrainSize * 0.5f);
}

void GuiTerrPreviewCtrl::initPersistFields()
{
    Parent::initPersistFields();
}


ConsoleMethod(GuiTerrPreviewCtrl, reset, void, 2, 2, "Reset the view of the terrain.")
{
    object->reset();
}

ConsoleMethod(GuiTerrPreviewCtrl, setRoot, void, 2, 2, "Add the origin to the root and reset the origin.")
{
    object->setRoot();
}

ConsoleMethod(GuiTerrPreviewCtrl, getRoot, const char*, 2, 2, "Return a Point2F representing the position of the root.")
{
    Point2F p = object->getRoot();

    static char rootbuf[32];
    dSprintf(rootbuf, sizeof(rootbuf), "%g %g", p.x, -p.y);
    return rootbuf;
}

ConsoleMethod(GuiTerrPreviewCtrl, setOrigin, void, 4, 4, "(float x, float y)"
    "Set the origin of the view.")
{
    object->setOrigin(Point2F(dAtof(argv[2]), -dAtof(argv[3])));
}

ConsoleMethod(GuiTerrPreviewCtrl, getOrigin, const char*, 2, 2, "Return a Point2F containing the position of the origin.")
{
    Point2F p = object->getOrigin();

    static char originbuf[32];
    dSprintf(originbuf, sizeof(originbuf), "%g %g", p.x, -p.y);
    return originbuf;
}

ConsoleMethod(GuiTerrPreviewCtrl, getValue, const char*, 2, 2, "Returns a 4-tuple containing: root_x root_y origin_x origin_y")
{
    Point2F r = object->getRoot();
    Point2F o = object->getOrigin();

    static char valuebuf[64];
    dSprintf(valuebuf, sizeof(valuebuf), "%g %g %g %g", r.x, -r.y, o.x, -o.y);
    return valuebuf;
}

ConsoleMethod(GuiTerrPreviewCtrl, setValue, void, 3, 3, "Accepts a 4-tuple in the same form as getValue returns.\n\n"
    "@see GuiTerrPreviewCtrl::getValue()")
{
    Point2F r, o;
    dSscanf(argv[2], "%g %g %g %g", &r.x, &r.y, &o.x, &o.y);
    r.y = -r.y;
    o.y = -o.y;
    object->reset();
    object->setRoot(r);
    object->setOrigin(o);
}

bool GuiTerrPreviewCtrl::onWake()
{
    if (!Parent::onWake())
        return false;

    return true;
}

void GuiTerrPreviewCtrl::onSleep()
{
    Parent::onSleep();
}

/*
void GuiTerrPreviewCtrl::setBitmap(const TextureHandle &handle)
{
   mTextureHandle = handle;
}
*/

void GuiTerrPreviewCtrl::reset()
{
    mRoot.set(0, 0);
    mOrigin.set(0, 0);
}

void GuiTerrPreviewCtrl::setRoot()
{
    mRoot += mOrigin;
    mOrigin.set(0, 0);
}

void GuiTerrPreviewCtrl::setRoot(const Point2F& p)
{
    mRoot = p;
}

void GuiTerrPreviewCtrl::setOrigin(const Point2F& p)
{
    mOrigin = p;
}


Point2F& GuiTerrPreviewCtrl::wrap(const Point2F& p)
{
    static Point2F result;
    result = p;

    while (result.x < 0.0f)
        result.x += mTerrainSize;
    while (result.x > mTerrainSize)
        result.x -= mTerrainSize;
    while (result.y < 0.0f)
        result.y += mTerrainSize;
    while (result.y > mTerrainSize)
        result.y -= mTerrainSize;

    return result;
}

Point2F& GuiTerrPreviewCtrl::worldToTexture(const Point2F& p)
{
    static Point2F result;
    result = wrap(p + mRoot) / mTerrainSize;
    return result;
}


Point2F& GuiTerrPreviewCtrl::worldToCtrl(const Point2F& p)
{
    static Point2F result;
    result = wrap(p - mCamera + mOrigin - mWorldScreenCenter);
    result *= mBounds.extent.x / mTerrainSize;
    return result;
}


void GuiTerrPreviewCtrl::onPreRender()
{
    setUpdate();
}

void GuiTerrPreviewCtrl::onRender(Point2I offset, const RectI& updateRect)
{
#if 0
    struct CameraQuery query;
    GameProcessCameraQuery(&query);
    Point3F cameraRot;

    MatrixF matrix = query.cameraMatrix;
    matrix.getColumn(3, &cameraRot);           // get Camera translation
    mCamera.set(cameraRot.x, -cameraRot.y);
    matrix.getRow(1, &cameraRot);              // get camera rotation

    mTerrainSize = 8 * 256;
    TerrainBlock* terrBlock = dynamic_cast<TerrainBlock*>(Sim::findObject("Terrain"));
    if (terrBlock)
        mTerrainSize = terrBlock->getSquareSize() * TerrainBlock::BlockSize;


    //----------------------------------------- RENDER the Terrain Bitmap
    if (mTextureHandle)
    {
        TextureObject* texture = (TextureObject*)mTextureHandle;
        if (texture)
        {
            glDisable(GL_LIGHTING);
            glDisable(GL_BLEND);

            glEnable(GL_TEXTURE_2D);

            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
            glBindTexture(GL_TEXTURE_2D, texture->texGLName);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

            Point2F screenP1(offset.x - 0.5f, offset.y + 0.5f);
            Point2F screenP2(offset.x + mBounds.extent.x - 0.5f, offset.y + mBounds.extent.x + 0.5f);
            Point2F textureP1(worldToTexture(mCamera));
            Point2F textureP2(textureP1 + Point2F(1.0f, 1.0f));

            // the texture if flipped horz to reflect how the terrain is really drawn
            glDisable(GL_CULL_FACE);
            glBegin(GL_TRIANGLE_FAN);
            glTexCoord2f(textureP1.x, textureP2.y);
            glVertex2f(screenP1.x, screenP2.y);       // left bottom

            glTexCoord2f(textureP2.x, textureP2.y);
            glVertex2f(screenP2.x, screenP2.y);       // right bottom

            glTexCoord2f(textureP2.x, textureP1.y);
            glVertex2f(screenP2.x, screenP1.y);       // right top

            glTexCoord2f(textureP1.x, textureP1.y);
            glVertex2f(screenP1.x, screenP1.y);       // left top
            glEnd();

            glDisable(GL_TEXTURE_2D);
        }
    }
    else
    {
        RectI rect(offset.x, offset.y, mBounds.extent.x, mBounds.extent.y);
        dglDrawRectFill(rect, ColorI(0, 0, 0));
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //----------------------------------------- RENDER the '+' at the center of the Block

    glColor4f(1.0f, 1.0f, 1.0f, 0.7f);
    Point2F center(worldToCtrl(Point2F(0, 0)));
    S32 y;
    for (y = -1; y <= 1; y++)
    {
        F32 yoffset = offset.y + y * 256.0f;
        for (S32 x = -1; x <= 1; x++)
        {
            F32 xoffset = offset.x + x * 256.0f;
            glBegin(GL_LINES);
            glVertex2f(xoffset + center.x, yoffset + center.y - 5);
            glVertex2f(xoffset + center.x, yoffset + center.y + 6);
            glVertex2f(xoffset + center.x - 5, yoffset + center.y);
            glVertex2f(xoffset + center.x + 6, yoffset + center.y);
            glEnd();
        }
    }

    //----------------------------------------- RENDER the Block Corners
    Point2F cornerf(worldToCtrl(Point2F(-mTerrainSize / 2.0f, -mTerrainSize / 2.0f)));
    Point2I corner = Point2I((S32)cornerf.x, (S32)cornerf.y);
    for (y = -1; y <= 1; y++)
    {
        S32 yoffset = offset.y + y * 256;
        for (S32 x = -1; x <= 1; x++)
        {
            S32 xoffset = offset.x + x * 256;
            glBegin(GL_LINE_STRIP);
            glColor4f(1.0f, 1.0f, 1.0f, 0.0f);
            glVertex2i(xoffset + corner.x, yoffset + corner.y - 128);
            glColor4f(1.0f, 1.0f, 1.0f, 0.7f);
            glVertex2i(xoffset + corner.x, yoffset + corner.y);
            glColor4f(1.0f, 1.0f, 1.0f, 0.0f);
            glVertex2i(xoffset + corner.x + 128, yoffset + corner.y);
            glEnd();
            glBegin(GL_LINE_STRIP);
            glColor4f(1.0f, 1.0f, 1.0f, 0.0f);
            glVertex2i(xoffset + corner.x, yoffset + corner.y + 128);
            glColor4f(1.0f, 1.0f, 1.0f, 0.7f);
            glVertex2i(xoffset + corner.x, yoffset + corner.y);
            glColor4f(1.0f, 1.0f, 1.0f, 0.0f);
            glVertex2i(xoffset + corner.x - 128, yoffset + corner.y);
            glEnd();
        }
    }


    //----------------------------------------- RENDER the Viewcone
    Point2F pointA(cameraRot.x * -40, cameraRot.y * -40);
    Point2F pointB(-pointA.y, pointA.x);

    F32 tann = mTan(0.5f);
    Point2F point1(pointA + pointB * tann);
    Point2F point2(pointA - pointB * tann);

    center.set((F32)(offset.x + mBounds.extent.x / 2), (F32)(offset.y + mBounds.extent.y / 2));
    glBegin(GL_LINE_STRIP);
    glColor4f(1.0f, 0.0f, 0.0f, 0.7f);
    glVertex2i((S32)(center.x + point1.x), (S32)(center.y + point1.y));
    glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
    glVertex2i((S32)center.x, (S32)center.y);
    glColor4f(1.0f, 0.0f, 0.0f, 0.7f);
    glVertex2i((S32)(center.x + point2.x), (S32)(center.y + point2.y));
    glEnd();

    glDisable(GL_BLEND);

    /* debuging stuff
    Point2I loc(offset.x +5, offset.y+10);
     dglSetBitmapModulation(mProfile->mFontColor);
     dglDrawText(mProfile->mFont, loc, avar("mCamera(%3.2f, %3.2f)", mCamera.x, mCamera.y)); loc.y += 10;
     dglDrawText(mProfile->mFont, loc, avar("mRoot(%3.2f, %3.2f)", mRoot.x, mRoot.y)); loc.y += 10;
     dglDrawText(mProfile->mFont, loc, avar("mOrigin(%3.2f, %3.2f)", mOrigin.x, mOrigin.y)); loc.y += 10;
    */
#endif

    renderChildControls(offset, updateRect);
}

