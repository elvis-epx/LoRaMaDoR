/*
 * LoRaMaDoR (LoRa-based mesh network for hams) project
 * Copyright (c) 2019 PU5EPX
 */

// Light implementation of an associative array

#ifndef _ODICT_H
#define _ODICT_H

#include "Vector.h"
#include "Buffer.h"

template <class T> class Dict {
public:
	Dict() {}

	Dict(const Dict& model) {
		_keys = model._keys;
		_values = model._values;
	}

	Dict& operator=(const Dict& model) {
		_keys = model._keys;
		_values = model._values;
		return *this;
	}

	~Dict() {}

	bool has(const char* key) const {
		return indexOf(key) != -1;
	}

	bool has(const Buffer& key) const {
		return has(key.c_str());
	}

	const T& get(const char* key) const {
		return _values[indexOf(key)];
	}

	const T& get(const Buffer& key) const {
		return get(key.c_str());
	}

	const T& operator[](const char* key) const {
		return _values[indexOf(key)];
	}

	const T& operator[](const Buffer& key) const {
		return operator[](key.c_str());
	}

	void remove(const char* key) {
		int i = indexOf(key);
		if (i < 0) {
			return;
		}
		_keys.remov((unsigned) i);
		_values.remov((unsigned) i);
	}

	void remove(const Buffer& key) {
		return remove(key.c_str());
	}

	// due to limitations of C++, we can't avoid creating a new
	// empty element when an unknown subscript is request
	T& operator[](const char* key) {
		if (!has(key)) {
			put(key, T());
		}
		return _values[indexOf(key)];
	}

	T& operator[](const Buffer& key) {
		if (!has(key)) {
			put(key, T());
		}
		return _values[indexOf(key)];
	}

	bool put(const Buffer &akey, const T& value) {
		return put(akey.c_str(), value);
	}

	bool put(const char *key, const T& value) {
		int pos = indexOf(key);
		bool new_key = pos <= -1;

		if (new_key) {
			int insertion_pos = indexOf(key, 0, _keys.size(), false);
			_keys.insert(insertion_pos, key);
			_values.insert(insertion_pos, value);
		} else {
			_keys[pos] = key;	
			_values[pos] = value;
		}
	
		return new_key;
	}

	size_t count() const {
		return _keys.size();
	}

	const Vector<Buffer>& keys() const{
		return _keys;
	}

	int indexOf(const char *key, size_t from, size_t to, bool exact) const {
		if ((to - from) <= 3) {
			// linear search
			for (size_t i = from; i < to; ++i) {
				int cmp = _keys[i].compareTo(key);
				if (cmp == 0) {
					// good for exact and non-exact queriers
					return (int) i;
				} else if ((! exact) && (cmp > 0)) {
					// non-exact, looks for insertion point
					return (int) i;
				}
			}

			return exact ? -1 : (int) to;
		}

		// recursive binary search
		size_t middle = (from + to) / 2;
		int cmp = _keys[middle].compareTo(key);
		if (cmp == 0) {
			return (int) middle;
		} else if (cmp > 0) {
			// middle key > our key, look into left
			return indexOf(key, from, middle, exact);
		} else {
			// middle key < our key, look into right 
			return indexOf(key, middle + 1, to, exact);
		}

		return -1;
	}

	int indexOf(const Buffer &key) const {
		return indexOf(key.c_str());
	}

	int indexOf(const char *key) const {
		return indexOf(key, 0, _keys.size(), true);
	}

private:
	Vector<Buffer> _keys;
	Vector<T> _values;
};

#endif
