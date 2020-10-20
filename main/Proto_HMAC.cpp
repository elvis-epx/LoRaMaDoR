/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

// Implementation of confirmed request (C parameter)

#include "Proto_HMAC.h"
#include "Network.h"
#include "Packet.h"
#include "CLI.h"
#include "sha256.h"

static const char* hex = "0123456789abcdef";

Proto_HMAC::Proto_HMAC(Network *net): L4Protocol(net)
{
}

static Buffer calc_hmac(const Buffer& data)
{
	Sha256 hmac;
	hmac.initHmac((uint8_t*) "abracadabra", 11);
	for (size_t i = 0; i < data.length(); ++i) {
		hmac.write(data.charAt(i));
	}
	uint8_t* res = hmac.resultHmac();
	// convert the first 48 bits of HMAC (6 octets)
	// to 12 'base16' characters
	char b64[13];
	for (size_t i = 0; i < 6; ++i) {
		b64[i*2+0] = hex[(res[i] & 0xf)];
		b64[i*2+1] = hex[((res[i] >> 4) & 0xf)];
	}

	return Buffer(b64, 12);
}

L4rxHandlerResponse Proto_HMAC::rx(const Packet& orig_pkt)
{
	// FIXME implement handling
	// FIXME check if key exists, recover key

	auto p = orig_pkt.params();

	if (p.has("RREQ") || p.has("RRSP")) {
		return L4rxHandlerResponse();
	}

	if (! p.has("H")) {
		logs("HMAC not in packet", "");
		return L4rxHandlerResponse(Ptr<Packet>(0), true);
	}

	Buffer recv_hmac = p.get("H");
	if (recv_hmac.length() != 12) {
		logs("HMAC of invalid size", "");
		return L4rxHandlerResponse(Ptr<Packet>(0), true);
	}

	// recalculate HMAC locally and compare
	auto data = Buffer(orig_pkt.to()) + orig_pkt.from() +
		orig_pkt.params().s_ident() + orig_pkt.msg();
	auto hmac = calc_hmac(data);

	if (hmac != recv_hmac) {
		logs("HMAC: inconsistent", "");
		logs(hmac.c_str(), recv_hmac.c_str());
		return L4rxHandlerResponse(Ptr<Packet>(0), true);
	}

	return L4rxHandlerResponse();
}

L4txHandlerResponse Proto_HMAC::tx(const Packet& orig_pkt)
{
	// FIXME check if key exists, recover key
	auto p = orig_pkt.params();
	if (p.has("RREQ") || p.has("RRSP")) {
		return L4txHandlerResponse();
	}

	auto data = Buffer(orig_pkt.to()) + orig_pkt.from() +
		orig_pkt.params().s_ident() + orig_pkt.msg();
	auto hmac = calc_hmac(data);

	p.put("H", hmac);
	return L4txHandlerResponse(orig_pkt.change_params(p));
}
