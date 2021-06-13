//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GAME_H_
#define _GAME_H_

#ifndef _TORQUE_TYPES_H_
#include "platform/types.h"
#endif

#ifndef _MMATRIX_H_
#include "math/mMatrix.h"
#endif

struct CameraQuery;

const F32 MinCameraFov = 1.f;      ///< min camera FOV
const F32 MaxCameraFov = 179.f;    ///< max camera FOV

extern bool gSPMode;
extern bool gRenderPreview;
extern SimSet* gSPModeSet;

/// Actually renders the world.  This is the function that will render the scene ONLY - new guis, no damage flashes.
void GameRenderWorld();
/// Renders overlays such as damage flashes, white outs, and water masks.  These are usually a color applied over the entire screen.
void GameRenderFilters(const CameraQuery& camq);
/// Does the same thing as GameGetCameraTransform, but fills in other data including information about the far and near clipping planes.
bool GameProcessCameraQuery(CameraQuery* query);
/// Gets the position, rotation, and velocity of the camera.
bool GameGetCameraTransform(MatrixF* mat, Point3F* velocity);
/// Gets the camera field of view angle.
F32 GameGetCameraFov();
/// Sets the field of view angle of the camera.
void GameSetCameraFov(F32 fov);
/// Sets where the camera fov will be change to.  This is for non-instantaneous zooms/retractions.
void GameSetCameraTargetFov(F32 fov);
/// Update the camera fov to be closer to the target fov.
void GameUpdateCameraFov();
/// Initializes graphics related console variables.
void GameInit();

/// Processes the next frame, including gui, rendering, and tick interpolation.
/// This function will only have an effect when executed on the client.
bool clientProcess(U32 timeDelta);
/// Processes the next cycle on the server.  This function will only have an effect when executed on the server.
bool serverProcess(U32 timeDelta);

bool spmodeProcess(U32 timeDelta);

#endif
