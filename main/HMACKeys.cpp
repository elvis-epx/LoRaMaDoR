/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2020 PU5EPX
 */

// Implementation of packet confirmation (C,CO options)

#include "HMACKeys.h"
#include "NVRAM.h"

Buffer HMACKeys::get_key_for(const Callsign &c)
{
	// TODO allow to store keys per-prefix (with or without SSID, etc.)
	return arduino_nvram_psk_load();
}
