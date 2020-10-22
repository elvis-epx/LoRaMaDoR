/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2020 PU5EPX
 */

// Implementation of remote switch command

#ifndef __PROTO_SWITCH_H
#define __PROTO_SWITCH_H

#include "L7Protocol.h"

class Proto_Switch: public L7Protocol {
public:
	Proto_Switch(Network* net);
	virtual L7HandlerResponse handle(const Packet&);

	Proto_Switch() = delete;
	Proto_Switch(const Proto_Switch&) = delete;
	Proto_Switch(Proto_Switch&&) = delete;
	Proto_Switch& operator=(const Proto_Switch&) = delete;
	Proto_Switch& operator=(Proto_Switch&&) = delete;
};

#endif
