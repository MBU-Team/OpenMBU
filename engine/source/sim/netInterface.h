#ifndef _H_NETINTERFACE
#define _H_NETINTERFACE

#include "core/torqueConfig.h"

/// NetInterface class.  Manages all valid and pending notify protocol connections.
///
/// @see NetConnection, GameConnection, NetObject, NetEvent
class NetInterface
{
public:
    /// PacketType is encoded as the first byte of each packet.  If the LSB of
    /// the first byte is set (i.e. if the type number is odd), then the packet
    /// is a data protocol packet, otherwise it's an OOB packet, suitable for
    /// use in strange protocols, like game querying or connection initialization.
    enum PacketTypes
    {
        MasterServerGameTypesRequest = 2,
        MasterServerGameTypesResponse = 4,
        MasterServerListRequest = 6,
        MasterServerListResponse = 8,
        GameMasterInfoRequest = 10,
        GameMasterInfoResponse = 12,
        GamePingRequest = 14,
        GamePingResponse = 16,
        GameInfoRequest = 18,
        GameInfoResponse = 20,
        GameHeartbeat = 22,
        GGCPacket     = 24,
        ConnectChallengeRequest = 26,
        ConnectChallengeReject = 28,
        ConnectChallengeResponse = 30,
        ConnectRequest = 32,
        ConnectReject = 34,
        ConnectAccept = 36,
        Disconnect = 38,
#ifdef TORQUE_NET_HOLEPUNCHING
        Punch = 40,
        ArrangedConnectRequest = 42,

        MasterServerRequestArrangedConnection = 46,
        MasterServerClientRequestedArrangedConnection = 48,
        MasterServerAcceptArrangedConnection = 50,
        MasterServerArrangedConnectionAccepted = 52,
        MasterServerRejectArrangedConnection = 54,
        MasterServerArrangedConnectionRejected = 56,
        MasterServerGamePingRequest = 58,
        MasterServerGamePingResponse = 60,
        MasterServerGameInfoRequest = 62,
        MasterServerGameInfoResponse = 64,
        MasterServerRelayRequest = 66,
        MasterServerRelayResponse = 68,
        MasterServerRelayReady = 72,
        MasterServerJoinInvite = 74,
        MasterServerJoinInviteResponse = 76,
#endif // TORQUE_NET_HOLEPUNCHING
#ifdef TORQUE_FAST_FILE_TRANSFER
        FileTransferPacket = 82,
#endif // TORQUE_FAST_FILE_TRANSFER
    };
protected:

    Vector<NetConnection*> mPendingConnections;    ///< List of connections that are in the startup phase.
    U32                     mLastTimeoutCheckTime;  ///< Last time all the active connections were checked for timeouts.
    U32                     mRandomHashData[12];    ///< Data that gets hashed with connect challenge requests to prevent connection spoofing.
    bool                    mRandomDataInitialized; ///< Have we initialized our random number generator?
    bool                    mAllowConnections;      ///< Is this NetInterface allowing connections at this time?

    enum NetInterfaceConstants
    {
        MaxPendingConnects = 20,     ///< Maximum number of pending connections.  If new connection requests come in before
        ChallengeRetryCount = 4,      ///< Number of times to send connect challenge requests before giving up.
        ChallengeRetryTime = 2500,   ///< Timeout interval in milliseconds before retrying connect challenge.

        ConnectRetryCount = 4,     ///< Number of times to send connect requests before giving up.
        ConnectRetryTime = 2500,  ///< Timeout interval in milliseconds before retrying connect request.
        TimeoutCheckInterval = 1500,  ///< Interval in milliseconds between checking for connection timeouts.
    };

    /// Initialize random data.
    void initRandomData();

    /// @name Connection management
    /// Most of these are pretty self-explanatory.
    /// @{

    void            addPendingConnection(NetConnection* conn);
    NetConnection* findPendingConnection(const NetAddress* address, U32 packetSequence);
    void         removePendingConnection(NetConnection* conn);

    void   sendConnectChallengeRequest(NetConnection* conn);
    void handleConnectChallengeRequest(const NetAddress* addr, BitStream* stream);

    void handleConnectChallengeResponse(const NetAddress* address, BitStream* stream);

    void   sendConnectRequest(NetConnection* conn);
    void handleConnectRequest(const NetAddress* address, BitStream* stream);

    void   sendConnectAccept(NetConnection* conn);
    void handleConnectAccept(const NetAddress* address, BitStream* stream);

    void   sendConnectReject(NetConnection* conn, const char* reason);
    void handleConnectReject(const NetAddress* address, BitStream* stream);

#ifdef TORQUE_NET_HOLEPUNCHING

public:
    /// Begins the connection handshaking process for an arranged connection.
    void startArrangedConnection(NetConnection *conn);

    /// Begins connecting to the relay server.
    void startRelayConnection(NetConnection* conn, const NetAddress* theAddress);
    
protected:
    /// Sends Punch packets to each address in the possible connection address list.
    void sendPunchPackets(NetConnection *conn);

    /// Handles an incoming Punch packet from a remote host.
    void handlePunch(const NetAddress* theAddress, BitStream *stream);

    /// Sends an arranged connect request.
    void sendArrangedConnectRequest(NetConnection *conn);

    /// Handles an incoming connect request from an arranged connection.
    void handleArrangedConnectRequest(const NetAddress* theAddress, BitStream *stream);

    /// Sends relay requests
    void sendRelayPackets(NetConnection* conn);

#endif // TORQUE_NET_HOLEPUNCHING

    void handleDisconnect(const NetAddress* address, BitStream* stream);

    /// @}

    /// Calculate an MD5 sum representing a connection, and store it into addressDigest.
    void computeNetMD5(const NetAddress* address, U32 connectSequence, U32 addressDigest[4]);

public:
    NetInterface();

    /// Returns whether or not this NetInterface allows connections from remote hosts.
    bool doesAllowConnections() { return mAllowConnections; }

    /// Sets whether or not this NetInterface allows connections from remote hosts.
    void setAllowsConnections(bool conn) { mAllowConnections = conn; }

    /// Dispatch function for processing all network packets through this NetInterface.
    virtual void processPacketReceiveEvent(PacketReceiveEvent* event);

    /// Handles all packets that don't fall into the category of connection handshake or game data.
    virtual void handleInfoPacket(const NetAddress* address, U8 packetType, BitStream* stream);

    /// Checks all connections marked as client to server for packet sends.
    void processClient();

    /// Checks all connections marked as server to client for packet sends.
    void processServer();

    /// Begins the connection handshaking process for a connection.
    void startConnection(NetConnection* conn);

    /// Checks for timeouts on all valid and pending connections.
    void checkTimeouts();

    /// Send a disconnect packet on a connection, along with a reason.
    void sendDisconnectPacket(NetConnection* conn, const char* reason);
};

/// The global net interface instance.
extern NetInterface* GNet;
#endif
