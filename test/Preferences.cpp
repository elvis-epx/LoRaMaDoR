#include "Dict.h"
#include "Buffer.h"
#include "Preferences.h"

static Dict<Buffer> nvram;

void Preferences::begin(const char*)
{
}

void Preferences::begin(const char*, bool)
{
}

void Preferences::end()
{
}

uint32_t Preferences::getUInt(const char* key)
{
	if (!nvram.has(key)) {
		return 0;
	}
	return nvram[key].toInt();
}

void Preferences::putUInt(const char* key, uint32_t value)
{
	nvram[key] = Buffer::itoa(value);
}

size_t Preferences::getString(const char* key, char* value, size_t maxlen)
{
	if (!nvram.has(key) || maxlen == 0) {
		return 0;
	}

	const Buffer bvalue = nvram[key];
	if (bvalue.empty()) {
		value[0] = 0;
		return 1;
	}

	size_t i = bvalue.length() + 1; // with \0
	if (i > maxlen) {
		i = maxlen;
	}
	memcpy(value, bvalue.c_str(), i);

	return i;
}

void Preferences::putString(const char* key, const char* value)
{
	nvram[key] = value;
}
