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

void ping_self()
{
	printf("$$$$$ CLI PING SELF\n");
	cli_simtype("QL:PING payload\r");
	char *cmd;
	asprintf(&cmd, "%s:PING payload\r", Net->me().buf().cold());
	cli_simtype(cmd);
	free(cmd);
}

void ping()
{
	printf("$$$$$ CLI PING\n");
	const Dict<Neighbour> neigh = Net->neigh();
	const Vector<Buffer> neigh_cs = Net->neigh().keys();
	auto n = neigh.count();
	if (!n) {
		return;
	}

	Buffer scs = neigh_cs[arduino_random(0, n)];
	Callsign cs(scs);
	if (!cs.is_valid()) {
		printf("Neigh callsign invalid %s\n", scs.cold());
		exit(2);
	}
	char *cmd;
	asprintf(&cmd, "%s:PING payload\r", scs.cold());
	cli_simtype(cmd);
	free(cmd);
}

void rreq()
{
	printf("$$$$$ CLI RREQ\n");
	const Dict<Neighbour> neigh = Net->neigh();
	const Vector<Buffer> neigh_cs = Net->neigh().keys();
	unsigned int n = neigh.count();
	if (!n) {
		return;
	}
	Buffer scs = neigh_cs[arduino_random(0, n)];
	Callsign cs(scs);
	if (!cs.is_valid()) {
		printf("Neigh callsign invalid %s\n", scs.cold());
		exit(2);
	}
	char *cmd;
	asprintf(&cmd, "%s:RREQ\r", scs.cold());
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

	Net = Ptr<Network>(new Network(Callsign("INV")));
	Net = Ptr<Network>(new Network(cs));
	cli_simtype("!callsi\bgn\r");
	cli_simtype("!callsj\bign\r");
	cli_simtype("!callsign 5\r");
	cli_simtype("!callsign ABCD\r");
	cli_simtype("!callsign QC\r");
	cli_simtype("!nodebug\r");
	cli_simtype("!ssid bla\r");
	cli_simtype("!ssid\r");
	cli_simtype("!password ble\r");
	cli_simtype("!password\r");
	cli_simtype("!reset\r");
	cli_simtype("!restart\r");
	cli_simtype("!debug\r");
	cli_simtype("A b\r");
	cli_simtype("A $=d\r\r\r");
	cli_simtype("AAAAA:$=d\r");
	cli_simtype("AAAAA xsxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\r");
	cli_simtype("!lastid\r");
	cli_simtype("\xff\xff\xff\xf0\x01");
	cli_simtype("\r!wifi\r");
	cli_simtype("\r!help\r");

	// Add a couple of old data to exercise cleanup run paths
	Net->_recv_log()["UNKNOWN:1234"] = RecvLogItem(-50, -90 * 60 * 1000);
	Net->_neighbours()["UNKNOWN"] = Neighbour(-50, -90 * 60 * 1000);

	// Main loop simulation (in Arduino, would be a busy loop)
	int s = lora_emu_socket();

	int x = 2000;
	while (x-- > 0) {
		if (arduino_random(0, 100) == 0) {
			ping();
		} else if (arduino_random(0, 100) == 0) {
			ping_self();
		} else if (arduino_random(0, 100) == 0) {
			rreq();
		} else if (arduino_random(0, 100) == 0) {
			cli_simtype("!neigh\r");
		}

		Ptr<Task> tsk = Net->_task_mgr().next_task();

		fd_set set;
		FD_ZERO(&set);
		FD_SET(s, &set);

		struct timeval timeout;
		struct timeval* ptimeout = 0;
		if (tsk) {
			long int now = arduino_millis();
			long int to = tsk->next_run() - now;
			printf("Timeout: %s %ld\n", tsk->get_name(), to);
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
