//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "gui/core/guiCanvas.h"
#include "game/guiPlayerView.h"
#include "console/consoleTypes.h"

static const F32 MaxOrbitDist = 5.0f;
static const S32 MaxAnimations = 6;

IMPLEMENT_CONOBJECT(GuiPlayerView);

//------------------------------------------------------------------------------
GuiPlayerView::GuiPlayerView() : GuiTSCtrl()
{
    mActive = true;
    mMouseState = None;
    mModel = NULL;
    mWeapon = NULL;
    mLastMousePoint.set(0, 0);
    lastRenderTime = 0;
    runThread = 0;
    wNode = -1;
    pNode = -1;
    mAnimationSeq = 0;
}


//------------------------------------------------------------------------------
GuiPlayerView::~GuiPlayerView()
{
    if (mModel)
    {
        delete mModel;
        mModel = NULL;
    }
    if (mWeapon)
    {
        delete mWeapon;
        mWeapon = NULL;
    }
}


//------------------------------------------------------------------------------
ConsoleMethod(GuiPlayerView, setModel, void, 4, 4, "playerView.setModel( raceGender, skin )")
{
    argc;
    object->setPlayerModel(argv[2], argv[3]);
}

ConsoleMethod(GuiPlayerView, setSeq, void, 3, 3, "playerView.setSeq( index )")
{
    argc;
    object->setPlayerSeq(dAtoi(argv[2]));
}

//------------------------------------------------------------------------------
bool GuiPlayerView::onWake()
{
    if (!Parent::onWake())
        return(false);

    mCameraMatrix.identity();
    mCameraRot.set(0, 0, 3.9);
    mCameraPos.set(0, 1.75, 1.25);
    mCameraMatrix.setColumn(3, mCameraPos);
    mOrbitPos.set(0, 0, 0);
    mOrbitDist = 3.5f;

    return(true);
}

//------------------------------------------------------------------------------
void GuiPlayerView::onMouseDown(const GuiEvent& event)
{
    if (!mActive || !mVisible || !mAwake)
        return;

    mMouseState = Rotating;
    mLastMousePoint = event.mousePoint;
    mouseLock();
}


//------------------------------------------------------------------------------
void GuiPlayerView::onMouseUp(const GuiEvent&/*event*/)
{
    mouseUnlock();
    mMouseState = None;
}


//------------------------------------------------------------------------------
void GuiPlayerView::onMouseDragged(const GuiEvent& event)
{
    if (mMouseState != Rotating)
        return;

    Point2I delta = event.mousePoint - mLastMousePoint;
    mLastMousePoint = event.mousePoint;

    mCameraRot.x += (delta.y * 0.01);
    mCameraRot.z += (delta.x * 0.01);
}


//------------------------------------------------------------------------------
void GuiPlayerView::onRightMouseDown(const GuiEvent& event)
{
    mMouseState = Zooming;
    mLastMousePoint = event.mousePoint;
    mouseLock();
}


//------------------------------------------------------------------------------
void GuiPlayerView::onRightMouseUp(const GuiEvent&/*event*/)
{
    mouseUnlock();
    mMouseState = None;
}


//------------------------------------------------------------------------------
void GuiPlayerView::onRightMouseDragged(const GuiEvent& event)
{
    if (mMouseState != Zooming)
        return;

    S32 delta = event.mousePoint.y - mLastMousePoint.y;
    mLastMousePoint = event.mousePoint;

    mOrbitDist += (delta * 0.01);
}

void GuiPlayerView::setPlayerSeq(S32 index)
{
    if (index > MaxAnimations || index < 0)
        return;

    mAnimationSeq = index;
}

//------------------------------------------------------------------------------
void GuiPlayerView::setPlayerModel(const char* shape, const char* skin)
{
    if (mModel)
    {
        delete mModel;
        mModel = NULL;
    }

    if (mWeapon)
    {
        delete mWeapon;
        mWeapon = NULL;
    }

    runThread = 0;

    Resource<TSShape> hShape = ResourceManager->load(shape);
    if (!bool(hShape))
        return;

    mModel = new TSShapeInstance(hShape, true);
    AssertFatal(mModel, "ERROR!  Failed to load warrior model!");

    /*
       // Set the skin:
       if ( !mModel->ownMaterialList() )
          mModel->cloneMaterialList();

       TSMaterialList* materialList = mModel->getMaterialList();
       for ( U32 i = 0; i < materialList->mMaterialNames.size(); i++ )
       {
          const char* name = materialList->mMaterialNames[i];
          if ( !name )
             continue;

          const U32 len = dStrlen( name );
          AssertFatal( len < 200, "GuiPlayerView::setPlayerModel - Skin name exceeds maximum length!" );
          if ( len < 6 )
             continue;

          const char* replace = dStrstr( name, "base." );
          if ( !replace )
             continue;

          char newName[256];
          dStrncpy( newName, name, replace - name );
          newName[replace - name] = 0;
          dStrcat( newName, skin );
          dStrcat( newName, "." );
          dStrcat( newName, replace + 5 );

          TextureHandle test = TextureHandle( newName, MeshTexture, false );
          if ( test.getGLName() )
             materialList->mMaterials[i] = test;
          else
             materialList->mMaterials[i] = TextureHandle( name, MeshTexture, false );
       }
    */
    // Initialize camera values:
    mOrbitPos = mModel->getShape()->center;
    mMinOrbitDist = mModel->getShape()->radius;

    //   // initialize run thread
    //   S32 sequence = hShape->findSequence("dummyRun");
    //
    //   if( sequence != -1 )
    //   {
    //      runThread = mModel->addThread();
    //      mModel->setPos( runThread, 0 );
    //      mModel->setTimeScale( runThread, 1 );
    //      mModel->setSequence( runThread, sequence, 0 );
    //   }

       // create a weapon for this dude
    Resource<TSShape> wShape;
    if (dStrcmp(shape, "heavy_male") == 0 || dStrcmp(shape, "bioderm_heavy") == 0 || dStrcmp(shape, "heavy_female") == 0)
    {
        wShape = ResourceManager->load("shapes/weapon_mortar.dts");
    }
    else if (dStrcmp(shape, "medium_male") == 0 || dStrcmp(shape, "bioderm_medium") == 0 || dStrcmp(shape, "medium_female") == 0)
    {
        wShape = ResourceManager->load("shapes/weapon_plasma.dts");
    }
    else
    {
        wShape = ResourceManager->load("shapes/weapon_disc.dts");
    }

    if (!bool(wShape))
        return;

    pNode = hShape->findNode("mount0");
    wNode = wShape->findNode("mountPoint");

    mWeapon = new TSShapeInstance(wShape, true);
    AssertFatal(mWeapon, "ERROR!  Failed to load Weapon Model!");

    // the first time recording
    lastRenderTime = Platform::getVirtualMilliseconds();
}

void GuiPlayerView::getWeaponTransform(MatrixF* mat)
{
    MatrixF weapTrans = mWeapon->mNodeTransforms[wNode];
    Point3F weapOffset = -weapTrans.getPosition();
    MatrixF modelTrans = mModel->mNodeTransforms[pNode];
    modelTrans.mulP(weapOffset);
    modelTrans.setPosition(weapOffset);
    *mat = modelTrans;
}

//------------------------------------------------------------------------------
bool GuiPlayerView::processCameraQuery(CameraQuery* query)
{
    // Make sure the orbit distance is within the acceptable range:
    mOrbitDist = (mOrbitDist < mMinOrbitDist) ? mMinOrbitDist : ((mOrbitDist > MaxOrbitDist) ? MaxOrbitDist : mOrbitDist);

    // Adjust the camera so that we are still facing the model:
    Point3F vec;
    MatrixF xRot, zRot;
    xRot.set(EulerF(mCameraRot.x, 0, 0));
    zRot.set(EulerF(0, 0, mCameraRot.z));

    mCameraMatrix.mul(zRot, xRot);
    mCameraMatrix.getColumn(1, &vec);
    vec *= mOrbitDist;
    mCameraPos = mOrbitPos - vec;

    query->farPlane = 2100.0;
    query->nearPlane = query->farPlane / 5000.0f;
    query->fov = 3.1415 / 3.5;
    mCameraMatrix.setColumn(3, mCameraPos);
    query->cameraMatrix = mCameraMatrix;

    return(true);
}


//------------------------------------------------------------------------------
void GuiPlayerView::renderWorld(const RectI& updateRect)
{
    /*
       if ( !(bool)mModel || !(bool)mWeapon )
          return;

       S32 time = Platform::getVirtualMilliseconds();
       S32 dt = time - lastRenderTime;
       lastRenderTime = time;

       glClear( GL_DEPTH_BUFFER_BIT );
       glMatrixMode( GL_MODELVIEW );

       glEnable( GL_DEPTH_TEST );
       glDepthFunc( GL_LEQUAL );

       // animate and render in a run pose
       F32 fdt = dt;
    //   mModel->advanceTime( fdt/1000.f, runThread );
       mModel->animate();
       mModel->render();

       // render a weapon
       MatrixF mat;
       getWeaponTransform( &mat );
       glPushMatrix();
       dglMultMatrix( &mat );

       mWeapon->render();

       glPopMatrix();

       glDisable( GL_DEPTH_TEST );
       dglSetClipRect( updateRect );
       dglSetCanonicalState();
    */
}

void GuiPlayerView::onMouseEnter(const GuiEvent& event)
{
    Con::executef(this, 1, "onMouseEnter");
}

void GuiPlayerView::onMouseLeave(const GuiEvent& event)
{
    Con::executef(this, 1, "onMouseLeave");
}
