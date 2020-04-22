#include <WiFi.h>

#include "Packet.h"
#include "Network.h"
#include "Display.h"
#include "ArduinoBridge.h"
#include "CLI.h"

const char* ssid = "EPX";
const char* password = "abracadabra";
WiFiServer wifiServer(23);
WiFiClient telnet_client;

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
	Serial.println();

  delay(1000);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  Serial.println("Connected to the WiFi network");
  Serial.println(WiFi.localIP());

  wifiServer.begin();
}

void loop()
{
  if (!telnet_client) {
	   while (Serial.available() > 0) {
		   cli_type(Serial.read());
	   }
     telnet_client = wifiServer.available();
     if (telnet_client) {
        Serial.println("Telnet client connected");
        cli_telnetmode(true);
     }
  } else {
     if (telnet_client.connected()) {
        while (telnet_client.connected() && telnet_client.available() > 0) {
           int c = telnet_client.read();
           Serial.print("Received char via telnet ");
           Serial.println(c);
           cli_type(c);
        }
     } else {
        telnet_client = WiFiClient();
        Serial.println("Telnet client disconnected");
        cli_telnetmode(false);
     }
  }

	Net->run_tasks(millis());
}

void platform_print(const char *msg)
{
  if (!telnet_client) {
    Serial.print(msg);
  } else {
    telnet_client.print(msg);
  }
}
