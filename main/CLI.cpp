#ifndef UNDER_TEST
#include <Arduino.h>
#endif
#include "Buffer.h"
#include "ArduinoBridge.h"
#include "Display.h"
#include "Network.h"
#include "CLI.h"

extern Ptr<Network> Net;
bool debug = false;

void logs(const char* a, const char* b) {
	if (!debug) return;
	Buffer msg = Buffer::sprintf("%s %s", a, b);
	cli_print(msg);
}

void logi(const char* a, long int b) {
	if (!debug) return;
	Buffer msg = Buffer::sprintf("%s %ld", a, b);
	cli_print(msg);
}

void app_recv(Ptr<Packet> pkt)
{
	Buffer msg = Buffer::sprintf("%s < %s %s\n\r(%s rssi %d)",
				pkt->to().buf().cold(), pkt->from().buf().cold(), pkt->msg().cold(),
				pkt->params().serialized().cold(), pkt->rssi());
	cli_print(msg);
	Buffer msga = Buffer::sprintf("%s < %s", pkt->to().buf().cold(), pkt->from().buf().cold());
	Buffer msgb = Buffer::sprintf("id %ld rssi %d", pkt->params().ident(), pkt->rssi());
	Buffer msgc = Buffer::sprintf("p %s", pkt->params().serialized().cold());
	oled_show(msga.cold(), pkt->msg().cold(), msgb.cold(), msgc.cold());
}

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

static void cli_lastid()
{
	auto b = Buffer::sprintf("Last packet ID #%ld", Net->get_last_pkt_id());
	console_println(b.cold());
}

static void cli_neigh()
{
	console_println("---------------------------");
	console_println(Buffer::sprintf("Neighbourhood of %s:", Net->me().buf().cold()).cold());
	auto neigh = Net->neigh();
	for (auto i = 0; i < neigh.count(); ++i) {
		Buffer cs = neigh.keys()[i];
		int rssi = neigh[cs].rssi;
		auto b = Buffer::sprintf("    %s rssi %d", cs.cold(), rssi);
		console_println(b.cold());
	}
	console_println("---------------------------");
}

static void cli_parse_meta(Buffer cmd)
{
	cmd.strip();
	if (cmd.strncmp("callsign ", 9) == 0) {
		cmd.cut(9);
		cli_parse_callsign(cmd);
	} else if (cmd.strncmp("debug", 5) == 0 && cmd.length() == 5) {
		console_println("Debug on.");
		debug = true;
	} else if (cmd.strncmp("nodebug", 7) == 0 && cmd.length() == 7) {
		console_println("Debug off.");
		debug = false;
	} else if (cmd.strncmp("neigh", 5) == 0 && cmd.length() == 5) {
		cli_neigh();
	} else if (cmd.strncmp("lastid", 6) == 0 && cmd.length() == 6) {
		cli_lastid();
	} else if (cmd.strncmp("callsign", 8) == 0 && cmd.length() == 8) {
		cli_parse_callsign("");
	} else {
		console_print("Unknown cmd: ");
		console_println(cmd.cold());
	}
}

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

void cli_simtype(const char *c)
{
	while (*c) {
		cli_type(*c++);
	}
}

static bool telnet_mode = false;
static int telnet_iac = 0;

void cli_telnetmode(bool mode)
{
	telnet_mode = mode;
	telnet_iac = 0;
}

void cli_type(char c) {
	if (telnet_mode && telnet_iac == 2) {
		// inside IAC sequence
		if (c == 255) {
			// 0xff 0xff = 0xff
			cli_buf.append(c);
			telnet_iac = 0;
		} else {
			// continued IAC sequence
			telnet_iac = 1;
		}
	} else if (telnet_mode && telnet_iac == 1) {
		// end of IAC sequence
		telnet_iac = 0;
	} else if (telnet_mode && c == 255) {
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

void cli_print(const Buffer &msg)
{
	console_println();
	console_println(msg.cold());
	console_print(cli_buf.cold());
}
