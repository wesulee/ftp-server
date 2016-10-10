#pragma once

#include <cstddef>	// size_t
#include <fstream>
#include <memory>


// A wrapper for buffered writes to a file.
class OutputFileBuffer {
public:
	OutputFileBuffer();
	OutputFileBuffer(const OutputFileBuffer&) = delete;
	~OutputFileBuffer() {}
	void setFile(std::ofstream*);
	void setCapacity(const std::size_t);
	bool write(const char*, const std::size_t);
	void flush(void);
	OutputFileBuffer& operator=(const OutputFileBuffer&) = delete;
private:
	void doWrite(void);

	std::unique_ptr<char> buf;
	std::size_t capacity;	// allocation size of buf
	std::size_t size;		// number of valid bytes in buf
	std::ofstream* file;
};
