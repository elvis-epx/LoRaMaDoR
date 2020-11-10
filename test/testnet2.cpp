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

int main(int argc, char* argv[])
{
	if (argc < 7) {
		printf("Specify, callsign, repeater mode, coverage bitmask, "
			"HMAC PSK, serial emulation port and crypto PSK\n");
		return 1;
	}

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

	sprintf(cli_cmd, "!hmacpsk %s\r", argv[4]);
	cli_simtype(cli_cmd);
	sprintf(cli_cmd, "!cryptopsk %s\r", argv[6]);
	cli_simtype(cli_cmd);
	sprintf(cli_cmd, "!repeater %d\r", repeater);
	cli_simtype(cli_cmd);
	sprintf(cli_cmd, "!callsign %s\r", argv[1]);
	cli_simtype(cli_cmd);

	Net = Ptr<Network>(new Network());
	console_setup(Net);
	cli_simtype("!beacon 30\r");
	cli_simtype("!debug\r");
	cli_simtype("!hmacpsk\r");
	cli_simtype("!cryptopsk\r");
	cli_simtype("!beacon1st 10\r");

	// Main loop simulation (in Arduino, would be a busy loop)
	int s = lora_emu_socket();
	int s2 = Serial.emu_listen_socket();

	while (true) {
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
