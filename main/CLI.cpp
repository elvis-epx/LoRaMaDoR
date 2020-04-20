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

void cli_parse_callsign(const Buffer &candidate)
{
	if (candidate.empty()) {
		serial_print("Callsign is ");
		serial_println(Net->me().buf().cold());
		return;
	}
	
	if (candidate.charAt(0) == 'Q') {
		serial_print("Invalid Q callsign: ");
		serial_println(candidate.cold());
		return;
	}

	Callsign callsign(candidate);

	if (! callsign.is_valid()) {
		serial_print("Invalid callsign: ");
		serial_println(candidate.cold());
		return;
	}
	
	arduino_nvram_callsign_save(callsign);
	serial_println("Callsign saved, restarting...");
	arduino_restart();
}

void cli_parse_meta(Buffer cmd)
{
	cmd.strip();
	if (cmd.strncmp("callsign ", 9) == 0) {
		cmd.cut(9);
		cli_parse_callsign(cmd);
	} else if (cmd.strncmp("debug", 5) == 0 && cmd.length() == 5) {
		serial_println("Debug on.");
		debug = true;
	} else if (cmd.strncmp("nodebug", 7) == 0 && cmd.length() == 7) {
		serial_println("Debug off.");
		debug = false;
	} else if (cmd.strncmp("callsign", 8) == 0 && cmd.length() == 8) {
		cli_parse_callsign("");
	} else {
		serial_print("Unknown cmd: ");
		serial_println(cmd.cold());
	}
}

void cli_parse_packet(Buffer cmd)
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
		serial_print("Invalid destination: ");
		serial_println(cdest.cold());
		return;
	}

	Params params(sparams);
	if (! params.is_valid_without_ident()) {
		serial_print("Invalid params: ");
		serial_println(sparams.cold());
		return;
	}

	Net->send(dest, params, payload);
}

void cli_parse(Buffer cmd)
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

void cli_enter() {
	serial_println();
	if (cli_buf.empty()) {
		return;
	}
	// serial_print("Typed: ");
	// serial_println(cli_buffer);
	cli_parse(cli_buf);
	cli_buf = "";
}

void cli_type(char c) {
	if (c == 13) {
		cli_enter();
	} else if (c == 8 || c == 127) {
		if (! cli_buf.empty()) {
			cli_buf.cut(-1);
			serial_print((char) 8);
			serial_print(' ');
			serial_print((char) 8);
		}
	} else if (cli_buf.length() > 500) {
		return;
	} else {
		cli_buf.append(c);
		serial_print(c);
	}
}

void cli_print(const Buffer &msg)
{
	serial_println();
	serial_println(msg.cold());
	serial_print(cli_buf.cold());
}
