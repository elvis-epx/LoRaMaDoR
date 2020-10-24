/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

// Platform-dependent functions. They are faked on Linux to run tests.

#ifndef __ARDUINO_BRIDGE
#define __ARDUINO_BRIDGE

#include <cstddef>
#include "Pointer.h"
#include "Packet.h"
#include "Callsign.h"

int64_t arduino_millis_nw();
int32_t arduino_random(int32_t min, int32_t max);

uint32_t arduino_nvram_repeater_load();
void arduino_nvram_repeater_save(uint32_t);

uint32_t arduino_nvram_beacon_load();
void arduino_nvram_beacon_save(uint32_t);

uint32_t arduino_nvram_beacon_first_load();
void arduino_nvram_beacon_first_save(uint32_t);

uint32_t arduino_nvram_id_load();
void arduino_nvram_id_save(uint32_t);

Callsign arduino_nvram_callsign_load();
void arduino_nvram_callsign_save(const Callsign&);

Buffer arduino_nvram_psk_load();
void arduino_nvram_psk_save(const Buffer &b);

Buffer arduino_nvram_load(const char *);
void arduino_nvram_save(const char *, const Buffer&);

void arduino_restart();

#endif
