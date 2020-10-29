// Emulation of certain Arduino APIs for testing on UNIX

#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Buffer.h"
#include "Pointer.h"
#include "Packet.h"

// Emulation of millis() and random()

static struct timeval tm_first;
static bool virgin = true;

static void init_things()
{
	virgin = false;
	gettimeofday(&tm_first, 0);
	srandom(tm_first.tv_sec + tm_first.tv_usec);
}

uint32_t _arduino_millis()
{
	if (virgin) init_things();
	struct timeval tm;
	gettimeofday(&tm, 0);
	int64_t now_us   = tm.tv_sec       * 1000000LL + tm.tv_usec;
	int64_t start_us = tm_first.tv_sec * 1000000LL + tm_first.tv_usec;
	return (now_us - start_us) / 1000 + 1;
}

int32_t arduino_random(int32_t min, int32_t max)
{
	if (virgin) init_things();
	return min + random() % (max - min);
}

// Emulation of LoRa APIs, network and radio

#define PORT 6000
#define GROUP "239.0.0.1"

static int sock = -1;
static int coverage = 0;

int lora_emu_socket()
{
	return sock;
}

// simulate different coverage areas
// e.g. packet sent to coverage 0x01 is seen by stations
// with coverage 0x03 but not by stations with cov. 0x02
void lora_emu_socket_coverage(int c)
{
	coverage = c;
}

static void setup_lora()
{
	// from https://web.cs.wpi.edu/~claypool/courses/4514-B99/samples/multicast.c
	struct ip_mreq mreq;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		perror("fake: socket");
		exit(1);
	}

	// Self-receive must be enabled because we run multiple instances
	// on the same machine
	int loop = 1;
	if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)) < 0) {
		perror("fake: setsockopt loop");
		exit(1);
	}

	// Allow multiple listeners to the same port
	int optval = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) < 0) {
		perror("fake: setsockopt reuseport");
		exit(1);
	}

	// Enter multicast group
	mreq.imr_multiaddr.s_addr = inet_addr(GROUP);
	mreq.imr_interface.s_addr = INADDR_ANY;
	if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
					&mreq, sizeof(mreq)) < 0) {
		perror("fake: setsockopt mreq");
		exit(1);
	}

	// listen UDP port
	struct sockaddr_in addr;
	memset((char *)&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(PORT);

	if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		perror("fake: bind");
		exit(1);
	}
}

uint32_t lora_speed_bps()
{
	return 1200;
}

bool lora_tx(const Buffer& b)
{
	if (arduino_random(0, 10) == 0) {
		printf("fake: Simulate send pkt fail\n");
		return false;
	}

	// Send to multicast group & port
	struct sockaddr_in addr;
	memset((char *)&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(GROUP);
	addr.sin_port = htons(PORT);

	// add coverage bitmask
	Buffer c(b.length() + 1);
	*c.hot(0) = (char) coverage;
	memcpy(c.hot(1), b.c_str(), b.length());

	int sent = sendto(sock, c.c_str(), c.length(), 0, (struct sockaddr *) &addr, sizeof(addr));
	if (sent < 0) {
		perror("fake: sendto");
		exit(1);
	}
	printf("fake: Sent packet\n");
	return 1;
}

static void (*rx_callback)(char const*, size_t, int) = 0;

void lora_start(void (*new_cb)(char const*, size_t, int))
{
	setup_lora();
	rx_callback = new_cb;
}

void lora_emu_rx()
{
	char rawmsg[257];
	char *msg = rawmsg + 1;
	struct sockaddr_in from;
	socklen_t fromlen = sizeof(from);
	int len = recvfrom(sock, rawmsg, sizeof(rawmsg), 0, (struct sockaddr *) &from, &fromlen);
	if (len < 0) {
		perror("fake: recvfrom");
		exit(1);
	}
	printf("fake: Packet received, coverage %d\n", rawmsg[0]);
	if (! (coverage & rawmsg[0])) {
		printf("fake: Discarding packet out of simulated coverage.\n");
		return;
	}

	len -= 1;
	if (arduino_random(0, 3) == 0) {
		printf("fake: Received packet, corrupt it a little\n");
		for (int i = 0; i < 3; ++i) {
			msg[arduino_random(0, len)] = arduino_random(0, 256);
		}
	} else if (arduino_random(0, 100) == 0) {
		printf("fake: Received packet, corrupt it a lot\n");
		for (int i = 0; i < 30; ++i) {
			msg[arduino_random(0, len)] = arduino_random(0, 256);
		}
	} else {
		printf("fake: Received packet, not corrupting\n");
	}

	rx_callback(msg, len, -50);
}

void arduino_restart() {
	exit(0);
}

void oled_show(const char *, const char *, const char *, const char*)
{
}

Buffer get_wifi_status()
{
	return "Fake Wi-Fi status";
}

void telnet_print(const char *)
{
	// dummy; no Telnet mode in desktop testing mode
}
