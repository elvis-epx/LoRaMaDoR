#ifndef __SERIAL_H
#define __SERIAL_H

#include <cstdint>

struct SerialClass {
	static int available();
	static int availableForWrite();
	static char read();
	static void write(const uint8_t* s, int len);
	static void emu_listen_handle();
	static void emu_conn_handle();
	static int emu_conn_socket();
	static int emu_listen_socket();
	static void emu_port(int);
};

extern SerialClass Serial;

#endif
