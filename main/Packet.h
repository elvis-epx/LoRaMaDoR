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

struct Packet {
	Packet(const Callsign &to, const Callsign &from, 
		const Params& params, const Buffer& msg, int rssi=0);
	~Packet();

	static Ptr<Packet> decode_l2(const char* data, unsigned int len, int rssi, int& error);
	/* public for unit testing */
	static Ptr<Packet> decode_l3(const char* data, unsigned int len, int rssi, int& error);
	static Ptr<Packet> decode_l3(const char *data, int& error);

	Packet(const Packet &) = delete;
	Packet(Packet &&) = delete;
	Packet() = delete;
	Packet& operator=(const Packet &) = delete;
	bool operator==(const Packet &) = delete;

	Ptr<Packet> change_msg(const Buffer&) const;
	Ptr<Packet> change_params(const Params&) const;
	Buffer encode_l2() const;
	Buffer encode_l3() const; /* publicised for unit testing */
	bool is_dup(const Packet& other) const;
	const char *signature() const;
	Callsign to() const;
	Callsign from() const;
	const Params params() const;
	const Buffer msg() const;
	int rssi() const;

private:
	Callsign _to;
	Callsign _from;
	const Params _params;
	Buffer _signature;
	const Buffer _msg;
	int _rssi;
};

#endif
