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

L4HandlerResponse::L4HandlerResponse(Ptr<Packet> pkt, bool error):
	pkt(pkt), error(error)
{}

L4HandlerResponse::L4HandlerResponse():
	pkt(Ptr<Packet>(0)), error(false)
{}

L4Protocol::L4Protocol(Network *net): net(net)
{
	// Network becomes the owner
	net->add_l4protocol(this);
}

L4Protocol::~L4Protocol()
{
	net = 0;
}
