/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

#include <Arduino.h>
#include <stdlib.h>
#include "Buffer.h"
#include "Callsign.h"

// Platform-dependent functions. They are faked on Linux to run tests.

static uint32_t epoch = 0;
static uint32_t last_millis = 0;

int64_t arduino_millis_nw()
{
	uint32_t m = millis();
	if (m < last_millis) {
		// wrapped around
		epoch += 1;
	}
	last_millis = m;
	return (((int64_t) epoch) << 32) + m;
}

int32_t arduino_random(int32_t min, int32_t max)
{
	return random(min, max);
}

void arduino_restart() {
	ESP.restart();
}
