/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

// Main LoRaMaDoR network class, plus some auxiliary types

#include <Preferences.h>
#include "src/Network.h"
#include "src/Display.h"
#include "src/ArduinoBridge.h"
#include "src/Timestamp.h"
#include "src/Console.h"
#include "src/Telnet.h"

#include "driver/adc.h"
#include <esp_wifi.h>
#include <esp_bt.h>

Ptr<Network> Net;
Preferences prefs;

void setup()
{
	// Base current, beacons every 10s: 106mA (avg over 30min)
	// 80MHz CPU freq: -34mA
	// OLED completely off: -5mA
	// OLED with no text: -2mA
	// beacons every 600s: -3mA

	setCpuFrequencyMhz(80);
	esp_bt_controller_disable();
	// adc_power_off();

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
