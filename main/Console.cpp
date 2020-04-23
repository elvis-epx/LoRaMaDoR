#include <Arduino.h>
#include "Network.h"
#include "CLI.h"
#include "TCPIP.h"

static Ptr<Network> Net;
static bool is_telnet = false;

void cons_setup(Ptr<Network> net)
{
	Net = net;
}

void cons_handle()
{
	while (Serial.available() > 0) {
		int c = Serial.read();
		if (!is_telnet) cli_type(c);
	}
}

void cons_telnet_enable()
{
	is_telnet = true;
}

void cons_telnet_disable()
{
	is_telnet = false;
}

void cons_telnet_type(char c)
{	
	if (is_telnet) cli_type(c);
}

void platform_print(const char *msg)
{
	if (!is_telnet) {
		Serial.print(msg);
	} else {
		telnet_print(msg);
	}
}
