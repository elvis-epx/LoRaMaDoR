#include <Arduino.h>
#include "Network.h"
#include "CLI.h"
#include "TCPIP.h"
#include "Console.h"

static Ptr<Network> Net;
static bool is_telnet = false;

void cons_setup(Ptr<Network> net)
{
	Net = net;

	console_print(Net->me().buf().cold());
	console_println(" ready. Type !help to see available commands.");
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

