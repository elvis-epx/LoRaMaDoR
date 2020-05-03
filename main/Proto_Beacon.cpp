/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

// Implementation of periodic beacon (sent to QB).

#include "Proto_Beacon.h"
#include "Network.h"
#include "ArduinoBridge.h"

#ifndef UNDER_TEST
static const uint32_t AVG_BEACON_TIME = 10 * MINUTES;
static const uint32_t AVG_FIRST_BEACON_TIME = 30 * SECONDS;
#else
static const uint32_t AVG_BEACON_TIME = 10 * SECONDS;
static const uint32_t AVG_FIRST_BEACON_TIME = 1 * SECONDS;
#endif

// Task for periodic transmission of beacon packet.
class BeaconTask: public Task
{
public:
	BeaconTask(Proto_Beacon *b, uint32_t offset):
		Task("beacon", offset), beacon(b)
	{
	}
	~BeaconTask() {
		beacon = 0;
	}
protected:
	virtual uint32_t run2(uint32_t now)
	{
		return beacon->beacon();
	}
private:
	Proto_Beacon *beacon;
};


Proto_Beacon::Proto_Beacon(Network *net): Protocol(net)
{
	net->schedule(new BeaconTask(this,
		Network::fudge(AVG_FIRST_BEACON_TIME, 0.5)));
}

uint32_t Proto_Beacon::beacon() const
{
	Buffer uptime = Buffer::millis_to_hms(arduino_millis());
	Buffer msg = Buffer::sprintf("LoRaMaDoR up %s", uptime.c_str());
	net->send(Callsign("QB"), Params(), msg);
	uint32_t next = Network::fudge(AVG_BEACON_TIME, 0.5);
	// logi("Next beacon in ", next);
	return next;
}
