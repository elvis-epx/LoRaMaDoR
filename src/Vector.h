//
// Based on Vector frm
// Zac Staples
// zacstaples (at) mac (dot) com
//
// Light implementation of a vector or list

#ifndef __VECTOR_H
#define __VECTOR_H

#include <stddef.h>
#include <string.h>

template<class T>
class Vector {
	size_t sz;
	T** elem;
	size_t space;

public:
	Vector() : sz(0), elem(0), space(0) {}
	Vector(const int s) : sz(0) {
		reserve(s);
	}
	
	Vector& operator=(const Vector&);
	Vector& operator=(Vector&&);
	Vector(const Vector&);
	Vector(Vector&&);
	
	~Vector() { 
		clear();
	}
	
	void clear();
	T& operator[](size_t n) { return *elem[n]; }
	const T& operator[](size_t n) const { return *elem[n]; }
	
	size_t count() const { return sz; }
	size_t capacity() const { return space; }
	
	void reserve(size_t newalloc);
	void push_back(const T& val);
	void remov(size_t pos);
	void insert(size_t pos, const T& val);
};

template<class T> 
Vector<T>& Vector<T>::operator=(const Vector& a) {
	if (this==&a) return *this;

	clear();

	// new array
	elem = new T*[a.count()];
	space = sz = a.count();

	// copy elements
	for(size_t i=0; i < sz; ++i) {
		elem[i] = new T(a[i]);
	}

	return *this;
}

template<class T> 
Vector<T>::Vector(const Vector& a) {
	if (this==&a) return;

	elem = new T*[a.count()];
	space = sz = a.count();

	// copy elements
	for(size_t i=0; i < sz; ++i) {
		elem[i] = new T(a[i]);
	}
}

template<class T> 
Vector<T>::Vector(Vector&& a) {
	elem = a.elem;
	space = a.space;
	sz = a.sz;

	a.elem = 0;
	a.space = 0;
	a.sz = 0;
}

template<class T> 
void Vector<T>::clear()
{
	for (size_t i=0; i<sz; ++i) delete elem[i];
	delete [] elem;
	elem = 0;
	sz = space = 0;
}

template<class T> 
Vector<T>& Vector<T>::operator=(Vector&& a) {
	if(this==&a) return *this;

	clear();

	elem = a.elem;
	sz = a.sz;
	space = a.sz;

	a.elem = 0;
	a.sz = 0;
	a.space = 0;
}

template<class T> void Vector<T>::reserve(size_t newalloc){
	if(newalloc <= space) return;

	T** p = new T*[newalloc];
	if (elem) {
		memcpy(p, elem, sizeof(T*) * sz);
		delete [] elem;
	}
	elem = p;
	space = newalloc;	
}

template<class T> void
Vector<T>::remov(size_t pos){
	if (pos >= 0 && pos < sz) {
		delete elem[pos];
		--sz;
		// move pointers
		for (size_t i = pos; i < sz; ++i) {
			elem[i] = elem[i+1];
		}
	}
}

template<class T> 
void Vector<T>::push_back(const T& val){
	if(space == 0) reserve(4);				//start small
	else if(sz==space) reserve(2*space);
	elem[sz] = new T(val);
	++sz;
}

template<class T> 
void Vector<T>::insert(size_t pos, const T& val){
	if (pos >= sz || pos < 0) {
		push_back(val);
		return;
	}

	if(space == 0) reserve(4);
	else if(sz==space) reserve(2*space);

	// move pointers
	for (size_t i = sz; i > pos; --i) {
		elem[i] = elem[i-1];
	}
	++sz;
	elem[pos] = new T(val);
}

#endif
