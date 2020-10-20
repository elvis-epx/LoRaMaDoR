/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

// Main LoRaMaDoR network class, plus some auxiliary types

#include "ArduinoBridge.h"
#include "Network.h"
#include "Packet.h"
#include "Params.h"
#include "CLI.h"
#include "Radio.h"
#include "Proto_Ping.h"
#include "Proto_Beacon.h"
#include "Modf_R.h"
#include "Proto_C.h"
#include "Proto_HMAC.h"
#include "Proto_Rreq.h"
#include "Modf_Rreq.h"

static const uint32_t TX_BUSY_RETRY_TIME = 1 * SECONDS;

static const uint32_t NEIGH_PERSIST = 60 * MINUTES;
static const uint32_t NEIGH_CLEAN = 1 * MINUTES;

static const uint32_t RECV_LOG_PERSIST = 10 * MINUTES;
static const uint32_t RECV_LOG_CLEAN = 1 * MINUTES;

Peer::Peer(int rssi, int32_t timestamp):
	rssi(rssi), timestamp(timestamp)
{}

Peer::Peer()
{}

RecvLogItem::RecvLogItem(int rssi, int32_t timestamp):
	rssi(rssi), timestamp(timestamp)
{}

RecvLogItem::RecvLogItem()
{}

// Generates a random number with average avg and spread 'fudge'
uint32_t Network::fudge(uint32_t avg, double fudge)
{
	return arduino_random(avg * (1.0 - fudge), avg * (1.0 + fudge));
}

// Packet transmission task.
class PacketTx: public Task {
public:
	PacketTx(Network* net, const Buffer& encoded_packet, uint32_t offset): 
			Task("tx", offset), net(net), encoded_packet(encoded_packet)
	{
	}
protected:
	virtual uint32_t run2(uint32_t now) {
		return net->tx(encoded_packet);
	}
private:
	Network *net;
	const Buffer encoded_packet;
};

// Packet routing task.
class PacketFwd: public Task {
public:
	PacketFwd(Network* net, const Ptr<Packet> packet, bool we_are_origin):
		Task("fwd", 0), net(net), packet(packet), we_are_origin(we_are_origin)
	{
	}
protected:
	virtual uint32_t run2(uint32_t now)
	{
		net->route(packet, we_are_origin, now);
		// forces this task to be one-off
		return 0;
	}
private:
	Network *net;
	const Ptr<Packet> packet;
	const bool we_are_origin;
};

// Prune neighborhood table task.
class CleanNeighTask: public Task {
public:
	CleanNeighTask(Network* net, uint32_t offset):
		Task("neigh", offset), net(net)
	{
	}
protected:
	virtual uint32_t run2(uint32_t now)
	{
		return net->clean_neigh(now);
	}
private:
	Network *net;
};


// Prune packet rx log task.
class CleanRecvLogTask: public Task {
public:
	CleanRecvLogTask(Network* net, uint32_t offset):
		Task("recvlog", offset), net(net)
	{
	}
protected:
	virtual uint32_t run2(uint32_t now)
	{
		return net->clean_recv_log(now);
	}
private:
	Network *net;
};


// bridges C-style callback from LoRa/Radio module to network object
void radio_recv_trampoline(const char *recv_area, size_t plen, int rssi);
Network* trampoline_target = 0;

//////////////////////////// Network class proper

Network::Network(const Callsign &callsign, uint32_t repeater)
{
	if (! callsign.is_valid()) {
		return;
	}

	my_callsign = callsign;
	repeater_function_activated = repeater;
	last_pkt_id = arduino_nvram_id_load();

	// Periodic housecleaning tasks
	schedule(new CleanRecvLogTask(this, RECV_LOG_CLEAN));
	schedule(new CleanNeighTask(this, NEIGH_CLEAN));

	// Core L7 protocols
	// (should come before others, since e.g. RREQ does not check HMAC)
	new Proto_Beacon(this);
	new Proto_Ping(this);
	new Proto_Rreq(this);

	// Core L4 protocols
	new Proto_HMAC(this); // must be the first to handle rx
	new Proto_C(this);

	// Core L3 modifiers
	new Modf_R(this);
	new Modf_Rreq(this);

	trampoline_target = this;
	lora_start(radio_recv_trampoline);
}

Network::~Network()
{
	// makes sure tasks stop before protocols
	task_mgr.stop();
	l4protocols.clear();
	l7protocols.clear();
	modifiers.clear();
}

// Add a protocol to stack. Called by Protocol class itself,
// so it never has to be called explicitly when stantiating 
// a protocol.
void Network::add_l7protocol(L7Protocol* p)
{
	l7protocols.push_back(Ptr<L7Protocol>(p));
}

void Network::add_l4protocol(L4Protocol* p)
{
	l4protocols.push_back(Ptr<L4Protocol>(p));
}

// Add a modifier to stack. Called by Modifier class itself,
// so it never has to be called explicitly when stantiating 
// a protocol.
void Network::add_modifier(Modifier * p)
{
	modifiers.push_back(Ptr<Modifier>(p));
}

// Gets next packet ID and saves to NVRAM
size_t Network::get_next_pkt_id()
{
	if (++last_pkt_id > 9999) {
		last_pkt_id = 1;
	}
	arduino_nvram_id_save(last_pkt_id);
	return last_pkt_id;
}

size_t Network::get_last_pkt_id() const
{
	return last_pkt_id;
}

// Called from application layer to send a packet
// (i.e. originate a packet)
uint32_t Network::send(const Callsign &to, Params params, const Buffer& msg)
{
	uint32_t id = get_next_pkt_id();
	params.set_ident(id);
	Ptr<Packet> pkt(new Packet(to, me(), params, msg));
	_send(pkt);
	return id;
}

// Second phase of packet origination
void Network::_send(Ptr<Packet> pkt)
{
	// handle L4 protocols, in reverse order of RX
	for (size_t i = l4protocols.size(); i > 0; --i) {
		auto response = l4protocols[i-1]->tx(*pkt);
		if (response.pkt) {
			pkt = response.pkt;
		}
	}

	// schedule radio routing/transmission
	schedule(new PacketFwd(this, pkt, true));
}

// Receive packet targeted to this station
void Network::recv(Ptr<Packet> pkt)
{
	logs("Received pkt", pkt->encode_l3());

	// handle L4 protocols
	for (size_t i = 0; i < l4protocols.size(); ++i) {
		auto response = l4protocols[i]->rx(*pkt);
		if (response.pkt) {
			_send(response.pkt);
		}
		if (response.error) {
			logs("L4 error", response.error_msg);
			return;
		}
	}

	// check if packet can be handled automatically by L7 protocol
	for (size_t i = 0; i < l7protocols.size(); ++i) {
		auto response = l7protocols[i]->handle(*pkt);
		if (response.pkt) {
			_send(response.pkt);
			break;
		}
	}

	app_recv(pkt);
}

// Called by LoRa layer
void radio_recv_trampoline(const char *recv_area, size_t plen, int rssi)
{
	// ugly, but...
	trampoline_target->radio_recv(recv_area, plen, rssi);
}

// Handle packet from radio, schedule processing
void Network::radio_recv(const char *recv_area, size_t plen, int rssi)
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
uint32_t Network::clean_recv_log(uint32_t now)
{
	Vector<Buffer> remove_list;
	int32_t cutoff = now - RECV_LOG_PERSIST;

	const Vector<Buffer>& keys = recv_log.keys();
	for (size_t i = 0; i < keys.size(); ++i) {
		if (recv_log[keys[i]].timestamp < cutoff) {
			remove_list.push_back(keys[i]);
		}
	}

	for (size_t i = 0; i < remove_list.size(); ++i) {
		recv_log.remove(remove_list[i]);
		// logs("Forgotten packet", remove_list[i]);
	}

	return RECV_LOG_CLEAN;
}

// purge neighbors that have been silent for a while
uint32_t Network::clean_neigh(uint32_t now)
{
	Vector<Buffer> remove_list;
	int32_t cutoff = now - NEIGH_PERSIST;

	{
	const Vector<Buffer>& keys = neigh.keys();
	for (size_t i = 0; i < keys.size(); ++i) {
		if (neigh[keys[i]].timestamp < cutoff) {
			remove_list.push_back(keys[i]);
		}
	}

	for (size_t i = 0; i < remove_list.size(); ++i) {
		neigh.remove(remove_list[i]);
		logs("Forgotten neigh", remove_list[i]);
	}
	}

	{
	const Vector<Buffer>& keys = peerlist.keys();
	for (size_t i = 0; i < keys.size(); ++i) {
		if (peerlist[keys[i]].timestamp < cutoff) {
			remove_list.push_back(keys[i]);
		}
	}

	for (size_t i = 0; i < remove_list.size(); ++i) {
		peerlist.remove(remove_list[i]);
		logs("Forgotten peer", remove_list[i]);
	}
	}

	return NEIGH_CLEAN;
}

// execute packet transmission
uint32_t Network::tx(const Buffer& encoded_packet)
{
	if (! lora_tx(encoded_packet)) {
		return TX_BUSY_RETRY_TIME;
	}
	return 0;
}

/* Update neighbor and peer lists based on a packet that
   was sent to us, either unicast or QB/QC */
void Network::update_peerlist(uint32_t now, const Ptr<Packet> &pkt)
{
	Buffer from = pkt->from();

	if (! peerlist.has(from)) {
		logs("discovered peer", from);
	}
	peerlist[from] = Peer(pkt->rssi(), now);

	if (! pkt->params().has("R")) {
		// no R = not forwarded; fresh from source
		if (! neigh.has(from)) {
			logs("discovered neighbor", from);
		}
		neigh[from] = Peer(pkt->rssi(), now);
	}
}

// handle packet received from radio or from application layer
void Network::route(Ptr<Packet> pkt, bool we_are_origin, uint32_t now)
{
	if (we_are_origin) {
		if (me() == pkt->to() || pkt->to().is_lo()) {
			recv(pkt);
			return;
		}

		// Annotate to detect duplicates
		recv_log[pkt->signature()] = RecvLogItem(pkt->rssi(), now);
		// Transmit
		schedule(new PacketTx(this, pkt->encode_l2(), 50));
		logs("tx ", pkt->encode_l3());
		return;
	}

	// Packet originated from us but received via radio = loop
	if (me() == pkt->from()) {
		logs("pkt loop", pkt->signature());
		return;
	}

	// Discard received duplicates
	if (recv_log.has(pkt->signature())) {
		logs("pkt dup", pkt->signature());
		return;
	}
	recv_log[pkt->signature()] = RecvLogItem(pkt->rssi(), now);

	if (me() == pkt->to()) {
		// We are the sole final destination
		update_peerlist(now, pkt);
		recv(pkt);
		return;
	}

	if (pkt->to() == "QB" || pkt->to() == "QC") {
		// We are just one of the destinations
		update_peerlist(now, pkt);
		recv(pkt);
	}

	// Diffusion routing from this point on
	if (! repeater_function_activated) {
		return;
	}

	// Forward packet modifiers
	// They can add params and/or change msg
	for (size_t i = 0; i < modifiers.size(); ++i) {
		auto modified_pkt = modifiers[i]->modify(*pkt);
		if (modified_pkt) {
			pkt = modified_pkt;
		}
	}

	Buffer encoded_pkt = pkt->encode_l2();

	// TX delay in bits: packet size x stations nearby x 2
	uint32_t bit_delay = encoded_pkt.length() * 8;
	bit_delay *= 2 * (1 + neigh.count());

	// convert delay in bits to milisseconds
	// e.g. 900 bits @ 600 bps = 1500 ms
	uint32_t delay = bit_delay * SECONDS / lora_speed_bps();

	logi("relaying w/ delay", delay);

	schedule(new PacketTx(this, encoded_pkt, delay));
	logs("relay ", pkt->encode_l3());
}

// Schedule a Task. Run later via run_tasks().
void Network::schedule(Task *task)
{
	task_mgr.schedule(Ptr<Task>(task));
}

// Run pending tasks. Called by system may loop.
void Network::run_tasks(uint32_t millis)
{
	task_mgr.run(millis);
}

Callsign Network::me() const {
	return my_callsign;
}

const Dict<Peer>& Network::neighbors() const
{
	return neigh;
}

const Dict<Peer>& Network::peers() const
{
	return peerlist;
}

/* For testing purposes only! */
TaskManager& Network::_task_mgr()
{
	return task_mgr;
}

/* For testing purposes only! */
Dict<Peer>& Network::_neighbors()
{
	return neigh;
}

/* For testing purposes only! */
Dict<Peer>& Network::_peers()
{
	return peerlist;
}

/* For testing purposes only! */
Dict<RecvLogItem>& Network::_recv_log()
{
	return recv_log;
}
