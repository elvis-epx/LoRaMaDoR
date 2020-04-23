#include <WiFi.h>

#include "Packet.h"
#include "Network.h"
#include "Display.h"
#include "ArduinoBridge.h"
#include "CLI.h"

const char* ssid = "EPX";
const char* password = "abracadabra";
WiFiServer wifiServer(23);
int wifi_status = 0;
unsigned long int wifi_timeout = 0;
WiFiClient telnet_client;
bool is_telnet = false;

Ptr<Network> Net;

void setup()
{
	Serial.begin(115200);
	oled_init();

	oled_show("Starting...", "", "", "");
	Callsign cs = arduino_nvram_callsign_load();
	Net = Ptr<Network>(new Network(cs));
	oled_show("Net configured", cs.buf().cold(), "", "");
  Serial.print(cs.buf().cold());
	Serial.println(" ready");

  // should be dependent on configured SSID and password
  wifi_status = 1;
  wifi_timeout = millis() + 1000; 
}

void loop()
{
  if (wifi_status == 1) {
    if (millis() > wifi_timeout) {
      WiFi.mode(WIFI_STA);
      WiFi.begin(ssid, password);
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
    cli_telnetmode(false);
  }

  if (!is_telnet && wifi_status == 3 && (telnet_client = wifiServer.available())) {
    is_telnet = true;
    Serial.println("Telnet client connected");
    cli_telnetmode(true);
  }

	while (Serial.available() > 0) {
    int c = Serial.read();
    if (!is_telnet) cli_type(c);
	}
  
  if (is_telnet) {
    while (telnet_client && telnet_client.available() > 0) {
       cli_type(telnet_client.read());
    }
  }

	Net->run_tasks(millis());
}

void platform_print(const char *msg)
{
  if (!is_telnet) {
    Serial.print(msg);
  } else {
    telnet_client.print(msg);
  }
}
