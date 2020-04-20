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
static unsigned int nvram_id = 0;
static Callsign nvram_cs("FIXMEE-1");
static bool virgin = true;

unsigned long int arduino_millis()
{
	if (virgin) {
		gettimeofday(&tm_first, 0);
		virgin = false;
		srandom(tm_first.tv_sec + tm_first.tv_usec);
	}
	struct timeval tm;
	gettimeofday(&tm, 0);
	return ((tm.tv_sec * 1000000 + tm.tv_usec)
		- (tm_first.tv_sec * 1000000 + tm_first.tv_usec)) / 1000 + 1;
}

long int arduino_random(long int min, long int max)
{
	if (virgin) {
		gettimeofday(&tm_first, 0);
		virgin = false;
		srandom(tm_first.tv_sec + tm_first.tv_usec);
	}
	return min + random() % (max - min);
}

// Emulation of LoRa APIs, network and radio

#define PORT 6000
#define GROUP "239.0.0.1"

static int sock = -1;

int lora_emu_socket()
{
	return sock;
}

static void setup_lora()
{
	// from https://web.cs.wpi.edu/~claypool/courses/4514-B99/samples/multicast.c
	struct ip_mreq mreq;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		perror("socket");
		exit(1);
	}

	// Self-receive must be enabled because we run multiple instances
	// on the same machine
	int loop = 1;
	if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)) < 0) {
		perror("setsockopt loop");
		exit(1);
	}

	// Allow multiple listeners to the same port
	int optval = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) < 0) {
		perror("setsockopt reuseport");
		exit(1);
	}

	// Enter multicast group
	mreq.imr_multiaddr.s_addr = inet_addr(GROUP);
	mreq.imr_interface.s_addr = INADDR_ANY;
	if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
					&mreq, sizeof(mreq)) < 0) {
		perror("setsockopt mreq");
		exit(1);
	}

	// listen UDP port
	struct sockaddr_in addr;
	memset((char *)&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(PORT);

	if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		perror("bind");
		exit(1);
	}
}

unsigned long int lora_speed_bps()
{
	return 1200;
}

bool lora_tx(const Buffer& b)
{
	if (arduino_random(0, 10) == 0) {
		printf("Simulate send pkt fail\n");
		return false;
	}

	// Send to multicast group & port
	struct sockaddr_in addr;
	memset((char *)&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(GROUP);
	addr.sin_port = htons(PORT);

	int sent = sendto(sock, b.cold(), b.length(), 0, (struct sockaddr *) &addr, sizeof(addr));
	if (sent < 0) {
		perror("sendto");
		exit(1);
	}
	printf("Sent packet\n");
	return 1;
}

static void (*rx_callback)(char const*, unsigned int, int) = 0;

void lora_start(void (*new_cb)(char const*, unsigned int, int))
{
	setup_lora();
	rx_callback = new_cb;
}

void lora_emu_rx()
{
	char message[256];
	struct sockaddr_in from;
	socklen_t fromlen = sizeof(from);
	int rec = recvfrom(sock, message, sizeof(message), 0, (struct sockaddr *) &from, &fromlen);
	if (rec < 0) {
		perror("recvfrom");
		exit(1);
	}
	printf("Received packet\n");
	rx_callback(message, rec, -50);
}

unsigned int arduino_nvram_id_load()
{
	if (nvram_id == 0) {
		nvram_id = arduino_random(1, 9999);
	}
	return nvram_id;
}

void arduino_nvram_id_save(unsigned int id)
{
	nvram_id = id;
}

void serial_print(const char *msg) {
	printf("%s", msg);
}

void serial_print(char msg) {
	printf("%c", msg);
}

void serial_println(const char *msg) {
	printf("%s\n", msg);
}

void serial_println() {
	printf("\n");
}

void arduino_restart() {
	// exit(0);
}

void arduino_nvram_callsign_save(const Callsign &cs)
{
	nvram_cs = cs;
}

void oled_show(const char *, const char *, const char *, const char*)
{
}
