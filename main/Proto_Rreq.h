/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

// Implementation of RREQ protocol responder

#ifndef __PROTO_RREQ_H
#define __PROTO_RREQ_H

#include "L7Protocol.h"

class Proto_Rreq: public L7Protocol {
public:
	Proto_Rreq(Network* net);
	virtual L7HandlerResponse handle(const Packet&);

	Proto_Rreq() = delete;
	Proto_Rreq(const Proto_Rreq&) = delete;
	Proto_Rreq(Proto_Rreq&&) = delete;
	Proto_Rreq& operator=(const Proto_Rreq&) = delete;
	Proto_Rreq& operator=(Proto_Rreq&&) = delete;
};

#endif
