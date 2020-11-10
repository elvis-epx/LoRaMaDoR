/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2020 PU5EPX
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <cassert>
#include "AES.h"
#include "sha256.h"

#include "CryptoKeys.h"
#include "NVRAM.h"
#include "ArduinoBridge.h"
#include "Console.h"

static bool valid = false;
static Buffer psk;

Buffer CryptoKeys::get_key()
{
	if (!valid) {
		psk = arduino_nvram_crypto_psk_load();
		valid = true;
	}
	return psk;
}

#define MAGIC 0x05
#define LENGTH_LEN  2

// TODO cryptografically secure IV
void gen_iv(uint8_t* buffer, int len)
{
	buffer[0] = MAGIC;
	for (int i = 1; i < len; ++i) {
		buffer[i] = arduino_random(0, 256);
	}
}

void CryptoKeys::encrypt(Buffer& b)
{
	Buffer key = CryptoKeys::get_key();
	if (key.empty()) return;
	_encrypt(key, b);
}

void CryptoKeys::_encrypt(const Buffer& key, Buffer& b)
{
	assert(key.length() == 32);

	AES256 aes256;
	uint8_t* ukey = (uint8_t*) calloc(sizeof(uint8_t), aes256.keySize());
	::memcpy(ukey, key.c_str(), key.length());
	aes256.setKey(ukey, aes256.keySize());
	::free(ukey);

	size_t payload_len = b.length();
	size_t tot_len = aes256.blockSize() + LENGTH_LEN + payload_len;
	size_t enc_blocks = (tot_len - 1) / aes256.blockSize() + 1;
	tot_len = enc_blocks * aes256.blockSize();

	uint8_t* buffer = (uint8_t*) calloc(1, tot_len);
	gen_iv(buffer, aes256.blockSize());
	buffer[aes256.blockSize() + 0] = payload_len % 256;
	buffer[aes256.blockSize() + 1] = payload_len / 256;
	::memcpy(buffer + aes256.blockSize() + LENGTH_LEN, b.c_str(), payload_len);

	for (size_t i = 1; i < enc_blocks; ++i) {
		size_t offset_ant = (i - 1) * aes256.blockSize();
		size_t offset = i * aes256.blockSize();
		// pre-scramble using IV or previous block
		for (size_t j = 0; j < aes256.blockSize(); ++j) {
			buffer[offset + j] ^= buffer[offset_ant + j];
		}
		// encrypt
		aes256.encryptBlock(buffer + offset, buffer + offset);
	}

	b = Buffer((char*) buffer, tot_len);
	::free(buffer);
}

int CryptoKeys::decrypt(const char *cbuffer_in, const size_t tot_len,
			char **buffer_out, size_t *payload_len)
{
	Buffer key = CryptoKeys::get_key();

	if (key.empty()) {
		buffer_out = 0;
		payload_len = 0;
		if (tot_len > 0 && cbuffer_in[0] == MAGIC) {
			return CryptoKeys::ERR_ENCRYPTED;
		}
		return CryptoKeys::OK_CLEARTEXT;
	}

	if (tot_len > 0 && cbuffer_in[0] != MAGIC) {
		return CryptoKeys::ERR_NOT_ENCRYPTED;
	}
	return _decrypt(key, cbuffer_in, tot_len, buffer_out, payload_len);
}

int CryptoKeys::_decrypt(const Buffer& key, const char *cbuffer_in, const size_t tot_len,
				char **buffer_out, size_t *payload_len)
{
	// if receiver has the wrong key, the payload will be mangled and will
	// be most probably rejected

	// TODO handle situations above more robustly instead of probabilisticly

	assert(key.length() == 32);

	AES256 aes256;
	uint8_t* ukey = (uint8_t*) calloc(sizeof(uint8_t), aes256.keySize());
	::memcpy(ukey, key.c_str(), key.length());
	aes256.setKey(ukey, aes256.keySize());
	::free(ukey);

	if (tot_len < (2 * aes256.blockSize())) {
		logs("crypto", "packet too short");
		return CryptoKeys::ERR_DECRIPTION;
	}

	if (tot_len % aes256.blockSize() != 0) {
		logs("crypto", "packet not a multiple of block");
		return CryptoKeys::ERR_DECRIPTION;
	}

	size_t blocks = tot_len / aes256.blockSize();
	const uint8_t* buffer_in = (const uint8_t*) cbuffer_in;

	uint8_t* buffer_interm = (uint8_t*) malloc(tot_len);
	::memcpy(buffer_interm, buffer_in, tot_len);

	for (size_t i = blocks - 1; i >= 1; --i) {
		size_t offset = i * aes256.blockSize();
		size_t offset_ant = (i - 1) * aes256.blockSize();
		// decrypt
		aes256.decryptBlock(buffer_interm + offset, buffer_interm + offset);
		// de-scramble based on previous block
		for (size_t j = 0; j < aes256.blockSize(); ++j) {
			buffer_interm[offset + j] ^= buffer_interm[offset_ant + j];
		}
	}

	/*
	printf("After decryption \n");
	for (size_t i = 0; i < tot_len; ++i) {
		printf("%02x ", buffer_interm[i]);
	}
	printf("\n");
	*/
	
	*payload_len = buffer_interm[aes256.blockSize() + 0] +
			buffer_interm[aes256.blockSize() + 1] * 256;
	size_t calc_enc_len = aes256.blockSize() + LENGTH_LEN + *payload_len;
	size_t calc_blocks = (calc_enc_len - 1) / aes256.blockSize() + 1;

	if (blocks != calc_blocks) {
		logs("crypto", "block count incompatible with alleged payload length");
		::free(buffer_interm);
		return CryptoKeys::ERR_DECRIPTION;
	}

	*buffer_out = (char*) malloc(*payload_len);
	::memcpy(*buffer_out, buffer_interm + aes256.blockSize() + LENGTH_LEN, *payload_len);
	::free(buffer_interm);

	return CryptoKeys::OK_DECRYPTED;
}

void CryptoKeys::invalidate()
{
	valid = false;
}

static const char* hex = "0123456789abcdef";

Buffer CryptoKeys::hash_key(const Buffer& key)
{
	Sha256 hash;
	hash.init();
	hash.write(2);
	for (size_t i = 0; i < key.length(); ++i) {
		hash.write((uint8_t) key.charAt(i));
	}
	uint8_t* res = hash.result();
	char b64[32];
	for (size_t i = 0; i < 16; ++i) {
		b64[i*2+0] = hex[(res[i] >> 4) & 0xf];
		b64[i*2+1] = hex[res[i] & 0xf];
	}
	return Buffer(b64, 32);
}
