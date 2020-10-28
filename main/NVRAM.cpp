/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2020 PU5EPX
 */

#include <stdlib.h>
#ifdef UNDER_TEST
#include "Preferences.h"
#else
#include <Preferences.h>
#endif
#include "Buffer.h"
#include "Callsign.h"
#include "NVRAM.h"

extern Preferences prefs;

uint32_t arduino_nvram_id_load()
{
	prefs.begin("LoRaMaDoR");
	uint32_t id = prefs.getUInt("lastid");
	prefs.end();

	if (id <= 0 || id > 9999) {
		id = 1;
	}

	return id;
}

void arduino_nvram_id_save(uint32_t id)
{
	prefs.begin("LoRaMaDoR", false);
	prefs.putUInt("lastid", id);
	prefs.end();

}

uint32_t arduino_nvram_repeater_load()
{
	prefs.begin("LoRaMaDoR");
	uint32_t r = prefs.getUInt("repeater");
	prefs.end();

	return r;
}

void arduino_nvram_repeater_save(uint32_t r)
{
	prefs.begin("LoRaMaDoR", false);
	prefs.putUInt("repeater", r);
	prefs.end();
}

uint32_t arduino_nvram_beacon_load()
{
	prefs.begin("LoRaMaDoR");
	uint32_t b = prefs.getUInt("beacon");
	prefs.end();

	if (b < 10 || b > 600) {
		b = 600;
	}

	return b;
}

void arduino_nvram_beacon_save(uint32_t b)
{
	if (b < 10 || b > 600) {
		b = 600;
	}

	prefs.begin("LoRaMaDoR", false);
	prefs.putUInt("beacon", b);
	prefs.end();
}

uint32_t arduino_nvram_beacon_first_load()
{
	prefs.begin("LoRaMaDoR");
	uint32_t b = prefs.getUInt("beacon1");
	prefs.end();

	if (b < 1 || b > 300) {
		b = 30;
	}

	return b;
}

void arduino_nvram_beacon_first_save(uint32_t b)
{
	if (b < 1 || b > 300) {
		b = 30;
	}

	prefs.begin("LoRaMaDoR", false);
	prefs.putUInt("beacon1", b);
	prefs.end();
}

Callsign arduino_nvram_callsign_load()
{
	char candidate[12];
	prefs.begin("LoRaMaDoR");
	// len includes \0
	size_t len = prefs.getString("callsign", candidate, 11);
	prefs.end();

	Callsign cs;

	if (len <= 1) {
		cs = Callsign("FIXMEE-1");
	} else {
		cs = Callsign(Buffer(candidate, len - 1));
		if (!cs.is_valid()) {
			cs = Callsign("FIXMEE-2");
		}
	}

	return cs;
}

void arduino_nvram_callsign_save(const Callsign &new_callsign)
{
	prefs.begin("LoRaMaDoR", false);
	prefs.putString("callsign", Buffer(new_callsign).c_str());
	prefs.end();
}

Buffer arduino_nvram_psk_load()
{
	char candidate[33];
	prefs.begin("LoRaMaDoR");
	// len includes \0
	size_t len = prefs.getString("psk", candidate, 33);
	prefs.end();

	if (len <= 1) {
		return "";
	}
	return Buffer(candidate, len - 1);
}

void arduino_nvram_psk_save(const Buffer &b)
{
	prefs.begin("LoRaMaDoR", false);
	prefs.putString("psk", b.c_str());
	prefs.end();
}

// used by Wi-Fi SSID and password
void arduino_nvram_save(const char *key, const Buffer& value)
{
	prefs.begin("LoRaMaDoR", false);
	prefs.putString(key, value.c_str());
	prefs.end();
}

// used by Wi-Fi SSID and password
Buffer arduino_nvram_load(const char *key)
{
	char candidate[65];
	prefs.begin("LoRaMaDoR", false);
	// len includes \0
	size_t len = prefs.getString(key, candidate, 65);
	prefs.end();

	if (len <= 1) {
		return "None";
	}
	return Buffer(candidate, len - 1);
}
