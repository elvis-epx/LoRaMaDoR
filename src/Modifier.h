/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

// Abstract class for application protocols that modify a forwarded packet

#ifndef __MODIFIER_H
#define __MODIFIER_H

#include <cstddef>
#include <cstdint>
#include "Pointer.h"

class Network;
class Packet;

class Modifier {
public:
	Modifier(Network*);
	virtual Ptr<Packet> modify(const Packet&) = 0;
	virtual ~Modifier();
protected:
	Network *net;
	// This class must be new()ed and not fooled around
	Modifier() = delete;
	Modifier(const Modifier&) = delete;
	Modifier(Modifier&&) = delete;
	Modifier& operator=(const Modifier&) = delete;
	Modifier& operator=(Modifier&&) = delete;
};

#endif
