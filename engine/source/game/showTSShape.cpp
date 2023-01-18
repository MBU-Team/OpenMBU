//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "game/showTSShape.h"
#include "gui/core/guiControl.h"
#include "gui/core/guiTSControl.h"
#include "gui/controls/guiTextCtrl.h"
#include "gui/controls/guiTextListCtrl.h"
#include "gui/controls/guiSliderCtrl.h"
#include "gui/controls/guiTextEditCtrl.h"
#include "gui/core/guiCanvas.h"
#include "sceneGraph/sceneGraph.h"
#include "sceneGraph/sceneState.h"
#include "console/consoleTypes.h"

#ifdef TORQUE_TERRAIN
#include "terrain/terrData.h"
#include "terrain/terrRender.h"
#endif

ShowTSShape* ShowTSShape::currentShow = NULL;

Vector<char*> ShowTSShape::loadQueue(__FILE__, __LINE__);

const char* emapName[] = { "emap.bmp" };

//MaterialList emapList(1,emapName);
F32 emapAlpha = 0.0f;
F32 ambR = 0.2f;
F32 ambB = 0.2f;
F32 ambG = 0.2f;
F32 diffR = 0.9f;
F32 diffB = 0.9f;
F32 diffG = 0.9f;
Point4F lightDir(0.57f, 0.57f, -0.57f, 0.0f);
bool gInitLightingSliders = false;

//------------------------------------------------------------
// simple camera methods and variables
//------------------------------------------------------------

Point3F camPos;
MatrixF cameraMatrix;
EulerF camRot;

F32 gShowForwardAction = 0.0f;
F32 gShowBackwardAction = 0.0f;
F32 gShowUpAction = 0.0f;
F32 gShowDownAction = 0.0f;
F32 gShowLeftAction = 0.0f;
F32 gShowRightAction = 0.0f;
F32 gShowMovementSpeed = 1.0f;

float initialShowDistance = 20.0f;
Point3F* orbitPos = NULL;
float orbitDist = 10.0f;
float minOrbitDist = 1.0f;
float maxOrbitASin = 0.2f;
bool keyboardControlsShape = false;

float gShowShapeLeftSpeed = 0.0f;
float gShowShapeRightSpeed = 0.0f;

void setOrbit(Point3F* orb, bool useCurrentDist)
{
    orbitPos = orb;

    if (!useCurrentDist)
    {
        Point3F vec;
        cameraMatrix.getColumn(1, &vec);
        orbitDist = mDot(vec, *orbitPos - camPos);
    }
    if (orbitDist < minOrbitDist)
        orbitDist = minOrbitDist;
}

void setOrbit(Point3F* orb, float newOrbitDist)
{
    orbitPos = orb;

    orbitDist = newOrbitDist;
    if (orbitDist < minOrbitDist)
        orbitDist = minOrbitDist;
}

void setDetailSlider()
{
    ShowTSShape* currentShow = ShowTSShape::currentShow;
    if (!currentShow)
        return;

    GuiSliderCtrl* slider = static_cast<GuiSliderCtrl*>(Sim::findObject("showDetailSlider"));
    if (!slider)
        return;

    // find current detail level and number of detail levels of currentShow
    S32 dl = currentShow->getCurrentDetail();
    S32 numDL = currentShow->getNumDetails();

    // set slider range to be correct
    slider->setField("range", avar("0 %g", -0.01f + (F32)numDL));
    // set slider ticks to be correct
    slider->setField("ticks", avar("%i", numDL));
    // set slider value to be correct
    slider->setField("value", avar("%i", dl));
}

//------------------------------------------------------------
// show gui related methods
//------------------------------------------------------------

void showUpdateThreadControl()
{
    char buffer[32];

    // update dialogs
    GuiTextListCtrl* threadList = dynamic_cast<GuiTextListCtrl*>(Sim::findObject("threadList"));
    GuiTextListCtrl* sequenceList = dynamic_cast<GuiTextListCtrl*>(Sim::findObject("sequenceList"));
    if (!threadList || !sequenceList)
    {
        // just for debugging, look up w/o dynamic cast...
        threadList = threadList ? NULL : static_cast<GuiTextListCtrl*>(Sim::findObject("threadList"));
        sequenceList = sequenceList ? NULL : static_cast<GuiTextListCtrl*>(Sim::findObject("sequenceList"));
        AssertFatal(!threadList && !sequenceList, "showUpdateThreadControl: threadList or sequenceList of wrong gui type");
        // something wrong, just exit
        return;
    }

    ShowTSShape* currentShow = ShowTSShape::currentShow;
    if (!currentShow)
    {
        // no current show...zero out lists
        threadList->clear();
        sequenceList->clear();
        return;
    }

    // the thread list:

    threadList->setCellSize(Point2I(threadList->mBounds.extent.x, 16));
    U32 count = currentShow->getThreadCount();

    if (count != threadList->getNumEntries())
    {
        threadList->clear();
        for (U32 i = 0; i < count; i++)
        {
            dSprintf(buffer, sizeof(buffer), "%i", i);
            threadList->addEntry(i, buffer);
        }
    }
    S32 selectedThread = threadList->getSelectedCell().y;
    if (threadList->getNumEntries() == 0 && selectedThread >= 0)
        // no threads, select nothing...
        threadList->deselectCells();
    else if (threadList->getNumEntries() > 0 && selectedThread < 0)
        // we have threads but nothing selected...
        threadList->setSelectedCell(Point2I(0, 0));
    selectedThread = threadList->getSelectedCell().y;

    // the sequence list:

    sequenceList->setCellSize(Point2I(sequenceList->mBounds.extent.x, 16));
    // only change the text if it needs changing ... this avoids infinite loops since
    // changing text can result in this routine getting called again
    S32 i;
    for (i = 0; i < currentShow->getSequenceCount() && i < sequenceList->getNumEntries(); i++)
        if (dStrcmp(currentShow->getSequenceName(i), sequenceList->mList[i].text))
            break;
    while (i < sequenceList->getNumEntries())
        sequenceList->removeEntryByIndex(i);
    for (; i < currentShow->getSequenceCount(); i++)
        sequenceList->addEntry(i, currentShow->getSequenceName(i));
    if (selectedThread == -1)
    {
        if (sequenceList->getSelectedCell().y >= 0)
            sequenceList->deselectCells();
    }
    else if (sequenceList->getSelectedCell().y != currentShow->getSequenceNum(selectedThread))
        sequenceList->setSelectedCell(Point2I(0, currentShow->getSequenceNum(selectedThread)));

    // update displayed scale value
    GuiTextCtrl* text = dynamic_cast<GuiTextCtrl*>(Sim::findObject("showScaleValue"));
    if (text && selectedThread >= 0)
    {
        dSprintf(buffer, 32, "scale = %2.2f", currentShow->getThreadScale(selectedThread));
        text->setText(buffer);
    }
}


//------------------------------------------------------------
// basic class for displaying a tsshape
//------------------------------------------------------------

ShowTSShape::ShowTSShape(const char* shapeName)
{
    mTypeMask = StaticObjectType;

    shapeInstance = NULL;
    dStrcpy(shapeNameBuffer, shapeName);
    timeScale = 1.0f;
    addGround = false;
    stayGrounded = true;
    char fileBuffer[512];
    dStrcpy(fileBuffer, shapeName);

    // before loading shape, execute associated script
    char* ext = dStrstr(static_cast<char*>(fileBuffer), const_cast<char*>(".dts"));
    if (ext)
    {
        ext[1] = 'c';
        ext[2] = 's';
        ext[3] = '\0';
        Con::executef(2, "exec", fileBuffer);
        ext[1] = 'd';
        ext[2] = 't';
        ext[3] = 's';
    }

    // also exec associated materials (if any)
    char matPath[4096];
    dStrcpy(matPath, shapeName);
    char* lastSlash = dStrrchr(matPath, '/');
    if (lastSlash)
    {
        dStrcpy(lastSlash, "/materials.cs");
        Con::executef(2, "exec", matPath);
    }

    Resource<TSShape> hShape;
    hShape = ResourceManager->load(fileBuffer);
    if (!bool(hShape))
        return;

    shapeInstance = new TSShapeInstance(hShape, true);
    //   emapList.load(MeshTexture,0,true);
    //   shapeInstance->setEnvironmentMap(emapList.getMaterial(0));

    newThread(); // does nothing if shape has no sequences

 //   // TODO: fix this in exporter at some point...
    Point3F& center = const_cast<Point3F&>(shapeInstance->getShape()->center);
    center = shapeInstance->getShape()->bounds.min + shapeInstance->getShape()->bounds.max;
    center *= 0.5f;

    mat.identity();
    setPosition(camPos);
}

//--------------------------------------------------------------------------
bool ShowTSShape::prepRenderImage(SceneState* state, const U32 stateKey,
    const U32 /*startZone*/, const bool /*modifyBaseState*/)
{
    if (isLastState(state, stateKey))
        return false;
    setLastState(state, stateKey);

    prepBatchRender(state);

    return false;
}

void ShowTSShape::prepBatchRender(SceneState* state)
{
    // setup light
    LightInfo* sunlight = getCurrentClientSceneGraph()->getLightManager()->sgGetSpecialLight(LightManager::sgSunLightType);

    sunlight->mType = LightInfo::Vector;
    sunlight->mDirection.set(-lightDir.x, -lightDir.y, -lightDir.z);
    sunlight->mColor.set(diffR, diffG, diffB);
    sunlight->mAmbient.set(ambR, ambG, ambB);


    RectI viewport = GFX->getViewport();
    state->setupObjectProjection(this);

    MatrixF proj = GFX->getProjectionMatrix();
    GFX->pushWorldMatrix();

    MatrixF cameraMatrix = GFX->getWorldMatrix();
    MatrixF objMatrix = getRenderTransform();

    TSMesh::setCamTrans(cameraMatrix);
    TSMesh::setSceneState(state);
    TSMesh::setObject(this);

    MatrixF mat = objMatrix;
    mat.scale(mObjScale);
    GFX->setWorldMatrix(mat);

    // select detail level
    if (Con::getBoolVariable("$showAutoDetail", false))
    {
        shapeInstance->selectCurrentDetail();
        GuiSliderCtrl* slider = static_cast<GuiSliderCtrl*>(Sim::findObject("showDetailSlider"));
        // set slider value to be correct
        if (slider && slider->mAwake)
        {
            char buffer[32];
            dSprintf(buffer, 32, "%g", (F32)shapeInstance->getCurrentDetail() + 1.0f - shapeInstance->getCurrentIntraDetail());
            slider->setScriptValue(buffer);
        }
    }
    else
    {
        GuiSliderCtrl* slider = static_cast<GuiSliderCtrl*>(Sim::findObject("showDetailSlider"));
        // set current detail level based on slider
        if (slider)
            shapeInstance->setCurrentDetail((S32)slider->getValue(), 1.0f - (F32)((F32)slider->getValue() - (S32)slider->getValue()));
    }

    shapeInstance->animate();
    shapeInstance->render();

    GFX->popWorldMatrix();
    GFX->setProjectionMatrix(proj);
    GFX->setViewport(viewport);

}



//--------------------------------------
bool ShowTSShape::onAdd()
{
    mWorldToObj = mObjToWorld;
    mWorldToObj.affineInverse();
    resetWorldBox();

    if (!Parent::onAdd() || !shapeInstance)
        // shape instantiation failed
        return false;

    // make us the center of attention
    orbitUs();

    // We're always a client object...
    getCurrentClientContainer()->addObject(this);
    getCurrentClientSceneGraph()->addObjectToScene(this);

    mObjBox = shapeInstance->getShape()->bounds;
    resetWorldBox();

    SimSet* showSet = static_cast<SimSet*>(Sim::findObject("showSet"));
    if (!showSet)
    {
        showSet = new SimSet;
        showSet->registerObject("showSet");
        Sim::getRootGroup()->addObject(showSet);
    }
    showSet->addObject(this);

    currentShow = this;

    return true;
}

void ShowTSShape::onRemove()
{
    removeFromScene();

    if (this == currentShow)
    {
        currentShow = NULL;
        showUpdateThreadControl();
    }
    Parent::onRemove();
}

ShowTSShape::~ShowTSShape()
{
    delete shapeInstance;
    if (orbitPos == &centerPos)
        orbitPos = NULL;
}

void ShowTSShape::newThread()
{
    if (shapeInstance->getShape()->sequences.empty())
        return;

    threads.increment();
    threads.last() = shapeInstance->addThread();
    play.push_back(true);
    scale.push_back(1.0f);
}

void ShowTSShape::deleteThread(S32 th)
{
    if (th < 0 || th >= threads.size())
        return;

    shapeInstance->destroyThread(threads[th]);
    threads.erase(th);
    play.erase(th);
    scale.erase(th);
}

void ShowTSShape::setPosition(Point3F p)
{
#ifdef TORQUE_TERRAIN
    if (stayGrounded && getCurrentClientSceneGraph()->getCurrentTerrain())
        // won't actually change p.z if on empty square
        getCurrentClientSceneGraph()->getCurrentTerrain()->getHeight(Point2F(p.x, p.y), &p.z);
#endif

    centerPos = p + shapeInstance->getShape()->center;
    mat.setColumn(3, p);
    setTransform(mat);
}

void ShowTSShape::addGroundTransform(const MatrixF& ground)
{
    // add some ground transform to our current transform
    MatrixF m;
    m.mul(mat, ground);
    mat = m;
    Point3F p;
    mat.getColumn(3, &p);
    setPosition(p);
}

void ShowTSShape::orbitUs()
{
    // make us the center of attention
    minOrbitDist = shapeInstance->getShape()->radius;
    setOrbit(&centerPos, false);
}

bool ShowTSShape::handleMovement(F32 leftSpeed, F32 rightSpeed,
    F32 forwardSpeed, F32 backwardSpeed,
    F32 upSpeed, F32 downSpeed,
    F32 delta)
{
    if (!currentShow)
        return false;

    Point3F vec, p;

    // turn shape left/right
    F32 turnLR = (gShowShapeRightSpeed - gShowShapeLeftSpeed) * delta * 0.1f;
    MatrixF turn(EulerF(0, 0, turnLR));
    currentShow->addGroundTransform(turn);

    if (keyboardControlsShape && currentShow)
    {
        currentShow->getFeetPosition(&p);

        // the following moves shape with camera orthoganol to world
        // depends on camera not having roll

        cameraMatrix.getColumn(0, &vec);
        vec.z = 0.0f;
        vec.normalize();
        p += vec * (rightSpeed - leftSpeed) * delta;
        if (!orbitPos)
            // move camera with shape
            camPos += vec * (rightSpeed - leftSpeed) * delta;

        cameraMatrix.getColumn(1, &vec);
        vec.z = 0.0f;
        vec.normalize();
        p += vec * (forwardSpeed - backwardSpeed) * delta;
        if (!orbitPos)
            // move camera with shape
            camPos += vec * (forwardSpeed - backwardSpeed) * delta;

        vec.set(0, 0, 1.0f);
        p += vec * (upSpeed - downSpeed) * delta;
        if (!orbitPos)
            // move camera with shape
            camPos += vec * (upSpeed - downSpeed) * delta;

        currentShow->setPosition(p);

        // if orbit cam is on, zero out speeds and adjust like normal
        // if not, let normal camera move us with these speeds...
        if (orbitPos)
            leftSpeed = rightSpeed = forwardSpeed = backwardSpeed = upSpeed = downSpeed = 0.0f;
    }

    if (!orbitPos)
        return false;

    // forward/backward = closer/farther

    orbitDist -= (forwardSpeed - backwardSpeed) * delta;
    if (orbitDist < minOrbitDist)
        orbitDist = minOrbitDist;
    F32 invD = orbitDist > 0.001f ? 1.0f / orbitDist : 0.0f;

    // up/down & left/right = rotate

    F32 xr = (upSpeed - downSpeed) * invD * delta;    // convert linear speed to angular
    F32 zr = (leftSpeed - rightSpeed) * invD * delta; // convert linear speed to angular

    // cap rotation rate (for when you're real close to orbit point)
    if (xr > maxOrbitASin)
        xr = maxOrbitASin;
    else if (xr < -maxOrbitASin)
        xr = -maxOrbitASin;
    if (zr > maxOrbitASin)
        zr = maxOrbitASin;
    else if (zr < -maxOrbitASin)
        zr = -maxOrbitASin;

    camRot.x += mAsin(xr);
    camRot.z += mAsin(zr);

    MatrixF xRot, zRot, newCameraRot;
    xRot.set(EulerF(camRot.x, 0, 0));
    zRot.set(EulerF(0, 0, camRot.z));
    newCameraRot.mul(zRot, xRot);

    // adjust cameraPos so we still face orbitPos (and are still d units away)
    newCameraRot.getColumn(1, &vec);
    vec *= orbitDist;
    camPos = *orbitPos - vec;

    return true;
}

void ShowTSShape::render()
{
    /*
       bool wasLit  = glIsEnabled(GL_LIGHTING);
       bool wasLit0 = glIsEnabled(GL_LIGHT0);

       glEnable(GL_LIGHTING);
       glEnable(GL_LIGHT0);

       glLightfv(GL_LIGHT0,GL_POSITION,(float*)&lightDir);

       GLfloat amb[]  = {ambR,ambG,ambB,0.0f};
       GLfloat diff[] = {diffR,diffG,diffB,1.0f};
       GLfloat matProp[] = {1.0f,1.0f,1.0f,1.0f};
       GLfloat zeroColor[] = {0,0,0,0};

       glLightModelfv(GL_LIGHT_MODEL_AMBIENT,amb);
       glLightfv(GL_LIGHT0,GL_DIFFUSE,diff);
       glLightfv(GL_LIGHT0,GL_AMBIENT,zeroColor);
       glLightfv(GL_LIGHT0,GL_SPECULAR,zeroColor);

       glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE,matProp);
       glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,zeroColor);
       glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,zeroColor);

    //   glPushMatrix();
    //   dglMultMatrix(&mat);

       shapeInstance->setEnvironmentMapOn(true,emapAlpha);

       if (Con::getBoolVariable("$showAutoDetail",false))
       {
          shapeInstance->selectCurrentDetail();
          GuiSliderCtrl * slider = static_cast<GuiSliderCtrl*>(Sim::findObject("showDetailSlider"));
          // set slider value to be correct
          if (slider && slider->mAwake)
          {
             char buffer[32];
             dSprintf(buffer,32,"%g",(F32)shapeInstance->getCurrentDetail()+1.0f-shapeInstance->getCurrentIntraDetail());
             slider->setScriptValue(buffer);
          }
       }
       else
       {
          GuiSliderCtrl * slider = static_cast<GuiSliderCtrl*>(Sim::findObject("showDetailSlider"));
          // set current detail level based on slider
          if (slider)
             shapeInstance->setCurrentDetail((S32)slider->getValue(),1.0f-(F32)((F32)slider->getValue()-(S32)slider->getValue()));
       }
       GuiTextCtrl * detailText1 = static_cast<GuiTextCtrl*>(Sim::findObject("showDetailInfoText1"));
       GuiTextCtrl * detailText2 = static_cast<GuiTextCtrl*>(Sim::findObject("showDetailInfoText2"));
       if (detailText1 && detailText2)
       {
          char buffer[128];
          S32 dl = shapeInstance->getCurrentDetail();
          S32 dlSize = (S32) (dl>=0 ? shapeInstance->getShape()->details[dl].size : 0);
          S32 polys = dl>=0 ? shapeInstance->getShape()->details[dl].polyCount : 0;
          Point3F p;
          mObjToWorld.getColumn(3,&p);
          p -= camPos;
          F32 dist = p.len();
          F32 pixelSize = dglProjectRadius(dist,shapeInstance->getShape()->radius) * dglGetPixelScale() * TSShapeInstance::smDetailAdjust;
          dSprintf(buffer,128,"detail level: %i, detail size: %i",dl,dlSize);
          detailText1->setText(buffer);
          dSprintf(buffer,128,"size: %g, polys: %i, dist: %g",pixelSize,polys,dist);
          detailText2->setText(buffer);
       }

       shapeInstance->animate();

       shapeInstance->render();

    //   glPopMatrix();

       if (!wasLit)
          glDisable(GL_LIGHTING);
       if (!wasLit0)
          glDisable(GL_LIGHT0);
    */
}

void ShowTSShape::reset()
{
    SimSet* set = static_cast<SimSet*>(Sim::findObject("showSet"));
    if (!set)
        return;

    for (SimSet::iterator itr = set->begin(); itr != set->end(); itr++)
        static_cast<ShowTSShape*>(*itr)->resetInstance();
}

void ShowTSShape::resetInstance()
{
    // do nothing for now...
    // eventually, may need to delete shape and then reload it, but seems to work fine now
}

void ShowTSShape::advanceTime(U32 delta)
{
    // get the show set...
    SimSet* set = static_cast<SimSet*>(Sim::findObject("showSet"));

    // update the instances...
    if (set)
        for (SimSet::iterator itr = set->begin(); itr != set->end(); itr++)
            static_cast<ShowTSShape*>(*itr)->advanceTimeInstance(delta);

    // set thread position slider
    GuiSliderCtrl* slider = static_cast<GuiSliderCtrl*>(Sim::findObject("threadPosition"));
    GuiTextListCtrl* threadList = static_cast<GuiTextListCtrl*>(Sim::findObject("threadList"));
    GuiTextCtrl* transitionSignal = static_cast<GuiTextCtrl*>(Sim::findObject("transitionSignal"));
    bool inTransition = false;
    if (currentShow && threadList && slider && slider->mAwake && threadList->getSelectedCell().y >= 0)
    {
        S32 th = threadList->getSelectedCell().y;
        if (currentShow->getPlay(th))
        {
            char buffer[32];
            dSprintf(buffer, 32, "%g", currentShow->getPos(th));
            slider->setScriptValue(buffer);
        }
        else
            currentShow->setPos(th, slider->getValue());

        inTransition = currentShow->isInTransition(th);
    }

    if (transitionSignal)
    {
        if (inTransition)
            transitionSignal->setText("T");
        else
            transitionSignal->setText(" ");
    }

    if (!gInitLightingSliders)
    {
        char buffer[32];
        slider = static_cast<GuiSliderCtrl*>(Sim::findObject("emapAlpha"));
        if (slider)
        {
            dSprintf(buffer, 32, "%f", emapAlpha);
            slider->setField("value", buffer);
        }
        slider = static_cast<GuiSliderCtrl*>(Sim::findObject("ambR"));
        if (slider)
        {
            dSprintf(buffer, 32, "%g", ambR);
            slider->setField("value", buffer);
        }
        slider = static_cast<GuiSliderCtrl*>(Sim::findObject("ambG"));
        if (slider)
        {
            dSprintf(buffer, 32, "%g", ambG);
            slider->setField("value", buffer);
        }
        slider = static_cast<GuiSliderCtrl*>(Sim::findObject("ambB"));
        if (slider)
        {
            dSprintf(buffer, 32, "%g", ambB);
            slider->setField("value", buffer);
        }
        slider = static_cast<GuiSliderCtrl*>(Sim::findObject("diffR"));
        if (slider)
        {
            dSprintf(buffer, 32, "%g", diffR);
            slider->setField("value", buffer);
        }
        slider = static_cast<GuiSliderCtrl*>(Sim::findObject("diffG"));
        if (slider)
        {
            dSprintf(buffer, 32, "%g", diffG);
            slider->setField("value", buffer);
        }
        slider = static_cast<GuiSliderCtrl*>(Sim::findObject("diffB"));
        if (slider)
        {
            dSprintf(buffer, 32, "%g", diffB);
            slider->setField("value", buffer);
        }
        gInitLightingSliders = true;
    }

    // handle lighting sliders...
    slider = static_cast<GuiSliderCtrl*>(Sim::findObject("emapAlpha"));
    if (slider)
        emapAlpha = slider->getValue();
    slider = static_cast<GuiSliderCtrl*>(Sim::findObject("ambR"));
    if (slider)
        ambR = slider->getValue();
    slider = static_cast<GuiSliderCtrl*>(Sim::findObject("ambG"));
    if (slider)
        ambG = slider->getValue();
    slider = static_cast<GuiSliderCtrl*>(Sim::findObject("ambB"));
    if (slider)
        ambB = slider->getValue();
    slider = static_cast<GuiSliderCtrl*>(Sim::findObject("diffR"));
    if (slider)
        diffR = slider->getValue();
    slider = static_cast<GuiSliderCtrl*>(Sim::findObject("diffG"));
    if (slider)
        diffG = slider->getValue();
    slider = static_cast<GuiSliderCtrl*>(Sim::findObject("diffB"));
    if (slider)
        diffB = slider->getValue();


    // handle movement...a little weird because spliced from early version:

    F32 leftSpeed = gShowLeftAction;
    F32 rightSpeed = gShowRightAction;
    F32 forwardSpeed = gShowForwardAction;
    F32 backwardSpeed = gShowBackwardAction;
    F32 upSpeed = gShowUpAction;
    F32 downSpeed = gShowDownAction;

    F32 timeScale = gShowMovementSpeed * 25.0f * ((F32)delta) / F32(1000);
    if (!ShowTSShape::handleMovement(leftSpeed, rightSpeed, forwardSpeed, backwardSpeed, upSpeed, downSpeed, timeScale))
    {
        Point3F vec;
        cameraMatrix.getColumn(0, &vec);
        camPos += vec * (rightSpeed - leftSpeed) * timeScale;
        cameraMatrix.getColumn(1, &vec);
        camPos += vec * (forwardSpeed - backwardSpeed) * timeScale;
        cameraMatrix.getColumn(2, &vec);
        camPos += vec * (upSpeed - downSpeed) * timeScale;
    }
}

void ShowTSShape::advanceTimeInstance(U32 delta)
{
    float dt = timeScale * 0.001f * (float)delta;

    for (S32 i = 0; i < threads.size(); i++)
        if (play[i])
            shapeInstance->advanceTime(scale[i] * dt, threads[i]);

    if (addGround)
    {
        shapeInstance->animateGround();
        addGroundTransform(shapeInstance->getGroundTransform());
    }

    shapeInstance->animate();
}

//------------------------------------------------------------
// TS Control for show objects
//------------------------------------------------------------

bool ShowProcessCameraQuery(CameraQuery* q)
{
    MatrixF xRot, zRot;
    xRot.set(EulerF(camRot.x, 0, 0));
    zRot.set(EulerF(0, 0, camRot.z));

    cameraMatrix.mul(zRot, xRot);
    q->nearPlane = 0.1;
    q->farPlane = 2100.0;
    q->fov = 3.1415 / 2;
    cameraMatrix.setColumn(3, camPos);
    q->cameraMatrix = cameraMatrix;
    return true;
}

void ShowRenderWorld()
{
    static bool initGlow = false;
    if (!initGlow)
    {
        initGlow = true;
        GlowBuffer* glowBuff = static_cast<GlowBuffer*>(Sim::findObject("GlowBufferData"));
        if (glowBuff)
        {
            glowBuff->init();
        }
    }

    getCurrentClientSceneGraph()->renderScene();

}

class ShowTSCtrl : public GuiTSCtrl
{
    typedef GuiTSCtrl Parent;
public:

    bool processCameraQuery(CameraQuery* query);
    void renderWorld(const RectI& updateRect);
    //   void onMouseMove(const GuiEvent &evt);

    DECLARE_CONOBJECT(ShowTSCtrl);
};

IMPLEMENT_CONOBJECT(ShowTSCtrl);

bool ShowTSCtrl::processCameraQuery(CameraQuery* q)
{
    return ShowProcessCameraQuery(q);
}

void ShowTSCtrl::renderWorld(const RectI& updateRect)
{
    SceneGraph::renderFog = false;

    ShowRenderWorld();
    GFX->setClipRect(updateRect);
}

//------------------------------------------------------------
// console functions for show plugin
//------------------------------------------------------------
ConsoleFunctionGroupBegin(ShowTool, "Functions for controlling the show tool.");

ConsoleFunction(showShapeLoad, void, 2, 3, "(string shapeName, bool faceCamera)")
{
    argc;

    Point3F vec, pos;
    cameraMatrix.getColumn(1, &vec);
    cameraMatrix.getColumn(3, &pos);
    vec *= initialShowDistance;

    ShowTSShape* show = new ShowTSShape(argv[1]);

    if (show->shapeLoaded())
    {
        show->setPosition(pos + vec);
        show->registerObject();
        Sim::getRootGroup()->addObject(show);
    }
    else
        delete show;

    showUpdateThreadControl();

    // make sure detail slider is set correctly
    setDetailSlider();
}

ConsoleFunction(showSequenceLoad, void, 2, 3, "(string sequenceFile, string sequenceName=NULL)")
{
    ShowTSShape* currentShow = ShowTSShape::currentShow;
    if (!currentShow)
        return;

    char buffer[512];

    if (argc == 2)
        dSprintf(buffer, 512, "new TSShapeConstructor() { baseShape=\"%s\";sequence0=\"%s\"; };", currentShow->getShapeName(), argv[1]);
    else
        dSprintf(buffer, 512, "new TSShapeConstructor() { baseShape=\"%s\";sequence0=\"%s %s\"; };", currentShow->getShapeName(), argv[1], argv[2]);

    Con::evaluate(buffer);
    ShowTSShape::reset();
}

ConsoleFunction(showSelectSequence, void, 1, 1, "")
{
    ShowTSShape* currentShow = ShowTSShape::currentShow;
    if (!currentShow)
        return;

    GuiTextListCtrl* threadList = static_cast<GuiTextListCtrl*>(Sim::findObject("threadList"));
    if (!threadList)
        return;

    GuiTextListCtrl* sequenceList = static_cast<GuiTextListCtrl*>(Sim::findObject("sequenceList"));
    if (!sequenceList)
        return;

    S32 threadNum = threadList->getSelectedCell().y;
    S32 seq = sequenceList->getSelectedCell().y;

    if (threadNum >= 0 && seq >= 0)
    {
        if (Con::getBoolVariable("$showTransition", false) && !currentShow->isBlend(seq))
        {
            F32 pos;
            F32 dur = Con::getFloatVariable("$showTransitionDuration", 0.2f);
            GuiSliderCtrl* tpos = static_cast<GuiSliderCtrl*>(Sim::findObject("transitionPosition"));
            if (Con::getBoolVariable("$showTransitionSynched", true) || !tpos)
                pos = currentShow->getPos(threadNum);
            else
                pos = tpos->getValue();
            bool targetPlay = Con::getBoolVariable("$showTransitionTargetPlay", true);
            currentShow->transitionToSequence(threadNum, seq, pos, dur, targetPlay);
        }
        else
            currentShow->setSequence(threadNum, seq);

        GuiSliderCtrl* slider = static_cast<GuiSliderCtrl*>(Sim::findObject("threadPosition"));
        if (slider && slider->getRoot())
        {
            char buffer[32];
            dSprintf(buffer, 32, "%g", currentShow->getPos(threadNum));
            slider->setScriptValue(buffer);
        }
    }

    showUpdateThreadControl();
    return;
}

ConsoleFunction(showSetDetailSlider, void, 1, 1, "")
{
    setDetailSlider();
}

ConsoleFunction(showUpdateThreadControl, void, 1, 1, "")
{
    showUpdateThreadControl();
}

ConsoleFunction(showPlay, void, 1, 2, "(int threadNum = -1)")
{
    ShowTSShape* currentShow = ShowTSShape::currentShow;
    if (!currentShow)
        return;

    if (argc == 1)
    {
        for (S32 i = 0; i < currentShow->getThreadCount(); i++)
            currentShow->setPlay(i, 1);
    }
    else
    {
        S32 i = dAtoi(argv[1]);
        if (i >= 0)
            currentShow->setPlay(i, 1);
    }
}

ConsoleFunction(showStop, void, 1, 2, "(int threadNum = -1)")
{
    ShowTSShape* currentShow = ShowTSShape::currentShow;
    if (!currentShow)
        return;

    if (argc == 1)
    {
        for (S32 i = 0; i < currentShow->getThreadCount(); i++)
            currentShow->setPlay(i, 0);
    }
    else
    {
        S32 i = dAtoi(argv[1]);
        if (i >= 0)
            currentShow->setPlay(i, 0);
    }
}

ConsoleFunction(showSetScale, void, 3, 3, "(int threadNum, float scale)")
{
    ShowTSShape* currentShow = ShowTSShape::currentShow;
    if (!currentShow)
        return;

    S32 idx = dAtoi(argv[1]);
    float s = dAtof(argv[2]);
    if (idx >= 0)
        currentShow->setThreadScale(idx, s);

    showUpdateThreadControl();
}

ConsoleFunction(showSetPos, void, 2, 2, "(int threadNum, float pos)")
{
    ShowTSShape* currentShow = ShowTSShape::currentShow;
    if (!currentShow)
        return;

    S32 idx = dAtoi(argv[1]);
    float s = dAtof(argv[2]);
    if (idx >= 0)
        currentShow->setPos(idx, s);
}
ConsoleFunction(showNewThread, void, 1, 1, "")
{
    ShowTSShape* currentShow = ShowTSShape::currentShow;
    if (!currentShow)
        return;

    currentShow->newThread();

    showUpdateThreadControl();
}

ConsoleFunction(showDeleteThread, void, 2, 2, "(int threadNum)")
{
    ShowTSShape* currentShow = ShowTSShape::currentShow;
    if (!currentShow)
        return;

    S32 th = dAtoi(argv[1]);
    if (th >= 0)
        currentShow->deleteThread(th);

    showUpdateThreadControl();
}


ConsoleFunction(showToggleRoot, void, 1, 1, "")
{
    ShowTSShape* currentShow = ShowTSShape::currentShow;
    if (!currentShow)
        return;

    currentShow->setAnimateRoot(!currentShow->getAnimateRoot());
}



ConsoleFunction(showToggleStick, void, 1, 1, "")
{
    ShowTSShape* currentShow = ShowTSShape::currentShow;
    if (!currentShow)
        return;

    currentShow->setStickToGround(!currentShow->getStickToGround());
}

ConsoleFunction(showSetCamera, void, 2, 2, "(char orbitShape) t or T to orbit, else free-fly.")
{
    ShowTSShape* currentShow = ShowTSShape::currentShow;

    if (argv[1][0] == 't' || argv[1][0] == 'T' && currentShow)
        // orbit
        currentShow->orbitUs();
    else
        // no orbit -- camera moves freely
        orbitPos = NULL;
}

ConsoleFunction(showSetKeyboard, void, 2, 2, "(char moveShape) Set to t or T.")
{
    keyboardControlsShape = (argv[1][0] == 't' || argv[1][0] == 'T');
}

ConsoleFunction(showTurnLeft, void, 2, 2, "(float amt)")
{
    gShowShapeLeftSpeed = dAtof(argv[1]);
}


ConsoleFunction(showTurnRight, void, 2, 2, "(float amt)")
{
    gShowShapeRightSpeed = dAtof(argv[1]);
}

ConsoleFunction(showSetLightDirection, void, 1, 1, "")
{
    cameraMatrix.getColumn(1, (Point3F*)&lightDir);
    lightDir.x *= -1.0f;
    lightDir.y *= -1.0f;
    lightDir.z *= -1.0f;
    lightDir.w *= -1.0f;
}

ConsoleFunctionGroupEnd(ShowTool);

void ShowInit()
{
    Con::addVariable("showForwardAction", TypeF32, &gShowForwardAction);
    Con::addVariable("showBackwardAction", TypeF32, &gShowBackwardAction);
    Con::addVariable("showUpAction", TypeF32, &gShowUpAction);
    Con::addVariable("showDownAction", TypeF32, &gShowDownAction);
    Con::addVariable("showLeftAction", TypeF32, &gShowLeftAction);
    Con::addVariable("showRightAction", TypeF32, &gShowRightAction);

    Con::addVariable("showMovementSpeed", TypeF32, &gShowMovementSpeed);

    Con::addVariable("showPitch", TypeF32, &camRot.x);
    Con::addVariable("showYaw", TypeF32, &camRot.z);

    cameraMatrix.identity();
    camRot.set(0, 0, 0);
    camPos.set(10, 10, 10);
    cameraMatrix.setColumn(3, camPos);
}
