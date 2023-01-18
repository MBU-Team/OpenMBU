//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "platform/event.h"
#include "core/dnet.h"
#include "editor/editTSCtrl.h"
#include "sceneGraph/sceneGraph.h"
#include "editor/editor.h"
#include "game/gameConnection.h"
#include "game/gameBase.h"
#include "game/missionArea.h"
#include "console/consoleTypes.h"
#include "terrain/terrData.h"
#include "game/game.h"
#include "game/sphere.h"
#include "gui/core/guiCanvas.h"

IMPLEMENT_CONOBJECT(EditTSCtrl);

//------------------------------------------------------------------------------

Point3F  EditTSCtrl::smCamPos;
EulerF   EditTSCtrl::smCamRot;
MatrixF  EditTSCtrl::smCamMatrix;
F32      EditTSCtrl::smVisibleDistance = 800.f;

EditTSCtrl::EditTSCtrl() :
    mEditManager(0)
{
    mRenderMissionArea = true;
    mMissionAreaFillColor.set(255, 0, 0, 20);
    mMissionAreaFrameColor.set(255, 0, 0, 128);

    mConsoleFrameColor.set(255, 0, 0, 255);
    mConsoleFillColor.set(255, 0, 0, 120);
    mConsoleSphereLevel = 1;
    mConsoleCircleSegments = 32;
    mConsoleLineWidth = 1;
    mRightMousePassThru = true;

    mConsoleRendering = false;
}

EditTSCtrl::~EditTSCtrl()
{
}

//------------------------------------------------------------------------------

bool EditTSCtrl::onAdd()
{
    if (!Parent::onAdd())
        return(false);

    // give all derived access to the fields
    setModStaticFields(true);
    return true;
}

void EditTSCtrl::onRender(Point2I offset, const RectI& updateRect)
{
    updateGuiInfo();
    Parent::onRender(offset, updateRect);

}

//------------------------------------------------------------------------------

void EditTSCtrl::initPersistFields()
{
    Parent::initPersistFields();
    addGroup("Mission Area");
    addField("renderMissionArea", TypeBool, Offset(mRenderMissionArea, EditTSCtrl));
    addField("missionAreaFillColor", TypeColorI, Offset(mMissionAreaFillColor, EditTSCtrl));
    addField("missionAreaFrameColor", TypeColorI, Offset(mMissionAreaFrameColor, EditTSCtrl));
    endGroup("Mission Area");

    addGroup("Misc");
    addField("consoleFrameColor", TypeColorI, Offset(mConsoleFrameColor, EditTSCtrl));
    addField("consoleFillColor", TypeColorI, Offset(mConsoleFillColor, EditTSCtrl));
    addField("consoleSphereLevel", TypeS32, Offset(mConsoleSphereLevel, EditTSCtrl));
    addField("consoleCircleSegments", TypeS32, Offset(mConsoleCircleSegments, EditTSCtrl));
    addField("consoleLineWidth", TypeS32, Offset(mConsoleLineWidth, EditTSCtrl));
    endGroup("Misc");
}

void EditTSCtrl::consoleInit()
{
    Con::addVariable("pref::Editor::visibleDistance", TypeF32, &EditTSCtrl::smVisibleDistance);
}

//------------------------------------------------------------------------------

void EditTSCtrl::make3DMouseEvent(Gui3DMouseEvent& gui3DMouseEvent, const GuiEvent& event)
{
    (GuiEvent&)(gui3DMouseEvent) = event;

    // get the eye pos and the mouse vec from that...
    Point3F sp(event.mousePoint.x, event.mousePoint.y, 1);

    Point3F wp;
    unproject(sp, &wp);

    gui3DMouseEvent.pos = smCamPos;
    gui3DMouseEvent.vec = wp - smCamPos;
    gui3DMouseEvent.vec.normalize();
}

//------------------------------------------------------------------------------

void EditTSCtrl::getCursor(GuiCursor*& cursor, bool& visible, const GuiEvent& event)
{
    make3DMouseEvent(mLastEvent, event);
    get3DCursor(cursor, visible, mLastEvent);
}

void EditTSCtrl::get3DCursor(GuiCursor*& cursor, bool& visible, const Gui3DMouseEvent& event)
{
    event;
    cursor = NULL;
    visible = true;
}

//------------------------------------------------------------------------------

void EditTSCtrl::onMouseUp(const GuiEvent& event)
{
    make3DMouseEvent(mLastEvent, event);
    on3DMouseUp(mLastEvent);
}

void EditTSCtrl::onMouseDown(const GuiEvent& event)
{
    make3DMouseEvent(mLastEvent, event);
    on3DMouseDown(mLastEvent);
}

void EditTSCtrl::onMouseMove(const GuiEvent& event)
{
    make3DMouseEvent(mLastEvent, event);
    on3DMouseMove(mLastEvent);
}

void EditTSCtrl::onMouseDragged(const GuiEvent& event)
{
    make3DMouseEvent(mLastEvent, event);
    on3DMouseDragged(mLastEvent);

}

void EditTSCtrl::onMouseEnter(const GuiEvent& event)
{
    make3DMouseEvent(mLastEvent, event);
    on3DMouseEnter(mLastEvent);
}

void EditTSCtrl::onMouseLeave(const GuiEvent& event)
{
    make3DMouseEvent(mLastEvent, event);
    on3DMouseLeave(mLastEvent);
}

void EditTSCtrl::onRightMouseDown(const GuiEvent& event)
{
    // always process the right mouse event first...

    make3DMouseEvent(mLastEvent, event);
    on3DRightMouseDown(mLastEvent);

    if (mRightMousePassThru && mProfile->mCanKeyFocus)
    {
        // ok, gotta disable the mouse
        // script functions are lockMouse(true); Canvas.cursorOff();
        Platform::setWindowLocked(true);
        Canvas->setCursorON(false);
        setFirstResponder();
    }
}

void EditTSCtrl::onRightMouseUp(const GuiEvent& event)
{
    make3DMouseEvent(mLastEvent, event);
    on3DRightMouseUp(mLastEvent);
}

void EditTSCtrl::onRightMouseDragged(const GuiEvent& event)
{
    make3DMouseEvent(mLastEvent, event);
    on3DRightMouseDragged(mLastEvent);
}

bool EditTSCtrl::onInputEvent(const InputEvent& event)
{
    if (mRightMousePassThru && event.deviceType == MouseDeviceType &&
        event.objInst == KEY_BUTTON1 && event.action == SI_BREAK)
    {
        // if the right mouse pass thru is enabled,
        // we want to reactivate mouse on a right mouse button up
        Platform::setWindowLocked(false);
        Canvas->setCursorON(true);
    }
    // we return false so that the canvas can properly process the right mouse button up...
    return false;
}

//------------------------------------------------------------------------------

void EditTSCtrl::renderWorld(const RectI& updateRect)
{
    GFX->setZEnable(true);
    GFX->setZFunc(GFXCmpLessEqual);
    GFX->setCullMode(GFXCullNone);
    GFX->setBaseRenderState();

    getCurrentClientSceneGraph()->renderScene();

    // render the mission area...
    if (mRenderMissionArea)
        renderMissionArea();

    GFX->setZEnable(false);

    // render through console callbacks
    SimSet* missionGroup = static_cast<SimSet*>(Sim::findObject("MissionGroup"));
    if (missionGroup)
    {
        mConsoleRendering = true;
        GFX->setZEnable(true);

        for (SimSetIterator itr(missionGroup); *itr; ++itr)
        {
            char buf[2][16];
            dSprintf(buf[0], 16, (*itr)->isSelected() ? "true" : "false");
            dSprintf(buf[1], 16, (*itr)->isExpanded() ? "true" : "false");
            Con::executef(*itr, 4, "onEditorRender", getIdString(), buf[0], buf[1]);
        }

        GFX->setZEnable(false);
        mConsoleRendering = false;
    }

    GFX->disableShaders();

    // render the editor stuff
    renderScene(updateRect);


    GFX->setClipRect(updateRect);
    GFX->setZEnable(true);

}

void EditTSCtrl::renderMissionArea()
{
    /*
       MissionArea * obj = dynamic_cast<MissionArea*>(Sim::findObject("MissionArea"));
       TerrainBlock * terrain = dynamic_cast<TerrainBlock*>(Sim::findObject("Terrain"));
       if(!terrain)
          return;

       GridSquare * gs = terrain->findSquare(TerrainBlock::BlockShift, Point2I(0,0));
       F32 height = F32(gs->maxHeight) * 0.03125f + 10.f;

       //
       const RectI &area = obj->getArea();
       Point2F min(area.point.x, area.point.y);
       Point2F max(area.point.x + area.extent.x, area.point.y + area.extent.y);

       glDisable(GL_CULL_FACE);
       glEnable(GL_BLEND);
       glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

       ColorI & a = mMissionAreaFillColor;
       ColorI & b = mMissionAreaFrameColor;
       for(U32 i = 0; i < 2; i++)
       {
          //
          if(i){glColor4ub(a.red,a.green,a.blue,a.alpha);glBegin(GL_QUADS);} else {glColor4f(b.red,b.green,b.blue,b.alpha); glBegin(GL_LINE_LOOP);}
          glVertex3f(min.x, min.y, 0);
          glVertex3f(max.x, min.y, 0);
          glVertex3f(max.x, min.y, height);
          glVertex3f(min.x, min.y, height);
          glEnd();

          //
          if(i){glColor4ub(a.red,a.green,a.blue,a.alpha);glBegin(GL_QUADS);} else {glColor4f(b.red,b.green,b.blue,b.alpha); glBegin(GL_LINE_LOOP);}
          glVertex3f(min.x, max.y, 0);
          glVertex3f(max.x, max.y, 0);
          glVertex3f(max.x, max.y, height);
          glVertex3f(min.x, max.y, height);
          glEnd();

          //
          if(i){glColor4ub(a.red,a.green,a.blue,a.alpha);glBegin(GL_QUADS);} else {glColor4f(b.red,b.green,b.blue,b.alpha); glBegin(GL_LINE_LOOP);}
          glVertex3f(min.x, min.y, 0);
          glVertex3f(min.x, max.y, 0);
          glVertex3f(min.x, max.y, height);
          glVertex3f(min.x, min.y, height);
          glEnd();

          //
          if(i){glColor4ub(a.red,a.green,a.blue,a.alpha);glBegin(GL_QUADS);} else {glColor4f(b.red,b.green,b.blue,b.alpha); glBegin(GL_LINE_LOOP);}
          glVertex3f(max.x, min.y, 0);
          glVertex3f(max.x, max.y, 0);
          glVertex3f(max.x, max.y, height);
          glVertex3f(max.x, min.y, height);
          glEnd();
       }

       glDisable(GL_BLEND);
    */
}

//------------------------------------------------------------------------------

#include "terrain/environment/sky.h"

bool EditTSCtrl::processCameraQuery(CameraQuery* query)
{
    GameConnection* connection = dynamic_cast<GameConnection*>(NetConnection::getConnectionToServer());
    if (connection)
    {
        if (connection->getControlCameraTransform(0.032, &query->cameraMatrix)) {
            query->farPlane = getMax(smVisibleDistance, 50.f);
            query->nearPlane = query->farPlane / 5000.0f;
            query->fov = mDegToRad(90.0);

            smCamMatrix = query->cameraMatrix;
            smCamMatrix.getColumn(3, &smCamPos);
            smCamRot.set(0, 0, 0);
            return(true);
        }
    }
    return(false);

}

//------------------------------------------------------------------------------
// sort the surfaces: not correct when camera is inside sphere but not
// inside tesselated representation of sphere....
struct SortInfo
{
    U32 idx;
    F32 dot;
};

static int QSORT_CALLBACK alphaSort(const void* p1, const void* p2)
{
    const SortInfo* ip1 = (const SortInfo*)p1;
    const SortInfo* ip2 = (const SortInfo*)p2;

    if (ip1->dot > ip2->dot)
        return(1);
    if (ip1->dot == ip2->dot)
        return(0);
    return(-1);
}

//------------------------------------------------------------------------------
ConsoleMethod(EditTSCtrl, renderSphere, void, 4, 5, "(Point3F pos, float radius, int subdivisions=NULL)")
{
    /*
       if(!object->mConsoleRendering)
          return;

       static Sphere sphere(Sphere::Icosahedron);

       if(!object->mConsoleFrameColor.alpha && !object->mConsoleFillColor.alpha)
          return;

       S32 sphereLevel = object->mConsoleSphereLevel;
       if(argc == 5)
          sphereLevel = dAtoi(argv[4]);

       const Sphere::TriangleMesh * mesh = sphere.getMesh(sphereLevel);

       Point3F pos;
       dSscanf(argv[2], "%f %f %f", &pos.x, &pos.y, &pos.z);

       F32 radius = dAtoi(argv[3]);

       glEnable(GL_BLEND);
       glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

       // sort the surfaces back->front
       Vector<SortInfo> sortInfos;

       Point3F camNormal = object->smCamPos - pos;
       camNormal.normalize();

       sortInfos.setSize(mesh->numPoly);
       for(U32 i = 0; i < mesh->numPoly; i++)
       {
          sortInfos[i].idx = i;
          sortInfos[i].dot = mDot(camNormal, mesh->poly[i].normal);
       }
       dQsort(sortInfos.address(), sortInfos.size(), sizeof(SortInfo), alphaSort);

       // frame
       if(object->mConsoleFrameColor.alpha)
       {
          glColor4ub(object->mConsoleFrameColor.red,
                     object->mConsoleFrameColor.green,
                     object->mConsoleFrameColor.blue,
                     object->mConsoleFrameColor.alpha);

          glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

          glLineWidth(object->mConsoleLineWidth);
          glBegin(GL_TRIANGLES);
          for(U32 i = 0; i < mesh->numPoly; i++)
          {
             Sphere::Triangle & tri = mesh->poly[sortInfos[i].idx];
             for(S32 j = 2; j >= 0; j--)
                glVertex3f(tri.pnt[j].x * radius + pos.x,
                           tri.pnt[j].y * radius + pos.y,
                           tri.pnt[j].z * radius + pos.z);
          }
          glEnd();

          glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
          glLineWidth(1);
       }

       // fill
       if(object->mConsoleFillColor.alpha)
       {
          glColor4ub(object->mConsoleFillColor.red,
                     object->mConsoleFillColor.green,
                     object->mConsoleFillColor.blue,
                     object->mConsoleFillColor.alpha);

          glBegin(GL_TRIANGLES);
          for(U32 i = 0; i < mesh->numPoly; i++)
          {
             Sphere::Triangle & tri = mesh->poly[sortInfos[i].idx];
             for(S32 j = 2; j >= 0; j--)
                glVertex3f(tri.pnt[j].x * radius + pos.x,
                           tri.pnt[j].y * radius + pos.y,
                           tri.pnt[j].z * radius + pos.z);
          }
          glEnd();
       }
       glDisable(GL_BLEND);
    */
}

ConsoleMethod(EditTSCtrl, renderCircle, void, 5, 6, "(Point3F pos, Point3F normal, float radius, int segments=NULL)")
{
    /*
       if(!object->mConsoleRendering)
          return;

       if(!object->mConsoleFrameColor.alpha && !object->mConsoleFillColor.alpha)
          return;

       Point3F pos, normal;
       dSscanf(argv[2], "%g %g %g", &pos.x, &pos.y, &pos.z);
       dSscanf(argv[3], "%g %g %g", &normal.x, &normal.y, &normal.z);

       F32 radius = dAtoi(argv[4]);

       S32 segments = object->mConsoleCircleSegments;
       if(argc == 6)
          segments = dAtoi(argv[5]);

       normal.normalize();

       AngAxisF aa;
       mCross(normal, Point3F(0,0,1), &aa.axis);
       aa.axis.normalizeSafe();
       aa.angle = mAcos(mClampF(mDot(normal, Point3F(0,0,1)), -1.f, 1.f));

       if(aa.angle == 0.f)
          aa.axis.set(0,0,1);

       MatrixF mat;
       aa.setMatrix(&mat);

       F32 step = M_2PI / segments;
       F32 angle = 0.f;

       glEnable(GL_BLEND);
       glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

       Vector<Point3F> points;
       segments--;
       for(U32 i = 0; i < segments; i++)
       {
          Point3F pnt(mCos(angle), mSin(angle), 0.f);

          mat.mulP(pnt);
          pnt *= radius;
          pnt += pos;

          points.push_back(pnt);
          angle += step;
       }

       // framed
       if(object->mConsoleFrameColor.alpha)
       {
          glColor4ub(object->mConsoleFrameColor.red,
                     object->mConsoleFrameColor.green,
                     object->mConsoleFrameColor.blue,
                     object->mConsoleFrameColor.alpha);
          glLineWidth(object->mConsoleLineWidth);
          glBegin(GL_LINE_LOOP);
          for(U32 i = 0; i < points.size(); i++)
             glVertex3f(points[i].x, points[i].y, points[i].z);
          glEnd();
          glLineWidth(1);
       }

       // filled
       if(object->mConsoleFillColor.alpha)
       {
          glColor4ub(object->mConsoleFillColor.red,
                     object->mConsoleFillColor.green,
                     object->mConsoleFillColor.blue,
                     object->mConsoleFillColor.alpha);

          glBegin(GL_TRIANGLES);

          for(S32 i = 0; i < points.size(); i++)
          {
             S32 j = (i + 1) % points.size();
             glVertex3f(points[i].x, points[i].y, points[i].z);
             glVertex3f(points[j].x, points[j].y, points[j].z);
             glVertex3f(pos.x, pos.y, pos.z);
          }

          glEnd();
       }

       glDisable(GL_BLEND);
    */
}

ConsoleMethod(EditTSCtrl, renderTriangle, void, 5, 5, "(Point3F a, Point3F b, Point3F c)")
{
    /*
       if(!object->mConsoleRendering)
          return;

       if(!object->mConsoleFrameColor.alpha && !object->mConsoleFillColor.alpha)
          return;

       Point3F pnts[3];
       for(U32 i = 0; i < 3; i++)
          dSscanf(argv[i+2], "%f %f %f", &pnts[i].x, &pnts[i].y, &pnts[i].z);

       glEnable(GL_BLEND);
       glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

       // frame
       if(object->mConsoleFrameColor.alpha)
       {
          glColor4ub(object->mConsoleFrameColor.red,
                     object->mConsoleFrameColor.green,
                     object->mConsoleFrameColor.blue,
                     object->mConsoleFrameColor.alpha);
          glLineWidth(object->mConsoleLineWidth);
          glBegin(GL_LINE_LOOP);
          for(U32 i = 0; i < 3; i++)
             glVertex3f(pnts[i].x, pnts[i].y, pnts[i].z);
          glEnd();
          glLineWidth(1);
       }

       // fill
       if(object->mConsoleFillColor.alpha)
       {
          glColor4ub(object->mConsoleFillColor.red,
                     object->mConsoleFillColor.green,
                     object->mConsoleFillColor.blue,
                     object->mConsoleFillColor.alpha);

          glBegin(GL_TRIANGLES);
          for(U32 i = 0; i < 3; i++)
             glVertex3f(pnts[i].x, pnts[i].y, pnts[i].z);
          glEnd();
       }
       glDisable(GL_BLEND);
    */
}

ConsoleMethod(EditTSCtrl, renderLine, void, 4, 5, "(Point3F start, Point3F end, int width)")
{
    /*
       if(!object->mConsoleRendering)
          return;

       if(!object->mConsoleFrameColor.alpha)
          return;

       Point3F start, end;
       dSscanf(argv[2], "%f %f %f", &start.x, &start.y, &start.z);
       dSscanf(argv[3], "%f %f %f", &end.x, &end.y, &end.z);

       S32 width = object->mConsoleLineWidth;
       if(argc == 5)
          width = dAtoi(argv[4]);

       glEnable(GL_BLEND);
       glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

       glColor4ub(object->mConsoleFrameColor.red,
                  object->mConsoleFrameColor.green,
                  object->mConsoleFrameColor.blue,
                  object->mConsoleFrameColor.alpha);

       glLineWidth(width);

       glBegin(GL_LINES);
       glVertex3f(start.x, start.y, start.z);
       glVertex3f(end.x, end.y, end.z);
       glEnd();

       glLineWidth(1);
       glDisable(GL_BLEND);
    */
}
