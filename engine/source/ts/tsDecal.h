//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _TSDECAL_H_
#define _TSDECAL_H_

#ifndef _TSMESH_H_
#include "ts/tsMesh.h"
#endif

/// Decals!  The lovely detailing thingies, e.g. bullet hole marks.
class TSDecalMesh
{
public:

    /// The mesh that we are decaling
    TSMesh* targetMesh;

    /// @name Topology
    /// @{
    ToolVector<TSDrawPrimitive> primitives;
    ToolVector<U16> indices;
    /// @}

    /// @name Render Data
    /// indexed by decal frame...
    /// @{
    ToolVector<S32> startPrimitive;
    ToolVector<Point4F> texgenS;
    ToolVector<Point4F> texgenT;
    /// @}

    /// We only allow 1 material per decal...
    S32 materialIndex;

    /// override render function
    void render(S32 frame, S32 decalFrame, TSMaterialList*);

    void disassemble();
    void assemble(bool skip);

    static void initDecalMaterials();
    static void resetDecalMaterials();
};


#endif

