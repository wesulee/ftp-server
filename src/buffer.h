#pragma once

#include "utility.h"
#include <algorithm>	// copy
#include <array>
#include <string>


struct Buffer {
	typedef std::array<char, Constants::CMD_BUF_SZ> ArrayType;

	std::size_t size(void) const;
	void clear(void);
	void append(const char*, const std::size_t);
	std::string str(void) const;
	bool full(void) const;
	constexpr std::size_t capacity(void) const;
	const char& operator[](const std::size_t) const;

	ArrayType buf;
	std::size_t sz = 0;
};


inline
std::size_t Buffer::size() const {
	return sz;
}


inline
void Buffer::clear() {
	sz = 0;
}


// copies strSz bytes from str into buf
inline
void Buffer::append(const char* str, const std::size_t strSz) {
	std::copy(
		str,
		str + strSz,
		buf.data() + sz
	);
	sz += strSz;
}


inline
std::string Buffer::str() const {
	return std::string{buf.data(), sz};
}


inline
bool Buffer::full() const {
	return (sz == buf.size());
}


inline
constexpr std::size_t Buffer::capacity() const {
	return buf.size();
}


inline
const char& Buffer::operator[](const std::size_t i) const {
	return buf[i];
}
