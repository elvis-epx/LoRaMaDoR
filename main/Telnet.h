/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

// Functions related to Wi-Fi and Telnet server support

#ifndef __TELNET_H
#define __TELNET_H

class Network;

void wifi_setup(Ptr<Network>);
void wifi_handle();
void telnet_print(const char *);
Buffer get_wifi_status();

#endif
