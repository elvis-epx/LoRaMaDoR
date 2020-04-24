// LoRaMaDoR (LoRa-based mesh network for hams) project
// Copyright (c) 2019 PU5EPX

#include "ArduinoBridge.h"
#include "Network.h"
#include "Packet.h"
#include "Params.h"
#include "Radio.h"
#include "Proto_Ping.h"
#include "Proto_Beacon.h"
#include "Proto_R.h"
#include "Proto_Rreq.h"

static const unsigned int TX_BUSY_RETRY_TIME = 1 * SECONDS;

static const unsigned int NEIGH_PERSIST = 60 * MINUTES;
static const unsigned int NEIGH_CLEAN = 1 * MINUTES;

static const unsigned int RECV_LOG_PERSIST = 10 * MINUTES;
static const unsigned int RECV_LOG_CLEAN = 1 * MINUTES;

unsigned long int Network::fudge(unsigned long int avg, double fudge)
{
	return arduino_random(avg * (1.0 - fudge), avg * (1.0 + fudge));
}

class PacketTx: public Task {
public:
	PacketTx(Network* net, const Buffer& encoded_packet, unsigned long int offset): 
			Task("tx", offset), net(net), encoded_packet(encoded_packet)
	{
	}
protected:
	virtual unsigned long int run2(unsigned long int now) {
		return net->tx(encoded_packet);
	}
private:
	Network *net;
	const Buffer encoded_packet;
};


class PacketFwd: public Task {
public:
	PacketFwd(Network* net, const Ptr<Packet> packet, bool we_are_origin):
		Task("fwd", 0), net(net), packet(packet), we_are_origin(we_are_origin)
	{
	}
protected:
	virtual unsigned long int run2(unsigned long int now)
	{
		net->forward(packet, we_are_origin, now);
		// forces this task to be one-off
		return 0;
	}
private:
	Network *net;
	const Ptr<Packet> packet;
	const bool we_are_origin;
};


class CleanNeighTask: public Task {
public:
	CleanNeighTask(Network* net, unsigned long int offset):
		Task("neigh", offset), net(net)
	{
	}
protected:
	virtual unsigned long int run2(unsigned long int now)
	{
		return net->clean_neigh(now);
	}
private:
	Network *net;
};


class CleanRecvLogTask: public Task {
public:
	CleanRecvLogTask(Network* net, unsigned long int offset):
		Task("recvlog", offset), net(net)
	{
	}
protected:
	virtual unsigned long int run2(unsigned long int now)
	{
		return net->clean_recv_log(now);
	}
private:
	Network *net;
};


// bridges C-style callback from LoRa/Radio module to network object
void radio_recv_trampoline(const char *recv_area, unsigned int plen, int rssi);

//////////////////////////// Network class proper

Network* trampoline_target = 0;

Network::Network(const Callsign &callsign)
{
	if (! callsign.is_valid()) {
		return;
	}

	my_callsign = callsign;
	last_pkt_id = arduino_nvram_id_load();

	schedule(new CleanRecvLogTask(this, RECV_LOG_CLEAN));
	schedule(new CleanNeighTask(this, NEIGH_CLEAN));

	// Core protocols
	new Proto_Ping(this);
	new Proto_Rreq(this);
	new Proto_R(this);
	new Proto_Beacon(this);

	trampoline_target = this;
	lora_start(radio_recv_trampoline);
}

Network::~Network()
{
	// makes sure tasks stop before protocols
	task_mgr.stop();
	protocols.clear();
}

void Network::add_protocol(Protocol* p)
{
	protocols.push_back(Ptr<Protocol>(p));
}

unsigned int Network::get_next_pkt_id()
{
	if (++last_pkt_id > 9999) {
		last_pkt_id = 1;
	}
	arduino_nvram_id_save(last_pkt_id);
	return last_pkt_id;
}

unsigned int Network::get_last_pkt_id() const
{
	return last_pkt_id;
}

// Called from application layer to send a packet
void Network::send(const Callsign &to, Params params, const Buffer& msg)
{
	params.set_ident(get_next_pkt_id());
	Ptr<Packet> pkt(new Packet(to, me(), params, msg));
	sendmsg(pkt);
}

// schedule radio transmission
void Network::sendmsg(const Ptr<Packet> pkt)
{
	schedule(new PacketFwd(this, pkt, true));
}

// Receive packet targeted to this station
void Network::recv(Ptr<Packet> pkt)
{
	logs("Received pkt", pkt->encode_l3().cold());

	// check if packet can be handled automatically
	for (unsigned int i = 0; i < protocols.size(); ++i) {
		Ptr<Packet> response = protocols[i]->handle(*pkt);
		if (response) {
			sendmsg(response);
			return;
		}
	}

	// if not handled automatically, deliver to application layer
	app_recv(pkt);
}

// Called by LoRa layer
void radio_recv_trampoline(const char *recv_area, unsigned int plen, int rssi)
{
	// ugly, but...
	trampoline_target->radio_recv(recv_area, plen, rssi);
}

// Handle packet from radio, schedule processing
void Network::radio_recv(const char *recv_area, unsigned int plen, int rssi)
{
	int error;
	Ptr<Packet> pkt = Packet::decode_l2(recv_area, plen, rssi, error);
	if (!pkt) {
		logi("rx invalid pkt err", error);
		return;
	}
	logi("rx good packet, RSSI =", rssi);
	schedule(new PacketFwd(this, pkt, false));
}

// purge old packet IDs from recv log
unsigned long int Network::clean_recv_log(unsigned long int now)
{
	Vector<Buffer> remove_list;
	long int cutoff = now - RECV_LOG_PERSIST;

	const Vector<Buffer>& keys = recv_log.keys();
	for (unsigned int i = 0; i < keys.size(); ++i) {
		if (recv_log[keys[i]].timestamp < cutoff) {
			remove_list.push_back(keys[i]);
		}
	}

	for (unsigned int i = 0; i < remove_list.size(); ++i) {
		recv_log.remove(remove_list[i]);
		// logs("Forgotten packet", remove_list[i].cold());
	}

	return RECV_LOG_CLEAN;
}

// purge neighbours that have been silent for a while
unsigned long int Network::clean_neigh(unsigned long int now)
{
	Vector<Buffer> remove_list;
	long int cutoff = now - NEIGH_PERSIST;

	const Vector<Buffer>& keys = neighbours.keys();
	for (unsigned int i = 0; i < keys.size(); ++i) {
		if (neighbours[keys[i]].timestamp < cutoff) {
			remove_list.push_back(keys[i]);
		}
	}

	for (unsigned int i = 0; i < remove_list.size(); ++i) {
		neighbours.remove(remove_list[i]);
		logs("Forgotten station", remove_list[i].cold());
	}

	return NEIGH_CLEAN;
}

// execute packet transmission
unsigned long int Network::tx(const Buffer& encoded_packet)
{
	if (! lora_tx(encoded_packet)) {
		return TX_BUSY_RETRY_TIME;
	}
	return 0;
}

// handle packet received from radio or from application layer
void Network::forward(Ptr<Packet> pkt, bool we_are_origin, unsigned long int now)
{
	int rssi = pkt->rssi();

	if (we_are_origin) {
		if (me().equal(pkt->to()) || pkt->to().is_localhost()) {
			recv(pkt);
			return;
		}

		// Annotate to detect duplicates
		recv_log[pkt->signature()] = RecvLogItem(rssi, now);
		// Transmit
		schedule(new PacketTx(this, pkt->encode_l2(), 50));
		logs("tx ", pkt->encode_l3().cold());
		return;
	}

	// Packet originated from us but received via radio = loop
	if (me().equal(pkt->from())) {
		logs("pkt loop", pkt->signature());
		return;
	}

	// Discard received duplicates
	if (recv_log.has(pkt->signature())) {
		logs("pkt dup", pkt->signature());
		return;
	}
	recv_log[pkt->signature()] = RecvLogItem(rssi, now);

	if (me().equal(pkt->to())) {
		// We are the final destination
		recv(pkt);
		return;
	}

	if (pkt->to().equal("QB") || pkt->to().equal("QC")) {
		// We are just one of the destinations
		if (! pkt->params().has("R")) {
			if (! neighbours.has(pkt->from().buf())) {
				logs("discovered neighbour", pkt->from().buf().cold());
			}
			neighbours[pkt->from().buf()] = Neighbour(rssi, now);
		}
		recv(pkt);
	}

	// Diffusion routing from this point on

	// Forward packet modifiers
	// They can add params and/or change msg
	for (unsigned int i = 0; i < protocols.size(); ++i) {
		Ptr<Packet> modified_pkt = protocols[i]->modify(*pkt);
		if (modified_pkt) {
			// replace packet by modified vesion
			pkt = modified_pkt;
			break;
		}
	}

	Buffer encoded_pkt = pkt->encode_l2();

	// TX delay in bits: packet size x stations nearby x 2
	unsigned long int bit_delay = encoded_pkt.length() * 8;
	bit_delay *= 2 * (1 + neighbours.count());

	// convert delay in bits to milisseconds
	// e.g. 900 bits @ 600 bps = 1500 ms
	unsigned long int delay = bit_delay * SECONDS / lora_speed_bps();

	logi("relaying w/ delay", delay);

	schedule(new PacketTx(this, encoded_pkt, delay));
	logs("relay ", pkt->encode_l3().cold());
}

void Network::schedule(Task *task)
{
	task_mgr.schedule(Ptr<Task>(task));
}

void Network::run_tasks(unsigned long int millis)
{
	task_mgr.run(millis);
}

Callsign Network::me() const {
	return my_callsign;
}

Dict<Neighbour> Network::neigh() const
{
	return neighbours;
}

/* For testing purposes only */
TaskManager& Network::_task_mgr()
{
	return task_mgr;
}

/* For testing purposes only */
Dict<Neighbour>& Network::_neighbours()
{
	return neighbours;
}

/* For testing purposes only */
Dict<RecvLogItem>& Network::_recv_log()
{
	return recv_log;
}
