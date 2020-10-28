/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

// Main LoRaMaDoR network class, plus some auxiliary types

#include <Preferences.h>
#include "Packet.h"
#include "Network.h"
#include "Display.h"
#include "ArduinoBridge.h"
#include "NVRAM.h"
#include "Console.h"
#include "TCPIP.h"

Ptr<Network> Net;
Preferences prefs;

void setup()
{
	Serial.begin(115200);
	oled_init();
	oled_show("Starting...", "", "", "");

	Callsign cs = arduino_nvram_callsign_load();
	uint32_t is_repeater = arduino_nvram_repeater_load();
	Net = Ptr<Network>(new Network(cs, is_repeater));
	oled_show("Net up!", Buffer(cs).c_str(), "", "");

	console_setup(Net);
	wifi_setup(Net);
}

void loop()
{
	wifi_handle();
	console_handle();
	Net->run_tasks(arduino_millis_nw());
}
