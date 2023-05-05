#ifndef _RENDER_MARBLE_SHADOW_MGR_H_
#define _RENDER_MARBLE_SHADOW_MGR_H_

#include "renderElemMgr.h"

//**************************************************************************
// RenderMarbleShadowMgr
//**************************************************************************
class RenderMarbleShadowMgr : public RenderElemMgr
{
private:
    ShaderData* mShaderStencil;

public:
    RenderMarbleShadowMgr();

    void render();
};

#endif
