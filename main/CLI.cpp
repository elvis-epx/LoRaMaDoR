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
	cli_print(Buffer(a) + " " + b);
}

void logs(const char* a, const Buffer &b)
{
	if (!debug) return;
	cli_print(Buffer(a) + " " + b);
}

void logi(const char* a, int32_t b) {
	if (!debug) return;
	cli_print(Buffer(a) + " " + Buffer::itoa(b));
}

// Print a representation of a received packet
void app_recv(Ptr<Packet> pkt)
{
	Buffer msg = Buffer(pkt->to()) + " < " + pkt->from() + " " +
		pkt->msg().c_str() + /* avoid \0s in message */
		"\r\n(" + pkt->params().serialized() + " rssi " +
		Buffer::itoa(pkt->rssi()) + ")";
	cli_print(msg);
	Buffer msga = Buffer(pkt->to()) + " < " + pkt->from();
	Buffer msgb = Buffer("id ") + pkt->params().s_ident() +
		" rssi " + Buffer::itoa(pkt->rssi());
	Buffer msgc = Buffer("p ") + pkt->params().serialized();
	oled_show(msga.c_str(), pkt->msg().c_str(), msgb.c_str(), msgc.c_str());
}

// Configure callsign and store to NVRAM
static void cli_parse_callsign(const Buffer &candidate)
{
	if (candidate.empty()) {
		console_print("Callsign is ");
		console_println(Net->me());
		return;
	}
	
	if (candidate.charAt(0) == 'Q') {
		console_print("Invalid Q callsign: ");
		console_println(candidate);
		return;
	}

	Callsign callsign(candidate);

	if (! callsign.is_valid()) {
		console_print("Invalid callsign: ");
		console_println(candidate);
		return;
	}
	
	arduino_nvram_callsign_save(callsign);
	console_println("Callsign saved, restarting...");
	arduino_restart();
}

// Configure or print repeater function status
static void cli_parse_repeater(const Buffer &candidate)
{
	if (candidate.empty()) {
		console_print("Repeater function is ");
		console_println(arduino_nvram_repeater_load() ? "1 (on)" : "0 (off)");
		return;
	}
	
	if (candidate.charAt(0) != '0' && candidate.charAt(0) != '1') {
		console_println("Invalid new value, should be 0 or 1");
		return;
	}
	
	arduino_nvram_repeater_save(candidate.charAt(0) - '0');
	console_println("Repeater config saved. Effective next restart.");
}

// Configure or print beacon interval time in seconds
static void cli_parse_beacon(const Buffer &candidate)
{
	if (candidate.empty()) {
		console_print("Beacon average interval is ");
		console_print(Buffer::itoa(arduino_nvram_beacon_load()));
		console_println("s");
		return;
	}

	int b = candidate.toInt();
	
	if (b < 10 || b > 600) {
		console_println("Invalid value. Beacon interval must be 10..600s.");
		return;
	}
	
	arduino_nvram_beacon_save(b);
	console_println("Beacon interval saved. Effective after next beacon.");
}

// Configure pre-shared key (PSK) for HMAC packet authentication
static void cli_parse_psk(Buffer candidate)
{
	if (candidate.empty()) {
		Buffer psk = arduino_nvram_psk_load();
		if (psk.empty()) {
			console_println("No pre-shared key is configured.");
		} else {
			console_print("Pre-shared key is '");
			console_print(psk);
			console_println("'");
			console_println("FIXME scrub the key for security reasons");
		}
		return;
	}

	if (candidate.length() > 32) {
		console_println("Pre-shared key must have between 1 and 32 ASCII chars.");
		return;
	}

	if (candidate == "None") {
		candidate = "";
	}
	
	arduino_nvram_psk_save(candidate);
	console_println("Pre-shared key saved, effective immediately.");
	console_println("Make sure your peers are using the same key.");
	console_println("Activate !debug mode to check HMAC-related issues.");
}

// Configure or print beacon first interval time in seconds
static void cli_parse_beacon_first(const Buffer &candidate)
{
	if (candidate.empty()) {
		console_print("Beacon 1st average interval is ");
		console_print(Buffer::itoa(arduino_nvram_beacon_first_load()));
		console_println("s");
		return;
	}

	int b = candidate.toInt();
	
	if (b < 1 || b > 300) {
		console_println("Invalid value. Beacon 1st interval must be 1..300s.");
		return;
	}
	
	arduino_nvram_beacon_first_save(b);
	console_println("Beacon 1st interval saved. Effective next restart.");
}

// Wi-Fi network name (SSID) configuration
static void cli_parse_ssid(Buffer candidate)
{
	candidate.strip();
	if (candidate.empty()) {
		console_print("SSID is '");
		console_print(arduino_nvram_load("ssid"));
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
		console_print(arduino_nvram_load("password"));
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
	auto b = Buffer("Last packet ID #");
	b += Buffer::itoa(Net->get_last_pkt_id());
	console_println(b);
}

// System uptime
static void cli_uptime()
{
	Buffer hms = Buffer::millis_to_hms(arduino_millis_nw());
	console_println(Buffer("Uptime ") + hms);
}

// Print list of detected network neighbors
static void cli_neigh()
{
	console_println("---------------------------");
	console_println(Buffer("Neighborhood of ") + Net->me() + ":");

	auto now = arduino_millis_nw();
	auto neigh = Net->neighbors();
	for (auto i = 0; i < neigh.count(); ++i) {
		Buffer cs = neigh.keys()[i];
		int rssi = neigh[cs].rssi;
		int64_t since = now - neigh[cs].timestamp;
		Buffer ssince = Buffer::millis_to_hms(since);
		auto b = Buffer("    ") + cs + " last seen " + ssince +
			" ago, rssi " + Buffer::itoa(rssi);
		console_println(b);
	}
	auto peers = Net->peers();
	for (auto i = 0; i < peers.count(); ++i) {
		Buffer cs = peers.keys()[i];
		if (neigh.has(cs)) {
			continue;
		}
		int64_t since = now - peers[cs].timestamp;
		Buffer ssince = Buffer::millis_to_hms(since);
		auto b = Buffer("    ") + cs + " last seen " + ssince +
			" ago, non adjacent";
		console_println(b);
	}
	console_println("---------------------------");
}

// Print Wi-Fi status information
static void cli_wifi()
{
	console_println(get_wifi_status());
}

static void cli_parse_help()
{
	console_println();
	console_println("Available commands:");
	console_println();
	console_println("  !callsign [CALLSIGN]   Get/set callsign");
	console_println("  !ssid [SSID]           Get/set Wi-Fi network (None to disable)");
	console_println("  !password [PASSWORD]   Get/set Wi-Fi password (None if no password)");
	console_println("  !repeater [0 or 1]     Get/set repeater function switch");
	console_println("  !beacon [10..600]      Get/set beacon average time (in seconds)");
	console_println("  !beacon1st [10..300]   Get/set first beacon avg time");
	console_println("  !psk [KEY]             Get/Set optional HMAC pre-shared key (None to disable)");
	console_println("  !wifi                  Show Wi-Fi/network status");
	console_println("  !debug                 Enable debug/verbose mode");
	console_println("  !nodebug               Disable debug mode");
	console_println("  !restart or !reset     Restart controller");
	console_println("  !neigh                 List known neighbors");
	console_println("  !lastid                Last sent packet #");
	console_println("  !uptime                Show uptime");
	console_println();
}

// !command switchboard
static void cli_parse_meta(Buffer cmd)
{
	cmd.strip();
	if (cmd.startsWith("callsign ")) {
		cmd.cut(9);
		cli_parse_callsign(cmd);
	} else if (cmd == "callsign") {
		cli_parse_callsign("");
	} else if (cmd.startsWith("repeater ")) {
		cmd.cut(9);
		cli_parse_repeater(cmd);
	} else if (cmd == "repeater") {
		cli_parse_repeater("");
	} else if (cmd.startsWith("beacon ")) {
		cmd.cut(7);
		cli_parse_beacon(cmd);
	} else if (cmd.startsWith("psk ")) {
		cmd.cut(4);
		cli_parse_psk(cmd);
	} else if (cmd.startsWith("psk")) {
		cmd.cut(3);
		cli_parse_psk(cmd);
	} else if (cmd == "beacon") {
		cli_parse_beacon("");
	} else if (cmd.startsWith("beacon1st ")) {
		cmd.cut(10);
		cli_parse_beacon_first(cmd);
	} else if (cmd == "beacon1st") {
		cli_parse_beacon_first("");
	} else if (cmd.startsWith("ssid ")) {
		cmd.cut(5);
		cli_parse_ssid(cmd);
	} else if (cmd == "ssid") {
		cli_parse_ssid("");
	} else if (cmd.startsWith("password ")) {
		cmd.cut(9);
		cli_parse_password(cmd);
	} else if (cmd == "password") {
		cli_parse_password("");
	} else if (cmd == "wifi") {
		cli_wifi();
	} else if (cmd == "help") {
		cli_parse_help();
	} else if (cmd == "debug") {
		console_println("Debug on.");
		debug = true;
	} else if (cmd == "restart" || cmd == "reset") {
		console_println("Restarting...");
		arduino_restart();
	} else if (cmd == "nodebug") {
		console_println("Debug off.");
		debug = false;
	} else if (cmd == "neigh") {
		cli_neigh();
	} else if (cmd == "lastid") {
		cli_lastid();
	} else if (cmd == "uptime") {
		cli_uptime();
	} else {
		console_print("Unknown cmd: ");
		console_println(cmd);
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
		console_println(cdest);
		return;
	}

	if (dest.is_reserved()) {
		console_println("QB is reserved to automatic beacon.");
		return;
	}

	Params params(sparams);
	if (! params.is_valid_without_ident()) {
		console_print("Invalid params: ");
		console_println(sparams);
		return;
	}

	uint32_t id = Net->send(dest, params, payload);
	console_println(Buffer("Sent packet #") + Buffer::itoa(id) + ".");
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
			cli_buf += c;
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
		cli_buf += c;
		console_print(c);
	}
}

// Print a message, and reposition the cursor so any command
// that was being typed, is not lost.
void cli_print(const Buffer &msg)
{
	console_println();
	console_println(msg);
	console_print(cli_buf);
}
