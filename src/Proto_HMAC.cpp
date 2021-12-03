/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

// Implementation of confirmed request (C parameter)

#include "Proto_HMAC.h"
#include "HMACKeys.h"
#include "Network.h"
#include "Packet.h"
#include "LoRa-trans/src/sha256.h"

Proto_HMAC::Proto_HMAC(Network *net): L4Protocol(net)
{
}

L4rxHandlerResponse Proto_HMAC::rx(const Packet& orig_pkt)
{
	Buffer key = HMACKeys::get_key_for(orig_pkt.from());
	if (key.empty()) {
		return L4rxHandlerResponse();
	}
	return Proto_HMAC_rx(key, orig_pkt);
}

L4rxHandlerResponse Proto_HMAC_rx(const Buffer& key, const Packet& orig_pkt)
{
	auto p = orig_pkt.params();

	if (p.has("RREQ") || p.has("RRSP")) {
		return L4rxHandlerResponse();
	}

	if (! p.has("H")) {
		return L4rxHandlerResponse(false, Callsign(), Params(), "",
			true, "Packet w/o HMAC");
	}

	Buffer recv_hmac = p.get("H");
	if (recv_hmac.length() != 12) {
		return L4rxHandlerResponse(false, Callsign(), Params(), "",
			true, "Invalid HMAC size");
	}

	// recalculate HMAC locally and compare
	auto data = Buffer(orig_pkt.to()) + orig_pkt.from() +
		orig_pkt.params().s_ident() + orig_pkt.msg();
	auto hmac = HMACKeys::hmac(key, data);

	if (hmac != recv_hmac) {
		return L4rxHandlerResponse(false, Callsign(), Params(), "",
			true, "Bad HMAC");
	}

	return L4rxHandlerResponse();
}

L4txHandlerResponse Proto_HMAC::tx(const Packet& orig_pkt)
{
	Buffer key = HMACKeys::get_key_for(orig_pkt.from());
	if (key.empty()) {
		return L4txHandlerResponse();
	}
	return Proto_HMAC_tx(key, orig_pkt);
}

L4txHandlerResponse Proto_HMAC_tx(const Buffer& key, const Packet& orig_pkt)
{
	auto p = orig_pkt.params();
	if (p.has("RREQ") || p.has("RRSP")) {
		return L4txHandlerResponse();
	}

	auto data = Buffer(orig_pkt.to()) + orig_pkt.from() +
		orig_pkt.params().s_ident() + orig_pkt.msg();
	auto hmac = HMACKeys::hmac(key, data);

	p.put("H", hmac);
	return L4txHandlerResponse(orig_pkt.change_params(p));
}
