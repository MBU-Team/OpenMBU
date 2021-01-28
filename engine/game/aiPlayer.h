//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _AIPLAYER_H_
#define _AIPLAYER_H_

#ifndef _PLAYER_H_
#include "game/player.h"
#endif


class AIPlayer : public Player {

	typedef Player Parent;

public:
	enum MoveState {
		ModeStop,
		ModeMove,
		ModeStuck,
	};

private:
		MoveState mMoveState;
		F32 mMoveSpeed;
		F32 mMoveTolerance;                 // Distance from destination before we stop
		Point3F mMoveDestination;           // Destination for movement
      Point3F mLastLocation;              // For stuck check
      bool mMoveSlowdown;                 // Slowdown as we near the destination

		SimObjectPtr<GameBase> mAimObject; // Object to point at, overrides location
		bool mAimLocationSet;               // Has an aim location been set?
		Point3F mAimLocation;               // Point to look at
      bool mTargetInLOS;                  // Is target object visible?

      Point3F mAimOffset;

      // Utility Methods
      void throwCallback( const char *name );
public:
		DECLARE_CONOBJECT( AIPlayer );

		AIPlayer();
      ~AIPlayer();

		virtual bool getAIMove( Move *move );

		// Targeting and aiming sets/gets
		void setAimObject( GameBase *targetObject );
      void setAimObject( GameBase *targetObject, Point3F offset );
		GameBase* getAimObject() const  { return mAimObject; }
		void setAimLocation( const Point3F &location );
		Point3F getAimLocation() const { return mAimLocation; }
		void clearAim();

		// Movement sets/gets
		void setMoveSpeed( const F32 speed );
		F32 getMoveSpeed() const { return mMoveSpeed; }
		void setMoveTolerance( const F32 tolerance );
		F32 getMoveTolerance() const { return mMoveTolerance; }
		void setMoveDestination( const Point3F &location, bool slowdown );
		Point3F getMoveDestination() const { return mMoveDestination; }
		void stopMove();

};

#endif
