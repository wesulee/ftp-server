#pragma once

#include <cstddef>	// size_t
#include <fstream>
#include <memory>


// A wrapper for buffered reads from a file.
// This only calls read on file, so error checking must be done with
//   owner of file.
class InputFileBuffer {
public:
	InputFileBuffer();
	InputFileBuffer(const InputFileBuffer&) = delete;
	~InputFileBuffer() {}
	void setFile(std::ifstream*);
	void setCapacity(const std::size_t);
	std::size_t read(char*, const std::size_t);
	InputFileBuffer& operator=(const InputFileBuffer&) = delete;
private:
	void fillBuf(void);

	std::unique_ptr<char> buf;
	std::size_t capacity;	// allocation size of buf
	std::size_t size;	// number of valid bytes in buf
	std::size_t index;	// number of bytes read in current buf
	std::ifstream* file;
};
