/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

// Main LoRaMaDoR network class, plus some auxiliary types

#include <Preferences.h>
#include "Network.h"
#include "Display.h"
#include "ArduinoBridge.h"
#include "Timestamp.h"
#include "Console.h"
#include "TCPIP.h"

Ptr<Network> Net;
Preferences prefs;

void setup()
{
	Serial.begin(115200);
	oled_init();
	oled_show("Starting...", "", "", "");

	Net = Ptr<Network>(new Network());

	oled_show("Net up!", Buffer(Net->me()).c_str(), "", "");
	console_setup(Net);
	wifi_setup(Net);
}

void loop()
{
	wifi_handle();
	console_handle();
	Net->run_tasks(sys_timestamp());
}
