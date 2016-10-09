#include "file_writer.h"
#include "buffer.h"
#include "session.h"
#include "utility.h"
#include <algorithm>	// min
#include <cassert>


// TODO handle case of empty file
FileWriter::FileWriter(Session& sess, DataResponse& dr, const Path& p)
: DataWriter{sess, dr}, path{p}, goodFlag{true} {
	file.open(path.string(), std::ifstream::binary | std::ifstream::ate);
	if (!file.is_open()) {
		goodFlag = false;
		return;
	}
	// get file size
	fileSz = static_cast<std::size_t>(file.tellg());
	if (fileSz == static_cast<std::size_t>(-1)) {
		goodFlag = false;
		return;
	}
	// reset file position to beginning
	file.seekg(0, file.beg);
	if (!file.good()) {
		goodFlag = false;
		return;
	}
}


void FileWriter::send() {
	assert(goodFlag);
	// setup file buffer
	fileBuf.setCapacity(std::min(fileSz, Constants::FILE_BUF_SZ));
	fileBuf.setFile(&file);
	refillOutputBuffer();
	writeSome();
}


bool FileWriter::good() const {
	return goodFlag;
}


void FileWriter::writeSome() {
	if (bufIndex == outputBuffer.size()) {
		refillOutputBuffer();
	}
	session.getDTPSocket().async_write_some(
		boost::asio::buffer(
			outputBuffer.data() + bufIndex,
			outputBuffer.size() - bufIndex
		),
		[this](const boost::system::error_code& ec, std::size_t nBytes) {
			asioCallback(ec, nBytes);
		}
	);
}


bool FileWriter::done() const {
	return (bytesSent >= fileSz);
}


void FileWriter::refillOutputBuffer() {
	bufIndex = 0;
	outputBuffer.setSize(fileBuf.read(outputBuffer.data(), outputBuffer.capacity()));
	if (file.fail())
		goodFlag = false;
}


void FileWriter::asioCallback(const boost::system::error_code& ec, std::size_t nBytes) {
	bytesSent += nBytes;
	bufIndex += nBytes;
	doWriteCallback(ec, nBytes);
}
