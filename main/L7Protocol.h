/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

// Abstract class for application protocols

#ifndef __L7PROTOCOL_H
#define __L7PROTOCOL_H

#include <cstddef>
#include <cstdint>
#include "Pointer.h"
#include "Callsign.h"
#include "Buffer.h"
#include "Params.h"

class Network;
class Packet;

struct L7HandlerResponse {
	L7HandlerResponse();
	L7HandlerResponse(bool, const Callsign &, const Params&, const Buffer&);

	bool has_packet;
	Callsign to;
	Params params;
	Buffer msg;
};

class L7Protocol {
public:
	L7Protocol(Network*);
	virtual L7HandlerResponse handle(const Packet&);
	virtual ~L7Protocol();
protected:
	Network *net;
	// This class must be new()ed and not fooled around
	L7Protocol() = delete;
	L7Protocol(const L7Protocol&) = delete;
	L7Protocol(L7Protocol&&) = delete;
	L7Protocol& operator=(const L7Protocol&) = delete;
	L7Protocol& operator=(L7Protocol&&) = delete;
};

#endif
