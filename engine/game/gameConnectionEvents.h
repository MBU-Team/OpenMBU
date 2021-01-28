//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GAMECONNECTIONEVENTS_H_
#define _GAMECONNECTIONEVENTS_H_

class QuitEvent : public SimEvent
{
   void process(SimObject *object)
   {
      Platform::postQuitMessage(0);
   }
};

class SimDataBlockEvent : public NetEvent
{
   SimObjectId id;
   SimDataBlock *mObj;
   U32 mIndex;
   U32 mTotal;
   U32 mMissionSequence;
   bool mProcess;
  public:
   ~SimDataBlockEvent();
   SimDataBlockEvent(SimDataBlock* obj = NULL, U32 index = 0, U32 total = 0, U32 missionSequence = 0);
   void pack(NetConnection *, BitStream *bstream);
   void write(NetConnection *, BitStream *bstream);
   void unpack(NetConnection *cptr, BitStream *bstream);
   void process(NetConnection*);
   void notifyDelivered(NetConnection *, bool);
#ifdef TORQUE_DEBUG_NET
   const char *getDebugName();
#endif
   DECLARE_CONOBJECT(SimDataBlockEvent);
};

class Sim2DAudioEvent: public NetEvent
{
  private:
   const AudioProfile *mProfile;

  public:
   Sim2DAudioEvent(const AudioProfile *profile=NULL);
   void pack(NetConnection *, BitStream *bstream);
   void write(NetConnection *, BitStream *bstream);
   void unpack(NetConnection *, BitStream *bstream);
   void process(NetConnection *);
   DECLARE_CONOBJECT(Sim2DAudioEvent);
};

class Sim3DAudioEvent: public NetEvent
{
  private:
   const AudioProfile *mProfile;
   MatrixF mTransform;

  public:
   Sim3DAudioEvent(const AudioProfile *profile=NULL,const MatrixF* mat=NULL);
   void pack(NetConnection *, BitStream *bstream);
   void write(NetConnection *, BitStream *bstream);
   void unpack(NetConnection *, BitStream *bstream);
   void process(NetConnection *);
   DECLARE_CONOBJECT(Sim3DAudioEvent);
};


//----------------------------------------------------------------------------
// used to set the crc for the current mission (mission lighting)
//----------------------------------------------------------------------------
class SetMissionCRCEvent : public NetEvent
{
   private:
      U32   mCrc;

   public:
      SetMissionCRCEvent(U32 crc = 0xffffffff)
         { mCrc = crc; }
      void pack(NetConnection *, BitStream * bstream)
         { bstream->write(mCrc); }
      void write(NetConnection * con, BitStream * bstream)
         { pack(con, bstream); }
      void unpack(NetConnection *, BitStream * bstream)
         { bstream->read(&mCrc); }
      void process(NetConnection * con)
         { static_cast<GameConnection*>(con)->setMissionCRC(mCrc); }

      DECLARE_CONOBJECT(SetMissionCRCEvent);
};

#endif
