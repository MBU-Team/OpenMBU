//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "platform/platformAudio.h"
#include "console/simBase.h"
#include "audio/audioDataBlock.h"


extern ALuint alxGetWaveLen(ALuint buffer);

extern F32 mAudioTypeVolume[Audio::NumAudioTypes];

//--------------------------------------------------------------------------
// Expose all al get/set methods...
//--------------------------------------------------------------------------
enum AL_GetSetBits {
    Source = BIT(0),
    Listener = BIT(1),
    Context = BIT(2),
    Environment = BIT(3),
    Get = BIT(4),
    Set = BIT(5),
    Int = BIT(6),
    Float = BIT(7),
    Float3 = BIT(8),
    Float6 = BIT(9)
};

static ALenum getEnum(const char* name, U32 flags)
{
    AssertFatal(name, "Audio getEnum: bad param");

    static struct {
        char* mName;
        ALenum   mAlenum;
        U32      mFlags;
    } table[] = {
        //-----------------------------------------------------------------------------------------------------------------
        // "name"                           ENUM                             Flags
        //-----------------------------------------------------------------------------------------------------------------
        { "AL_GAIN",                        AL_GAIN,                         (Source | Listener | Get | Set | Float) },
        { "AL_GAIN_LINEAR",                 AL_GAIN_LINEAR,                  (Source | Listener | Get | Set | Float) },
        { "AL_PITCH",                       AL_PITCH,                        (Source | Get | Set | Float) },
        { "AL_REFERENCE_DISTANCE",          AL_REFERENCE_DISTANCE,           (Source | Get | Set | Float) },
        { "AL_MAX_DISTANCE",                AL_MAX_DISTANCE,                 (Source | Get | Set | Float) },
        { "AL_CONE_OUTER_GAIN",             AL_CONE_OUTER_GAIN,              (Source | Get | Set | Float) },
        { "AL_POSITION",                    AL_POSITION,                     (Source | Listener | Get | Set | Float3) },
        { "AL_DIRECTION",                   AL_DIRECTION,                    (Source | Get | Set | Float3) },
        { "AL_VELOCITY",                    AL_VELOCITY,                     (Source | Listener | Get | Set | Float3) },
        { "AL_ORIENTATION",                 AL_ORIENTATION,                  (Listener | Set | Float6) },
        { "AL_CONE_INNER_ANGLE",            AL_CONE_INNER_ANGLE,             (Source | Get | Set | Int) },
        { "AL_CONE_OUTER_ANGLE",            AL_CONE_OUTER_ANGLE,             (Source | Get | Set | Int) },
        { "AL_LOOPING",                     AL_LOOPING,                      (Source | Get | Set | Int) },
        //{ "AL_STREAMING",                   AL_STREAMING,                    (Source|Get|Set|Int) },
        //{ "AL_BUFFER",                      AL_BUFFER,                       (Source|Get|Set|Int) },

        { "AL_VENDOR",                      AL_VENDOR,                       (Context | Get) },
        { "AL_VERSION",                     AL_VERSION,                      (Context | Get) },
        { "AL_RENDERER",                    AL_RENDERER,                     (Context | Get) },
        { "AL_EXTENSIONS",                  AL_EXTENSIONS,                   (Context | Get) },

        /*
        // environment
        { "AL_ENV_ROOM_IASIG",                       AL_ENV_ROOM_IASIG,                        (Environment|Get|Set|Int) },
        { "AL_ENV_ROOM_HIGH_FREQUENCY_IASIG",        AL_ENV_ROOM_HIGH_FREQUENCY_IASIG,         (Environment|Get|Set|Int) },
        { "AL_ENV_REFLECTIONS_IASIG",                AL_ENV_REFLECTIONS_IASIG,                 (Environment|Get|Set|Int) },
        { "AL_ENV_REVERB_IASIG",                     AL_ENV_REVERB_IASIG,                      (Environment|Get|Set|Int) },
        { "AL_ENV_ROOM_ROLLOFF_FACTOR_IASIG",        AL_ENV_ROOM_ROLLOFF_FACTOR_IASIG,         (Environment|Get|Set|Float) },
        { "AL_ENV_DECAY_TIME_IASIG",                 AL_ENV_DECAY_TIME_IASIG,                  (Environment|Get|Set|Float) },
        { "AL_ENV_DECAY_HIGH_FREQUENCY_RATIO_IASIG", AL_ENV_DECAY_HIGH_FREQUENCY_RATIO_IASIG,  (Environment|Get|Set|Float) },
        { "AL_ENV_REFLECTIONS_DELAY_IASIG",          AL_ENV_REFLECTIONS_DELAY_IASIG,           (Environment|Get|Set|Float) },
        { "AL_ENV_REVERB_DELAY_IASIG",               AL_ENV_REVERB_DELAY_IASIG,                (Environment|Get|Set|Float) },
        { "AL_ENV_DIFFUSION_IASIG",                  AL_ENV_DIFFUSION_IASIG,                   (Environment|Get|Set|Float) },
        { "AL_ENV_DENSITY_IASIG",                    AL_ENV_DENSITY_IASIG,                     (Environment|Get|Set|Float) },
        { "AL_ENV_HIGH_FREQUENCY_REFERENCE_IASIG",   AL_ENV_HIGH_FREQUENCY_REFERENCE_IASIG,    (Environment|Get|Set|Float) },

        { "AL_ENV_ROOM_VOLUME_EXT",                  AL_ENV_ROOM_VOLUME_EXT,                   (Environment|Get|Set|Int) },
        { "AL_ENV_FLAGS_EXT",                        AL_ENV_FLAGS_EXT,                         (Environment|Get|Set|Int) },
        { "AL_ENV_EFFECT_VOLUME_EXT",                AL_ENV_EFFECT_VOLUME_EXT,                 (Environment|Get|Set|Float) },
        { "AL_ENV_DAMPING_EXT",                      AL_ENV_DAMPING_EXT,                       (Environment|Get|Set|Float) },
        { "AL_ENV_ENVIRONMENT_SIZE_EXT",             AL_ENV_ENVIRONMENT_SIZE_EXT,              (Environment|Get|Set|Float) },

        // sample environment
        { "AL_ENV_SAMPLE_DIRECT_EXT",                AL_ENV_SAMPLE_DIRECT_EXT,                 (Source|Get|Set|Int) },
        { "AL_ENV_SAMPLE_DIRECT_HF_EXT",             AL_ENV_SAMPLE_DIRECT_HF_EXT,              (Source|Get|Set|Int) },
        { "AL_ENV_SAMPLE_ROOM_EXT",                  AL_ENV_SAMPLE_ROOM_EXT,                   (Source|Get|Set|Int) },
        { "AL_ENV_SAMPLE_ROOM_HF_EXT",               AL_ENV_SAMPLE_ROOM_HF_EXT,                (Source|Get|Set|Int) },
        { "AL_ENV_SAMPLE_OUTSIDE_VOLUME_HF_EXT",     AL_ENV_SAMPLE_OUTSIDE_VOLUME_HF_EXT,      (Source|Get|Set|Int) },
        { "AL_ENV_SAMPLE_FLAGS_EXT",                 AL_ENV_SAMPLE_FLAGS_EXT,                  (Source|Get|Set|Int) },

        { "AL_ENV_SAMPLE_REVERB_MIX_EXT",            AL_ENV_SAMPLE_REVERB_MIX_EXT,             (Source|Get|Set|Float) },
        { "AL_ENV_SAMPLE_OBSTRUCTION_EXT",           AL_ENV_SAMPLE_OBSTRUCTION_EXT,            (Source|Get|Set|Float) },
        { "AL_ENV_SAMPLE_OBSTRUCTION_LF_RATIO_EXT",  AL_ENV_SAMPLE_OBSTRUCTION_LF_RATIO_EXT,   (Source|Get|Set|Float) },
        { "AL_ENV_SAMPLE_OCCLUSION_EXT",             AL_ENV_SAMPLE_OCCLUSION_EXT,              (Source|Get|Set|Float) },
        { "AL_ENV_SAMPLE_OCCLUSION_LF_RATIO_EXT",    AL_ENV_SAMPLE_OCCLUSION_LF_RATIO_EXT,     (Source|Get|Set|Float) },
        { "AL_ENV_SAMPLE_OCCLUSION_ROOM_RATIO_EXT",  AL_ENV_SAMPLE_OCCLUSION_ROOM_RATIO_EXT,   (Source|Get|Set|Float) },
        { "AL_ENV_SAMPLE_ROOM_ROLLOFF_EXT",          AL_ENV_SAMPLE_ROOM_ROLLOFF_EXT,           (Source|Get|Set|Float) },
        { "AL_ENV_SAMPLE_AIR_ABSORPTION_EXT",        AL_ENV_SAMPLE_AIR_ABSORPTION_EXT,         (Source|Get|Set|Float) },
     */
    };
    for (U32 i = 0; i < (sizeof(table) / sizeof(table[0])); i++)
    {
        if ((table[i].mFlags & flags) != flags)
            continue;

        if (dStricmp(table[i].mName, name) == 0)
            return(table[i].mAlenum);
    }

    return(AL_INVALID);
}


//-----------------------------------------------
ConsoleFunctionGroupBegin(Audio, "Functions dealing with the OpenAL audio layer.\n\n"
    "@see www.OpenAL.org for what these functions do. Variances from posted"
    "     behaviour is described below.");

ConsoleFunction(OpenALInitDriver, bool, 1, 1, "Initializes the OpenAL driver.\n\n"
    "@note You must call this before any sounds will work!")
{
    if (Audio::OpenALInit())
    {
        static bool registered = false;
        if (!registered) {
            ResourceManager->registerExtension(".wav", AudioBuffer::construct);
            ResourceManager->registerExtension(".ogg", AudioBuffer::construct);
        }
        registered = true;
        return true;
    }
    return false;
}

//-----------------------------------------------
ConsoleFunction(OpenALShutdownDriver, void, 1, 1, "OpenALShutdownDriver()")
{
    Audio::OpenALShutdown();
}


//-----------------------------------------------
ConsoleFunction(OpenALRegisterExtensions, void, 1, 1, "OpenALRegisterExtensions()")
{
}

//-----------------------------------------------
ConsoleFunction(alGetString, const char*, 2, 2, "(string item)\n\n"
    "This wraps alGetString().")
{
    argc;
    ALenum e = getEnum(argv[1], (Context | Get));
    if (e == AL_INVALID)
    {
        Con::errorf(ConsoleLogEntry::General, "alGetString: invalid enum name '%s'", argv[1]);
        return "";
    }

    return (const char*)alGetString(e);
}


//--------------------------------------------------------------------------
// Soundfile
//--------------------------------------------------------------------------
ConsoleFunction(alxGetWaveLen, S32, 2, 2, "(string filename)\n\n"
    "@param filename File to determine length of.\n"
    "@returns Length in milliseconds.")
{
    Resource<AudioBuffer> buffer = AudioBuffer::find(argv[1]);
    if (bool(buffer)) {
        ALuint alBuffer = buffer->getALBuffer();
        return alxGetWaveLen(alBuffer);
    }
    else {
        return 0;
    }
}

//--------------------------------------------------------------------------
// Source
//--------------------------------------------------------------------------
ConsoleFunction(alxCreateSource, S32, 2, 6,
    "(profile) or "
    "(profile, x,y,z) or "
    "(description, filename) or "
    "(description, filename, x,y,z)")
{
    AudioDescription* description = NULL;
    AudioProfile* profile = dynamic_cast<AudioProfile*>(Sim::findObject(argv[1]));
    if (profile == NULL)
    {
        description = dynamic_cast<AudioDescription*>(Sim::findObject(argv[1]));
        if (description == NULL)
        {
            Con::printf("Unable to locate audio profile/description '%s'", argv[1]);
            return NULL_AUDIOHANDLE;
        }
    }

    if (profile)
    {
        if (argc == 2)
            return alxCreateSource(profile);

        MatrixF transform;
        transform.set(EulerF(0, 0, 0), Point3F(dAtof(argv[2]), dAtof(argv[3]), dAtof(argv[4])));
        return alxCreateSource(profile, &transform);
    }

    if (description)
    {

        if (argc == 3)
            return alxCreateSource(description, argv[2]);

        MatrixF transform;
        transform.set(EulerF(0, 0, 0), Point3F(dAtof(argv[3]), dAtof(argv[4]), dAtof(argv[5])));
        return alxCreateSource(description, argv[2], &transform);
    }

    return NULL_AUDIOHANDLE;
}


//-----------------------------------------------
ConsoleFunction(alxSourcef, void, 4, 4, "(handle, ALenum, value)")
{
    ALenum e = getEnum(argv[2], (Source | Set | Float));
    if (e == AL_INVALID)
    {
        Con::errorf(ConsoleLogEntry::General, "cAudio_alxSourcef: invalid enum name '%s'", argv[2]);
        return;
    }

    alxSourcef(dAtoi(argv[1]), e, dAtof(argv[3]));
}


//-----------------------------------------------
ConsoleFunction(alxSource3f, void, 3, 6, "(handle, ALenum, x, y, z)\n\n"
    "@note You can replace the last three parameters with a string, "
    "\"x y z\"")
{
    ALenum e = getEnum(argv[2], (Source | Set | Float3));
    if (e == AL_INVALID)
    {
        Con::errorf(ConsoleLogEntry::General, "cAudio_alxSource3f: invalid enum name '%s'", argv[2]);
        return;
    }

    if ((argc != 4 && argc != 6))
    {
        Con::errorf(ConsoleLogEntry::General, "cAudio_alxSource3f: wrong number of args");
        return;
    }

    Point3F pos;
    if (argc == 4)
        dSscanf(argv[3], "%g %g %g", &pos.x, &pos.y, &pos.z);
    else
    {
        pos.x = dAtof(argv[3]);
        pos.y = dAtof(argv[4]);
        pos.z = dAtof(argv[5]);
    }

    alxSource3f(dAtoi(argv[1]), e, pos.x, pos.y, pos.z);
}


//-----------------------------------------------
ConsoleFunction(alxSourcei, void, 4, 4, "(handle, ALenum, value)")
{
    ALenum e = getEnum(argv[2], (Source | Set | Int));
    if (e == AL_INVALID)
    {
        Con::errorf(ConsoleLogEntry::General, "cAudio_alxSourcei: invalid enum name '%s'", argv[2]);
        return;
    }

    alxSourcei(dAtoi(argv[1]), e, dAtoi(argv[3]));
}


//-----------------------------------------------
ConsoleFunction(alxGetSourcef, F32, 3, 3, "(handle, ALenum)")
{
    ALenum e = getEnum(argv[2], (Source | Get | Float));
    if (e == AL_INVALID)
    {
        Con::errorf(ConsoleLogEntry::General, "cAudio_alxGetSourcef: invalid enum name '%s'", argv[2]);
        return(0.f);
    }

    F32 value;
    alxGetSourcef(dAtoi(argv[1]), e, &value);
    return(value);
}


//-----------------------------------------------
ConsoleFunction(alxGetSource3f, const char*, 3, 3, "(handle, ALenum)")
{
    ALenum e = getEnum(argv[2], (Source | Get | Float));
    if (e == AL_INVALID)
    {
        Con::errorf(ConsoleLogEntry::General, "cAudio_alxGetSource3f: invalid enum name '%s'", argv[2]);
        return("0 0 0");
    }

    F32 value1, value2, value3;
    alxGetSource3f(dAtoi(argv[1]), e, &value1, &value2, &value3);

    char* ret = Con::getReturnBuffer(64);
    dSprintf(ret, 64, "%7.3f %7.3 %7.3", value1, value2, value3);
    return(ret);
}


//-----------------------------------------------
ConsoleFunction(alxGetSourcei, S32, 3, 3, "(handle, ALenum)")
{
    ALenum e = getEnum(argv[2], (Source | Get | Int));
    if (e == AL_INVALID)
    {
        Con::errorf(ConsoleLogEntry::General, "cAudio_alxGetSourcei: invalid enum name '%s'", argv[2]);
        return(0);
    }

    S32 value;
    alxGetSourcei(dAtoi(argv[1]), e, &value);
    return(value);
}


//-----------------------------------------------
ConsoleFunction(alxPlay, S32, 2, 5, "alxPlay(handle) or "
    "alxPlay(profile) or "
    "alxPlay(profile, x,y,z)")
{

    AudioProfile* profile = dynamic_cast<AudioProfile*>(Sim::findObject(argv[1]));
    if (profile == NULL)
    {
        AUDIOHANDLE handle = dAtoi(argv[1]);
        if (handle != 0)
            return alxPlay(handle);
        else
        {
            Con::printf("Unable to locate audio profile '%s'", argv[1]);
            return NULL_AUDIOHANDLE;
        }
    }

    Point3F pos(0.f, 0.f, 0.f);
    if (argc == 3)
        dSscanf(argv[2], "%g %g %g", &pos.x, &pos.y, &pos.z);
    else if (argc == 5)
        pos.set(dAtof(argv[2]), dAtof(argv[3]), dAtof(argv[4]));

    MatrixF transform;
    transform.set(EulerF(0, 0, 0), pos);

    return alxPlay(profile, &transform, NULL);
}

//-----------------------------------------------
ConsoleFunction(alxStop, void, 2, 2, "(int handle)")
{
    AUDIOHANDLE handle = dAtoi(argv[1]);
    if (handle == NULL_AUDIOHANDLE)
        return;
    alxStop(handle);
}

//-----------------------------------------------
ConsoleFunction(alxStopAll, void, 1, 1, "()")
{
    alxStopAll();
}

//-----------------------------------------------
ConsoleFunction(alxIsPlaying, bool, 2, 5, "alxIsPlaying(handle)")
{
    AUDIOHANDLE handle = dAtoi(argv[1]);
    if (handle == NULL_AUDIOHANDLE)
        return false;
    return alxIsPlaying(handle);
}

//-----------------------------------------------
ConsoleFunction(alxSetMasterVolume, void, 2, 2, "(float volume)")
{
    alxSetMasterVolume(dAtof(argv[1]));
}


//--------------------------------------------------------------------------
// Listener
//--------------------------------------------------------------------------
#ifndef MB_ULTRA
ConsoleFunction(alxListenerf, void, 3, 3, "alxListener(ALenum, value)")
{
    ALenum e = getEnum(argv[1], (Listener | Set | Float));
    if (e == AL_INVALID)
    {
        Con::errorf(ConsoleLogEntry::General, "alxListenerf: invalid enum name '%s'", argv[1]);
        return;
    }

    alxListenerf(e, dAtof(argv[2]));
}
#endif


//-----------------------------------------------
ConsoleFunction(alListener3f, void, 3, 5, "alListener3f(ALenum, \"x y z\") or "
    "alListener3f(ALenum, x, y, z)")
{
    ALenum e = getEnum(argv[1], (Listener | Set | Float3));
    if (e == AL_INVALID)
    {
        Con::errorf(ConsoleLogEntry::General, "alListener3f: invalid enum name '%s'", argv[1]);
        return;
    }

    if (argc != 3 || argc != 5)
    {
        Con::errorf(ConsoleLogEntry::General, "alListener3f: wrong number of arguments");
        return;
    }

    Point3F pos;
    if (argc == 3)
        dSscanf(argv[2], "%g %g %g", &pos.x, &pos.y, &pos.z);
    else
    {
        pos.x = dAtof(argv[2]);
        pos.y = dAtof(argv[3]);
        pos.z = dAtof(argv[4]);
    }

    alListener3f(e, pos.x, pos.y, pos.z);
}


//-----------------------------------------------
ConsoleFunction(alxGetListenerf, F32, 2, 2, "alxGetListenerf(Alenum)")
{
    ALenum e = getEnum(argv[1], (Source | Get | Float));
    if (e == AL_INVALID)
    {
        Con::errorf(ConsoleLogEntry::General, "alxGetListenerf: invalid enum name '%s'", argv[1]);
        return(0.f);
    }

    F32 value;
    alxGetListenerf(e, &value);
    return(value);
}


//-----------------------------------------------
ConsoleFunction(alGetListener3f, const char*, 2, 2, "alGetListener3f(Alenum)")
{
    ALenum e = getEnum(argv[2], (Source | Get | Float));
    if (e == AL_INVALID)
    {
        Con::errorf(ConsoleLogEntry::General, "alGetListener3f: invalid enum name '%s'", argv[1]);
        return("0 0 0");
    }

    F32 value1, value2, value3;
    alGetListener3f(e, &value1, &value2, &value3);

    char* ret = Con::getReturnBuffer(64);
    dSprintf(ret, 64, "%7.3f %7.3 %7.3", value1, value2, value3);
    return(ret);
}


//-----------------------------------------------
ConsoleFunction(alGetListeneri, S32, 2, 2, "alGetListeneri(Alenum)")
{
    ALenum e = getEnum(argv[1], (Source | Get | Int));
    if (e == AL_INVALID)
    {
        Con::errorf(ConsoleLogEntry::General, "alGetListeneri: invalid enum name '%s'", argv[1]);
        return(0);
    }

    S32 value;
    alGetListeneri(e, &value);
    return(value);
}


//--------------------------------------------------------------------------
// Channel Volumes
//--------------------------------------------------------------------------
ConsoleFunction(alxGetChannelVolume, F32, 2, 2, "(int channel_id)\n\n"
    "@param  channel_id  ID of channel to fetch volume from.\n"
    "@return Volume of channel.")
{
    U32 type = dAtoi(argv[1]);
    if (type >= Audio::NumAudioTypes)
    {
        Con::errorf(ConsoleLogEntry::General, "alxGetChannelVolume: invalid channel '%d'", dAtoi(argv[1]));
        return(0.f);
    }

    return(mAudioTypeVolume[type]);
}

//-----------------------------------------------

ConsoleFunction(alxSetChannelVolume, bool, 3, 3, "(int channel_id, float volume)\n\n"
    "@param channel_id  ID of channel to set volume on.\n"
    "@param volume      New volume of channel, from 0.0f-1.0f"
)
{
    U32 type = dAtoi(argv[1]);
    F32 volume = mClampF(dAtof(argv[2]), 0.f, 1.f);

    if (type >= Audio::NumAudioTypes)
    {
        Con::errorf(ConsoleLogEntry::General, "alxSetChannelVolume: channel '%d' out of range [0, %d]", dAtoi(argv[1]), Audio::NumAudioTypes);
        return false;
    }

    mAudioTypeVolume[type] = volume;
    alxUpdateTypeGain(type);
    return true;
}

//-----------------------------------------------
ConsoleFunction(alxGetStreamPosition, F32, 2, 2, "alxGetStreamPosition(handle)")
{
    AUDIOHANDLE handle = dAtoi(argv[1]);

    if (handle == NULL_AUDIOHANDLE)
        return -1;

    return alxGetStreamPosition(handle);
}

//-----------------------------------------------
ConsoleFunction(alxGetStreamDuration, F32, 2, 2, "alxGetStreamDuration(handle)")
{
    AUDIOHANDLE handle = dAtoi(argv[1]);

    if (handle == NULL_AUDIOHANDLE)
        return -1;

    return alxGetStreamDuration(handle);
}


ConsoleFunctionGroupEnd(Audio);
