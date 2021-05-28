//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "debugDraw.h"

DebugDrawer* gDebugDraw;

IMPLEMENT_CONOBJECT(DebugDrawer);

DebugDrawer::DebugDrawer()
{
    mHead = NULL;
    isFrozen = false;

#ifdef ENABLE_DEBUGDRAW
    isDrawing = true;
#else
    isDrawing = false;
#endif
}

DebugDrawer::~DebugDrawer()
{
}

void DebugDrawer::consoleInit()
{
}

void DebugDrawer::initPersistFields()
{
}

void DebugDrawer::init()
{
#ifdef ENABLE_DEBUGDRAW
    gDebugDraw = new DebugDrawer();
    gDebugDraw->registerObject("DebugDraw");

#ifndef TORQUE_DEBUG
    Con::errorf("===============================================================");
    Con::errorf("=====  WARNING! DEBUG DRAWER ENABLED!                       ===");
    Con::errorf("=====       Turn me off in final build, thanks.             ===");
    Con::errorf("=====        I will draw a gross line to get your attention.===");
    Con::errorf("=====                                      -- BJG           ===");
    Con::errorf("===============================================================");

    // You can disable this code if you know what you're doing. Just be sure not to ship with
    // DebugDraw enabled! 

    // DebugDraw can be used for all sorts of __cheats__ and __bad things__.
    gDebugDraw->drawLine(Point3F(-10000, -10000, -10000), Point3F(10000, 10000, 10000), ColorF(1, 0, 0));
    gDebugDraw->setLastTTL(15 * 60 * 1000);
#else
    gDebugDraw->drawLine(Point3F(-10000, -10000, -10000), Point3F(10000, 10000, 10000), ColorF(0, 1, 0));
    gDebugDraw->setLastTTL(30 * 1000);
#endif
#endif

}

void DebugDrawer::render()
{
#ifdef ENABLE_DEBUGDRAW
    if (!isDrawing) return;

    SimTime curTime = Sim::getCurrentTime();

    GFX->setBaseRenderState();
    GFX->disableShaders();

    GFX->setCullMode(GFXCullNone);
    GFX->setAlphaBlendEnable(false);
    GFX->setTextureStageColorOp(0, GFXTOPDisable);
    GFX->setZEnable(true);

    for (DebugPrim** walk = &mHead; *walk; )
    {
        DebugPrim* p = *walk;

        // Set up Z testing...
        if (p->useZ)
            GFX->setZEnable(false);
        else
            GFX->setZEnable(false);

        Point3F d;

        switch (p->type)
        {
        case DebugPrim::Tri:
            PrimBuild::begin(GFXLineStrip, 4);

            PrimBuild::color(p->color);

            PrimBuild::vertex3fv(p->a);
            PrimBuild::vertex3fv(p->b);
            PrimBuild::vertex3fv(p->c);
            PrimBuild::vertex3fv(p->a);

            PrimBuild::end();
            break;
        case DebugPrim::Sphere:
            AssertISV(false, "BJGTODO - Implement me.");
            break;
        case DebugPrim::Box:
            d = p->a - p->b;
            GFX->drawWireCube(d * 0.5, (p->a + p->b) * 0.5, p->color);
            break;
        case DebugPrim::Line:
            PrimBuild::begin(GFXLineStrip, 2);

            PrimBuild::color(p->color);

            PrimBuild::vertex3fv(p->a);
            PrimBuild::vertex3fv(p->b);

            PrimBuild::end();
            break;
        }

        if (p->dieTime <= curTime && !isFrozen)
        {
            *walk = p->next;
            mPrimChunker.free(p);
        }
        else
            walk = &((*walk)->next);
    }

#endif
}

void DebugDrawer::drawBox(Point3F a, Point3F b, ColorF color)
{
    if (isFrozen || !isDrawing) return;

    DebugPrim* n = mPrimChunker.alloc();

    n->useZ = true;
    n->dieTime = 0;
    n->a = a;
    n->b = b;
    n->color = color;
    n->type = DebugPrim::Box;

    n->next = mHead;
    mHead = n;
}

void DebugDrawer::drawLine(Point3F a, Point3F b, ColorF color)
{
    if (isFrozen || !isDrawing) return;

    DebugPrim* n = mPrimChunker.alloc();

    n->useZ = true;
    n->dieTime = 0;
    n->a = a;
    n->b = b;
    n->color = color;
    n->type = DebugPrim::Line;

    n->next = mHead;
    mHead = n;
}

void DebugDrawer::drawSphere(Point3F center, F32 radius, ColorF color)
{
    if (isFrozen || !isDrawing) return;
    AssertISV(false, "BJGTODO - Implement me.");
}

void DebugDrawer::drawTri(Point3F a, Point3F b, Point3F c, ColorF color)
{
    if (isFrozen || !isDrawing) return;

    DebugPrim* n = mPrimChunker.alloc();

    n->useZ = true;
    n->dieTime = 0;
    n->a = a;
    n->b = b;
    n->b = c;
    n->color = color;
    n->type = DebugPrim::Tri;

    n->next = mHead;
    mHead = n;
}


void DebugDrawer::setLastTTL(U32 ms)
{
    AssertFatal(mHead, "Tried to set last with nothing in the list!");
    mHead->dieTime = Sim::getCurrentTime() + ms;
}

void DebugDrawer::setLastZTest(bool enabled)
{
    AssertFatal(mHead, "Tried to set last with nothing in the list!");
    mHead->useZ = enabled;
}

ConsoleMethod(DebugDrawer, drawLine, void, 4, 4, "(Point3F a, Point3F b)")
{
    Point3F a, b;

    dSscanf(argv[2], "%f %f %f", &a.x, &a.y, &a.z);
    dSscanf(argv[3], "%f %f %f", &b.x, &b.y, &b.z);

    object->drawLine(a, b);
}

ConsoleMethod(DebugDrawer, setLastTTL, void, 3, 3, "(U32 ms)")
{
    object->setLastTTL(dAtoi(argv[2]));
}

ConsoleMethod(DebugDrawer, setLastZTest, void, 3, 3, "(bool enabled)")
{
    object->setLastZTest(dAtob(argv[2]));
}

ConsoleMethod(DebugDrawer, toggleFreeze, void, 2, 2, "() - Toggle freeze mode.")
{
    object->toggleFreeze();
}

ConsoleMethod(DebugDrawer, toggleDrawing, void, 2, 2, "() - Enabled/disable drawing.")
{
    object->toggleDrawing();
}
