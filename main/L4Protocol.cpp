/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

/* Abstract class for transport (intermediate) protocols.
 *
 * A transport protocol class may have:
 *
 * 1) a handler (concrete implementation of a handler()).
 * This method is called when a packet is received by the station,
 * that is, this station is the final destination. This is true
 * even for QC, QB packets (in which this station is not the *only*
 * destination).
 *
 * The class may choose to handle the packet and return a response.
 * Regardless of any handling, all other L4 protocols and some L7
 * protocol may also handle the packet in their own ways.
 * 
 * See Proto_C.cpp (confirm packet) for a concrete example.
 */ 

#include "L4Protocol.h"
#include "Network.h"
#include "Packet.h"

L4rxHandlerResponse::L4rxHandlerResponse(bool has_packet, const Callsign& to,
		const Params& params, const Buffer& msg, bool error,
		const Buffer& error_msg):
	has_packet(has_packet), to(to), params(params), msg(msg),
		error(error), error_msg(error_msg)
{}

L4rxHandlerResponse::L4rxHandlerResponse():
	has_packet(false), to(Callsign()), params(Params()), msg(""),
		error(false), error_msg("")
{}

L4txHandlerResponse::L4txHandlerResponse(Ptr<Packet> pkt):
	pkt(pkt)
{}

L4txHandlerResponse::L4txHandlerResponse():
	pkt(Ptr<Packet>(0))
{}

L4Protocol::L4Protocol(Network *net): net(net)
{
	// Network becomes the owner
	net->add_l4protocol(this);
}

L4txHandlerResponse L4Protocol::tx(const Packet&)
{
	return L4txHandlerResponse();
}

L4Protocol::~L4Protocol()
{
	net = 0;
}
