#pragma once

#include "data_writer.h"
#include <string>
#include <vector>


class Path;


// MLSD command
class MLSDWriter : public DataWriter {
public:
	MLSDWriter(Session&, DataResponse&, const Path&);
	~MLSDWriter() = default;
	void send(void) override;
	bool good(void) const override;
	void writeSome(void) override;
	bool done(void) const override;
	void finish(const AsioData&) override;
private:
	void setupEntry(void);
	void updateEntry(void);
	void asioCallback(const boost::system::error_code&, std::size_t);

	std::vector<std::string> entries;
	std::size_t entriesIndex;
	std::size_t entryBytes;	// number of bytes written for current entry
	std::size_t bufIndex;	// index into buffer
	bool doneFlag;
};
