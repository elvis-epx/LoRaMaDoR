/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

#ifndef UNDER_TEST
#include <Arduino.h>
#endif
#include "Buffer.h"
#include "ArduinoBridge.h"
#include "Display.h"
#include "Network.h"
#include "CLI.h"
#include "Console.h"
#include "TCPIP.h"

// Command-line interface implementation.

extern Ptr<Network> Net;
bool debug = false;

// May be called from anywhere, but mostly from network stack.
void logs(const char* a, const char* b) {
	if (!debug) return;
	Buffer msg = Buffer::sprintf("%s %s", a, b);
	cli_print(msg);
}

void logi(const char* a, int32_t b) {
	if (!debug) return;
	Buffer msg = Buffer::sprintf("%s %d", a, b);
	cli_print(msg);
}

// Print a representation of a received packet
void app_recv(Ptr<Packet> pkt)
{
	Buffer msg = Buffer::sprintf("%s < %s %s\n\r(%s rssi %d)",
				pkt->to().buf().cold(), pkt->from().buf().cold(), pkt->msg().cold(),
				pkt->params().serialized().cold(), pkt->rssi());
	cli_print(msg);
	Buffer msga = Buffer::sprintf("%s < %s", pkt->to().buf().cold(), pkt->from().buf().cold());
	Buffer msgb = Buffer::sprintf("id %d rssi %d", pkt->params().ident(), pkt->rssi());
	Buffer msgc = Buffer::sprintf("p %s", pkt->params().serialized().cold());
	oled_show(msga.cold(), pkt->msg().cold(), msgb.cold(), msgc.cold());
}

// Configure callsign and store to NVRAM
static void cli_parse_callsign(const Buffer &candidate)
{
	if (candidate.empty()) {
		console_print("Callsign is ");
		console_println(Net->me().buf().cold());
		return;
	}
	
	if (candidate.charAt(0) == 'Q') {
		console_print("Invalid Q callsign: ");
		console_println(candidate.cold());
		return;
	}

	Callsign callsign(candidate);

	if (! callsign.is_valid()) {
		console_print("Invalid callsign: ");
		console_println(candidate.cold());
		return;
	}
	
	arduino_nvram_callsign_save(callsign);
	console_println("Callsign saved, restarting...");
	arduino_restart();
}

// Wi-Fi network name (SSID) configuration
static void cli_parse_ssid(Buffer candidate)
{
	candidate.strip();
	if (candidate.empty()) {
		console_print("SSID is '");
		console_print(arduino_nvram_load("ssid").cold());
		console_println("'");
		console_println("Set SSID to None to disable Wi-Fi.");
		return;
	}
	
	arduino_nvram_save("ssid", candidate);
	console_println("SSID saved, call !restart to apply");
}

// Wi-Fi network password configuration
static void cli_parse_password(Buffer candidate)
{
	candidate.strip();
	if (candidate.empty()) {
		console_print("Wi-Fi password is '");
		console_print(arduino_nvram_load("password").cold());
		console_println("'");
		console_println("Set password to None for Wi-Fi network without password.");
		return;
	}
	
	arduino_nvram_save("password", candidate);
	console_println("Wi-Fi password saved, call !restart to apply");
}

// ID of the latest packet sent by us
static void cli_lastid()
{
	auto b = Buffer::sprintf("Last packet ID #%d", Net->get_last_pkt_id());
	console_println(b.cold());
}

// System uptime
static void cli_uptime()
{
	Buffer hms = Buffer::millis_to_hms(arduino_millis());
	console_println(Buffer::sprintf("Uptime %s", hms.cold()).cold());
}

// Print list of detected network neighbours
static void cli_neigh()
{
	console_println("---------------------------");
	console_println(Buffer::sprintf("Neighbourhood of %s:", Net->me().buf().cold()).cold());
	auto neigh = Net->neigh();
	for (auto i = 0; i < neigh.count(); ++i) {
		Buffer cs = neigh.keys()[i];
		int rssi = neigh[cs].rssi;
		int32_t since = arduino_millis() - neigh[cs].timestamp;
		Buffer ssince = Buffer::millis_to_hms(since);
		auto b = Buffer::sprintf("    %s llast seen %s ago w/ %d rssi",
					cs.cold(), ssince.cold(), rssi);
		console_println(b.cold());
	}
	console_println("---------------------------");
}

// Print Wi-Fi status information
static void cli_wifi()
{
	console_println(get_wifi_status().cold());
}

static void cli_parse_help()
{
	console_println();
	console_println("Available commands:");
	console_println();
	console_println("  !callsign CALLSIGN     Set callsign");
	console_println("  !callsign              Get current callsign");
	console_println("  !ssid SSID             Set Wi-Fi network name (None to disable)");
	console_println("  !ssid                  Get configured Wi-Fi network name");
	console_println("  !password PASSWORD     Set Wi-Fi password (None if no password)");
	console_println("  !password              Get configured Wi-Fi password");
	console_println("  !wifi                  Show Wi-Fi/network status");
	console_println("  !debug                 Enable debug/verbose mode");
	console_println("  !nodebug               Disable debug mode");
	console_println("  !restart or !reset     Restart controller");
	console_println("  !neigh                 List known neighbours");
	console_println("  !lastid                Last sent packet #");
	console_println("  !uptime                Show uptime");
	console_println();
}

// !command switchboard
static void cli_parse_meta(Buffer cmd)
{
	cmd.strip();
	if (cmd.strncmp("callsign ", 9) == 0) {
		cmd.cut(9);
		cli_parse_callsign(cmd);
	} else if (cmd.strncmp("callsign", 8) == 0 && cmd.length() == 8) {
		cli_parse_callsign("");
	} else if (cmd.strncmp("ssid ", 5) == 0) {
		cmd.cut(5);
		cli_parse_ssid(cmd);
	} else if (cmd.strncmp("ssid", 4) == 0 && cmd.length() == 4) {
		cli_parse_ssid("");
	} else if (cmd.strncmp("password ", 9) == 0) {
		cmd.cut(9);
		cli_parse_password(cmd);
	} else if (cmd.strncmp("password", 8) == 0 && cmd.length() == 8) {
		cli_parse_password("");
	} else if (cmd.strncmp("wifi", 4) == 0 && cmd.length() == 4) {
		cli_wifi();
	} else if (cmd.strncmp("help", 4) == 0 && cmd.length() == 4) {
		cli_parse_help();
	} else if (cmd.strncmp("debug", 5) == 0 && cmd.length() == 5) {
		console_println("Debug on.");
		debug = true;
	} else if (cmd.strncmp("restart", 7) == 0 && cmd.length() == 7) {
		console_println("Restarting...");
		arduino_restart();
	} else if (cmd.strncmp("reset", 5) == 0 && cmd.length() == 5) {
		console_println("Restarting...");
		arduino_restart();
	} else if (cmd.strncmp("nodebug", 7) == 0 && cmd.length() == 7) {
		console_println("Debug off.");
		debug = false;
	} else if (cmd.strncmp("neigh", 5) == 0 && cmd.length() == 5) {
		cli_neigh();
	} else if (cmd.strncmp("lastid", 6) == 0 && cmd.length() == 6) {
		cli_lastid();
	} else if (cmd.strncmp("uptime", 6) == 0 && cmd.length() == 6) {
		cli_uptime();
	} else {
		console_print("Unknown cmd: ");
		console_println(cmd.cold());
	}
}

// Parse a packet typed in console
static void cli_parse_packet(Buffer cmd)
{
	Buffer preamble;
	Buffer payload = "";
	int sp = cmd.indexOf(' ');
	if (sp < 0) {
		preamble = cmd;
	} else {
		preamble = cmd.substr(0, sp);
		payload = cmd.substr(sp + 1);
	}

	Buffer cdest;
	Buffer sparams = "";
	int sep = preamble.indexOf(':');
	if (sep < 0) {
		cdest = preamble;
	} else {
		cdest = preamble.substr(0, sep);
		sparams = preamble.substr(sep + 1);
	}

	Callsign dest(cdest);

	if (! dest.is_valid()) {
		console_print("Invalid destination: ");
		console_println(cdest.cold());
		return;
	}

	Params params(sparams);
	if (! params.is_valid_without_ident()) {
		console_print("Invalid params: ");
		console_println(sparams.cold());
		return;
	}

	Net->send(dest, params, payload);
}

// Parse a command or packet typed in CLI
static void cli_parse(Buffer cmd)
{
	cmd.lstrip();
	if (cmd.charAt(0) == '!') {
		cmd.cut(1);
		cli_parse_meta(cmd);
	} else {
		cli_parse_packet(cmd);
	}
}

Buffer cli_buf;

// ENTER pressed in CLI
static void cli_enter() {
	console_println();
	if (cli_buf.empty()) {
		return;
	}
	// console_print("Typed: ");
	// console_println(cli_buffer);
	cli_parse(cli_buf);
	cli_buf = "";
}

// Simulate typing. Used by unit testing.
void cli_simtype(const char *c)
{
	while (*c) {
		cli_type(*c++);
	}
}

// Telnet IAC is a special sequence sent by a telnet client
// to configure the server. We don't honor these configurations,
// but a Telnet client sends them anyway, so they need to be
// filtered out.
static int telnet_iac = 0;

// Handled a typed character.
void cli_type(char c) {
	if (telnet_iac == 2) {
		// inside IAC sequence
		if (c == '\xff') {
			// 0xff 0xff = 0xff
			cli_buf.append(c);
			telnet_iac = 0;
		} else {
			// continued IAC sequence
			telnet_iac = 1;
		}
	} else if (telnet_iac == 1) {
		// end of IAC sequence
		telnet_iac = 0;
	} else if (c == '\xff') {
		// enter IAC mode
		telnet_iac = 2;
	} else if (c == 13) {
		cli_enter();
	} else if (c == 8 || c == 127) {
		if (! cli_buf.empty()) {
			cli_buf.cut(-1);
			console_print((char) 8);
			console_print(' ');
			console_print((char) 8);
		}
	} else if (c < 32) {
		// ignore non-handled control chars
	} else if (cli_buf.length() > 200) {
		return;
	} else {
		cli_buf.append(c);
		console_print(c);
	}
}

// Print a message, and reposition the cursor so any command
// that was being typed, is not lost.
void cli_print(const Buffer &msg)
{
	console_println();
	console_println(msg.cold());
	console_print(cli_buf.cold());
}
