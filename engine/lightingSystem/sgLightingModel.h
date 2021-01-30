//-----------------------------------------------
// Synapse Gaming - Lighting System
// Copyright © Synapse Gaming 2003
// Written by John Kabus
//-----------------------------------------------
#ifndef _SGLIGHTINGMODEL_H_
#define _SGLIGHTINGMODEL_H_

#ifndef _MMATH_H_
#include "math/mMath.h"
#endif
#ifndef _COLOR_H_
#include "core/color.h"
#endif
#ifndef _TVECTOR_H_
#include "core/tVector.h"
#endif
#ifndef _DATACHUNKER_H_
#include "core/dataChunker.h"
#endif
#include "console/console.h"
#include "console/consoleTypes.h"
#include "sceneGraph/lightManager.h"
#include "core/stringTable.h"
#include "core/tVector.h"

#define SG_LIGHTINGMODEL_NAME	"SG - %s (Lighting Pack)"

#define SG_DYNAMICLIGHTING_TYPE2

class sgLightingModel
{
protected:
    bool sgStateSet;
    bool sgStateInitLM;
    LightInfo* sgLight;
public:
    char sgLightingModelName[64];
    sgLightingModel();
    void sgRegisterLightingModel();
    virtual void sgSetState(LightInfo* light)
    {
        AssertFatal((sgStateSet == false), "sgLightingModel: State not properly reset.");
        sgStateSet = true;
        sgStateInitLM = false;
        sgLight = light;
    }
    virtual void sgLightingGL(S32 gllight)
    {
        AssertFatal((sgStateSet == true), "sgLightingModel: State not properly set.");
    }
    virtual void sgInitStateLM()
    {
        AssertFatal((sgStateSet == true), "sgLightingModel: State not properly set.");
        sgStateInitLM = true;
    }
    virtual void sgLightingLM(Point3F point, VectorF normal, ColorF& diffuse, ColorF& ambient, Point3F& lightingnormal)
    {
        AssertFatal((sgStateInitLM == true), "sgLightingModel: State not properly init.");
    }
    virtual void sgResetState()
    {
        sgStateSet = false;
        sgStateInitLM = false;
        sgLight = NULL;
    }
    virtual F32 sgScoreLight(LightInfo* light, const SphereF& sphere)
    {
        AssertFatal((sgStateSet == true), "sgLightingModel: State not properly set.");
        return 0.0f;
    }
    virtual bool sgCanIlluminate(const Box3F& box) = 0;
    virtual F32 sgGetMaxRadius(bool speedoverquality = false, bool glstyle = false) = 0;
    virtual Point3F sgGetModelInfo() = 0;
protected:
    bool sgGeneratedDynamicLightingTexture;
    GFXTexHandle sgDynamicLightingTextureOmni;
    GFXTexHandle sgDynamicLightingTextureSpot;
    void sgGenerateDynamicLightingTexture();
public:
    GFXTexHandle sgGetDynamicLightingTextureOmni();
    GFXTexHandle sgGetDynamicLightingTextureSpot();
};

class sgLightingModelGLBase : public sgLightingModel
{
protected:
    F32 sgConstantAttenuation;
    F32 sgLinearAttenuation;
    F32 sgQuadraticAttenuation;
    F32 spotanglecos;
    F32 spotamountinner;
    F32 spotamountouter;
    Point4F sgLightParamDiffuse;
    Point4F sgLightParamAmbient;
    Point4F sgLightParamPosition;
    Point4F sgLightParamDirection;
public:
    virtual void sgSetState(LightInfo* light);
    virtual void sgLightingGL(S32 gllight);
    virtual void sgInitStateLM();
    virtual void sgLightingLM(Point3F point, VectorF normal, ColorF& diffuse, ColorF& ambient, Point3F& lightingnormal);
    virtual F32 sgScoreLight(LightInfo* light, const SphereF& sphere);
    virtual bool sgCanIlluminate(const Box3F& box);
    virtual F32 sgGetMaxRadius(bool speedoverquality = false, bool glstyle = false);
    virtual Point3F sgGetModelInfo()
    {
        return Point3F(sgConstantAttenuation,
            sgLinearAttenuation, sgQuadraticAttenuation);
    }
};

class sgLightingModelAdvanced : public sgLightingModelGLBase
{
protected:
    F32 maxlightdistancesquared;
public:
    sgLightingModelAdvanced()
    {
        dSprintf(sgLightingModelName, sizeof(sgLightingModelName),
            SG_LIGHTINGMODEL_NAME, "Original Advanced");
    }
    virtual void sgSetState(LightInfo* light);
    virtual void sgInitStateLM();
    virtual void sgLightingLM(Point3F point, VectorF normal, ColorF& diffuse, ColorF& ambient, Point3F& lightingnormal);
    virtual F32 sgScoreLight(LightInfo* light, const SphereF& sphere);
    virtual bool sgCanIlluminate(const Box3F& box);
    virtual F32 sgGetMaxRadius(bool speedoverquality = false, bool glstyle = false)
    {
        AssertFatal((sgStateSet == true), "sgLightingModel: State not properly set.");
        if (glstyle)
            return sgLightingModelGLBase::sgGetMaxRadius(speedoverquality, glstyle);
        return sgLight->mRadius;
    }
};

class sgLightingModelStock : public sgLightingModelGLBase
{
public:
    sgLightingModelStock()
    {
        dSprintf(sgLightingModelName, sizeof(sgLightingModelName),
            SG_LIGHTINGMODEL_NAME, "Original Stock");
    }
    void sgSetState(LightInfo* light)
    {
        sgLightingModelGLBase::sgSetState(light);
        if (sgLight->mRadius > 0.0f)
            sgQuadraticAttenuation = (1.0f / (sgLight->mRadius * sgLight->mRadius));
        else
            sgQuadraticAttenuation = 0.0f;
    }
};

class sgLightingModelInverseSquare : public sgLightingModelGLBase
{
public:
    sgLightingModelInverseSquare()
    {
        dSprintf(sgLightingModelName, sizeof(sgLightingModelName),
            SG_LIGHTINGMODEL_NAME, "Inverse Square");
    }
    void sgSetState(LightInfo* light)
    {
        sgLightingModelGLBase::sgSetState(light);
        sgConstantAttenuation = 1.0f;
        if (sgLight->mRadius > 0.0f)
            sgQuadraticAttenuation = (1.0f / (sgLight->mRadius * sgLight->mRadius));
        else
            sgQuadraticAttenuation = 0.0f;
    }
};

class sgLightingModelInverseSquareFastFalloff : public sgLightingModelGLBase
{
public:
    sgLightingModelInverseSquareFastFalloff()
    {
        dSprintf(sgLightingModelName, sizeof(sgLightingModelName),
            SG_LIGHTINGMODEL_NAME, "Inverse Square Fast Falloff");
    }
    void sgSetState(LightInfo* light)
    {
        sgLightingModelGLBase::sgSetState(light);
        sgConstantAttenuation = 1.0f;
        if (sgLight->mRadius > 0.0f)
            sgQuadraticAttenuation = (10.0f / (sgLight->mRadius * sgLight->mRadius));
        else
            sgQuadraticAttenuation = 0.0f;
    }
};

class sgLightingModelNearLinear : public sgLightingModelGLBase
{
public:
    sgLightingModelNearLinear()
    {
        dSprintf(sgLightingModelName, sizeof(sgLightingModelName),
            SG_LIGHTINGMODEL_NAME, "Near Linear");
    }
    void sgSetState(LightInfo* light)
    {
        sgLightingModelGLBase::sgSetState(light);
        sgConstantAttenuation = 1.0f;
        if (sgLight->mRadius > 0.0f)
            sgLinearAttenuation = (1.0f / sgLight->mRadius);
        else
            sgLinearAttenuation = 0.0f;
    }
};

class sgLightingModelNearLinearFastFalloff : public sgLightingModelGLBase
{
public:
    sgLightingModelNearLinearFastFalloff()
    {
        dSprintf(sgLightingModelName, sizeof(sgLightingModelName),
            SG_LIGHTINGMODEL_NAME, "Near Linear Fast Falloff");
    }
    void sgSetState(LightInfo* light)
    {
        sgLightingModelGLBase::sgSetState(light);
        sgConstantAttenuation = 1.0f;
        if (sgLight->mRadius > 0.0f)
            sgLinearAttenuation = (10.0f / sgLight->mRadius);
        else
            sgLinearAttenuation = 0.0f;
    }
};

class sgLightingModelManager
{
private:
    static Vector<sgLightingModel*> sgLightingModels;
    static sgLightingModelStock sgDefaultModel;
    static sgLightingModelAdvanced sgAdvancedModel;
    static sgLightingModelInverseSquare sgInverseSquare;
    static sgLightingModelInverseSquareFastFalloff sgInverseSquareFastFalloff;
    static sgLightingModelNearLinear sgNearLinear;
    static sgLightingModelNearLinearFastFalloff sgNearLinearFastFalloff;
    static sgLightingModel* sgSunlightModel;
public:
    sgLightingModelManager() {}
    static void sgRegisterLightingModel(sgLightingModel* model) { sgLightingModels.push_back(model); }
    static sgLightingModel& sgGetLightingModel() { return sgDefaultModel; }
    static sgLightingModel& sgGetLightingModel(const char* name)
    {
        if ((name == NULL) || (name[0] == 0))
            return sgDefaultModel;
        for (U32 i = 0; i < sgLightingModels.size(); i++)
        {
            if (dStrcmp(sgLightingModels[i]->sgLightingModelName, name) == 0)
                return *(sgLightingModels[i]);
        }
        return sgDefaultModel;
    }
    static U32 sgGetLightingModelCount() { return sgLightingModels.size(); }
    static char* sgGetLightingModelName(U32 index)
    {
        if ((index < 0) || (index >= sgLightingModels.size()))
            return "";
        return sgLightingModels[index]->sgLightingModelName;
    }
    static const char* sgGetSunlightLightingModelName()
    {
        if (sgSunlightModel)
            return sgSunlightModel->sgLightingModelName;
        return NULL;
    }
    static char* sgGetAdvancedLightingModelName() { return sgAdvancedModel.sgLightingModelName; }
    static char* sgGetStockLightingModelName() { return sgDefaultModel.sgLightingModelName; }
};


#endif//_SGLIGHTINGMODEL_H_
