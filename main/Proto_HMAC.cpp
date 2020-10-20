/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

// Implementation of confirmed request (C parameter)

#include "Proto_HMAC.h"
#include "Network.h"
#include "Packet.h"
#include "sha256.h"

static const char* hex = "0123456789abcdef";

Proto_HMAC::Proto_HMAC(Network *net): L4Protocol(net)
{
}

L4rxHandlerResponse Proto_HMAC::rx(const Packet& pkt)
{
	// FIXME implement handling
	return L4rxHandlerResponse();
}

L4txHandlerResponse Proto_HMAC::tx(const Packet& pkt)
{
	// FIXME check if key exists, recover key

	Sha256 hmac;
	hmac.initHmac((uint8_t*) "abracadabra", 11);
	Buffer l3 = pkt.encode_l3();
	for (size_t i = 0; i < l3.length(); ++i) {
		hmac.write(l3.charAt(i));
	}
	uint8_t* res = hmac.resultHmac();
	// convert the first 48 bits of HMAC (6 octets)
	// to 12 'base16' characters
	char b64[12];
	for (size_t i = 0; i < 6; ++i) {
		b64[i*2+0] = hex[(res[i] & 0xf)];
		b64[i*2+1] = hex[((res[i] >> 4) & 0xf)];
	}

	Params new_params = pkt.params();
	new_params.put("H", Buffer(b64, 12));
	auto new_pkt = pkt.change_params(new_params);
	return L4txHandlerResponse(new_pkt);
}
