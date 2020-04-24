// LoRaMaDoR (LoRa-based mesh network for hams) project
// Copyright (c) 2020 PU5EPX
// Implementation of R earmarking

#ifndef __PROTO_R_H
#define __PROTO_R_H

#include "Protocol.h"

class Proto_R: public Protocol {
public:
	Proto_R(Network* net);
	virtual Ptr<Packet> modify(const Packet&);

	Proto_R() = delete;
	Proto_R(const Proto_R&) = delete;
	Proto_R(const Proto_R&&) = delete;
	Proto_R& operator=(const Proto_R&) = delete;
	Proto_R& operator=(const Proto_R&&) = delete;
};

#endif
