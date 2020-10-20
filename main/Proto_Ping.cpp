/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

// Implementation of PING protocol

#include "Proto_Ping.h"
#include "Network.h"
#include "Packet.h"

Proto_Ping::Proto_Ping(Network *net): L7Protocol(net)
{
}

L7HandlerResponse Proto_Ping::handle(const Packet& pkt)
{
	// We don't allow broadcast PING to QC or QB because many stations would
	// transmit at the same time, corrupting each other's messages.
	if (!pkt.to().is_bcast() && pkt.params().has("PING")) {
		Params pong = Params();
		pong.set_ident(net->get_next_pkt_id());
		pong.put_naked("PONG");
		auto np = new Packet(pkt.from(), net->me(), pong, pkt.msg());
		return L7HandlerResponse(Ptr<Packet>(np), true);
	}

	return L7HandlerResponse();
}
