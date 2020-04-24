#include "Proto_Ping.h"
#include "Network.h"
#include "Packet.h"

Proto_Ping::Proto_Ping(Network *net): Protocol(net)
{
}

Ptr<Packet> Proto_Ping::handle(const Packet& pkt)
{
	// We don't allow broadcast PING to QC or QB because many stations would
	// transmit at the same time, corrupting each other's messages.
	if ((!pkt.to().isQ() || pkt.to().is_localhost()) && pkt.params().has("PING")) {
		Params pong = Params();
		pong.set_ident(net->get_next_pkt_id());
		pong.put_naked("PONG");
		return Ptr<Packet>(new Packet(pkt.from(), net->me(), pong, pkt.msg()));
	}
	return Ptr<Packet>(0);
}
