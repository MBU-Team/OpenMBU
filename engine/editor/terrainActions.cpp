//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "editor/terrainActions.h"
#include "platform/event.h"
#include "gui/core/guiCanvas.h"

//------------------------------------------------------------------------------

void SelectAction::process(Selection * sel, const Gui3DMouseEvent & event, bool selChanged, Type type)
{
   if(sel == mTerrainEditor->getCurrentSel())
      return;

   if(type == Process)
      return;

   if(selChanged)
   {
      if(event.modifier & SI_CTRL)
      {
         for(U32 i = 0; i < sel->size(); i++)
            mTerrainEditor->getCurrentSel()->remove((*sel)[i]);
      }
      else
      {
         for(U32 i = 0; i < sel->size(); i++)
         {
            GridInfo gInfo;
            if(mTerrainEditor->getCurrentSel()->getInfo((*sel)[i].mGridPos, gInfo))
            {
               if(!gInfo.mPrimarySelect)
                  gInfo.mPrimarySelect = (*sel)[i].mPrimarySelect;

               if(gInfo.mWeight < (*sel)[i].mWeight)
                  gInfo.mWeight = (*sel)[i].mWeight;

               mTerrainEditor->getCurrentSel()->setInfo(gInfo);
            }
            else
               mTerrainEditor->getCurrentSel()->add((*sel)[i]);
         }
      }
   }
}

//------------------------------------------------------------------------------

void SoftSelectAction::process(Selection * sel, const Gui3DMouseEvent &, bool selChanged, Type type)
{
   // allow process of current selection
   Selection tmpSel;
   if(sel == mTerrainEditor->getCurrentSel())
   {
      tmpSel = *sel;
      sel = &tmpSel;
   }

   if(type == Begin || type == Process)
      mFilter.set(1, &mTerrainEditor->mSoftSelectFilter);

   //
   if(selChanged)
   {
      F32 radius = mTerrainEditor->mSoftSelectRadius;
      if(radius == 0.f)
         return;

      S32 squareSize = mTerrainEditor->getTerrainBlock()->getSquareSize();
      U32 offset = U32(radius / F32(squareSize)) + 1;

      for(U32 i = 0; i < sel->size(); i++)
      {
         GridInfo & info = (*sel)[i];

         info.mPrimarySelect = true;
         info.mWeight = mFilter.getValue(0);

         if(!mTerrainEditor->getCurrentSel()->add(info))
            mTerrainEditor->getCurrentSel()->setInfo(info);

         Point2F infoPos(info.mGridPos.x, info.mGridPos.y);

         //
         for(S32 x = info.mGridPos.x - offset; x < info.mGridPos.x + (offset << 1); x++)
            for(S32 y = info.mGridPos.y - offset; y < info.mGridPos.y + (offset << 1); y++)
            {
               //
               Point2F pos(x, y);

               F32 dist = Point2F(pos - infoPos).len() * F32(squareSize);

               if(dist > radius)
                  continue;

               F32 weight = mFilter.getValue(dist / radius);

               //
               GridInfo gInfo;
               if(mTerrainEditor->getCurrentSel()->getInfo(Point2I(x, y), gInfo))
               {
                  if(gInfo.mPrimarySelect)
                     continue;

                  if(gInfo.mWeight < weight)
                  {
                     gInfo.mWeight = weight;
                     mTerrainEditor->getCurrentSel()->setInfo(gInfo);
                  }
               }
               else
               {
                  mTerrainEditor->getGridInfo(Point2I(x, y), gInfo);
                  gInfo.mWeight = weight;
                  gInfo.mPrimarySelect = false;
                  mTerrainEditor->getCurrentSel()->add(gInfo);
               }
            }
      }
   }
}

//------------------------------------------------------------------------------

void OutlineSelectAction::process(Selection * sel, const Gui3DMouseEvent & event, bool, Type type)
{
   sel;event;type;
   switch(type)
   {
      case Begin:
         if(event.modifier & SI_SHIFT)
            break;

         mTerrainEditor->getCurrentSel()->reset();
         break;

      case End:
      case Update:

      default:
         return;
   }

   mLastEvent = event;
}

//------------------------------------------------------------------------------

void PaintMaterialAction::process(Selection * sel, const Gui3DMouseEvent &, bool selChanged, Type)
{
   S32 mat = mTerrainEditor->getPaintMaterial();
   if(selChanged && mat != -1)
   {
      for(U32 i = 0; i < sel->size(); i++)
      {
         GridInfo &inf = (*sel)[i];

         mTerrainEditor->getUndoSel()->add(inf);
         inf.mMaterialChanged = true;

         U32 dAmt = (U32)(inf.mWeight * 255);
         if(inf.mMaterialAlpha[mat] < dAmt)
         {
            inf.mMaterialAlpha[mat] = dAmt;
            U32 total = 0;

            for(S32 i = 0; i < TerrainBlock::MaterialGroups; i++)
            {
               if(i != mat)
                  total += inf.mMaterialAlpha[i];
            }
            if(total != 0)
            {
               // gotta scale them down...

               F32 scaleFactor = (255 - dAmt) / F32(total);
               for(S32 i = 0; i < TerrainBlock::MaterialGroups; i++)
               {
                  if(i != mat)
                     inf.mMaterialAlpha[i] = (U8)(inf.mMaterialAlpha[i] * scaleFactor);
               }
            }
         }
         mTerrainEditor->setGridInfo(inf);
      }
      mTerrainEditor->materialUpdateComplete();
   }
}

//------------------------------------------------------------------------------

void RaiseHeightAction::process(Selection * sel, const Gui3DMouseEvent &, bool selChanged, Type)
{
   // ok the raise height action is our "dirt pour" action
   // only works on brushes...

   Brush *brush = dynamic_cast<Brush *>(sel);
   if(!brush)
      return;
   Point2I brushPos = brush->getPosition();
   Point2I brushSize = brush->getSize();

   GridInfo cur; // the height at the brush position
   mTerrainEditor->getGridInfo(brushPos, cur);

   // we get 30 process actions per second (at least)
   F32 heightAdjust = mTerrainEditor->mAdjustHeightVal / 30;
   // nothing can get higher than the current brush pos adjusted height

   F32 maxHeight = cur.mHeight + heightAdjust;

   for(U32 i = 0; i < sel->size(); i++)
   {
      mTerrainEditor->getUndoSel()->add((*sel)[i]);
      if((*sel)[i].mHeight < maxHeight)
      {
         (*sel)[i].mHeight += heightAdjust * (*sel)[i].mWeight;
         if((*sel)[i].mHeight > maxHeight)
            (*sel)[i].mHeight = maxHeight;
      }
      mTerrainEditor->setGridInfo((*sel)[i]);
   }
   mTerrainEditor->gridUpdateComplete();
}

//------------------------------------------------------------------------------

void LowerHeightAction::process(Selection * sel, const Gui3DMouseEvent &, bool selChanged, Type)
{
   // ok the lower height action is our "dirt dig" action
   // only works on brushes...

   Brush *brush = dynamic_cast<Brush *>(sel);
   if(!brush)
      return;
   Point2I brushPos = brush->getPosition();
   Point2I brushSize = brush->getSize();

   GridInfo cur; // the height at the brush position
   mTerrainEditor->getGridInfo(brushPos, cur);

   // we get 30 process actions per second (at least)
   F32 heightAdjust = -mTerrainEditor->mAdjustHeightVal / 30;
   // nothing can get higher than the current brush pos adjusted height

   F32 maxHeight = cur.mHeight + heightAdjust;
   if(maxHeight < 0)
      maxHeight = 0;

   for(U32 i = 0; i < sel->size(); i++)
   {
      mTerrainEditor->getUndoSel()->add((*sel)[i]);
      if((*sel)[i].mHeight > maxHeight)
      {
         (*sel)[i].mHeight += heightAdjust * (*sel)[i].mWeight;
         if((*sel)[i].mHeight < maxHeight)
            (*sel)[i].mHeight = maxHeight;
      }
      mTerrainEditor->setGridInfo((*sel)[i]);
   }
   mTerrainEditor->gridUpdateComplete();
}

//------------------------------------------------------------------------------

void SetHeightAction::process(Selection * sel, const Gui3DMouseEvent &, bool selChanged, Type)
{
   if(selChanged)
   {
      for(U32 i = 0; i < sel->size(); i++)
      {
         mTerrainEditor->getUndoSel()->add((*sel)[i]);
         (*sel)[i].mHeight = mTerrainEditor->mSetHeightVal;
         mTerrainEditor->setGridInfo((*sel)[i]);
      }
      mTerrainEditor->gridUpdateComplete();
   }
}

//------------------------------------------------------------------------------

void SetEmptyAction::process(Selection * sel, const Gui3DMouseEvent &, bool selChanged, Type)
{
   if(selChanged)
   {
      mTerrainEditor->setMissionDirty();
      for(U32 i = 0; i < sel->size(); i++)
      {
         mTerrainEditor->getUndoSel()->add((*sel)[i]);
         (*sel)[i].mMaterial.flags |= TerrainBlock::Material::Empty;
         mTerrainEditor->setGridInfo((*sel)[i]);
      }
      mTerrainEditor->gridUpdateComplete();
   }
}

//------------------------------------------------------------------------------

void ClearEmptyAction::process(Selection * sel, const Gui3DMouseEvent &, bool selChanged, Type)
{
   if(selChanged)
   {
      mTerrainEditor->setMissionDirty();
      for(U32 i = 0; i < sel->size(); i++)
      {
         mTerrainEditor->getUndoSel()->add((*sel)[i]);
         (*sel)[i].mMaterial.flags &= ~TerrainBlock::Material::Empty;
         mTerrainEditor->setGridInfo((*sel)[i]);
      }
      mTerrainEditor->gridUpdateComplete();
   }
}

//------------------------------------------------------------------------------

void SetModifiedAction::process(Selection * sel, const Gui3DMouseEvent &, bool selChanged, Type)
{
   if(selChanged)
   {
      for(U32 i = 0; i < sel->size(); i++)
      {
         mTerrainEditor->getUndoSel()->add((*sel)[i]);
         (*sel)[i].mMaterial.flags |= TerrainBlock::Material::Modified;
         mTerrainEditor->setGridInfo((*sel)[i]);
      }
      mTerrainEditor->gridUpdateComplete();
   }
}

//------------------------------------------------------------------------------

void ClearModifiedAction::process(Selection * sel, const Gui3DMouseEvent &, bool selChanged, Type)
{
   if(selChanged)
   {
      for(U32 i = 0; i < sel->size(); i++)
      {
         mTerrainEditor->getUndoSel()->add((*sel)[i]);
         (*sel)[i].mMaterial.flags &= ~TerrainBlock::Material::Modified;
         mTerrainEditor->setGridInfo((*sel)[i]);
      }
      mTerrainEditor->gridUpdateComplete();
   }
}

//------------------------------------------------------------------------------

void ScaleHeightAction::process(Selection * sel, const Gui3DMouseEvent &, bool selChanged, Type)
{
   if(selChanged)
   {
      for(U32 i = 0; i < sel->size(); i++)
      {
         mTerrainEditor->getUndoSel()->add((*sel)[i]);
         (*sel)[i].mHeight *= mTerrainEditor->mScaleVal;
         mTerrainEditor->setGridInfo((*sel)[i]);
      }
      mTerrainEditor->gridUpdateComplete();
   }
}

void BrushAdjustHeightAction::process(Selection * sel, const Gui3DMouseEvent & event, bool, Type type)
{
   if(type == Process)
      return;

   //

   if(type == Begin)
   {
      mTerrainEditor->lockSelection(true);

      mFirstPos = mLastPos = event.mousePoint;
      Canvas->mouseLock(mTerrainEditor);

      // the way this works is:
      // construct a plane that goes through the collision point
      // with one axis up the terrain Z, and horizontally parallel to the
      // plane of projection

      // the cross of the camera ffdv and the terrain up vector produces
      // the cross plane vector.

      // all subsequent mouse actions are collided against the plane and the deltaZ
      // from the previous position is used to delta the selection up and down.
      Point3F cameraDir;

      EditTSCtrl::smCamMatrix.getColumn(1, &cameraDir);
      mTerrainEditor->getTerrainBlock()->getTransform().getColumn(2, &mTerrainUpVector);

      // ok, get the cross vector for the plane:
      Point3F planeCross;
      mCross(cameraDir, mTerrainUpVector, planeCross);

      planeCross.normalize();
      Point3F planeNormal;

      Point3F intersectPoint;
      mTerrainEditor->collide(event, intersectPoint);

      mCross(mTerrainUpVector, planeCross, planeNormal);
      mIntersectionPlane.set(intersectPoint, planeNormal);

      // ok, we have the intersection point...
      // project the collision point onto the up vector of the terrain

      mPreviousZ = mDot(mTerrainUpVector, intersectPoint);

      // add to undo
      for(U32 i = 0; i < sel->size(); i++)
      {
         mTerrainEditor->getUndoSel()->add((*sel)[i]);
         (*sel)[i].mStartHeight = (*sel)[i].mHeight;
      }
   }
   else if(type == Update)
   {
      // ok, collide the ray from the event with the intersection plane:

      Point3F intersectPoint;
      Point3F start = event.pos;
      Point3F end = start + event.vec * 1000;

      F32 t = mIntersectionPlane.intersect(start, end);

      m_point3F_interpolate( start, end, t, intersectPoint);
      F32 currentZ = mDot(mTerrainUpVector, intersectPoint);

      F32 diff = currentZ - mPreviousZ;
      //
      //F32 diff = (event.mousePoint.x - mLastPos.x) * mTerrainEditor->mAdjustHeightMouseScale;

      for(U32 i = 0; i < sel->size(); i++)
      {
         (*sel)[i].mHeight = (*sel)[i].mStartHeight + diff * (*sel)[i].mWeight;

         // clamp it
         if((*sel)[i].mHeight < 0.f)
            (*sel)[i].mHeight = 0.f;
         if((*sel)[i].mHeight > 2047.f)
            (*sel)[i].mHeight = 2047.f;

         mTerrainEditor->setGridInfoHeight((*sel)[i]);
      }
      mTerrainEditor->gridUpdateComplete();
      mLastPos = event.mousePoint;
   }
   else if(type == End)
   {
      Canvas->mouseUnlock(mTerrainEditor);
      Canvas->setCursorPos(mFirstPos);
   }
}

//------------------------------------------------------------------------------

AdjustHeightAction::AdjustHeightAction(TerrainEditor * editor) :
   BrushAdjustHeightAction(editor)
{
   mCursor = 0;
}

void AdjustHeightAction::process(Selection *sel, const Gui3DMouseEvent & event, bool b, Type type)
{
   Selection * curSel = mTerrainEditor->getCurrentSel();
   BrushAdjustHeightAction::process(curSel, event, b, type);
}

//------------------------------------------------------------------------------
// flatten the primary selection then blend in the rest...

void FlattenHeightAction::process(Selection * sel, const Gui3DMouseEvent &, bool selChanged, Type)
{
   if(!sel->size())
      return;

   if(selChanged)
   {
      F32 average = 0.f;

      // get the average height
      U32 cPrimary = 0;
      for(U32 k = 0; k < sel->size(); k++)
         if((*sel)[k].mPrimarySelect)
         {
            cPrimary++;
            average += (*sel)[k].mHeight;
         }

      average /= cPrimary;

      // set it
      for(U32 i = 0; i < sel->size(); i++)
      {
         mTerrainEditor->getUndoSel()->add((*sel)[i]);

         //
         if((*sel)[i].mPrimarySelect)
            (*sel)[i].mHeight = average;
         else
         {
            F32 h = average - (*sel)[i].mHeight;
            (*sel)[i].mHeight += (h * (*sel)[i].mWeight);
         }

         mTerrainEditor->setGridInfo((*sel)[i]);
      }
      mTerrainEditor->gridUpdateComplete();
   }
}

//------------------------------------------------------------------------------

void SmoothHeightAction::process(Selection * sel, const Gui3DMouseEvent &, bool selChanged, Type)
{
   if(!sel->size())
      return;

   if(selChanged)
   {
      F32 avgHeight = 0.f;
      for(U32 k = 0; k < sel->size(); k++)
      {
         mTerrainEditor->getUndoSel()->add((*sel)[k]);
         avgHeight += (*sel)[k].mHeight;
      }

      avgHeight /= sel->size();

      // clamp the terrain smooth factor...
      if(mTerrainEditor->mSmoothFactor < 0.f)
         mTerrainEditor->mSmoothFactor = 0.f;
      if(mTerrainEditor->mSmoothFactor > 1.f)
         mTerrainEditor->mSmoothFactor = 1.f;

      // linear
      for(U32 i = 0; i < sel->size(); i++)
      {
         (*sel)[i].mHeight += (avgHeight - (*sel)[i].mHeight) * mTerrainEditor->mSmoothFactor * (*sel)[i].mWeight;
         mTerrainEditor->setGridInfo((*sel)[i]);
      }
      mTerrainEditor->gridUpdateComplete();
   }
}

void SetMaterialGroupAction::process(Selection * sel, const Gui3DMouseEvent &, bool selChanged, Type)
{
   if(selChanged)
   {
      for(U32 i = 0; i < sel->size(); i++)
      {
         mTerrainEditor->getUndoSel()->add((*sel)[i]);

         (*sel)[i].mMaterial.flags |= TerrainBlock::Material::Modified;
         (*sel)[i].mMaterialGroup = mTerrainEditor->mMaterialGroup;
         mTerrainEditor->setGridInfo((*sel)[i]);
      }
      mTerrainEditor->gridUpdateComplete();
   }
}
