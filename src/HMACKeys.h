/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

#ifndef __HMACKEYS_H
#define __HMACKEYS_H

#include "Buffer.h"
#include "Callsign.h"

class HMACKeys {
public:
	static Buffer get_key_for(const Callsign &c);
	static Buffer hmac(const Buffer& key, const Buffer& data);
	static void invalidate();
	static Buffer hash_key(const Buffer& key);
};

#endif
