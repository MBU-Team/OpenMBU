//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------



#ifndef _X86UNIXMESSAGEBOX_H_
#define _X86UNIXMESSAGEBOX_H_

#include <X11/Xlib.h>
#include "core/tVector.h"

class XMessageBoxButton
{
   public:
      XMessageBoxButton();
      XMessageBoxButton(const char* label, int clickVal);

      const char *getLabel() { return static_cast<const char*>(mLabel); }
      int getClickVal() { return mClickVal; }

      int getLabelWidth() { return mLabelWidth; }
      void setLabelWidth(int width) { mLabelWidth = width; }

      void setButtonRect(int x, int y, int width, int height)
      {
         mX = x;
         mY = y;
         mWidth = width;
         mHeight = height;
      }
      void setMouseCoordinates(int x, int y)
      {
         mMouseX = x;
         mMouseY = y;
      }

      bool drawReverse()
      {
         return mMouseDown && pointInRect(mMouseX, mMouseY);
      }

      bool pointInRect(int x, int y)
      {
         if (x >= mX && x <= (mX+mWidth) &&
            y >= mY && y <= (mY+mHeight))
            return true;
         return false;
      }

      void setMouseDown(bool mouseDown) { mMouseDown = mouseDown; }
      bool isMouseDown() { return mMouseDown; }

   private:
      static const int LabelSize = 100;
      char mLabel[LabelSize];
      int mClickVal;
      int mLabelWidth;
      int mX, mY, mWidth, mHeight;
      int mMouseX, mMouseY;
      bool mMouseDown;
};

class XMessageBox
{
   public:
      static const int OK = 1;
      static const int Cancel = 2;
      static const int Retry = 3;

      XMessageBox(Display* display);
      ~XMessageBox();

      int alertOK(const char *windowTitle, const char *message);
      int alertOKCancel(const char *windowTitle, const char *message);
      int alertRetryCancel(const char *windowTitle, const char *message);
   private:
      int show();
      void repaint();
      void splitMessage();
      void clearMessageLines();
      int loadFont();
      void setDimensions();
      int getButtonLineWidth();

      const char* mMessage;
      const char* mTitle;
      Vector<XMessageBoxButton> mButtons;
      Vector<char*> mMessageLines;

      Display* mDisplay;
      GC mGC;
      Window mWin;
      XFontStruct* mFS;
      int mFontHeight;
      int mFontAscent;
      int mFontDescent;
      int mFontDirection;

      int mScreenWidth, mScreenHeight, mMaxWindowWidth, mMaxWindowHeight;
      int mMBWidth, mMBHeight;
};

#endif // #define _X86UNIXMESSAGEBOX_H_
