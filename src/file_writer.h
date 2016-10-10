#pragma once

#include "data_writer.h"
#include "input_file_buffer.h"
#include "path.h"
#include <fstream>


// RETR command
// Reads a file from filesystem and writes it to data connection.
class FileWriter : public DataWriter {
public:
	FileWriter(DataResponse&, const Path&);
	void send(void) override;
	bool good(void) const override;
	void writeSome(void) override;
	bool done(void) const override;
private:
	void refillOutputBuffer(void);
	void asioCallback(const boost::system::error_code&, std::size_t);

	InputFileBuffer fileBuf;
	std::ifstream file;
	Path path;
	std::size_t fileSz;
	std::size_t bufIndex;	// outputBuffer index
	bool goodFlag;
};
