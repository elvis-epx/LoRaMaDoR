/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

// Class that encapsulates a buffer or string

#ifndef __BUFFER_H
#define __BUFFER_H

#include <cstddef>
#include <cstdint>

class Buffer {
public:
	Buffer();
	Buffer(int len);
	Buffer(const char *, int len);
	Buffer(const char *);
	Buffer(const Buffer&);
	Buffer(Buffer&&);
	Buffer& operator=(const Buffer&);
	Buffer& operator=(Buffer&&);
	~Buffer();

	static Buffer sprintf(const char*, ...);
	static Buffer millis_to_hms(int32_t);
	Buffer substr(size_t start) const;
	Buffer substr(size_t start, size_t end) const;

	bool empty() const;
	size_t length() const;
	const char* c_str() const;
	char* hot(size_t); /* for testing purposes only */
	Buffer& uppercase();
	bool operator==(const char *cmp) const;
	bool operator==(const Buffer &) const;
	bool operator!=(const char *cmp) const;
	bool operator!=(const Buffer &) const;
	int compareTo(const Buffer &) const;
	int compareTo(const char *cmp) const;
	bool startsWith(const char *cmp) const;
	bool startsWith(const Buffer &) const;
	Buffer& append(const char *s, size_t length);
	Buffer& append(const char s);
	Buffer& append_str(const Buffer&);
	Buffer& cut(int);
	Buffer& lstrip();
	Buffer& rstrip();
	Buffer& strip();
	int indexOf(const char) const;
	int charAt(int) const;

	friend class BufferImpl;
private:
	char *buf;
	size_t len;
};

#endif
