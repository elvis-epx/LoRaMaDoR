/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

// Abstract class for transport protocols

#ifndef __L4PROTOCOL_H
#define __L4PROTOCOL_H

#include <cstddef>
#include <cstdint>
#include "Pointer.h"
#include "Buffer.h"
#include "Callsign.h"
#include "Params.h"

class Network;
class Packet;

struct L4rxHandlerResponse {
	L4rxHandlerResponse();
	L4rxHandlerResponse(bool, const Callsign &, const Params&, const Buffer&, bool, const Buffer&);
	bool has_packet;
	Callsign to;
	Params params;
	Buffer msg;
	bool error;
	Buffer error_msg;
};

struct L4txHandlerResponse {
	L4txHandlerResponse();
	L4txHandlerResponse(Ptr<Packet>);
	Ptr<Packet> pkt;
};

class L4Protocol {
public:
	L4Protocol(Network*);
	virtual L4rxHandlerResponse rx(const Packet&) = 0;
	virtual L4txHandlerResponse tx(const Packet&) = 0;
	virtual ~L4Protocol();
protected:
	Network *net;
	// This class must be new()ed and not fooled around
	L4Protocol() = delete;
	L4Protocol(const L4Protocol&) = delete;
	L4Protocol(L4Protocol&&) = delete;
	L4Protocol& operator=(const L4Protocol&) = delete;
	L4Protocol& operator=(L4Protocol&&) = delete;
};

#endif
