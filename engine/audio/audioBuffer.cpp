//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platformAL.h"
#include "audio/audioBuffer.h"
#include "core/stream.h"
#include "console/console.h"
#include "core/frameAllocator.h"

#ifndef TORQUE_NO_OGGVORBIS
#include "vorbis/codec.h"
#endif

#if defined(TORQUE_BIG_ENDIAN)
#define ENDIAN 1
#else
#define ENDIAN 0
#endif

//#define LOG_SOUND_LOADS

/// WAV File-header
struct WAVFileHdr
{
    ALubyte  id[4];
    ALsizei  size;
    ALubyte  type[4];
};

//// WAV Fmt-header
struct WAVFmtHdr
{
    ALushort format;
    ALushort channels;
    ALuint   samplesPerSec;
    ALuint   bytesPerSec;
    ALushort blockAlign;
    ALushort bitsPerSample;
};

/// WAV FmtEx-header
struct WAVFmtExHdr
{
    ALushort size;
    ALushort samplesPerBlock;
};

/// WAV Smpl-header
struct WAVSmplHdr
{
    ALuint   manufacturer;
    ALuint   product;
    ALuint   samplePeriod;
    ALuint   note;
    ALuint   fineTune;
    ALuint   SMPTEFormat;
    ALuint   SMPTEOffest;
    ALuint   loops;
    ALuint   samplerData;
    struct
    {
        ALuint identifier;
        ALuint type;
        ALuint start;
        ALuint end;
        ALuint fraction;
        ALuint count;
    }      loop[1];
};

/// WAV Chunk-header
struct WAVChunkHdr
{
    ALubyte  id[4];
    ALuint   size;
};

#define CHUNKSIZE 4096



//--------------------------------------
AudioBuffer::AudioBuffer(StringTableEntry filename)
{
    AssertFatal(StringTable->lookup(filename), "AudioBuffer:: filename is not a string table entry");

    mFilename = filename;
    mLoading = false;
    malBuffer = 0;
}

AudioBuffer::~AudioBuffer()
{
    if (malBuffer != 0) {
        alDeleteBuffers(1, &malBuffer);
    }
}

//--------------------------------------
Resource<AudioBuffer> AudioBuffer::find(const char* filename)
{
    U32 mark = FrameAllocator::getWaterMark();
    char* f2 = NULL;

    Resource<AudioBuffer> buffer = ResourceManager->load(filename);
    if (bool(buffer) == false)
    {
        // wav file doesn't exist, try ogg file instead
        S32 len = dStrlen(filename);
        if (len > 3 && !dStricmp(filename + len - 4, ".wav"))
        {
            f2 = (char*)FrameAllocator::alloc(len + 1);
            dStrcpy(f2, filename);
            f2[len - 3] = 'o';
            f2[len - 2] = 'g';
            f2[len - 1] = 'g';
            buffer = ResourceManager->load(filename);
        }
    }

    // if resource still not there, try to create it if file exists
    if (bool(buffer) == false)
    {
        // see if the file exists -- first try default, then try ogg
        if (ResourceManager->getPathOf(filename))
        {
            AudioBuffer* temp = new AudioBuffer(StringTable->insert(filename));
            ResourceManager->add(filename, temp);
            buffer = ResourceManager->load(filename);
        }
        else if (f2 && ResourceManager->getPathOf(f2))
        {
            AudioBuffer* temp = new AudioBuffer(StringTable->insert(f2));
            ResourceManager->add(f2, temp);
            buffer = ResourceManager->load(f2);
        }
    }

    FrameAllocator::setWaterMark(mark);
    return buffer;
}

ResourceInstance* AudioBuffer::construct(Stream&)
{
    return NULL;
}

//-----------------------------------------------------------------
ALuint AudioBuffer::getALBuffer()
{
    if (!alcGetCurrentContext())
        return 0;

    // clear the error state
    alGetError();

    // Intangir> fix for newest openAL from creative (it returns true, yea right 0 is not a valid buffer)
    // it MIGHT not work at all for all i know.
    if (malBuffer && alIsBuffer(malBuffer))
        return malBuffer;

    alGenBuffers(1, &malBuffer);
    if (alGetError() != AL_NO_ERROR)
        return 0;

    ResourceObject* obj = ResourceManager->find(mFilename);
    if (obj)
    {
        bool readSuccess = false;
        S32 len = dStrlen(mFilename);

        if (len > 3 && !dStricmp(mFilename + len - 4, ".wav"))
        {
#ifdef LOG_SOUND_LOADS
            Con::printf("Reading WAV: %s\n", mFilename);
#endif
            readSuccess = readWAV(obj);
        }
#ifndef TORQUE_NO_OGGVORBIS
        else if (len > 3 && !dStricmp(mFilename + len - 4, ".ogg"))
        {
#ifdef LOG_SOUND_LOADS
            Con::printf("Reading Ogg: %s\n", mFilename);
#endif
            readSuccess = readOgg(obj);
        }
#endif
        if (readSuccess)
            return(malBuffer);
    }

    alDeleteBuffers(1, &malBuffer);
    return 0;
}

/*!   The Read a WAV file from the given ResourceObject and initialize
      an alBuffer with it.
*/
bool AudioBuffer::readWAV(ResourceObject* obj)
{
    WAVChunkHdr chunkHdr;
    WAVFmtExHdr fmtExHdr;
    WAVFileHdr  fileHdr;
    WAVSmplHdr  smplHdr;
    WAVFmtHdr   fmtHdr;

    ALenum  format = AL_FORMAT_MONO16;
    char* data = NULL;
    ALsizei size = 0;
    ALsizei freq = 22050;
    ALboolean loop = AL_FALSE;

    Stream* stream = ResourceManager->openStream(obj);
    if (!stream)
        return false;

    stream->read(4, &fileHdr.id[0]);
    stream->read(&fileHdr.size);
    stream->read(4, &fileHdr.type[0]);

    fileHdr.size = ((fileHdr.size + 1) & ~1) - 4;

    stream->read(4, &chunkHdr.id[0]);
    stream->read(&chunkHdr.size);
    // unread chunk data rounded up to nearest WORD
    S32 chunkRemaining = chunkHdr.size + (chunkHdr.size & 1);

    while ((fileHdr.size != 0) && (stream->getStatus() != Stream::EOS))
    {
        // WAV Format header
        if (!dStrncmp((const char*)chunkHdr.id, "fmt ", 4))
        {
            stream->read(&fmtHdr.format);
            stream->read(&fmtHdr.channels);
            stream->read(&fmtHdr.samplesPerSec);
            stream->read(&fmtHdr.bytesPerSec);
            stream->read(&fmtHdr.blockAlign);
            stream->read(&fmtHdr.bitsPerSample);

            if (fmtHdr.format == 0x0001)
            {
                format = (fmtHdr.channels == 1 ?
                    (fmtHdr.bitsPerSample == 8 ? AL_FORMAT_MONO8 : AL_FORMAT_MONO16) :
                    (fmtHdr.bitsPerSample == 8 ? AL_FORMAT_STEREO8 : AL_FORMAT_STEREO16));
                freq = fmtHdr.samplesPerSec;
                chunkRemaining -= sizeof(WAVFmtHdr);
            }
            else
            {
                stream->read(sizeof(WAVFmtExHdr), &fmtExHdr);
                chunkRemaining -= sizeof(WAVFmtExHdr);
            }
        }
        // WAV Format header
        else if (!dStrncmp((const char*)chunkHdr.id, "data", 4))
        {
            if (fmtHdr.format == 0x0001)
            {
                size = chunkHdr.size;
                data = new char[chunkHdr.size];
                if (data)
                {
                    stream->read(chunkHdr.size, data);
#if defined(TORQUE_BIG_ENDIAN)
                    // need to endian-flip the 16-bit data.
                    if (fmtHdr.bitsPerSample == 16) // !!!TBD we don't handle stereo, so may be RL flipped.
                    {
                        U16* ds = (U16*)data;
                        U16* de = (U16*)(data + size);
                        while (ds < de)
                        {
                            *ds = convertLEndianToHost(*ds);
                            ds++;
                        }
                    }
#endif
                    chunkRemaining -= chunkHdr.size;
                }
                else
                    break;
            }
            else if (fmtHdr.format == 0x0011)
            {
                //IMA ADPCM
            }
            else if (fmtHdr.format == 0x0055)
            {
                //MP3 WAVE
            }
        }
        // WAV Loop header
        else if (!dStrncmp((const char*)chunkHdr.id, "smpl", 4))
        {
            // this struct read is NOT endian safe but it is ok because
            // we are only testing the loops field against ZERO
            stream->read(sizeof(WAVSmplHdr), &smplHdr);
            loop = (smplHdr.loops ? AL_TRUE : AL_FALSE);
            chunkRemaining -= sizeof(WAVSmplHdr);
        }

        // either we have unread chunk data or we found an unknown chunk type
        // loop and read up to 1K bytes at a time until we have
        // read to the end of this chunk
        char buffer[1024];
        AssertFatal(chunkRemaining >= 0, "AudioBuffer::readWAV: remaining chunk data should never be less than zero.");
        while (chunkRemaining > 0)
        {
            S32 readSize = getMin(1024, chunkRemaining);
            stream->read(readSize, buffer);
            chunkRemaining -= readSize;
        }

        fileHdr.size -= (((chunkHdr.size + 1) & ~1) + 8);

        // read next chunk header...
        stream->read(4, &chunkHdr.id[0]);
        stream->read(&chunkHdr.size);
        // unread chunk data rounded up to nearest WORD
        chunkRemaining = chunkHdr.size + (chunkHdr.size & 1);
    }

    ResourceManager->closeStream(stream);
    if (data)
    {
        alBufferData(malBuffer, format, data, size, freq);
        delete[] data;
        return (alGetError() == AL_NO_ERROR);
    }

    return false;
}

#ifndef TORQUE_NO_OGGVORBIS
/*!   The Read an Ogg Vorbis file from the given ResourceObject and initialize
      an alBuffer with it.
*/
bool AudioBuffer::readOgg(ResourceObject* obj)
{
    OggVorbisFile vf;
    vorbis_info* vi;

    ALenum  format = AL_FORMAT_MONO16;
    char* data = NULL;
    ALsizei size = 0;
    ALsizei freq = 22050;
    ALboolean loop = AL_FALSE;
    int current_section = 0;

#if defined(TORQUE_BIG_ENDIAN)
    int endian = 1;
#else
    int endian = 0;
#endif

    int eof = 0;

    Stream* stream = ResourceManager->openStream(obj);
    if (!stream)
        return false;

    if (vf.ov_open(stream, NULL, 0) < 0) {
        return false;
    }

    //Read Vorbis File Info
    vi = vf.ov_info(-1);
    freq = vi->rate;

    long samples = (long)vf.ov_pcm_total(-1);

    if (vi->channels == 1) {
        format = AL_FORMAT_MONO16;
        size = 2 * samples;
    }
    else {
        format = AL_FORMAT_STEREO16;
        size = 4 * samples;
    }


    data = new char[size];

    if (data)
    {
        long ret = oggRead(&vf, data, size, endian, &current_section);
    }

    /* cleanup */
    vf.ov_clear();

    ResourceManager->closeStream(stream);
    if (data)
    {
        alBufferData(malBuffer, format, data, size, freq);
        delete[] data;
        return (alGetError() == AL_NO_ERROR);
    }

    return false;
}

// ov_read() only returns a maximum of one page worth of data
// this helper function will repeat the read until buffer is full
long AudioBuffer::oggRead(OggVorbisFile* vf, char* buffer, int length,
    int bigendianp, int* bitstream)
{
    long bytesRead = 0;
    long totalBytes = 0;
    long offset = 0;
    long bytesToRead = 0;
    //while((offset + CHUNKSIZE) < length) {
    while ((offset) < length)
    {
        if ((length - offset) < CHUNKSIZE)
            bytesToRead = length - offset;
        else
            bytesToRead = CHUNKSIZE;

        bytesRead = vf->ov_read(buffer, bytesToRead, bigendianp, bitstream);
        if (bytesRead <= 0)
            break;
        offset += bytesRead;
        buffer += bytesRead;
    }
    return offset;
}
#endif
