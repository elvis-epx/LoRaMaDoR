/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

// Implementation of R earmarking of forwarded packets

#include "Modf_R.h"
#include "Network.h"
#include "Packet.h"

Modf_R::Modf_R(Network *net): Modifier(net)
{
}

Ptr<Packet> Modf_R::modify(const Packet& pkt)
{
	// earmarks all forwarded packets
	Params new_params = pkt.params();
	new_params.put_naked("R");
	return pkt.change_params(new_params);
}
