/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

// Functions related to low-level LoRa functionality used by Network.

#ifndef __RADIO_H
#define __RADIO_H

#include "Buffer.h"

void lora_start(void (*cb)(const char *buf, size_t plen, int rssi));
bool lora_tx(const Buffer& packet);
uint32_t lora_speed_bps();

#endif
