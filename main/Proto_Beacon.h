// LoRaMaDoR (LoRa-based mesh network for hams) project
// Copyright (c) 2020 PU5EPX
// Implementation of periodic eacon

#ifndef __PROTO_BEACON_H
#define __PROTO_BEACON_H

#include "Protocol.h"

class BeaconTask;

class Proto_Beacon: public Protocol {
public:
	Proto_Beacon(Network* net);
private:
	unsigned long int beacon() const;
	friend class BeaconTask;

	Proto_Beacon() = delete;
	Proto_Beacon(const Proto_Beacon&) = delete;
	Proto_Beacon(Proto_Beacon&&) = delete;
	Proto_Beacon& operator=(const Proto_Beacon&) = delete;
	Proto_Beacon& operator=(Proto_Beacon&&) = delete;
};

#endif
