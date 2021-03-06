/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

// Functions related to low-level LoRa functionality used by Network.

#ifndef __AVR__
#include <SPI.h>
#endif
#include <LoRa.h>
#include "Radio.h"
#include "Config.h"

#ifndef __AVR__

// Pin defintion of WIFI LoRa 32
// HelTec AutoMation 2017 support@heltec.cn 
#define SCK     5    // GPIO5  -- SX127x's SCK
#define MISO    19   // GPIO19 -- SX127x's MISO
#define MOSI    27   // GPIO27 -- SX127x's MOSI

#define SS      18   // GPIO18 -- SX127x's CS
#define RST     14   // GPIO14 -- SX127x's RESET
#define DIO0    26   // GPIO26 -- SX127x's IRQ(Interrupt Request)

#else

#define SS 8
#define RST 4
#define DIO0 7

#endif

uint32_t lora_speed_bps()
{
	uint32_t bps = BWIDTH;
	bps *= SPREAD;
	bps *= 4;
	bps /= CR4SLSH;
	bps /= (1 << SPREAD);
	return bps;
}

static bool setup_lora_common();

#ifndef __AVR__
static bool setup_lora()
{
	SPI.begin(SCK, MISO, MOSI, SS);
	return setup_lora_common();
}
#else
static bool setup_lora()
{
	return setup_lora_common();
}
#endif

static bool setup_lora_common()
{
	LoRa.setPins(SS, RST, DIO0);

	if (!LoRa.begin(BAND)) {
		return false;
	}

	LoRa.setTxPower(POWER, PABOOST);
	LoRa.setSpreadingFactor(SPREAD);
	LoRa.setSignalBandwidth(BWIDTH);
	LoRa.setCodingRate4(CR4SLSH);
	LoRa.disableCrc();

	return true;
}

static const int STATUS_IDLE = 0;
static const int STATUS_RECEIVING = 1;
static const int STATUS_TRANSMITTING = 2;

static int status = STATUS_IDLE;

static char recv_area[255];
static void (*rx_callback)(const char *, size_t, int) = 0;

static void on_receive(int plen)
{
	int rssi = LoRa.packetRssi();
	for (size_t i = 0; i < plen && i < sizeof(recv_area); i++) {
		recv_area[i] = LoRa.read();
	}
	if (rx_callback) {
		rx_callback(recv_area, plen, rssi);
	}
}

void lora_resume_rx()
{
	if (status != STATUS_RECEIVING) {
		status = STATUS_RECEIVING;
		LoRa.receive();
	}
}

// Asynchronous transmission
bool lora_tx(const Buffer& packet)
{
	if (status == STATUS_TRANSMITTING) {
		return false;
	}

	if (! LoRa.beginPacket()) {
		// can only fail if in tx mode, don't touch anything
		return false;
	}

	status = STATUS_TRANSMITTING;
	LoRa.write((uint8_t*) packet.c_str(), packet.length());
	LoRa.endPacket(true);
	return true;
}

void lora_tx_done()
{
	status = STATUS_IDLE;
	lora_resume_rx();
}

void lora_start(void (*cb)(const char *buf, size_t plen, int rssi))
{
	rx_callback = cb;
	setup_lora();
	LoRa.onReceive(on_receive);
	LoRa.onTxDone(lora_tx_done);
	// for some reason, crashes if receives before transmitting
	// lora_resume_rx();
}
