//------------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//------------------------------------------------------------------------------

#include "gui/game/guiObjectView.h"
#include "renderInstance/renderInstMgr.h"
#include "sceneGraph/lightManager.h"
#include "lightingSystem/sgLightManager.h"
#include "sceneGraph/sceneGraph.h"

//-----------------------------------------------------------------------------
// GuiObjectView
//-----------------------------------------------------------------------------

GuiObjectView::GuiObjectView()
    : mMaxOrbitDist(5.0f),
      mMinOrbitDist(0.0f),
      mOrbitDist(9.5f),
      mMouseState(None),
      mModel(NULL),
      mMountedModel(NULL),
      mLastMousePoint(0, 0),
      lastRenderTime(0),
      runThread(0),
      mMountNode(NO_NODE),
      mAnimationSeq(0),
      mFakeSun(NULL)
{
    mActive = true;

    // TODO: lots of hardcoded things in here
    mCameraMatrix.identity();
    mCameraRot.set(0.0f, 0.0f, 0.0f);//3.9f);
    mCameraRotSpeed.set(0.0f, 0.0f, 0.001f);
    mCameraPos.set(0.0f, 1.75f, 1.25f);
    mCameraMatrix.setColumn(3, mCameraPos);
    mOrbitPos.set(0.0f, 0.0f, 0.0f);

    mModelName = NULL;
    mSkinName = StringTable->insert("base");
    mAutoSize = false;
}

GuiObjectView::~GuiObjectView()
{
    SAFE_DELETE(mModel);
    SAFE_DELETE(mMountedModel);
    SAFE_DELETE(mFakeSun);
}

void GuiObjectView::onSleep()
{
    Parent::onSleep();
    setObjectModel(NULL, NULL);
}

bool GuiObjectView::onWake()
{
    if (!Parent::onWake())
        return false;

    if (!mFakeSun)
        mFakeSun = new LightInfo();
    mFakeSun->mColor.set(1.0f, 1.0f, 1.0f);
    mFakeSun->mAmbient.set(0.5f, 0.5f, 0.5f);
    mFakeSun->mDirection.set(0.f, 0.707f, -0.707f);

    if (mModelName && mModelName[0])
        setObjectModel(mModelName, mSkinName);

    return true;
}

void GuiObjectView::initPersistFields()
{
    Parent::initPersistFields();

    addField("model", TypeFilename, Offset(mModelName, GuiObjectView));
    addField("skin", TypeString, Offset(mSkinName, GuiObjectView));
    addField("cameraRotX", TypeF32, Offset(mCameraRot.x, GuiObjectView));
    addField("cameraRotY", TypeF32, Offset(mCameraRot.y, GuiObjectView));
    addField("cameraRotZ", TypeF32, Offset(mCameraRot.z, GuiObjectView));
    addField("cameraXRotSpeed", TypeF32, Offset(mCameraRotSpeed.x, GuiObjectView));
    addField("cameraYRotSpeed", TypeF32, Offset(mCameraRotSpeed.y, GuiObjectView));
    addField("cameraZRotSpeed", TypeF32, Offset(mCameraRotSpeed.z, GuiObjectView));
    addField("orbitDistance", TypeF32, Offset(mOrbitDist, GuiObjectView));
    addField("autoSize", TypeBool, Offset(mAutoSize, GuiObjectView));
}

void GuiObjectView::onMouseDown(const GuiEvent &event)
{
    if (!mActive || !mVisible || !mAwake)
    {
        return;
    }

    mMouseState = Rotating;
    mLastMousePoint = event.mousePoint;
    mouseLock();
}

void GuiObjectView::onMouseUp(const GuiEvent &event)
{
    mouseUnlock();
    mMouseState = None;
}

void GuiObjectView::onMouseDragged(const GuiEvent &event)
{
    if (mMouseState != Rotating)
    {
        return;
    }

    Point2I delta = event.mousePoint - mLastMousePoint;
    mLastMousePoint = event.mousePoint;

    mCameraRot.x += (delta.y * 0.01f);
    mCameraRot.z += (delta.x * 0.01f);
}

void GuiObjectView::onRightMouseDown(const GuiEvent &event)
{
    mMouseState = Zooming;
    mLastMousePoint = event.mousePoint;
    mouseLock();
}

void GuiObjectView::onRightMouseUp(const GuiEvent &event)
{
    mouseUnlock();
    mMouseState = None;
}

void GuiObjectView::onRightMouseDragged(const GuiEvent &event)
{
    if (mMouseState != Zooming)
    {
        return;
    }

    S32 delta = event.mousePoint.y - mLastMousePoint.y;
    mLastMousePoint = event.mousePoint;

    mOrbitDist += (delta * 0.01f);
}

void GuiObjectView::setObjectAnimation(S32 index)
{
    if ((0 > index) || (index > MAX_ANIMATIONS))
    {
        Con::warnf(avar(
            "GuiObjectView: The index %d is outside the permissible range. Please specify an animation index in the range [0, %d]",
            index, MAX_ANIMATIONS));
        return;
    }

    mAnimationSeq = index;
}

void GuiObjectView::setObjectModel(const char *modelName, const char *skinName)
{
    SAFE_DELETE(mModel);

    runThread = 0;

    if (!modelName)
        return;

    Resource<TSShape> model = ResourceManager->load(modelName);
    if (!bool(model))
    {
        Con::warnf(avar("GuiObjectView: Failed to load model %s. Please check your model name and load a valid model.",
                        modelName));
        return;
    }

    mModel = new TSShapeInstance(model, true);
    AssertFatal(mModel,
                avar("GuiObjectView: Failed to load model %s. Please check your model name and load a valid model.",
                     modelName));

    if (dStrcmp(skinName, "base") != 0)
    {
        StringHandle skinHandle = StringHandle(skinName);
        mModel->reSkin(skinHandle);
    }

    // Initialize camera values:
    mOrbitPos = mModel->getShape()->center;
    mMinOrbitDist = mModel->getShape()->radius;

    // the first time recording
    lastRenderTime = Platform::getVirtualMilliseconds();

    char *mountName = new char[16];
    dStrncpy(mountName, avar("mount%d", 0), 15);
    mMountNode = mModel->getShape()->findNode(mountName);
    delete[] mountName;

}

void GuiObjectView::setEmpty()
{
    SAFE_DELETE(mModel);
}

void GuiObjectView::setMountedObject(const char *modelName, S32 mountPoint)
{
    SAFE_DELETE(mMountedModel);

    // create a weapon for this dude
    Resource<TSShape> model = ResourceManager->load(modelName);

    if (!bool(model))
    {
        Con::warnf(avar(
            "GuiObjectView: Failed to load mounted object model %s. Please check your model name and load a valid model.",
            modelName));
        return;
    }

    char *mountName = new char[16];
    dStrncpy(mountName, avar("mount%d", 0), 15);
    mMountNode = mModel->getShape()->findNode(mountName);
    delete[] mountName;


    mMountedModel = new TSShapeInstance(model, true);
    AssertFatal(mMountedModel, avar(
        "GuiObjectView: Failed to load mounted object model %s. Please check your model name and load a valid model.",
        modelName));
}

void GuiObjectView::getMountedObjTransform(MatrixF *mat)
{
    if ((!mMountedModel) || (mMountNode == NO_NODE))
    {
        // there is no mounted model or node to mount to
        return;
    }

    MatrixF mountedTrans;
    mountedTrans.identity();

    S32 mountPoint = mMountedModel->getShape()->findNode("mountPoint");
    if (mountPoint != -1)
        mountedTrans = mMountedModel->mNodeTransforms[mountPoint];

    Point3F mountedOffset = -mountedTrans.getPosition();
    MatrixF modelTrans = mModel->mNodeTransforms[mMountNode];
    modelTrans.mulP(mountedOffset);
    modelTrans.setPosition(mountedOffset);
    *mat = modelTrans;
}

bool GuiObjectView::processCameraQuery(CameraQuery *query)
{
    // Adjust the camera so that we are still facing the model:
    Point3F vec;
    MatrixF xRot, zRot;
    xRot.set(EulerF(mCameraRot.x, 0.0f, 0.0f));
    zRot.set(EulerF(0.0f, 0.0f, mCameraRot.z));

    mCameraMatrix.mul(zRot, xRot);
    mCameraMatrix.getColumn(1, &vec);
    vec *= mOrbitDist;
    mCameraPos = mOrbitPos - vec;

    query->farPlane = 2100.0f;
    query->nearPlane = query->farPlane / 5000.0f;
    query->fov = 45.0f;
    mCameraMatrix.setColumn(3, mCameraPos);
    query->cameraMatrix = mCameraMatrix;

    return true;
}

void GuiObjectView::onMouseEnter(const GuiEvent &event)
{
    Con::executef(this, 1, "onMouseEnter");
}

void GuiObjectView::onMouseLeave(const GuiEvent &event)
{
    Con::executef(this, 1, "onMouseLeave");
}

void GuiObjectView::renderWorld(const RectI &updateRect)
{
    if ((!mModel) && (!mMountedModel))
    {
        // nothing to render, punt
        return;
    }

    S32 time = Platform::getVirtualMilliseconds();
    S32 dt = time - lastRenderTime;
    lastRenderTime = time;

    getCurrentClientSceneGraph()->buildFogTexture(NULL);

    LightManager *lm = getCurrentClientSceneGraph()->getLightManager();
    //lm->sgUnregisterAllLights();
    //lm->sgClearSpecialLights();
    LightInfo* oldLight = lm->sgGetSpecialLight(LightManager::sgSunLightType);
    lm->sgSetSpecialLight(LightManager::sgSunLightType, mFakeSun);

    GFX->setZEnable(true);
    GFX->setZWriteEnable(true);
    GFX->setZFunc(GFXCmpLessEqual);
    GFX->setCullMode(GFXCullNone);

    MatrixF meshCamTrans(true);
    TSMesh::setCamTrans(meshCamTrans);
    TSMesh::setSceneState(NULL);
    TSMesh::setObject(NULL);

    if (mModel)
    {
        mCameraRot += mCameraRotSpeed * dt;

        // animate and render in a run pose
        //F32 fdt = dt;
        // mModel->advanceTime( fdt/1000.f, runThread );
        // mModel->animate();
        mModel->render();
    }

    if (mMountedModel)
    {
        // render a weapon
        MatrixF mat;
        getMountedObjTransform(&mat);

        GFX->pushWorldMatrix();
        GFX->multWorld(mat);

        mMountedModel->render();

        GFX->popWorldMatrix();
    }

    gRenderInstManager.sort();
    gRenderInstManager.render();
    gRenderInstManager.clear();

    lm->sgSetSpecialLight(LightManager::sgSunLightType, oldLight);

    //lm->sgUnregisterGlobalLight(mFakeSun);
    //lm->sgClearSpecialLights();
    //lm->sgUnregisterAllLights();
}

void GuiObjectView::setOrbitDistance(F32 distance)
{
    // Make sure the orbit distance is within the acceptable range
    mOrbitDist = mClampF(distance, mMinOrbitDist, mMaxOrbitDist);
}

//-----------------------------------------------------------------------------
// Console stuff (GuiObjectView)
//-----------------------------------------------------------------------------

IMPLEMENT_CONOBJECT(GuiObjectView);

ConsoleMethod(GuiObjectView, setModel, void, 4, 4,
              "(string shapeName)\n"
              "(string skinName)\n"
              "Sets the model to be displayed in this control\n\n"
              "\\param shapeName Name of the model to display.\n"
              "\\param skinName Name of the skin to use for the model.\n")
{
    argc;
    object->setObjectModel(argv[2], argv[3]);
}

ConsoleMethod(GuiObjectView, setSeq, void, 3, 3,
              "(int index)\n"
              "Sets the animation to play for the viewed object.\n\n"
              "\\param index The index of the animation to play.")
{
    argc;
    object->setObjectAnimation(dAtoi(argv[2]));
}

ConsoleMethod(GuiObjectView, setMount, void, 4, 4,
              "(string shapeName, int mountPoint)\n"
              "Mounts the given model to the specified mount point of the primary model displayed in this control.\n\n"
              "\\param shapeName Name of the model to mount."
              "\\param mountPoint Index of the mount point to be mounted to. Corresponds to \"mountPointN\" in your shape where N is the number passed here.")
{
    argc;
    object->setMountedObject(argv[2], dAtoi(argv[3]));
}

ConsoleMethod(GuiObjectView, setOrbitDistance, void, 3, 3,
              "(float distance)\n"
              "Sets the distance at which the camera orbits the object. Clamped to the acceptable range defined in the class by min and max orbit distances.\n\n"
              "\\param distance The distance to set the orbit to (will be clamped).")
{
    argc;
    object->setOrbitDistance(dAtof(argv[2]));
}

ConsoleMethod(GuiObjectView, setEmpty, void, 2, 2,
              "()\n"
              "Removes the model displayed in this control\n\n")
{
    argc;
    object->setEmpty();
}
