/**********************************************************************
 *
 * MilkShape 3D Model Import/Export API
 *
 * May 10 2000, Mete Ciragan, chUmbaLum sOft
 *
 **********************************************************************/

#ifndef __MSLIB_H__
#define __MSLIB_H__



#ifdef MSLIB_EXPORTS
#define MSLIB_API __declspec(dllexport)
#else
#define MSLIB_API __declspec(dllimport)
#endif /* MSLIB_EXPORTS */



#ifdef WIN32
#include <pshpack1.h>
#endif /* WIN32 */



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/**********************************************************************
 *
 * Constants
 *
 **********************************************************************/

#define MS_MAX_NAME             32
#define MS_MAX_PATH             256



/**********************************************************************
 *
 * Types
 *
 **********************************************************************/

#ifndef byte
typedef unsigned char byte;
#endif /* byte */

#ifndef word
typedef unsigned short word;
#endif /* word */

typedef float   msVec4[4];
typedef float   msVec3[3];
typedef float   msVec2[2];

/* msFlag */
typedef enum {
    eSelected = 1, eSelected2 = 2, eHidden = 4, eDirty = 8, eAveraged = 16, eUnused = 32
} msFlag;

/* msVertex */
typedef struct msVertex
{
    byte        nFlags;
    msVec3      Vertex;
    float       u, v;
    char        nBoneIndex;
} msVertex;

/* msTriangle */
typedef struct
{
    word        nFlags;
    word        nVertexIndices[3];
    word        nNormalIndices[3];
    msVec3      Normal;
    byte        nSmoothingGroup;
} msTriangle;

/* msMesh */
typedef struct msMesh
{
    byte        nFlags;
    char        szName[MS_MAX_NAME];
    char        nMaterialIndex;
    
    word        nNumVertices;
    word        nNumAllocedVertices;
    msVertex*   pVertices;

    word        nNumNormals;
    word        nNumAllocedNormals;
    msVec3*     pNormals;

    word        nNumTriangles;
    word        nNumAllocedTriangles;
    msTriangle* pTriangles;
} msMesh;

/* msMaterial */
typedef struct msMaterial
{
    int         nFlags;
    char        szName[MS_MAX_NAME];
    msVec4      Ambient;
    msVec4      Diffuse;
    msVec4      Specular;
    msVec4      Emissive;
    float       fShininess;
    float       fTransparency;
    char        szDiffuseTexture[MS_MAX_PATH];
    char        szAlphaTexture[MS_MAX_PATH];
    int         nName;
} msMaterial;

/* msPositionKey */
typedef struct msPositionKey
{
    float       fTime;
    msVec3      Position;
} msPositionKey;

/* msRotationKey */
typedef struct msRotationKey
{
    float   fTime;
    msVec3  Rotation;
} msRotationKey;

/* msBone */
typedef struct msBone
{
    int             nFlags;
    char            szName[MS_MAX_NAME];
    char            szParentName[MS_MAX_NAME];
    msVec3          Position;
    msVec3          Rotation;

    int             nNumPositionKeys;
    int             nNumAllocedPositionKeys;
    msPositionKey*  pPositionKeys;

    int             nNumRotationKeys;
    int             nNumAllocedRotationKeys;
    msRotationKey*  pRotationKeys;
} msBone;

/* msModel */
typedef struct msModel
{
    int         nNumMeshes;
    int         nNumAllocedMeshes;
    msMesh*     pMeshes;

    int         nNumMaterials;
    int         nNumAllocedMaterials;
    msMaterial* pMaterials;

    int         nNumBones;
    int         nNumAllocedBones;
    msBone*     pBones;

    int         nFrame;
    int         nTotalFrames;

    msVec3      Position;
    msVec3      Rotation;
} msModel;



/**********************************************************************
 *
 * MilkShape 3D Interface
 *
 **********************************************************************/

/**********************************************************************
 * msModel
 **********************************************************************/

MSLIB_API void          msModel_Destroy (msModel *pModel);

MSLIB_API int           msModel_GetMeshCount (msModel *pModel);
MSLIB_API int           msModel_AddMesh (msModel *pModel);
MSLIB_API msMesh*       msModel_GetMeshAt (msModel *pModel, int nIndex);
MSLIB_API int           msModel_FindMeshByName (msModel *pModel, const char *szName);

MSLIB_API int           msModel_GetMaterialCount (msModel *pModel);
MSLIB_API int           msModel_AddMaterial (msModel *pModel);
MSLIB_API msMaterial*   msModel_GetMaterialAt (msModel *pModel, int nIndex);
MSLIB_API int           msModel_FindMaterialByName (msModel *pModel, const char *szName);

MSLIB_API int           msModel_GetBoneCount (msModel *pModel);
MSLIB_API int           msModel_AddBone (msModel *pModel);
MSLIB_API msBone*       msModel_GetBoneAt (msModel *pModel, int nIndex);
MSLIB_API int           msModel_FindBoneByName (msModel *pModel, const char *szName);

MSLIB_API int           msModel_SetFrame (msModel *pModel, int nFrame);
MSLIB_API int           msModel_GetFrame (msModel *pModel);
MSLIB_API int           msModel_SetTotalFrames (msModel *pModel, int nTotalFrames);
MSLIB_API int           msModel_GetTotalFrames (msModel *pModel);
MSLIB_API void          msModel_SetPosition (msModel *pModel, msVec3 Position);
MSLIB_API void          msModel_GetPosition (msModel *pModel, msVec3 Position);
MSLIB_API void          msModel_SetRotation (msModel *pModel, msVec3 Rotation);
MSLIB_API void          msModel_GetRotation (msModel *pModel, msVec3 Rotation);

/**********************************************************************
 * msMesh
 **********************************************************************/

MSLIB_API void          msMesh_Destroy (msMesh *pMesh);
MSLIB_API void          msMesh_SetFlags (msMesh *pMesh, byte nFlags);
MSLIB_API byte          msMesh_GetFlags (msMesh *pMesh);
MSLIB_API void          msMesh_SetName (msMesh *pMesh, const char *szName);
MSLIB_API void          msMesh_GetName (msMesh *pMesh, char *szName, int nMaxLength);
MSLIB_API void          msMesh_SetMaterialIndex (msMesh *pMesh, int nIndex);
MSLIB_API int           msMesh_GetMaterialIndex (msMesh *pMesh);

MSLIB_API int           msMesh_GetVertexCount (msMesh *pMesh);
MSLIB_API int           msMesh_AddVertex (msMesh *pMesh);
MSLIB_API msVertex*     msMesh_GetVertexAt (msMesh *pMesh, int nIndex);
MSLIB_API msVertex*     msMesh_GetInterpolatedVertexAt (msMesh *pMesh, int nIndex); // NOT YET IMPLEMENTED

MSLIB_API int           msMesh_GetTriangleCount (msMesh *pMesh);
MSLIB_API int           msMesh_AddTriangle (msMesh *pMesh);
MSLIB_API msTriangle*   msMesh_GetTriangleAt (msMesh *pMesh, int nIndex);

MSLIB_API int           msMesh_GetVertexNormalCount (msMesh *pMesh);
MSLIB_API int           msMesh_AddVertexNormal (msMesh *pMesh);
MSLIB_API void          msMesh_SetVertexNormalAt (msMesh *pMesh, int nIndex, msVec3 Normal);
MSLIB_API void          msMesh_GetVertexNormalAt (msMesh *pMesh, int nIndex, msVec3 Normal);
MSLIB_API void          msMesh_GetInterpolatedVertexNormalAt (msMesh *pMesh, int nIndex, msVec3 Normal); // NOT YET IMPLEMENTED

/**********************************************************************
 * msTriangle
 **********************************************************************/

MSLIB_API void          msTriangle_SetFlags (msTriangle* pTriangle, word nFlags);
MSLIB_API word          msTriangle_GetFlags (msTriangle* pTriangle);
MSLIB_API void          msTriangle_SetVertexIndices (msTriangle *pTriangle, word nIndices[]);
MSLIB_API void          msTriangle_GetVertexIndices (msTriangle *pTriangle, word nIndices[]);
MSLIB_API void          msTriangle_SetNormalIndices (msTriangle *pTriangle, word nNormalIndices[]);
MSLIB_API void          msTriangle_GetNormalIndices (msTriangle *pTriangle, word nNormalIndices[]);
MSLIB_API void          msTriangle_SetSmoothingGroup (msTriangle *pTriangle, byte nSmoothingGroup);
MSLIB_API byte          msTriangle_GetSmoothingGroup (msTriangle *pTriangle);

/**********************************************************************
 * msVertex
 **********************************************************************/

MSLIB_API void          msVertex_SetFlags (msVertex* pVertex, byte nFlags);
MSLIB_API byte          msVertex_GetFlags (msVertex* pVertex);
MSLIB_API void          msVertex_SetVertex (msVertex* pVertex, msVec3 Vertex);
MSLIB_API void          msVertex_GetVertex (msVertex* pVertex, msVec3 Vertex);
MSLIB_API void          msVertex_SetTexCoords (msVertex* pVertex, msVec2 st);
MSLIB_API void          msVertex_GetTexCoords (msVertex* pVertex, msVec2 st);
MSLIB_API int           msVertex_SetBoneIndex (msVertex* pVertex, int nBoneIndex);
MSLIB_API int           msVertex_GetBoneIndex (msVertex* pVertex);

/**********************************************************************
 * msMaterial
 **********************************************************************/

MSLIB_API void          msMaterial_SetName (msMaterial *pMaterial, const char *szName);
MSLIB_API void          msMaterial_GetName (msMaterial *pMaterial, char *szName, int nMaxLength);
MSLIB_API void          msMaterial_SetAmbient (msMaterial *pMaterial, msVec4 Ambient);
MSLIB_API void          msMaterial_SetAmbient (msMaterial *pMaterial, msVec4 Ambient);
MSLIB_API void          msMaterial_GetAmbient (msMaterial *pMaterial, msVec4 Ambient);
MSLIB_API void          msMaterial_SetDiffuse (msMaterial *pMaterial, msVec4 Diffuse);
MSLIB_API void          msMaterial_GetDiffuse (msMaterial *pMaterial, msVec4 Diffuse);
MSLIB_API void          msMaterial_SetSpecular (msMaterial *pMaterial, msVec4 Specular);
MSLIB_API void          msMaterial_GetSpecular (msMaterial *pMaterial, msVec4 Specular);
MSLIB_API void          msMaterial_SetEmissive (msMaterial *pMaterial, msVec4 Emissive);
MSLIB_API void          msMaterial_GetEmissive (msMaterial *pMaterial, msVec4 Emissive);
MSLIB_API void          msMaterial_SetShininess (msMaterial *pMaterial, float fShininess);
MSLIB_API float         msMaterial_GetShininess (msMaterial *pMaterial);
MSLIB_API void          msMaterial_SetTransparency (msMaterial *pMaterial, float fTransparency);
MSLIB_API float         msMaterial_GetTransparency (msMaterial *pMaterial);
MSLIB_API void          msMaterial_SetDiffuseTexture (msMaterial *pMaterial, const char *szDiffuseTexture);
MSLIB_API void          msMaterial_GetDiffuseTexture (msMaterial *pMaterial, char *szDiffuseTexture, int nMaxLength);
MSLIB_API void          msMaterial_SetAlphaTexture (msMaterial *pMaterial, const char *szAlphaTexture);
MSLIB_API void          msMaterial_GetAlphaTexture (msMaterial *pMaterial, char *szAlphaTexture, int nMaxLength);

/**********************************************************************
 * msBone
 **********************************************************************/

MSLIB_API void          msBone_Destroy (msBone *pBone);
MSLIB_API void          msBone_SetFlags (msBone *pBone, int nFlags);
MSLIB_API int           msBone_GetFlags (msBone *pBone);
MSLIB_API void          msBone_SetName (msBone *pBone, const char *szName);
MSLIB_API void          msBone_GetName (msBone *pBone, char *szName, int nMaxLength);
MSLIB_API void          msBone_SetParentName (msBone *pBone, const char *szParentName);
MSLIB_API void          msBone_GetParentName (msBone *pBone, char *szParentName, int nMaxLength);
MSLIB_API void          msBone_SetPosition (msBone *pBone, msVec3 Position);
MSLIB_API void          msBone_GetPosition (msBone *pBone, msVec3 Position);
MSLIB_API void          msBone_GetInterpolatedPosition (msBone *pBone, msVec3 Position); // NOT YET IMPLEMENTED
MSLIB_API void          msBone_SetRotation (msBone *pBone, msVec3 Rotation);
MSLIB_API void          msBone_GetRotation (msBone *pBone, msVec3 Rotation);
MSLIB_API void          msBone_GetInterpolatedRotation (msBone *pBone, msVec3 Rotation); // NOT YET IMPLEMENTED

MSLIB_API int           msBone_GetPositionKeyCount (msBone *pBone);
MSLIB_API int           msBone_AddPositionKey (msBone *pBone, float fTime, msVec3 Position);
MSLIB_API msPositionKey* msBone_GetPositionKeyAt (msBone *pBone, int nIndex);

MSLIB_API int           msBone_GetRotationKeyCount (msBone *pBone);
MSLIB_API int           msBone_AddRotationKey (msBone *pBone, float fTime, msVec3 Rotation);
MSLIB_API msRotationKey* msBone_GetRotationKeyAt (msBone *pBone, int nIndex);



#ifdef __cplusplus
}
#endif /* __cplusplus */



#ifdef WIN32
#include <poppack.h>
#endif /* WIN32 */



#endif /* __MSLIB_H__ */