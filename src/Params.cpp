/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

// Class that encapsulates the parameters of a LoRaMaDoR packet.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Params.h"

// Internal value of a naked parameter
static const char *naked = " n@ ";

// Parse one parameter key
static bool parse_symbol_param(const char *data, size_t len, Buffer& key, Buffer& value)
{
	size_t skey_len = 0;
	size_t svalue_len = 0;

	// find '=' separator, if exists
	const char *equal = (const char*) memchr(data, '=', len);

	if (! equal) {
		// naked key
		skey_len = len;
		svalue_len = 0;
	} else {
		// key=value, value may be empty
		skey_len = equal - data;
		svalue_len = len - skey_len - 1;
	}

	// check key name characters
	for (size_t i = 0; i < skey_len; ++i) {
		char c = data[i];
		if (c >= 'a' && c <= 'z') {
		} else if (c >= 'A' && c <= 'Z') {
		} else if (c >= '0' && c <= '9') {
		} else {
			return false;
		}
	}
	
	// check value characters, if there is a value
	if (equal) {
		for (size_t i = 0; i < svalue_len; ++i) {
			char c = equal[1 + i];
			if (strchr("= ,:<", c) || c == 0) {
				return false;
			}
		}
	}

	key = Buffer(data, skey_len);

	if (equal) {
		value = Buffer(equal + 1, svalue_len);
	} else {
		value = Buffer(naked);
	}

	return true;
}

// Parse the packet ID
static bool parse_ident_param(const char* s, size_t len, uint32_t &ident)
{
	char *stop;
	ident = strtol(s, &stop, 10);
	if (ident <= 0) {
		return false;
	} else if (ident > 999999) {
		return false;
	} else if (len != (unsigned) (stop - s)) {
		return false;
	}

	char n[10];
	snprintf(n, 10, "%d", ident);
	if (strlen(n) != len) {
		return false;
	}

	return true;
}

// Parse a parameter of any kind
static bool parse_param(const char* data, size_t len,
		uint32_t &ident, Buffer &key, Buffer &value)
{
	char c = data[0];

	if (c >= '0' && c <= '9') {
		return parse_ident_param(data, len, ident);
	} else if (c >= 'a' && c <= 'z') {
		ident = 0;
		return parse_symbol_param(data, len, key, value);
	} else if (c >= 'A' && c <= 'Z') {
		ident = 0;
		return parse_symbol_param(data, len, key, value);
	}

	return false;
}

// Parse parameters of a packet
static bool parse_params(const char *data, size_t len,
		uint32_t &ident, Dict<Buffer> &params)
{
	ident = 0;
	params = Dict<Buffer>();

	while (len > 0) {
		size_t param_len;
		size_t advance_len;

		const char *comma = (const char*) memchr(data, ',', len);
		if (! comma) {
			// last, or only, param
			param_len = len;
			advance_len = len;
		} else {
			param_len = comma - data;
			advance_len = param_len + 1;
		}

		if (! param_len) {
			return false;
		}

		uint32_t tident = 0;
		Buffer key;
		Buffer value;

		if (! parse_param(data, param_len, tident, key, value)) {
			return false;
		}
		if (tident) {
			// parameter is ident
			ident = tident;
		} else {
			// parameter is key=value or naked key
			key.uppercase();
			params.put(key, value);
		}

		data += advance_len;
		len -= advance_len;
	}

	return true;
}

Params::Params()
{
	_ident = 0;
	valid = true;
}

Params::Params(Buffer b)
{
	valid = parse_params(b.c_str(), b.length(), _ident, items);
}

Vector<Buffer> Params::keys() const
{
	return items.keys();
}

// Generate the wire format of the parameter list
Buffer Params::serialized() const
{
	Buffer buf = s_ident();

	const Vector<Buffer>& keys = items.keys();
	for (size_t i = 0; i < keys.count(); ++i) {
		const Buffer& key = keys[i];
		const Buffer& value = items[key];
		buf += ',';
		buf += key;
		if (value != naked) {
			buf += '=';
			buf += value;
		}
	}

	return buf;
}

// The parsed parameters are valid and contain a packet ID
bool Params::is_valid_with_ident() const
{
	return valid && _ident;
}

// The parsed parameters are valid but may not contain a packet ID
bool Params::is_valid_without_ident() const
{
	return valid;
}

// Packet ID. Zero is nil.
uint32_t Params::ident() const
{
	return _ident;
}

Buffer Params::s_ident() const
{
	return Buffer::itoa(_ident);
}

// Number of parameters (not couting packet ID).
size_t Params::count() const
{
	return items.count();
}

// Get parameter by key. Case-insensitive.
Buffer Params::get(const char *key) const
{
	Buffer ukey(key);
	ukey.uppercase();
	return items.get(ukey);
}

// Check if a parameter is present. Case-insensitive.
bool Params::has(const char *key) const
{
	Buffer ukey(key);
	ukey.uppercase();
	return items.has(ukey);
}

// Add or replace a parameter with value. Key is case-insensitive.
void Params::put(const char *key, const Buffer& value)
{
	Buffer ukey(key);
	ukey.uppercase();
	items.put(ukey, value);
}

// Add or replace a naked parameter (key w/o value)
void Params::put_naked(const char *key)
{
	Buffer ukey(key);
	ukey.uppercase();
	items.put(ukey, naked);
}

// Remove a parameter
void Params::remove(const char *key)
{
	items.remove(key);
}

// Returns whether the parameter is naked (key w/o value).
// Undefined if key does not exist at all.
bool Params::is_key_naked(const char* key) const
{
	Buffer ukey(key);
	ukey.uppercase();
	return items.has(ukey) && items.get(ukey) == naked;
}

// Set packet ID. Used only when creating a new packet for tx.
void Params::set_ident(uint32_t new_ident)
{
	_ident = new_ident;
}
