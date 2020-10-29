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
 * the server may use this challenge as transaction ID. Minimum challenge
 * size is 8 chars.
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
 * Target and value are numbers from 0 to 65535.
 * Target must be a non-zero number.
 *
 * This packet can also query the status of a switch, replacing 'value'
 * with "?".
 *
 * The command cannot be replay-attacked since the challenge/response pair
 * is spent as soon as the command is received. The only thing the attacker
 * can get is the idempotent response (packet D), during the time window
 * the server keeps the transaction pair in memory.
 * 
 * Type D (server -> client): "D,challenge,response,target,value"
 *
 * Confirmation for the client. Value can be "?" if the target number
 * was invalid (e.g. switch 4 if there are only 3 switches).
 */

#include "Proto_Switch.h"
#include "Network.h"
#include "Packet.h"
#include "CLI.h"
#include "ArduinoBridge.h"
#include "Timestamp.h"

// TODO replace by real hardware switches
static const int sw_count = 3;
static int sw[] = {0, 0, 0};

// Task to handle timeout of ongoing transactions

class SwitchTimeoutTask: public Task
{
public:
	SwitchTimeoutTask(Proto_Switch *p, int64_t offset):
		Task("switch", offset), p(p)
	{
	}
	~SwitchTimeoutTask() {
		p = 0;
	}
protected:
	virtual int64_t run2(int64_t now)
	{
		p->process_timeouts(now);
		return 10 * SECONDS;
	}
private:
	Proto_Switch *p;
};

Proto_Switch::Proto_Switch(Network *net): L7Protocol(net)
{
	net->schedule(new SwitchTimeoutTask(this, 10 * SECONDS));
} 

static bool parse(Buffer msg, char &type, Buffer &challenge,
			Buffer &response, int &target, int &value,
			Buffer &err)
{
	int pos = msg.indexOf(',');
	if (pos != 1) {
		err = "type should be a single char";
		return false;
	}
	type = (char) msg.charAt(0);
	if ((type != 'A') && (type != 'C')) {
		err = "invalid type";
		return false;
	}
	msg.cut(2);

	pos = msg.indexOf(',');
	if (type == 'A') {
		if (pos >= 0) {
			err = "packet A should have a single field";
			return false;
		}
		if (msg.length() < 8) {
			err = "challenge must have at least 8 chars";
			return false;
		}
		challenge = msg;
		return true;
	}

	// type C from this point on
	if (pos < 8) {
		err = "challenge should be at least 8 chars long";
		return false;
	}
	challenge = msg.substr(0, pos);
	msg.cut(pos + 1);

	pos = msg.indexOf(',');
	if (pos < 8) {
		err = "response should be at least 8 chars long";
		return false;
	}
	response = msg.substr(0, pos);
	msg.cut(pos + 1);

	pos = msg.indexOf(',');
	if (pos <= 0) {
		err = "target number not found";
		return false;
	}
	target = msg.substr(0, pos).toInt();
	if (target < 1 || target > 65535) {
		err = "target number should be 1..65535";
		return false;
	}
	msg.cut(pos + 1);

	Buffer svalue;
	pos = msg.indexOf(',');
	if (pos < 0) {
		// all that's left is the value
		svalue = msg;
	} else if (pos == 0) {
		// empty value, tolerate
		value = 0;
	} else {
		// ignore further fields
		svalue = msg.substr(0, pos);
	}

	if (svalue == "?") {
		// query
		value = -1;
	} else {
		// set
		value = svalue.toInt();
		if (value < 1 || value > 65535) {
			err = "value number should be 0..65535";
			return false;
		}
	}

	return true;
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

	char type;
	Buffer challenge, response, err;
	int target, value;

	if (!parse(pkt.msg(), type, challenge, response, target, value, err)) {
		logs("SW packet parsing error", err);
		return L7HandlerResponse();
	}

	Buffer key = Buffer(pkt.from()) + challenge;

	if (type == 'A') {
		// got challenge
		if (transactions.has(key)) {
			// return the same response
			response = transactions[key].response;
		} else {
			// generate response token and add transaction to table
			response = Network::gen_random_token(8);
			auto trans = SwitchTransaction();
			trans.from = pkt.from();
			trans.challenge = challenge;
			trans.response = response;
			trans.timeout = sys_timestamp() + 60 * SECONDS;
			trans.done = false;
			transactions[key] = trans;
		}

		// send packet B
		Params swb = Params();
		swb.put_naked("SW");
		Buffer msg = Buffer("B,") + challenge + "," + response;
		return L7HandlerResponse(true, pkt.from(), swb, msg);

	} else if (type == 'C') {
		// got challenge + response + command
		if (! transactions.has(key)) {
			logs("SW type C unknown challenge", "");
			return L7HandlerResponse();
		}
		auto trans = transactions[key];

		if (trans.response != response) {
			logs("SW type C mismatched response", "");
			return L7HandlerResponse();
		}

		Buffer svalue;

		if (target <= sw_count) {
			if (value >= 0) {
				// set switch
				if (!trans.done) {
					// TODO apply command to hardware switch
					sw[target - 1] = value;
				}
			} else {
				// query switch
				// TODO get value from hardware switch
				value = sw[target - 1];
			}
			svalue = Buffer::itoa(value);
		} else {
			// unknown target
			svalue = "?";
		}

		trans.done = true;
		trans.timeout = sys_timestamp() + 60 * SECONDS;

		// send packet D
		Params swd = Params();
		swd.put_naked("SW");
		Buffer msg = Buffer("D,") + challenge + "," + response + "," +
				Buffer::itoa(target) + "," + svalue;
		return L7HandlerResponse(true, pkt.from(), swd, msg);
	}

	return L7HandlerResponse();
}

void Proto_Switch::process_timeouts(int64_t now)
{
	Vector<Buffer> remove_list;

	for (size_t i = 0; i < transactions.keys().size(); ++i) {
		const Buffer key = transactions.keys()[i];
		const auto trans = transactions[key];
		if (trans.timeout < now) {
			remove_list.push_back(key);
		}
	}

	for (size_t i = 0; i < remove_list.size(); ++i) {
		transactions.remove(remove_list[i]);
	}
}
