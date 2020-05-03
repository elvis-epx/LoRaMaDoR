/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

// Implementation of confirmed request (C parameer)

#include "Proto_C.h"
#include "Network.h"
#include "Packet.h"

Proto_C::Proto_C(Network *net): Protocol(net)
{
}

HandlerResponse Proto_C::handle(const Packet& pkt)
{
	if (! pkt.params().has("C")) {
		// does not request to be confirmed
		return HandlerResponse();
	}
	if (pkt.params().has("CO")) {
		// do not confirm a confirmation
		return HandlerResponse();
	}
	if (pkt.to().is_bcast()) {
		// do not confirm broadcast packets
		return HandlerResponse();
	}

	Params co = Params();
	co.set_ident(net->get_next_pkt_id());
	co.put_naked("CO");
	Buffer msg = Buffer("confirm #") + pkt.params().s_ident();
	auto np = new Packet(pkt.from(), net->me(), co, msg);
	return HandlerResponse(Ptr<Packet>(np), false);
}
