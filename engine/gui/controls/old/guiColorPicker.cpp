//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "console/console.h"
#include "gfx/gfxDevice.h"
#include "console/consoleTypes.h"
#include "platform/platformAudio.h"
#include "gui/core/guiCanvas.h"
#include "gui/controls/guiButtonCtrl.h"
#include "gui/core/guiDefaultControlRender.h"
#include "gui/controls/guiColorPicker.h"
#include "gfx/primBuilder.h"

/// @name Common colors we use
/// @{
ColorF colorWhite(1.,1.,1.);
ColorF colorWhiteBlend(1.,1.,1.,.75);
ColorF colorBlack(.0,.0,.0);

ColorI GuiColorPickerCtrl::mColorRange[7] = {
	ColorI(255,0,0),     // Red
	ColorI(255,0,255),   // Pink
	ColorI(0,0,255),     // Blue
	ColorI(0,255,255),   // Light blue
	ColorI(0,255,0),     // Green
	ColorI(255,255,0),   // Yellow
	ColorI(255,0,0),     // Red
};
/// @}

IMPLEMENT_CONOBJECT(GuiColorPickerCtrl);

//--------------------------------------------------------------------------
GuiColorPickerCtrl::GuiColorPickerCtrl()
{
   mBounds.extent.set(140, 30);
   mDisplayMode = pPallet;
   mBaseColor = ColorF(1.,.0,1.);
   mPickColor = ColorF(.0,.0,.0);
   mSelectorPos = Point2I(0,0);
   mMouseDown = mMouseOver = false;
   mActive = true;
   mPositionChanged = false;
   mSelectorGap = 1;
   mActionOnMove = false;
}

//--------------------------------------------------------------------------
static EnumTable::Enums gColorPickerModeEnums[] =
{
   { GuiColorPickerCtrl::pPallet,		"Pallet"   },
   { GuiColorPickerCtrl::pHorizColorRange,	"HorizColor"},
   { GuiColorPickerCtrl::pVertColorRange,	"VertColor" },
   { GuiColorPickerCtrl::pBlendColorRange,	"BlendColor"},
   { GuiColorPickerCtrl::pHorizAlphaRange,	"HorizAlpha"},
   { GuiColorPickerCtrl::pVertAlphaRange,	"VertAlpha" },
   { GuiColorPickerCtrl::pDropperBackground,	"Dropper" },
};

static EnumTable gColorPickerModeTable( 7, gColorPickerModeEnums );

//--------------------------------------------------------------------------
void GuiColorPickerCtrl::initPersistFields()
{
   Parent::initPersistFields();
   
   addGroup("ColorPicker");
   addField("BaseColor", TypeColorF, Offset(mBaseColor, GuiColorPickerCtrl));
   addField("PickColor", TypeColorF, Offset(mPickColor, GuiColorPickerCtrl));
   addField("SelectorGap", TypeS32,  Offset(mSelectorGap, GuiColorPickerCtrl)); 
   addField("DisplayMode", TypeEnum, Offset(mDisplayMode, GuiColorPickerCtrl), 1, &gColorPickerModeTable );
   addField("ActionOnMove", TypeBool,Offset(mActionOnMove, GuiColorPickerCtrl));
   endGroup("ColorPicker");
}

//--------------------------------------------------------------------------
// Function to draw a box which can have 4 different colors in each corner blended together
void drawBlendBox(RectI &bounds, ColorF &c1, ColorF &c2, ColorF &c3, ColorF &c4)
{
   S32 l = bounds.point.x, r = bounds.point.x + bounds.extent.x - 1;
   S32 t = bounds.point.y, b = bounds.point.y + bounds.extent.y - 1;
   
   GFX->setAlphaBlendEnable(true);
   GFX->setSrcBlend( GFXBlendSrcAlpha );
   GFX->setDestBlend( GFXBlendInvSrcAlpha );
   GFX->setTextureStageColorOp( 0, GFXTOPDisable );  

   PrimBuild::begin( GFXTriangleFan, 4 );
      PrimBuild::color( c1 );
      PrimBuild::vertex2f( l, t );

      PrimBuild::color( c2 );
      PrimBuild::vertex2f( r, t );

      PrimBuild::color( c3 );
      PrimBuild::vertex2f( r, b );

      PrimBuild::color( c4 );
      PrimBuild::vertex2f( l, b );

   PrimBuild::end();
      
   /*
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glDisable(GL_TEXTURE_2D);

   glBegin(GL_QUADS);
      glColor4f(c1.red, c1.green, c1.blue, c1.alpha);
      glVertex2f(l, t);
      glColor4f(c2.red, c2.green, c2.blue, c2.alpha);
      glVertex2f(r, t);
      glColor4f(c3.red, c3.green, c3.blue, c3.alpha);
      glVertex2f(r, b);
      glColor4f(c4.red, c4.green, c4.blue, c4.alpha);
      glVertex2f(l, b);
   glEnd();
*/
}

//--------------------------------------------------------------------------
/// Function to draw a set of boxes blending throughout an array of colors
void drawBlendRangeBox(RectI &bounds, bool vertical, U8 numColors, ColorI *colors)
{
   S32 l = bounds.point.x, r = bounds.point.x + bounds.extent.x - 1;
   S32 t = bounds.point.y, b = bounds.point.y + bounds.extent.y - 1;
   
   // Calculate increment value
   S32 x_inc = int(mFloor((r - l) / (numColors-1)));
   S32 y_inc = int(mFloor((b - t) / (numColors-1)));
   
   GFX->setAlphaBlendEnable(true);
   GFX->setSrcBlend( GFXBlendSrcAlpha );
   GFX->setDestBlend( GFXBlendInvSrcAlpha );
   GFX->setTextureStageColorOp( 0, GFXTOPDisable );  


   // This code needs to be converted to not use quads... or we need to add
   // quad list support to GFX if D3D supports it...

/* 
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glDisable(GL_TEXTURE_2D);

   glBegin(GL_QUADS);
      for (U16 i=0;i<numColors-1;i++) 
      {
         // If we are at the end, x_inc and y_inc need to go to the end (otherwise there is a rendering bug)
         if (i == numColors-2) 
         {
            x_inc += r-l-1;
            y_inc += b-t-1;
         }
         if (!vertical)  // Horizontal (+x)
         {
            // First color
            glColor4ub(colors[i].red, colors[i].green, colors[i].blue, colors[i].alpha);
            glVertex2f(l, t);
            glVertex2f(l, b);
            // Second color
            glColor4ub(colors[i+1].red, colors[i+1].green, colors[i+1].blue, colors[i+1].alpha);
            glVertex2f(l+x_inc, b);
            glVertex2f(l+x_inc, t);
            l += x_inc;
         }
         else  // Vertical (+y)
         {
            // First color
            glColor4ub(colors[i].red, colors[i].green, colors[i].blue, colors[i].alpha);
            glVertex2f(l, t);
            glVertex2f(r, t);
            // Second color
            glColor4ub(colors[i+1].red, colors[i+1].green, colors[i+1].blue, colors[i+1].alpha);
            glVertex2f(r, t+y_inc);
            glVertex2f(l, t+y_inc);
            t += y_inc;
        }
     }
   glEnd();
*/
}

void GuiColorPickerCtrl::drawSelector(RectI &bounds, Point2I &selectorPos, SelectorMode mode)
{
	U16 sMax = mSelectorGap*2;
	switch (mode)
	{
		case sVertical:
			// Now draw the vertical selector
			// Up -> Pos
			if (selectorPos.y != bounds.point.y+1)
				dglDrawLine(selectorPos.x, bounds.point.y, selectorPos.x, selectorPos.y-sMax-1, colorWhiteBlend);
			// Down -> Pos
			if (selectorPos.y != bounds.point.y+bounds.extent.y) 
				dglDrawLine(selectorPos.x,	selectorPos.y + sMax, selectorPos.x, bounds.point.y + bounds.extent.y, colorWhiteBlend);
		break;
		case sHorizontal:
			// Now draw the horizontal selector
			// Left -> Pos
			if (selectorPos.x != bounds.point.x) 
//            dglDrawLine(bounds.point.x, selectorPos.y-1, selectorPos.x-sMax, selectorPos.y-1, colorWhiteBlend);
			// Right -> Pos
			if (selectorPos.x != bounds.point.x) 
//            dglDrawLine(bounds.point.x+mSelectorPos.x+sMax, selectorPos.y-1, bounds.point.x + bounds.extent.x, selectorPos.y-1, colorWhiteBlend);
		break;
	}
}

//--------------------------------------------------------------------------
/// Function to invoke calls to draw the picker box and selector
void GuiColorPickerCtrl::renderColorBox(RectI &bounds)
{
   RectI pickerBounds;
   pickerBounds.point.x = bounds.point.x+1;
   pickerBounds.point.y = bounds.point.y+1;
   pickerBounds.extent.x = bounds.extent.x-1;
   pickerBounds.extent.y = bounds.extent.y-1;
   
   if (mProfile->mBorder)
   {
      dglDrawRect(bounds, mProfile->mBorderColor);
   }
      
   Point2I selectorPos = Point2I(bounds.point.x+mSelectorPos.x+1, bounds.point.y+mSelectorPos.y+1);

   // Draw color box differently depending on mode
   switch (mDisplayMode)
   {
   case pHorizColorRange:
      dglDrawBlendRangeBox( pickerBounds, false, 7, mColorRange);
      drawSelector( pickerBounds, selectorPos, sVertical );
   break;
   case pVertColorRange:
      dglDrawBlendRangeBox( pickerBounds, true, 7, mColorRange);
      drawSelector( pickerBounds, selectorPos, sHorizontal );
   break;
   case pHorizAlphaRange:
      dglDrawBlendBox( pickerBounds, colorBlack, colorWhite, colorWhite, colorBlack );
      drawSelector( pickerBounds, selectorPos, sVertical );
   break;
   case pVertAlphaRange:
      dglDrawBlendBox( pickerBounds, colorBlack, colorBlack, colorWhite, colorWhite );
      drawSelector( pickerBounds, selectorPos, sHorizontal ); 
   break;
   case pBlendColorRange:
      dglDrawBlendBox( pickerBounds, colorWhite, mBaseColor, colorBlack, colorBlack );
      drawSelector( pickerBounds, selectorPos, sHorizontal );      
      drawSelector( pickerBounds, selectorPos, sVertical );
   break;
   case pDropperBackground:
   break;
   case pPallet:
   default:
      dglDrawRectFill( pickerBounds, mBaseColor );
   break;
   }
}

void GuiColorPickerCtrl::onRender(Point2I offset, const RectI& updateRect)
{
   RectI boundsRect(offset, mBounds.extent); 
   renderColorBox(boundsRect);

   if (mPositionChanged) 
   {
      mPositionChanged = false;
      Point2I extent = Canvas->getExtent();
      // If we are anything but a pallete, change the pick color
      if (mDisplayMode != pPallet) 
      {
         // To simplify things a bit, just read the color via glReadPixels()
         GLfloat rBuffer[4];
         glReadBuffer(GL_BACK);

         U32 buf_x = offset.x+mSelectorPos.x+1;
         U32 buf_y = extent.y-(offset.y+mSelectorPos.y+1);
         glReadPixels(buf_x, buf_y, 1, 1, GL_RGBA, GL_FLOAT, rBuffer);

         mPickColor.red = rBuffer[0];
         mPickColor.green = rBuffer[1];
         mPickColor.blue = rBuffer[2];
         mPickColor.alpha = rBuffer[3]; 

         // Now do onAction() if we are allowed
         if (mActionOnMove) 
            onAction();
      }
   }
   
   //render the children
   renderChildControls( offset, updateRect);
}

//--------------------------------------------------------------------------
void GuiColorPickerCtrl::setSelectorPos(const Point2I &pos)
{
   Point2I extent = mBounds.extent;
   RectI rect;
   if (mDisplayMode != pDropperBackground) 
   {
      extent.x -= 3;
      extent.y -= 2;
      rect = RectI(Point2I(1,1), extent);
   }
   else
   {
      rect = RectI(Point2I(0,0), extent);
   }
   
   if (rect.pointInRect(pos)) 
   {
      mSelectorPos = pos;
      mPositionChanged = true;
      // We now need to update
      setUpdate();
   }
}

//--------------------------------------------------------------------------
void GuiColorPickerCtrl::onMouseDown(const GuiEvent &event)
{
   if (!mActive)
      return;
   
   if (mDisplayMode == pDropperBackground)
      return;
   
   if (mProfile->mCanKeyFocus)
      setFirstResponder();

   // Update the picker cross position
   if (mDisplayMode != pPallet)
      setSelectorPos(globalToLocalCoord(event.mousePoint)); 
   
   mMouseDown = true;
}

//--------------------------------------------------------------------------
void GuiColorPickerCtrl::onMouseDragged(const GuiEvent &event)
{
   if ((mActive && mMouseDown) || (mActive && (mDisplayMode == pDropperBackground)))
   {
      // Update the picker cross position
      if (mDisplayMode != pPallet)
         setSelectorPos(globalToLocalCoord(event.mousePoint));
   }
}

//--------------------------------------------------------------------------
void GuiColorPickerCtrl::onMouseMove(const GuiEvent &event)
{
   // Only for dropper mode
   if (mActive && (mDisplayMode == pDropperBackground))
      setSelectorPos(globalToLocalCoord(event.mousePoint));
}

//--------------------------------------------------------------------------
void GuiColorPickerCtrl::onMouseEnter(const GuiEvent &event)
{
   mMouseOver = true;
}

//--------------------------------------------------------------------------
void GuiColorPickerCtrl::onMouseLeave(const GuiEvent &)
{
   // Reset state
   mMouseOver = false;
   mMouseDown = false;
}

//--------------------------------------------------------------------------
void GuiColorPickerCtrl::onMouseUp(const GuiEvent &)
{
   //if we released the mouse within this control, perform the action
   if (mActive && mMouseDown && (mDisplayMode != pDropperBackground)) 
   {
      onAction();
      mMouseDown = false;
   }
   else if (mActive && (mDisplayMode == pDropperBackground)) 
   {
      // In a dropper, the alt command executes the mouse up action (to signal stopping)
      if ( mAltConsoleCommand[0] )
         Con::evaluate( mAltConsoleCommand, false );
   }
}

//--------------------------------------------------------------------------
const char *GuiColorPickerCtrl::getScriptValue()
{
   static char temp[256];
   ColorF color = getValue();
   dSprintf(temp,256,"%f %f %f %f",color.red, color.green, color.blue, color.alpha);
   return temp; 
}

//--------------------------------------------------------------------------    
void GuiColorPickerCtrl::setScriptValue(const char *value)
{
   ColorF newValue;
   dSscanf(value, "%f %f %f %f", &newValue.red, &newValue.green, &newValue.blue, &newValue.alpha);
   setValue(newValue);
}

ConsoleMethod(GuiColorPickerCtrl, getSelectorPos, const char*, 2, 2, "Gets the current position of the selector")
{
   char *temp = Con::getReturnBuffer(256);
   Point2I pos;
   pos = object->getSelectorPos();
   dSprintf(temp,256,"%d %d",pos.x, pos.y); 
   return temp;
}

ConsoleMethod(GuiColorPickerCtrl, setSelectorPos, void, 3, 3, "Sets the current position of the selector")
{
   Point2I newPos;
   dSscanf(argv[2], "%d %d", &newPos.x, &newPos.y);
   object->setSelectorPos(newPos);
}

ConsoleMethod(GuiColorPickerCtrl, updateColor, void, 2, 2, "Forces update of pick color")
{
	object->updateColor();
}
