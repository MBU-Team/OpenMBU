//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "particle.h"
#include "console/consoleTypes.h"
#include "core/bitStream.h"
#include "math/mRandom.h"

static ParticleData gDefaultParticleData;


IMPLEMENT_CO_DATABLOCK_V1(ParticleData);

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
ParticleData::ParticleData()
{
    dragCoefficient = 0.0f;
    windCoefficient = 1.0f;
    gravityCoefficient = 0.0f;
    inheritedVelFactor = 0.0f;
    constantAcceleration = 0.0f;
    lifetimeMS = 1000;
    lifetimeVarianceMS = 0;
    spinSpeed = 0.0;
    spinRandomMin = 0.0;
    spinRandomMax = 0.0;
    useInvAlpha = false;
    animateTexture = false;

    numFrames = 1;
    framesPerSec = numFrames;

    S32 i;
    for (i = 0; i < PDC_NUM_KEYS; i++)
    {
        colors[i].set(1.0, 1.0, 1.0, 1.0);
        sizes[i] = 1.0;
    }

    times[0] = 0.0f;
    times[1] = 1.0f;
    times[2] = 2.0f;
    times[3] = 2.0f;

    dMemset(textureNameList, 0, sizeof(textureNameList));
    dMemset(textureList, 0, sizeof(textureList));

}

//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------
ParticleData::~ParticleData()
{
}

//-----------------------------------------------------------------------------
// initPersistFields
//-----------------------------------------------------------------------------
void ParticleData::initPersistFields()
{
    Parent::initPersistFields();

    addField("dragCoefficient", TypeF32, Offset(dragCoefficient, ParticleData));
    addField("windCoefficient", TypeF32, Offset(windCoefficient, ParticleData));
    addField("gravityCoefficient", TypeF32, Offset(gravityCoefficient, ParticleData));
    addField("inheritedVelFactor", TypeF32, Offset(inheritedVelFactor, ParticleData));
    addField("constantAcceleration", TypeF32, Offset(constantAcceleration, ParticleData));
    addField("lifetimeMS", TypeS32, Offset(lifetimeMS, ParticleData));
    addField("lifetimeVarianceMS", TypeS32, Offset(lifetimeVarianceMS, ParticleData));
    addField("spinSpeed", TypeF32, Offset(spinSpeed, ParticleData));
    addField("spinRandomMin", TypeF32, Offset(spinRandomMin, ParticleData));
    addField("spinRandomMax", TypeF32, Offset(spinRandomMax, ParticleData));
    addField("useInvAlpha", TypeBool, Offset(useInvAlpha, ParticleData));
    addField("animateTexture", TypeBool, Offset(animateTexture, ParticleData));
    addField("framesPerSec", TypeS32, Offset(framesPerSec, ParticleData));

    addField("textureName", TypeFilename, Offset(textureNameList, ParticleData), PDC_MAX_TEX);
    addField("animTexName", TypeFilename, Offset(textureNameList, ParticleData), PDC_MAX_TEX);

    // Interpolation variables
    addField("colors", TypeColorF, Offset(colors, ParticleData), PDC_NUM_KEYS);
    addField("sizes", TypeF32, Offset(sizes, ParticleData), PDC_NUM_KEYS);
    addField("times", TypeF32, Offset(times, ParticleData), PDC_NUM_KEYS);
}

//-----------------------------------------------------------------------------
// Pack data
//-----------------------------------------------------------------------------
void ParticleData::packData(BitStream* stream)
{
    Parent::packData(stream);

    stream->writeFloat(dragCoefficient / 5, 10);
    if (stream->writeFlag(windCoefficient != gDefaultParticleData.windCoefficient))
        stream->write(windCoefficient);
    stream->writeSignedFloat(gravityCoefficient / 10, 12);
    stream->writeFloat(inheritedVelFactor, 9);
    if (stream->writeFlag(constantAcceleration != gDefaultParticleData.constantAcceleration))
        stream->write(constantAcceleration);

    stream->write(lifetimeMS);
    stream->write(lifetimeVarianceMS);

    if (stream->writeFlag(spinSpeed != gDefaultParticleData.spinSpeed))
        stream->write(spinSpeed);
    if (stream->writeFlag(spinRandomMin != gDefaultParticleData.spinRandomMin || spinRandomMax != gDefaultParticleData.spinRandomMax))
    {
        stream->writeInt((S32)(spinRandomMin + 1000), 11);
        stream->writeInt((S32)(spinRandomMax + 1000), 11);
    }
    stream->writeFlag(useInvAlpha);

    S32 i, count;

    // see how many frames there are:
    for (count = 0; count < 3; count++)
        if (times[count] >= 1)
            break;

    count++;

    stream->writeInt(count - 1, 2);

    for (i = 0; i < count; i++)
    {
        stream->writeFloat(colors[i].red, 7);
        stream->writeFloat(colors[i].green, 7);
        stream->writeFloat(colors[i].blue, 7);
        stream->writeFloat(colors[i].alpha, 7);
        stream->writeFloat(sizes[i] / MaxParticleSize, 14);
        stream->writeFloat(times[i], 8);
    }

    for (count = 0; count < PDC_MAX_TEX; count++)
        if (!textureNameList[count])
            break;
    stream->writeInt(count, 6);
    for (i = 0; i < count; i++)
        stream->writeString(textureNameList[i]);
}

//-----------------------------------------------------------------------------
// Unpack data
//-----------------------------------------------------------------------------
void ParticleData::unpackData(BitStream* stream)
{
    Parent::unpackData(stream);

    dragCoefficient = stream->readFloat(10) * 5;
    if (stream->readFlag())
        stream->read(&windCoefficient);
    else
        windCoefficient = gDefaultParticleData.windCoefficient;
    gravityCoefficient = stream->readSignedFloat(12) * 10;
    inheritedVelFactor = stream->readFloat(9);
    if (stream->readFlag())
        stream->read(&constantAcceleration);
    else
        constantAcceleration = gDefaultParticleData.constantAcceleration;

    stream->read(&lifetimeMS);
    stream->read(&lifetimeVarianceMS);

    if (stream->readFlag())
        stream->read(&spinSpeed);
    else
        spinSpeed = gDefaultParticleData.spinSpeed;

    if (stream->readFlag())
    {
        spinRandomMin = stream->readInt(11) - 1000;
        spinRandomMax = stream->readInt(11) - 1000;
    }
    else
    {
        spinRandomMin = gDefaultParticleData.spinRandomMin;
        spinRandomMax = gDefaultParticleData.spinRandomMax;
    }

    useInvAlpha = stream->readFlag();

    S32 i;
    S32 count = stream->readInt(2) + 1;
    for (i = 0; i < count; i++)
    {
        colors[i].red = stream->readFloat(7);
        colors[i].green = stream->readFloat(7);
        colors[i].blue = stream->readFloat(7);
        colors[i].alpha = stream->readFloat(7);
        sizes[i] = stream->readFloat(14) * MaxParticleSize;
        times[i] = stream->readFloat(8);
    }
    count = stream->readInt(6);
    for (i = 0; i < count; i++)
        textureNameList[i] = stream->readSTString();
}

//-----------------------------------------------------------------------------
// onAdd
//-----------------------------------------------------------------------------
bool ParticleData::onAdd()
{
    if (Parent::onAdd() == false)
        return false;

    if (dragCoefficient < 0.0) {
        Con::warnf(ConsoleLogEntry::General, "ParticleData(%s) drag coeff less than 0", getName());
        dragCoefficient = 0.0f;
    }
    if (lifetimeMS < 1) {
        Con::warnf(ConsoleLogEntry::General, "ParticleData(%s) lifetime < 1 ms", getName());
        lifetimeMS = 1;
    }
    if (lifetimeVarianceMS >= lifetimeMS) {
        Con::warnf(ConsoleLogEntry::General, "ParticleData(%s) lifetimeVariance >= lifetime", getName());
        lifetimeVarianceMS = lifetimeMS - 1;
    }
    if (spinSpeed > 10000.0 || spinSpeed < -10000.0) {
        Con::warnf(ConsoleLogEntry::General, "ParticleData(%s) spinSpeed invalid", getName());
        return false;
    }
    if (spinRandomMin > 10000.0 || spinRandomMin < -10000.0) {
        Con::warnf(ConsoleLogEntry::General, "ParticleData(%s) spinRandomMin invalid", getName());
        spinRandomMin = -360.0;
        return false;
    }
    if (spinRandomMin > spinRandomMax) {
        Con::warnf(ConsoleLogEntry::General, "ParticleData(%s) spinRandomMin greater than spinRandomMax", getName());
        spinRandomMin = spinRandomMax - (spinRandomMin - spinRandomMax);
        return false;
    }
    if (spinRandomMax > 10000.0 || spinRandomMax < -10000.0) {
        Con::warnf(ConsoleLogEntry::General, "ParticleData(%s) spinRandomMax invalid", getName());
        spinRandomMax = 360.0;
        return false;
    }
    if (numFrames > PDC_MAX_TEX) {
        Con::warnf(ConsoleLogEntry::General, "ParticleData(%s) numFrames invalid", getName());
        numFrames = PDC_MAX_TEX;
        return false;
    }
    if (framesPerSec > 200) {
        Con::warnf(ConsoleLogEntry::General, "ParticleData(%s) framesPerSec invalid", getName());
        framesPerSec = 20;
        return false;
    }

    times[0] = 0.0f;
    for (U32 i = 1; i < 4; i++) {
        if (times[i] < times[i - 1]) {
            Con::warnf(ConsoleLogEntry::General, "ParticleData(%s) times[%d] < times[%d]", getName(), i, i - 1);
            times[i] = times[i - 1];
        }
    }

    return true;
}

//-----------------------------------------------------------------------------
// preload
//-----------------------------------------------------------------------------
bool ParticleData::preload(bool server, char errorBuffer[256])
{
    if (Parent::preload(server, errorBuffer) == false)
        return false;

    bool error = false;
    if (!server)
    {
        numFrames = 0;
        for (int i = 0; i < PDC_MAX_TEX; i++)
        {
            if (textureNameList[i] && textureNameList[i][0])
            {
                textureList[i] = GFXTexHandle(textureNameList[i], &GFXDefaultStaticDiffuseProfile);
                if (!textureList[i])
                {
                    dSprintf(errorBuffer, 256, "Missing particle texture: %s", textureNameList[i]);
                    error = true;
                }
                numFrames++;
            }
        }
    }

    return !error;
}

//-----------------------------------------------------------------------------
// Initialize particle
//-----------------------------------------------------------------------------
void ParticleData::initializeParticle(Particle* init, const Point3F& inheritVelocity)
{
    init->dataBlock = this;

    // Calculate the constant accleration...
    init->vel += inheritVelocity * inheritedVelFactor;
    init->acc = init->vel * constantAcceleration;

    // Calculate this instance's lifetime...
    init->totalLifetime = lifetimeMS;
    if (lifetimeVarianceMS != 0)
        init->totalLifetime += S32(gRandGen.randI() % (2 * lifetimeVarianceMS + 1)) - S32(lifetimeVarianceMS);

    // assign spin amount
    init->spinSpeed = spinSpeed + gRandGen.randF(spinRandomMin, spinRandomMax);
}

