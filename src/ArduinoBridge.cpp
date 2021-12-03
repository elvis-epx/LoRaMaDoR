/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

#include <Arduino.h>
#include <stdlib.h>
#include "Buffer.h"
#include "Callsign.h"

// Platform-dependent functions. They are faked on Linux to run tests.

uint32_t _arduino_millis()
{
	return millis();
}

int32_t arduino_random2(int32_t min, int32_t max)
{
	return random(min, max);
}

void arduino_restart() {
	ESP.restart();
}
