/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

/*
 * Implementation of Switch protocol (server side)
 *
 * This class implements the server side of the SW protocol to control remote
 * devices, mainly switches. The protocol is confirmed and resistant to replay
 * attacks. HMAC packet signing is required since the replay attack mitigation
 * depends on authenticated messages.
 *
 * Though the protocol is confirmed, the initiative of retransmission lies on
 * the client. The only support at server side for retransmission is to be
 * idempotent.
 *
 * Setting a switch involves exchanging four packet types: A, B, C and D. The
 * payload * in any case is a comma-separate list of values.
 * 
 * Type A (client -> server): "A,challenge"
 * 
 * The challenge should be a random string (or number). The client must
 * guarantee that different transactions have different challenges, since
 * the server may use this challenge as transaction ID.
 *
 * The server knows the challenge came from the client station since the
 * packet is HMAC-signed.
 * 
 * Type B (server -> client): "B,challenge,response"
 *
 * The server sends back the challenge from A plus a random response string
 * (or number). The server stores the challenge/response pair for some time.
 *
 * If the server receives a packet A with a known challenge, 
 * it is treated as a retransmission of the same transaction and the same
 * response is sent back in packet B.
 * 
 * The client knows this packet came from the server since it is HMAC-signed
 * and the challenge matches the value sent in packet A. A replay attack would
 * stop here since the challenge would not be recognized by the client.
 *
 * Type C (client -> server): "C,challenge,response,target,value" 
 * 
 * This packet is the actual command to the underlying hardware. It is accepted
 * only if the server knows the challenge/response pair. The server keeps this
 * pair in memory for a limited time after the command to handle any client
 * retransmissions idempotently.
 *
 * The command cannot be replay-attacked since the challenge/response pair
 * is spent as soon as the command is received. The only thing the attacker
 * can get is the idempotent response (packet D), during the time window
 * the server keeps the transaction pair in memory.
 * 
 * Type D (server -> client): "D,challenge,response,target,value"
 *
 * Confirmation for the client. 
 */
 


#include "Proto_Switch.h"
#include "Network.h"
#include "Packet.h"

namespace {
	struct Req {
		Callsign from;
		Buffer challenge;
		Buffer response;
		int age;
		bool applied;
	};
};

// FIXME Task subclass

Proto_Switch::Proto_Switch(Network *net): L7Protocol(net)
{
	// FIXME list of ongoing requests
	// FIXME start handler of ongoing requests
}

L7HandlerResponse Proto_Switch::handle(const Packet& pkt)
{
	if (pkt.to().is_bcast() || !pkt.params().has("SW")) {
		return L7HandlerResponse();
	}

	if (!pkt.params().has("H")) {
		logs("SW demands HMAC keys are configured", "");
		return L7HandlerResponse();
	}

	Buffer type, challenge, response;

	if (!parse(pkt.msg(), &type, &challenge, &response, &target, &value)) {
		logs("SW packet parsing error", "");
		return L7HandlerResponse();
	}

	if (type == "A") {
		// got challenge
		if (station_challenge_in_list) {
			// reset age
			// recover response
		} else {
			// create item in list
			// FIXME generate response
			response = "3";
		}

		// send packet B
		Params swb = Params();
		swb.put_naked("SW");
		msg = Buffer("B,") + challenge + "," + response;
		return L7HandlerResponse(true, pkt.from(), swb, msg);

	} else if (type == "C") {
		// got challenge + response + command
		if (!station_challenge_in_list) {
			logs("SW type C unknown challenge", "");
			return L7HandlerResponse();
		{

		if (!station_response_tallies) {
			logs("SW type C mismatched response", "");
			return L7HandlerResponse();
		}

		if (!applied) {
			// apply command in hardware
		}
		applied = true;

		// send packet D
		Params swd = Params();
		swd.put_naked("SW");
		msg = Buffer("D,") + challenge + "," + response + "," + target + "," + value;
		return L7HandlerResponse(true, pkt.from(), swd, msg);
	}

	return L7HandlerResponse();
}
