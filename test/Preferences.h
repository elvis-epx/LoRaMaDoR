#ifndef __PREFERENCES_H
#define __PREFERENCES_H

#include <cstdint>

struct Preferences {
	static void begin(const char*);
	static void begin(const char*, bool);
	static void end();
	static uint32_t getUInt(const char*);
	static void putUInt(const char*, uint32_t);
	static size_t getString(const char*, char*, size_t);
	static void putString(const char*, const char*);
};

#endif // __PREFERENCES_H
