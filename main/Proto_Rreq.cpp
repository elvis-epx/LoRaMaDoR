/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

// Implementation of RREQ protocol

#include "Proto_Rreq.h"
#include "Network.h"
#include "Packet.h"

Proto_Rreq::Proto_Rreq(Network *net): L7Protocol(net)
{
}

L7HandlerResponse Proto_Rreq::handle(const Packet& pkt)
{
	// Respond to RREQ packet
	if (!pkt.to().is_bcast() && pkt.params().has("RREQ")) {
		Buffer msg = pkt.msg();
		msg += (msg.empty() ? "*" : " *");
		msg += net->me();
		msg += ' ';
		msg += Buffer::itoa(pkt.rssi());

		Params rrsp = Params();
		rrsp.put_naked("RRSP");

		return L7HandlerResponse(true, pkt.from(), rrsp, msg);
	}
	return L7HandlerResponse();
}
