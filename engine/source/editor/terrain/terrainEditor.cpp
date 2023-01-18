//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "editor/terrain/terrainEditor.h"
#include "game/collisionTest.h"
#include "terrain/terrData.h"
#include "gui/core/guiCanvas.h"
#include "console/consoleTypes.h"
#include "editor/terrain/terrainActions.h"
#include "interior/interiorInstance.h"
#include "interior/interior.h"
#include "game/gameConnection.h"
#include "sim/netObject.h"
#include "core/frameAllocator.h"
#include "gfx/gfxDevice.h"

IMPLEMENT_CONOBJECT(TerrainEditor);

Selection::Selection() :
    Vector<GridInfo>(__FILE__, __LINE__),
    mName(0),
    mUndoFlags(0),
    mHashListSize(1024)
{
    VECTOR_SET_ASSOCIATION(mHashLists);

    // clear the hash list
    mHashLists.setSize(mHashListSize);
    reset();
}

Selection::~Selection()
{
}

void Selection::reset()
{
    for (U32 i = 0; i < mHashListSize; i++)
        mHashLists[i] = -1;
    clear();
}

U32 Selection::getHashIndex(const Point2I& pos)
{
    Point2F pnt = Point2F(pos.x, pos.y) + Point2F(1.3f, 3.5f);
    return((U32)(mFloor(mHashLists.size() * mFmod(pnt.len() * 0.618f, 1))));
}

S32 Selection::lookup(const Point2I& pos)
{
    U32 index = getHashIndex(pos);

    S32 entry = mHashLists[index];

    while (entry != -1)
    {
        if ((*this)[entry].mGridPos == pos)
            return(entry);

        entry = (*this)[entry].mNext;
    }

    return(-1);
}

void Selection::insert(GridInfo& info)
{
    U32 index = getHashIndex(info.mGridPos);

    info.mNext = mHashLists[index];
    info.mPrev = -1;

    if (info.mNext != -1)
        (*this)[info.mNext].mPrev = size();

    mHashLists[index] = size();

    push_back(info);
}

bool Selection::remove(const GridInfo& info)
{
    U32 index = getHashIndex(info.mGridPos);

    S32 entry = mHashLists[index];

    if (entry == -1)
        return(false);

    // front?
    if ((*this)[entry].mGridPos == info.mGridPos)
        mHashLists[index] = (*this)[entry].mNext;

    while (entry != -1)
    {
        if ((*this)[entry].mGridPos == info.mGridPos)
        {
            if ((*this)[entry].mPrev != -1)
                (*this)[(*this)[entry].mPrev].mNext = (*this)[entry].mNext;
            if ((*this)[entry].mNext != -1)
                (*this)[(*this)[entry].mNext].mPrev = (*this)[entry].mPrev;

            // swap?
            if (entry != (size() - 1))
            {
                U32 last = size() - 1;

                (*this)[entry] = (*this)[size() - 1];

                if ((*this)[entry].mPrev != -1)
                    (*this)[(*this)[entry].mPrev].mNext = entry;
                else
                {
                    U32 idx = getHashIndex((*this)[entry].mGridPos);
                    AssertFatal(mHashLists[idx] == ((*this).size() - 1), "doh");
                    mHashLists[idx] = entry;
                }
                if ((*this)[entry].mNext != -1)
                    (*this)[(*this)[entry].mNext].mPrev = entry;
            }

            pop_back();
            return(true);
        }

        entry = (*this)[entry].mNext;
    }
    return(false);
}

// add unique grid info into the selection - test uniqueness by grid position
bool Selection::add(GridInfo& info)
{
    S32 index = lookup(info.mGridPos);
    if (index != -1)
        return(false);

    insert(info);
    return(true);
}

bool Selection::getInfo(Point2I pos, GridInfo& info)
{
    S32 index = lookup(pos);
    if (index == -1)
        return(false);

    info = (*this)[index];
    return(true);
}

bool Selection::setInfo(GridInfo& info)
{
    S32 index = lookup(info.mGridPos);
    if (index == -1)
        return(false);

    S32 next = (*this)[index].mNext;
    S32 prev = (*this)[index].mPrev;

    (*this)[index] = info;
    (*this)[index].mNext = next;
    (*this)[index].mPrev = prev;

    return(true);
}

F32 Selection::getAvgHeight()
{
    if (!size())
        return(0);

    F32 avg = 0.f;
    for (U32 i = 0; i < size(); i++)
        avg += (*this)[i].mHeight;

    return(avg / size());
}

//------------------------------------------------------------------------------

Brush::Brush(TerrainEditor* editor) :
    mTerrainEditor(editor)
{
    mSize = mTerrainEditor->getBrushSize();
}

const Point2I& Brush::getPosition()
{
    return(mGridPos);
}

void Brush::setPosition(const Point3F& pos)
{
    Point2I gPos;
    mTerrainEditor->worldToGrid(pos, gPos);
    setPosition(gPos);
}

void Brush::setPosition(const Point2I& pos)
{
    mGridPos = pos;
    update();
}

//------------------------------------------------------------------------------

void Brush::update()
{
    rebuild();
    // soft selection?
 //   if(mTerrainEditor->mEnableSoftBrushes)
 //   {
 //      Gui3DMouseEvent event;
 //      TerrainAction * action = mTerrainEditor->lookupAction("softSelect");
 //      AssertFatal(action, "Brush::update: no 'softSelect' action found!");

       //
 //      mTerrainEditor->setCurrentSel(this);
 //      action->process(this, event, true, TerrainAction::Process);
 //      mTerrainEditor->resetCurrentSel();
 //   }
}

//------------------------------------------------------------------------------

void BoxBrush::rebuild()
{
    reset();
    Filter filter;
    filter.set(1, &mTerrainEditor->mSoftSelectFilter);
    //
    // mSize should always be odd.

    S32 centerX = (mSize.x - 1) / 2;
    S32 centerY = (mSize.y - 1) / 2;

    F32 xFactorScale = F32(centerX) / (F32(centerX) + 0.5);
    F32 yFactorScale = F32(centerY) / (F32(centerY) + 0.5);

    for (S32 x = 0; x < mSize.x; x++)
    {
        for (S32 y = 0; y < mSize.y; y++)
        {
            GridInfo info;
            mTerrainEditor->getGridInfo(Point2I(mGridPos.x + x - centerX, mGridPos.y + y - centerY), info);
            if (mTerrainEditor->mEnableSoftBrushes && centerX != 0 && centerY != 0)
            {

                F32 xFactor = (mFabs(centerX - x) / F32(centerX)) * xFactorScale;
                F32 yFactor = (mFabs(centerY - y) / F32(centerY)) * yFactorScale;

                info.mWeight = filter.getValue(xFactor > yFactor ? xFactor : yFactor);
            }
            push_back(info);
        }
    }
}

//------------------------------------------------------------------------------

void EllipseBrush::rebuild()
{
    reset();
    Point3F center(F32(mSize.x - 1) / 2, F32(mSize.y - 1) / 2, 0);
    Filter filter;
    filter.set(1, &mTerrainEditor->mSoftSelectFilter);

    // a point is in a circle if:
    // x^2 + y^2 <= r^2
    // a point is in an ellipse if:
    // (ax)^2 + (by)^2 <= 1
    // where a = 1/halfEllipseWidth and b = 1/halfEllipseHeight

    // for a soft-selected ellipse,
    // the factor is simply the filtered: ((ax)^2 + (by)^2)

    F32 a = 1 / (F32(mSize.x) * 0.5);
    F32 b = 1 / (F32(mSize.y) * 0.5);

    for (U32 x = 0; x < mSize.x; x++)
    {
        for (U32 y = 0; y < mSize.y; y++)
        {
            F32 xp = center.x - x;
            F32 yp = center.y - y;

            F32 factor = (a * a * xp * xp) + (b * b * yp * yp);
            if (factor > 1)
                continue;

            GridInfo info;
            mTerrainEditor->getGridInfo(Point2I((S32)(mGridPos.x + x - center.x), (S32)(mGridPos.y + y - center.y)), info);
            if (mTerrainEditor->mEnableSoftBrushes)
                info.mWeight = filter.getValue(factor);

            push_back(info);
        }
    }
}

//------------------------------------------------------------------------------

SelectionBrush::SelectionBrush(TerrainEditor* editor) :
    Brush(editor)
{
    //... grab the current selection
}

void SelectionBrush::rebuild()
{
    reset();
    //... move the selection
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

TerrainEditor::TerrainEditor() :
    mTerrainBlock(0),
    mMousePos(0, 0, 0),
    mMouseBrush(0),
    mInAction(false),
    mUndoLimit(20),
    mUndoSel(0),
    mRebuildEmpty(false),
    mRebuildTextures(false),
    mGridUpdateMin(255, 255),
    mGridUpdateMax(0, 0)
{
    VECTOR_SET_ASSOCIATION(mActions);
    VECTOR_SET_ASSOCIATION(mUndoList);
    VECTOR_SET_ASSOCIATION(mRedoList);
    VECTOR_SET_ASSOCIATION(mBaseMaterialInfos);

    //
    resetCurrentSel();

    //
    mBrushSize.set(1, 1);
    mMouseBrush = new BoxBrush(this);
    mMouseDownSeq = 0;
    mIsDirty = false;
    mIsMissionDirty = false;
    mPaintMaterial = NULL;

    // add in all the actions here..
    mActions.push_back(new SelectAction(this));
    mActions.push_back(new SoftSelectAction(this));
    mActions.push_back(new OutlineSelectAction(this));
    mActions.push_back(new PaintMaterialAction(this));
    mActions.push_back(new RaiseHeightAction(this));
    mActions.push_back(new LowerHeightAction(this));
    mActions.push_back(new SetHeightAction(this));
    mActions.push_back(new SetEmptyAction(this));
    mActions.push_back(new ClearEmptyAction(this));
    mActions.push_back(new ScaleHeightAction(this));
    mActions.push_back(new BrushAdjustHeightAction(this));
    mActions.push_back(new AdjustHeightAction(this));
    mActions.push_back(new FlattenHeightAction(this));
    mActions.push_back(new SmoothHeightAction(this));
    mActions.push_back(new SetMaterialGroupAction(this));
    mActions.push_back(new SetModifiedAction(this));
    mActions.push_back(new ClearModifiedAction(this));

    // set the default action
    mCurrentAction = mActions[0];
    mRenderBrush = mCurrentAction->useMouseBrush();

    // persist data defaults
    mRenderBorder = true;
    mBorderHeight = 10;
    mBorderFillColor.set(0, 255, 0, 20);
    mBorderFrameColor.set(0, 255, 0, 128);
    mBorderLineMode = false;
    mSelectionHidden = false;
    mEnableSoftBrushes = false;
    mRenderVertexSelection = false;
    mProcessUsesBrush = false;
    mCurrentCursor = NULL;
    mCursorVisible = true;

    //
    mAdjustHeightVal = 10;
    mSetHeightVal = 100;
    mScaleVal = 1;
    mSmoothFactor = 0.1f;
    mMaterialGroup = 0;
    mSoftSelectRadius = 50.f;
    mAdjustHeightMouseScale = 0.1f;

    mSoftSelectDefaultFilter = StringTable->insert("1.000000 0.833333 0.666667 0.500000 0.333333 0.166667 0.000000");
    mSoftSelectFilter = mSoftSelectDefaultFilter;;
}

TerrainEditor::~TerrainEditor()
{
    // mouse
    delete mMouseBrush;

    // terrain actions
    U32 i;
    for (i = 0; i < mActions.size(); i++)
        delete mActions[i];

    // undo stuff
    clearUndo(mUndoList);
    clearUndo(mRedoList);
    delete mUndoSel;

    // base material infos
    for (i = 0; i < mBaseMaterialInfos.size(); i++)
        delete mBaseMaterialInfos[i];
}

//------------------------------------------------------------------------------

TerrainAction* TerrainEditor::lookupAction(const char* name)
{
    for (U32 i = 0; i < mActions.size(); i++)
        if (!dStricmp(mActions[i]->getName(), name))
            return(mActions[i]);
    return(0);
}

//------------------------------------------------------------------------------

bool TerrainEditor::onAdd()
{
    if (!Parent::onAdd())
        return(false);

    SimObject* obj = Sim::findObject("EditorArrowCursor");
    if (!obj)
    {
        Con::errorf(ConsoleLogEntry::General, "TerrainEditor::onAdd: failed to load cursor");
        return(false);
    }

    mDefaultCursor = dynamic_cast<GuiCursor*>(obj);

    return(true);
}

//------------------------------------------------------------------------------

void TerrainEditor::onDeleteNotify(SimObject* object)
{
    Parent::onDeleteNotify(object);

    if (mTerrainBlock != dynamic_cast<TerrainBlock*>(object))
        return;

    mTerrainBlock = 0;
}

void TerrainEditor::setCursor(GuiCursor* cursor)
{
    mCurrentCursor = cursor ? cursor : mDefaultCursor;
}


S32 TerrainEditor::getPaintMaterial()
{
    if (!mPaintMaterial)
        return -1;

    return getClientTerrain()->getMaterialAlphaIndex(mPaintMaterial);
}

//------------------------------------------------------------------------------

TerrainBlock* TerrainEditor::getClientTerrain()
{
    // do the client..

    NetConnection* toServer = NetConnection::getConnectionToServer();
    NetConnection* toClient = NetConnection::getLocalClientConnection();

    S32 index = toClient->getGhostIndex(mTerrainBlock);

    return(dynamic_cast<TerrainBlock*>(toServer->resolveGhost(index)));
}

//------------------------------------------------------------------------------

bool TerrainEditor::gridToWorld(const Point2I& gPos, Point3F& wPos)
{
    const MatrixF& mat = mTerrainBlock->getTransform();
    Point3F origin;
    mat.getColumn(3, &origin);

    wPos.x = gPos.x * (float)mTerrainBlock->getSquareSize() + origin.x;
    wPos.y = gPos.y * (float)mTerrainBlock->getSquareSize() + origin.y;
    wPos.z = getGridHeight(gPos);

    return(!(gPos.x >> TerrainBlock::BlockShift || gPos.y >> TerrainBlock::BlockShift));
}

bool TerrainEditor::worldToGrid(const Point3F& wPos, Point2I& gPos)
{
    const MatrixF& mat = mTerrainBlock->getTransform();
    Point3F origin;
    mat.getColumn(3, &origin);
    F32 squareSize = (F32)mTerrainBlock->getSquareSize();
    F32 halfSquareSize = squareSize / 2;

    float x = (wPos.x - origin.x + halfSquareSize) / squareSize;
    float y = (wPos.y - origin.y + halfSquareSize) / squareSize;

    gPos.x = (S32)mFloor(x);
    gPos.y = (S32)mFloor(y);

    return(!(gPos.x >> TerrainBlock::BlockShift || gPos.y >> TerrainBlock::BlockShift));
}

bool TerrainEditor::gridToCenter(const Point2I& gPos, Point2I& cPos)
{
    cPos.x = gPos.x & TerrainBlock::BlockMask;
    cPos.y = gPos.y & TerrainBlock::BlockMask;

    return(!(gPos.x >> TerrainBlock::BlockShift || gPos.y >> TerrainBlock::BlockShift));
}

//------------------------------------------------------------------------------

bool TerrainEditor::getGridInfo(const Point3F& wPos, GridInfo& info)
{
    Point2I gPos;
    worldToGrid(wPos, gPos);
    return getGridInfo(gPos, info);
}

bool TerrainEditor::getGridInfo(const Point2I& gPos, GridInfo& info)
{
    //
    info.mGridPos = gPos;
    info.mMaterial = getGridMaterial(gPos);
    info.mHeight = getGridHeight(gPos);
    info.mWeight = 1.f;
    info.mPrimarySelect = true;
    info.mMaterialChanged = false;

    Point2I cPos;
    gridToCenter(gPos, cPos);

    info.mMaterialGroup = mTerrainBlock->getBaseMaterial(cPos.x, cPos.y);
    mTerrainBlock->getMaterialAlpha(gPos, info.mMaterialAlpha);

    return(!(gPos.x >> TerrainBlock::BlockShift || gPos.y >> TerrainBlock::BlockShift));
}

void TerrainEditor::setGridInfo(const GridInfo& info)
{
    setGridHeight(info.mGridPos, info.mHeight);
    setGridMaterial(info.mGridPos, info.mMaterial);
    setGridMaterialGroup(info.mGridPos, info.mMaterialGroup);
    if (info.mMaterialChanged)
        mTerrainBlock->setMaterialAlpha(info.mGridPos, info.mMaterialAlpha);
}

//------------------------------------------------------------------------------

F32 TerrainEditor::getGridHeight(const Point2I& gPos)
{
    Point2I cPos;
    gridToCenter(gPos, cPos);
    return(fixedToFloat(mTerrainBlock->getHeight(cPos.x, cPos.y)));
}

void TerrainEditor::gridUpdateComplete()
{
    if (mGridUpdateMin.x <= mGridUpdateMax.x)
        mTerrainBlock->updateGrid(mGridUpdateMin, mGridUpdateMax);
    mGridUpdateMin.set(256, 256);
    mGridUpdateMax.set(0, 0);
}

void TerrainEditor::materialUpdateComplete()
{
    if (mGridUpdateMin.x <= mGridUpdateMax.x)
    {
        TerrainBlock* clientTerrain = getClientTerrain();
        clientTerrain->updateGridMaterials(mGridUpdateMin, mGridUpdateMax);
        mTerrainBlock->updateGrid(mGridUpdateMin, mGridUpdateMax);
    }
    mGridUpdateMin.set(256, 256);
    mGridUpdateMax.set(0, 0);
}

void TerrainEditor::setGridHeight(const Point2I& gPos, const F32 height)
{
    Point2I cPos;
    gridToCenter(gPos, cPos);
    if (cPos.x < mGridUpdateMin.x)
        mGridUpdateMin.x = cPos.x;
    if (cPos.y < mGridUpdateMin.y)
        mGridUpdateMin.y = cPos.y;
    if (cPos.x > mGridUpdateMax.x)
        mGridUpdateMax.x = cPos.x;
    if (cPos.y > mGridUpdateMax.y)
        mGridUpdateMax.y = cPos.y;

    mTerrainBlock->setHeight(cPos, height);
}

TerrainBlock::Material TerrainEditor::getGridMaterial(const Point2I& gPos)
{
    Point2I cPos;
    gridToCenter(gPos, cPos);
    return(*mTerrainBlock->getMaterial(cPos.x, cPos.y));
}

void TerrainEditor::setGridMaterial(const Point2I& gPos, const TerrainBlock::Material& material)
{
    Point2I cPos;
    gridToCenter(gPos, cPos);

    // check if empty has been altered...
    TerrainBlock::Material* mat = mTerrainBlock->getMaterial(cPos.x, cPos.y);

    if ((mat->flags & TerrainBlock::Material::Empty) ^ (material.flags & TerrainBlock::Material::Empty))
        mRebuildEmpty = true;

    *mat = material;
}

U8 TerrainEditor::getGridMaterialGroup(const Point2I& gPos)
{
    Point2I cPos;
    gridToCenter(gPos, cPos);

    return(mTerrainBlock->getBaseMaterial(cPos.x, cPos.y));
}

// basematerials are shared through a resource... so work on client object
// so wont need to load textures....

void TerrainEditor::setGridMaterialGroup(const Point2I& gPos, const U8 group)
{
    Point2I cPos;
    gridToCenter(gPos, cPos);

    TerrainBlock* clientTerrain = getClientTerrain();

    clientTerrain->setBaseMaterial(cPos.x, cPos.y, group);
}

//------------------------------------------------------------------------------

bool TerrainEditor::collide(const Gui3DMouseEvent& event, Point3F& pos)
{
    if (!mTerrainBlock)
        return(false);

    // call the terrain block's ray collision routine directly
    Point3F startPnt = event.pos;
    Point3F endPnt = event.pos + event.vec * 1000;
    Point3F tStartPnt, tEndPnt;

    mTerrainBlock->getTransform().mulP(startPnt, &tStartPnt);
    mTerrainBlock->getTransform().mulP(endPnt, &tEndPnt);

    RayInfo ri;
    if (mTerrainBlock->castRayI(tStartPnt, tEndPnt, &ri, true))
    {
        ri.point.interpolate(startPnt, endPnt, ri.t);
        pos = ri.point;
        return(true);
    }
    return(false);
}

//------------------------------------------------------------------------------

void TerrainEditor::updateGuiInfo()
{
    char buf[128];

    // mouse num grids
    // mouse avg height
    // selection num grids
    // selection avg height
    dSprintf(buf, sizeof(buf), "%d %g %d %g",
        mMouseBrush->size(), mMouseBrush->getAvgHeight(),
        mDefaultSel.size(), mDefaultSel.getAvgHeight());
    Con::executef(this, 2, "onGuiUpdate", buf);
}

//------------------------------------------------------------------------------

void TerrainEditor::renderScene(const RectI&)
{
    if (!mTerrainBlock)
        return;

    if (!mSelectionHidden)
        renderSelection(mDefaultSel, ColorF(1, 0, 0), ColorF(0, 1, 0), ColorF(0, 0, 1), ColorF(0, 0, 1), true, false);

    if (mRenderBrush && mMouseBrush->size())
        renderSelection(*mMouseBrush, ColorF(1, 0, 0), ColorF(0, 1, 0), ColorF(0, 0, 1), ColorF(0, 0, 1), false, true);

    if (mRenderBorder)
        renderBorder();
}

//------------------------------------------------------------------------------

void TerrainEditor::renderSelection(const Selection& sel, const ColorF& inColorFull, const ColorF& inColorNone, const ColorF& outColorFull, const ColorF& outColorNone, bool renderFill, bool renderFrame)
{
    Vector<GFXVertexPC> vertexBuffer;
    ColorF color;
    ColorI iColor;

    vertexBuffer.setSize(sel.size() * 5);

    if (mRenderVertexSelection)
    {

        for (U32 i = 0; i < sel.size(); i++)
        {
            Point3F wPos;
            bool center = gridToWorld(sel[i].mGridPos, wPos);

            if (center)
            {
                if (sel[i].mWeight < 0.f || sel[i].mWeight > 1.f)
                    color = inColorFull;
                else
                    color.interpolate(inColorNone, inColorFull, sel[i].mWeight);
            }
            else
            {
                if (sel[i].mWeight < 0.f || sel[i].mWeight > 1.f)
                    color = outColorFull;
                else
                    color.interpolate(outColorFull, outColorNone, sel[i].mWeight);
            }
            //
            iColor = color;

            GFXVertexPC* verts = &(vertexBuffer[i * 5]);

            verts[0].point = wPos + Point3F(-1, -1, 0);
            verts[0].color = iColor;
            verts[1].point = wPos + Point3F(1, -1, 0);
            verts[1].color = iColor;
            verts[2].point = wPos + Point3F(1, 1, 0);
            verts[2].color = iColor;
            verts[3].point = wPos + Point3F(-1, 1, 0);
            verts[3].color = iColor;
            verts[4].point = verts[0].point;
            verts[4].color = iColor;
        }
    }
    else
    {
        // walk the points in the selection
        for (U32 i = 0; i < sel.size(); i++)
        {
            Point2I gPos = sel[i].mGridPos;

            GFXVertexPC* verts = &(vertexBuffer[i * 5]);

            bool center = gridToWorld(gPos, verts[0].point);
            gridToWorld(Point2I(gPos.x + 1, gPos.y), verts[1].point);
            gridToWorld(Point2I(gPos.x + 1, gPos.y + 1), verts[2].point);
            gridToWorld(Point2I(gPos.x, gPos.y + 1), verts[3].point);
            verts[4].point = verts[0].point;

            if (center)
            {
                if (sel[i].mWeight < 0.f || sel[i].mWeight > 1.f)
                    color = inColorFull;
                else
                    color.interpolate(inColorNone, inColorFull, sel[i].mWeight);
            }
            else
            {
                if (sel[i].mWeight < 0.f || sel[i].mWeight > 1.f)
                    color = outColorFull;
                else
                    color.interpolate(outColorFull, outColorNone, sel[i].mWeight);
            }

            iColor = color;

            verts[0].color = iColor;
            verts[1].color = iColor;
            verts[2].color = iColor;
            verts[3].color = iColor;
            verts[4].color = iColor;
        }
    }

    // Render this bad boy, by stuffing everything into a volatile buffer
    // and rendering...
    GFXVertexBufferHandle<GFXVertexPC> selectionVB(GFX, vertexBuffer.size());

    selectionVB.lock(0, vertexBuffer.size());

    // Copy stuff
    dMemcpy((void*)&selectionVB[0], (void*)&vertexBuffer[0], sizeof(GFXVertexPC) * vertexBuffer.size());

    selectionVB.unlock();

    GFX->setBaseRenderState();

    GFX->setAlphaBlendEnable(true);
    GFX->setSrcBlend(GFXBlendSrcAlpha);
    GFX->setDestBlend(GFXBlendDestAlpha);

    GFX->setVertexBuffer(selectionVB);
    GFX->disableShaders();
    GFX->setCullMode(GFXCullNone);

    if (renderFill)
        for (U32 i = 0; i < sel.size(); i++)
            GFX->drawPrimitive(GFXTriangleFan, i * 5, 4);

    GFX->setAlphaBlendEnable(false);

    if (renderFrame)
        for (U32 i = 0; i < sel.size(); i++)
            GFX->drawPrimitive(GFXLineStrip, i * 5, 4);

}

//------------------------------------------------------------------------------

void TerrainEditor::renderBorder()
{
    /*
       glDisable(GL_CULL_FACE);
       glEnable(GL_BLEND);
       glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

       Point2I pos(0,0);
       Point2I dir[4] = {
          Point2I(1,0),
          Point2I(0,1),
          Point2I(-1,0),
          Point2I(0,-1)
       };

       //
       if(mBorderLineMode)
       {
          glColor4ub(mBorderFrameColor.red, mBorderFrameColor.green, mBorderFrameColor.blue, mBorderFrameColor.alpha);
          glBegin(GL_LINE_STRIP);
          for(U32 i = 0; i < 4; i++)
          {
             for(U32 j = 0; j < TerrainBlock::BlockSize; j++)
             {
                Point3F wPos;
                gridToWorld(pos, wPos);
                glVertex3f(wPos.x, wPos.y, wPos.z);
                pos += dir[i];
             }
          }

          Point3F wPos;
          gridToWorld(Point2I(0,0),  wPos);
          glVertex3f(wPos.x, wPos.y, wPos.z);
          glEnd();
       }
       else
       {
          glEnable(GL_DEPTH_TEST);
          GridSquare * gs = mTerrainBlock->findSquare(TerrainBlock::BlockShift, Point2I(0,0));
          F32 height = F32(gs->maxHeight) * 0.03125f + mBorderHeight;

          const MatrixF & mat = mTerrainBlock->getTransform();
          Point3F pos;
          mat.getColumn(3, &pos);

          Point2F min(pos.x, pos.y);
          Point2F max(pos.x + TerrainBlock::BlockSize * mTerrainBlock->getSquareSize(),
                      pos.y + TerrainBlock::BlockSize * mTerrainBlock->getSquareSize());

          ColorI & a = mBorderFillColor;
          ColorI & b = mBorderFrameColor;

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
          glDisable(GL_DEPTH_TEST);
       }

       glDisable(GL_BLEND);
    */
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void TerrainEditor::addUndo(Vector<Selection*>& list, Selection* sel)
{
    AssertFatal(sel != NULL, "TerrainEditor::addUndo - invalid selection");
    list.push_front(sel);
    if (list.size() == mUndoLimit)
    {
        Selection* undo = list[list.size() - 1];
        delete undo;
        list.pop_back();
    }
    setDirty();
}

void TerrainEditor::clearUndo(Vector<Selection*>& list)
{
    for (U32 i = 0; i < list.size(); i++)
        delete list[i];
    list.clear();
}

bool TerrainEditor::processUndo(Vector<Selection*>& src, Vector<Selection*>& dest)
{
    if (!src.size())
        return(false);

    Selection* task = src.front();
    src.pop_front();

    Selection* save = new Selection;
    for (U32 i = 0; i < task->size(); i++)
    {
        GridInfo info;
        getGridInfo((*task)[i].mGridPos, info);
        save->add(info);
        setGridInfo((*task)[i]);
    }
    gridUpdateComplete();

    delete task;
    addUndo(dest, save);

    rebuild();

    return(true);
}

//------------------------------------------------------------------------------
//
void TerrainEditor::rebuild()
{
    // empty
    if (mRebuildEmpty)
    {
        mTerrainBlock->rebuildEmptyFlags();
        mTerrainBlock->packEmptySquares();
        mRebuildEmpty = false;
    }
}

//------------------------------------------------------------------------------

void TerrainEditor::on3DMouseUp(const Gui3DMouseEvent& event)
{
    if (!mTerrainBlock)
        return;

    if (isMouseLocked())
    {
        mouseUnlock();
        mMouseDownSeq++;
        mCurrentAction->process(mMouseBrush, event, false, TerrainAction::End);
        setCursor(0);

        if (mUndoSel->size())
        {
            addUndo(mUndoList, mUndoSel);
            clearUndo(mRedoList);
        }
        else
            delete mUndoSel;

        mUndoSel = 0;
        mInAction = false;

        rebuild();
    }
}

class TerrainProcessActionEvent : public SimEvent
{
    U32 mSequence;
public:
    TerrainProcessActionEvent(U32 seq)
    {
        mSequence = seq;
    }
    void process(SimObject* object)
    {
        ((TerrainEditor*)object)->processActionTick(mSequence);
    }
};

void TerrainEditor::processActionTick(U32 sequence)
{
    if (mMouseDownSeq == sequence)
    {
        Sim::postEvent(this, new TerrainProcessActionEvent(mMouseDownSeq), Sim::getCurrentTime() + 30);
        mCurrentAction->process(mMouseBrush, mLastEvent, false, TerrainAction::Update);
    }
}

void TerrainEditor::on3DMouseDown(const Gui3DMouseEvent& event)
{
    if (!mTerrainBlock)
        return;

    mSelectionLocked = false;

    mouseLock();
    mMouseDownSeq++;
    mUndoSel = new Selection;
    mCurrentAction->process(mMouseBrush, event, true, TerrainAction::Begin);
    // process on ticks - every 30th of a second.
    Sim::postEvent(this, new TerrainProcessActionEvent(mMouseDownSeq), Sim::getCurrentTime() + 30);
}

void TerrainEditor::on3DMouseMove(const Gui3DMouseEvent& event)
{
    if (!mTerrainBlock)
        return;

    Point3F pos;
    if (!collide(event, pos))
    {
        mMouseBrush->reset();
        mCursorVisible = true;
    }
    else
    {
        //
        if (mRenderBrush)
            mCursorVisible = false;
        mMousePos = pos;

        mMouseBrush->setPosition(mMousePos);
    }
}

void TerrainEditor::on3DMouseDragged(const Gui3DMouseEvent& event)
{
    if (!mTerrainBlock)
        return;

    if (!isMouseLocked())
        return;

    Point3F pos;
    if (!mSelectionLocked)
    {
        if (!collide(event, pos))
        {
            mMouseBrush->reset();
            return;
        }
    }

    // check if the mouse has actually moved in grid space
    bool selChanged = false;
    if (!mSelectionLocked)
    {
        Point2I gMouse;
        Point2I gLastMouse;
        worldToGrid(pos, gMouse);
        worldToGrid(mMousePos, gLastMouse);

        //
        mMousePos = pos;
        mMouseBrush->setPosition(mMousePos);

        selChanged = gMouse != gLastMouse;
    }
    if (selChanged)
        mCurrentAction->process(mMouseBrush, event, true, TerrainAction::Update);
}

void TerrainEditor::getCursor(GuiCursor*& cursor, bool& visible, const GuiEvent& event)
{
    event;
    cursor = mCurrentCursor;
    visible = mCursorVisible;
}

//------------------------------------------------------------------------------
// any console function which depends on a terrainBlock attached to the editor
// should call this
bool checkTerrainBlock(TerrainEditor* object, const char* funcName)
{
    if (!object->terrainBlockValid())
    {
        Con::errorf(ConsoleLogEntry::Script, "TerrainEditor::%s: not attached to a terrain block!", funcName);
        return(false);
    }
    return(true);
}

//------------------------------------------------------------------------------
static void findObjectsCallback(SceneObject* obj, void* val)
{
    Vector<SceneObject*>* list = (Vector<SceneObject*>*)val;
    list->push_back(obj);
}

// XA: Methods added for interfacing with the consoleMethods.

void TerrainEditor::attachTerrain(TerrainBlock* terrBlock)
{
    mTerrainBlock = terrBlock;
}

void TerrainEditor::setBrushType(const char* type)
{
    if (!dStricmp(type, "box"))
    {
        delete mMouseBrush;
        mMouseBrush = new BoxBrush(this);
    }
    else if (!dStricmp(type, "ellipse"))
    {
        delete mMouseBrush;
        mMouseBrush = new EllipseBrush(this);
    }
    else if (!dStricmp(type, "selection"))
    {
        delete mMouseBrush;
        mMouseBrush = new SelectionBrush(this);
    }
    else {}
}

void TerrainEditor::setBrushSize(S32 w, S32 h)
{
    mBrushSize.set(w, h);
    mMouseBrush->setSize(mBrushSize);
}

const char* TerrainEditor::getBrushPos()
{
    AssertFatal(mMouseBrush != NULL, "TerrainEditor::getBrushPos: no mouse brush!");

    Point2I pos = mMouseBrush->getPosition();
    char* ret = Con::getReturnBuffer(32);
    dSprintf(ret, sizeof(ret), "%d %d", pos.x, pos.y);
    return(ret);
}

void TerrainEditor::setBrushPos(Point2I pos)
{
    AssertFatal(mMouseBrush != NULL, "TerrainEditor::setBrushPos: no mouse brush!");
    mMouseBrush->setPosition(pos);
}

void TerrainEditor::setAction(const char* action)
{
    for (U32 i = 0; i < mActions.size(); i++)
    {
        if (!dStricmp(mActions[i]->getName(), action))
        {
            mCurrentAction = mActions[i];

            //
            mRenderBrush = mCurrentAction->useMouseBrush();
            return;
        }
    }
}

const char* TerrainEditor::getActionName(U32 index)
{
    if (index >= mActions.size())
        return("");
    return(mActions[index]->getName());
}

const char* TerrainEditor::getCurrentAction()
{
    return(mCurrentAction->getName());
}

S32 TerrainEditor::getNumActions()
{
    return(mActions.size());
}

void TerrainEditor::resetSelWeights(bool clear)
{
    //
    if (!clear)
    {
        for (U32 i = 0; i < mDefaultSel.size(); i++)
        {
            mDefaultSel[i].mPrimarySelect = false;
            mDefaultSel[i].mWeight = 1.f;
        }
        return;
    }

    Selection sel;

    U32 i;
    for (i = 0; i < mDefaultSel.size(); i++)
    {
        if (mDefaultSel[i].mPrimarySelect)
        {
            mDefaultSel[i].mWeight = 1.f;
            sel.add(mDefaultSel[i]);
        }
    }

    mDefaultSel.reset();

    for (i = 0; i < sel.size(); i++)
        mDefaultSel.add(sel[i]);
}

void TerrainEditor::undo()
{
    if (!checkTerrainBlock(this, "undoAction"))
        return;

    processUndo(mUndoList, mRedoList);
}

void TerrainEditor::redo()
{
    if (!checkTerrainBlock(this, "redoAction"))
        return;

    processUndo(mRedoList, mUndoList);
}

void TerrainEditor::clearSelection()
{
    mDefaultSel.reset();
}

void TerrainEditor::processAction(const char* sAction)
{
    if (!checkTerrainBlock(this, "processAction"))
        return;

    TerrainAction* action = mCurrentAction;
    if (sAction != "")
    {
        action = lookupAction(sAction);

        if (!action)
        {
            Con::errorf(ConsoleLogEntry::General, "TerrainEditor::cProcessAction: invalid action name '%s'.", sAction);
            return;
        }
    }

    if (!getCurrentSel()->size() && !mProcessUsesBrush)
        return;

    mUndoSel = new Selection;

    Gui3DMouseEvent event;
    if (mProcessUsesBrush)
        action->process(mMouseBrush, event, true, TerrainAction::Process);
    else
        action->process(getCurrentSel(), event, true, TerrainAction::Process);

    rebuild();

    // check if should delete the undo
    if (mUndoSel->size())
    {
        addUndo(mUndoList, mUndoSel);
        clearUndo(mRedoList);
    }
    else
        delete mUndoSel;

    mUndoSel = 0;
}

void TerrainEditor::buildMaterialMap()
{
    if (!checkTerrainBlock(this, "buildMaterialMap"))
        return;
    mTerrainBlock->buildMaterialMap();
}

S32 TerrainEditor::getNumTextures()
{
    if (!checkTerrainBlock(this, "getNumTextures"))
        return(0);

    // walk all the possible material lists and count them..
    U32 count = 0;
    for (U32 i = 0; i < TerrainBlock::MaterialGroups; i++)
        if (mTerrainBlock->mMaterialFileName[i] &&
            *mTerrainBlock->mMaterialFileName[i])
            count++;
    return count;
}

const char* TerrainEditor::getTextureName(S32 group)
{
    if (!checkTerrainBlock(this, "getTextureName"))
        return("");

    // textures only exist on the client..
    NetConnection* toServer = NetConnection::getConnectionToServer();
    NetConnection* toClient = NetConnection::getLocalClientConnection();

    S32 index = toClient->getGhostIndex(mTerrainBlock);

    TerrainBlock* terrBlock = dynamic_cast<TerrainBlock*>(toServer->resolveGhost(index));
    if (!terrBlock)
        return("");

    // possibly in range?
    if (group < 0 || group >= TerrainBlock::MaterialGroups)
        return("");

    // now find the i-th group
    U32 count = 0;
    bool found = false;
    for (U32 i = 0; !found && (i < TerrainBlock::MaterialGroups); i++)
    {
        // count it
        if (terrBlock->mMaterialFileName[i] &&
            *terrBlock->mMaterialFileName[i])
            count++;

        if ((group + 1) == count)
        {
            group = i;
            found = true;
        }
    }

    if (!found)
        return("");
    return terrBlock->mMaterialFileName[group];
}

void TerrainEditor::markEmptySquares()
{
    if (!checkTerrainBlock(this, "markEmptySquares"))
        return;

    // build a list of all the marked interiors
    Vector<InteriorInstance*> interiors;
    U32 mask = InteriorObjectType;
    gServerContainer.findObjects(mask, findObjectsCallback, &interiors);

    // walk the terrain and empty any grid which clips to an interior
    for (U32 x = 0; x < TerrainBlock::BlockSize; x++)
        for (U32 y = 0; y < TerrainBlock::BlockSize; y++)
        {
            TerrainBlock::Material* material = mTerrainBlock->getMaterial(x, y);
            material->flags |= ~(TerrainBlock::Material::Empty);

            Point3F a, b;
            gridToWorld(Point2I(x, y), a);
            gridToWorld(Point2I(x + 1, y + 1), b);

            Box3F box;
            box.min = a;
            box.max = b;

            box.min.setMin(b);
            box.max.setMax(a);

            const MatrixF& terrOMat = mTerrainBlock->getTransform();
            const MatrixF& terrWMat = mTerrainBlock->getWorldTransform();

            terrWMat.mulP(box.min);
            terrWMat.mulP(box.max);

            for (U32 i = 0; i < interiors.size(); i++)
            {
                MatrixF mat = interiors[i]->getWorldTransform();
                mat.scale(interiors[i]->getScale());
                mat.mul(terrOMat);

                U32 waterMark = FrameAllocator::getWaterMark();
                U16* zoneVector = (U16*)FrameAllocator::alloc(interiors[i]->getDetailLevel(0)->getNumZones());
                U32 numZones = 0;
                interiors[i]->getDetailLevel(0)->scanZones(box, mat,
                    zoneVector, &numZones);
                if (numZones != 0)
                {
                    Con::printf("%d %d", x, y);
                    material->flags |= TerrainBlock::Material::Empty;
                    FrameAllocator::setWaterMark(waterMark);
                    break;
                }
                FrameAllocator::setWaterMark(waterMark);
            }
        }

    // rebuild stuff..
    mTerrainBlock->buildGridMap();
    mTerrainBlock->rebuildEmptyFlags();
    mTerrainBlock->packEmptySquares();
}

void TerrainEditor::clearModifiedFlags()
{
    if (!checkTerrainBlock(this, "clearModifiedFlags"))
        return;

    //
    for (U32 i = 0; i < (TerrainBlock::BlockSize * TerrainBlock::BlockSize); i++)
        mTerrainBlock->materialMap[i].flags &= ~TerrainBlock::Material::Modified;
}

void TerrainEditor::mirrorTerrain(S32 mirrorIndex)
{
    if (!checkTerrainBlock(this, "mirrorTerrain"))
        return;

    TerrainBlock* terrain = mTerrainBlock;
    setDirty();

    //
    enum {
        top = BIT(0),
        bottom = BIT(1),
        left = BIT(2),
        right = BIT(3)
    };

    U32 sides[8] =
    {
       bottom,
       bottom | left,
       left,
       left | top,
       top,
       top | right,
       right,
       bottom | right
    };

    U32 n = TerrainBlock::BlockSize;
    U32 side = sides[mirrorIndex % 8];
    bool diag = mirrorIndex & 0x01;

    Point2I src((side & right) ? (n - 1) : 0, (side & bottom) ? (n - 1) : 0);
    Point2I dest((side & left) ? (n - 1) : 0, (side & top) ? (n - 1) : 0);
    Point2I origSrc(src);
    Point2I origDest(dest);

    // determine the run length
    U32 minStride = ((side & top) || (side & bottom)) ? n : n / 2;
    U32 majStride = ((side & left) || (side & right)) ? n : n / 2;

    Point2I srcStep((side & right) ? -1 : 1, (side & bottom) ? -1 : 1);
    Point2I destStep((side & left) ? -1 : 1, (side & top) ? -1 : 1);

    //
    U16* heights = terrain->getHeightAddress(0, 0);
    U8* baseMaterials = terrain->getBaseMaterialAddress(0, 0);
    TerrainBlock::Material* materials = terrain->getMaterial(0, 0);

    // create an undo selection
    Selection* undo = new Selection;

    // walk through all the positions
    for (U32 i = 0; i < majStride; i++)
    {
        for (U32 j = 0; j < minStride; j++)
        {
            // skip the same position
            if (src != dest)
            {
                U32 si = src.x + (src.y << TerrainBlock::BlockShift);
                U32 di = dest.x + (dest.y << TerrainBlock::BlockShift);

                // add to undo selection
                GridInfo info;
                getGridInfo(dest, info);
                undo->add(info);

                //... copy info... (height, basematerial, material)
                heights[di] = heights[si];
                baseMaterials[di] = baseMaterials[si];
                materials[di] = materials[si];
            }

            // get to the new position
            src.x += srcStep.x;
            diag ? (dest.y += destStep.y) : (dest.x += destStep.x);
        }

        // get the next position for a run
        src.y += srcStep.y;
        diag ? (dest.x += destStep.x) : (dest.y += destStep.y);

        // reset the minor run
        src.x = origSrc.x;
        diag ? (dest.y = origDest.y) : (dest.x = origDest.x);

        // shorten the run length for diag runs
        if (diag)
            minStride--;
    }

    // rebuild stuff..
    terrain->buildGridMap();
    terrain->rebuildEmptyFlags();
    terrain->packEmptySquares();

    // add undo selection to undo list and clear redo
    addUndo(mUndoList, undo);
    clearUndo(mRedoList);
}

void TerrainEditor::popBaseMaterialInfo()
{
    if (!checkTerrainBlock(this, "popMaterialInfo"))
        return;

    if (!mBaseMaterialInfos.size())
        return;

    TerrainBlock* terrain = mTerrainBlock;

    BaseMaterialInfo* info = mBaseMaterialInfos.front();

    // names
    for (U32 i = 0; i < TerrainBlock::MaterialGroups; i++)
        terrain->mMaterialFileName[i] = info->mMaterialNames[i];

    // base materials
    dMemcpy(terrain->mBaseMaterialMap, info->mBaseMaterials,
        TerrainBlock::BlockSize * TerrainBlock::BlockSize);

    // kill it..
    delete info;
    mBaseMaterialInfos.pop_front();

    // rebuild
    terrain->refreshMaterialLists();
    terrain->buildGridMap();
}

void TerrainEditor::pushBaseMaterialInfo()
{
    if (!checkTerrainBlock(this, "pushMaterialInfo"))
        return;

    TerrainBlock* terrain = mTerrainBlock;

    BaseMaterialInfo* info = new BaseMaterialInfo;

    // copy the material list names
    for (U32 i = 0; i < TerrainBlock::MaterialGroups; i++)
        info->mMaterialNames[i] = terrain->mMaterialFileName[i];

    // copy the base materials
    dMemcpy(info->mBaseMaterials, terrain->mBaseMaterialMap,
        TerrainBlock::BlockSize * TerrainBlock::BlockSize);

    mBaseMaterialInfos.push_front(info);
}

void TerrainEditor::setLoneBaseMaterial(const char* materialListBaseName)
{
    if (!checkTerrainBlock(this, "setLoneBaseMaterial"))
        return;

    TerrainBlock* terrain = mTerrainBlock;

    // force the material group
    terrain->mMaterialFileName[0] = StringTable->insert(materialListBaseName);
    dMemset(terrain->getBaseMaterialAddress(0, 0),
        TerrainBlock::BlockSize * TerrainBlock::BlockSize, 0);

    terrain->refreshMaterialLists();
    terrain->buildGridMap();
}

//------------------------------------------------------------------------------

ConsoleMethod(TerrainEditor, attachTerrain, void, 2, 3, "(TerrainBlock terrain)")
{
    TerrainBlock* terrBlock = 0;

    SimSet* missionGroup = dynamic_cast<SimSet*>(Sim::findObject("MissionGroup"));
    if (!missionGroup)
    {
        Con::errorf(ConsoleLogEntry::Script, "TerrainEditor::attach: no mission group found");
        return;
    }

    // attach to first found terrainBlock
    if (argc == 2)
    {
        for (SimSetIterator itr(missionGroup); *itr; ++itr)
        {
            terrBlock = dynamic_cast<TerrainBlock*>(*itr);
            if (terrBlock)
                break;
        }

        if (!terrBlock)
            Con::errorf(ConsoleLogEntry::Script, "TerrainEditor::attach: no TerrainBlock objects found!");
    }
    else  // attach to named object
    {
        terrBlock = dynamic_cast<TerrainBlock*>(Sim::findObject(argv[2]));

        if (!terrBlock)
            Con::errorf(ConsoleLogEntry::Script, "TerrainEditor::attach: failed to attach to object '%s'", argv[2]);
    }

    if (terrBlock && !terrBlock->isServerObject())
    {
        Con::errorf(ConsoleLogEntry::Script, "TerrainEditor::attach: cannot attach to client TerrainBlock");
        terrBlock = 0;
    }

    object->attachTerrain(terrBlock);
}

ConsoleMethod(TerrainEditor, setBrushType, void, 3, 3, "(string type)"
    "One of box, ellipse, selection.")
{
    object->setBrushType(argv[2]);
}

ConsoleMethod(TerrainEditor, setBrushSize, void, 4, 4, "(int w, int h)")
{
    S32 w = dAtoi(argv[2]);
    S32 h = dAtoi(argv[3]);

    //
    if (w < 1 || w > Brush::MaxBrushDim || h < 1 || h > Brush::MaxBrushDim)
    {
        Con::errorf(ConsoleLogEntry::General, "TerrainEditor::cSetBrushSize: invalid brush dimension. [1-%d].", Brush::MaxBrushDim);
        return;
    }

    object->setBrushSize(w, h);
}

ConsoleMethod(TerrainEditor, getBrushPos, const char*, 2, 2, "Returns a Point2I.")
{
    return object->getBrushPos();
}

ConsoleMethod(TerrainEditor, setBrushPos, void, 3, 4, "(int x, int y)")
{
    //
    Point2I pos;
    if (argc == 3)
        dSscanf(argv[2], "%d %d", &pos.x, &pos.y);
    else
    {
        pos.x = dAtoi(argv[2]);
        pos.y = dAtoi(argv[3]);
    }

    object->setBrushPos(pos);
}

ConsoleMethod(TerrainEditor, setAction, void, 3, 3, "(string action_name)")
{
    object->setAction(argv[2]);
}

ConsoleMethod(TerrainEditor, getActionName, const char*, 3, 3, "(int num)")
{
    return (object->getActionName(dAtoi(argv[2])));
}

ConsoleMethod(TerrainEditor, getNumActions, S32, 2, 2, "")
{
    return(object->getNumActions());
}

ConsoleMethod(TerrainEditor, getCurrentAction, const char*, 2, 2, "")
{
    return object->getCurrentAction();
}

ConsoleMethod(TerrainEditor, resetSelWeights, void, 3, 3, "(bool clear)")
{
    object->resetSelWeights(dAtob(argv[2]));
}

ConsoleMethod(TerrainEditor, undo, void, 2, 2, "")
{
    object->undo();
}

ConsoleMethod(TerrainEditor, redo, void, 2, 2, "")
{
    object->redo();
}

ConsoleMethod(TerrainEditor, clearSelection, void, 2, 2, "")
{
    object->clearSelection();
}

ConsoleMethod(TerrainEditor, processAction, void, 2, 3, "(string action=NULL)")
{
    if (argc == 3)
        object->processAction(argv[2]);
    else object->processAction("");
}

ConsoleMethod(TerrainEditor, buildMaterialMap, void, 2, 2, "")
{
    object->buildMaterialMap();
}

ConsoleMethod(TerrainEditor, getNumTextures, S32, 2, 2, "")
{
    return object->getNumTextures();
}

ConsoleMethod(TerrainEditor, getTextureName, const char*, 3, 3, "(int index)")
{
    return object->getTextureName(dAtoi(argv[2]));
}

ConsoleMethod(TerrainEditor, markEmptySquares, void, 2, 2, "")
{
    object->markEmptySquares();
}

ConsoleMethod(TerrainEditor, clearModifiedFlags, void, 2, 2, "")
{
    object->clearModifiedFlags();
}

ConsoleMethod(TerrainEditor, mirrorTerrain, void, 3, 3, "")
{
    object->mirrorTerrain(dAtoi(argv[2]));
}

ConsoleMethod(TerrainEditor, pushBaseMaterialInfo, void, 2, 2, "")
{
    object->pushBaseMaterialInfo();
}

ConsoleMethod(TerrainEditor, popBaseMaterialInfo, void, 2, 2, "")
{
    object->popBaseMaterialInfo();
}

ConsoleMethod(TerrainEditor, setLoneBaseMaterial, void, 3, 3, "(string materialListBaseName)")
{
    object->setLoneBaseMaterial(argv[2]);
}

ConsoleMethod(TerrainEditor, setTerraformOverlay, void, 3, 3, "(bool overlayEnable) - sets the terraformer current heightmap to draw as an overlay over the current terrain.")
{
    // XA: This one needs to be implemented :)
}

ConsoleMethod(TerrainEditor, setTerrainMaterials, void, 3, 3, "(string matList) sets the list of current terrain materials.")
{
    TerrainEditor* tEditor = (TerrainEditor*)object;
    TerrainBlock* terr = tEditor->getTerrainBlock();
    if (!terr)
        return;
    Resource<TerrainFile> file = terr->getFile();
    const char* fileList = argv[2];
    for (U32 i = 0; i < TerrainBlock::MaterialGroups; i++)
    {
        U32 len;
        const char* spos = dStrchr(fileList, '\n');
        if (!spos)
            len = dStrlen(fileList);
        else
            len = spos - fileList;

        if (len)
            file->mMaterialFileName[i] = StringTable->insertn(fileList, len, true);
        else
            file->mMaterialFileName[i] = 0;
        fileList += len;
        if (*fileList)
            fileList++;
    }
    tEditor->getClientTerrain()->buildMaterialMap();
}

ConsoleMethod(TerrainEditor, getTerrainMaterials, const char*, 2, 2, "() gets the list of current terrain materials.")
{
    char* ret = Con::getReturnBuffer(4096);
    TerrainEditor* tEditor = (TerrainEditor*)object;
    TerrainBlock* terr = tEditor->getTerrainBlock();
    if (!terr)
        return "";
    ret[0] = 0;
    Resource<TerrainFile> file = terr->getFile();
    for (U32 i = 0; i < TerrainBlock::MaterialGroups; i++)
    {
        if (file->mMaterialFileName[i])
            dStrcat(ret, file->mMaterialFileName[i]);
        dStrcat(ret, "\n");
    }
    return ret;
}

//------------------------------------------------------------------------------

void TerrainEditor::initPersistFields()
{
    Parent::initPersistFields();
    addGroup("Misc");
    addField("isDirty", TypeBool, Offset(mIsDirty, TerrainEditor));
    addField("isMissionDirty", TypeBool, Offset(mIsMissionDirty, TerrainEditor));
    addField("renderBorder", TypeBool, Offset(mRenderBorder, TerrainEditor));
    addField("borderHeight", TypeF32, Offset(mBorderHeight, TerrainEditor));
    addField("borderFillColor", TypeColorI, Offset(mBorderFillColor, TerrainEditor));
    addField("borderFrameColor", TypeColorI, Offset(mBorderFrameColor, TerrainEditor));
    addField("borderLineMode", TypeBool, Offset(mBorderLineMode, TerrainEditor));
    addField("selectionHidden", TypeBool, Offset(mSelectionHidden, TerrainEditor));
    addField("enableSoftBrushes", TypeBool, Offset(mEnableSoftBrushes, TerrainEditor));
    addField("renderVertexSelection", TypeBool, Offset(mRenderVertexSelection, TerrainEditor));
    addField("processUsesBrush", TypeBool, Offset(mProcessUsesBrush, TerrainEditor));

    // action values...
    addField("adjustHeightVal", TypeF32, Offset(mAdjustHeightVal, TerrainEditor));
    addField("setHeightVal", TypeF32, Offset(mSetHeightVal, TerrainEditor));
    addField("scaleVal", TypeF32, Offset(mScaleVal, TerrainEditor));
    addField("smoothFactor", TypeF32, Offset(mSmoothFactor, TerrainEditor));
    addField("materialGroup", TypeS32, Offset(mMaterialGroup, TerrainEditor));
    addField("softSelectRadius", TypeF32, Offset(mSoftSelectRadius, TerrainEditor));
    addField("softSelectFilter", TypeString, Offset(mSoftSelectFilter, TerrainEditor));
    addField("softSelectDefaultFilter", TypeString, Offset(mSoftSelectDefaultFilter, TerrainEditor));
    addField("adjustHeightMouseScale", TypeF32, Offset(mAdjustHeightMouseScale, TerrainEditor));
    addField("paintMaterial", TypeCaseString, Offset(mPaintMaterial, TerrainEditor));
    endGroup("Misc");
}
