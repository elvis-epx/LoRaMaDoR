// LoRaMaDoR (LoRa-based mesh network for hams) project
// Copyright (c) 2020 PU5EPX
// Abstract class for application protocols

#ifndef __PROTOCOL_H
#define __PROTOCOL_H

#include "Pointer.h"

class Network;
class Packet;

class Protocol {
public:
	Protocol(Network*);
	virtual Ptr<Packet> handle(const Packet&);
	virtual Ptr<Packet> modify(const Packet&);
	virtual ~Protocol() = 0;
protected:
	Network *net;
	// This class must be new()ed and not fooled around
	Protocol(const Protocol&) = delete;
	Protocol(const Protocol&&) = delete;
	Protocol& operator=(const Protocol&) = delete;
	Protocol& operator=(const Protocol&&) = delete;
};

#endif
