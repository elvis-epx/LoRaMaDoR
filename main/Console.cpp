#include <Arduino.h>
#include "Network.h"
#include "CLI.h"
#include "TCPIP.h"
#include "Console.h"

static Ptr<Network> Net;
static bool is_telnet = false;
static Buffer output_buffer;

void cons_setup(Ptr<Network> net)
{
	Net = net;

	console_print(Net->me().buf().cold());
	console_println(" ready. Type !help to see available commands.");
}

void cons_handle()
{
	if (Serial.available() > 0) {
		int c = Serial.read();
		if (!is_telnet) cli_type(c);
	}

	int a = Serial.availableForWrite();
	int b = output_buffer.length();
	int c = (a < b ? a : b);
	if (c > 0) {
		Serial.write((const uint8_t*) output_buffer.cold(), c);
		output_buffer.cut(c);
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

void serial_print(const char *msg)
{
	output_buffer.append_str(msg);
}

void serial_println(const char *msg)
{
	output_buffer.append_str(msg);
	output_buffer.append_str("\r\n");
}

void platform_print(const char *msg)
{
	if (!is_telnet) {
		serial_print(msg);
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

