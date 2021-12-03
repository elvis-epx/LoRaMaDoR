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
#include "CryptoKeys.h"

static const int MSGSIZ_SHORT = 50;
static const int MSGSIZ_MEDIUM = 100;
static const int MSGSIZ_LONG = 200;
static const int REDUNDANCY_SHORT = 10;
static const int REDUNDANCY_MEDIUM = 14;
static const int REDUNDANCY_LONG = 20;

char rs_encoded[MSGSIZ_LONG + REDUNDANCY_LONG];
char rs_decoded[MSGSIZ_LONG];

RS::ReedSolomon<MSGSIZ_SHORT, REDUNDANCY_SHORT> rsf_short;
RS::ReedSolomon<MSGSIZ_MEDIUM, REDUNDANCY_MEDIUM> rsf_medium;
RS::ReedSolomon<MSGSIZ_LONG, REDUNDANCY_LONG> rsf_long;

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
			_to(to), _from(from), _params(params), _msg(msg), _rssi(rssi),
			_was_encrypted(false)
{
	_signature = Buffer(_from) + ":" + params.s_ident();
}

Packet::~Packet()
{
}

static Ptr<Packet> decode_l2b(const char* data, size_t len, int rssi, int &error,
				bool maybe_encrypted)
{
	if (!maybe_encrypted) {
		return Packet::decode_l3(data, len, rssi, error, false);
	}

	char *udata;
	size_t ulen;
	Ptr<Packet> p;
	int decrypt_res = CryptoKeys::decrypt(data, len, &udata, &ulen);

	if (decrypt_res == CryptoKeys::OK_DECRYPTED) {
		p = Packet::decode_l3(udata, ulen, rssi, error, true);
		::free(udata);
	} else if (decrypt_res == CryptoKeys::OK_CLEARTEXT) {
		p = Packet::decode_l3(data, len, rssi, error, false);
	} else if (decrypt_res == CryptoKeys::ERR_NOT_ENCRYPTED) {
		error = 1900;
	} else if (decrypt_res == CryptoKeys::ERR_ENCRYPTED) {
		error = 1901;
	} else if (decrypt_res == CryptoKeys::ERR_DECRIPTION) {
		error = 1902;
	}

	return p;
}

// Decode packet coming from layer 1.
Ptr<Packet> Packet::decode_l2(const char *data, size_t len, int rssi, int& error, bool maybe_encrypted)
{
	error = 0;
	if (len <= REDUNDANCY_SHORT || len > (MSGSIZ_LONG + REDUNDANCY_LONG)) {
		error = 999;
		return Ptr<Packet>(0);
	}

	memset(rs_encoded, 0, sizeof(rs_encoded));
	if (len <= (MSGSIZ_SHORT + REDUNDANCY_SHORT)) {
		memcpy(rs_encoded, data, len - REDUNDANCY_SHORT);
		memcpy(rs_encoded + MSGSIZ_SHORT, data + len - REDUNDANCY_SHORT, REDUNDANCY_SHORT);
		if (rsf_short.Decode(rs_encoded, rs_decoded)) {
			error = 998;
			return Ptr<Packet>(0);
		}
		return decode_l2b(rs_decoded, len - REDUNDANCY_SHORT, rssi, error, maybe_encrypted);
	} else if (len <= (MSGSIZ_MEDIUM + REDUNDANCY_MEDIUM)) {
		memcpy(rs_encoded, data, len - REDUNDANCY_MEDIUM);
		memcpy(rs_encoded + MSGSIZ_MEDIUM, data + len - REDUNDANCY_MEDIUM, REDUNDANCY_MEDIUM);
		if (rsf_medium.Decode(rs_encoded, rs_decoded)) {
			error = 998;
			return Ptr<Packet>(0);
		}
		return decode_l2b(rs_decoded, len - REDUNDANCY_MEDIUM, rssi, error, maybe_encrypted);
	} else {
		memcpy(rs_encoded, data, len - REDUNDANCY_LONG);
		memcpy(rs_encoded + MSGSIZ_LONG, data + len - REDUNDANCY_LONG, REDUNDANCY_LONG);
		if (rsf_long.Decode(rs_encoded, rs_decoded)) {
			error = 997;
			return Ptr<Packet>(0);
		}
		return decode_l2b(rs_decoded, len - REDUNDANCY_LONG, rssi, error, maybe_encrypted);
	}
}

// Decode packet coming from layer 1 that may be encrypted.
Ptr<Packet> Packet::decode_l2e(const char *data, size_t len, int rssi, int& error)
{
	return decode_l2(data, len, rssi, error, true);
}

// Decode unencrypted packet coming from layer 1.
Ptr<Packet> Packet::decode_l2u(const char *data, size_t len, int rssi, int& error)
{
	return decode_l2(data, len, rssi, error, false);
}

// Decode packet coming from layer 2.
Ptr<Packet> Packet::decode_l3(const char* data, int& error, bool encrypted)
{
	return decode_l3(data, strlen(data), -50, error, encrypted);
}

// Decode packet coming from layer 2.
Ptr<Packet> Packet::decode_l3(const char* data, size_t len, int rssi, int &error,
				bool encrypted)
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

	Ptr<Packet> p = Ptr<Packet>(new Packet(to, from, params, Buffer(msg, msg_len), rssi));
	if (p && encrypted) {
		p->set_encrypted();
	}
	return p;
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

	if (b.length() > MSGSIZ_LONG) {
		// FIXME return error or warning
		b = b.substr(0, MSGSIZ_LONG);
	}

	return b;
}

void Packet::append_fec(Buffer& b)
{
	memset(rs_decoded, 0, sizeof(rs_decoded));
	memcpy(rs_decoded, b.c_str(), b.length());
	if (b.length() <= MSGSIZ_SHORT) {
		rsf_short.Encode(rs_decoded, rs_encoded);
		b.append(rs_encoded + MSGSIZ_SHORT, REDUNDANCY_SHORT);
	} else if (b.length() <= MSGSIZ_MEDIUM) {
		rsf_medium.Encode(rs_decoded, rs_encoded);
		b.append(rs_encoded + MSGSIZ_MEDIUM, REDUNDANCY_MEDIUM);
	} else {
		rsf_long.Encode(rs_decoded, rs_encoded);
		b.append(rs_encoded + MSGSIZ_LONG, REDUNDANCY_LONG);
	}
}

Buffer Packet::encode_l2(bool possibly_encrypted) const
{
	Buffer b = encode_l3();
	if (possibly_encrypted) CryptoKeys::encrypt(b);
	append_fec(b);
	return b;
}

// Encode an unencrypted packet in level 2, with FEC and ready to be sent.
Buffer Packet::encode_l2u() const
{
	return encode_l2(false);
}

// Encode a packet in level 2, with FEC and ready to be sent.
// Encryption will be applied if key is stored on NVRAM
Buffer Packet::encode_l2e() const
{
	return encode_l2(true);
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

// returns True when packet was received in encrypted form
bool Packet::was_encrypted() const
{
	return _was_encrypted;
}

// Marks packet as received encrypted
void Packet::set_encrypted()
{
	_was_encrypted = true;
}
