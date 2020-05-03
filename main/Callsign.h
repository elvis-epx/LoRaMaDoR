/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

// Class that encapsulates a callsign

#ifndef __CALLSIGN_H
#define __CALLSIGN_H

#include "Buffer.h"
#include <cstdarg>

class Callsign
{
public:
	Callsign();
	Callsign(Buffer);
	operator Buffer() const;
	bool is_valid() const;
	bool is_bcast() const;
	bool is_q() const;
	bool is_lo() const;
	bool is_reserved() const;
	bool operator==(Buffer) const;
	bool operator==(const Callsign&) const;
private:
	static bool check(const Buffer&);
	Buffer name;
	bool valid;
};

#endif
