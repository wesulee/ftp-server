#include "input_file_buffer.h"
#include <cassert>


InputFileBuffer::InputFileBuffer() : capacity{0}, file{nullptr} {

}


void InputFileBuffer::setFile(std::ifstream* ptr) {
	assert(!file);
	file = ptr;
}


void InputFileBuffer::setCapacity(const std::size_t c) {
	assert(c > 0);
	capacity = c;
	buf.reset(new char[capacity]);
}


// returns number of bytes written to dst
// writes at most dstSz bytes
// if this returns 0, then EOF or some error has occurred
std::size_t InputFileBuffer::read(char* dst, const std::size_t dstSz) {
	if (index == size) {
		fillBuf();
	}
	const std::size_t nBytes = std::min(dstSz, size - index);
	std::copy(
		buf.get() + index,
		buf.get() + index + nBytes,
		dst
	);
	index += nBytes;
	return nBytes;
}


// Note: resets index to 0
void InputFileBuffer::fillBuf() {
	index = 0;
	file->read(buf.get(), static_cast<std::ifstream::pos_type>(capacity));
	size = static_cast<std::size_t>(file->gcount());
	if (size == static_cast<std::size_t>(-1)) {
		size = 0;		// error has occurred
	}
}
