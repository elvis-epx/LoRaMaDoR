#include <Arduino.h>
#include <stdlib.h>
#include <Preferences.h>
#include "Buffer.h"
#include "Callsign.h"

extern void platform_print(const char*);

Preferences prefs;

unsigned long int arduino_millis()
{
	return millis();
}

long int arduino_random(long int min, long int max)
{
	return random(min, max);
}

unsigned int arduino_nvram_id_load()
{
	prefs.begin("LoRaMaDoR");
	unsigned int id = prefs.getUInt("lastid");
	prefs.end();

	if (id <= 0 || id > 9999) {
		id = 1;
	}

	return id;
}

void arduino_nvram_id_save(unsigned int id)
{
	prefs.begin("LoRaMaDoR", false);
	prefs.putUInt("lastid", id);
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
	prefs.putString("callsign", new_callsign.buf().cold());
	prefs.end();
}

void arduino_nvram_save(const char *key, const Buffer& value)
{
	prefs.begin("LoRaMaDoR", false);
	prefs.putString(key, value.cold());
	prefs.end();
}

Buffer arduino_nvram_load(const char *key)
{
  prefs.begin("LoRaMaDoR", false);
	String svalue = prefs.getString(key, String("None"));
  prefs.end();
	return Buffer(svalue.c_str());
}

void console_print(const char *msg) {
	platform_print(msg);
}

void console_print(char c) {
	char msg[] = {c, 0};
	platform_print(msg);
}

void console_println(const char *msg) {
	platform_print(msg);
	platform_print("\r\n");
}

void console_println() {
	platform_print("\r\n");
}

void arduino_restart() {
	ESP.restart();
}
