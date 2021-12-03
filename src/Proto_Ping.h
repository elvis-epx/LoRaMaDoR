/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

// Implementation of PING protocol responder

#ifndef __PROTO_PING_H
#define __PROTO_PING_H

#include "L7Protocol.h"

class Proto_Ping: public L7Protocol {
public:
	Proto_Ping(Network* net);
	virtual L7HandlerResponse handle(const Packet&);

	Proto_Ping() = delete;
	Proto_Ping(const Proto_Ping&) = delete;
	Proto_Ping(Proto_Ping&&) = delete;
	Proto_Ping& operator=(const Proto_Ping&) = delete;
	Proto_Ping& operator=(Proto_Ping&&) = delete;
};

#endif
