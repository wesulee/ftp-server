#pragma once

#include "utility.h"
#include <algorithm>	// copy
#include <array>
#include <string>


struct CommandBuffer {
	typedef std::array<char, Constants::CMD_BUF_SZ> ArrayType;
	ArrayType buf;
	std::size_t sz = 0;

	void append(const char*, const std::size_t);
	std::string str(void) const;
};


// copies strSz bytes from str into buf
inline
void CommandBuffer::append(const char* str, const std::size_t strSz) {
	std::copy(
		str,
		str + strSz,
		buf.data() + sz
	);
	sz += strSz;
}


inline
std::string CommandBuffer::str() const {
	return std::string{buf.data(), sz};
}
