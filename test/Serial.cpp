#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "Serial.h"

SerialClass Serial;
static bool telnet_connected = false;
// FIXME link with Telnet server and fill implementations in

int SerialClass::available()
{
	// FIXME implement Telnet link
	return 0;
}	

int SerialClass::availableForWrite()
{
	// FIXME implement Telnet link
	return 999999;
}	

char SerialClass::read()
{
	// FIXME implement Telnet link
	return 0;
}

void SerialClass::write(const uint8_t *data, int len)
{
	// FIXME implement Telnet link
	char *tmp = (char*) calloc(1, len + 1);
	memcpy(tmp, data, len);
	printf("%s", tmp);
	free(tmp);
}
