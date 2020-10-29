/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

#ifdef UNDER_TEST
#include "Serial.h"
#else
#include <Arduino.h>
#endif
#include "Network.h"
#include "CLI.h"
#include "TCPIP.h"
#include "Console.h"

// Serial and telnet console. Intermediates communication between
// CLI and the platform streams (serial, Telnet).

static Ptr<Network> Net;
static bool is_telnet = false;
static Buffer output_buffer;

// Called by main Arduino setup().
void console_setup(Ptr<Network> net)
{
	Net = net;
	console_print("callsign: ");
	console_print(Net->me());
	console_println(" ready. Type !help to see available commands.");
}

// Called by main Arduino loop(). 
// Handles serial communication, using non-blocking writes.
void console_handle()
{
	if (Serial.available() > 0) {
		int c = Serial.read();
		if (!is_telnet) cli_type(c);
	}

	int a = Serial.availableForWrite();
	int b = output_buffer.length();
	int c = (a < b ? a : b);
	if (c > 0) {
		Serial.write((const uint8_t*) output_buffer.c_str(), c);
		output_buffer.cut(c);
	}
}

void console_telnet_enable()
{
	is_telnet = true;
}

void console_telnet_disable()
{
	is_telnet = false;
}

// Receive typed character from Telnet socket
void console_telnet_type(char c)
{	
	if (is_telnet) cli_type(c);
}

// Print to serial console (through a buffer; see console_handle())
void serial_print(const char *msg)
{
	output_buffer += msg;
}

// Redirects console print to Telnet if there is a connection,
// otherwise sends to serial console.
static void platform_print(const char *msg)
{
	if (!is_telnet) {
		serial_print(msg);
	} else {
		telnet_print(msg);
	}
}

// Print string on console.
void console_print(const char *msg) {
	platform_print(msg);
}

void console_print(const Buffer &msg) {
	platform_print(msg.c_str());
}

void console_print(char c) {
	char msg[] = {c, 0};
	platform_print(msg);
}

void console_println(const char *msg) {
	platform_print(msg);
	platform_print("\r\n");
}

void console_println(const Buffer &msg) {
	platform_print(msg.c_str());
	platform_print("\r\n");
}

void console_println() {
	platform_print("\r\n");
}

