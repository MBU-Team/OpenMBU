//-----------------------------------------------------------------------------
// Torque Shader Engine
// Written by Melvyn May, Started on 13th March 2002, finished 27th March 2002.
//
// "My code is written for the Torque community, so do your worst with it,
//	just don't rip-it-off and call it your own without even thanking me".
//
//	- Melv (working hard to become an associate ... hint).
//
//-----------------------------------------------------------------------------

#include "dgl/dgl.h"
#include "console/consoleTypes.h"
#include "core/bitStream.h"
#include "math/mRandom.h"
#include "math/mathIO.h"
#include "terrain/terrData.h"
#include "game/gameConnection.h"
#include "console/simBase.h"
#include "sceneGraph/sceneGraph.h"
#include "fxShapeReplicator.h"

//------------------------------------------------------------------------------
//
//	Put this in /example/common/editor/editor.cs in function [Editor::create()] (around line 66).
//
//   // Ignore Replicated fxStatic Instances.
//   EWorldEditor.ignoreObjClass("fxShapeReplicatedStatic");
//
//------------------------------------------------------------------------------
//
//	Put this in /example/common/editor/EditorGui.cs in [function Creator::init( %this )]
//
//   %Environment_Item[8] = "fxShapeReplicator";  <-- ADD THIS.
//
//------------------------------------------------------------------------------
//
//	Put the function in /example/common/editor/ObjectBuilderGui.gui [around line 458] ...
//
//	function ObjectBuilderGui::buildfxShapeReplicator(%this)
//	{
//		%this.className = "fxShapeReplicator";
//		%this.process();
//	}
//
//------------------------------------------------------------------------------
//
//	Put this in /example/common/client/missionDownload.cs in [function clientCmdMissionStartPhase3(%seq,%missionName)] (line 65)
//	after codeline 'onPhase2Complete();'.
//
//	StartClientReplication();
//
//------------------------------------------------------------------------------
//
//	Put this in /engine/console/simBase.h (around line 509) in
//
//	namespace Sim
//  {
//	   DeclareNamedSet(fxReplicatorSet)  <-- ADD THIS (Note no semi-colon).
//
//------------------------------------------------------------------------------
//
//	Put this in /engine/console/simBase.cc (around line 19) in
//
//  ImplementNamedSet(fxReplicatorSet)  <-- ADD THIS
//
//------------------------------------------------------------------------------
//
//	Put this in /engine/console/simManager.cc [function void init()] (around line 269).
//
//	namespace Sim
//  {
//		InstantiateNamedSet(fxReplicatorSet);  <-- ADD THIS
//
//------------------------------------------------------------------------------

extern bool gEditingMission;

//------------------------------------------------------------------------------

IMPLEMENT_CO_NETOBJECT_V1(fxShapeReplicator);
IMPLEMENT_CO_NETOBJECT_V1(fxShapeReplicatedStatic);


//------------------------------------------------------------------------------
// Class: fxShapeReplicator
//------------------------------------------------------------------------------

fxShapeReplicator::fxShapeReplicator()
{
	// Setup NetObject.
	mTypeMask |= StaticObjectType | StaticTSObjectType | StaticRenderedObjectType;
	mAddedToScene = false;
	mNetFlags.set(Ghostable | ScopeAlways);

	// Reset Client Replication Started.
	mClientReplicationStarted = false;

	// Reset Shape Count.
	mCurrentShapeCount = 0;

	// Reset Creation Area Angle Animation.
	mCreationAreaAngle = 0;
}

//------------------------------------------------------------------------------

fxShapeReplicator::~fxShapeReplicator()
{
}

//------------------------------------------------------------------------------

void fxShapeReplicator::initPersistFields()
{
	// Initialise parents' persistent fields.
	Parent::initPersistFields();

	// Add out own persistent fields.
	addGroup( "Debugging" );	// MM: Added Group Header.
	addField( "HideReplications",	TypeBool,		Offset( mFieldData.mHideReplications,		fxShapeReplicator ) );
	addField( "ShowPlacementArea",	TypeBool,		Offset( mFieldData.mShowPlacementArea,		fxShapeReplicator ) );
	addField( "PlacementAreaHeight",TypeS32,		Offset( mFieldData.mPlacementBandHeight,	fxShapeReplicator ) );
	addField( "PlacementColour",	TypeColorF,		Offset( mFieldData.mPlaceAreaColour,		fxShapeReplicator ) );
	endGroup( "Debugging" );	// MM: Added Group Footer.

	addGroup( "Media" );	// MM: Added Group Header.
	addField( "ShapeFile",			TypeFilename,	Offset( mFieldData.mShapeFile,				fxShapeReplicator ) );
	endGroup( "Media" );	// MM: Added Group Footer.

	addGroup( "Replications" );	// MM: Added Group Header.
	addField( "Seed",				TypeS32,		Offset( mFieldData.mSeed,					fxShapeReplicator ) );
	addField( "ShapeCount",			TypeS32,		Offset( mFieldData.mShapeCount,				fxShapeReplicator ) );
	addField( "ShapeRetries",		TypeS32,		Offset( mFieldData.mShapeRetries,			fxShapeReplicator ) );
	endGroup( "Replications" );	// MM: Added Group Footer.

	addGroup( "Placement Radius" );	// MM: Added Group Header.
	addField( "InnerRadiusX",		TypeS32,		Offset( mFieldData.mInnerRadiusX,			fxShapeReplicator ) );
	addField( "InnerRadiusY",		TypeS32,		Offset( mFieldData.mInnerRadiusY,			fxShapeReplicator ) );
	addField( "OuterRadiusX",		TypeS32,		Offset( mFieldData.mOuterRadiusX,			fxShapeReplicator ) );
	addField( "OuterRadiusY",		TypeS32,		Offset( mFieldData.mOuterRadiusY,			fxShapeReplicator ) );
	endGroup( "Placement Radius" );	// MM: Added Group Footer.

	addGroup( "Restraints" );	// MM: Added Group Header.
	addField( "AllowOnTerrain",		TypeBool,		Offset( mFieldData.mAllowOnTerrain,			fxShapeReplicator ) );
	addField( "AllowOnInteriors",	TypeBool,		Offset( mFieldData.mAllowOnInteriors,		fxShapeReplicator ) );
	addField( "AllowOnStatics",		TypeBool,		Offset( mFieldData.mAllowStatics,			fxShapeReplicator ) );
	addField( "AllowOnWater",		TypeBool,		Offset( mFieldData.mAllowOnWater,			fxShapeReplicator ) );
	addField( "AllowWaterSurface",	TypeBool,		Offset( mFieldData.mAllowWaterSurface,		fxShapeReplicator ) );
	addField( "AlignToTerrain",		TypeBool,		Offset( mFieldData.mAlignToTerrain,			fxShapeReplicator ) );
	addField( "Interactions",		TypeBool,		Offset( mFieldData.mInteractions,			fxShapeReplicator ) );
	addField( "AllowedTerrainSlope",TypeS32,		Offset( mFieldData.mAllowedTerrainSlope,	fxShapeReplicator ) );
	addField( "TerrainAlignment",	TypePoint3F,	Offset( mFieldData.mTerrainAlignment,		fxShapeReplicator ) );
	endGroup( "Restraints" );	// MM: Added Group Footer.

	addGroup( "Object Transforms" );	// MM: Added Group Header.
	addField( "ShapeScaleMin",		TypePoint3F,	Offset( mFieldData.mShapeScaleMin,			fxShapeReplicator ) );
	addField( "ShapeScaleMax",		TypePoint3F,	Offset( mFieldData.mShapeScaleMax,			fxShapeReplicator ) );
	addField( "ShapeRotateMin",		TypePoint3F,	Offset( mFieldData.mShapeRotateMin,			fxShapeReplicator ) );
	addField( "ShapeRotateMax",		TypePoint3F,	Offset( mFieldData.mShapeRotateMax,			fxShapeReplicator ) );
	addField( "OffsetZ",			TypeS32,		Offset( mFieldData.mOffsetZ,				fxShapeReplicator ) );
	endGroup( "Object Transforms" );	// MM: Added Group Footer.

}

//------------------------------------------------------------------------------

void fxShapeReplicator::CreateShapes(void)
{
	F32				HypX, HypY;
	F32				Angle;
	U32				RelocationRetry;
	Point3F			ShapePosition;
	Point3F			ShapeStart;
	Point3F			ShapeEnd;
	Point3F			ShapeScale;
	EulerF			ShapeRotation;
	QuatF			QRotation;
	bool			CollisionResult;
	RayInfo			RayEvent;
	TSShape*		pShape;
	Container*		pContainer;


	// Don't create shapes if we are hiding replications.
	if (mFieldData.mHideReplications) return;

	// Cannot continue without shapes!
	if (mFieldData.mShapeFile == "") return;

	// Check that we can position somewhere!
	if (!(	mFieldData.mAllowOnTerrain ||
			mFieldData.mAllowOnInteriors ||
			mFieldData.mAllowStatics ||
			mFieldData.mAllowOnWater))
	{
		// Problem ...
		Con::warnf(ConsoleLogEntry::General, "[%s] - Could not place object, All alloweds are off!", getName());

		// Return here.
		return;
	}

	// Check Shapes.
	AssertFatal(mCurrentShapeCount==0,"Shapes already present, this should not be possible!")

	// Check that we have a shape...
	if (!mFieldData.mShapeFile) return;

	// Set Seed.
	RandomGen.setSeed(mFieldData.mSeed);

	// Set shape vector.
	mReplicatedShapes.reserve(mFieldData.mShapeCount);

	// Add shapes.
	for (U32 idx = 0; idx < mFieldData.mShapeCount; idx++)
	{
		fxShapeReplicatedStatic*	fxStatic;

		// Create our static shape.
		fxStatic = new fxShapeReplicatedStatic();

		// Set the 'shapeName' field.
		fxStatic->setField("shapeName", mFieldData.mShapeFile);

		// Is this Replicator on the Server?
		if (isServerObject())
			// Yes, so stop it from Ghosting. (Hack, Hack, Hack!)
			fxStatic->touchNetFlags(Ghostable, false);
		else
			// No, so flag as ghost object. (Another damn Hack!)
			fxStatic->touchNetFlags(IsGhost, true);

		// Register the Object.
		if (!fxStatic->registerObject())
		{
			// Problem ...
			Con::warnf(ConsoleLogEntry::General, "[%s] - Could not load shape file '%s'!", getName(), mFieldData.mShapeFile);

			// Destroy Shape.
			delete fxStatic;

			// Destroy existing hapes.
			DestroyShapes();

			// Quit.
			return;
		}

		// Get Allocated Shape.
		pShape = fxStatic->getShape();

		// Reset Relocation Retry.
		RelocationRetry = mFieldData.mShapeRetries;

		// Find it a home ...
		do
		{
			// Get the Replicator Position.
			ShapePosition = getPosition();

			// Calculate a random offset
			HypX	= RandomGen.randF(mFieldData.mInnerRadiusX, mFieldData.mOuterRadiusX);
			HypY	= RandomGen.randF(mFieldData.mInnerRadiusY, mFieldData.mOuterRadiusY);
			Angle	= RandomGen.randF(0, M_2PI);

			// Calcualte the new position.
			ShapePosition.x += HypX * mCos(Angle);
			ShapePosition.y += HypY * mSin(Angle);


			// Initialise RayCast Search Start/End Positions.
			ShapeStart = ShapeEnd = ShapePosition;
			ShapeStart.z = 2000.f;
			ShapeEnd.z= -2000.f;

			// Is this the Server?
			if (isServerObject())
				// Perform Ray Cast Collision on Server Terrain.
				CollisionResult = gServerContainer.castRay(ShapeStart, ShapeEnd, FXREPLICATOR_COLLISION_MASK, &RayEvent);
			else
				// Perform Ray Cast Collision on Client Terrain.
				CollisionResult = gClientContainer.castRay(	ShapeStart, ShapeEnd, FXREPLICATOR_COLLISION_MASK, &RayEvent);

			// Did we hit anything?
			if (CollisionResult)
			{
				// For now, let's pretend we didn't get a collision.
				CollisionResult = false;

				// Yes, so get it's type.
				U32 CollisionType = RayEvent.object->getTypeMask();

				// Check Illegal Placements.
				if (((CollisionType & TerrainObjectType) && !mFieldData.mAllowOnTerrain)	||
					((CollisionType & InteriorObjectType) && !mFieldData.mAllowOnInteriors)	||
					((CollisionType & StaticTSObjectType) && !mFieldData.mAllowStatics)	||
					((CollisionType & WaterObjectType) && !mFieldData.mAllowOnWater) ) continue;

				// If we collided with water and are not allowing on the water surface then let's find the
				// terrain underneath and pass this on as the original collision else fail.
				//
				// NOTE:- We need to do this on the server/client as appropriate.
				if ((CollisionType & WaterObjectType) && !mFieldData.mAllowWaterSurface)
				{
					// Is this the Server?
					if (isServerObject())
					{
						// Yes, so do it on the server container.
						if (!gServerContainer.castRay( ShapeStart, ShapeEnd, FXREPLICATOR_NOWATER_COLLISION_MASK, &RayEvent)) continue;
					}
					else
					{
						// No, so do it on the client container.
						if (!gClientContainer.castRay( ShapeStart, ShapeEnd, FXREPLICATOR_NOWATER_COLLISION_MASK, &RayEvent)) continue;
					}
				}

				// We passed with flying colours so carry on.
				CollisionResult = true;
			}

			// Invalidate if we are below Allowed Terrain Angle.
			if (RayEvent.normal.z < mSin(mDegToRad(90.0f-mFieldData.mAllowedTerrainSlope))) CollisionResult = false;

		// Wait until we get a collision.
		} while(!CollisionResult && --RelocationRetry);

		// Check for Relocation Problem.
		if (RelocationRetry > 0)
		{
			// Adjust Impact point.
			RayEvent.point.z += mFieldData.mOffsetZ;

			// Set New Position.
			ShapePosition = RayEvent.point;
		}
		else
		{
			// Warning.
			Con::warnf(ConsoleLogEntry::General, "[%s] - Could not find satisfactory position for shape '%s' on %s!", getName(), mFieldData.mShapeFile,isServerObject()?"Server":"Client");

			// Unregister Object.
			fxStatic->unregisterObject();

			// Destroy Shape.
			delete fxStatic;

			// Skip to next.
			continue;
		}

		// Get Shape Transform.
		MatrixF XForm = fxStatic->getTransform();

		// Are we aligning to Terrain?
		if (mFieldData.mAlignToTerrain)
		{
			// Yes, so set rotation to Terrain Impact Normal.
			ShapeRotation = RayEvent.normal * mFieldData.mTerrainAlignment;
		}
		else
		{
			// No, so choose a new Rotation (in Radians).
			ShapeRotation.set(	mDegToRad(RandomGen.randF(mFieldData.mShapeRotateMin.x, mFieldData.mShapeRotateMax.x)),
								mDegToRad(RandomGen.randF(mFieldData.mShapeRotateMin.y, mFieldData.mShapeRotateMax.y)),
								mDegToRad(RandomGen.randF(mFieldData.mShapeRotateMin.z, mFieldData.mShapeRotateMax.z)));
		}

		// Set Quaternion Roation.
		QRotation.set(ShapeRotation);

		// Set Transform Rotation.
		QRotation.setMatrix(&XForm);

		// Set Position.
		XForm.setColumn(3, ShapePosition);

		// Set Shape Position / Rotation.
		fxStatic->setTransform(XForm);

		// Choose a new Scale.
		ShapeScale.set(	RandomGen.randF(mFieldData.mShapeScaleMin.x, mFieldData.mShapeScaleMax.x),
						RandomGen.randF(mFieldData.mShapeScaleMin.y, mFieldData.mShapeScaleMax.y),
						RandomGen.randF(mFieldData.mShapeScaleMin.z, mFieldData.mShapeScaleMax.z));

		// Set Shape Scale.
		fxStatic->setScale(ShapeScale);

		// Lock it.
		fxStatic->setLocked(true);

		// Store Shape in Replicated Shapes Vector.
		mReplicatedShapes[mCurrentShapeCount++] = fxStatic;

	}
}

//------------------------------------------------------------------------------

void fxShapeReplicator::DestroyShapes(void)
{
	// Finish if we didn't create any shapes.
	if (mCurrentShapeCount == 0) return;

	// Remove shapes.
	for (U32 idx = 0; idx < mCurrentShapeCount; idx++)
	{
		fxShapeReplicatedStatic* fxStatic;

		// Fetch the Shape Object.
		fxStatic = mReplicatedShapes[idx];

		// Got a Shape?
		if (fxStatic)
		{
			// Unlock it.
			fxStatic->setLocked(false);

			// Unregister the object.
			fxStatic->unregisterObject();

			// Delete it.
			delete fxStatic;
		}
	}

	// Empty the Replicated Shapes Vector.
	mReplicatedShapes.empty();

	// Reset Shape Count.
	mCurrentShapeCount = 0;
}

//------------------------------------------------------------------------------

void fxShapeReplicator::RenewShapes(void)
{
	// Destroy any shapes.
	DestroyShapes();

	// Don't create shapes on the Server if we don't need interactions.
	if (isServerObject() && !mFieldData.mInteractions) return;

	// Create Shapes.
	CreateShapes();
}

//------------------------------------------------------------------------------

void fxShapeReplicator::StartUp(void)
{
	// Flag, Client Replication Started.
	mClientReplicationStarted = true;

	// Renew shapes.
	RenewShapes();
}

//------------------------------------------------------------------------------

bool fxShapeReplicator::onAdd()
{
	if(!Parent::onAdd()) return(false);

	// Add the Replicator to the Replicator Set.
	dynamic_cast<SimSet*>(Sim::findObject("fxReplicatorSet"))->addObject(this);

	// Set Default Object Box.
	mObjBox.min.set( -0.5, -0.5, -0.5 );
	mObjBox.max.set(  0.5,  0.5,  0.5 );

	// Reset the World Box.
	resetWorldBox();

	// Are we editing the Mission?
	if(gEditingMission)
	{
		// Yes, so set the Render Transform.
		setRenderTransform(mObjToWorld);

		// Add to Scene.
		addToScene();
		mAddedToScene = true;

		// If we are in the editor and we are on the client then
		// we can manually startup replication.
		if (isClientObject()) mClientReplicationStarted = true;
	}

	// Renew shapes on Server.
	if (isServerObject()) RenewShapes();

	// Return OK.
	return(true);
}

//------------------------------------------------------------------------------

void fxShapeReplicator::onRemove()
{
	// Remove the Replicator from the Replicator Set.
	dynamic_cast<SimSet*>(Sim::findObject("fxReplicatorSet"))->removeObject(this);

	// Are we editing the Mission?
	if(gEditingMission)
	{
		// Yes, so remove from Scene.
		removeFromScene();
		mAddedToScene = false;
	}

	// Destroy Shapes.
	DestroyShapes();

	// Do Parent.
	Parent::onRemove();
}

//------------------------------------------------------------------------------

void fxShapeReplicator::inspectPostApply()
{
	// Set Parent.
	Parent::inspectPostApply();

	// Renew Shapes.
	RenewShapes();

	// Set Replication Mask.
	setMaskBits(ReplicationMask);
}

//------------------------------------------------------------------------------

void fxShapeReplicator::onEditorEnable()
{
	// Are we in the Scene?
	if(!mAddedToScene)
	{
		// No, so add to scene.
		addToScene();
		mAddedToScene = true;
	}
}

//------------------------------------------------------------------------------

void fxShapeReplicator::onEditorDisable()
{
	// Are we in the Scene?
	if(mAddedToScene)
	{
		// Yes, so remove from scene.
		removeFromScene();
		mAddedToScene = false;
	}
}

//------------------------------------------------------------------------------

ConsoleFunction(StartClientReplication, void, 1, 1, "StartClientReplication()")
{
	// Find the Replicator Set.
	SimSet *fxReplicatorSet = dynamic_cast<SimSet*>(Sim::findObject("fxReplicatorSet"));

	// Return if Error.
	if (!fxReplicatorSet) return;

	// StartUp Replication Object.
	for (SimSetIterator itr(fxReplicatorSet); *itr; ++itr)
	{
		// Fetch the Replicator Object.
		fxShapeReplicator* Replicator = static_cast<fxShapeReplicator*>(*itr);
		// Start Client Objects Only.
		if (Replicator->isClientObject()) Replicator->StartUp();
	}
	// Info ...
	Con::printf("Client Replication Startup has Happened!");
}


//------------------------------------------------------------------------------

bool fxShapeReplicator::prepRenderImage(SceneState* state, const U32 stateKey, const U32 startZone,
								const bool modifyBaseZoneState)
{
	// Return if last state.
	if (isLastState(state, stateKey)) return false;
	// Set Last State.
	setLastState(state, stateKey);

   // Is Object Rendered?
   if (state->isObjectRendered(this))
   {
		// Yes, so get a SceneRenderImage.
		SceneRenderImage* image = new SceneRenderImage;
		// Populate it.
		image->obj = this;
		image->sortType = SceneRenderImage::Normal;
		// Insert it into the scene images.
		state->insertRenderImage(image);
   }

   return false;
}

//------------------------------------------------------------------------------

void fxShapeReplicator::renderObject(SceneState* state, SceneRenderImage*)
{
	// Return if placement area not needed.
	if (!mFieldData.mShowPlacementArea) return;

	// Check we are in Canonical State.
	AssertFatal(dglIsInCanonicalState(), "Error, GL not in canonical state on entry");

	// Setup out the Projection Matrix/Viewport.
	RectI viewport;
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	dglGetViewport(&viewport);
	state->setupBaseProjection();

	// Setup our rendering state.
	glPushMatrix();
	dglMultMatrix(&getTransform());
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

	// Do we need to draw the Inner Radius?
	if (mFieldData.mInnerRadiusX || mFieldData.mInnerRadiusY)
	{
		// Yes, so draw Inner Radius.
		glBegin(GL_TRIANGLE_STRIP);
		for (U32 Angle = mCreationAreaAngle; Angle < (mCreationAreaAngle+360); Angle++)
		{
			F32		XPos, YPos;

			// Calculate Position.
			XPos = mFieldData.mInnerRadiusX * mCos(mDegToRad(-(F32)Angle));
			YPos = mFieldData.mInnerRadiusY * mSin(mDegToRad(-(F32)Angle));

			// Set Colour.
			glColor4f(	mFieldData.mPlaceAreaColour.red,
						mFieldData.mPlaceAreaColour.green,
						mFieldData.mPlaceAreaColour.blue,
						AREA_ANIMATION_ARC * (Angle-mCreationAreaAngle));

			// Draw Arc Line.
			glVertex3f(	XPos, YPos, -(F32)mFieldData.mPlacementBandHeight/2.0f);
			glVertex3f(	XPos, YPos, +(F32)mFieldData.mPlacementBandHeight/2.0f);

		}
		glEnd();
	}

	// Do we need to draw the Outer Radius?
	if (mFieldData.mOuterRadiusX || mFieldData.mOuterRadiusY)
	{
		// Yes, so draw Outer Radius.
		glBegin(GL_TRIANGLE_STRIP);
		for (U32 Angle = mCreationAreaAngle; Angle < (mCreationAreaAngle+360); Angle++)
		{
			F32		XPos, YPos;

			// Calculate Position.
			XPos = mFieldData.mOuterRadiusX * mCos(mDegToRad(-(F32)Angle));
			YPos = mFieldData.mOuterRadiusY * mSin(mDegToRad(-(F32)Angle));

			// Set Colour.
			glColor4f(	mFieldData.mPlaceAreaColour.red,
						mFieldData.mPlaceAreaColour.green,
						mFieldData.mPlaceAreaColour.blue,
						AREA_ANIMATION_ARC * (Angle-mCreationAreaAngle));

			// Draw Arc Line.
			glVertex3f(	XPos, YPos, -(F32)mFieldData.mPlacementBandHeight/2.0f);
			glVertex3f(	XPos, YPos, +(F32)mFieldData.mPlacementBandHeight/2.0f);

		}
		glEnd();
	}

	// Restore rendering state.
	glDisable(GL_BLEND);
	glPopMatrix();

	// Animate Area Selection.
	mCreationAreaAngle += 20;
	mCreationAreaAngle = mCreationAreaAngle % 360;

	// Restore out nice and friendly canonical state.
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	dglSetViewport(viewport);

	// Check we have restored Canonical State.
	AssertFatal(dglIsInCanonicalState(), "Error, GL not in canonical state on exit");
}

//------------------------------------------------------------------------------

U32 fxShapeReplicator::packUpdate(NetConnection * con, U32 mask, BitStream * stream)
{
	// Pack Parent.
	U32 retMask = Parent::packUpdate(con, mask, stream);

	// Write Replication Flag.
	if (stream->writeFlag(mask & ReplicationMask))
	{
		stream->writeAffineTransform(mObjToWorld);						// Replicator Position.

		stream->writeInt(mFieldData.mSeed, 32);							// Replicator Seed.
		stream->writeInt(mFieldData.mShapeCount, 32);					// Shapes Count.
		stream->writeInt(mFieldData.mShapeRetries, 32);					// Shapes Retries.
		stream->writeString(mFieldData.mShapeFile);
		stream->writeInt(mFieldData.mInnerRadiusX, 32);					// Shapes Inner Radius X.
		stream->writeInt(mFieldData.mInnerRadiusY, 32);					// Shapes Inner Radius Y.
		stream->writeInt(mFieldData.mOuterRadiusX, 32);					// Shapes Outer Radius X.
		stream->writeInt(mFieldData.mOuterRadiusY, 32);					// Shapes Outer Radius Y.
		mathWrite(*stream, mFieldData.mShapeScaleMin);					// Shapes Scale Min.
		mathWrite(*stream, mFieldData.mShapeScaleMax);					// Shapes Scale Max.
		mathWrite(*stream, mFieldData.mShapeRotateMin);					// Shapes Rotate Min.
		mathWrite(*stream, mFieldData.mShapeRotateMax);					// Shapes Rotate Max.
		stream->writeSignedInt(mFieldData.mOffsetZ, 32);				// Shapes Offset Z.
		stream->writeFlag(mFieldData.mAllowOnTerrain);					// Allow on Terrain.
		stream->writeFlag(mFieldData.mAllowOnInteriors);				// Allow on Interiors.
		stream->writeFlag(mFieldData.mAllowStatics);					// Allow on Statics.
		stream->writeFlag(mFieldData.mAllowOnWater);					// Allow on Water.
		stream->writeFlag(mFieldData.mAllowWaterSurface);				// Allow on Water Surface.
		stream->writeSignedInt(mFieldData.mAllowedTerrainSlope, 32);	// Shapes Offset Z.
		stream->writeFlag(mFieldData.mAlignToTerrain);					// Shapes AlignToTerrain.
		mathWrite(*stream, mFieldData.mTerrainAlignment);				// Write Terrain Alignment.
		stream->writeFlag(mFieldData.mHideReplications);				// Hide Replications.
		stream->writeFlag(mFieldData.mInteractions);					// Shape Interactions.
		stream->writeFlag(mFieldData.mShowPlacementArea);				// Show Placement Area Flag.
		stream->writeInt(mFieldData.mPlacementBandHeight, 32);			// Placement Area Height.
		stream->write(mFieldData.mPlaceAreaColour);
	}

	// Were done ...
	return(retMask);
}

//------------------------------------------------------------------------------

void fxShapeReplicator::unpackUpdate(NetConnection * con, BitStream * stream)
{
	// Unpack Parent.
	Parent::unpackUpdate(con, stream);

	// Read Replication Details.
	if(stream->readFlag())
	{
		MatrixF		ReplicatorObjectMatrix;

		stream->readAffineTransform(&ReplicatorObjectMatrix);				// Replication Position.

		mFieldData.mSeed					= stream->readInt(32);			// Replicator Seed.
		mFieldData.mShapeCount				= stream->readInt(32);			// Shapes Count.
		mFieldData.mShapeRetries			= stream->readInt(32);			// Shapes Retries.
		mFieldData.mShapeFile				= stream->readSTString();		// Shape File.
		mFieldData.mInnerRadiusX			= stream->readInt(32);			// Shapes Inner Radius X.
		mFieldData.mInnerRadiusY			= stream->readInt(32);			// Shapes Inner Radius Y.
		mFieldData.mOuterRadiusX			= stream->readInt(32);			// Shapes Outer Radius X.
		mFieldData.mOuterRadiusY			= stream->readInt(32);			// Shapes Outer Radius Y.
		mathRead(*stream, &mFieldData.mShapeScaleMin);						// Shapes Scale Min.
		mathRead(*stream, &mFieldData.mShapeScaleMax);						// Shapes Scale Max.
		mathRead(*stream, &mFieldData.mShapeRotateMin);						// Shapes Rotate Min.
		mathRead(*stream, &mFieldData.mShapeRotateMax);						// Shapes Rotate Max.
		mFieldData.mOffsetZ					= stream->readSignedInt(32);	// Shapes Offset Z.
		mFieldData.mAllowOnTerrain			= stream->readFlag();			// Allow on Terrain.
		mFieldData.mAllowOnInteriors		= stream->readFlag();			// Allow on Interiors.
		mFieldData.mAllowStatics			= stream->readFlag();			// Allow on Statics.
		mFieldData.mAllowOnWater			= stream->readFlag();			// Allow on Water.
		mFieldData.mAllowWaterSurface		= stream->readFlag();			// Allow on Water Surface.
		mFieldData.mAllowedTerrainSlope		= stream->readSignedInt(32);	// Allowed Terrain Slope.
		mFieldData.mAlignToTerrain			= stream->readFlag();			// Read AlignToTerrain.
		mathRead(*stream, &mFieldData.mTerrainAlignment);					// Read Terrain Alignment.
		mFieldData.mHideReplications		= stream->readFlag();			// Hide Replications.
		mFieldData.mInteractions			= stream->readFlag();			// Read Interactions.
		mFieldData.mShowPlacementArea	= stream->readFlag();				// Show Placement Area Flag.
		mFieldData.mPlacementBandHeight	= stream->readInt(32);				// Placement Area Height.
		stream->read(&mFieldData.mPlaceAreaColour);

		// Set Transform.
		setTransform(ReplicatorObjectMatrix);

		// Renew Shapes (only if replication has started).
		if (mClientReplicationStarted) RenewShapes();
	}
}

