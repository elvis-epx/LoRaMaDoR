/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

#ifndef UNDER_TEST
#include <Arduino.h>
#endif
#include "Buffer.h"
#include "ArduinoBridge.h"
#include "Timestamp.h"
#include "NVRAM.h"
#include "Display.h"
#include "Network.h"
#include "CLI.h"
#include "Console.h"
#include "Telnet.h"

// Command-line interface implementation.

extern Ptr<Network> Net;
bool debug = false;
bool tnc = false; // console used by a computer, not a human
Buffer cli_buf;

// Print a message, and reposition the cursor so any command
// that was being typed, is not lost.
// In TNC mode, just print the message.
static void cli_print(const Buffer &msg)
{
	if (!tnc) console_println();
	console_println(msg);
	if (!tnc) console_print(cli_buf);
}

// May be called from anywhere, but mostly from network stack.
void logs(const char* a, const char* b) {
	if (!debug && !tnc) return;
	cli_print(Buffer("debug: ") + a + " " + b);
}

void logs(const char* a, const Buffer &b)
{
	if (!debug && !tnc) return;
	cli_print(Buffer("debug: ") + a + " " + b);
}

void logi(const char* a, int32_t b) {
	if (!debug && !tnc) return;
	cli_print(Buffer("debug: ") + a + " " + Buffer::itoa(b));
}

// Print a representation of a received packet
void app_recv(Ptr<Packet> pkt)
{
	if (tnc) {
		Buffer data = pkt->encode_l3();
		cli_print(Buffer("pkt: ") +
			Buffer::itoa(pkt->rssi()) + " " +
			Buffer::itoa(data.length()) + " " +
			data);
		return;
	}

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
		console_print("cli: Callsign is ");
		console_println(Net->me());
		return;
	}
	
	if (candidate.charAt(0) == 'Q') {
		console_print("cli: Invalid Q callsign: ");
		console_println(candidate);
		return;
	}

	Callsign callsign(candidate);

	if (! callsign.is_valid()) {
		console_print("cli: Invalid callsign: ");
		console_println(candidate);
		return;
	}
	
	arduino_nvram_callsign_save(callsign);
	console_println("cli: Callsign saved. Effective next restart.");
	console_println("cli: Issue !reset or !restart to apply immediately.");
}

// Configure or print repeater function status
static void cli_parse_repeater(const Buffer &candidate)
{
	if (candidate.empty()) {
		console_print("cli: Repeater function is ");
		console_println(arduino_nvram_repeater_load() ? "1 (on)" : "0 (off)");
		return;
	}
	
	if (candidate.charAt(0) != '0' && candidate.charAt(0) != '1') {
		console_println("cli: Invalid new value, should be 0 or 1");
		return;
	}
	
	arduino_nvram_repeater_save(candidate.charAt(0) - '0');
	console_println("cli: Repeater config saved. Effective next restart.");
}

// Configure or print beacon interval time in seconds
static void cli_parse_beacon(const Buffer &candidate)
{
	if (candidate.empty()) {
		console_print("cli: Beacon average interval is ");
		console_print(Buffer::itoa(arduino_nvram_beacon_load()));
		console_println("s");
		return;
	}

	int b = candidate.toInt();
	
	if (b < 10 || b > 600) {
		console_println("cli: Invalid value. Beacon interval must be 10..600s.");
		return;
	}
	
	arduino_nvram_beacon_save(b);
	console_println("cli: Beacon interval saved. Effective after next beacon.");
}

// Configure pre-shared key (PSK) for HMAC packet authentication
static void cli_parse_psk(Buffer candidate)
{
	if (candidate.empty()) {
		Buffer psk = arduino_nvram_psk_load();
		if (psk.empty()) {
			console_println("cli: No pre-shared key is configured.");
		} else {
			console_print("cli: Pre-shared key is '");
			console_print(psk);
			console_println("'");
			// console_println("FIXME scrub the key for security reasons");
		}
		return;
	}

	if (candidate.length() > 32) {
		console_println("cli: Pre-shared key must have between 1 and 32 ASCII chars.");
		return;
	}

	if (candidate == "None") {
		candidate = "";
	}
	
	arduino_nvram_psk_save(candidate);
	console_println("cli: Pre-shared key saved, effective immediately.");
	console_println("cli: Make sure your peers are using the same key.");
	console_println("cli: Activate !debug mode to check HMAC-related issues.");
}

// Configure or print beacon first interval time in seconds
static void cli_parse_beacon_first(const Buffer &candidate)
{
	if (candidate.empty()) {
		console_print("cli: Beacon 1st average interval is ");
		console_print(Buffer::itoa(arduino_nvram_beacon_first_load()));
		console_println("s");
		return;
	}

	int b = candidate.toInt();
	
	if (b < 1 || b > 300) {
		console_println("cli: Invalid value. Beacon 1st interval must be 1..300s.");
		return;
	}
	
	arduino_nvram_beacon_first_save(b);
	console_println("cli: Beacon 1st interval saved. Effective next restart.");
}

// Wi-Fi network name (SSID) configuration
static void cli_parse_ssid(Buffer candidate)
{
	candidate.strip();
	if (candidate.empty()) {
		console_print("cli: SSID is '");
		console_print(arduino_nvram_load("ssid"));
		console_println("'");
		console_println("cli: Set SSID to None to disable Wi-Fi.");
		return;
	}

	if (candidate.length() > 64) {
		console_println("cli: maximum SSID length is 64.");
		return;
	}
	
	arduino_nvram_save("ssid", candidate);
	console_println("cli: SSID saved, call !restart to apply");
}

// Wi-Fi network password configuration
static void cli_parse_password(Buffer candidate)
{
	candidate.strip();
	if (candidate.empty()) {
		console_print("cli: Wi-Fi password is '");
		console_print(arduino_nvram_load("password"));
		console_println("'");
		console_println("cli: Set password to None for Wi-Fi network without password.");
		return;
	}
	
	if (candidate.length() > 64) {
		console_println("cli: maximum password length is 64.");
		return;
	}
	
	arduino_nvram_save("password", candidate);
	console_println("cli: Wi-Fi password saved, call !restart to apply");
}

// ID of the latest packet sent by us
static void cli_lastid()
{
	auto b = Buffer("cli: Last packet ID #");
	b += Buffer::itoa(Net->get_last_pkt_id());
	console_println(b);
}

// System uptime
static void cli_uptime()
{
	Buffer hms = Buffer::millis_to_hms(sys_timestamp());
	console_println(Buffer("cli: Uptime ") + hms);
}

// Print list of detected network neighbors
static void cli_neigh()
{
	console_println("cli: ---------------------------");
	console_println(Buffer("cli: Neighborhood of ") + Net->me() + ":");

	auto now = sys_timestamp();
	auto neigh = Net->neighbors();
	for (size_t i = 0; i < neigh.count(); ++i) {
		Buffer cs = neigh.keys()[i];
		int rssi = neigh[cs].rssi;
		int64_t since = now - neigh[cs].timestamp;
		Buffer ssince = Buffer::millis_to_hms(since);
		auto b = Buffer("cli:     ") + cs + " last seen " + ssince +
			" ago, rssi " + Buffer::itoa(rssi);
		console_println(b);
	}
	auto peers = Net->peers();
	for (size_t i = 0; i < peers.count(); ++i) {
		Buffer cs = peers.keys()[i];
		if (neigh.has(cs)) {
			continue;
		}
		int64_t since = now - peers[cs].timestamp;
		Buffer ssince = Buffer::millis_to_hms(since);
		auto b = Buffer("cli:     ") + cs + " last seen " + ssince +
			" ago, non adjacent";
		console_println(b);
	}
	console_println("cli: --------------------------");
}

// Print Wi-Fi status information
static void cli_wifi()
{
	console_println(Buffer("cli: ") + get_wifi_status());
}

static void cli_parse_help()
{
	console_println("cli: ");
	console_println("cli: Available commands:");
	console_println("cli: ");
	console_println("cli:  !callsign [CALLSIGN]   Get/set callsign");
	console_println("cli:  !ssid [SSID]           Get/set Wi-Fi network (None to disable)");
	console_println("cli:  !password [PASSWORD]   Get/set Wi-Fi password (None if no password)");
	console_println("cli:  !repeater [0 or 1]     Get/set repeater function switch");
	console_println("cli:  !beacon [10..600]      Get/set beacon average time (in seconds)");
	console_println("cli:  !beacon1st [10..300]   Get/set first beacon avg time");
	console_println("cli:  !psk [KEY]             Get/Set optional HMAC pre-shared key (None to disable)");
	console_println("cli:  !wifi                  Show Wi-Fi/network status");
	console_println("cli:  !defconfig             Reset all configurations saved in NVRAM");
	console_println("cli:  !debug / !nodebug      Enable/disable debug and verbose mode");
	console_println("cli:  !tnc / !notnc          Enable/disable TNC mode");
	console_println("cli:  !restart or !reset     Restart controller");
	console_println("cli:  !neigh                 List known neighbors");
	console_println("cli:  !lastid                Last sent packet #");
	console_println("cli:  !uptime                Show uptime");
	console_println("cli:");
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
		console_println("cli: Debug on.");
		debug = true;
	} else if (cmd == "tnc") {
		console_println("cli: TNC mode on.");
		tnc = true;
	} else if (cmd == "restart" || cmd == "reset") {
		console_println("cli: Restarting...");
		arduino_restart();
	} else if (cmd == "defconfig") {
		console_println("cli: cleaning NVRAM...");
		arduino_nvram_clear_all();
		arduino_restart();
	} else if (cmd == "nodebug") {
		console_println("cli: Debug off.");
		debug = false;
	} else if (cmd == "notnc") {
		console_println("cli: TNC mode off");
		tnc = false;
	} else if (cmd == "neigh") {
		cli_neigh();
	} else if (cmd == "lastid") {
		cli_lastid();
	} else if (cmd == "uptime") {
		cli_uptime();
	} else {
		console_print("cli: Unknown cmd: ");
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
		console_print("net: Invalid destination: ");
		console_println(cdest);
		return;
	}

	if (dest.is_reserved()) {
		console_println("net: QB is reserved to automatic beacon.");
		return;
	}

	Params params(sparams);
	if (! params.is_valid_without_ident()) {
		console_print("net: Invalid params: ");
		console_println(sparams);
		return;
	}

	uint32_t id = Net->send(dest, params, payload);
	console_println(Buffer("net: Sent packet #") + Buffer::itoa(id) + ".");
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

// ENTER pressed in CLI
static void cli_enter() {
	if (!tnc) console_println();
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
			if (!tnc) console_print((char) 8);
			if (!tnc) console_print(' ');
			if (!tnc) console_print((char) 8);
		}
	} else if (c < 32) {
		// ignore non-handled control chars
	} else if (cli_buf.length() > 200) {
		return;
	} else {
		cli_buf += c;
		if (!tnc) console_print(c);
	}
}
