#ifndef _LEVEL_GEOMETRY_H_
#define _LEVEL_GEOMETRY_H_

#include "math/mBox.h"
#include "core/stream.h"
#include "sceneGraph/sceneState.h"
#include "gfx/gfxStructs.h"
#include "gfx/gfxPrimitiveBuffer.h"
#include "gfx/gfxVertexBuffer.h"

class LevelModelInstance;

class LevelModel
{
public:
    LevelModel();
    ~LevelModel();

    bool read(Stream &stream);
    bool write(Stream &stream) const;

private:
    static const U32 smIdentifier;
    static const U32 smFileVersion;

public:
    bool prepForRendering(const char* path);
    void prepBatchRender(LevelModelInstance* lmdInst, SceneState* state);
    Box3F getBoundingBox() const;

private:
    GFXVertexBufferHandle<GFXVertexPNTTBN> mVertBuff;
    GFXPrimitiveBufferHandle               mPrimBuff;
};

#endif // _LEVEL_GEOMETRY_H_