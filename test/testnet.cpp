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
#include "CLI.h"

// Radio emulation hooks in FakeArduino.cpp
int lora_emu_socket();
void lora_emu_rx();

Ptr<Network> Net(0);

void ping()
{
	printf("$$$$$ CLI PING\n");
	unsigned int n = Net->neighbours.count();
	if (!n) {
		return;
	}
	const char *scs = Net->neighbours.keys()[arduino_random(0, n)].cold();
	Callsign cs(scs);
	if (!cs.is_valid()) {
		printf("Neigh callsign invalid %s\n", scs);
		exit(2);
	}
	char *cmd;
	asprintf(&cmd, "%s:PING payload\r", scs);
	cli_simtype(cmd);
	free(cmd);
}

void rreq()
{
	printf("$$$$$ CLI RREQ\n");
	unsigned int n = Net->neighbours.count();
	if (!n) {
		return;
	}
	const char *scs = Net->neighbours.keys()[arduino_random(0, n)].cold();
	Callsign cs(scs);
	if (!cs.is_valid()) {
		printf("Neigh callsign invalid %s\n", scs);
		exit(2);
	}
	char *cmd;
	asprintf(&cmd, "%s:RREQ\r", scs);
	cli_simtype(cmd);
	free(cmd);
}

int main(int argc, char* argv[])
{
	if (argc < 2) {
		printf("Specify a callsign\n");
		return 1;
	}

	Callsign cs(argv[1]);
	if (!cs.is_valid()) {
		printf("Callsign invalid.\n");
		return 2;
	}

	Net = Ptr<Network>(new Network(cs));
	cli_simtype("!callsi\bgn\r");
	cli_simtype("!callsj\bign\r");
	cli_simtype("!callsign 5\r");
	cli_simtype("!callsign ABCD\r");
	cli_simtype("!debug\r");

	// Main loop simulation (in Arduino, would be a busy loop)
	int s = lora_emu_socket();

	int x = 1000;
	while (x-- > 0) {
		if (arduino_random(0, 100) == 0) {
			ping();
		}
		if (arduino_random(0, 100) == 0) {
			rreq();
		}

		Ptr<Task> tsk = Net->task_mgr.next_task();

		fd_set set;
		FD_ZERO(&set);
		FD_SET(s, &set);

		struct timeval timeout;
		struct timeval* ptimeout = 0;
		if (tsk) {
			long int now = arduino_millis();
			long int to = tsk->next_run() - now;
			// printf("Timeout: %s %ld\n", tsk->get_name(), to);
			if (to < 0) {
				to = 0;
			}
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

		int sel = select(s + 1, &set, NULL, NULL, ptimeout);
		if (sel < 0) {
			perror("select() failure");
			return 1;
		}

		if (FD_ISSET(s, &set)) {
			lora_emu_rx();
		} else {
			Net->run_tasks(arduino_millis());
		}
	}
}
