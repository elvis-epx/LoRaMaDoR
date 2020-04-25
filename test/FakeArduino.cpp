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
static uint32_t nvram_id = 0;
static Callsign nvram_cs("FIXMEE-1");
static bool virgin = true;
static Dict<Buffer> nvram;

static void init_things()
{
	virgin = false;
	gettimeofday(&tm_first, 0);
	srandom(tm_first.tv_sec + tm_first.tv_usec);
}

int32_t arduino_millis()
{
	if (virgin) init_things();
	struct timeval tm;
	gettimeofday(&tm, 0);
	return ((tm.tv_sec * 1000000 + tm.tv_usec)
		- (tm_first.tv_sec * 1000000 + tm_first.tv_usec)) / 1000 + 1;
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

uint32_t lora_speed_bps()
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

static void (*rx_callback)(char const*, size_t, int) = 0;

void lora_start(void (*new_cb)(char const*, size_t, int))
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
	if (arduino_random(0, 3) == 0) {
		printf("Received packet, corrupt it a little\n");
		for (int i = 0; i < 3; ++i) {
			message[arduino_random(0, rec)] = arduino_random(0, 256);
		}
	} else if (arduino_random(0, 100) == 0) {
		printf("Received packet, corrupt it a lot\n");
		for (int i = 0; i < 30; ++i) {
			message[arduino_random(0, rec)] = arduino_random(0, 256);
		}
	} else {
		printf("Received packet, not corrupting\n");
	}

	rx_callback(message, rec, -50);
}

uint32_t arduino_nvram_id_load()
{
	if (nvram_id == 0) {
		nvram_id = arduino_random(9900, 10000);
	}
	return nvram_id;
}

void arduino_nvram_id_save(uint32_t id)
{
	nvram_id = id;
}

void console_print(const Buffer &b) {
	printf("%s", b.cold());
}

void console_print(const char *msg) {
	printf("%s", msg);
}

void console_print(char msg) {
	printf("%c", msg);
}

void console_println(const Buffer& b) {
	printf("%s\n", b.cold());
}

void console_println(const char *msg) {
	printf("%s\n", msg);
}

void console_println() {
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

void arduino_nvram_save(const char *key, const Buffer& value)
{
	printf("key %s value %s\n", key, value.cold());
	nvram[key] = value;
}

Buffer arduino_nvram_load(const char *key)
{
	if (!nvram.has(key)) {
		nvram[key] = "None";
	}
	return nvram[key];
}

Buffer get_wifi_status()
{
	return "Fake Wi-Fi status";
}
