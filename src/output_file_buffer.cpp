#include "output_file_buffer.h"
#include <algorithm>	// min
#include <cassert>


OutputFileBuffer::OutputFileBuffer() : capacity{0}, size{0}, file{nullptr} {
}


void OutputFileBuffer::setFile(std::ofstream* ptr) {
	assert(!file);
	file = ptr;
}


void OutputFileBuffer::setCapacity(const std::size_t c) {
	assert(c > 0);
	assert(!buf);
	capacity = c;
	buf.reset(new char[capacity]);
}


// returns true if a write was performed
// when buf becomes full, write to file immediately
bool OutputFileBuffer::write(const char* data, const std::size_t dataSz) {
	std::size_t remDataSz = dataSz;
	bool ret = false;
	while (remDataSz > 0) {
		const std::size_t appendSz = std::min(remDataSz, capacity - size);
		remDataSz -= appendSz;
		std::copy(
			data,
			data + appendSz,
			buf.get() + size
		);
		data += appendSz;
		size += appendSz;
		if (size == capacity) {
			doWrite();
			ret = true;
		}
	}
	return ret;
}


void OutputFileBuffer::flush() {
	if (size > 0) {
		doWrite();
	}
}


void OutputFileBuffer::doWrite() {
	assert(size > 0);
	file->write(buf.get(), static_cast<std::ofstream::pos_type>(size));
	size = 0;
}
