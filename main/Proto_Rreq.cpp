/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

// Implementation of RREQ protocol

#include "Proto_Rreq.h"
#include "Network.h"
#include "Packet.h"

Proto_Rreq::Proto_Rreq(Network *net): Protocol(net)
{
}

HandlerResponse Proto_Rreq::handle(const Packet& pkt)
{
	// Respond to RREQ packet
	if (!pkt.to().is_bcast() && pkt.params().has("RREQ")) {
		Buffer msg = pkt.msg();
		msg += '|';
		msg += net->me();
		Params rrsp = Params();
		rrsp.set_ident(net->get_next_pkt_id());
		rrsp.put_naked("RRSP");
		auto np = new Packet(pkt.from(), net->me(), rrsp, msg);
		return HandlerResponse(Ptr<Packet>(np), true);
	}
	return HandlerResponse();
}

Ptr<Packet> Proto_Rreq::modify(const Packet& pkt)
{
	// Add ourselves to chain in forwarded RREQ and RRSP pkts
	if (! pkt.to().is_q()) {
		// not QB, QC, etc.
		if (pkt.params().has("RREQ") || pkt.params().has("RRSP")) {
			Buffer new_msg = pkt.msg();
			new_msg += '>';
			new_msg += net->me();
			return pkt.change_msg(new_msg);
		}
	}
	return Ptr<Packet>(0);
}
