/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

// Platform-dependent functions. They are faked on Linux to run tests.

#ifndef __ARDUINO_BRIDGE
#define __ARDUINO_BRIDGE

#include <cstdint>

int64_t arduino_millis_nw();
int32_t arduino_random(int32_t min, int32_t max);
void arduino_restart();

#endif
