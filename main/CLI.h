/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

// Command-line interface implementation.

#ifndef __CLI_H
#define __CLI_H

void logs(const char*, const char*);
void logs(const char*, const Buffer&);
void logi(const char*, int32_t);
void app_recv(Ptr<Packet>);
void cli_type(const char);
void cli_simtype(const char *);

#endif
