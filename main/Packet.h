/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

// Class that encapsulates a LoRaMaDoR packet.
// Includes layer-2 and layer-3 parsing and encoding.

#ifndef __PACKET_H
#define  __PACKET_H

#include "Vector.h"
#include "Buffer.h"
#include "Pointer.h"
#include "Callsign.h"
#include "Params.h"

class Packet {
public:
	Packet(const Callsign &to, const Callsign &from, 
		const Params& params, const Buffer& msg, int rssi=0);
	~Packet();

	static Ptr<Packet> decode_l2e(const char* data, size_t len, int rssi, int& error);
	/* next 2 are public for unit testing */
	static Ptr<Packet> decode_l3(const char* data, size_t len, int rssi, int& error, bool encrypted);
	static Ptr<Packet> decode_l3(const char *data, int& error, bool encrypted);

	Packet(const Packet &) = delete;
	Packet(Packet &&) = delete;
	Packet() = delete;
	Packet& operator=(const Packet &) = delete;
	bool operator==(const Packet &) = delete;

	Ptr<Packet> change_msg(const Buffer&) const;
	Ptr<Packet> change_params(const Params&) const;
	/* next 2 are public for unit testing */
	Buffer encode_l2u() const;
	static Ptr<Packet> decode_l2u(const char* data, size_t len, int rssi, int& error);
	Buffer encode_l2e() const;
	Buffer encode_l3() const;
	bool is_dup(const Packet& other) const;
	Buffer signature() const;
	Callsign to() const;
	Callsign from() const;
	const Params params() const;
	const Buffer msg() const;
	int rssi() const;
	bool was_encrypted() const;
	void set_encrypted();

private:
	Buffer encode_l2(bool) const;
	static Ptr<Packet> decode_l2(const char* data, size_t len, int rssi, int& error, bool);

	Callsign _to;
	Callsign _from;
	const Params _params;
	Buffer _signature;
	const Buffer _msg;
	int _rssi;
	bool _was_encrypted;
};

#endif
