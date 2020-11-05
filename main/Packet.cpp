/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

// Class that encapsulates a LoRaMaDoR packet.
// Includes layer-2 and layer-3 parsing and encoding.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Packet.h"
#include "Params.h"
#include "RS-FEC.h"

static const int MSGSIZE_SHORT = 80;
static const int MSGSIZE_LONG = 180;
static const int REDUNDANCY = 20;

char rs_encoded[MSGSIZE_LONG + REDUNDANCY];
char rs_decoded[MSGSIZE_LONG];

RS::ReedSolomon<MSGSIZE_LONG, REDUNDANCY> rs_long;
RS::ReedSolomon<MSGSIZE_SHORT, REDUNDANCY> rs_short;

// Decode packet preamble (except callsigns).
static bool decode_preamble(const char* data, size_t len,
		Callsign &to, Callsign &from, Params& params, int& error)
{
	const char *d1 = (const char*) memchr(data, '<', len);
	const char *d2 = (const char*) memchr(data, ':', len);

	if (d1 == 0 || d2 == 0) {
		error = 100;
		return false;
	} else if (d1 >= d2)  {
		error = 101;
		return false;
	}

	to = Callsign(Buffer(data, d1 - data));
	from = Callsign(Buffer(d1 + 1, d2 - d1 - 1));

	if (!to.is_valid() || !from.is_valid()) {
		error = 104;
		return false;
	}

	const char *sparams = d2 + 1;
	size_t sparams_len = len - (d2 - data) - 1;
	params = Params(Buffer(sparams, sparams_len));

	if (! params.is_valid_with_ident()) {
		error = 105;
		return false;
	}

	return true;
}

Packet::Packet(const Callsign &to, const Callsign &from,
			const Params& params, const Buffer& msg, int rssi): 
			_to(to), _from(from), _params(params), _msg(msg), _rssi(rssi)
{
	_signature = Buffer(_from) + ":" + params.s_ident();
}

Packet::~Packet()
{
}

// Decode packet coming from layer 1.
Ptr<Packet> Packet::decode_l2(const char *data, size_t len, int rssi, int& error)
{
	error = 0;
	if (len <= REDUNDANCY || len > (MSGSIZE_LONG + REDUNDANCY)) {
		error = 999;
		return Ptr<Packet>(0);
	}

	memset(rs_encoded, 0, sizeof(rs_encoded));
	if (len <= (MSGSIZE_SHORT + REDUNDANCY)) {
		memcpy(rs_encoded, data, len - REDUNDANCY);
		memcpy(rs_encoded + MSGSIZE_SHORT, data + len - REDUNDANCY, REDUNDANCY);
		if (rs_short.Decode(rs_encoded, rs_decoded)) {
			error = 998;
			return Ptr<Packet>(0);
		}
		return decode_l3(rs_decoded, len - REDUNDANCY, rssi, error);
	} else {
		memcpy(rs_encoded, data, len - REDUNDANCY);
		memcpy(rs_encoded + MSGSIZE_LONG, data + len - REDUNDANCY, REDUNDANCY);
		if (rs_long.Decode(rs_encoded, rs_decoded)) {
			error = 997;
			return Ptr<Packet>(0);
		}
		return decode_l3(rs_decoded, len - REDUNDANCY, rssi, error);
	}
}

// Decode packet coming from layer 2.
Ptr<Packet> Packet::decode_l3(const char* data, int& error)
{
	return decode_l3(data, strlen(data), -50, error);
}

// Decode packet coming from layer 2.
Ptr<Packet> Packet::decode_l3(const char* data, size_t len, int rssi, int &error)
{
	const char *preamble = 0;
	const char *msg = 0;
	size_t preamble_len = 0;
	size_t msg_len = 0;

	const char *msgd = (const char*) memchr(data, ' ', len);
	
	if (msgd) {
		preamble = data;
		preamble_len = msgd - data;
		msg = msgd + 1;
		msg_len = len - preamble_len - 1;
	} else {
		// valid packet with no message
		preamble = data;
		preamble_len = len;
	}

	Callsign to;
	Callsign from;
	Params params;

	if (! decode_preamble(preamble, preamble_len, to, from, params, error)) {
		return Ptr<Packet>(0);
	}

	return Ptr<Packet>(new Packet(to, from, params, Buffer(msg, msg_len), rssi));
}

// Generate a new packet, based on present packet, with modified message.
Ptr<Packet> Packet::change_msg(const Buffer& msg) const
{
	return Ptr<Packet>(new Packet(this->to(), this->from(), this->params(), msg));
}

// Generate a new packet, based on present packet, with modified parameters.
Ptr<Packet> Packet::change_params(const Params&new_params) const
{
	return Ptr<Packet>(new Packet(this->to(), this->from(), new_params, this->msg()));
}

// Encode a packet in layer 3.
Buffer Packet::encode_l3() const
{
	Buffer b(_to);
	b += '<';
	b += _from;
	b += ':';
	b += _params.serialized();
	b += ' ';
	b += _msg;

	if (b.length() > MSGSIZE_LONG) {
		// FIXME return error or warning
		b = b.substr(0, MSGSIZE_LONG);
	}

	return b;
}

// Encode a packet in level 2, with FEC and ready to be sent.
Buffer Packet::encode_l2() const
{
	Buffer b = encode_l3();

	memset(rs_decoded, 0, sizeof(rs_decoded));
	memcpy(rs_decoded, b.c_str(), b.length());
	if (b.length() <= MSGSIZE_SHORT) {
		rs_short.Encode(rs_decoded, rs_encoded);
		b.append(rs_encoded + MSGSIZE_SHORT, REDUNDANCY);
	} else {
		rs_long.Encode(rs_decoded, rs_encoded);
		b.append(rs_encoded + MSGSIZE_LONG, REDUNDANCY);
	}

	return b;
}

// Packet unique identification (prefix + ID).
Buffer Packet::signature() const
{
	return _signature;
}

// Compares signature with another packet
bool Packet::is_dup(const Packet& other) const
{
	return _signature == other._signature;
}

// Returns destination callsign of this packet.
Callsign Packet::to() const
{
	return _to;
}

// Returns source callsign
Callsign Packet::from() const
{
	return _from;
}

// returns parameters of this packet
const Params Packet::params() const
{
	return _params;
}

// returns the message or payload of this packet.
const Buffer Packet::msg() const
{
	return _msg;
}

// returns the RSSI tagged to this packet upon reception
// (undefined if packet was not received via LoRa.)
int Packet::rssi() const
{
	return _rssi;
}
