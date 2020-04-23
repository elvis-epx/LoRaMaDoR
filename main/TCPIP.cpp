#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include "Console.h"

static Ptr<Network> Net;
static Buffer ssid;
static Buffer password;
static WiFiServer wifiServer(23);
static int wifi_status = 0;
static unsigned long int wifi_timeout = 0;
static WiFiClient telnet_client;
static bool is_telnet = false;
Buffer ip = "(none)";
bool mdns = false;

void wifi_setup(Ptr<Network> net)
{
	Net = net;
	ssid = arduino_nvram_load("ssid");
	password = arduino_nvram_load("password");

	Serial.print("Wi-Fi SSID ");
	Serial.println(ssid.cold());

	if (!ssid.str_equal("None")) {
		wifi_status = 1;
		wifi_timeout = millis() + 1000; 
	}
}

Buffer get_wifi_status()
{
	if (wifi_status == 0) {
		return "Wi-Fi disabled.";
	}

	Buffer status = "Wi-Fi SSID ";
	status.append_str(ssid);
	if (wifi_status == 1) {
		status.append_str(", waiting to reconnect");
	} else if (wifi_status == 2) {
		status.append_str(", connecting");
	} else if (wifi_status == 3) {
		status.append_str(", connected, IP ");
		status.append_str(ip);
		if (mdns) {
			status.append_str(", Bonjour name ");
			status.append_str(Net->me().buf().cold());
			status.append_str(".local");
		} else {
			status.append_str(", no Bonjour");
		}
	}
	return status;
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
			ip = Buffer(WiFi.localIP().toString().c_str());
			Serial.println(ip.cold());
			wifiServer.begin();
			if (!MDNS.begin(Net->me().buf().cold())) {
				Serial.println("mDNS not ok, use IP to connect.");
				mdns = false;
			} else {
				Serial.print("Local net name: ");
				Serial.print(Net->me().buf().cold());
				Serial.println(".local");
				mdns = true;
			}
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
			ip = "(none)";
			mdns = false;
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
