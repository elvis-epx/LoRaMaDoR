#ifndef __SERIAL_H
#define __SERIAL_H

#include <cstdint>

struct SerialClass {
	static int available();
	static int availableForWrite();
	static char read();
	static void write(const uint8_t* s, int len);
};

extern SerialClass Serial;

#endif
