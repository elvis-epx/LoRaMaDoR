/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

#ifndef __CRYPTOKEYS_H
#define __CRYPTOKEYS_H

#include "Buffer.h"
#include "Callsign.h"

class CryptoKeys {
public:
	static Buffer get_key();
	static void encrypt(Buffer&);
	static bool decrypt(const char *, const size_t, char **, size_t *);
	/* public because of unit testing */
	static void _encrypt(const Buffer&, Buffer&);
	static bool _decrypt(const Buffer&, const char *, const size_t, char **, size_t *);
};

#endif
