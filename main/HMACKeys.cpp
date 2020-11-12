/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2020 PU5EPX
 */

#include "HMACKeys.h"
#include "NVRAM.h"
#include "sha256.h"

static bool valid = false;
static Buffer psk;

// TODO allow to store keys per-prefix (with or without SSID, etc.)
Buffer HMACKeys::get_key_for(const Callsign &c)
{
	if (!valid) {
		psk = arduino_nvram_hmac_psk_load();
		valid = true;
	}
	return psk;
}

void HMACKeys::invalidate()
{
	valid = false;
}

static const char* hex = "0123456789abcdef";

Buffer HMACKeys::hmac(const Buffer& key, const Buffer& data)
{
	Sha256 hmac;
	hmac.initHmac((uint8_t*) key.c_str(), key.length());
	for (size_t i = 0; i < data.length(); ++i) {
		hmac.write((uint8_t) data.charAt(i));
	}
	uint8_t* res = hmac.resultHmac();
	// convert the first 48 bits of HMAC (6 octets) to hex
	char b64[13];
	for (size_t i = 0; i < 6; ++i) {
		b64[i*2+0] = hex[(res[i] >> 4) & 0xf];
		b64[i*2+1] = hex[res[i] & 0xf];
	}

	return Buffer(b64, 12);
}

Buffer HMACKeys::hash_key(const Buffer& key)
{
	Sha256 hash;
	hash.init();
	hash.write(1);
	for (size_t i = 0; i < key.length(); ++i) {
		hash.write((uint8_t) key.charAt(i));
	}
	uint8_t* res = hash.result();
	char b64[32];
	for (size_t i = 0; i < 16; ++i) {
		b64[i*2+0] = hex[(res[i] >> 4) & 0xf];
		b64[i*2+1] = hex[res[i] & 0xf];
	}
	return Buffer(b64, 32);
}
