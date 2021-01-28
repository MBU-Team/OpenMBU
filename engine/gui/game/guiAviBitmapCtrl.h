//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GUIAVIBITMAPCTRL_H_
#define _GUIAVIBITMAPCTRL_H_

#if !ENABLE_AVI_GUI || !ENABLE_MPG_GUI

class GuiAviBitmapCtrl : public GuiControl
{
  private:
   typedef GuiControl Parent;

  protected:
   bool mDone;

  public:
   DECLARE_CONOBJECT(GuiAviBitmapCtrl);
   GuiAviBitmapCtrl();
   ~GuiAviBitmapCtrl();
   static void initPersistFields();
};

#endif /* No movie control */


#if ENABLE_AVI_GUI

class GuiAviBitmapCtrl : public GuiControl
{
  private:
   typedef GuiControl Parent;

  protected:
   StringTableEntry mAviFilename;
   StringTableEntry mWavFilename;
   U32 mNumTextures;
   TextureHandle *mTextureHandles;
   U32 mWidthCount;
   U32 mHeightCount;
   U32 mBitmapWidth;
   U32 mBitmapAlignedWidth;
   U32 mBitmapHeight;

   PAVIFILE   mPFile;
   PAVISTREAM mPAviVideo;   // video stream to play

   AUDIOHANDLE mWavHandle;      // music to play along with it

   bool mBPlaying;
   bool mDone;
   bool mLetterBox;
   F32 mFrate;
   F32 mSpeed;
   S32 mTimePlayStart;
   S32 mTimePlayStartPos;
   S16 mPlayFPrev;
   S16 mPlayFSkipped;
   S32 mVidsCurrent;        // attempted frame to draw
   S32 mVidsPrevious;       // last successfully decoded frame
   S32 mVidsPrevKey, mVidsNextKey;
   S32 mVidsFirst, mVidsLast;
   S32 mCBVBuf;
   U8 *mPVBuf;

   HIC mHic;
   FOURCC mFccHandler;
   BITMAPINFOHEADER *mPBiSrc;
   BITMAPINFOHEADER *mPBiDst;
   S32 mCBuf;
   U8 *mPBuf;
   bool mSwapRB;
   ALint mAudioLatency;

   S32 fileOpen();
   S32 fileClose();
   S32 movieOpen();
   S32 movieClose();
   S32 vidsVideoOpen();
   S32 vidsVideoClose();
   S32 vidsVideoStart();
   S32 vidsVideoStop();
   S32 vidsVideoDraw();
   S32 vidsTimeToSample(S32 lTime);
   bool vidsIsKey(S32 frame = -1);
   void vidsResetDraw() { mVidsPrevious = -1; }
   bool vidsSync();
   void vidsCatchup();
   S32 vcmOpen(FOURCC fccHandler, BITMAPINFOHEADER *pbiSrc);
   S32 vcmClose();
   S32 vcmBegin();
   S32 vcmEnd();
   S32 vcmDrawStart();
   S32 vcmDrawStop();
   S32 vcmDraw(U64 dwICflags = 0);
   S32 vcmDrawIn(U64 dwICflags = 0);
   bool sndOpen();
   void sndStart();
   void sndStop();
   S32 getMilliseconds();

  public:
   DECLARE_CONOBJECT(GuiAviBitmapCtrl);

   GuiAviBitmapCtrl();
   ~GuiAviBitmapCtrl();

   static void initPersistFields();

   void setFilename(const char *filename);
   S32 movieStart();
   S32 movieStop();

   bool onWake();
   void onSleep();
   void onMouseDown(const GuiEvent&);
   bool onKeyDown(const GuiEvent&);
   void onRender(Point2I offset, const RectI &updateRect);
};

#endif /* ENABLE_AVI_GUI */

#if ENABLE_MPG_GUI

class GuiAviBitmapCtrl : public GuiControl
{
  private:
   typedef GuiControl Parent;

  protected:
   StringTableEntry mAviFilename;
   StringTableEntry mWavFilename;
   U32 mNumTextures;
   TextureHandle *mTextureHandles;
   U32 mWidthCount;
   U32 mHeightCount;
   U32 mBitmapWidth;
   U32 mBitmapAlignedWidth;
   U32 mBitmapHeight;

   SDL_Surface *mSurface;
   U8 *mPBuf;
   SDL_mutex *mDecodeLock;
   ALint mAudioLatency;

   SMPEG *mMPEG;      // video stream to play

   AUDIOHANDLE mWavHandle;      // music to play along with it

   bool mBPlaying;
   bool mDone;
   bool mLetterBox;
   SMPEG_Info mInfo;

   S32 fileOpen();
   S32 fileClose();
   S32 movieOpen();
   S32 movieClose();
   bool sndOpen();
   void sndStart();
   void sndStop();

  public:
   DECLARE_CONOBJECT(GuiAviBitmapCtrl);

   GuiAviBitmapCtrl();
   ~GuiAviBitmapCtrl();

   static void initPersistFields();

   void setFilename(const char *filename);
   S32 movieStart();
   S32 movieStop();

   bool onWake();
   void onSleep();
   void onMouseDown(const GuiEvent&);
   bool onKeyDown(const GuiEvent&);
   void onRender(Point2I offset, const RectI &updateRect);
};

#endif /* ENABLE_MPG_GUI */

#endif /* _GUIAVIBITMAPCTRL_H_ */
