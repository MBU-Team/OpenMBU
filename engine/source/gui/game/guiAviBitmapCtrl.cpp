//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "gui/core/guiControl.h"

// This control currently disabled until streaming is 
// added back into the audio library

#if defined(TORQUE_OS_LINUX) || defined(TORQUE_OS_OPENBSD)
#if DEDICATED
#define  ENABLE_AVI_GUI    0
#define  ENABLE_MPG_GUI    0
#else
#define  ENABLE_AVI_GUI    0
#define  ENABLE_MPG_GUI    0 // was 1
#endif
#else
/* Windows */
#define  ENABLE_AVI_GUI    0
#define  ENABLE_MPG_GUI    0
#endif

#if ENABLE_AVI_GUI || ENABLE_MPG_GUI
#include "audio/audio.h"
#if ENABLE_AVI_GUI
#include <windows.h>
#include <vfw.h>
#endif
#if ENABLE_MPG_GUI
#include "smpeg.h"
#endif
#endif /* Either AVI or MPG GUI */


#include "gui/game/guiAviBitmapCtrl.h"
#include "console/consoleTypes.h"
//#include "dgl/dgl.h"

//----------------------------------------------------------------------------


#if !ENABLE_AVI_GUI || !ENABLE_MPG_GUI
// Version for Loki which will compile (and do nothing)- 
IMPLEMENT_CONOBJECT(GuiAviBitmapCtrl);
GuiAviBitmapCtrl::GuiAviBitmapCtrl()
{
    mDone = true;
}
GuiAviBitmapCtrl::~GuiAviBitmapCtrl()
{
}

void GuiAviBitmapCtrl::initPersistFields()
{
    Parent::initPersistFields();
    addGroup("Misc");		// MM: Added Group Header.
    addField("done", TypeBool, Offset(mDone, GuiAviBitmapCtrl));
    endGroup("Misc");		// MM: Added Group Footer.
}
#else

// Code common to both AVI and MPG players

#define ALIGNULONG(bytes) ((((bytes) + 3) / 4) * 4)

// Error codes
#define MOVERR_OK                0
#define MOVERR_NOVIDEOSTREAM     -1
#define MOVERR_PLAYING           -4


//----------------------------------------------------------------------------
ConsoleMethod(GuiAviBitmapCtrl, setFilename, void, 3, 3, "(string filename)")
{
    object->setFilename(argv[2]);
}

//----------------------------------------------------------------------------
ConsoleMethod(GuiAviBitmapCtrl, play, void, 2, 2, "Start playback.")
{
    object->movieStart();
}

//----------------------------------------------------------------------------
ConsoleMethod(GuiAviBitmapCtrl, stop, void, 2, 2, "Stop playback.")
{
    object->movieStop();
}

//////////////////////////////////////////////////////////////////////////////
// Audio Operations

bool GuiAviBitmapCtrl::sndOpen()
{
    mAudioLatency = 0;

#if 0
    // streaming disabled in this build

    // Open up the audio.  Taking the easy way and using two separate streams.
    // Don't treat as error if it doesn't open- 
    dSprintf(fileBuffer, sizeof(fileBuffer), "%s", mWavFilename);
    Audio::Description desc;
    desc.mIs3D = false;
    desc.mVolume = 1.0f;
    desc.mIsLooping = false;
    desc.mType = Audio::MusicAudioType;
    mWavHandle = alxCreateSource(&desc, fileBuffer, 0);
#endif    

    mWavHandle = NULL_AUDIOHANDLE;
    // alxGetContexti(ALC_BUFFER_LATENCY, &mAudioLatency);

    return (mWavHandle != NULL_AUDIOHANDLE);
}

void GuiAviBitmapCtrl::sndStart()
{
    if (mWavHandle != NULL_AUDIOHANDLE)
        alxPlay(mWavHandle);
}

void GuiAviBitmapCtrl::sndStop()
{
    if (mWavHandle != NULL_AUDIOHANDLE)
    {
        alxStop(mWavHandle);
        mWavHandle = NULL_AUDIOHANDLE;
    }
}


#if ENABLE_AVI_GUI

#define FOURCC_iv50 mmioFOURCC('i','v','5','0')
#define FOURCC_IV50 mmioFOURCC('I','V','5','0')

// Error codes
#define VIDSERR_OK               0
#define VIDSERR_NOVIDEO          -20
#define VIDSERR_VCM              -21
#define VIDSERR_NOTSTARTED       -23
#define VIDSERR_READ             -24

#define VCMERR_OK                0
#define VCMERR_INTERNAL          -1

//----------------------------------------------------------------------------
IMPLEMENT_CONOBJECT(GuiAviBitmapCtrl);

GuiAviBitmapCtrl::GuiAviBitmapCtrl()
{
    mAviFilename = StringTable->insert("");
    mTextureHandles = NULL;

    mPFile = NULL;
    mPAviVideo = NULL;
    mBPlaying = false;
    mDone = false;
    mLetterBox = false;
    mTimePlayStart = -1;
    mPVBuf = NULL;
    mPBuf = NULL;
    mPBiSrc = mPBiDst = NULL;
    mHic = NULL;
    mFccHandler = 0;
    mSwapRB = true;
    mSpeed = 1.0;
    mWavHandle = NULL_AUDIOHANDLE;

    AVIFileInit();
}

//----------------------------------------------------------------------------
GuiAviBitmapCtrl::~GuiAviBitmapCtrl()
{
    vcmClose();
    AVIFileExit();
}

//////////////////////////////////////////////////////////////////////////////
// AVI File Operations

// Open a file
//
S32 GuiAviBitmapCtrl::fileOpen()
{
    S32 rval;
    if (!dStrcmp(mAviFilename, ""))
        return MOVERR_NOVIDEOSTREAM;

    rval = AVIFileOpen(&mPFile, mAviFilename, OF_SHARE_DENY_WRITE, 0);
    if (rval)
    {
        fileClose();
        return rval;
    }

    return MOVERR_OK;
}

// Close a file
//
S32 GuiAviBitmapCtrl::fileClose()
{
    if (mPFile)
    {
        AVIFileRelease(mPFile);
        mPFile = NULL;
    }
    return MOVERR_OK;
}

//////////////////////////////////////////////////////////////////////////////
// Movie Operations

// A movie must have a video stream
// Return codes should be properly done but for now, 0 is no error
// while !0 is error unless otherwise noted

// Open a movie (video stream) from an open file
//
S32 GuiAviBitmapCtrl::movieOpen()
{
    S32 rval;

    // Open first video stream found
    // Note that we don't handle multiple video streams
    rval = AVIFileGetStream(mPFile, &mPAviVideo, streamtypeVIDEO, 0);
    if (rval == AVIERR_NODATA)
    {
        mPAviVideo = NULL;

        return MOVERR_NOVIDEOSTREAM;
    }

    // Get video stream info
    AVISTREAMINFO avis;

    AVIStreamInfo(mPAviVideo, &avis, sizeof(avis));

    mFrate = F32(avis.dwRate) / F32(avis.dwScale);

    // Open vids
    rval = vidsVideoOpen();
    if (rval)
        return rval;

    return MOVERR_OK;
}

// Close movie stream
//
S32 GuiAviBitmapCtrl::movieClose()
{
    // Make sure movie is stopped
    movieStop();

    // Release streams
    if (mPAviVideo)
        AVIStreamRelease(mPAviVideo);

    mPAviVideo = 0;

    return MOVERR_OK;
}

//////////////////////////////////////////////////////////////////////////////
// Video Operations

// Open and init VIDS
//
S32 GuiAviBitmapCtrl::vidsVideoOpen()
{
    if (!mPAviVideo)
        return VIDSERR_NOVIDEO;

    mTimePlayStart = -1;

    // Get stream info
    AVISTREAMINFO avis;

    AVIStreamInfo(mPAviVideo, &avis, sizeof(avis));

    mVidsFirst = mVidsCurrent = avis.dwStart;
    mVidsLast = avis.dwStart + avis.dwLength - 1;

    mVidsPrevious = mVidsCurrent - 1;

    // This was not initialized and causing problems in RTEST- 
    mVidsPrevKey = -100000;

    // Read first frame to get source info
    S32 cb, rval;
    BITMAPINFOHEADER biFormat;

    AVIStreamFormatSize(mPAviVideo, 0, (long*)&cb);
    if (cb != sizeof(BITMAPINFOHEADER))
        return -1;
    rval = AVIStreamReadFormat(mPAviVideo, 0, &biFormat, (long*)&cb);
    rval = rval;

    // Open VCM for this instance
    if (vcmOpen(avis.fccHandler, &biFormat))
    {
        vidsVideoClose();

        return VIDSERR_VCM;
    }

    // Create temporary buffer
    mCBVBuf = biFormat.biHeight * ALIGNULONG(biFormat.biWidth)
        * biFormat.biBitCount / 8;
    mPVBuf = (unsigned char*)dMalloc(mCBVBuf);
    AssertFatal(mPVBuf, "Out of memory");
    dMemset(mPVBuf, 0, mCBVBuf);

    // Reset internal skip count
    mPlayFSkipped = 0;

    return VIDSERR_OK;
}

// Close VIDS
//
S32 GuiAviBitmapCtrl::vidsVideoClose()
{
    if (mPVBuf)
    {
        dFree(mPVBuf);
        mPVBuf = NULL;
    }

    vcmClose();

    return VIDSERR_OK;
}

// Start video, note that all decode/draw is done in vidsDraw()
//
// If fStart < 0, then start on current frame
//
S32 GuiAviBitmapCtrl::vidsVideoStart()
{
    // Begin VCM
    if (vcmBegin())
    {
        vidsVideoStop();

        return VIDSERR_VCM;
    }

    // Pass seperate start message to VCM only when playing
    if (mBPlaying)
        vcmDrawStart();

    // Get start time
    mTimePlayStart = getMilliseconds();
    mTimePlayStartPos = AVIStreamSampleToTime(mPAviVideo, mVidsCurrent);

    // Init play stats
    mPlayFPrev = mVidsCurrent;

    return VIDSERR_OK;
}

// Stop video
//
S32 GuiAviBitmapCtrl::vidsVideoStop()
{
    vcmDrawStop();
    vcmEnd();

    mTimePlayStart = -1;

    return VIDSERR_OK;
}

// Draw current frame of video
//
S32 GuiAviBitmapCtrl::vidsVideoDraw()
{
    // Check if started
    if (mTimePlayStart < 0)
        return VIDSERR_NOTSTARTED;

    if (mBPlaying)
    {
        S32 lTime = mTimePlayStartPos + (getMilliseconds() - mTimePlayStart);

        mVidsCurrent = vidsTimeToSample(lTime);

        if (mVidsCurrent > mVidsLast)
            mVidsCurrent = mVidsLast;

        if (mVidsCurrent == mVidsPrevious)
            // Going too fast!  Should actually return a ms
            //  count so calling app can Sleep() if desired.
            return VIDSERR_OK;
    }
    else
    {
        if (mVidsCurrent > mVidsLast)
            mVidsCurrent = mVidsLast;

        if (mVidsCurrent == mVidsPrevious)
            vidsResetDraw();
    }

    if (!vidsSync())
        return VIDSERR_OK; // don't draw this frame

    S32 rval = AVIStreamRead(mPAviVideo,
        mVidsCurrent,
        1,
        mPVBuf,
        mCBVBuf,
        NULL,
        NULL);
    if (rval)
        return VIDSERR_READ;

    if (vcmDraw())
        return VIDSERR_VCM;

    mVidsPrevious = mVidsCurrent;

    if (mBPlaying)
    {
        mPlayFSkipped += mVidsCurrent - mPlayFPrev - 1;
        mPlayFPrev = mVidsCurrent;

        if (mVidsCurrent == mVidsLast)
        {
            mVidsCurrent = -1;

            return movieStop();
        }
    }

    return VIDSERR_OK;
}

// Convert ms to sample (frame)
//
S32 GuiAviBitmapCtrl::vidsTimeToSample(S32 lTime)
{
    S32 lSamp = AVIStreamTimeToSample(mPAviVideo, lTime);

    return lSamp;
}

// TRUE if frame is KEY, if frame < 0 then check current frame
//
bool GuiAviBitmapCtrl::vidsIsKey(S32 frame /* = -1 */)
{
    if (!mPAviVideo)
        return false;

    if (frame < 0)
        frame = mVidsCurrent;

    return AVIStreamIsKeyFrame(mPAviVideo, frame);
}

//////////////////////////////////////////////////////////////////////////////
// Internal video routines

// Synchronization and Keyframe Management:
//   pretty simple plan, don't do anything too fancy.
//
bool GuiAviBitmapCtrl::vidsSync()
{
#define dist(x,y) ((x)-(y))
#define ABS_(x) (x<0 ? (-(x)) : (x))

    if (mVidsCurrent < mVidsPrevious) // seeked back - reset draw
        mVidsPrevious = -1;

    if (dist(mVidsCurrent, mVidsPrevious) == 1)
    {
        // normal situation
        // fall thru and draw
    }
    else
    {
        // SKIPPED
        if (AVIStreamIsKeyFrame(mPAviVideo, mVidsCurrent))
        {
            // we are on KF boundry just reset and start here
            mVidsPrevKey = mVidsCurrent;
            mVidsNextKey = AVIStreamNextKeyFrame(mPAviVideo, mVidsCurrent);
            // fall thru and draw
        }
        else
        {
            if (dist(mVidsCurrent, mVidsPrevious) == 2)
            {
                // one frame off - just draw
                vidsCatchup();
                // fall thru and draw
            }
            else
            {
                // We are greater than one frame off:
                //  if we went past a K frame, update K frame info then:
                //  if we are closer to previous frame than catchup and draw
                //  if we are closer to next KEY frame than don't draw
                if ((mVidsNextKey < mVidsCurrent) || (mVidsPrevKey > mVidsCurrent))   // seeked past previous key frame
                {
                    // went past a K frame
                    mVidsPrevKey = AVIStreamPrevKeyFrame(mPAviVideo, mVidsCurrent);
                    mVidsNextKey = AVIStreamNextKeyFrame(mPAviVideo, mVidsCurrent);
                }

                if (ABS_(dist(mVidsCurrent, mVidsPrevKey)) <= ABS_(dist(mVidsCurrent, mVidsNextKey)))
                    vidsCatchup();
                // fall thru and draw
                else
                    if (mBPlaying)
                        return false; // m_vidsPrev NOT updated
                    else
                        vidsCatchup();  // if not playing than we want to
                                        // draw the current frame
            }
        }
    }

    return true;
}

// Readies to draw (but doesn't draw) m_vidsCurrent.
//  We just ICDECOMPRESS_HURRYUP frames from m_vidsPrevious or
//   m_vidsPrevKey, whichever is closer.
//  Updates m_vidsPrevious.
//
void GuiAviBitmapCtrl::vidsCatchup()
{
    if (mVidsPrevious < mVidsPrevKey)
        mVidsPrevious = mVidsPrevKey - 1;

    S32 catchup = mVidsPrevious + 1;

    while (catchup < mVidsCurrent)
    {
        S32 rval = AVIStreamRead(mPAviVideo,
            catchup,
            1,
            mPVBuf,
            mCBVBuf,
            NULL,
            NULL);

        if (rval)
            break;

        if (!mBPlaying)
            vcmDrawIn();
        else
            vcmDrawIn(ICDECOMPRESS_HURRYUP);

        mVidsPrevious = catchup++;
    }
}

// Note that between vcmOpen() and vcmClose(), the source information does not
//  change.  If it does, open a new one.
//
S32 GuiAviBitmapCtrl::vcmOpen(FOURCC fccHandler, BITMAPINFOHEADER* pbiSrc)
{
    if (fccHandler == 0)
        mFccHandler = pbiSrc->biCompression;
    else
        mFccHandler = fccHandler;

    if (mFccHandler == FOURCC_IV50)
        mFccHandler = FOURCC_iv50;

    // Open codec
    mHic = ICLocate(ICTYPE_VIDEO, fccHandler, pbiSrc, 0, ICMODE_DECOMPRESS);
    if (!mHic) return AVIERR_NOCOMPRESSOR;

    delete[] mPBiSrc;
    mPBiSrc = (BITMAPINFOHEADER*) new char[sizeof(BITMAPINFOHEADER) +
        256 * sizeof(RGBQUAD)];

    delete[] mPBiDst;
    mPBiDst = (BITMAPINFOHEADER*) new char[sizeof(BITMAPINFOHEADER) +
        256 * sizeof(RGBQUAD)];
    AssertFatal(mPBiSrc && mPBiDst, "Out of memory");

    dMemcpy(mPBiSrc, pbiSrc, sizeof(BITMAPINFOHEADER));

    // Initialize destination bitmap header
    dMemcpy(mPBiDst, mPBiSrc, sizeof(BITMAPINFOHEADER));
    // Default destination bitmap header
    mPBiDst->biBitCount = 24;
    mPBiDst->biCompression = BI_RGB;
    mPBiDst->biSizeImage = mPBiDst->biHeight * ALIGNULONG(mPBiDst->biWidth) * mPBiDst->biBitCount / 8;

    // Create temporary buffer
    mCBuf = mPBiDst->biSizeImage;
    mPBuf = (U8*)dMalloc(mCBuf);
    AssertFatal(mPBuf, "Out of memory");
    dMemset(mPBuf, 0, mCBuf);

    return VCMERR_OK;
}

S32 GuiAviBitmapCtrl::vcmClose()
{
    if (mPBiSrc) delete[] mPBiSrc;
    if (mPBiDst) delete[] mPBiDst;
    mPBiSrc = mPBiDst = NULL;

    if (mPBuf)
    {
        dFree(mPBuf);
        mPBuf = NULL;
    }

    if (mHic)
    {
        ICClose(mHic);
        mHic = NULL;
    }

    mFccHandler = 0;

    return VCMERR_OK;
}

// vcmBegin() and vcmEnd() (de)initializes a series of vcmDraw()'s
// The user must end and restart a sequence when the destination
//  parameters change.
// Note that if the source information changes vcmOpen()/vcmClose()
//  must be used (since fcc might be different).
// fInit initializes the sequence (do the first time and when resetting
//  parameters
//
S32 GuiAviBitmapCtrl::vcmBegin()
{
    S32 rval;

    if (!mHic)
        return VCMERR_INTERNAL;

    rval = ICDecompressExQuery(mHic, 0,
        mPBiSrc, NULL, 0, 0, mBitmapWidth, mBitmapHeight,
        mPBiDst, NULL, 0, 0, mBitmapWidth, mBitmapHeight);

    if (rval) return rval;

    rval = ICDecompressExBegin(mHic, 0,
        mPBiSrc, NULL, 0, 0, mBitmapWidth, mBitmapHeight,
        mPBiDst, NULL, 0, 0, mBitmapWidth, mBitmapHeight);

    if (rval) return rval;

    return AVIERR_OK;
}

S32 GuiAviBitmapCtrl::vcmEnd()
{
    return VCMERR_OK;
}

// vcmDrawStart/vcmDrawStop are not absolutely necessary but some codecs
// use them to do timing (to tell when playing real time)
//
S32 GuiAviBitmapCtrl::vcmDrawStart()
{
    // Send ICM_DRAW_BEGIN.
    // this is only for telling   the codec what our frame rate is - zero out all other members.
    ICDrawBegin(mHic,
        0, 0, 0, 0,
        0, 0, 0, 0,
        NULL,
        0, 0, 0, 0,
        (DWORD)(1.0 / mFrate * 1000.0 * 1000.0), // dwRate
        (DWORD)(1000 * 1000));                   // dwScale

// Send ICM_DRAW_START.
    ICDrawStart(mHic);

    return VCMERR_OK;
}

S32 GuiAviBitmapCtrl::vcmDrawStop()
{
    // Send ICM_DRAW_STOP
    ICDrawStop(mHic);

    // Send ICM_DRAW_END
    ICDrawEnd(mHic);

    return VCMERR_OK;
}

S32 GuiAviBitmapCtrl::vcmDraw(U64 dwICflags)
{
    S32 rval;

    rval = ICDecompressEx(mHic, dwICflags,
        mPBiSrc, mPVBuf, 0, 0, mBitmapWidth, mBitmapHeight,
        mPBiDst, mPBuf, 0, 0, mBitmapWidth, mBitmapHeight);

    if (rval)
        // Normal in case of ICM_HURRYUP flag (rval = 1)
        return rval;

    for (U32 j = 0; j < mHeightCount; j++)
    {
        U32 y = j * 256;
        U32 height = getMin(mBitmapHeight - y, U32(256));

        for (U32 i = 0; i < mWidthCount; i++)
        {
            U32 index = j * mWidthCount + i;
            U32 x = i * 256;
            U32 width = getMin(mBitmapWidth - x, U32(256));
            GBitmap* bmp = mTextureHandles[index].getBitmap();

            for (U32 lp = 0; lp < height; lp++)
            {
                const U8* src = &mPBuf[3 * ((mBitmapHeight - (y + lp + 1)) * mBitmapAlignedWidth + x)];
                U8* dest = bmp->getAddress(0, lp);

                // counting on the artist to switch the R & B channels so we don't have to in runtime
                if (!mSwapRB)
                    dMemcpy(dest, src, width * 3);
                else
                    for (U32 k = 0; k < width; ++k)
                    {
                        *dest++ = src[2];
                        *dest++ = src[1];
                        *dest++ = src[0];
                        src += 3;
                    }
            }

            mTextureHandles[index].refresh();
        }
    }

    return VCMERR_OK;
}

S32 GuiAviBitmapCtrl::vcmDrawIn(U64 dwICflags)
{
    // If we are not displaying frames, IVI still writes to the buffer
    S32 rval = ICDecompressEx(mHic, dwICflags,
        mPBiSrc, mPVBuf, 0, 0, mBitmapWidth, mBitmapHeight,
        mPBiDst, mPBuf, 0, 0, mBitmapWidth, mBitmapHeight);

    if (rval)
        // Normal in case of ICM_HURRYUP flag (rval = 1)
        return rval;

    return VCMERR_OK;
}

void GuiAviBitmapCtrl::initPersistFields()
{
    Parent::initPersistFields();

    addGroup("Media");
    addField("aviFilename", TypeFilename, Offset(mAviFilename, GuiAviBitmapCtrl));
    addField("wavFilename", TypeFilename, Offset(mWavFilename, GuiAviBitmapCtrl));
    endGroup("Media");

    addGroup("Misc");
    addField("swapRB", TypeBool, Offset(mSwapRB, GuiAviBitmapCtrl));
    addField("done", TypeBool, Offset(mDone, GuiAviBitmapCtrl));
    addField("letterBox", TypeBool, Offset(mLetterBox, GuiAviBitmapCtrl));
    addField("speed", TypeF32, Offset(mSpeed, GuiAviBitmapCtrl));
    endGroup("Misc");
}

//----------------------------------------------------------------------------
void GuiAviBitmapCtrl::setFilename(const char* filename)
{
    bool awake = mAwake;

    if (awake)
        onSleep();

    mAviFilename = StringTable->insert(filename);

    if (awake)
        onWake();
}

// Start a movie, i.e. begin play from stopped state
//  or restart from paused state
//
S32 GuiAviBitmapCtrl::movieStart()
{
    if (!mPAviVideo)
        return MOVERR_NOVIDEOSTREAM;

    // Check if starting without stopping
    if (mBPlaying)
        return MOVERR_PLAYING;

    mBPlaying = true;

    sndStart();

    // Start video, note only one state var, play or stop
    vidsVideoStart();
    setUpdate();

    return MOVERR_OK;
}

// Stop playing a movie
//
S32 GuiAviBitmapCtrl::movieStop()
{
    mBPlaying = false;
    vidsVideoStop();

    sndStop();

    // notify the script
    // Con::executef(this,1,"movieStopped");

    mDone = true;

    return MOVERR_OK;
}

//----------------------------------------------------------------------------
bool GuiAviBitmapCtrl::onWake()
{
    if (!Parent::onWake()) return false;

    if (fileOpen() || movieOpen())
    {
        mDone = true;

        // we return TRUE here, or else the damn thing gets deleted and that's
        // just plain bad.

        return true;
    }

    sndOpen();

    mBitmapWidth = mPBiSrc->biWidth;
    mBitmapAlignedWidth = ALIGNULONG(mBitmapWidth);
    mBitmapHeight = mPBiSrc->biHeight;
    mWidthCount = mBitmapWidth / 256;
    mHeightCount = mBitmapHeight / 256;
    if (mBitmapWidth % 256)
        mWidthCount++;
    if (mBitmapHeight % 256)
        mHeightCount++;
    mNumTextures = mWidthCount * mHeightCount;
    mTextureHandles = new TextureHandle[mNumTextures];
    for (U32 j = 0; j < mHeightCount; j++)
    {
        U32 y = j * 256;
        U32 height = getMin(mBitmapWidth - y, U32(256));

        for (U32 i = 0; i < mWidthCount; i++)
        {
            char nameBuffer[64];
            U32 index = j * mWidthCount + i;

            dSprintf(nameBuffer, sizeof(nameBuffer), "%s_#%d_#%d", mAviFilename, i, j);
            mTextureHandles[index] = TextureHandle(nameBuffer, RegisteredTexture, true);
            if (!bool(mTextureHandles[index]))
            {
                U32 x = i * 256;
                U32 width = getMin(mBitmapWidth - x, U32(256));

                const GBitmap* bmp = new GBitmap(width, height, false, GBitmap::RGB);

                mTextureHandles[index] = TextureHandle(nameBuffer, bmp, true);
            }
        }
    }

    return true;
}

//----------------------------------------------------------------------------
void GuiAviBitmapCtrl::onSleep()
{
    movieClose();
    fileClose();

    if (mTextureHandles)
    {
        delete[] mTextureHandles;
        mTextureHandles = NULL;
    }

    Parent::onSleep();
}

//----------------------------------------------------------------------------
void GuiAviBitmapCtrl::onMouseDown(const GuiEvent&)
{
    // end the movie NOW!
    movieStop();
}

//----------------------------------------------------------------------------
bool GuiAviBitmapCtrl::onKeyDown(const GuiEvent&)
{
    // end the movie NOW!
    movieStop();
    return true;
}

//----------------------------------------------------------------------------
// Playing with speed for debugging (glitch shows up when skipping occurs).

S32 GuiAviBitmapCtrl::getMilliseconds()
{
    F32   ms;

    if (mSpeed > 0)
    {
        ms = Platform::getRealMilliseconds();
        ms *= mSpeed;
    }
    else
    {
        // Try to force the glitch (negative speed)- 
        static F32 deterministicClock = 0.0f;
        ms = deterministicClock;
        deterministicClock -= mSpeed;
    }

    return S32(ms);
}

//----------------------------------------------------------------------------
void GuiAviBitmapCtrl::onRender(Point2I offset, const RectI& updateRect)
{
    vidsVideoDraw();

    if (mTextureHandles)
    {
        RectI    displayRect(mBounds);
        S32      verticalDisplace = 0;

        if (mLetterBox)
        {
            // Our supplied picture is 3/4 height 640x360 for 640x480 res.  But let's allow
            // for a regular full-window version since it might come in handy elsewhere (so
            // letterBox is public flag on the GUI).  
            verticalDisplace = (mBounds.extent.y >> 3);
            displayRect.extent.y = (displayRect.extent.y * 3 >> 2);
            displayRect.point.y += verticalDisplace;
            RectI upperRect(offset.x, offset.y, displayRect.extent.x, verticalDisplace + 1);
            GFX->drawRectFill(upperRect, mProfile->mFillColorHL);
        }

        // Scale into the letterbox-
        F32 widthScale = F32(displayRect.extent.x) / F32(mBitmapWidth);
        F32 heightScale = F32(displayRect.extent.y) / F32(mBitmapHeight);

        offset.y += verticalDisplace;

        GFX->setBitmapModulation(ColorF(1, 1, 1));
        for (U32 i = 0; i < mWidthCount; i++)
        {
            for (U32 j = 0; j < mHeightCount; j++)
            {
                TextureHandle t = mTextureHandles[j * mWidthCount + i];
                RectI stretchRegion;

                stretchRegion.point.x = i * 256 * widthScale + offset.x;
                stretchRegion.point.y = j * 256 * heightScale + offset.y;
                stretchRegion.extent.x = (i * 256 + t.getWidth()) * widthScale + offset.x - stretchRegion.point.x;
                stretchRegion.extent.y = (j * 256 + t.getHeight()) * heightScale + offset.y - stretchRegion.point.y;
                dglDrawBitmapStretch(t, stretchRegion);
            }
        }

        if (mLetterBox)
        {
            // For some reason the above loop draws white below, and this rect fill has to
            // come after - got to look at that math...  Also don't know why we need the
            // extra width & height here...
            RectI lowerRect(offset.x, mBounds.extent.y - verticalDisplace - 1,
                mBounds.extent.x + 2, verticalDisplace + 2);
            GFX->drawRectFill(lowerRect, mProfile->mFillColorHL);
        }

        renderChildControls(offset, updateRect);

        if (mBPlaying)
            setUpdate();
    }
    else
        Parent::onRender(offset, updateRect);
}

#endif /* ENABLE_AVI_GUI */


#if ENABLE_MPG_GUI

//----------------------------------------------------------------------------
IMPLEMENT_CONOBJECT(GuiAviBitmapCtrl);

GuiAviBitmapCtrl::GuiAviBitmapCtrl()
{
    mAviFilename = StringTable->insert("");
    mTextureHandles = NULL;

    mMPEG = NULL;
    mBPlaying = false;
    mDone = false;
    mLetterBox = false;
    mSurface = NULL;
    mPBuf = NULL;
    mDecodeLock = NULL;
    mWavHandle = NULL_AUDIOHANDLE;
}

//----------------------------------------------------------------------------
GuiAviBitmapCtrl::~GuiAviBitmapCtrl()
{
}

//////////////////////////////////////////////////////////////////////////////
// MPEG File Operations

// Open a file
//
S32 GuiAviBitmapCtrl::fileOpen()
{
    char fileBuffer[256];

    if (!dStrcmp(mAviFilename, ""))
        return MOVERR_NOVIDEOSTREAM;

    // FixMe: Should convert this to have a dir attribute...
    dSprintf(fileBuffer, sizeof(fileBuffer), "FixMe/textures/%s", mAviFilename);

    // Convert filename from .avi to .mpg
    char* ext;
    ext = dStrstr(static_cast<const char*>(fileBuffer), ".avi");
    if (!ext)
        ext = dStrstr(static_cast<const char*>(fileBuffer), ".AVI");
    if (ext)
        dStrcpy(ext, ".mpg");

    mMPEG = SMPEG_new(fileBuffer, &mInfo, 0);
    if (!mMPEG || (SMPEG_status(mMPEG) == SMPEG_ERROR))
    {
        fileClose();
        return MOVERR_NOVIDEOSTREAM;
    }
    return MOVERR_OK;
}

// Close a file
//
S32 GuiAviBitmapCtrl::fileClose()
{
    if (mMPEG)
    {
        SMPEG_delete(mMPEG);
        mMPEG = NULL;
    }
    return MOVERR_OK;
}

//////////////////////////////////////////////////////////////////////////////
// Movie Operations

// A movie must have a video stream
// Return codes should be properly done but for now, 0 is no error
// while !0 is error unless otherwise noted

// Open a movie (video stream) from an open file
//
S32 GuiAviBitmapCtrl::movieOpen()
{
    // If the file was opened successfully, it's an MPEG video
    return MOVERR_OK;
}

// Close movie stream
//
S32 GuiAviBitmapCtrl::movieClose()
{
    // Make sure movie is stopped
    movieStop();

    return MOVERR_OK;
}


void GuiAviBitmapCtrl::initPersistFields()
{
    Parent::initPersistFields();

    addGroup("Media");
    addField("aviFilename", TypeFilename, Offset(mAviFilename, GuiAviBitmapCtrl));
    addField("wavFilename", TypeFilename, Offset(mWavFilename, GuiAviBitmapCtrl));
    endGroup("Media");

    addGroup("Misc");
    addField("done", TypeBool, Offset(mDone, GuiAviBitmapCtrl));
    addField("letterBox", TypeBool, Offset(mLetterBox, GuiAviBitmapCtrl));
    endGroup("Misc");
}

//----------------------------------------------------------------------------
void GuiAviBitmapCtrl::setFilename(const char* filename)
{
    bool awake = mAwake;

    if (awake)
        onSleep();

    mAviFilename = StringTable->insert(filename);

    if (awake)
        onWake();
}

// Start a movie, i.e. begin play from stopped state
//  or restart from paused state
//
S32 GuiAviBitmapCtrl::movieStart()
{
    if (!mMPEG)
        return MOVERR_NOVIDEOSTREAM;

    // Check if starting without stopping
    if (mBPlaying)
        return MOVERR_PLAYING;

    mBPlaying = true;

    sndStart();

    // Start video, note only one state var, play or stop
    SMPEG_play(mMPEG);

    return MOVERR_OK;
}

// Stop playing a movie
//
S32 GuiAviBitmapCtrl::movieStop()
{
    mBPlaying = false;

    if (mMPEG)
    {
        SMPEG_stop(mMPEG);
    }
    sndStop();

    // notify the script
    // Con::executef(this,1,"movieStopped");

    mDone = true;

    return MOVERR_OK;
}

//----------------------------------------------------------------------------
bool GuiAviBitmapCtrl::onWake()
{
    if (!Parent::onWake()) return false;

    if (fileOpen() || movieOpen())
    {
        mDone = true;
        // Never return false from onWake, the object will be freed, but
        // not removed from the gui framework, so the game crashes later.
        return true;
    }

    sndOpen();

    mBitmapWidth = mInfo.width;
    mBitmapAlignedWidth = ALIGNULONG(mBitmapWidth);
    AssertFatal(mBitmapAlignedWidth == mBitmapWidth, "Unaligned MPEG data");
    mBitmapHeight = mInfo.height;
    mWidthCount = mBitmapWidth / 256;
    mHeightCount = mBitmapHeight / 256;
    if (mBitmapWidth % 256)
        mWidthCount++;
    if (mBitmapHeight % 256)
        mHeightCount++;
    mNumTextures = mWidthCount * mHeightCount;
    mTextureHandles = new TextureHandle[mNumTextures];
    for (U32 j = 0; j < mHeightCount; j++)
    {
        U32 y = j * 256;
        U32 height = getMin(mBitmapWidth - y, U32(256));

        for (U32 i = 0; i < mWidthCount; i++)
        {
            char nameBuffer[64];
            U32 index = j * mWidthCount + i;

            dSprintf(nameBuffer, sizeof(nameBuffer), "%s_#%d_#%d", mAviFilename, i, j);
            mTextureHandles[index] = TextureHandle(nameBuffer, RegisteredTexture, true);
            if (!bool(mTextureHandles[index]))
            {
                U32 x = i * 256;
                U32 width = getMin(mBitmapWidth - x, U32(256));

                const GBitmap* bmp = new GBitmap(width, height, false, GBitmap::RGB);

                mTextureHandles[index] = TextureHandle(nameBuffer, bmp, true);
            }
        }
    }

    // Allocate the SDL surface for the YUV decoding
    // It shore wood be nice if SDL could decode YUV to GL textures...
    mPBuf = (U8*)dMalloc(mBitmapWidth * 3 * mBitmapHeight);
    AssertFatal(mPBuf, "Out of memory for buffer.");
    mSurface = SDL_CreateRGBSurfaceFrom(mPBuf,
        mBitmapWidth, mBitmapHeight, 24,
        mBitmapWidth * 3,
        0x000000FF, 0x0000FF00, 0x00FF0000, 0);
    AssertFatal(mSurface, "Out of memory for a surface!");

    // Target the decoding to our surface
    mDecodeLock = SDL_CreateMutex();
    AssertFatal(mDecodeLock, "Out of memory for a decode lock!");
    SMPEG_setdisplay(mMPEG, mSurface, mDecodeLock, NULL);

    return true;
}

//----------------------------------------------------------------------------
void GuiAviBitmapCtrl::onSleep()
{
    movieClose();
    fileClose();

    if (mSurface)
    {
        SDL_FreeSurface(mSurface);
        mSurface = NULL;
    }

    if (mPBuf)
    {
        dFree(mPBuf);
        mPBuf = NULL;
    }

    if (mTextureHandles)
    {
        delete[] mTextureHandles;
        mTextureHandles = NULL;
    }

    Parent::onSleep();
}

//----------------------------------------------------------------------------
void GuiAviBitmapCtrl::onMouseDown(const GuiEvent&)
{
    // end the movie NOW!
    movieStop();
}

//----------------------------------------------------------------------------
bool GuiAviBitmapCtrl::onKeyDown(const GuiEvent&)
{
    // end the movie NOW!
    movieStop();
    return true;
}

//----------------------------------------------------------------------------
void GuiAviBitmapCtrl::onRender(Point2I offset, const RectI& updateRect)
{
    if (mTextureHandles)
    {
        RectI    displayRect(mBounds);
        S32      verticalDisplace = 0;

        // Get the converted RGB data from SMPEG
        SDL_LockMutex(mDecodeLock);
        for (U32 j = 0; j < mHeightCount; j++)
        {
            U32 y = j * 256;
            U32 height = getMin(mBitmapHeight - y, U32(256));

            for (U32 i = 0; i < mWidthCount; i++)
            {
                U32 index = j * mWidthCount + i;
                U32 x = i * 256;
                U32 width = getMin(mBitmapWidth - x, U32(256));
                GBitmap* bmp = mTextureHandles[index].getBitmap();

                for (U32 lp = 0; lp < height; lp++)
                {
                    const U8* src = &mPBuf[3 * ((y + lp) * mBitmapAlignedWidth + x)];
                    U8* dest = bmp->getAddress(0, lp);

                    dMemcpy(dest, src, width * 3);
                }
                mTextureHandles[index].refresh();
            }
        }
        SDL_UnlockMutex(mDecodeLock);

        if (mLetterBox)
        {
            // Our supplied picture is 3/4 height 640x360 for 640x480 res.  But let's allow
            // for a regular full-window version since it might come in handy elsewhere (so
            // letterBox is public flag on the GUI).
            verticalDisplace = (mBounds.extent.y >> 3);
            displayRect.extent.y = (displayRect.extent.y * 3 >> 2);
            displayRect.point.y += verticalDisplace;
            RectI upperRect(offset.x, offset.y, displayRect.extent.x, verticalDisplace + 1);
            GFX->drawRectFill(upperRect, mProfile->mFillColorHL);
        }

        // Scale into the letterbox-
        F32 widthScale = F32(displayRect.extent.x) / F32(mBitmapWidth);
        F32 heightScale = F32(displayRect.extent.y) / F32(mBitmapHeight);

        offset.y += verticalDisplace;

        GFX->setBitmapModulation(ColorF(1, 1, 1));
        for (U32 i = 0; i < mWidthCount; i++)
        {
            for (U32 j = 0; j < mHeightCount; j++)
            {
                TextureHandle t = mTextureHandles[j * mWidthCount + i];
                RectI stretchRegion;

                stretchRegion.point.x = i * 256 * widthScale + offset.x;
                stretchRegion.point.y = j * 256 * heightScale + offset.y;
                stretchRegion.extent.x = (i * 256 + t.getWidth()) * widthScale + offset.x - stretchRegion.point.x;
                stretchRegion.extent.y = (j * 256 + t.getHeight()) * heightScale + offset.y - stretchRegion.point.y;
                dglDrawBitmapStretch(t, stretchRegion);
            }
        }

        if (mLetterBox)
        {
            // For some reason the above loop draws white below, and this rect fill has to
            // come after - got to look at that math...  Also don't know why we need the
            // extra width & height here...
            RectI lowerRect(offset.x, mBounds.extent.y - verticalDisplace - 1,
                mBounds.extent.x + 2, verticalDisplace + 2);
            GFX->drawRectFill(lowerRect, mProfile->mFillColorHL);
        }

        renderChildControls(offset, updateRect);

        mBPlaying = (SMPEG_status(mMPEG) == SMPEG_PLAYING);
        if (mBPlaying)
            setUpdate();
        else
            movieStop();
    }
    else
        Parent::onRender(offset, updateRect);
}

#endif /* ENABLE_MPG_GUI */

#endif /* Enabled Movie GUI */
