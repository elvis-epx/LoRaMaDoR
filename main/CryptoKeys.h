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
	static int decrypt(const char *, const size_t, char **, size_t *);
	/* public because of unit testing */
	static void _encrypt(const Buffer&, Buffer&);
	static int _decrypt(const Buffer&, const char *, const size_t, char **, size_t *);

	static const int ERR_NOT_ENCRYPTED = 1;
	static const int ERR_ENCRYPTED = 2;
	static const int ERR_DECRIPTION = 3;
	static const int OK_DECRYPTED = 4;
	static const int OK_CLEARTEXT = 5;
};

#endif
