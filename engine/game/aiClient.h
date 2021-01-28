//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _AICLIENT_H_
#define _AICLIENT_H_

#ifndef _AICONNECTION_H_
#include "game/aiConnection.h"
#endif
#ifndef _TVECTOR_H_
#include "core/tVector.h"
#endif

class Player;

class AIClient : public AIConnection {

	typedef AIConnection Parent;

	private:
		enum {
			FireTrigger = 0,
			JumpTrigger = 2,
			JetTrigger = 3,
			GrenadeTrigger = 4,
			MineTrigger = 5
		};

		F32 mMoveSpeed;
		S32 mMoveMode;
		F32 mMoveTolerance; // How close to the destination before we stop

		bool mTriggers[MaxTriggerKeys];

		Player *mPlayer;

		Point3F mMoveDestination;
		Point3F mLocation;
      Point3F mLastLocation; // For stuck check

		bool mAimToDestination;
		Point3F mAimLocation;
      bool mTargetInLOS;
		
		SimObjectPtr<ShapeBase> mTargetObject;

      // Utility Methods
      void throwCallback( const char *name );
	public:
		
		DECLARE_CONOBJECT( AIClient );

		enum {
			ModeStop = 0,
			ModeMove,
			ModeStuck,
			ModeCount		// This is in there as a max index value
		};

		AIClient();
      ~AIClient();

		void getMoveList( Move **movePtr,U32 *numMoves );

		// ---Targeting and aiming sets/gets
		void setTargetObject( ShapeBase *targetObject );
		S32 getTargetObject() const;

		// ---Movement sets/gets
		void setMoveSpeed( const F32 speed );
		F32 getMoveSpeed() const { return mMoveSpeed; }

		void setMoveMode( S32 mode );
		S32 getMoveMode() const { return mMoveMode; }

		void setMoveTolerance( const F32 tolerance );
		F32 getMoveTolerance() const { return mMoveTolerance; }

		void setMoveDestination( const Point3F &location );
		Point3F getMoveDestination() const { return mMoveDestination; }

      Point3F getLocation() const { return mLocation; }

		// ---Facing(Aiming) sets/gets
		void setAimLocation( const Point3F &location );
		Point3F getAimLocation() const { return mAimLocation; }
		void clearAim();

		// ---Other
		void missionCycleCleanup();
      void onAdd( const char *nameSpace );
};

#endif
