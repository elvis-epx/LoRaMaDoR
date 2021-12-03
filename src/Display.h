/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

// Functions related to OLED display

#ifndef __DISPLAY_H
#define __DISPLAY_H

void oled_init();
void oled_show(const char *ma, const char *mb, const char *mc, const char *md);

#endif
