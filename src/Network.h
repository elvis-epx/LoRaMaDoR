/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

// Main LoRaMaDoR network class, plus some auxiliary types

#ifndef __NETWORK_H
#define __NETWORK_H

#include <cstddef>
#include <cstdint>
#include "Vector.h"
#include "Dict.h"
#include "Task.h"
#include "Params.h"
#include "Callsign.h"
#include "LoRaL2/LoRaL2.h"

#define MAX_PACKET_ID 9999

class L7Protocol;
class L4Protocol;
class Modifier;
class Packet;

struct Peer {
	Peer(int rssi, int64_t timestamp);
	Peer();
	int rssi;
	int64_t timestamp;
};

struct RecvLogItem {
	RecvLogItem(int rssi, int64_t timestamp);
	RecvLogItem();
	int rssi;
	int64_t timestamp;
};

class Network: public LoRaL2Observer {
public:
	Network();
	virtual ~Network();

	Callsign me() const;
	bool am_i_repeater() const;
	uint32_t send(const Callsign &to, Params params, const Buffer& msg);
	void run_tasks(int64_t);
	const Dict<Peer>& neighbors() const;
	const Dict<Peer>& repeaters() const;
	const Dict<Peer>& peers() const;
	static uint32_t fudge(uint32_t avg, double fudge);
	static Buffer gen_random_token(int);
	size_t max_payload() const;

	// publicised to bridge with uncoupled code
	virtual void recv(LoRaL2Packet *);
	size_t get_last_pkt_id() const;

	// publicised to be called by protocols
	void schedule(Task*);

	// Called by each Protocol or Modifier subclass constructor.
	// Never call this yourself.
	void add_l7protocol(L7Protocol*);
	void add_l4protocol(L4Protocol*);
	void add_modifier(Modifier*);

	// publicised to be called by Tasks
	int64_t tx(const Buffer&);
	void route(Ptr<Packet>, bool, int64_t);
	int64_t clean_recv_log(int64_t);
	int64_t clean_neigh(int64_t);

	// publicised for testing purposes
	TaskManager& _task_mgr();
	Dict<Peer>& _neighbors();
	Dict<Peer>& _repeaters();
	Dict<Peer>& _peers();
	Dict<RecvLogItem>& _recv_log();

private:
	void recv(Ptr<Packet> pkt);
	size_t get_next_pkt_id();
	void update_peerlist(int64_t, const Ptr<Packet> &);

	Callsign my_callsign;
	uint32_t repeater_function_activated;

	Ptr<LoRaL2> transport;
	TaskManager task_mgr;
	Dict<Peer> neigh;
	Dict<Peer> reptr;
	Dict<Peer> peerlist;
	Dict<RecvLogItem> recv_log;
	size_t last_pkt_id;
	Vector< Ptr<L7Protocol> > l7protocols;
	Vector< Ptr<L4Protocol> > l4protocols;
	Vector< Ptr<Modifier> > modifiers;
};

#endif
