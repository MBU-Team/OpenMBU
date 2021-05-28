#if 0
//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "game/audioEmitter.h"
#include "game/ambientAudioManager.h"
#include "console/consoleTypes.h"
#include "editor/editor.h"
#include "sceneGraph/sceneState.h"
#include "game/gameConnection.h"

//------------------------------------------------------------------------------
static MRandomLCG sgRandom(0xdeadbeef);

#define UPDATE_BUMP_MS  50
extern ALuint alxGetWaveLen(ALuint buffer);
extern ALuint alxFindSource(AUDIOHANDLE handle);

IMPLEMENT_CO_NETOBJECT_V1(AudioEmitter);

//------------------------------------------------------------------------------
class AudioEmitterLoopEvent : public SimEvent
{
public:
    void process(SimObject* object) {
        ((AudioEmitter*)object)->processLoopEvent();
    }
};

//------------------------------------------------------------------------------
AudioEmitter::AudioEmitter(bool client)
{
    // ::packData depends on these values... make sure it is updated with changes
    mTypeMask |= MarkerObjectType;

    if (client)
        mNetFlags = 0;
    else
        mNetFlags.set(Ghostable | ScopeAlways);

    mAudioHandle = NULL_AUDIOHANDLE;

    mAudioProfileId = 0;
    mAudioDescriptionId = 0;

    mLoopCount = smDefaultDescription.mLoopCount;
    mEventID = 0;

    mOwnedByClient = false;

    // field defaults
    mAudioProfile = 0;
    mAudioDescription = 0;
    mFilename = 0;

    mUseProfileDescription = false;
    mOutsideAmbient = true;
    mDescription = smDefaultDescription;
}

Audio::Description AudioEmitter::smDefaultDescription;

//------------------------------------------------------------------------------
void AudioEmitter::processLoopEvent()
{
    if (mLoopCount == 0)
        return;
    else if (mLoopCount > 0)
        mLoopCount--;

    if (mUseProfileDescription && mAudioProfileId)
        return;

    if (!mDescription.mIsLooping || ((mDescription.mLoopCount <= 0) && !mDescription.mMaxLoopGap))
        return;

    // check if still playing...
    U32 source = alxFindSource(mAudioHandle);
    if (source != -1)
    {
        ALint state = AL_STOPPED;
        alGetSourcei(source, AL_SOURCE_STATE, &state);
        if (state == AL_PLAYING)
        {
            mEventID = Sim::postEvent(this, new AudioEmitterLoopEvent, Sim::getCurrentTime() + UPDATE_BUMP_MS);
            return;
        }
    }

    mEventID = 0;
    mDirty.set(SourceMask);
    update();
}

//------------------------------------------------------------------------------
bool AudioEmitter::update()
{
    // object does not know it is really a client object if created through event
    AssertFatal(isClientObject() || mOwnedByClient, "AudioEmitter::update: only clients can update!");

    if (mDirty.test(LoopCount))
        mLoopCount = mDescription.mLoopCount;

    // updating source?
    if (mDirty.test(SourceMask | UseProfileDescription) || (!mUseProfileDescription &&
        ((mDescription.mIsLooping && mDirty.test(LoopingMask)) || mDirty.test(IsLooping | Is3D | AudioType))))
    {
        if (mAudioHandle != NULL_AUDIOHANDLE)
        {
            alxStop(mAudioHandle);
            mAudioHandle = NULL_AUDIOHANDLE;
        }

        // profile:
        if (mDirty.test(Profile))
        {
            if (!mAudioProfileId)
                mAudioProfile = 0;
            else
                mAudioProfile = dynamic_cast<AudioProfile*>(Sim::findObject(mAudioProfileId));
        }

        // description:
        if (mDirty.test(Description))
        {
            if (!mAudioDescriptionId)
                mAudioDescription = 0;
            else
                mAudioDescription = dynamic_cast<AudioDescription*>(Sim::findObject(mAudioDescriptionId));
        }

        MatrixF transform = getTransform();

        // use the profile?
        if (mUseProfileDescription && mAudioProfileId)
            mAudioHandle = alxCreateSource(mAudioProfile, &transform);
        else
        {
            // grab the filename
            const char* fileName = 0;
            if (mFilename && mFilename[0])
                fileName = mFilename;
            else if (mAudioProfile)
                fileName = mAudioProfile->mFilename;

            if (!fileName)
            {
                Con::errorf(ConsoleLogEntry::General, "AudioEmitter::update: invalid audio filename!");
                return(false);
            }

            // use a description?
            if (mAudioDescription)
                mAudioHandle = alxCreateSource(mAudioDescription, fileName, &transform);
            else
            {
                if (mDescription.mIsLooping)
                {
                    S32 minGap = mClamp(mDescription.mMinLoopGap, 0, mDescription.mMaxLoopGap);
                    S32 maxGap = mClamp(mDescription.mMaxLoopGap, mDescription.mMinLoopGap, mDescription.mMaxLoopGap);

                    // controlling looping?
                    if ((mDescription.mLoopCount != -1) || maxGap)
                    {
                        mDescription.mIsLooping = false;
                        mAudioHandle = alxCreateSource(&mDescription, fileName, &transform);
                        mDescription.mIsLooping = true;

                        // handle may come back as null.. (0 volume, no handles, ...), still
                        // want to try again (assume good buffer) since this is a looper
                        Resource<AudioBuffer> buffer = AudioBuffer::find(fileName);
                        S32 waveLen = 0;
                        if (bool(buffer))
                        {
                            ALuint alBuffer = buffer->getALBuffer();
                            waveLen = alxGetWaveLen(alBuffer);
                        }

                        if (waveLen)
                        {
                            S32 offset = waveLen + minGap + sgRandom.randI(minGap, maxGap);

                            if (mEventID)
                                Sim::cancelEvent(mEventID);
                            mEventID = Sim::postEvent(this, new AudioEmitterLoopEvent, Sim::getCurrentTime() + offset);

                            if (mAudioHandle == NULL_AUDIOHANDLE)
                                return(true);
                        }
                    }
                    else
                        mAudioHandle = alxCreateSource(&mDescription, fileName, &transform);
                }
                else
                    mAudioHandle = alxCreateSource(&mDescription, fileName, &transform);
            }
        }

        if (mAudioHandle == NULL_AUDIOHANDLE)
        {
            Con::errorf(ConsoleLogEntry::General, "AudioEmitter::update: failed to create source!");
            return(false);
        }

        alxPlay(mAudioHandle);
    }
    else
    {
        // don't clear dirty flags until there is a source
        if (mAudioHandle == NULL_AUDIOHANDLE)
            return(true);

        if (mDirty.test(Transform))
        {
            Point3F pos;
            mObjToWorld.getColumn(3, &pos);
            alxSource3f(mAudioHandle, AL_POSITION, pos.x, pos.y, pos.z);
        }

        // grab the description
        const Audio::Description* desc = 0;

        if (mUseProfileDescription && mAudioProfile)
            desc = mAudioProfile->getDescription();
        else if (mAudioDescription)
            desc = mAudioDescription->getDescription();
        else
            desc = &mDescription;

        if (!desc)
            return(true);

        if (mDirty.test(Volume))
            alxSourcef(mAudioHandle, AL_GAIN_LINEAR, desc->mVolume);

        if (mDirty.test(ReferenceDistance | MaxDistance))
        {
            alxSourcef(mAudioHandle, AL_REFERENCE_DISTANCE, desc->mReferenceDistance);
            alxSourcef(mAudioHandle, AL_MAX_DISTANCE, desc->mMaxDistance);
        }

        if (mDirty.test(ConeInsideAngle))
            alxSourcei(mAudioHandle, AL_CONE_INNER_ANGLE, desc->mConeInsideAngle);

        if (mDirty.test(ConeOutsideAngle))
            alxSourcei(mAudioHandle, AL_CONE_OUTER_ANGLE, desc->mConeOutsideAngle);

        if (mDirty.test(ConeOutsideVolume))
            alxSourcef(mAudioHandle, AL_CONE_OUTER_GAIN, desc->mConeOutsideVolume);
    }

    mDirty.clear();
    return(true);
}

//------------------------------------------------------------------------------
bool AudioEmitter::onAdd()
{
    if (!Parent::onAdd())
        return(false);

    // object does not know it is really a client object if created through event
    if (isServerObject() && !mOwnedByClient)
    {
        // validate the object data
        mDescription.mVolume = mClampF(mDescription.mVolume, 0.0f, 1.0f);
        mDescription.mLoopCount = mClamp(mDescription.mLoopCount, -1, mDescription.mLoopCount);
        mDescription.mMaxLoopGap = mClamp(mDescription.mMaxLoopGap, mDescription.mMinLoopGap, mDescription.mMaxLoopGap);
        mDescription.mMinLoopGap = mClamp(mDescription.mMinLoopGap, 0, mDescription.mMaxLoopGap);

        if (mDescription.mIs3D)
        {
            mDescription.mReferenceDistance = mClampF(mDescription.mReferenceDistance, 0.f, mDescription.mReferenceDistance);
            mDescription.mMaxDistance = (mDescription.mMaxDistance > mDescription.mReferenceDistance) ? mDescription.mMaxDistance : (mDescription.mReferenceDistance + 0.01f);
            mDescription.mConeInsideAngle = mClamp(mDescription.mConeInsideAngle, 0, 360);
            mDescription.mConeOutsideAngle = mClamp(mDescription.mConeOutsideAngle, mDescription.mConeInsideAngle, 360);
            mDescription.mConeOutsideVolume = mClampF(mDescription.mConeOutsideVolume, 0.0f, 1.0f);
            mDescription.mConeVector.normalize();
        }
    }
    else
    {
        if (!update())
        {
            Con::errorf(ConsoleLogEntry::General, "AudioEmitter::onAdd: client failed initial update!");
            if (mOwnedByClient)
                return(false);
        }
        gAmbientAudioManager.addEmitter(this);
    }

    //
    mObjBox.max = mObjScale;
    mObjBox.min = mObjScale;
    mObjBox.min.neg();
    resetWorldBox();
    addToScene();

    return(true);
}

void AudioEmitter::onRemove()
{
    if (isClientObject())
    {
        gAmbientAudioManager.removeEmitter(this);
        if (mAudioHandle != NULL_AUDIOHANDLE)
            alxStop(mAudioHandle);
    }
    removeFromScene();
    Parent::onRemove();
}

void AudioEmitter::setTransform(const MatrixF& mat)
{
    // Set the transform directly from the matrix created by inspector
    // Also, be sure to strip out the pointing vector and place it in the cone vector
    Point3F pointing;
    mat.getColumn(1, &pointing);
    mDescription.mConeVector = pointing;
    Parent::setTransform(mat);
    setMaskBits(TransformUpdateMask);
}

void AudioEmitter::setScale(const VectorF&)
{
}

//------------------------------------------------------------------------------
namespace {
    static Point3F cubePoints[8] = {
       Point3F(-1, -1, -1), Point3F(-1, -1,  1), Point3F(-1,  1, -1), Point3F(-1,  1,  1),
       Point3F(1, -1, -1), Point3F(1, -1,  1), Point3F(1,  1, -1), Point3F(1,  1,  1)
    };

    static U32 cubeFaces[6][4] = {
       { 0, 2, 6, 4 }, { 0, 2, 3, 1 }, { 0, 1, 5, 4 },
       { 3, 2, 6, 7 }, { 7, 6, 4, 5 }, { 3, 7, 5, 1 }
    };

    static Point2F textureCoords[4] = {
       Point2F(0, 0), Point2F(1, 0), Point2F(1, 1), Point2F(0, 1)
    };
}

bool AudioEmitter::prepRenderImage(SceneState* state, const U32 stateKey, const U32, const bool)
{
    if (!gEditingMission || isLastState(state, stateKey))
        return(false);

    setLastState(state, stateKey);
    if (gEditingMission && state->isObjectRendered(this))
    {
        /*
              SceneRenderImage * image = new SceneRenderImage;
              image->obj = this;
              state->insertRenderImage(image);
        */
    }
    return(false);
}

void AudioEmitter::renderObject(SceneState*)
{
    /*
       TextureHandle texture("special/BlueImpact", BitmapTexture, false);
       if(bool(texture))
       {
          glEnable(GL_TEXTURE_2D);
          glBindTexture(GL_TEXTURE_2D, texture.getGLName());
       }

       glPushMatrix();
       dglMultMatrix(&mObjToWorld);
       glScalef(mObjScale.x, mObjScale.y, mObjScale.z);

       glDisable(GL_CULL_FACE);
       Point3F size(1.f, 1.f, 1.f);

       // draw the cube
       for(int i = 0; i < 6; i++)
       {
          glBegin(GL_QUADS);
          for(int vert = 0; vert < 4; vert++)
          {
             int idx = cubeFaces[i][vert];
             if(bool(texture))
                glTexCoord2f(textureCoords[vert].x, textureCoords[vert].y);
             glVertex3f(cubePoints[idx].x * size.x, cubePoints[idx].y * size.y, cubePoints[idx].z * size.z);
          }
          glEnd();
       }

       if(bool(texture))
          glDisable(GL_TEXTURE_2D);

       glPopMatrix();
    */
}

//------------------------------------------------------------------------------
namespace {
    static AudioProfile* saveAudioProfile;
    static AudioDescription* saveAudioDescription;
    static StringTableEntry saveFilename;
    static bool saveUseProfileDescription;
    static Audio::Description saveDescription;
    static Point3F savePos;
    static bool saveOutsideAmbient;
}

void AudioEmitter::inspectPreApply()
{
    if (isClientObject())
        return;

    Parent::inspectPostApply();

    mObjToWorld.getColumn(3, &savePos);
    saveAudioProfile = mAudioProfile;
    saveAudioDescription = mAudioDescription;
    saveFilename = mFilename;
    saveUseProfileDescription = mUseProfileDescription;
    saveDescription = mDescription;
    saveOutsideAmbient = mOutsideAmbient;
}

void AudioEmitter::inspectPostApply()
{
    if (isClientObject())
        return;

    Parent::inspectPostApply();

    Point3F pos;
    mObjToWorld.getColumn(3, &pos);

    // set some dirty flags
    mDirty.clear();
    mDirty.set((savePos != pos) ? Transform : 0);
    mDirty.set((saveAudioProfile != mAudioProfile) ? Profile : 0);
    mDirty.set((saveAudioDescription != mAudioDescription) ? Description : 0);
    mDirty.set((saveFilename != mFilename) ? Filename : 0);
    mDirty.set((saveUseProfileDescription != mUseProfileDescription) ? UseProfileDescription : 0);
    mDirty.set((saveDescription.mVolume != mDescription.mVolume) ? Volume : 0);
    mDirty.set((saveDescription.mIsLooping != mDescription.mIsLooping) ? IsLooping : 0);
    mDirty.set((saveDescription.mIs3D != mDescription.mIs3D) ? Is3D : 0);
    mDirty.set((saveDescription.mReferenceDistance != mDescription.mReferenceDistance) ? ReferenceDistance : 0);
    mDirty.set((saveDescription.mMaxDistance != mDescription.mMaxDistance) ? MaxDistance : 0);
    mDirty.set((saveDescription.mConeInsideAngle != mDescription.mConeInsideAngle) ? ConeInsideAngle : 0);
    mDirty.set((saveDescription.mConeOutsideAngle != mDescription.mConeOutsideAngle) ? ConeOutsideAngle : 0);
    mDirty.set((saveDescription.mConeOutsideVolume != mDescription.mConeOutsideVolume) ? ConeOutsideVolume : 0);
    mDirty.set((saveDescription.mConeVector != mDescription.mConeVector) ? ConeVector : 0);
    mDirty.set((saveDescription.mLoopCount != mDescription.mLoopCount) ? LoopCount : 0);
    mDirty.set((saveDescription.mMinLoopGap != mDescription.mMinLoopGap) ? MinLoopGap : 0);
    mDirty.set((saveDescription.mMaxLoopGap != mDescription.mMaxLoopGap) ? MaxLoopGap : 0);
    mDirty.set((saveDescription.mType != mDescription.mType) ? AudioType : 0);
    mDirty.set((saveOutsideAmbient != mOutsideAmbient) ? OutsideAmbient : 0);

    if (mDirty)
        setMaskBits(DirtyUpdateMask);
}

void AudioEmitter::packData(NetConnection*, U32 mask, BitStream* stream)
{
    // initial update
    if (stream->writeFlag(mask & InitialUpdateMask))
    {
        mask |= TransformUpdateMask;
        mDirty = AllDirtyMask;

        // see if can remove some items from initial update: description checked below
        if (!mAudioProfile)
            mDirty.clear(Profile);
        if (!mAudioDescription)
            mDirty.clear(Description);
        if (!mFilename || !mFilename[0])
            mDirty.clear(Filename);
        if (!mUseProfileDescription)
            mDirty.clear(UseProfileDescription);
        if (mOutsideAmbient)
            mDirty.clear(OutsideAmbient);
    }

    // transform
    if (stream->writeFlag(mask & TransformUpdateMask))
        stream->writeAffineTransform(mObjToWorld);

    // profile
    if (stream->writeFlag(mDirty.test(Profile)))
        if (stream->writeFlag(mAudioProfile))
            stream->writeRangedU32(mAudioProfile->getId(), DataBlockObjectIdFirst, DataBlockObjectIdLast);

    // description
    if (stream->writeFlag(mDirty.test(Description)))
        if (stream->writeFlag(mAudioDescription))
            stream->writeRangedU32(mAudioDescription->getId(), DataBlockObjectIdFirst, DataBlockObjectIdLast);

    // filename
    if (stream->writeFlag(mDirty.test(Filename)))
        stream->writeString(mFilename);

    // useprofiledescription
    if (stream->writeFlag(mDirty.test(UseProfileDescription)))
    {
        if (mUseProfileDescription)
        {
            if (!mAudioProfile)
                mUseProfileDescription = false;
            else
                mDirty.clear(UseProfileDescriptionMask);
        }

        if (!mUseProfileDescription)
            mDirty.set(UseProfileDescriptionMask);

        stream->writeFlag(mUseProfileDescription);
    }

    if (mAudioDescription && !mUseProfileDescription)
        mDirty.clear(UseProfileDescriptionMask);

    // check initial update against the default description
    if ((mask & InitialUpdateMask) && !mUseProfileDescription)
    {
        if (mDescription.mVolume == smDefaultDescription.mVolume) mDirty.clear(Volume);
        AssertFatal(smDefaultDescription.mIsLooping, "Doh!");
        if (mDescription.mIsLooping != smDefaultDescription.mIsLooping)
            mDirty.clear(LoopingMask);
        else
        {
            mDirty.clear(IsLooping);
            if (mDescription.mLoopCount == smDefaultDescription.mLoopCount) mDirty.clear(LoopCount);
            if (mDescription.mMinLoopGap == smDefaultDescription.mMinLoopGap) mDirty.clear(MinLoopGap);
            if (mDescription.mMaxLoopGap == smDefaultDescription.mMaxLoopGap) mDirty.clear(MaxLoopGap);
        }
        AssertFatal(smDefaultDescription.mIs3D, "Doh!");
        if (mDescription.mIs3D != smDefaultDescription.mIs3D)
            mDirty.clear(Is3DMask);
        else
        {
            mDirty.clear(Is3D);
            if (mDescription.mReferenceDistance == smDefaultDescription.mReferenceDistance) mDirty.clear(ReferenceDistance);
            if (mDescription.mMaxDistance == smDefaultDescription.mMaxDistance) mDirty.clear(MaxDistance);
            if (mDescription.mConeInsideAngle == smDefaultDescription.mConeInsideAngle) mDirty.clear(ConeInsideAngle);
            if (mDescription.mConeOutsideAngle == smDefaultDescription.mConeOutsideAngle) mDirty.clear(ConeOutsideAngle);
            if (mDescription.mConeOutsideVolume == smDefaultDescription.mConeOutsideVolume) mDirty.clear(ConeOutsideVolume);
            if (mDescription.mConeVector == smDefaultDescription.mConeVector)  mDirty.clear(ConeVector);
        }
        if (mDescription.mType == smDefaultDescription.mType) mDirty.clear(AudioType);
    }

    // volume
    if (stream->writeFlag(mDirty.test(Volume)))
        stream->write(mDescription.mVolume);

    // islooping
    if (stream->writeFlag(mDirty.test(IsLooping)))
    {
        mDescription.mIsLooping ? mDirty.set(LoopingMask) : mDirty.clear(LoopingMask);
        stream->writeFlag(mDescription.mIsLooping);
    }

    // is3d
    if (stream->writeFlag(mDirty.test(Is3D)))
    {
        mDescription.mIs3D ? mDirty.set(Is3DMask) : mDirty.clear(Is3DMask);
        stream->writeFlag(mDescription.mIs3D);
    }

    // ReferenceDistance
    if (stream->writeFlag(mDirty.test(ReferenceDistance)))
        stream->write(mDescription.mReferenceDistance);

    // maxdistance
    if (stream->writeFlag(mDirty.test(MaxDistance)))
        stream->write(mDescription.mMaxDistance);

    // coneinsideangle
    if (stream->writeFlag(mDirty.test(ConeInsideAngle)))
        stream->write(mDescription.mConeInsideAngle);

    // coneoutsideangle
    if (stream->writeFlag(mDirty.test(ConeOutsideAngle)))
        stream->write(mDescription.mConeOutsideAngle);

    // coneoutsidevolume
    if (stream->writeFlag(mDirty.test(ConeOutsideVolume)))
        stream->write(mDescription.mConeOutsideVolume);

    // conevector
    if (stream->writeFlag(mDirty.test(ConeVector)))
    {
        stream->write(mDescription.mConeVector.x);
        stream->write(mDescription.mConeVector.y);
        stream->write(mDescription.mConeVector.z);
    }

    // loopcount
    if (stream->writeFlag(mDirty.test(LoopCount)))
        stream->write(mDescription.mLoopCount);

    // minloopgap
    if (stream->writeFlag(mDirty.test(MinLoopGap)))
        stream->write(mDescription.mMinLoopGap);

    // maxloopgap
    if (stream->writeFlag(mDirty.test(MaxLoopGap)))
        stream->write(mDescription.mMaxLoopGap);

    // audiotype
    if (stream->writeFlag(mDirty.test(AudioType)))
        stream->write(mDescription.mType);

    // outside ambient
    if (stream->writeFlag(mDirty.test(OutsideAmbient)))
        stream->writeFlag(mOutsideAmbient);

    mDirty.clear();
}

//------------------------------------------------------------------------------
U32 AudioEmitter::packUpdate(NetConnection* con, U32 mask, BitStream* stream)
{
    U32 retMask = Parent::packUpdate(con, mask, stream);
    packData(con, mask, stream);
    return(retMask);
}

bool AudioEmitter::readDirtyFlag(BitStream* stream, U32 mask)
{
    bool flag = stream->readFlag();
    if (flag)
        mDirty.set(mask);
    return(flag);
}

void AudioEmitter::unpackData(NetConnection*, BitStream* stream)
{
    // initial update?
    bool initialUpdate = stream->readFlag();

    // transform
    if (readDirtyFlag(stream, Transform))
    {
        MatrixF mat;
        stream->readAffineTransform(&mat);
        Parent::setTransform(mat);
    }

    // profile
    if (readDirtyFlag(stream, Profile))
        if (stream->readFlag())
            mAudioProfileId = stream->readRangedU32(DataBlockObjectIdFirst, DataBlockObjectIdLast);
        else
            mAudioProfileId = 0;

    // description
    if (readDirtyFlag(stream, Description))
        if (stream->readFlag())
            mAudioDescriptionId = stream->readRangedU32(DataBlockObjectIdFirst, DataBlockObjectIdLast);
        else
            mAudioDescriptionId = 0;

    // filename
    if (readDirtyFlag(stream, Filename))
        mFilename = stream->readSTString();

    // useprofiledescription
    if (readDirtyFlag(stream, UseProfileDescription))
        mUseProfileDescription = stream->readFlag();

    // volume
    if (readDirtyFlag(stream, Volume))
        stream->read(&mDescription.mVolume);

    // islooping
    if (readDirtyFlag(stream, IsLooping))
        mDescription.mIsLooping = stream->readFlag();

    // is3d
    if (readDirtyFlag(stream, Is3D))
        mDescription.mIs3D = stream->readFlag();

    // ReferenceDistance
    if (readDirtyFlag(stream, ReferenceDistance))
        stream->read(&mDescription.mReferenceDistance);

    // maxdistance
    if (readDirtyFlag(stream, MaxDistance))
        stream->read(&mDescription.mMaxDistance);

    // coneinsideangle
    if (readDirtyFlag(stream, ConeInsideAngle))
        stream->read(&mDescription.mConeInsideAngle);

    // coneoutsideangle
    if (readDirtyFlag(stream, ConeOutsideAngle))
        stream->read(&mDescription.mConeOutsideAngle);

    // coneoutsidevolume
    if (readDirtyFlag(stream, ConeOutsideVolume))
        stream->read(&mDescription.mConeOutsideVolume);

    // conevector
    if (readDirtyFlag(stream, ConeVector))
    {
        stream->read(&mDescription.mConeVector.x);
        stream->read(&mDescription.mConeVector.y);
        stream->read(&mDescription.mConeVector.z);
    }

    // loopcount
    if (readDirtyFlag(stream, LoopCount))
        stream->read(&mDescription.mLoopCount);

    // minloopgap
    if (readDirtyFlag(stream, MinLoopGap))
    {
        stream->read(&mDescription.mMinLoopGap);
        if (mDescription.mMinLoopGap < 0)
            mDescription.mMinLoopGap = 0;
    }

    // maxloopgap
    if (readDirtyFlag(stream, MaxLoopGap))
    {
        stream->read(&mDescription.mMaxLoopGap);
        if (mDescription.mMaxLoopGap < 0)
            mDescription.mMaxLoopGap = 0;
    }

    // audiotype
    if (readDirtyFlag(stream, AudioType))
    {
        stream->read(&mDescription.mType);
        if (mDescription.mType >= Audio::NumAudioTypes)
            mDescription.mType = 0;
    }

    // outside ambient
    if (readDirtyFlag(stream, OutsideAmbient))
        mOutsideAmbient = stream->readFlag();

    // update the emitter now?
    if (!initialUpdate)
        update();
}

void AudioEmitter::unpackUpdate(NetConnection* con, BitStream* stream)
{
    Parent::unpackUpdate(con, stream);
    unpackData(con, stream);
}

void AudioEmitter::initPersistFields()
{
    Parent::initPersistFields();

    addGroup("Profile");
    addField("profile", TypeAudioProfilePtr, Offset(mAudioProfile, AudioEmitter));
    addField("useProfileDescription", TypeBool, Offset(mUseProfileDescription, AudioEmitter));
    endGroup("Profile");

    addGroup("Media");
    addField("description", TypeAudioDescriptionPtr, Offset(mAudioDescription, AudioEmitter));
    addField("fileName", TypeFilename, Offset(mFilename, AudioEmitter));
    addField("type", TypeS32, Offset(mDescription.mType, AudioEmitter));
    endGroup("Media");

    addGroup("Sound");
    addField("volume", TypeF32, Offset(mDescription.mVolume, AudioEmitter));
    addField("outsideAmbient", TypeBool, Offset(mOutsideAmbient, AudioEmitter));
    addField("ReferenceDistance", TypeF32, Offset(mDescription.mReferenceDistance, AudioEmitter));
    addField("maxDistance", TypeF32, Offset(mDescription.mMaxDistance, AudioEmitter));
    endGroup("Sound");

    addGroup("Looping");
    addField("isLooping", TypeBool, Offset(mDescription.mIsLooping, AudioEmitter));
    addField("is3D", TypeBool, Offset(mDescription.mIs3D, AudioEmitter));
    addField("loopCount", TypeS32, Offset(mDescription.mLoopCount, AudioEmitter));
    addField("minLoopGap", TypeS32, Offset(mDescription.mMinLoopGap, AudioEmitter));
    addField("maxLoopGap", TypeS32, Offset(mDescription.mMaxLoopGap, AudioEmitter));
    endGroup("Looping");

    addGroup("Sound Cone");
    addField("coneInsideAngle", TypeS32, Offset(mDescription.mConeInsideAngle, AudioEmitter));
    addField("coneOutsideAngle", TypeS32, Offset(mDescription.mConeOutsideAngle, AudioEmitter));
    addField("coneOutsideVolume", TypeF32, Offset(mDescription.mConeOutsideVolume, AudioEmitter));
    addField("coneVector", TypePoint3F, Offset(mDescription.mConeVector, AudioEmitter));
    endGroup("Sound Cone");



    // create the static description
    smDefaultDescription.mVolume = 1.0f;
    smDefaultDescription.mIsLooping = true;
    smDefaultDescription.mIs3D = true;
    smDefaultDescription.mReferenceDistance = 1.0f;
    smDefaultDescription.mMaxDistance = 100.0f;
    smDefaultDescription.mConeInsideAngle = 360;
    smDefaultDescription.mConeOutsideAngle = 360;
    smDefaultDescription.mConeOutsideVolume = 1.0f;
    smDefaultDescription.mConeVector.set(0, 0, 1);
    smDefaultDescription.mLoopCount = -1;
    smDefaultDescription.mMinLoopGap = 0;
    smDefaultDescription.mMaxLoopGap = 0;
    smDefaultDescription.mType = 0;
}

////--------------------------------------------------------------------------
//IMPLEMENT_CO_NETOBJECT_V1(ClientAudioEmitter);
//
//ClientAudioEmitter::ClientAudioEmitter()
//{
//   mNetFlags = 0;
//   mEventCount = 0;
//}
//
//bool ClientAudioEmitter::onAdd()
//{
//   if(!SceneObject::onAdd())
//      return(false);
//
//   // create and send events to all clients
//   bool postedEvent = false;
//   for(NetConnection * conn = NetConnection::getConnectionList(); conn; conn = conn->getNext())
//   {
//      if(!conn->isConnectionToServer())
//      {
//         postedEvent = true;
//         conn->postNetEvent(new AudioEmitterToEvent(this));
//      }
//   }
//
//   return(postedEvent);
//}
//
//void ClientAudioEmitter::initPersistFields()
//{
//   Parent::initPersistFields();
//}
//
////--------------------------------------------------------------------------
//IMPLEMENT_CO_NETEVENT_V1(AudioEmitterToEvent);
//
//AudioEmitterToEvent::AudioEmitterToEvent(ClientAudioEmitter * emitter)
//{
//   mEmitter = emitter;
//   if(bool(mEmitter))
//      mEmitter->mEventCount++;
//}
//
//void AudioEmitterToEvent::notifyDelivered(NetConnection *, bool)
//{
//   if(bool(mEmitter) && mEmitter->mEventCount && (--mEmitter->mEventCount == 0))
//   {
//      mEmitter->deleteObject();
//      mEmitter = 0;
//   }
//}
//
//void AudioEmitterToEvent::write(NetConnection * conn, BitStream * bstream)
//{
//   if(bool(mEmitter))
//      mEmitter->packData(conn, AudioEmitter::InitialUpdateMask, bstream);
//}
//
//void AudioEmitterToEvent::pack(NetConnection * conn, BitStream * bstream)
//{
//   if(bool(mEmitter))
//      mEmitter->packData(conn, AudioEmitter::InitialUpdateMask, bstream);
//}
//
//void AudioEmitterToEvent::unpack(NetConnection * conn, BitStream * bstream)
//{
//   // clients are responsible for deleting these audioemitters!
//   AudioEmitter * emitter = new AudioEmitter(true);
//   emitter->mOwnedByClient = true;
//   emitter->unpackData(conn, bstream);
//   emitter->registerOb
#endif