#pragma once

#include "data_reader.h"
#include "output_file_buffer.h"
#include "path.h"
#include <fstream>
#include <string>


// STOR command
// Reads a file from data connection and writes it to filesystem.
class FileReader : public DataReader {
public:
	FileReader(DataResponse&, const Path&, const std::string&);
	void receive(void) override;
	bool good(void) const override;
	void readSome(void) override;
	bool done(void) const override;
	void finish(const AsioData&) override;
private:
	void asioCallback(const boost::system::error_code&, std::size_t);

	OutputFileBuffer fileBuf;
	std::ofstream file;
	Path path;
	bool goodFlag;
	bool doneFlag;
};
