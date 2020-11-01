/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

// Implementation of RREQ protocol

#include "Modf_Rreq.h"
#include "Network.h"
#include "Packet.h"

Modf_Rreq::Modf_Rreq(Network *net): Modifier(net)
{
}

Ptr<Packet> Modf_Rreq::modify(const Packet& pkt)
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