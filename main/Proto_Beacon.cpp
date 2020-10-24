/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

// Implementation of periodic beacon (sent to QB).

#include "Proto_Beacon.h"
#include "Network.h"
#include "ArduinoBridge.h"

// Task for periodic transmission of beacon packet.
class BeaconTask: public Task
{
public:
	BeaconTask(Proto_Beacon *b, int64_t offset):
		Task("beacon", offset), beacon(b)
	{
	}
	~BeaconTask() {
		beacon = 0;
	}
protected:
	virtual int64_t run2(int64_t now)
	{
		return beacon->beacon();
	}
private:
	Proto_Beacon *beacon;
};


Proto_Beacon::Proto_Beacon(Network *net): L7Protocol(net)
{
	net->schedule(new BeaconTask(this,
		Network::fudge(arduino_nvram_beacon_first_load() * 1000, 0.5)));
}

int64_t Proto_Beacon::beacon() const
{
	Buffer uptime = Buffer::millis_to_hms(arduino_millis_nw());
	Buffer msg = Buffer("LoRaMaDoR up ") + uptime;
	net->send(Callsign("QB"), Params(), msg);
	uint32_t next = Network::fudge(arduino_nvram_beacon_load() * 1000, 0.5);
	// logi("Next beacon in ", next);
	return next;
}
