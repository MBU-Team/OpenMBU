//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _DNET_H_
#define _DNET_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _EVENT_H_
#include "platform/event.h"
#endif

class BitStream;
class ResizeBitStream;

/// The base class for Torque's networking protocol.
///
/// This implements a sliding window connected message stream over an unreliable transport (UDP). It
/// provides a simple notify protocol to allow subclasses to be aware of what packets were sent
/// succesfully and which failed.
///
/// Basically, a window size of 32 is provided, and each packet contains in the header a bitmask,
/// acknowledging the receipt (or failure to receive) of the last 32 packets.
///
/// @see NetConnection, @ref NetProtocol
class ConnectionProtocol
{
protected:
   U32 mLastSeqRecvdAtSend[32];
   U32 mLastSeqRecvd;
   U32 mHighestAckedSeq;
   U32 mLastSendSeq;
   U32 mAckMask;
   U32 mConnectSequence;
   U32 mLastRecvAckAck;
   bool mConnectionEstablished;
public:
   ConnectionProtocol();

   void buildSendPacketHeader(BitStream *bstream, S32 packetType = 0);

   void sendPingPacket();
   void sendAckPacket();
   void setConnectionEstablished() { mConnectionEstablished = true; }

   bool windowFull();
   bool connectionEstablished();
   void setConnectSequence(U32 connectSeq) { mConnectSequence = connectSeq; }

   virtual void writeDemoStartBlock(ResizeBitStream *stream);
   virtual bool readDemoStartBlock(BitStream *stream);

   virtual void processRawPacket(BitStream *bstream);
   virtual Net::Error sendPacket(BitStream *bstream) = 0;
   virtual void keepAlive() = 0;
   virtual void handleConnectionEstablished() = 0;
   virtual void handleNotify(bool recvd) = 0;
   virtual void handlePacket(BitStream *bstream) = 0;
};

#endif
