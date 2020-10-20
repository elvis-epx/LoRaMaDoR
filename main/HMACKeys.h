/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

// Implementation of packet confirmation (C,CO options)

#ifndef __HMACKEYS_H
#define __HMACKEYS_H

#include "Buffer.h"
#include "Callsign.h"

class HMACKeys {
public:
	static Buffer get_key_for(const Callsign &c);
};

#endif
