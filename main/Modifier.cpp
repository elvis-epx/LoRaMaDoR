/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

/* Abstract class for modifier application protocols.
 *
 * A modifier (concrete impl. of modify()). This method is called
 * when a packet is to be forwarded by the station, that is, this
 * station is not the *sole* final destination (therefore, QC and QB
 * packets will be offered to modify()).
 *
 * The modifier may return a modified packet, or 0 to pass it on.
 * More than one Modifier can modify a packet. The order is not
 * guaranteed, so implementations must take care not to disrupt
 * other protocols' work.
 *
 * See Modf_Rreq.cpp for a concrete example. This class adds our
 * callsign to the payload of a forwarded RREQ or RRSP packet
 * (and also handles a RREQ request).
 */ 

#include "Modifier.h"
#include "Network.h"
#include "Packet.h"

Modifier::Modifier(Network *net): net(net)
{
	// Network becomes the owner
	net->add_modifier(this);
}

Modifier::~Modifier()
{
	net = 0;
}
