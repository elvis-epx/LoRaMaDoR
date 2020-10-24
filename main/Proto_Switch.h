/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2020 PU5EPX
 */

// Implementation of remote switch command

#ifndef __PROTO_SWITCH_H
#define __PROTO_SWITCH_H

#include "L7Protocol.h"
#include "Callsign.h"
#include "Dict.h"

struct SwitchTransaction {
	Callsign from;
	Buffer challenge;
	Buffer response;
	uint32_t timeout;
	bool done;
};

class SwitchTimeoutTask;

class Proto_Switch: public L7Protocol {
public:
	Proto_Switch(Network* net);
	virtual L7HandlerResponse handle(const Packet&);

	Proto_Switch() = delete;
	Proto_Switch(const Proto_Switch&) = delete;
	Proto_Switch(Proto_Switch&&) = delete;
	Proto_Switch& operator=(const Proto_Switch&) = delete;
	Proto_Switch& operator=(Proto_Switch&&) = delete;
private:
	friend class SwitchTimeoutTask;
	Dict<SwitchTransaction> transactions;
};

#endif
