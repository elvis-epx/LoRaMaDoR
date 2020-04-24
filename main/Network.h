// LoRaMaDoR (LoRa-based mesh network for hams) project
// Copyright (c) 2019 PU5EPX

#ifndef __NETWORK_H
#define __NETWORK_H

#include "Vector.h"
#include "Dict.h"
#include "Task.h"
#include "Params.h"
#include "Callsign.h"

#define SECONDS 1000
#define MINUTES 60000

class Protocol;
class Packet;

struct Neighbour {
	Neighbour(int rssi, unsigned long int timestamp):
		rssi(rssi), timestamp(timestamp) {}
	Neighbour() {}
	int rssi;
	long int timestamp;
};

struct RecvLogItem {
	RecvLogItem(int rssi, unsigned long int timestamp):
		rssi(rssi), timestamp(timestamp) {}
	RecvLogItem() {}
	int rssi;
	long int timestamp;
};

class Network {
public:
	Network(const Callsign &callsign);
	virtual ~Network();

	Callsign me() const;
	void send(const Callsign &to, Params params, const Buffer& msg);
	void run_tasks(unsigned long int);
	Dict<Neighbour> neigh() const;
	static unsigned long int fudge(unsigned long int avg, double fudge);

	// publicised to bridge with uncoupled code
	void radio_recv(const char *recv_area, unsigned int plen, int rssi);
	unsigned int get_last_pkt_id() const;
	unsigned int get_next_pkt_id();
	void add_protocol(Protocol*);
	void schedule(Task*);

	// publicised to be called by Tasks
	unsigned long int tx(const Buffer&);
	void forward(Ptr<Packet>, bool, unsigned long int);
	unsigned long int clean_recv_log(unsigned long int);
	unsigned long int clean_neigh(unsigned long int);

	// publicised for testing purposes
	TaskManager& _task_mgr();
	Dict<Neighbour>& _neighbours();
	Dict<RecvLogItem>& _recv_log();

private:
	void recv(Ptr<Packet> pkt);
	void sendmsg(const Ptr<Packet> pkt);

	Callsign my_callsign;
	TaskManager task_mgr;
	Dict<Neighbour> neighbours;
	Dict<RecvLogItem> recv_log;
	unsigned int last_pkt_id;
	Vector< Ptr<Protocol> > protocols;
};

#endif
