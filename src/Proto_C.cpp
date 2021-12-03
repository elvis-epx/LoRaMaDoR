/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

// Implementation of confirmed request (C parameter)

#include "Proto_C.h"
#include "Network.h"
#include "Packet.h"

Proto_C::Proto_C(Network *net): L4Protocol(net)
{
}

L4rxHandlerResponse Proto_C::rx(const Packet& pkt)
{
	if (! pkt.params().has("C")) {
		// does not request confirmation
		return L4rxHandlerResponse();
	}
	if (pkt.params().has("CO")) {
		// do not confirm a confirmation
		return L4rxHandlerResponse();
	}
	if (pkt.to().is_bcast()) {
		// do not confirm broadcast packets
		return L4rxHandlerResponse();
	}

	Buffer msg = Buffer("confirm ") + pkt.params().s_ident();

	Params co = Params();
	co.put_naked("CO");

	return L4rxHandlerResponse(true, pkt.from(), co, msg, false, "");
}

L4txHandlerResponse Proto_C::tx(const Packet& pkt)
{
	// Do not act upon packets sent with C flag
	return L4txHandlerResponse();
}
