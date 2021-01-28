//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _DYN_MAX_UTIL_H
#define _DYN_MAX_UTIL_H

#pragma pack(push,8)
#include <max.h>
#include <iparamb2.h>
#include <ISkin.h>
#include <decomp.h>
#pragma pack(pop)

#include "math/mMath.h"
#include "ts/tsTransform.h"

#define DEFAULT_TIME 0

extern Point3F &       Point3ToPoint3F(Point3 & p3, Point3F & p3f);

extern MatrixF &       convertToMatrixF(Matrix3 & mat3,MatrixF & matf);

extern void            zapScale(Matrix3 & mat);

extern TriObject *     getTriObject( INode *pNode, S32 time, S32 multiResVerts, bool & deleteIt);

extern void            getLocalNodeTransform(INode *pNode, INode *parent, AffineParts & child0, AffineParts & parent0, S32 time, Quat16 & rot, Point3F & trans, Quat16 & srot, Point3F & scale);

extern void            getBlendNodeTransform(INode *pNode, INode *parent, AffineParts & child0, AffineParts & parent0, S32 time, S32 referenceTime, Quat16 & rot, Point3F & trans, Quat16 & srot, Point3F & scale);

extern Matrix3 &       getLocalNodeMatrix(INode *pNode, INode *parent, S32 time, Matrix3 & matrix, AffineParts & child0, AffineParts & parent0);

extern void            computeObjectOffset(INode * pNode, Matrix3 & mat);

extern F32             findVolume(INode * pNode, S32 & polyCount);

extern void            getDeltaTransform(INode * pNode, S32 time1, S32 time2, Quat16 & rot, Point3F & trans, Quat16 & srot, Point3F & scale);

extern bool            isVis(INode * pNode, S32 time);

extern F32             getVisValue(INode * pNode, S32 time);

extern bool            animatesVis(INode * pNode, const Interval & range, bool & error);

extern bool            animatesFrame(INode * pNode, const Interval & range, bool & error);

extern bool            animatesMatFrame(INode * pNode, const Interval & range, bool & error);

extern void            embedSubtree(Interface *);

extern void            renumberNodes(Interface *, S32 newSize);

extern void            registerDetails(Interface * ip);

extern char *          chopTrailingNumber(const char *, S32 & size);

extern bool            hasMesh(INode *pNode);

extern void            findSkinData(INode * pNode, ISkin **skin, ISkinContextData ** skinData);

extern bool            hasSkin(INode * pNode);

extern bool            pointInPoly(const Point3F & point, const Point3F & normal, const Point3F * verts, U32 n);

#endif // _DYN_MAX_UTIL_H
