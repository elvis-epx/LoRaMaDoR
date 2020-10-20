/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

// Implementation of R earmarking of forwarded packets

#ifndef __MODF_R_H
#define __MODF_R_H

#include "Modifier.h"

class Modf_R: public Modifier {
public:
	Modf_R(Network* net);
	virtual Ptr<Packet> modify(const Packet&);

	Modf_R() = delete;
	Modf_R(const Modf_R&) = delete;
	Modf_R(Modf_R&&) = delete;
	Modf_R& operator=(const Modf_R&) = delete;
	Modf_R& operator=(Modf_R&&) = delete;
};

#endif
