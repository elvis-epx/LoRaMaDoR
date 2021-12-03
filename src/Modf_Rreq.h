/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

// Implementation of RREQ protocol responder

#ifndef __MODF_RREQ_H
#define __MODF_RREQ_H

#include "Modifier.h"

class Modf_Rreq: public Modifier {
public:
	Modf_Rreq(Network* net);
	virtual Ptr<Packet> modify(const Packet&);

	Modf_Rreq() = delete;
	Modf_Rreq(const Modf_Rreq&) = delete;
	Modf_Rreq(Modf_Rreq&&) = delete;
	Modf_Rreq& operator=(const Modf_Rreq&) = delete;
	Modf_Rreq& operator=(Modf_Rreq&&) = delete;
};

#endif
