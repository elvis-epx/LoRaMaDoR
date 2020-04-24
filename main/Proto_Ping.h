// LoRaMaDoR (LoRa-based mesh network for hams) project
// Copyright (c) 2020 PU5EPX
// Implementation of PING protocol

#ifndef __PROTO_PING_H
#define __PROTO_PING_H

#include "Protocol.h"

class Proto_Ping: public Protocol {
public:
	Proto_Ping(Network* net);
	virtual Ptr<Packet> handle(const Packet&);

	Proto_Ping() = delete;
	Proto_Ping(const Proto_Ping&) = delete;
	Proto_Ping(Proto_Ping&&) = delete;
	Proto_Ping& operator=(const Proto_Ping&) = delete;
	Proto_Ping& operator=(Proto_Ping&&) = delete;
};

#endif
