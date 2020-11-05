#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "Network.h"
#include "ArduinoBridge.h"
#include "Timestamp.h"
#include "NVRAM.h"
#include "CLI.h"
#include "Serial.h"
#include "Console.h"

// Radio emulation hooks in FakeArduino.cpp
int lora_emu_socket();
void lora_emu_socket_coverage(int coverage);
void lora_emu_rx();

Ptr<Network> Net(0);

void ping_self()
{
	printf("$$$$$ CLI PING SELF\n");
	cli_simtype("QL:PING payload\r");
	char *cmd;
	asprintf(&cmd, "%s:PING payload\r", Buffer(Net->me()).c_str());
	cli_simtype(cmd);
	free(cmd);
}

void ping()
{
	printf("$$$$$ CLI PING\n");
	const Dict<Peer> neigh = Net->peers();
	const Vector<Buffer> neigh_cs = neigh.keys();
	auto n = neigh.count();
	if (!n) {
		return;
	}

	Buffer scs = neigh_cs[arduino_random(0, (int) n)];
	Callsign cs(scs);
	if (!cs.is_valid()) {
		printf("Neigh callsign invalid %s\n", scs.c_str());
		exit(2);
	}
	char *cmd;
	asprintf(&cmd, "%s:PING payload\r", scs.c_str());
	cli_simtype(cmd);
	free(cmd);
}

void rreq()
{
	printf("$$$$$ CLI RREQ\n");
	const Dict<Peer> neigh = Net->peers();
	const Vector<Buffer> neigh_cs = neigh.keys();
	auto n = neigh.count();
	if (!n) {
		return;
	}
	Buffer scs = neigh_cs[arduino_random(0, (int) n)];
	Callsign cs(scs);
	if (!cs.is_valid()) {
		printf("Neigh callsign invalid %s\n", scs.c_str());
		exit(2);
	}
	char *cmd;
	asprintf(&cmd, "%s:RREQ\r", scs.c_str());
	cli_simtype(cmd);
	free(cmd);
}

void sendm_bad()
{
	cli_simtype("QB ola\r");
}

void sendm()
{
	printf("$$$$$ CLI send msg\n");
	const Dict<Peer> neigh = Net->peers();
	const Vector<Buffer> neigh_cs = neigh.keys();
	auto n = neigh.count();
	if (!n) {
		return;
	}
	Buffer scs = neigh_cs[arduino_random(0, (int) n)];
	Callsign cs(scs);
	if (!cs.is_valid()) {
		printf("Neigh callsign invalid %s\n", scs.c_str());
		exit(2);
	}
	char *cmd;
	int opt = arduino_random(0, 4);
	if (opt == 0) {
		asprintf(&cmd, "%s ola\r", scs.c_str());
	} else if (opt == 1) {
		asprintf(&cmd, "%s:C ola\r", scs.c_str());
	} else if (opt == 2) {
		asprintf(&cmd, "%s:C,CO ola\r", scs.c_str());
	} else {
		asprintf(&cmd, "QC:C ola\r");
	}
	cli_simtype(cmd);
	free(cmd);
}

int main(int argc, char* argv[])
{
	if (argc < 6) {
		printf("Specify, callsign, repeater mode, coverage bitmask, HMAC PSK and serial emulation port\n");
		return 1;
	}

	arduino_nvram_id_save(MAX_PACKET_ID - 10);

	int repeater = atoi(argv[2]) ? 1 : 0;
	int coverage = atoi(argv[3]) & 0xff;
	int serialemu = atoi(argv[5]);
	if (repeater < 0 || repeater > 1) {
		printf("repeater mode must be 0 or 1\n");
	}
	if (!coverage) {
		printf("Coverage bitmask must not be 0\n");
		return 1;
	}

	Serial.emu_port(serialemu);
	lora_emu_socket_coverage(coverage);

	Callsign cs(argv[1]);
	if (!cs.is_valid()) {
		printf("Callsign invalid.\n");
		return 2;
	}

	char cli_cmd[100];

	cli_simtype("!beacon1st\r");
	cli_simtype("!beacon1st 0\r");
	cli_simtype("!beacon1st 1\r");
	cli_simtype("!psk None\r");
	assert(arduino_nvram_psk_load() == "");
	cli_simtype("!psk\r");
	cli_simtype("!psk abracadabra\r");
	assert(arduino_nvram_psk_load() == "abracadabra");
	cli_simtype("!psk\r");
	cli_simtype("!psk \r");
	assert(arduino_nvram_psk_load() == "abracadabra");
	cli_simtype("!psk ddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd\r");
	assert(arduino_nvram_psk_load() == "abracadabra");
	cli_simtype("!psk None\r");
	assert(arduino_nvram_psk_load() == "");

	sprintf(cli_cmd, "!repeater %d\r", repeater);
	cli_simtype(cli_cmd);
	sprintf(cli_cmd, "!callsign %s\r", argv[1]);
	cli_simtype(cli_cmd);
	sprintf(cli_cmd, "!psk %s\r", argv[4]);
	cli_simtype(cli_cmd);

	Net = Ptr<Network>(new Network());
	console_setup(Net);

	cli_simtype("!callsi\bgn\r");
	cli_simtype("!callsj\bign\r");
	cli_simtype("!callsign 5\r");
	cli_simtype("!callsign ABCD\r");
	cli_simtype("!callsign QC\r");
	cli_simtype("!nodebug\r");
	cli_simtype("!ssid bla\r");
	cli_simtype("!ssid 012345678901234567890123456789012345678901234567890123456789012345\r");
	assert(arduino_nvram_load("ssid") == "bla");
	cli_simtype("!ssid\r");
	cli_simtype("!repeater\r");
	cli_simtype("!repeater a\r");
	cli_simtype("!repeater 1\r");
	cli_simtype("!beacon 0\r");
	cli_simtype("!beacon 10\r");
	cli_simtype("!beacon\r");
	cli_simtype("!password ble\r");
	cli_simtype("!password 012345678901234567890123456789012345678901234567890123456789012345\r");
	assert(arduino_nvram_load("password") == "ble");
	cli_simtype("!password\r");
	cli_simtype("!debug\r");
	cli_simtype("A b\r");
	cli_simtype("A $=d\r\r\r");
	cli_simtype("AAAAA:$=d\r");
	cli_simtype("AAAAA " 
		"01234567890123456789012345678901234567890123456789" 
		"01234567890123456789012345678901234567890123456789" 
		"01234567890123456789012345678901234567890123456789" 
		"01234567890123456789012345678901234567890123456789" 
		"01234567890123456789012345678901234567890123456789" 
		"01234567890123456789012345678901234567890123456789" 
		"01234567890123456789012345678901234567890123456789" 
		"01234567890123456789012345678901234567890123456789" 
		"01234567890123456789012345678901234567890123456789" 
		"01234567890123456789012345678901234567890123456789" 
		"01234567890123456789012345678901234567890123456789" 
		"01234567890123456789012345678901234567890123456789" 
		"01234567890123456789012345678901234567890123456789" 
		"\r");
	cli_simtype("!lastid\r");
	cli_simtype("\xff\xff\xff\xf0\x01");
	cli_simtype("\r!wifi\r");
	cli_simtype("\r!help\r");
	cli_simtype("\r!notnc\r");

	logs("test", "test");

	// Add a couple of old data to exercise cleanup run paths
	Net->_recv_log()["UNKNOWN:1234"] = RecvLogItem(-50, -90 * 60 * 1000);
	Net->_neighbors()["UNKNOWN"] = Peer(-50, -90 * 60 * 1000);
	Net->_peers()["UNKNOWN"] = Peer(-50, -90 * 60 * 1000);
	Net->_repeaters()["UNKNOWN"] = Peer(-50, -90 * 60 * 1000);

	// Main loop simulation (in Arduino, would be a busy loop)
	int s = lora_emu_socket();
	int s2 = Serial.emu_listen_socket();

	while (true) {
		if (arduino_random(0, 100) == 0) {
			ping();
		} else if (arduino_random(0, 100) == 0) {
			ping_self();
		} else if (arduino_random(0, 100) == 0) {
			rreq();
		} else if (arduino_random(0, 100) == 0) {
			sendm();
		} else if (arduino_random(0, 1000) == 0) {
			sendm_bad();
		} else if (arduino_random(0, 100) == 0) {
			cli_simtype("!neigh\r");
			cli_simtype("!uptime\r");
		}

		Ptr<Task> tsk = Net->_task_mgr().next_task();

		fd_set set;
		FD_ZERO(&set);
		FD_SET(s, &set);
		if (s2 >= 0) {
			FD_SET(s2, &set);
		}
		// may change because this is the conn socket
		int s3 = Serial.emu_conn_socket();
		if (s3 >= 0) {
			FD_SET(s3, &set);
		}

		struct timeval timeout;
		struct timeval* ptimeout = 0;
		if (tsk) {
			int64_t now = sys_timestamp();
			int64_t to = tsk->next_run() - now;
			// printf("Timeout: %s %ld\n", tsk->get_name().c_str(), to);
			if (to < 0) {
				to = 0;
			}
			// 1-second timeout due to call to console_handle()
			to = 1;
			timeout.tv_sec = to / 1000;
			timeout.tv_usec = (to % 1000) * 1000;
			ptimeout = &timeout;
		} else {
			printf("No timeout = bug\n");
			return 2;
			// timeout.tv_sec = 1;
			// timeout.tv_usec = 0;
		}
		ptimeout = &timeout;

		int sel = select(FD_SETSIZE, &set, NULL, NULL, ptimeout);
		if (sel < 0) {
			perror("select() failure");
			return 1;
		}

		if (FD_ISSET(s, &set)) {
			lora_emu_rx();
		} else {
			Net->run_tasks(sys_timestamp());
		}
		if (s2 >= 0 && FD_ISSET(s2, &set)) {
			Serial.emu_listen_handle();
		}
		if (s3 >= 0 && FD_ISSET(s3, &set)) {
			Serial.emu_conn_handle();
		}
		console_handle();
	}
}
