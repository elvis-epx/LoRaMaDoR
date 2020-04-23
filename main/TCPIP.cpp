#include <Arduino.h>
#include <WiFi.h>

#include "ArduinoBridge.h"
#include "Console.h"

static Buffer ssid;
static Buffer password;
static WiFiServer wifiServer(23);
static int wifi_status = 0;
static unsigned long int wifi_timeout = 0;
static WiFiClient telnet_client;
static bool is_telnet = false;

void wifi_setup()
{
	ssid = arduino_nvram_load("ssid");
	password = arduino_nvram_load("password");

	Serial.print("Wi-Fi SSID ");
	Serial.println(ssid.cold());

	if (!ssid.str_equal("None")) {
		wifi_status = 1;
		wifi_timeout = millis() + 1000; 
	}
}

void wifi_handle()
{
	if (wifi_status == 1) {
		if (millis() > wifi_timeout) {
			WiFi.mode(WIFI_STA);
			if (password.str_equal("None")) {
				WiFi.begin(ssid.cold());
			} else {
				WiFi.begin(ssid.cold(), password.cold());
			}
			Serial.println("Connecting to WiFi...");
			wifi_status = 2;
			wifi_timeout = millis() + 20000;
		}
	} else if (wifi_status == 2) {
		int ws = WiFi.status();
		if (ws == WL_CONNECTED) {
			Serial.println("Connected to WiFi");
			Serial.println(WiFi.localIP().toString().c_str());
			wifiServer.begin();
			wifi_status = 3;
		} else if (millis() > wifi_timeout) {
			Serial.println("Failed to connect to WiFi");
			wifi_status = 1;
			wifi_timeout = millis() + 1000;
		}
	} else if (wifi_status == 3) {
		if (WiFi.status() != WL_CONNECTED) {
			Serial.println("Disconnected from WiFi.");
			wifi_status = 1;
			wifi_timeout = millis() + 1000;
			telnet_client.stop();
		}
	}

	if (is_telnet && !telnet_client) {
		is_telnet = false;
		Serial.println("Telnet client disconnected");
		cons_telnet_disable();
	}

	if (!is_telnet && wifi_status == 3 && (telnet_client = wifiServer.available())) {
		is_telnet = true;
		Serial.println("Telnet client connected");
		cons_telnet_enable();
	}

	if (is_telnet) {
		while (telnet_client && telnet_client.available() > 0) {
			cons_telnet_type(telnet_client.read());
		}
	}
}

void telnet_print(const char* c)
{
	telnet_client.print(c);
}
