#include "Proto_Beacon.h"
#include "Network.h"
#include "ArduinoBridge.h"

#ifndef UNDER_TEST
static const unsigned int AVG_BEACON_TIME = 10 * MINUTES;
static const unsigned int AVG_FIRST_BEACON_TIME = 30 * SECONDS;
#else
static const unsigned int AVG_BEACON_TIME = 10 * SECONDS;
static const unsigned int AVG_FIRST_BEACON_TIME = 1 * SECONDS;
#endif

class BeaconTask: public Task
{
public:
	BeaconTask(Proto_Beacon *b, unsigned long int offset):
		Task("beacon", offset), beacon(b)
	{ }
protected:
	virtual unsigned long int run2(unsigned long int now)
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

unsigned long int Proto_Beacon::beacon() const
{
	// FIXME convert to hms
	Buffer msg = Buffer::sprintf("LoRaMaDoR %ld 73", arduino_millis() / 1000);
	net->send(Callsign("QB"), Params(), msg);
	unsigned long int next = Network::fudge(AVG_BEACON_TIME, 0.5);
	// logi("Next beacon in ", next);
	return next;
}
