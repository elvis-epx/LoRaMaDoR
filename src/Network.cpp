/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

// Main LoRaMaDoR network class, plus some auxiliary types

#include "NVRAM.h"
#include "ArduinoBridge.h"
#include "Timestamp.h"
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
#include "Proto_Switch.h"
#include "Modf_Rreq.h"
#include "Config.h"

static const int64_t TX_BUSY_RETRY_TIME = 1 * SECONDS;

static const int64_t NEIGH_PERSIST = 60 * MINUTES;
static const int64_t NEIGH_CLEAN = 1 * MINUTES;

static const int64_t RECV_LOG_PERSIST = 10 * MINUTES;
static const int64_t RECV_LOG_CLEAN = 1 * MINUTES;

Peer::Peer(int rssi, int64_t timestamp):
	rssi(rssi), timestamp(timestamp)
{}

Peer::Peer()
{}

RecvLogItem::RecvLogItem(int rssi, int64_t timestamp):
	rssi(rssi), timestamp(timestamp)
{}

RecvLogItem::RecvLogItem()
{}

// Generates a random number with average avg and spread 'fudge'
uint32_t Network::fudge(uint32_t avg, double fudge)
{
	return arduino_random(avg * (1.0 - fudge), avg * (1.0 + fudge));
}

// Generates a random string with safe characters (~5 bits per char)
Buffer Network::gen_random_token(int len)
{
	char *s = new char[len + 1];
	for (int i = 0; i < len; ++i) {
		int n = arduino_random(0, 36);
		s[i] = "0123456789abcdefghijklmnopqrstuvwxyz_"[n];
	}
	s[len] = 0;
	Buffer b(s, len);
	delete[] s;
	return b;
}

// Packet transmission task.
class PacketTx: public Task {
public:
	PacketTx(Network* net, const Buffer& encoded_packet, int64_t offset): 
			Task("tx", offset), net(net), encoded_packet(encoded_packet)
	{
	}
protected:
	virtual int64_t run2(int64_t now) {
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
	virtual int64_t run2(int64_t now)
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
	CleanNeighTask(Network* net, int64_t offset):
		Task("neigh", offset), net(net)
	{
	}
protected:
	virtual int64_t run2(int64_t now)
	{
		return net->clean_neigh(now);
	}
private:
	Network *net;
};


// Prune packet rx log task.
class CleanRecvLogTask: public Task {
public:
	CleanRecvLogTask(Network* net, int64_t offset):
		Task("recvlog", offset), net(net)
	{
	}
protected:
	virtual int64_t run2(int64_t now)
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

Network::Network()
{
	my_callsign = arduino_nvram_callsign_load();
	if (! my_callsign.is_valid()) return;
	repeater_function_activated = arduino_nvram_repeater_load();
	last_pkt_id = arduino_nvram_id_load();

	// Periodic housecleaning tasks
	schedule(new CleanRecvLogTask(this, RECV_LOG_CLEAN));
	schedule(new CleanNeighTask(this, NEIGH_CLEAN));

	// Core L7 protocols
	// (should come before others, since e.g. RREQ does not check HMAC)
	new Proto_Beacon(this);
	new Proto_Ping(this);
	new Proto_Rreq(this);
#ifdef SWITCH_PROTO_SUPPORT
	new Proto_Switch(this);
#endif

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
	if (++last_pkt_id > MAX_PACKET_ID) {
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

	// handle L4 protocols, in reverse order of RX
	for (size_t i = l4protocols.count(); i > 0; --i) {
		auto response = l4protocols[i-1]->tx(*pkt);
		if (response.pkt) {
			// more than one L4 protocol can tweak the packet
			pkt = response.pkt;
		}
	}

	// schedule radio routing/transmission
	schedule(new PacketFwd(this, pkt, true));

	return id;
}

// Receive packet targeted to this station
void Network::recv(Ptr<Packet> pkt)
{
	logs("Received pkt", pkt->encode_l3());

	// handle L4 protocols
	for (size_t i = 0; i < l4protocols.count(); ++i) {
		auto response = l4protocols[i]->rx(*pkt);
		if (response.has_packet) {
			send(response.to, response.params, response.msg);
		}
		if (response.error) {
			logs("L4 error", response.error_msg);
			return;
		}
	}

	// check if packet can be handled automatically by L7 protocol
	for (size_t i = 0; i < l7protocols.count(); ++i) {
		auto response = l7protocols[i]->handle(*pkt);
		if (response.has_packet) {
			send(response.to, response.params, response.msg);
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
	Ptr<Packet> pkt = Packet::decode_l2e(recv_area, plen, rssi, error);
	if (!pkt) {
		logi("rx invalid pkt err", error);
		return;
	}
	logi("rx good packet, RSSI =", rssi);
	schedule(new PacketFwd(this, pkt, false));
}

// purge old packet IDs from recv log
int64_t Network::clean_recv_log(int64_t now)
{
	Vector<Buffer> remove_list;

	const Vector<Buffer>& keys = recv_log.keys();
	for (size_t i = 0; i < keys.count(); ++i) {
		if ((recv_log[keys[i]].timestamp + RECV_LOG_PERSIST) < now) {
			remove_list.push_back(keys[i]);
		}
	}

	for (size_t i = 0; i < remove_list.count(); ++i) {
		recv_log.remove(remove_list[i]);
		// logs("Forgotten packet", remove_list[i]);
	}

	return RECV_LOG_CLEAN;
}

// purge neighbors that have been silent for a while
int64_t Network::clean_neigh(int64_t now)
{
	{
	Vector<Buffer> remove_list;
	const Vector<Buffer>& keys = neigh.keys();
	for (size_t i = 0; i < keys.count(); ++i) {
		if ((neigh[keys[i]].timestamp + NEIGH_PERSIST) < now) {
			remove_list.push_back(keys[i]);
		}
	}

	for (size_t i = 0; i < remove_list.count(); ++i) {
		neigh.remove(remove_list[i]);
		logs("Forgotten neigh", remove_list[i]);
	}
	}

	{
	Vector<Buffer> remove_list;
	const Vector<Buffer>& keys = reptr.keys();
	for (size_t i = 0; i < keys.count(); ++i) {
		if ((reptr[keys[i]].timestamp + NEIGH_PERSIST) < now) {
			remove_list.push_back(keys[i]);
		}
	}

	for (size_t i = 0; i < remove_list.count(); ++i) {
		reptr.remove(remove_list[i]);
		logs("Forgotten repeater", remove_list[i]);
	}
	}

	{
	Vector<Buffer> remove_list;
	const Vector<Buffer>& keys = peerlist.keys();
	for (size_t i = 0; i < keys.count(); ++i) {
		if ((peerlist[keys[i]].timestamp + NEIGH_PERSIST) < now) {
			remove_list.push_back(keys[i]);
		}
	}

	for (size_t i = 0; i < remove_list.count(); ++i) {
		peerlist.remove(remove_list[i]);
		logs("Forgotten peer", remove_list[i]);
	}
	}

	return NEIGH_CLEAN;
}

// execute packet transmission
int64_t Network::tx(const Buffer& encoded_packet)
{
	if (! lora_tx(encoded_packet)) {
		return TX_BUSY_RETRY_TIME;
	}
	return 0;
}

/* Update neighbor and peer lists based on a packet that
   was sent to us, either unicast or QB/QR/QC */
void Network::update_peerlist(int64_t now, const Ptr<Packet> &pkt)
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

		if (pkt->to().is_repeater()) {
			if (! reptr.has(from)) {
				logs("discovered repeater", from);
			}
			reptr[from] = Peer(pkt->rssi(), now);
		}
	}
}

// handle packet received from radio or from application layer
void Network::route(Ptr<Packet> pkt, bool we_are_origin, int64_t now)
{
	if (we_are_origin) {
		if (me() == pkt->to() || pkt->to().is_lo()) {
			recv(pkt);
			return;
		}

		// Annotate to detect duplicates
		recv_log[pkt->signature()] = RecvLogItem(pkt->rssi(), now);
		// Transmit
		schedule(new PacketTx(this, pkt->encode_l2e(), 1));
		logs("tx ", pkt->encode_l3());
		return;
	}

	// Packet originated from us but received via radio = loop
	if (me() == pkt->from()) {
		// logs("pkt loop", pkt->signature());
		return;
	}

	// Discard received duplicates
	if (recv_log.has(pkt->signature())) {
		// logs("pkt dup", pkt->signature());
		return;
	}
	recv_log[pkt->signature()] = RecvLogItem(pkt->rssi(), now);

	if (me() == pkt->to()) {
		// We are the sole final destination
		update_peerlist(now, pkt);
		recv(pkt);
		return;
	}

	if (pkt->to().is_bcast()) {
		// We are just one of the destinations
		update_peerlist(now, pkt);
		recv(pkt);
	}

	// Diffusion routing from this point on
	if (! repeater_function_activated) {
		return;
	}

	bool already_repeated = pkt->params().has("R");

	// Forward packet modifiers
	// They can add params and/or change msg
	for (size_t i = 0; i < modifiers.count(); ++i) {
		auto modified_pkt = modifiers[i]->modify(*pkt);
		if (modified_pkt) {
			pkt = modified_pkt;
		}
	}

	Buffer encoded_pkt = pkt->encode_l2e();

	// Average TX delay: 2.5x the packet airtime
	// spread from 0 to 5x to mitigate collision in the case of multiple repeaters
	double packet_len = SECONDS * encoded_pkt.length() * 8
				/ lora_speed_bps();
	int64_t delay = Network::fudge(packet_len * 2.5, 0.95);
	// Packets already repeated: additional window to mitigate collision from
	// additional repeaters of the last hop
	if (already_repeated) {
		delay += packet_len * 5;
	}

	logi("relaying w/ delay", delay);
	schedule(new PacketTx(this, encoded_pkt, delay));
}

// Schedule a Task. Run later via run_tasks().
void Network::schedule(Task *task)
{
	task_mgr.schedule(Ptr<Task>(task));
}

// Run pending tasks. Called by system may loop.
void Network::run_tasks(int64_t millis)
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

const Dict<Peer>& Network::repeaters() const
{
	return reptr;
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
Dict<Peer>& Network::_repeaters()
{
	return reptr;
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

bool Network::am_i_repeater() const
{
	return repeater_function_activated;
}
