#include "Protocol.h"
#include "Network.h"
#include "Packet.h"

Protocol::Protocol(Network *net): net(net)
{
	// Network becomes the owner
	net->add_protocol(this);
}

Protocol::~Protocol()
{
	net = 0;
}

Ptr<Packet> Protocol::handle(const Packet&)
{
	// by default, does not handle upon receiving
	return Ptr<Packet>(0);
}

Ptr<Packet> Protocol::modify(const Packet&)
{
	// by default, does not modify upon forwarding
	return Ptr<Packet>(0);
}
