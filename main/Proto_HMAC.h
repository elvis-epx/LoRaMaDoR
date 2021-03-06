/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

// Implementation of HMAC signature (H=)

#ifndef __PROTO_HMAC_H
#define __PROTO_HMAC_H

#include "L4Protocol.h"
#include "Buffer.h"

class Proto_HMAC: public L4Protocol {
public:
	Proto_HMAC(Network* net);
	virtual L4txHandlerResponse tx(const Packet&);
	virtual L4rxHandlerResponse rx(const Packet&);

	Proto_HMAC() = delete;
	Proto_HMAC(const Proto_HMAC&) = delete;
	Proto_HMAC(Proto_HMAC&&) = delete;
	Proto_HMAC& operator=(const Proto_HMAC&) = delete;
	Proto_HMAC& operator=(Proto_HMAC&&) = delete;
};

L4txHandlerResponse Proto_HMAC_tx(const Buffer&, const Packet&);
L4rxHandlerResponse Proto_HMAC_rx(const Buffer&, const Packet&);
Buffer Proto_HMAC_hmac(const Buffer& key, const Buffer& data);

#endif
