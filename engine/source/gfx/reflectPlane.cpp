//-----------------------------------------------------------------------------
// Torque Shader Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "reflectPlane.h"
#include "math/mathUtils.h"
#include "gfx/gfxDevice.h"

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
ReflectPlane::ReflectPlane()
{
    mTexSize = 512;
}

//-----------------------------------------------------------------------------
// Set up render target texture
//-----------------------------------------------------------------------------
void ReflectPlane::setupTex(U32 texSize)
{
    mTexSize = texSize;
    mReflectTex.set(texSize, texSize, GFXFormatR8G8B8, &GFXDefaultRenderTargetZBufferProfile);
    mDepthTex.set( texSize, texSize, GFXFormatD24S8, &GFXDefaultZTargetProfile );
}

//--------------------------------------------------------------------------
// Set up camera matrix for a reflection on the plane
//--------------------------------------------------------------------------
MatrixF ReflectPlane::getCameraReflection(MatrixF& camTrans)
{
    Point3F normal = mPlane;

    // Figure out new cam position
    Point3F camPos = camTrans.getPosition();
    F32 dist = mPlane.distToPlane(camPos);
    Point3F newCamPos = camPos - normal * dist * 2.0;

    // Figure out new look direction
    Point3F i, j, k;
    camTrans.getColumn(0, &i);
    camTrans.getColumn(1, &j);
    camTrans.getColumn(2, &k);

    i = MathUtils::reflect(i, normal);
    j = MathUtils::reflect(j, normal);
    k = MathUtils::reflect(k, normal);
    //mCross( i, j, &k );


    MatrixF newTrans(true);
    newTrans.setColumn(0, i);
    newTrans.setColumn(1, j);
    newTrans.setColumn(2, k);

    newTrans.setPosition(newCamPos);

    return newTrans;
}

//--------------------------------------------------------------------------
// Helper function for frustum clipping
//--------------------------------------------------------------------------
inline float sgn(float a)
{
    if (a > 0.0F) return (1.0F);
    if (a < 0.0F) return (-1.0F);
    return (0.0F);
}

//--------------------------------------------------------------------------
// Oblique frustum clipping - use near plane of zbuffer as a clip plane
//--------------------------------------------------------------------------
MatrixF ReflectPlane::getFrustumClipProj(MatrixF& modelview)
{
    static MatrixF rotMat(EulerF((M_PI / 2.0), 0.0, 0.0));
    static MatrixF invRotMat(EulerF(-(M_PI / 2.0), 0.0, 0.0));


    MatrixF revModelview = modelview;
    revModelview = rotMat * revModelview;  // add rotation to modelview because it needs to be removed from projection

    // rotate clip plane into modelview space
    Point4F clipPlane;
    Point3F pnt = mPlane * -(mPlane.d + 0.0);
    Point3F norm = mPlane;

    revModelview.mulP(pnt);
    revModelview.mulV(norm);
    norm.normalize();

    clipPlane.set(norm.x, norm.y, norm.z, -mDot(pnt, norm));


    // Manipulate projection matrix
    //------------------------------------------------------------------------
    MatrixF proj = GFX->getProjectionMatrix();
    proj.mul(invRotMat);  // reverse rotation imposed by Torque
    proj.transpose();       // switch to row-major order

     // Calculate the clip-space corner point opposite the clipping plane
     // as (sgn(clipPlane.x), sgn(clipPlane.y), 1, 1) and
     // transform it into camera space by multiplying it
     // by the inverse of the projection matrix
    Vector4F	q;
    q.x = sgn(clipPlane.x) / proj(0, 0);
    q.y = sgn(clipPlane.y) / proj(1, 1);
    q.z = -1.0F;
    q.w = (1.0F - proj(2, 2)) / proj(3, 2);

    F32 a = 1.0 / (clipPlane.x * q.x + clipPlane.y * q.y + clipPlane.z * q.z + clipPlane.w * q.w);

    Vector4F c = clipPlane * a;

    // Replace the third column of the projection matrix
    proj.setColumn(2, c);
    proj.transpose(); // convert back to column major order
    proj.mul(rotMat);  // restore Torque rotation

    return proj;
}
