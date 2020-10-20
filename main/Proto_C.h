/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

// Implementation of packet confirmation (C,CO options)

#ifndef __PROTO_C_H
#define __PROTO_C_H

#include "Protocol.h"

class Proto_C: public Protocol {
public:
	Proto_C(Network* net);
	virtual HandlerResponse handle(const Packet&);

	Proto_C() = delete;
	Proto_C(const Proto_C&) = delete;
	Proto_C(Proto_C&&) = delete;
	Proto_C& operator=(const Proto_C&) = delete;
	Proto_C& operator=(Proto_C&&) = delete;
};

#endif
