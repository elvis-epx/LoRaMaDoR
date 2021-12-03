/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2020 PU5EPX
 */

#include "ArduinoBridge.h"
#include "Timestamp.h"

static uint32_t epoch = 0;
static uint32_t last_millis = 0;

int64_t sys_timestamp()
{
	uint32_t m = _arduino_millis();
	if ((m < last_millis) && (last_millis - m) > 0x10000000) {
		// wrapped around
		epoch += 1;
	}
	last_millis = m;
	return (((int64_t) epoch) << 32) + m;
}
