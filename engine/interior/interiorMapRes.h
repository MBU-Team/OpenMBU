#ifndef _INTERIORMAPRES_H_
#define _INTERIORMAPRES_H_

#ifndef _RESMANAGER_H_
#include "core/resManager.h"
#endif

#include "core/tokenizer.h"
//#include "dgl/gTexManager.h"
#include "materials/materialList.h"
#include "math/mPlane.h"

class InteriorMap;
class ConvexBrush;

extern ResourceInstance* constructInteriorMAP(Stream& stream);

class InteriorMapResource : public ResourceInstance
{
    typedef ResourceInstance Parent;

public:
    // Enums
    enum BrushType
    {
        Structural = 0,
        Detail = 1,
        Collision = 2,
        Portal = 3,
        Trigger = 4
    };

    enum
    {
        Unknown = 0,
        Valve220 = 1,
        QuakeOld = 2,
        QuakeNew = 3
    };

    // Structs
    struct Property
    {
        StringTableEntry name;
        StringTableEntry value;
    };

    // Supporting classes
    class Entity
    {
    public:
        StringTableEntry classname;
        Vector<Property> properties;

        Entity() {};
        ~Entity() {};

        char* getValue(char* property)
        {
            for (U32 i = 0; i < properties.size(); i++)
            {
                if (dStricmp(property, properties[i].name) == 0)
                    return (char*)properties[i].value;
            }

            return NULL;
        }
    };

    // Data
    StringTableEntry           mFileName;        // The name of the level
    StringTableEntry           mFilePath;        // The path of the level
    StringTableEntry           mTexPath;         // The texture path

    VectorPtr<Entity*>         mEntities;
    Vector<StringTableEntry>   mMaterials;

    VectorPtr<ConvexBrush*>    mBrushes;

    U32                        mBrushFormat;
    F32                        mBrushScale;
    bool                       mTexGensCalced;

    U32                        mNextBrushID;

    Entity* mWorldSpawn;

public:
    InteriorMapResource();
    ~InteriorMapResource();

    // I/O
    bool load(const char* filename);
    bool read(Stream& stream);
    bool write(Stream& stream);
    bool writeBrush(U32 brushIndex, Stream& stream);

    // Parsing
    bool parseMap(Tokenizer* toker);
    bool parseEntity(Tokenizer* toker);
    bool parsePatch(Tokenizer* toker);
    bool parseBrush(Tokenizer* toker, BrushType type);
    bool parsePlane(Tokenizer* toker);
    bool parseQuakeValve(Tokenizer* toker, VectorF normal, U32& tdx, PlaneF* texGens, F32* scale);
    bool parseQuakeNew(Tokenizer* toker, U32& tdx, PlaneF* texGens, F32* scale);
    bool parseValve220TexGens(Tokenizer* toker, PlaneF* texGens, F32* scale);
    bool parseQuakeTexGens(Tokenizer* toker, VectorF normal, PlaneF* texGens, F32* scale);

    // Texture manager
    U32   addTexture(char* texture);
    U32   getTextureSize(char* texture, F32* texSizes);
    bool  getTextureSize(U32 texIdx, F32* texSizes);
    S32   getTextureIndex(char* texture);
    char* getTextureName(U32 texIdx);

    // Utility
    bool validatePlane(Point3F k, Point3F j, Point3F l);

    // Returns the next valid ID and increments mNextBrushID
    U32 getNextBrushID() { mNextBrushID++; return mNextBrushID - 1; };
};

#endif
