/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

// Class that encapsulates the parameters of a LoRaMaDoR packet.

#ifndef __PARAMS_H
#define __PARAMS_H

#include <cstdint>
#include "Buffer.h"
#include "Dict.h"

class Params
{
public:
	Params();
	Params(Buffer);
	Buffer serialized() const;
	bool is_valid_with_ident() const;
	bool is_valid_without_ident() const;
	uint32_t ident() const;
	Buffer s_ident() const;
	size_t count() const;
	Buffer get(const char *) const;
	bool has(const char *) const;
	void put(const char *, const Buffer&);
	void put_naked(const char *);
	void remove(const char *);
	bool is_key_naked(const char *) const;
	void set_ident(uint32_t);
private:
	Dict<Buffer> items;
	uint32_t _ident;
	bool valid;
};

#endif
