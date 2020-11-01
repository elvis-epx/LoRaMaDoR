#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "Serial.h"
#include "Buffer.h"

// This is an emulation of Arduino Serial module. Connect with Telnet
// to communicate with the "serial" port.

// Do not confuse this testing device with the Telnet implementation in
// main/Telnet.h, which only works in Arduino and independently of the
// serial port (one can be connected to Arduino via serial and via
// Telnet at the same time).

SerialClass Serial;

static int port = 0;
static int listen_socket = -1;
static int conn_socket = -1;
static Buffer readbuf;

int SerialClass::available()
{
	return readbuf.length();
}	

int SerialClass::availableForWrite()
{
	// IMHO no need for write logic because it is just for testing
	return 999;
}	

char SerialClass::read()
{
	char c = 0;
	if (!readbuf.empty()) {
		c = readbuf.charAt(0);
		readbuf.cut(1);
	}
	return c;
}

void SerialClass::write(const uint8_t *data, int len)
{
	if (conn_socket < 0) {
		char *tmp = (char*) calloc(1, len + 1);
		memcpy(tmp, data, len);
		printf("%s", tmp);
		free(tmp);
	} else {
		printf("fake: emu writing to telnet connection\n");
		::write(conn_socket, (const void*) data, len);
	}
}

int SerialClass::emu_conn_socket()
{
	return conn_socket;
}

int SerialClass::emu_listen_socket()
{
	return listen_socket;
}

void SerialClass::emu_conn_handle()
{
	if (conn_socket < 0) return;
	printf("fake: emu reading from telnet connection\n");
	char buf[1500];
	int x = ::read(conn_socket, buf, sizeof(buf));
	if (x <= 0) {
		printf("fake: emu closed telnet connection\n");
		close(conn_socket);
		conn_socket = -1;
		return;
	}
	readbuf += Buffer(buf, x);
}

void SerialClass::emu_listen_handle()
{
	if (listen_socket < 0) return;
	printf("fake: emu accepting telnet connection\n");
	struct sockaddr_in connaddr;
	unsigned int len = sizeof(connaddr);

	if (conn_socket >= 0) {
		printf("fake: emu closed old connection\n");
		close(conn_socket);
	}

	conn_socket = accept(listen_socket, (struct sockaddr*) &connaddr, &len);
	if (conn_socket < 0) {
		printf("fake: emu server acccept failed...\n");
		return;
	}
}

void SerialClass::emu_port(int p)
{
	port = p;
	struct sockaddr_in servaddr;

	listen_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_socket == -1) {
		printf("fake: emu socket creation failed\n");
		exit(1);
	}
	bzero(&servaddr, sizeof(servaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(port);

	int x = 1;
	setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &x, sizeof(x));

	if ((bind(listen_socket, (struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) {
		printf("fake: emu socket bind failed...\n");
		exit(1);
	}

	if ((listen(listen_socket, 5)) != 0) {
		printf("fake: emu Listen failed...\n");
		exit(1);
	}

	signal(SIGPIPE, SIG_IGN);
	printf("fake: emu telnet ok!\n");
}
