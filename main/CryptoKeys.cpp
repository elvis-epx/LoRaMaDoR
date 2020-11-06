/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2020 PU5EPX
 */

#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <inttypes.h>
#include "AES.h"

#include "CryptoKeys.h"
#include "NVRAM.h"
#include "ArduinoBridge.h"
#include "Console.h"

Buffer CryptoKeys::get_key()
{
	return arduino_nvram_crypto_psk_load();
}

#define PREAMBLE '1'
#define PREAMBLE_SIZE  1
#define LEN_SIZE  2
#define IV_LENGTH 8

// TODO cryptografically secure IV
void gen_iv(uint8_t* buffer, int len)
{
	for (int i = 0; i < len; ++i) {
		buffer[i] = arduino_random(0, 256);
	}
}

void CryptoKeys::encrypt(Buffer& b)
{
	Buffer key = CryptoKeys::get_key();
	if (key.empty()) return;

	AES256 aes256;
	uint8_t* ukey = (uint8_t*) calloc(sizeof(uint8_t), aes256.keySize());
	::memcpy(ukey, key.c_str(), key.length());
	aes256.setKey(ukey, aes256.keySize());
	::free(ukey);

	size_t gross = b.length() + PREAMBLE_SIZE + LEN_SIZE + IV_LENGTH;
	size_t blocks = (gross - 1) % aes256.blockSize() + 1;
	gross = blocks * gross;
	uint8_t* buffer_in = (uint8_t*) malloc(gross);
	uint8_t* buffer_out = (uint8_t*) malloc(gross);
	buffer_in[0] = PREAMBLE;
	buffer_in[1] = b.length() % 256;
	buffer_in[2] = b.length() / 256;
	gen_iv(buffer_in + PREAMBLE_SIZE + LEN_SIZE, IV_LENGTH);
	::memcpy(buffer_in + PREAMBLE_SIZE + LEN_SIZE + IV_LENGTH, b.c_str(), b.length());

	for (size_t i = 0; i < blocks; ++i) {
		aes256.encryptBlock(buffer_out, buffer_in);
	}

	::free(buffer_in);
	b = Buffer((char*) buffer_out, gross);
	::free(buffer_out);
}

bool CryptoKeys::decrypt(const char *cbuffer_in, const size_t gross, char **buffer_out, size_t *payload_length)
{
	Buffer key = CryptoKeys::get_key();
	if (key.empty()) return false;

	// if receiver does not know the key, it will try to parse a mangled
	// packet that starts with \0 so it will be most probably rejected

	// if the received packet is not encrypted, it will be either mangled
	// by the decryption process (and rejected by the parser) or, most
	// often, will be rejected because the alleged size does not match
	// the packet size.

	// TODO handle the situations above more robustly instead of
	// probabilisticly

	AES256 aes256;
	uint8_t* ukey = (uint8_t*) calloc(sizeof(uint8_t), aes256.keySize());
	::memcpy(ukey, key.c_str(), key.length());
	aes256.setKey(ukey, aes256.keySize());
	::free(ukey);

	if (gross < aes256.blockSize()) {
		logs("crypto", "packet too short");
		return false;
	}

	if (gross % aes256.blockSize() != 0) {
		logs("crypto", "packet not a multiple of block");
		return false;
	}

	size_t blocks = gross / aes256.blockSize();
	const uint8_t* buffer_in = (const uint8_t*) cbuffer_in;

	if (buffer_in[0] != PREAMBLE) {
		logs("crypto", "packet with unknown preamble");
		return false;
	}

	*payload_length = buffer_in[1] + buffer_in[2] * 256;
	size_t calc_gross = *payload_length + PREAMBLE_SIZE + LEN_SIZE + IV_LENGTH;
	size_t calc_blocks = (calc_gross - 1) % aes256.blockSize() + 1;

	if (blocks != calc_blocks) {
		logs("crypto", "block count incompatible with alleged payload length");
		return false;
	}

	*buffer_out = (char*) malloc(gross);
	for (size_t i = 0; i < blocks; ++i) {
		aes256.decryptBlock((uint8_t*) *buffer_out, buffer_in);
	}

	return true;
}
