/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2020 PU5EPX
 */

#include "HMACKeys.h"
#include "NVRAM.h"

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
