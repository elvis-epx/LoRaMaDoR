#include "Proto_R.h"
#include "Network.h"
#include "Packet.h"

Proto_R::Proto_R(Network *net): Protocol(net)
{
}

Ptr<Packet> Proto_R::modify(const Packet& pkt)
{
	// earmarks all forwarded packets
	Params new_params = pkt.params();
	new_params.put_naked("R");
	return pkt.change_params(new_params);
}
