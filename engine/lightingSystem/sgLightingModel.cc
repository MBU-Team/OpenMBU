//-----------------------------------------------
// Synapse Gaming - Lighting System
// Copyright © Synapse Gaming 2003
// Written by John Kabus
//-----------------------------------------------
#include "lightingSystem/sgLighting.h"
#include "gfx/gfxDevice.h"
#include "lightingSystem/sgLightingModel.h"
#include "sceneGraph/lightManager.h"

//#define SG_TEST_LIGHTING_TEXTURE

#define SG_LIGHTING_MIN_CUTOFF					0.00392f
#define SG_DYNAMIC_LIGHTING_TEXTURE_SIZE		16
#define SG_DYNAMIC_LIGHTING_TEXTURE_HALFSIZE	F32(SG_DYNAMIC_LIGHTING_TEXTURE_SIZE / 2)

Vector<sgLightingModel*> sgLightingModelManager::sgLightingModels;
sgLightingModelStock sgLightingModelManager::sgDefaultModel;
sgLightingModelAdvanced sgLightingModelManager::sgAdvancedModel;
sgLightingModelInverseSquare sgLightingModelManager::sgInverseSquare;
sgLightingModelInverseSquareFastFalloff sgLightingModelManager::sgInverseSquareFastFalloff;
sgLightingModelNearLinear sgLightingModelManager::sgNearLinear;
sgLightingModelNearLinearFastFalloff sgLightingModelManager::sgNearLinearFastFalloff;

sgLightingModel* sgLightingModelManager::sgSunlightModel = &sgLightingModelManager::sgAdvancedModel;


#ifdef SG_TEST_LIGHTING_TEXTURE
GFXTexHandle sgTestLightingTexture;
#endif


ConsoleFunction(sgLightingModelCount, S32, 1, 1, "Get the number of Lighting Pack Lighting Models.")
{
    return sgLightingModelManager::sgGetLightingModelCount();
}

ConsoleFunction(sgLightingModelName, const char*, 2, 2, "Get the name of the Lighting Pack Lighting Model.")
{
    U32 index;
    index = dAtoi(argv[1]);
    return sgLightingModelManager::sgGetLightingModelName(index);
}

//-----------------------------------------------
//-----------------------------------------------

sgLightingModel::sgLightingModel()
{
    sgStateSet = false;
    sgStateInitLM = false;
    sgLight = NULL;
    sgLightingModelName[0] = 0;
    sgGeneratedDynamicLightingTexture = false;
    sgLightingModelManager::sgRegisterLightingModel(this);
}

GFXTexHandle sgLightingModel::sgGetDynamicLightingTextureOmni()
{
    // load these on demand...
    if (!sgGeneratedDynamicLightingTexture)
    {
        sgGenerateDynamicLightingTexture();
        sgGeneratedDynamicLightingTexture = true;
    }
    return sgDynamicLightingTextureOmni;
}

GFXTexHandle sgLightingModel::sgGetDynamicLightingTextureSpot()
{
    // load these on demand...
    if (!sgGeneratedDynamicLightingTexture)
    {
        sgGenerateDynamicLightingTexture();
        sgGeneratedDynamicLightingTexture = true;
    }
    return sgDynamicLightingTextureSpot;
}
/*
#ifndef SG_DYNAMICLIGHTING_TYPE2

void sgLightingModel::sgGenerateDynamicLightingTexture()
{
    LightInfo *oldlight = sgLight;

    // setup basic light...
    LightInfo light;
    light.mType = LightInfo::Point;
    light.mRadius = 10.0;
    light.mColor = ColorF(1.0, 1.0, 1.0);
    light.sgLocalAmbientAmount = 1.0;
    light.sgDoubleSidedAmbient = true;
    light.sgUseNormals = false;

    // set state...
    sgResetState();
    sgSetState(&light);
    sgInitStateLM();

    // get the basics...
    F32 dist = sgGetMaxRadius();
    Point3F halfamount(SG_DYNAMIC_LIGHTING_TEXTURE_HALFSIZE,
        SG_DYNAMIC_LIGHTING_TEXTURE_HALFSIZE, SG_DYNAMIC_LIGHTING_TEXTURE_HALFSIZE);
    Point3F invhalfamount(1 / SG_DYNAMIC_LIGHTING_TEXTURE_HALFSIZE,
        1 / SG_DYNAMIC_LIGHTING_TEXTURE_HALFSIZE, 1 / SG_DYNAMIC_LIGHTING_TEXTURE_HALFSIZE);
    Point3F radiusdist(dist, dist, dist);

    // misc...
    ColorF diff, attn;
    Point3F norm;

    // fill out the omni...
    U8 bits[SG_DYNAMIC_LIGHTING_TEXTURE_SIZE][SG_DYNAMIC_LIGHTING_TEXTURE_SIZE][SG_DYNAMIC_LIGHTING_TEXTURE_SIZE];
    for(U32 x=0; x<SG_DYNAMIC_LIGHTING_TEXTURE_SIZE; x++)
    {
        for(U32 y=0; y<SG_DYNAMIC_LIGHTING_TEXTURE_SIZE; y++)
        {
            for(U32 z=0; z<SG_DYNAMIC_LIGHTING_TEXTURE_SIZE; z++)
            {
                Point3F pos(x, y, z);
                pos -= halfamount;
                pos *= invhalfamount;
                pos *= radiusdist;

                if(x == 8 && y == 8 && z == 8)
                    x = y;

                diff = attn = ColorF(0.0f, 0.0f, 0.0f);
                norm = VectorF(0.0f, 0.0f, 0.0f);
                sgLightingLM(pos, SG_STATIC_SPOT_VECTOR_NORMALIZED, diff, attn, norm);

                bits[x][y][z] = mClampF((attn.red * 255.f), 0.0f, 255.0f);
            }
        }
    }

    // upload...
    sgDynamicLightingTextureOmni = GFX->mTextureManager->createTexture(SG_DYNAMIC_LIGHTING_TEXTURE_SIZE,
        SG_DYNAMIC_LIGHTING_TEXTURE_SIZE, SG_DYNAMIC_LIGHTING_TEXTURE_SIZE,
        bits, GFXFormatL8, &GFXDefaultStaticDiffuseProfile);

    // change to spot...
    light.mDirection = VectorF(-1.0, 0.0, 0.0);
    light.mType = LightInfo::SGStaticSpot;
    light.sgSpotAngle = 90;

    // set lighting model to spot...
    sgResetState();
    sgSetState(&light);
    sgInitStateLM();

    // fill out the spot...
    for(U32 x=0; x<SG_DYNAMIC_LIGHTING_TEXTURE_SIZE; x++)
    {
        for(U32 y=0; y<SG_DYNAMIC_LIGHTING_TEXTURE_SIZE; y++)
        {
            for(U32 z=0; z<SG_DYNAMIC_LIGHTING_TEXTURE_SIZE; z++)
            {
                Point3F pos(x, y, z);
                pos -= halfamount;
                pos *= invhalfamount;
                pos *= radiusdist;

                diff = attn = ColorF(0.0f, 0.0f, 0.0f);
                norm = VectorF(0.0f, 0.0f, 0.0f);
                sgLightingLM(pos, SG_STATIC_SPOT_VECTOR_NORMALIZED, diff, attn, norm);

                bits[x][y][z] = mClampF((attn.red * 255.f), 0.0f, 255.0f);
            }
        }
    }

    // upload...
    sgDynamicLightingTextureSpot = GFX->mTextureManager->createTexture(SG_DYNAMIC_LIGHTING_TEXTURE_SIZE,
        SG_DYNAMIC_LIGHTING_TEXTURE_SIZE, SG_DYNAMIC_LIGHTING_TEXTURE_SIZE,
        bits, GFXFormatL8, &GFXDefaultStaticDiffuseProfile);


#ifdef SG_TEST_LIGHTING_TEXTURE
    GBitmap *bitmap = new GBitmap(1, 1);
    U8 *bits = bitmap->getWritableBits();
    bits[0] = 255;
    bits[1] = 128;
    bits[2] = 128;
    sgTestLightingTexture = GFXTexHandle(bitmap, &GFXDefaultStaticDiffuseProfile, true);

    sgDynamicLightingTextureOmni = sgTestLightingTexture;
    sgDynamicLightingTextureSpot = sgTestLightingTexture;
#endif


    // back to the original state...
    sgResetState();
    sgSetState(oldlight);
}

#else
*/
void sgLightingModel::sgGenerateDynamicLightingTexture()
{
    LightInfo* oldlight = sgLight;

    // setup basic light...
    LightInfo light;
    light.mType = LightInfo::Point;
    light.mRadius = 10.0;
    light.mColor = ColorF(1.0, 1.0, 1.0);
    light.sgLocalAmbientAmount = 1.0;
    light.sgDoubleSidedAmbient = true;
    light.sgUseNormals = false;

    // set state...
    sgResetState();
    sgSetState(&light);
    sgInitStateLM();

    // get the basics...
    F32 dist = sgGetMaxRadius(true);
    Point3F halfamount(SG_DYNAMIC_LIGHTING_TEXTURE_HALFSIZE,
        SG_DYNAMIC_LIGHTING_TEXTURE_HALFSIZE, SG_DYNAMIC_LIGHTING_TEXTURE_HALFSIZE);
    Point3F invhalfamount(1 / SG_DYNAMIC_LIGHTING_TEXTURE_HALFSIZE,
        1 / SG_DYNAMIC_LIGHTING_TEXTURE_HALFSIZE, 1 / SG_DYNAMIC_LIGHTING_TEXTURE_HALFSIZE);
    Point3F radiusdist(dist, dist, dist);

    // misc...
    ColorF diff, attn;
    Point3F norm;

    // fill out the omni...
    U8 bits[SG_DYNAMIC_LIGHTING_TEXTURE_SIZE][SG_DYNAMIC_LIGHTING_TEXTURE_SIZE][SG_DYNAMIC_LIGHTING_TEXTURE_SIZE];
    for (U32 x = 0; x < SG_DYNAMIC_LIGHTING_TEXTURE_SIZE; x++)
    {
        for (U32 y = 0; y < SG_DYNAMIC_LIGHTING_TEXTURE_SIZE; y++)
        {
            for (U32 z = 0; z < SG_DYNAMIC_LIGHTING_TEXTURE_SIZE; z++)
            {
                Point3F pos(x, y, z);
                pos += Point3F(0.5f, 0.5f, 0.5f);
                pos -= halfamount;
                pos *= invhalfamount;
                pos *= radiusdist;

                diff = attn = ColorF(0.0f, 0.0f, 0.0f);
                norm = VectorF(0.0f, 0.0f, 0.0f);
                sgLightingLM(pos, SG_STATIC_SPOT_VECTOR_NORMALIZED, diff, attn, norm);

                bits[x][y][z] = mClamp(S32(attn.red * 255.f), S32(0), S32(255));
            }
        }
    }

    // upload...
    sgDynamicLightingTextureOmni = GFX->mTextureManager->createTexture(SG_DYNAMIC_LIGHTING_TEXTURE_SIZE,
        SG_DYNAMIC_LIGHTING_TEXTURE_SIZE, SG_DYNAMIC_LIGHTING_TEXTURE_SIZE,
        bits, GFXFormatL8, &GFXDefaultStaticDiffuseProfile);

    // change to spot...
    light.mDirection = VectorF(-1.0, 0.0, 0.0);
    light.mType = LightInfo::SGStaticSpot;
    light.sgSpotAngle = 90;

    // set lighting model to spot...
    sgResetState();
    sgSetState(&light);
    sgInitStateLM();

    // fill out the spot...
    for (U32 x = 0; x < SG_DYNAMIC_LIGHTING_TEXTURE_SIZE; x++)
    {
        for (U32 y = 0; y < SG_DYNAMIC_LIGHTING_TEXTURE_SIZE; y++)
        {
            for (U32 z = 0; z < SG_DYNAMIC_LIGHTING_TEXTURE_SIZE; z++)
            {
                Point3F pos(x, y, z);
                pos += Point3F(0.5f, 0.5f, 0.5f);
                pos -= halfamount;
                pos *= invhalfamount;
                pos *= radiusdist;

                diff = attn = ColorF(0.0f, 0.0f, 0.0f);
                norm = VectorF(0.0f, 0.0f, 0.0f);
                sgLightingLM(pos, SG_STATIC_SPOT_VECTOR_NORMALIZED, diff, attn, norm);

                bits[x][y][z] = mClamp(S32(attn.red * 255.f), S32(0), S32(255));
            }
        }
    }

    // upload...
    sgDynamicLightingTextureSpot = GFX->mTextureManager->createTexture(SG_DYNAMIC_LIGHTING_TEXTURE_SIZE,
        SG_DYNAMIC_LIGHTING_TEXTURE_SIZE, SG_DYNAMIC_LIGHTING_TEXTURE_SIZE,
        bits, GFXFormatL8, &GFXDefaultStaticDiffuseProfile);


#ifdef SG_TEST_LIGHTING_TEXTURE
    for (U32 x = 0; x < SG_DYNAMIC_LIGHTING_TEXTURE_SIZE; x++)
    {
        for (U32 y = 0; y < SG_DYNAMIC_LIGHTING_TEXTURE_SIZE; y++)
        {
            for (U32 z = 0; z < SG_DYNAMIC_LIGHTING_TEXTURE_SIZE; z++)
            {
                bits[x][y][z] = 255;
            }
        }
    }
    sgTestLightingTexture = GFX->mTextureManager->createTexture(SG_DYNAMIC_LIGHTING_TEXTURE_SIZE,
        SG_DYNAMIC_LIGHTING_TEXTURE_SIZE, SG_DYNAMIC_LIGHTING_TEXTURE_SIZE,
        bits, GFXFormatL8, &GFXDefaultStaticDiffuseProfile);

    sgDynamicLightingTextureOmni = sgTestLightingTexture;
    sgDynamicLightingTextureSpot = sgTestLightingTexture;
#endif


    // back to the original state...
    sgResetState();
    sgSetState(oldlight);
}

//#endif

//-----------------------------------------------
//-----------------------------------------------

F32 sgLightingModelAdvanced::sgScoreLight(LightInfo* light, const SphereF& sphere)
{
    sgLightingModel::sgScoreLight(light, sphere);

    F32 radsq = (light->mRadius * light->mRadius);
    if (radsq <= 0.0f)
        return 0.0f;
    VectorF dist = light->mPos - sphere.center;
    F32 distsq = dist.lenSquared();
    return getMax((1 - (distsq / radsq)), 0.0f);
}

bool sgLightingModelAdvanced::sgCanIlluminate(const Box3F& box)
{
    AssertFatal((sgStateSet == true), "sgLightingModel: State not properly set.");

    // can we skip the hard stuff?
    if ((sgLight->mType == LightInfo::Vector) || (sgLight->mType == LightInfo::Ambient))
        return true;

    // get the min dist...
    Point3F pos = box.getClosestPoint(sgLight->mPos);
    Point3F nor = sgLight->mPos - pos;
    F32 distsq = mDot(nor, nor);

    bool res = (distsq < (sgLight->mRadius* sgLight->mRadius));
    return res;
}

void sgLightingModelAdvanced::sgSetState(LightInfo* light)
{
    sgLightingModelGLBase::sgSetState(light);

    if (sgLight->mRadius > 0.0f)
        sgLinearAttenuation = (6.0f / sgLight->mRadius);
    else
        sgLinearAttenuation = 0.0;
}

void sgLightingModelAdvanced::sgInitStateLM()
{
    sgLightingModelGLBase::sgInitStateLM();

    maxlightdistancesquared = sgLight->mRadius * sgLight->mRadius;
}

void sgLightingModelAdvanced::sgLightingLM(Point3F point, VectorF normal, ColorF& diffuse, ColorF& ambient, Point3F& lightingnormal)
{
    // do not call sgLightingModelGLBase::sgLightingLM!!!
    // we're overriding the method!!!
    sgLightingModel::sgLightingLM(point, normal, diffuse, ambient, lightingnormal);

    Point3F lightvector;
    F32 distsquared;
    F32 spotlightingangle;

    // do this in lightmap...
    //diffuse = ColorF(0.0, 0.0, 0.0);
    //ambient = ColorF(0.0, 0.0, 0.0);
    //lightingnormal = Point3F(0.0, 0.0, 0.0);

    if (sgLight->mType != LightInfo::Vector)
    {
        // get the distance squared from the light to the
        // light map texel world space coord.
        lightvector = sgLight->mPos - point;
        distsquared = lightvector.lenSquared();

        if (distsquared >= maxlightdistancesquared)
            return;
    }
    else
    {
        // directional lighting is always close enough
        // just setup the needed variables...
        lightvector = sgLight->mDirection * -1;
    }

    // step two: find the spotlight amount...

    // vector lights are already normallized...
    if (sgLight->mType != LightInfo::Vector)
        lightvector.normalize();

    lightingnormal = lightvector;

    // is it a spotlight? is it pointing this way?
    if (sgLight->mType == LightInfo::SGStaticSpot)
    {
        spotlightingangle = mDot(lightvector, sgLight->mDirection);
        if (spotlightingangle < spotanglecos)
            return;
    }
    else
        spotlightingangle = 1.0;

    // get the lighting angle amount...
    // values coming from terrain are not normalized...
    normal.normalize();
    F32 angleamount = mDot(lightvector, normal);
    F32 lightamount = 1.0;
    if (sgLight->sgUseNormals)
        lightamount *= angleamount;

    F32 lightfalloff = 1.0;
    lightamount = getMax(getMin(lightamount, 1.0f), 0.0f);

    if (sgLight->mType != LightInfo::Vector)
    {
        // apply one of the point light lighting models...
        lightfalloff = getMax((1 - (distsquared / maxlightdistancesquared)), 0.0f);

        if (sgLight->mType == LightInfo::SGStaticSpot)
        {
            // apply the spotlight lighting model...
            spotlightingangle = mClampF(((spotlightingangle - spotamountouter) / spotamountinner), 0.0f, 1.0f);

            if (sgLight->sgSmoothSpotLight)
                spotlightingangle *= getMin((spotlightingangle * 1.2), 1.0);

            lightfalloff *= spotlightingangle;
        }

        lightamount *= lightfalloff;
    }

    lightamount *= (1 - sgLight->sgLocalAmbientAmount);
    lightamount = getMax(getMin(lightamount, 1.0f), 0.0f);
    diffuse = sgLight->mColor * lightamount;

    if (sgLight->sgDoubleSidedAmbient || (angleamount > 0.0f))
    {
        lightfalloff *= sgLight->sgLocalAmbientAmount;
        lightfalloff = getMax(getMin(lightfalloff, 1.0f), 0.0f);
        ambient = sgLight->mColor * lightfalloff;
    }
}

//-----------------------------------------------
//-----------------------------------------------

F32 sgLightingModelGLBase::sgScoreLight(LightInfo* light, const SphereF& sphere)
{
    sgLightingModel::sgScoreLight(light, sphere);

    if ((sgLight->mType == LightInfo::Vector) || (sgLight->mType == LightInfo::Ambient))
        return 0.5f;

    VectorF vect = light->mPos - sphere.center;
    F32 distsq = vect.lenSquared();
    F32 dist = 0;

    if (sgLinearAttenuation > 0.0f)
    {
        dist = mSqrt(distsq);
    }

    F32 amount = (sgConstantAttenuation + (sgLinearAttenuation * dist) + (sgQuadraticAttenuation * distsq));
    amount = getMax(amount, 0.000001f);
    if (amount != 0.0f)
        amount = 1 / amount;
    amount = getMax(amount, 0.0f);
    return amount;
}

bool sgLightingModelGLBase::sgCanIlluminate(const Box3F& box)
{
    AssertFatal((sgStateSet == true), "sgLightingModel: State not properly set.");

    // can we skip the hard stuff?
    if ((sgLight->mType == LightInfo::Vector) || (sgLight->mType == LightInfo::Ambient))
        return true;

    // get the min dist...
    Point3F pos = box.getClosestPoint(sgLight->mPos);
    Point3F nor = sgLight->mPos - pos;
    F32 distsq = mDot(nor, nor);
    F32 intensity = 0.0f;

    if (distsq > 0.0f)
    {
        // run the basic lighting equation...
        intensity = sgConstantAttenuation;
        intensity += (sgQuadraticAttenuation * distsq);
        if (sgLinearAttenuation > 0.0f)
            intensity += (sgLinearAttenuation * mSqrt(distsq));
    }
    else
    {
        intensity = 1.0f;
    }

    // this seems fine...
    if (intensity <= 0.0f)
        return false;

    intensity = 1.0f / intensity;

    ColorF col = sgLight->mColor * intensity;
    if ((col.red < SG_MIN_LEXEL_INTENSITY) && (col.green < SG_MIN_LEXEL_INTENSITY) &&
        (col.blue < SG_MIN_LEXEL_INTENSITY))
        return false;
    return true;
}

F32 sgLightingModelGLBase::sgGetMaxRadius(bool speedoverquality, bool glstyle)
{
    AssertFatal((sgStateSet == true), "sgLightingModel: State not properly set.");

    if ((sgLight->mType == LightInfo::Vector) || (sgLight->mType == LightInfo::Ambient))
        return 1000000.0f;

    // get started...
    // don't use F32_MAX, this value will be added to stuff...
    const F32 infinity = 1.0e+6F;
    F32 adjustedvalue;
    if (speedoverquality)
        adjustedvalue = (1.0f / SG_MIN_LEXEL_INTENSITY_SPEED_OVER_QUALITY) - sgConstantAttenuation;
    else
        adjustedvalue = (1.0f / SG_MIN_LEXEL_INTENSITY) - sgConstantAttenuation;

    // early out?
    if (adjustedvalue <= 0.0f)
        return 0.0f;

    // this is the default even when L and Q are both populated...
    if (sgLinearAttenuation > 0.0f)
        return adjustedvalue / sgLinearAttenuation;

    if (sgQuadraticAttenuation > 0.0f)
        return mSqrt(adjustedvalue / sgQuadraticAttenuation);

    if (sgConstantAttenuation > 0.0f)
        return infinity;

    return 0.0f;
}

void sgLightingModelGLBase::sgSetState(LightInfo* light)
{
    sgLightingModel::sgSetState(light);

    sgConstantAttenuation = 0;
    sgLinearAttenuation = 0;
    sgQuadraticAttenuation = 0;

    sgLightParamDiffuse = Point4F(light->mColor.red, light->mColor.green, light->mColor.blue, 1.0f);
    sgLightParamAmbient = Point4F(light->mAmbient.red, light->mAmbient.green, light->mAmbient.blue, 1.0f);
    sgLightParamPosition = Point4F(light->mPos.x, light->mPos.y, light->mPos.z, 1.0f);
    sgLightParamDirection = Point4F(-light->mDirection.x, -light->mDirection.y, -light->mDirection.z, 0.0f);
}

void sgLightingModelGLBase::sgLightingGL(S32 gllight)
{
    sgLightingModel::sgLightingGL(gllight);
    /*
    #if (SG_COMPATIBILITY_MODE == SG_COMPATIBILITY_MODE_TGE)
        const GLfloat black[] = {0.0, 0.0, 0.0, 0.0};

        if((sgLight->mType == LightInfo::Vector) || (sgLight->mType == LightInfo::Ambient))
        {
            F32 ambientFactor = (sgLightParamAmbient[0] + sgLightParamAmbient[1] + sgLightParamAmbient[2]) / 3.0f;
            F32 factor = mClampF(1.0 - ambientFactor, 0.f, 1.f);

            // attenuate (for in shadow...)
            sgLightParamDiffuse.x *= factor;
            sgLightParamDiffuse.y *= factor;
            sgLightParamDiffuse.z *= factor;

            glLightfv(gllight, GL_POSITION, (const GLfloat*)&sgLightParamDirection.x);

            // do not add this back in!!!
            // the DX wrapper is a bastard!!!
            //glLightfv(gllight, GL_SPOT_DIRECTION, (const GLfloat*)black);

            glLightfv(gllight, GL_DIFFUSE, (const GLfloat*)&sgLightParamDiffuse.x);
            glLightfv(gllight, GL_AMBIENT, (const GLfloat*)&sgLightParamAmbient.x);
            glLightfv(gllight, GL_SPECULAR, (const GLfloat*)black);

            sgLightManager::sgSetAmbientSelfIllumination(sgLight, &sgLightParamDiffuse.x, &sgLightParamAmbient.x);

            glLightf(gllight, GL_SPOT_CUTOFF, 180.f);

            glLightf(gllight, GL_CONSTANT_ATTENUATION, 0.0f);
            glLightf(gllight, GL_QUADRATIC_ATTENUATION, 0.0f);
            glLightf(gllight, GL_LINEAR_ATTENUATION, 1.0f);
        }
        else
        {
            glLightfv(gllight, GL_POSITION, (const GLfloat*)&sgLightParamPosition.x);

            // must be here and only set spots!!!
            // the DX wrapper is a bastard!!!
            if((sgLight->mType == LightInfo::SGStaticSpot) || (sgLight->mType == LightInfo::Spot))
                glLightfv(gllight, GL_SPOT_DIRECTION, (const GLfloat*)&sgLightParamDirection.x);

            glLightfv(gllight, GL_DIFFUSE, (const GLfloat*)&sgLightParamDiffuse.x);
            glLightfv(gllight, GL_AMBIENT, (const GLfloat*)black);
            glLightfv(gllight, GL_SPECULAR, (const GLfloat*)black);

            if((sgLight->mType == LightInfo::SGStaticSpot) || (sgLight->mType == LightInfo::Spot))
                glLightf(gllight, GL_SPOT_CUTOFF, (sgLight->sgLightInfoData.sgSpotAngle * 0.5));
            else
                glLightf(gllight, GL_SPOT_CUTOFF, 180.f);

            glLightf(gllight, GL_CONSTANT_ATTENUATION, sgConstantAttenuation);
            glLightf(gllight, GL_QUADRATIC_ATTENUATION, sgQuadraticAttenuation);
            glLightf(gllight, GL_LINEAR_ATTENUATION, sgLinearAttenuation);
        }
    #endif*/
}

void sgLightingModelGLBase::sgInitStateLM()
{
    sgLightingModel::sgInitStateLM();

    spotanglecos = mCos(mDegToRad(sgLight->sgSpotAngle * 0.5));
    spotamountinner = 1 - spotanglecos;
    spotamountouter = spotanglecos;
}

void sgLightingModelGLBase::sgLightingLM(Point3F point, VectorF normal, ColorF& diffuse, ColorF& ambient, Point3F& lightingnormal)
{
    sgLightingModel::sgLightingLM(point, normal, diffuse, ambient, lightingnormal);

    F32 anglefalloff = 0;

    // for speed set on exit - wrong! too often unset...
    // do this in lightmap...
    //diffuse = ColorF(0.0, 0.0, 0.0);
    //ambient = ColorF(0.0, 0.0, 0.0);
    //lightingnormal = Point3F(0.0, 0.0, 0.0);

    // get the lighting vector squared...
    if (sgLight->mType == LightInfo::Vector)
        lightingnormal = sgLight->mDirection * -1;
    else
        lightingnormal = sgLight->mPos - point;

    // early out 1 - is surface facing light without double sided ambient (for terrain)?
    anglefalloff = mDot(lightingnormal, normal);
    if ((anglefalloff <= 0.0f) && !(sgLight->sgDoubleSidedAmbient &&
        ((sgLight->sgLocalAmbientAmount >= SG_LIGHTING_MIN_CUTOFF) ||
            (sgLight->mType == LightInfo::Vector))))
    {
        // do this in lightmap...
        //diffuse = ColorF(0.0, 0.0, 0.0);
        //ambient = ColorF(0.0, 0.0, 0.0);
        //lightingnormal = Point3F(0.0, 0.0, 0.0);
        return;
    }

    F32 dist = 0;
    F32 distsq = 0;
    F32 distfalloff = 1.0;
    F32 spotlightangle = 1.0;

    // so far so good - now get the dist falloff...
    if (sgLight->mType != LightInfo::Vector)
    {
        bool didsqrt = false;
        // need this...
        distsq = lightingnormal.lenSquared();
        // only need now for linear...
        if (sgLinearAttenuation > 0.0f)
        {
            dist = mSqrt(distsq);
            didsqrt = true;
        }

        distfalloff = (sgConstantAttenuation + (sgLinearAttenuation * dist) + (sgQuadraticAttenuation * distsq));
        if (distfalloff > 0.0f)
            distfalloff = 1 / distfalloff;
        distfalloff = getMax(distfalloff, 0.0f);

        // early out 2 - too far away?
        if (distfalloff <= SG_LIGHTING_MIN_CUTOFF)
        {
            // do this in lightmap...
            //diffuse = ColorF(0.0, 0.0, 0.0);
            //ambient = ColorF(0.0, 0.0, 0.0);
            //lightingnormal = Point3F(0.0, 0.0, 0.0);
            return;
        }

        // get it now...
        if (!didsqrt)
            dist = mSqrt(distsq);

        // normalize...
        if (dist != 0.0f)
            lightingnormal /= dist;

        // safe to bailout here, ambient only follows spot...
        if (sgLight->mType == LightInfo::SGStaticSpot)
        {
            spotlightangle = mDot(lightingnormal, sgLight->mDirection);
            if (spotlightangle < spotanglecos)
            {
                //diffuse = ColorF(0.0, 0.0, 0.0);
                //ambient = ColorF(0.0, 0.0, 0.0);
                //lightingnormal = Point3F(0.0, 0.0, 0.0);
                return;
            }
        }
    }

    // values coming from terrain are not normalized...
    // lightingnormal is already normalized...
    if ((anglefalloff > 0.0f) && sgLight->sgUseNormals)
    {
        normal.normalize();
        anglefalloff = mDot(lightingnormal, normal);
    }
    anglefalloff = getMax(getMin(anglefalloff, 1.0f), 0.0f);


    if (sgLight->mType == LightInfo::SGStaticSpot)
    {
        // apply the spotlight lighting model...
        spotlightangle = mClampF(((spotlightangle - spotamountouter) / spotamountinner), 0.0f, 1.0f);

        if (sgLight->sgSmoothSpotLight)
            spotlightangle *= getMin((spotlightangle * 1.2), 1.0);
    }


    if ((sgLight->mType == LightInfo::Vector) && (anglefalloff < 0.0f))
        anglefalloff = anglefalloff;


    F32 ambientlightingamount = distfalloff * spotlightangle;

    if (anglefalloff > 0.0f)
    {
        F32 diffuselightingamount = ambientlightingamount;
        if (sgLight->sgUseNormals)
            diffuselightingamount *= anglefalloff;

        diffuselightingamount *= (1 - sgLight->sgLocalAmbientAmount);
        diffuselightingamount = getMax(getMin(diffuselightingamount, 1.0f), 0.0f);
        diffuse = sgLight->mColor * diffuselightingamount;
    }

    if (sgLight->sgDoubleSidedAmbient || (anglefalloff > 0.0f))
    {
        if (sgLight->mType == LightInfo::Vector)
        {
            ambient = sgLight->mAmbient;
        }
        else
        {
            ambientlightingamount *= sgLight->sgLocalAmbientAmount;
            ambientlightingamount = getMax(getMin(ambientlightingamount, 1.0f), 0.0f);
            ambient = sgLight->mColor * ambientlightingamount;
        }
    }


    /*Point3F lightvectorsq;
    Point3F lightvector;
    Point3F lightvectorfordist;
    F32 spotlightingangle;

    //F32 anglefalloff = 0;
    F32 dist;
    F32 distsq;

    // for speed set on exit...
    diffuse = ColorF(0.0, 0.0, 0.0);
    ambient = ColorF(0.0, 0.0, 0.0);
    lightingnormal = Point3F(0.0, 0.0, 0.0);

    // get light vector...
    if(sgLight->mType == LightInfo::Vector)
        lightvector = sgLight->mDirection * -1;
    else
    {
        lightvector = sgLight->mPos - point;
        // need this later...
        lightvectorfordist = lightvector;
        // vector lights are already normallized...
        lightvector.normalize();
    }

    lightingnormal = lightvector;

    // is it a spotlight? is it pointing this way?
    if(sgLight->mType == LightInfo::SGStaticSpot)
    {
        spotlightingangle = mDot(lightvector, sgLight->mDirection);
        if(spotlightingangle < spotanglecos)
            return;
    }
    else
    {
        // default...
        spotlightingangle = 1.0;
    }

    // get the attenuation info...
    if(sgLight->mType != LightInfo::Vector)
    {
        if(sgQuadraticAttenuation > 0.0f)
            distsq = lightvectorfordist.lenSquared();

        // should never use linear...
        if(sgLinearAttenuation > 0.0f)
            dist = lightvectorfordist.len();
    }

    // get the lighting angle amount...
    F32 angleamount = mDot(lightvector, normal);
    F32 lightamount = angleamount;
    F32 lightfalloff = 1.0;
    lightamount = getMax(getMin(lightamount, 1.0f), 0.0f);

    if(sgLight->mType != LightInfo::Vector)
    {
        // ahhh the lighting model...
        lightfalloff = (sgConstantAttenuation + (sgLinearAttenuation * dist) + (sgQuadraticAttenuation * distsq));
        if(lightfalloff != 0.0f)
            lightfalloff = 1 / lightfalloff;
        lightfalloff = getMax(lightfalloff, 0.0f);

        if(sgLight->mType == LightInfo::SGStaticSpot)
        {
            // apply the spotlight lighting model...
            spotlightingangle = mClampF(((spotlightingangle - spotamountouter) / spotamountinner), 0.0f, 1.0f);

            if(sgLight->sgLightInfoData.sgSmoothSpotLight)
                spotlightingangle *= getMin((spotlightingangle * 1.2), 1.0);

            lightfalloff *= spotlightingangle;
        }

        lightamount *= lightfalloff;
    }

    lightamount *= (1 - sgLight->sgLightInfoData.sgLocalAmbientAmount);
    lightamount = getMax(getMin(lightamount, 1.0f), 0.0f);
    diffuse = sgLight->mColor * lightamount;

    if(sgLight->sgLightInfoData.sgDoubleSidedAmbient || (angleamount > 0.0f))
    {
        lightfalloff *= sgLight->sgLightInfoData.sgLocalAmbientAmount;
        lightfalloff = getMax(getMin(lightfalloff, 1.0f), 0.0f);
        ambient = sgLight->mColor * lightfalloff;
    }*/
}

