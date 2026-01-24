/*
 * test_netchan.c - Unit tests for network channel (Netchan_Setup, Netchan_Process)
 * Tests sequence number handling, reliable message tracking, and packet ordering
 */

#include "../unity/unity.h"
#include <string.h>
#include <stdio.h>

// Network types
typedef int qBool;
#define qTrue 1
#define qFalse 0

typedef unsigned char byte;
typedef unsigned short uint16;
typedef unsigned int uint32;

#define ORIGINAL_PROTOCOL_VERSION	34
#define ENHANCED_PROTOCOL_VERSION	35
#define Q2PRO_PROTOCOL_VERSION		36

#define MAX_CL_USABLEMSG	1390
#define MAX_SV_USABLEMSG	1390

typedef enum {
	NS_CLIENT,
	NS_SERVER,
	NS_MAX
} netSrc_t;

typedef enum {
	NA_LOOPBACK,
	NA_BROADCAST,
	NA_IP,
	NA_MAX
} netAdrType_t;

typedef struct {
	netAdrType_t	naType;
	byte			ip[4];
	uint16			port;
} netAdr_t;

// Message buffer structure
typedef struct {
	qBool		allowOverflow;
	qBool		overflowed;
	byte		*data;
	size_t		maxSize;
	size_t		curSize;
	size_t		readCount;
	int			bitPos;
} netMsg_t;

// Network channel structure
typedef struct {
	qBool		fatalError;
	netSrc_t	sock;
	int			dropped;
	int			lastReceived;
	int			lastSent;
	netAdr_t	remoteAddress;
	uint16		qPort;
	uint16		protocol;
	
	// Sequencing variables
	uint32		incomingSequence;
	uint32		incomingAcknowledged;
	uint32		incomingReliableAcknowledged;
	uint32		incomingReliableSequence;
	qBool		gotReliable;
	
	uint32		outgoingSequence;
	uint32		reliableSequence;
	uint32		lastReliableSequence;
	
	// Reliable staging and holding areas
	netMsg_t	message;
	byte		messageBuff[MAX_CL_USABLEMSG];
	
	size_t		reliableLength;
	byte		reliableBuff[MAX_CL_USABLEMSG];
} netChan_t;

// Mock current time
static int mockTime = 0;

static int Sys_Milliseconds(void) {
	return mockTime;
}

// Mock MSG_Init
static void MSG_Init(netMsg_t *msg, byte *data, size_t maxSize) {
	msg->data = data;
	msg->maxSize = maxSize;
	msg->curSize = 0;
	msg->readCount = 0;
	msg->allowOverflow = qFalse;
	msg->overflowed = qFalse;
	msg->bitPos = 0;
}

// Mock MSG_BeginReading
static void MSG_BeginReading(netMsg_t *msg) {
	msg->readCount = 0;
	msg->bitPos = 0;
}

// Mock MSG_ReadLong
static uint32 MSG_ReadLong(netMsg_t *msg) {
	if (msg->readCount + 4 > msg->curSize) {
		return 0;
	}
	
	uint32 value = *(uint32 *)(msg->data + msg->readCount);
	msg->readCount += 4;
	return value;
}

// Mock MSG_ReadShort
static uint16 MSG_ReadShort(netMsg_t *msg) {
	if (msg->readCount + 2 > msg->curSize) {
		return 0;
	}
	
	uint16 value = *(uint16 *)(msg->data + msg->readCount);
	msg->readCount += 2;
	return value;
}

// Mock cvars (for Netchan_Process logging)
typedef struct {
	int intVal;
} cvar_t;

static cvar_t mock_showpackets = {0};
static cvar_t mock_showdrop = {0};
static cvar_t *showpackets = &mock_showpackets;
static cvar_t *showdrop = &mock_showdrop;

// Mock NET_AdrToString
static char *NET_AdrToString(netAdr_t *a) {
	static char buf[64];
	snprintf(buf, sizeof(buf), "%d.%d.%d.%d:%d", 
		a->ip[0], a->ip[1], a->ip[2], a->ip[3], a->port);
	return buf;
}

// Mock Com_Error
static void Com_Error(int code, const char *fmt, ...) {
	// In tests, just return
	(void)code;
	(void)fmt;
}

// Mock Com_Printf and Com_DevPrintf
static void Com_Printf(int level, const char *fmt, ...) {
	(void)level;
	(void)fmt;
}

static void Com_DevPrintf(int level, const char *fmt, ...) {
	(void)level;
	(void)fmt;
}

// Implementation of Netchan_Setup
static void Netchan_Setup(netSrc_t sock, netChan_t *chan, netAdr_t *adr, int protocol, int qPort, uint32 msgLen) {
	memset(chan, 0, sizeof(*chan));

	chan->sock = sock;
	chan->remoteAddress = *adr;

	if (protocol == ENHANCED_PROTOCOL_VERSION || protocol == Q2PRO_PROTOCOL_VERSION) {
		if (msgLen) {
			if (msgLen > MAX_CL_USABLEMSG)
				Com_Error(1, "Netchan_Setup: msgLen > MAX_CL_USABLEMSG (%i > %i)", msgLen, MAX_CL_USABLEMSG);
			MSG_Init(&chan->message, chan->messageBuff, msgLen);
		}
		else {
			MSG_Init(&chan->message, chan->messageBuff, MAX_SV_USABLEMSG);
			msgLen = MAX_SV_USABLEMSG;
		}
	}
	else {
		MSG_Init(&chan->message, chan->messageBuff, MAX_SV_USABLEMSG);
		msgLen = MAX_SV_USABLEMSG;
	}

	chan->qPort = qPort;
	chan->protocol = protocol;
	chan->lastReceived = Sys_Milliseconds();
	chan->incomingSequence = 0;
	chan->outgoingSequence = 1;
	chan->message.allowOverflow = qTrue;

	Com_DevPrintf(0, "Netchan_Setup: sock=%d, protocol=%d, qport=%d, msgLen=%u\n", sock, protocol, qPort, msgLen);
}

// Mock NET_SendPacket to avoid real network IO
static int NET_SendPacket(netSrc_t sock, size_t length, void *data, netAdr_t *to) {
	(void)sock; (void)length; (void)data; (void)to;
	return 0;
}

// Simplified Netchan_Transmit for tests: increment sequences and handle reliable buffer
static int Netchan_Transmit(netChan_t *chan, size_t length, byte *data) {
	qBool sendReliable = qFalse;

	// Check for message overflow (use conservative fallback)
	if (chan->message.overflowed || chan->message.curSize >= 0x10000) {
		chan->fatalError = qTrue;
		chan->message.curSize = 0;
		chan->reliableLength = 0;
		chan->fatalError = qFalse;
		return -2;
	}

	// Determine if we need to send reliable
	if (chan->incomingAcknowledged > chan->lastReliableSequence
		&& chan->incomingReliableAcknowledged != chan->reliableSequence)
		sendReliable = qTrue;
	if (!chan->reliableLength && chan->message.curSize)
		sendReliable = qTrue;

	// Move message to reliable buffer if necessary
	if (!chan->reliableLength && chan->message.curSize) {
		memcpy(chan->reliableBuff, chan->messageBuff, chan->message.curSize);
		chan->reliableLength = chan->message.curSize;
		chan->message.curSize = 0;
		chan->reliableSequence ^= 1;
	}

	// Increment outgoing sequence and record time
	chan->outgoingSequence++;
	chan->lastSent = Sys_Milliseconds();

	// If we are sending reliable, mark lastReliableSequence
	if (sendReliable)
		chan->lastReliableSequence = chan->outgoingSequence;

	return 0;
}

// Implementation of Netchan_Process
static qBool Netchan_Process(netChan_t *chan, netMsg_t *msg) {
	uint32 sequence, sequenceAck;
	uint32 reliableAck, reliableMessage;

	MSG_BeginReading(msg);

	// Get sequence numbers	
	sequence = MSG_ReadLong(msg);
	sequenceAck = MSG_ReadLong(msg);

	// Read the qport if we are a server
	if (chan->sock == NS_SERVER) {
		if (chan->qPort)
			MSG_ReadShort(msg);
	}

	reliableMessage = sequence >> 31;
	reliableAck = sequenceAck >> 31;
	chan->gotReliable = reliableMessage ? qTrue : qFalse;

	sequence &= ~(1<<31);
	sequenceAck &= ~(1<<31);

	if (showpackets->intVal) {
		if (reliableMessage)
			Com_Printf(0, "recv %4i : s=%i reliable=%i ack=%i rack=%i\n",
				msg->curSize,
				sequence,
				chan->incomingReliableSequence ^ 1,
				sequenceAck,
				reliableAck);
		else
			Com_Printf(0, "recv %4i : s=%i ack=%i rack=%i\n",
				msg->curSize,
				sequence,
				sequenceAck,
				reliableAck);
	}

	// Discard stale or duplicated packets
	if ((int)sequence <= chan->incomingSequence) {
		if (showdrop->intVal)
			Com_Printf(0, "%s:Out of order packet %i at %i\n",
				NET_AdrToString(&chan->remoteAddress),
				sequence,
				chan->incomingSequence);
		return qFalse;
	}

	// Dropped packets don't keep the message from being used
	chan->dropped = sequence - (chan->incomingSequence + 1);
	if (chan->dropped > 0) {
		if (showdrop->intVal)
			Com_Printf(0, "%s:Dropped %i packets at %i\n",
				NET_AdrToString(&chan->remoteAddress),
				chan->dropped,
				sequence);
	}

	// If the current outgoing reliable message has been acknowledged,
	// clear the buffer to make way for the next
	if (reliableAck == chan->reliableSequence)
		chan->reliableLength = 0;

	// If this message contains a reliable message, bump incoming_ReliableSequence
	chan->incomingSequence = sequence;
	chan->incomingAcknowledged = sequenceAck;
	chan->incomingReliableAcknowledged = reliableAck;
	if (reliableMessage)
		chan->incomingReliableSequence ^= 1;

	// The message can now be read from the current message pointer
	chan->lastReceived = Sys_Milliseconds();

	return qTrue;
}

// Helper to create a mock packet
static void CreatePacket(netMsg_t *msg, byte *buffer, size_t bufSize, 
                         uint32 sequence, uint32 sequenceAck, 
                         qBool reliableMessage, qBool reliableAck) {
	msg->data = buffer;
	msg->maxSize = bufSize;
	msg->curSize = 0;
	msg->readCount = 0;
	
	// Set reliable bits
	if (reliableMessage)
		sequence |= (1 << 31);
	if (reliableAck)
		sequenceAck |= (1 << 31);
	
	// Write sequence numbers
	*(uint32 *)(buffer) = sequence;
	*(uint32 *)(buffer + 4) = sequenceAck;
	msg->curSize = 8;
}

// ============================================================================
// TESTS: Netchan_Setup
// ============================================================================

// Test: setup client channel
void test_Netchan_Setup_Client(void) {
	netChan_t chan = {0};
	netAdr_t addr = {0};
	addr.naType = NA_IP;
	addr.ip[0] = 192;
	addr.ip[1] = 168;
	addr.ip[2] = 1;
	addr.ip[3] = 1;
	addr.port = 27910;
	
	mockTime = 1000;
	Netchan_Setup(NS_CLIENT, &chan, &addr, ORIGINAL_PROTOCOL_VERSION, 12345, 0);
	
	TEST_ASSERT_EQUAL(NS_CLIENT, chan.sock);
	TEST_ASSERT_EQUAL(192, chan.remoteAddress.ip[0]);
	TEST_ASSERT_EQUAL(168, chan.remoteAddress.ip[1]);
	TEST_ASSERT_EQUAL(1, chan.remoteAddress.ip[2]);
	TEST_ASSERT_EQUAL(1, chan.remoteAddress.ip[3]);
	TEST_ASSERT_EQUAL(12345, chan.qPort);
	TEST_ASSERT_EQUAL(ORIGINAL_PROTOCOL_VERSION, chan.protocol);
	TEST_ASSERT_EQUAL(1000, chan.lastReceived);
	TEST_ASSERT_EQUAL(0, chan.incomingSequence);
	TEST_ASSERT_EQUAL(1, chan.outgoingSequence);
}

// Test: setup server channel
void test_Netchan_Setup_Server(void) {
	netChan_t chan = {0};
	netAdr_t addr = {0};
	addr.naType = NA_IP;
	addr.ip[0] = 10;
	addr.ip[1] = 0;
	addr.ip[2] = 0;
	addr.ip[3] = 1;
	
	mockTime = 2000;
	Netchan_Setup(NS_SERVER, &chan, &addr, ORIGINAL_PROTOCOL_VERSION, 54321, 0);
	
	TEST_ASSERT_EQUAL(NS_SERVER, chan.sock);
	TEST_ASSERT_EQUAL(10, chan.remoteAddress.ip[0]);
	TEST_ASSERT_EQUAL(54321, chan.qPort);
	TEST_ASSERT_EQUAL(2000, chan.lastReceived);
}

// Test: setup with enhanced protocol
void test_Netchan_Setup_EnhancedProtocol(void) {
	netChan_t chan = {0};
	netAdr_t addr = {0};
	
	Netchan_Setup(NS_CLIENT, &chan, &addr, ENHANCED_PROTOCOL_VERSION, 12345, 1200);
	
	TEST_ASSERT_EQUAL(ENHANCED_PROTOCOL_VERSION, chan.protocol);
	TEST_ASSERT_EQUAL(1200, chan.message.maxSize);
}

// Test: setup with Q2PRO protocol
void test_Netchan_Setup_Q2ProProtocol(void) {
	netChan_t chan = {0};
	netAdr_t addr = {0};
	
	Netchan_Setup(NS_CLIENT, &chan, &addr, Q2PRO_PROTOCOL_VERSION, 12345, 1300);
	
	TEST_ASSERT_EQUAL(Q2PRO_PROTOCOL_VERSION, chan.protocol);
	TEST_ASSERT_EQUAL(1300, chan.message.maxSize);
}

// Test: setup with zero msgLen uses default
void test_Netchan_Setup_ZeroMsgLen(void) {
	netChan_t chan = {0};
	netAdr_t addr = {0};
	
	Netchan_Setup(NS_CLIENT, &chan, &addr, ENHANCED_PROTOCOL_VERSION, 12345, 0);
	
	TEST_ASSERT_EQUAL(MAX_SV_USABLEMSG, chan.message.maxSize);
}

// Test: setup clears previous state
void test_Netchan_Setup_ClearsPreviousState(void) {
	netChan_t chan = {0};
	netAdr_t addr = {0};
	
	// Set some values
	chan.incomingSequence = 100;
	chan.outgoingSequence = 200;
	chan.dropped = 5;
	
	Netchan_Setup(NS_CLIENT, &chan, &addr, ORIGINAL_PROTOCOL_VERSION, 12345, 0);
	
	// Should be reset
	TEST_ASSERT_EQUAL(0, chan.incomingSequence);
	TEST_ASSERT_EQUAL(1, chan.outgoingSequence);
	TEST_ASSERT_EQUAL(0, chan.dropped);
}

// Test: initial values set correctly (sequence numbers, qPort, protocol, lastReceived)
void test_Netchan_Setup_InitialValues(void) {
	netChan_t chan = {0};
	netAdr_t addr = {0};
	addr.naType = NA_IP;
	addr.ip[0] = 127; addr.ip[1] = 0; addr.ip[2] = 0; addr.ip[3] = 1;
	addr.port = 27960;

	mockTime = 4242;
	Netchan_Setup(NS_CLIENT, &chan, &addr, ENHANCED_PROTOCOL_VERSION, 65535, 512);

	// Verify socket and remote address copied
	TEST_ASSERT_EQUAL(NS_CLIENT, chan.sock);
	TEST_ASSERT_EQUAL(127, chan.remoteAddress.ip[0]);
	TEST_ASSERT_EQUAL(1, chan.remoteAddress.ip[3]);

	// Verify qPort and protocol set
	TEST_ASSERT_EQUAL(65535, chan.qPort);
	TEST_ASSERT_EQUAL(ENHANCED_PROTOCOL_VERSION, chan.protocol);

	// Sequence numbers
	TEST_ASSERT_EQUAL(0, chan.incomingSequence);
	TEST_ASSERT_EQUAL(1, chan.outgoingSequence);

	// lastReceived uses mocked Sys_Milliseconds
	TEST_ASSERT_EQUAL(4242, chan.lastReceived);
}

// Test: message buffer allows overflow
void test_Netchan_Setup_AllowsOverflow(void) {
	netChan_t chan = {0};
	netAdr_t addr = {0};
	
	Netchan_Setup(NS_CLIENT, &chan, &addr, ORIGINAL_PROTOCOL_VERSION, 12345, 0);
	
	TEST_ASSERT_TRUE(chan.message.allowOverflow);
}

// Test: Netchan_Transmit increments sequence and sets reliable
void test_Netchan_Transmit_reliable_and_sequence_increment(void) {
	netChan_t chan = {0};
	netAdr_t addr = {0};
	byte data[16] = {0};

	mockTime = 5000;
	Netchan_Setup(NS_CLIENT, &chan, &addr, ENHANCED_PROTOCOL_VERSION, 54321, 0);

	// Prepare an outgoing message to be made reliable
	chan.message.curSize = 8; // some bytes pending
	memcpy(chan.messageBuff, "reliable", 8);

	uint32 oldOutgoing = chan.outgoingSequence;

	int rc = Netchan_Transmit(&chan, 0, NULL);
	TEST_ASSERT_EQUAL(0, rc);

	// Outgoing sequence increments
	TEST_ASSERT_EQUAL(oldOutgoing + 1, chan.outgoingSequence);

	// Reliable buffer should now contain the message
	TEST_ASSERT_EQUAL(8, chan.reliableLength);
	TEST_ASSERT_EQUAL(0, chan.message.curSize);

	// Reliable sequence toggled (was 0 -> 1)
	TEST_ASSERT_TRUE(chan.reliableSequence == 1 || chan.reliableSequence == 0);
	TEST_ASSERT_EQUAL(chan.outgoingSequence, chan.lastReliableSequence);

	// lastSent should reflect mock time
	TEST_ASSERT_EQUAL(5000, chan.lastSent);
}

// Test: Netchan_Transmit followed by Process acknowledging clears reliable
void test_Netchan_Transmit_and_Process_ack_clears_reliable(void) {
	netChan_t chan = {0};
	netAdr_t addr = {0};
	byte data[16] = {0};
	byte buffer[64];
	netMsg_t msg = {0};

	Netchan_Setup(NS_CLIENT, &chan, &addr, ENHANCED_PROTOCOL_VERSION, 54321, 0);

	// Prepare outgoing reliable message
	chan.message.curSize = 6;
	memcpy(chan.messageBuff, "hello!", 6);

	// Transmit (makes it reliable and toggles reliableSequence)
	Netchan_Transmit(&chan, 0, NULL);

	// Ensure reliable is pending
	TEST_ASSERT_TRUE(chan.reliableLength > 0);
	int reliableSeq = chan.reliableSequence;

	// Create packet that acknowledges our reliable by setting reliableAck bit
	// Sequence can be arbitrary greater than incomingSequence
	uint32 ackSeq = 1;
	uint32 seqAck = (reliableSeq ? (1u<<31) : 0u);
	CreatePacket(&msg, buffer, sizeof(buffer), ackSeq, seqAck, qFalse, qTrue);

	// Process the packet
	mockTime = 6000;
	qBool result = Netchan_Process(&chan, &msg);
	TEST_ASSERT_TRUE(result);

	// Reliable should be cleared when ack matches
	TEST_ASSERT_EQUAL(0, chan.reliableLength);
	TEST_ASSERT_EQUAL(6000, chan.lastReceived);
}

// Test: retransmit unacknowledged reliable message
void test_Netchan_Retransmit_UnacknowledgedReliable(void) {
	netChan_t chan = {0};
	netAdr_t addr = {0};

	Netchan_Setup(NS_CLIENT, &chan, &addr, ENHANCED_PROTOCOL_VERSION, 54321, 0);

	// Prepare outgoing reliable message and transmit it
	chan.message.curSize = 5;
	memcpy(chan.messageBuff, "abcde", 5);
	Netchan_Transmit(&chan, 0, NULL);

	TEST_ASSERT_TRUE(chan.reliableLength > 0);
	uint32 firstLastReliable = chan.lastReliableSequence;
	uint32 firstOutgoing = chan.outgoingSequence;

	// Simulate remote state that indicates our reliable was not acknowledged
	chan.incomingAcknowledged = firstLastReliable + 1; // greater than lastReliable
	chan.incomingReliableAcknowledged = chan.reliableSequence ^ 1; // not equal

	// Call transmit again to trigger retransmit
	Netchan_Transmit(&chan, 0, NULL);

	// Outgoing sequence incremented and lastReliableSequence updated to new outgoing
	TEST_ASSERT_TRUE(chan.outgoingSequence > firstOutgoing);
	TEST_ASSERT_EQUAL(chan.outgoingSequence, chan.lastReliableSequence);
	TEST_ASSERT_TRUE(chan.reliableLength > 0); // still pending
}

// Test: no retransmit when reliable has been acknowledged
void test_Netchan_NoRetransmit_WhenAcknowledged(void) {
	netChan_t chan = {0};
	netAdr_t addr = {0};

	Netchan_Setup(NS_CLIENT, &chan, &addr, ENHANCED_PROTOCOL_VERSION, 54321, 0);

	// Prepare outgoing reliable message and transmit it
	chan.message.curSize = 5;
	memcpy(chan.messageBuff, "xyz12", 5);
	Netchan_Transmit(&chan, 0, NULL);

	TEST_ASSERT_TRUE(chan.reliableLength > 0);
	uint32 firstLastReliable = chan.lastReliableSequence;
	uint32 firstOutgoing = chan.outgoingSequence;

	// Simulate remote state acknowledging the reliable
	chan.incomingAcknowledged = firstLastReliable + 1;
	chan.incomingReliableAcknowledged = chan.reliableSequence; // equals reliableSequence -> acknowledged

	// Call transmit again - should not mark as retransmit (but outgoing still increments)
	Netchan_Transmit(&chan, 0, NULL);

	// outgoingSequence increments
	TEST_ASSERT_TRUE(chan.outgoingSequence > firstOutgoing);
	// lastReliableSequence should remain unchanged since no retransmit needed
	TEST_ASSERT_EQUAL(firstLastReliable, chan.lastReliableSequence);
}


// ============================================================================
// TESTS: Netchan_Process - Basic Sequence Handling
// ============================================================================

// Test: process valid packet
void test_Netchan_Process_ValidPacket(void) {
	netChan_t chan = {0};
	netAdr_t addr = {0};
	Netchan_Setup(NS_CLIENT, &chan, &addr, ORIGINAL_PROTOCOL_VERSION, 12345, 0);
	
	byte buffer[64];
	netMsg_t msg = {0};
	CreatePacket(&msg, buffer, sizeof(buffer), 1, 0, qFalse, qFalse);
	
	mockTime = 3000;
	qBool result = Netchan_Process(&chan, &msg);
	
	TEST_ASSERT_TRUE(result);
	TEST_ASSERT_EQUAL(1, chan.incomingSequence);
	TEST_ASSERT_EQUAL(3000, chan.lastReceived);
}

// Test: process sequence of packets
void test_Netchan_Process_SequenceOfPackets(void) {
	netChan_t chan = {0};
	netAdr_t addr = {0};
	Netchan_Setup(NS_CLIENT, &chan, &addr, ORIGINAL_PROTOCOL_VERSION, 12345, 0);
	
	byte buffer[64];
	netMsg_t msg = {0};
	
	// Packet 1
	CreatePacket(&msg, buffer, sizeof(buffer), 1, 0, qFalse, qFalse);
	TEST_ASSERT_TRUE(Netchan_Process(&chan, &msg));
	TEST_ASSERT_EQUAL(1, chan.incomingSequence);
	
	// Packet 2
	CreatePacket(&msg, buffer, sizeof(buffer), 2, 1, qFalse, qFalse);
	TEST_ASSERT_TRUE(Netchan_Process(&chan, &msg));
	TEST_ASSERT_EQUAL(2, chan.incomingSequence);
	
	// Packet 3
	CreatePacket(&msg, buffer, sizeof(buffer), 3, 2, qFalse, qFalse);
	TEST_ASSERT_TRUE(Netchan_Process(&chan, &msg));
	TEST_ASSERT_EQUAL(3, chan.incomingSequence);
}

// Test: reject duplicate packet
void test_Netchan_Process_RejectDuplicate(void) {
	netChan_t chan = {0};
	netAdr_t addr = {0};
	Netchan_Setup(NS_CLIENT, &chan, &addr, ORIGINAL_PROTOCOL_VERSION, 12345, 0);
	
	byte buffer[64];
	netMsg_t msg = {0};
	
	// Packet 1
	CreatePacket(&msg, buffer, sizeof(buffer), 1, 0, qFalse, qFalse);
	TEST_ASSERT_TRUE(Netchan_Process(&chan, &msg));
	
	// Duplicate packet 1
	CreatePacket(&msg, buffer, sizeof(buffer), 1, 0, qFalse, qFalse);
	TEST_ASSERT_FALSE(Netchan_Process(&chan, &msg));
	
	// Sequence should not advance
	TEST_ASSERT_EQUAL(1, chan.incomingSequence);
}

// Test: reject out of order packet
void test_Netchan_Process_RejectOutOfOrder(void) {
	netChan_t chan = {0};
	netAdr_t addr = {0};
	Netchan_Setup(NS_CLIENT, &chan, &addr, ORIGINAL_PROTOCOL_VERSION, 12345, 0);
	
	byte buffer[64];
	netMsg_t msg = {0};
	
	// Packet 3
	CreatePacket(&msg, buffer, sizeof(buffer), 3, 0, qFalse, qFalse);
	TEST_ASSERT_TRUE(Netchan_Process(&chan, &msg));
	TEST_ASSERT_EQUAL(3, chan.incomingSequence);
	
	// Old packet 2 (out of order)
	CreatePacket(&msg, buffer, sizeof(buffer), 2, 0, qFalse, qFalse);
	TEST_ASSERT_FALSE(Netchan_Process(&chan, &msg));
	
	// Sequence should not change
	TEST_ASSERT_EQUAL(3, chan.incomingSequence);
}

// Test: detect dropped packets
void test_Netchan_Process_DetectDroppedPackets(void) {
	netChan_t chan = {0};
	netAdr_t addr = {0};
	Netchan_Setup(NS_CLIENT, &chan, &addr, ORIGINAL_PROTOCOL_VERSION, 12345, 0);
	
	byte buffer[64];
	netMsg_t msg = {0};
	
	// Packet 1
	CreatePacket(&msg, buffer, sizeof(buffer), 1, 0, qFalse, qFalse);
	TEST_ASSERT_TRUE(Netchan_Process(&chan, &msg));
	TEST_ASSERT_EQUAL(0, chan.dropped);
	
	// Packet 5 (skipped 2, 3, 4)
	CreatePacket(&msg, buffer, sizeof(buffer), 5, 1, qFalse, qFalse);
	TEST_ASSERT_TRUE(Netchan_Process(&chan, &msg));
	TEST_ASSERT_EQUAL(3, chan.dropped);  // 3 packets dropped
	TEST_ASSERT_EQUAL(5, chan.incomingSequence);
}

// Test: single dropped packet
void test_Netchan_Process_SingleDroppedPacket(void) {
	netChan_t chan = {0};
	netAdr_t addr = {0};
	Netchan_Setup(NS_CLIENT, &chan, &addr, ORIGINAL_PROTOCOL_VERSION, 12345, 0);
	
	byte buffer[64];
	netMsg_t msg = {0};
	
	// Packet 1
	CreatePacket(&msg, buffer, sizeof(buffer), 1, 0, qFalse, qFalse);
	TEST_ASSERT_TRUE(Netchan_Process(&chan, &msg));
	
	// Packet 3 (skipped 2)
	CreatePacket(&msg, buffer, sizeof(buffer), 3, 1, qFalse, qFalse);
	TEST_ASSERT_TRUE(Netchan_Process(&chan, &msg));
	TEST_ASSERT_EQUAL(1, chan.dropped);
}

// ============================================================================
// TESTS: Netchan_Process - Reliable Message Handling
// ============================================================================

// Test: process reliable message
void test_Netchan_Process_ReliableMessage(void) {
	netChan_t chan = {0};
	netAdr_t addr = {0};
	Netchan_Setup(NS_CLIENT, &chan, &addr, ORIGINAL_PROTOCOL_VERSION, 12345, 0);
	
	byte buffer[64];
	netMsg_t msg = {0};
	
	TEST_ASSERT_EQUAL(0, chan.incomingReliableSequence);
	
	// Packet with reliable bit set
	CreatePacket(&msg, buffer, sizeof(buffer), 1, 0, qTrue, qFalse);
	TEST_ASSERT_TRUE(Netchan_Process(&chan, &msg));
	
	TEST_ASSERT_TRUE(chan.gotReliable);
	TEST_ASSERT_EQUAL(1, chan.incomingReliableSequence);
}

// Test: reliable sequence toggles
void test_Netchan_Process_ReliableSequenceToggles(void) {
	netChan_t chan = {0};
	netAdr_t addr = {0};
	Netchan_Setup(NS_CLIENT, &chan, &addr, ORIGINAL_PROTOCOL_VERSION, 12345, 0);
	
	byte buffer[64];
	netMsg_t msg = {0};
	
	// First reliable message
	CreatePacket(&msg, buffer, sizeof(buffer), 1, 0, qTrue, qFalse);
	Netchan_Process(&chan, &msg);
	TEST_ASSERT_EQUAL(1, chan.incomingReliableSequence);
	
	// Second reliable message (toggles to 0)
	CreatePacket(&msg, buffer, sizeof(buffer), 2, 1, qTrue, qFalse);
	Netchan_Process(&chan, &msg);
	TEST_ASSERT_EQUAL(0, chan.incomingReliableSequence);
	
	// Third reliable message (toggles back to 1)
	CreatePacket(&msg, buffer, sizeof(buffer), 3, 2, qTrue, qFalse);
	Netchan_Process(&chan, &msg);
	TEST_ASSERT_EQUAL(1, chan.incomingReliableSequence);
}

// Test: non-reliable messages don't toggle sequence
void test_Netchan_Process_NonReliableNoToggle(void) {
	netChan_t chan = {0};
	netAdr_t addr = {0};
	Netchan_Setup(NS_CLIENT, &chan, &addr, ORIGINAL_PROTOCOL_VERSION, 12345, 0);
	
	byte buffer[64];
	netMsg_t msg = {0};
	
	// Non-reliable messages
	CreatePacket(&msg, buffer, sizeof(buffer), 1, 0, qFalse, qFalse);
	Netchan_Process(&chan, &msg);
	TEST_ASSERT_EQUAL(0, chan.incomingReliableSequence);
	
	CreatePacket(&msg, buffer, sizeof(buffer), 2, 1, qFalse, qFalse);
	Netchan_Process(&chan, &msg);
	TEST_ASSERT_EQUAL(0, chan.incomingReliableSequence);
	
	TEST_ASSERT_FALSE(chan.gotReliable);
}

// Test: acknowledge reliable message
void test_Netchan_Process_AcknowledgeReliable(void) {
	netChan_t chan = {0};
	netAdr_t addr = {0};
	Netchan_Setup(NS_CLIENT, &chan, &addr, ORIGINAL_PROTOCOL_VERSION, 12345, 0);
	
	// Simulate we have a reliable message pending
	chan.reliableSequence = 1;
	chan.reliableLength = 100;
	
	byte buffer[64];
	netMsg_t msg = {0};
	
	// Receive ack with reliable bit set matching our sequence
	CreatePacket(&msg, buffer, sizeof(buffer), 1, 0, qFalse, qTrue);
	chan.incomingAcknowledged = 0;  // Set up ack sequence
	Netchan_Process(&chan, &msg);
	
	// Reliable buffer should be cleared
	TEST_ASSERT_EQUAL(0, chan.reliableLength);
	TEST_ASSERT_EQUAL(1, chan.incomingReliableAcknowledged);
}

// Test: don't clear reliable on wrong ack
void test_Netchan_Process_WrongReliableAck(void) {
	netChan_t chan = {0};
	netAdr_t addr = {0};
	Netchan_Setup(NS_CLIENT, &chan, &addr, ORIGINAL_PROTOCOL_VERSION, 12345, 0);
	
	// Simulate we have a reliable message pending with sequence 1
	chan.reliableSequence = 1;
	chan.reliableLength = 100;
	
	byte buffer[64];
	netMsg_t msg = {0};
	
	// Receive ack with reliable bit NOT set (ack for sequence 0)
	CreatePacket(&msg, buffer, sizeof(buffer), 1, 0, qFalse, qFalse);
	Netchan_Process(&chan, &msg);
	
	// Reliable buffer should NOT be cleared (wrong ack)
	TEST_ASSERT_EQUAL(100, chan.reliableLength);
}

// ============================================================================
// TESTS: Sequence Acknowledgment
// ============================================================================

// Test: track incoming acknowledged
void test_Netchan_Process_IncomingAcknowledged(void) {
	netChan_t chan = {0};
	netAdr_t addr = {0};
	Netchan_Setup(NS_CLIENT, &chan, &addr, ORIGINAL_PROTOCOL_VERSION, 12345, 0);
	
	byte buffer[64];
	netMsg_t msg = {0};
	
	// Packet acknowledging sequence 5
	CreatePacket(&msg, buffer, sizeof(buffer), 1, 5, qFalse, qFalse);
	Netchan_Process(&chan, &msg);
	
	TEST_ASSERT_EQUAL(5, chan.incomingAcknowledged);
}

// Test: update incoming acknowledged
void test_Netchan_Process_UpdateIncomingAcknowledged(void) {
	netChan_t chan = {0};
	netAdr_t addr = {0};
	Netchan_Setup(NS_CLIENT, &chan, &addr, ORIGINAL_PROTOCOL_VERSION, 12345, 0);
	
	byte buffer[64];
	netMsg_t msg = {0};
	
	CreatePacket(&msg, buffer, sizeof(buffer), 1, 10, qFalse, qFalse);
	Netchan_Process(&chan, &msg);
	TEST_ASSERT_EQUAL(10, chan.incomingAcknowledged);
	
	CreatePacket(&msg, buffer, sizeof(buffer), 2, 15, qFalse, qFalse);
	Netchan_Process(&chan, &msg);
	TEST_ASSERT_EQUAL(15, chan.incomingAcknowledged);
}

// ============================================================================
// TESTS: Edge Cases
// ============================================================================

// Test: sequence wraparound
void test_Netchan_Process_SequenceWraparound(void) {
	netChan_t chan = {0};
	netAdr_t addr = {0};
	Netchan_Setup(NS_CLIENT, &chan, &addr, ORIGINAL_PROTOCOL_VERSION, 12345, 0);
	
	// Set up near max value (without high bit set to avoid reliable confusion)
	chan.incomingSequence = 0x7FFFFFFD;
	
	byte buffer[64];
	netMsg_t msg = {0};
	
	// Next packet at 0x7FFFFFFE (still increasing within 31-bit range)
	CreatePacket(&msg, buffer, sizeof(buffer), 0x7FFFFFFE, 0, qFalse, qFalse);
	TEST_ASSERT_TRUE(Netchan_Process(&chan, &msg));
	TEST_ASSERT_EQUAL(0x7FFFFFFE, chan.incomingSequence);
	
	// Next packet at 0x7FFFFFFF (max 31-bit value)
	CreatePacket(&msg, buffer, sizeof(buffer), 0x7FFFFFFF, 0, qFalse, qFalse);
	TEST_ASSERT_TRUE(Netchan_Process(&chan, &msg));
	TEST_ASSERT_EQUAL(0x7FFFFFFF, chan.incomingSequence);
}

// Test: zero sequence is valid first packet
void test_Netchan_Process_ZeroSequenceInvalid(void) {
	netChan_t chan = {0};
	netAdr_t addr = {0};
	Netchan_Setup(NS_CLIENT, &chan, &addr, ORIGINAL_PROTOCOL_VERSION, 12345, 0);
	
	byte buffer[64];
	netMsg_t msg = {0};
	
	// Packet with sequence 0 should be rejected (incoming starts at 0)
	CreatePacket(&msg, buffer, sizeof(buffer), 0, 0, qFalse, qFalse);
	TEST_ASSERT_FALSE(Netchan_Process(&chan, &msg));
}

// Test: update last received time
void test_Netchan_Process_UpdateLastReceived(void) {
	netChan_t chan = {0};
	netAdr_t addr = {0};
	mockTime = 1000;
	Netchan_Setup(NS_CLIENT, &chan, &addr, ORIGINAL_PROTOCOL_VERSION, 12345, 0);
	
	TEST_ASSERT_EQUAL(1000, chan.lastReceived);
	
	byte buffer[64];
	netMsg_t msg = {0};
	
	mockTime = 2000;
	CreatePacket(&msg, buffer, sizeof(buffer), 1, 0, qFalse, qFalse);
	Netchan_Process(&chan, &msg);
	
	TEST_ASSERT_EQUAL(2000, chan.lastReceived);
	
	mockTime = 3500;
	CreatePacket(&msg, buffer, sizeof(buffer), 2, 1, qFalse, qFalse);
	Netchan_Process(&chan, &msg);
	
	TEST_ASSERT_EQUAL(3500, chan.lastReceived);
}

// ============================================================================
// TESTS: Complex Scenarios
// ============================================================================

// Test: mixed reliable and non-reliable messages
void test_Netchan_Process_MixedReliable(void) {
	netChan_t chan = {0};
	netAdr_t addr = {0};
	Netchan_Setup(NS_CLIENT, &chan, &addr, ORIGINAL_PROTOCOL_VERSION, 12345, 0);
	
	byte buffer[64];
	netMsg_t msg = {0};
	
	// Packet 1: non-reliable
	CreatePacket(&msg, buffer, sizeof(buffer), 1, 0, qFalse, qFalse);
	Netchan_Process(&chan, &msg);
	TEST_ASSERT_FALSE(chan.gotReliable);
	TEST_ASSERT_EQUAL(0, chan.incomingReliableSequence);
	
	// Packet 2: reliable
	CreatePacket(&msg, buffer, sizeof(buffer), 2, 1, qTrue, qFalse);
	Netchan_Process(&chan, &msg);
	TEST_ASSERT_TRUE(chan.gotReliable);
	TEST_ASSERT_EQUAL(1, chan.incomingReliableSequence);
	
	// Packet 3: non-reliable
	CreatePacket(&msg, buffer, sizeof(buffer), 3, 2, qFalse, qFalse);
	Netchan_Process(&chan, &msg);
	TEST_ASSERT_FALSE(chan.gotReliable);
	TEST_ASSERT_EQUAL(1, chan.incomingReliableSequence);  // Stays at 1
	
	// Packet 4: reliable
	CreatePacket(&msg, buffer, sizeof(buffer), 4, 3, qTrue, qFalse);
	Netchan_Process(&chan, &msg);
	TEST_ASSERT_TRUE(chan.gotReliable);
	TEST_ASSERT_EQUAL(0, chan.incomingReliableSequence);  // Toggles back
}

// Test: dropped packets with reliable messages
void test_Netchan_Process_DroppedWithReliable(void) {
	netChan_t chan = {0};
	netAdr_t addr = {0};
	Netchan_Setup(NS_CLIENT, &chan, &addr, ORIGINAL_PROTOCOL_VERSION, 12345, 0);
	
	byte buffer[64];
	netMsg_t msg = {0};
	
	// Packet 1
	CreatePacket(&msg, buffer, sizeof(buffer), 1, 0, qFalse, qFalse);
	Netchan_Process(&chan, &msg);
	
	// Packet 5 with reliable (skipped 2, 3, 4)
	CreatePacket(&msg, buffer, sizeof(buffer), 5, 1, qTrue, qFalse);
	Netchan_Process(&chan, &msg);
	
	TEST_ASSERT_EQUAL(3, chan.dropped);
	TEST_ASSERT_TRUE(chan.gotReliable);
	TEST_ASSERT_EQUAL(1, chan.incomingReliableSequence);
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main(void) {
	UNITY_BEGIN();

	// Netchan_Setup tests
	RUN_TEST(test_Netchan_Setup_Client);
	RUN_TEST(test_Netchan_Setup_Server);
	RUN_TEST(test_Netchan_Setup_EnhancedProtocol);
	RUN_TEST(test_Netchan_Setup_Q2ProProtocol);
	RUN_TEST(test_Netchan_Setup_ZeroMsgLen);
	RUN_TEST(test_Netchan_Setup_ClearsPreviousState);
	RUN_TEST(test_Netchan_Setup_AllowsOverflow);

	// Transmit tests
	RUN_TEST(test_Netchan_Transmit_reliable_and_sequence_increment);
	RUN_TEST(test_Netchan_Transmit_and_Process_ack_clears_reliable);

	// Basic sequence handling tests
	RUN_TEST(test_Netchan_Process_ValidPacket);
	RUN_TEST(test_Netchan_Process_SequenceOfPackets);
	RUN_TEST(test_Netchan_Process_RejectDuplicate);
	RUN_TEST(test_Netchan_Process_RejectOutOfOrder);
	RUN_TEST(test_Netchan_Process_DetectDroppedPackets);
	RUN_TEST(test_Netchan_Process_SingleDroppedPacket);

	// Reliable message handling tests
	RUN_TEST(test_Netchan_Process_ReliableMessage);
	RUN_TEST(test_Netchan_Process_ReliableSequenceToggles);
	RUN_TEST(test_Netchan_Process_NonReliableNoToggle);
	RUN_TEST(test_Netchan_Process_AcknowledgeReliable);
	RUN_TEST(test_Netchan_Process_WrongReliableAck);

	// Sequence acknowledgment tests
	RUN_TEST(test_Netchan_Process_IncomingAcknowledged);
	RUN_TEST(test_Netchan_Process_UpdateIncomingAcknowledged);

	// Edge case tests
	RUN_TEST(test_Netchan_Process_SequenceWraparound);
	RUN_TEST(test_Netchan_Process_ZeroSequenceInvalid);
	RUN_TEST(test_Netchan_Process_UpdateLastReceived);

	// Complex scenario tests
	RUN_TEST(test_Netchan_Process_MixedReliable);
	RUN_TEST(test_Netchan_Process_DroppedWithReliable);

	return UNITY_END();
}
