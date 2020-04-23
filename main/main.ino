#include "Packet.h"
#include "Network.h"
#include "Display.h"
#include "ArduinoBridge.h"
#include "Console.h"
#include "TCPIP.h"

Ptr<Network> Net;

void setup()
{
	Serial.begin(115200);
	oled_init();
	oled_show("Starting...", "", "", "");

	Callsign cs = arduino_nvram_callsign_load();
	Net = Ptr<Network>(new Network(cs));
	oled_show("Net configured", cs.buf().cold(), "", "");

	wifi_setup(Net);
	cons_setup(Net);

	Serial.print(cs.buf().cold());
	Serial.println(" ready");
}

void loop()
{
	wifi_handle();
	cons_handle();
	Net->run_tasks(millis());
}
