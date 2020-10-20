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

class Network;
class Packet;

struct L4HandlerResponse {
	L4HandlerResponse();
	L4HandlerResponse(Ptr<Packet>, bool);
	Ptr<Packet> pkt;
	bool error;
};

class L4Protocol {
public:
	L4Protocol(Network*);
	virtual L4HandlerResponse handle(const Packet&) = 0;
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
