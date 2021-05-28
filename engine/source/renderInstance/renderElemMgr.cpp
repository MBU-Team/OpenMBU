//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "renderElemMgr.h"
#include "materials/matInstance.h"
#include "../../game/shaders/shdrConsts.h"

//-----------------------------------------------------------------------------
// RenderElemMgr
//-----------------------------------------------------------------------------
RenderElemMgr::RenderElemMgr()
{
    mElementList.reserve(2048);
}

void RenderElemMgr::setupLights(RenderInst* inst, SceneGraphData& data)
{
    MatrixF objTrans = *inst->objXform;
    objTrans.inverse();

    if (Material::isDebugLightingEnabled())
    {
        // clean up later...
        dMemcpy(&data.light, Material::getDebugLight(), sizeof(data.light));
        dMemcpy(&data.lightSecondary, Material::getDebugLight(), sizeof(data.lightSecondary));
    } else {
        // clean up later...
        dMemcpy(&data.light, &inst->light, sizeof(inst->light));
        dMemcpy(&data.lightSecondary, &inst->lightSecondary, sizeof(inst->lightSecondary));
    }

    data.dynamicLight = inst->dynamicLight;
    data.dynamicLightSecondary = inst->dynamicLightSecondary;


    // fill in primary light
    //-------------------------
    Point3F lightPos = data.light.mPos;
    Point3F lightDir = data.light.mDirection;
    objTrans.mulP(lightPos);
    objTrans.mulV(lightDir);

    Point4F lightPosModel(lightPos.x, lightPos.y, lightPos.z, data.light.sgTempModelInfo[0]);
    GFX->setVertexShaderConstF(VC_LIGHT_POS1, (float*)&lightPosModel, 1);
    GFX->setVertexShaderConstF(VC_LIGHT_DIR1, (float*)&lightDir, 1);
    GFX->setVertexShaderConstF(VC_LIGHT_DIFFUSE1, (float*)&data.light.mColor, 1);
    GFX->setPixelShaderConstF(PC_AMBIENT_COLOR, (float*)&data.light.mAmbient, 1);

    if (inst->matInst && inst->matInst->getMaterial())
    {
        U32 stagenum = inst->matInst->getCurStageNum();
        if (!inst->matInst->getMaterial()->emissive[stagenum])
            GFX->setPixelShaderConstF(PC_DIFF_COLOR, (float*)&data.light.mColor, 1);
        else
        {
            ColorF selfillum = LightManager::sgGetSelfIlluminationColor(
                inst->matInst->getMaterial()->diffuse[stagenum]);
            GFX->setPixelShaderConstF(PC_DIFF_COLOR, (float*)&selfillum, 1);
        }
    }
    else
    {
        GFX->setPixelShaderConstF(PC_DIFF_COLOR, (float*)&data.light.mColor, 1);
    }

    MatrixF lightingmat = data.light.sgLightingTransform;
    GFX->setVertexShaderConstF(VC_LIGHT_TRANS, (float*)&lightingmat, 4);


    // fill in secondary light
    //-------------------------
    lightPos = data.lightSecondary.mPos;
    lightDir = data.lightSecondary.mDirection;
    objTrans.mulP(lightPos);
    objTrans.mulV(lightDir);

    lightPosModel = Point4F(lightPos.x, lightPos.y, lightPos.z, data.lightSecondary.sgTempModelInfo[0]);
    GFX->setVertexShaderConstF(VC_LIGHT_POS2, (float*)&lightPosModel, 1);
    GFX->setPixelShaderConstF(PC_DIFF_COLOR2, (float*)&data.lightSecondary.mColor, 1);

    lightingmat = data.lightSecondary.sgLightingTransform;
    GFX->setVertexShaderConstF(VC_LIGHT_TRANS2, (float*)&lightingmat, 4);

    if (Material::isDebugLightingEnabled())
        return;

    // need to reassign the textures...
    RenderPassData* pass = inst->matInst->getPass(inst->matInst->getCurPass());
    if (!pass)
        return;

    for (U32 i = 0; i < pass->numTex; i++)
    {
        if (pass->texFlags[i] == Material::DynamicLight)
        {
            GFX->setTextureStageAddressModeU(i, GFXAddressClamp);
            GFX->setTextureStageAddressModeV(i, GFXAddressClamp);
            GFX->setTextureStageAddressModeW(i, GFXAddressClamp);
            GFX->setTexture(i, data.dynamicLight);
        }

        if (pass->texFlags[i] == Material::DynamicLightSecondary)
        {
            GFX->setTextureStageAddressModeU(i, GFXAddressClamp);
            GFX->setTextureStageAddressModeV(i, GFXAddressClamp);
            GFX->setTextureStageAddressModeW(i, GFXAddressClamp);
            GFX->setTexture(i, data.dynamicLightSecondary);
        }
    }
}

//-----------------------------------------------------------------------------
// addElement
//-----------------------------------------------------------------------------
void RenderElemMgr::addElement(RenderInst* inst)
{
    mElementList.increment();
    MainSortElem& elem = mElementList.last();
    elem.inst = inst;
    elem.key = elem.key2 = 0;

    // sort by material
    if (inst->matInst)
    {
        elem.key = (U32)inst->matInst;
    }

    // sort by vertex buffer
    if (inst->vertBuff)
    {
        elem.key2 = (U32)inst->vertBuff->getPointer();
    }

}

//-----------------------------------------------------------------------------
// clear
//-----------------------------------------------------------------------------
void RenderElemMgr::clear()
{
    mElementList.clear();
}

//-----------------------------------------------------------------------------
// sort
//-----------------------------------------------------------------------------
void RenderElemMgr::sort()
{
    dQsort(mElementList.address(), mElementList.size(), sizeof(MainSortElem), cmpKeyFunc);
}

//-----------------------------------------------------------------------------
// QSort callback function
//-----------------------------------------------------------------------------
S32 FN_CDECL RenderElemMgr::cmpKeyFunc(const void* p1, const void* p2)
{
    const MainSortElem* mse1 = (const MainSortElem*)p1;
    const MainSortElem* mse2 = (const MainSortElem*)p2;

    // dynamic lights *MUST* be rendered after the base pass!
    if (mse1->inst && mse1->inst->matInst &&
        mse2->inst && mse2->inst->matInst)
    {
        S32 testA = S32(mse2->inst->matInst->isDynamicLightingMaterial()) -
            S32(mse1->inst->matInst->isDynamicLightingMaterial());
        if (testA != 0)
            return testA;
    }

    S32 test1 = S32(mse2->key) - S32(mse1->key);

    return (test1 == 0) ? S32(mse2->key2) - S32(mse1->key2) : test1;
}
