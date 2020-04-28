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
	if ((!pkt.to().isQ() || pkt.to().is_localhost()) && pkt.params().has("RREQ")) {
		Buffer msg = pkt.msg();
		msg.append('|');
		msg.append_str(net->me().buf());
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
	if (! pkt.to().isQ()) {
		// not QB, QC, etc.
		if (pkt.params().has("RREQ") || pkt.params().has("RRSP")) {
			Buffer new_msg = pkt.msg();
			new_msg.append('>');
			new_msg.append_str(net->me().buf());
			return pkt.change_msg(new_msg);
		}
	}
	return Ptr<Packet>(0);
}
