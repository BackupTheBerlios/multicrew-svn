/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file
 * @brief Provide Reliability Communication 
 * 
 * 
 * This file is part of RakNet Copyright 2003, 2004 Rakkarsoft LLC and
 * Kevin Jenkins.
 *
 * Usage of Raknet is subject to the appropriate licence agreement.
 * "Shareware" Licensees with Rakkarsoft LLC are subject to the
 * shareware license found at
 * http://www.rakkarsoft.com/shareWareLicense.html which you agreed to
 * upon purchase of a "Shareware license" "Commercial" Licensees with
 * Rakkarsoft LLC are subject to the commercial license found at
 * http://www.rakkarsoft.com/sourceCodeLicense.html which you agreed to
 * upon purchase of a "Commercial license" All other users are subject
 * to the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version.
 *
 * Refer to the appropriate license agreement for distribution,
 * modification, and warranty rights.
 */

#ifndef __RELIABILITY_LAYER_H
#define __RELIABILITY_LAYER_H

#include "MTUSize.h"
#include "LinkedList.h"
#include "ArrayList.h"
#include "SocketLayer.h"
#include "PacketPriority.h"
#include "Queue.h"
#include "BitStream.h"
#include "SimpleMutex.h"
#include "InternalPacket.h"
#include "InternalPacketPool.h"
#include "DataBlockEncryptor.h"
#include "RakNetStatistics.h" 
/**
 * Sizeof an UDP header in byte 
 */
#define UDP_HEADER_SIZE 28 
/**
 * Number of ordered streams available. 
 * You can use up to 32 ordered streams 
 */
#define NUMBER_OF_ORDERED_STREAMS 4096 // 2^5 
/**
 * Timeout before killing a connection. If no response to a reliable
 * packet for this long kill the connection
 */
const unsigned long TIMEOUT_TIME = 10000;
/**
 * If you change MAX_AVERAGE_PACKETS_PER_SECOND or TIMEOUT_TIME, you
 * must make sure RECEIVED_PACKET_LOG_LENGTH < the range of
 * PacketNumberType (held in InternalPacket.h)
 * 6553.5 is the maximum for an unsigned short
 * @attention 
 * take in account the value of RECEIVED_PACKET_LOG_LENGTH when changing this! 
 *
 */
const int MAX_AVERAGE_PACKETS_PER_SECOND = 6553;

/**
 * This value must be less than the range of PacketNumberType. 
 * PacketNumberType is in InternalPacket.h
 */
const int RECEIVED_PACKET_LOG_LENGTH = ( TIMEOUT_TIME / 1000 ) * MAX_AVERAGE_PACKETS_PER_SECOND;


#include "BitStream.h" 
/**
 * This class provide reliable communication services. 
 * 
 */

class ReliabilityLayer
{

public:
	/**
	 * Default Constructor
	 */
	ReliabilityLayer();
	
	/**
	 * Destructor
	 */
	~ReliabilityLayer();
	
	/**
	 * Resets the layer for reuse.
	 * Callable from multiple threads
	 */
	void Reset( void );
	/**
	 * Sets up encryption
	 * Callable from multiple threads
	 * @param key the key used for encryption 
	 */
	void SetEncryptionKey( const unsigned char *key );
	/**
	 * Assign a socket for the reliability layer to use for writing
	 * Callable from multiple threads
	 * @param s The socket to use for communication 
	 */
	void SetSocket( SOCKET s );
	
	/**
	 * Get the socket held by the reliability layer
	 * Callable from multiple threads
	 * @return Retrieve the socket so that every body can used it 
	 */
	SOCKET GetSocket( void );
	/**
	 * Must be called by the same thread as update Packets are read
	 * directly from the socket layer and skip the reliability layer
	 * because unconnected players do not use the reliability layer This
	 * function takes packet data after a player has been confirmed as
	 * connected. The game should not use that data directly because
	 * some data is used internally, such as packet acknowledgement and
	 * split packets 
	 * 
	 * @param buffer Store up to @em length byte from the incomming data 
	 * @param length The size of buffer 
	 * @return false on modified packets
	 */
	bool HandleSocketReceiveFromConnectedPlayer( char *buffer, int length );
	
	/**
	 * This gets an end-user packet already parsed out. 
	 *
	 * @param data The game data 
	 * @return Returns number of BITS put into the buffer 
	 * @note Callable from multiple threads
	 *
	 */
	int Receive( char**data );
	
	/**
	 * Puts data on the send queue
	 * @param bitStream contains the data to send
	 * @param priority is what priority to send the data at
	 * @param reliability is what reliability to use
	 * @param orderingChannel is from 0 to 31 and specifies what stream to use
	 * @param makeDataCopy if true @em bitStream will keep an internal copy of 
	 * the packet. 
	 * @param MTUSize
	 * @note Callable from multiple threads
	 * 
	 * @todo Document MTUSize parameter 
	 */
	bool Send( RakNet::BitStream *bitStream, PacketPriority priority, PacketReliability reliability, unsigned orderingChannel, bool makeDataCopy, int MTUSize );
	
	/**
	 * Run this once per game cycle.  Handles internal lists and
	 * actually does the send Must be called by the same thread as
	 * HandleSocketReceiveFromConnectedPlayer 
	 * 
	 * @param s the communication  end point 
	 * @param playerId The Unique Player Identifier who should
	 * have sent some packets
	 * @param MTUSize 
	 * @param time 
	 * @todo
	 * Document MTUSize and time parameter 
	 */
	void Update( SOCKET s, PlayerID playerId, int MTUSize, unsigned long time );
	
	/**
	 * If Read returns -1 and this returns true then a modified packet
	 * was detected
	 * @return true when a modified packet is detected 
	 */
	bool IsCheater( void ) const;
	/**
	 * Were you ever unable to deliver a packet despite retries?
	 * @return true if the connection no more responds 
	 */
	bool IsDeadConnection( void ) const;
	
	/**
	 * How long to wait between packet resends
	 * @param i time to wait before resending a packet 
	 */
	void SetLostPacketResendDelay( unsigned long i );
	
	/**
	 * Get Statistics
	 * @return The object containing all stats
	 */
	RakNetStatisticsStruct * const GetStatistics( void );
	
private:
	/**
	 * Returns true if we can or should send a frame.  False if we should not
	 * @param time The time to wait before sending a frame 
	 * @return true if we can or should send a frame. 
	 */
	bool IsFrameReady( unsigned long time );
	
	/**
	 * Generates a frame (coalesced packets)
	 * @param output The resulting BitStream 
	 * @param MTUSize 
	 * @param reliableDataSent True if we have to sent a reliable data 
	 * @param time Delay to send the packet or deadline 
	 * @todo
	 * Check for time paramter 
	 * 
	 */
	void GenerateFrame( RakNet::BitStream *output, int MTUSize, bool *reliableDataSent, unsigned long time );
	
	/**
	 * Writes a bitstream to the socket
	 * @param s The socket used for sending data 
	 * @param playerId The target of the communication 
	 * @param bitStream The data to send. 
	 */
	void SendBitStream( SOCKET s, PlayerID playerId, RakNet::BitStream *bitStream );
	/**
	 * Parse an internalPacket and create a bitstream to represent this data
	 * Returns number of bits used
	 */
	int WriteToBitStreamFromInternalPacket( RakNet::BitStream *bitStream, const InternalPacket *const internalPacket );
	
	// Parse a bitstream and create an internal packet to represent this data
	InternalPacket* CreateInternalPacketFromBitStream( RakNet::BitStream *bitStream, unsigned long time );
	
	// Does what the function name says
	void RemovePacketFromResendQueueAndDeleteOlderReliableSequenced( PacketNumberType packetNumber );
	
	// Acknowledge receipt of the packet with the specified packetNumber
	void SendAcknowledgementPacket( PacketNumberType packetNumber, unsigned long time );
	
	// This will return true if we should not send at this time
	bool IsSendThrottled( void );
	
	// We lost a packet
	void UpdatePacketloss( unsigned long time );
	
	// Parse an internalPacket and figure out how many header bits would be written.  Returns that number
	int GetBitStreamHeaderLength( const InternalPacket *const internalPacket );
	
	// Get the SHA1 code
	void GetSHA1( unsigned char * const buffer, unsigned long nbytes, char code[ SHA1_LENGTH ] );
	
	// Check the SHA1 code
	bool CheckSHA1( char code[ SHA1_LENGTH ], unsigned char * const buffer, unsigned long nbytes );
	
	// Search the specified list for sequenced packets on the specified ordering channel, optionally skipping those with splitPacketId, and delete them
	void DeleteSequencedPacketsInList( unsigned orderingChannel, BasicDataStructures::List<InternalPacket*>&theList, int splitPacketId = -1 );
	// Search the specified list for sequenced packets with a value less than orderingIndex and delete them
	void DeleteSequencedPacketsInList( unsigned orderingChannel, BasicDataStructures::Queue<InternalPacket*>&theList );
	
	// Returns true if newPacketOrderingIndex is older than the waitingForPacketOrderingIndex
	bool IsOlderOrderedPacket( unsigned char newPacketOrderingIndex, unsigned char waitingForPacketOrderingIndex );
	
	// Split the passed packet into chunks under MTU_SIZE bytes (including headers) and save those new chunks
	void SplitPacketAndDeleteOriginal( InternalPacket *internalPacket, int MTUSize );
	
	// Insert a packet into the split packet list
	void InsertIntoSplitPacketList( InternalPacket * internalPacket );
	
	// Take all split chunks with the specified splitPacketId and try to reconstruct a packet. If we can, allocate and return it.  Otherwise return 0
	InternalPacket * BuildPacketFromSplitPacketList( unsigned long splitPacketId, unsigned long time );
	
	// Delete any unreliable split packets that have long since expired
	void DeleteOldUnreliableSplitPackets( unsigned long time );
	
	// Creates a copy of the specified internal packet with data copied from the original starting at dataByteOffset for dataByteLength bytes.
	// Does not copy any split data parameters as that information is always generated does not have any reason to be copied
	InternalPacket * CreateInternalPacketCopy( InternalPacket *original, int dataByteOffset, int dataByteLength, unsigned long time );
	
	// Get the specified ordering list
	// LOCK THIS WHOLE BLOCK WITH reliabilityLayerMutexes[orderingList_MUTEX].Unlock();
	BasicDataStructures::LinkedList<InternalPacket*> *GetOrderingListAtOrderingStream( unsigned orderingChannel );
	
	// Add the internal packet to the ordering list in order based on order index
	void AddToOrderingList( InternalPacket * internalPacket );
	
	// Inserts a packet into the resend list in order
	// THIS WHOLE FUNCTION SHOULD BE LOCKED WITH
	// reliabilityLayerMutexes[resendQueue_MUTEX].Lock();
	void InsertPacketIntoResendQueue( InternalPacket *internalPacket, unsigned long time );
	
	// Memory handling
	void FreeMemory( bool freeAllImmediately );
	void FreeThreadedMemory( void );
	void FreeThreadSafeMemory( void );
	
	// Initialize the variables
	void InitializeVariables( void );
	
	// STUFF TO MUTEX HERE
	enum
	{
		// splitPacketList_MUTEX, // We don't have to mutex this as long as Update and HandleSocketReceiveFromConnectedPlayer are called by the same thread
		sendQueueSystemPriority_MUTEX,
		sendQueueHighPriority_MUTEX,
		sendQueueMediumPriority_MUTEX,
		sendQueueLowPriority_MUTEX,
		//resendQueue_MUTEX,// We don't have to mutex this as long as Update and HandleSocketReceiveFromConnectedPlayer are called by the same thread
		//orderingList_MUTEX,// We don't have to mutex this as long as Update and HandleSocketReceiveFromConnectedPlayer are called by the same thread
		//acknowledgementQueue_MUTEX,// We don't have to mutex this as long as Update and HandleSocketReceiveFromConnectedPlayer are called by the same thread
		// outputQueue_MUTEX,// We don't have to mutex this as long as Recieve and HandleSocketReceiveFromConnectedPlayer are called by the same thread
		packetNumber_MUTEX,
		// windowSize_MUTEX, // Causes long delays for some reason
		//lastAckTime_MUTEX,// We don't have to mutex this as long as Update and HandleSocketReceiveFromConnectedPlayer are called by the same thread
		//updateBitStream_MUTEX,// We don't have to mutex this as long as Update and HandleSocketReceiveFromConnectedPlayer are called by the same thread
		waitingForOrderedPacketWriteIndex_MUTEX,
		waitingForSequencedPacketWriteIndex_MUTEX,
		NUMBER_OF_RELIABILITY_LAYER_MUTEXES
	};
	SimpleMutex reliabilityLayerMutexes[ NUMBER_OF_RELIABILITY_LAYER_MUTEXES ];
	
	BasicDataStructures::List<InternalPacket*> splitPacketList;
	BasicDataStructures::List<BasicDataStructures::LinkedList<InternalPacket*>*> orderingList;
	BasicDataStructures::Queue<InternalPacket*> acknowledgementQueue, outputQueue;
	BasicDataStructures::Queue<InternalPacket*> sendQueue[ NUMBER_OF_PRIORITIES ], resendQueue;
	PacketNumberType packetNumber;
	//unsigned long windowSize;
	unsigned long lastAckTime;
	RakNet::BitStream updateBitStream;
	unsigned waitingForOrderedPacketWriteIndex[ NUMBER_OF_ORDERED_STREAMS ], waitingForSequencedPacketWriteIndex[ NUMBER_OF_ORDERED_STREAMS ];
	// Used for flow control (changed to regular TCP sliding window)
	// unsigned long maximumWindowSize, bytesSentSinceAck;
	// unsigned long outputWindowFullTime; // under linux if this last variable is on the line above it the delete operator crashes deleting this class!
	
	// STUFF TO NOT MUTEX HERE (called from non-conflicting threads, or value is not important)
	unsigned waitingForOrderedPacketReadIndex[ NUMBER_OF_ORDERED_STREAMS ], waitingForSequencedPacketReadIndex[ NUMBER_OF_ORDERED_STREAMS ];
	bool deadConnection, cheater;
	// unsigned long lastPacketSendTime,retransmittedFrames, sentPackets, sentFrames, receivedPacketsCount, bytesSent, bytesReceived,lastPacketReceivedTime;
	unsigned long lostPacketResendDelay;
	unsigned long splitPacketId;
	unsigned long receivedPackets[ RECEIVED_PACKET_LOG_LENGTH ];
	unsigned long blockWindowIncreaseUntilTime;
	RakNetStatisticsStruct statistics;
	
	// Windowing algorithm:
	// Start at a minimum size
	// Set the lossy window size to INFINITE
	// If the current window size is lower than the lossy window size, then increase the window size by 1 per frame if the number of acks is >= the window size and data is waiting to go out.
	// Otherwise, do the same, but also apply the limit of 1 frame per second. If we are more than 5 over the lossy window size, increase the lossy window size by 1
	// If we lose a frame, decrease the window size by 1 per frame lost.  Record the new window size as the lossy window size.
	int windowSize;
	int lossyWindowSize;
	unsigned long lastWindowIncreaseSizeTime;
	DataBlockEncryptor encryptor;
	
#ifdef __USE_IO_COMPLETION_PORTS
	/**
	 * @note Windows Port only 
	 *
	 */
	SOCKET readWriteSocket;
#endif
	/**
	 * This variable is so that free memory can be called by only the
	 * update thread so we don't have to mutex things so much 
	 */
	bool freeThreadedMemoryOnNextUpdate;
};

#endif

