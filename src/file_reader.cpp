#include "file_reader.h"
#include "buffer.h"
#include "data_response.h"
#include "session.h"
#include "utility.h"	// Constants
#include <cassert>


FileReader::FileReader(DataResponse& dr, const Path& dir, const std::string& name)
: DataReader{dr}, goodFlag{true}, doneFlag{false} {
	std::pair<Path, bool> reqPath = dir.create(name, file);
	if (!reqPath.second) {
		goodFlag = false;
		return;
	}
	path = reqPath.first;
}


void FileReader::receive() {
	assert(goodFlag);
	inputBuffer.clear();
	fileBuf.setCapacity(Constants::FILE_BUF_SZ);
	fileBuf.setFile(&file);
	readSome();
}


bool FileReader::good() const {
	return goodFlag;
}


void FileReader::readSome() {
	dataResp.session.getDTPSocket().async_read_some(
		boost::asio::buffer(
			inputBuffer.data() + inputBuffer.size(),
			inputBuffer.capacity() - inputBuffer.size()
		),
		[this](const boost::system::error_code& ec, std::size_t nBytes) {
			asioCallback(ec, nBytes);
		}
	);
}


// done only when data connection is closed by client
bool FileReader::done() const {
	return doneFlag;
}


void FileReader::finish(const AsioData& asioData) {
	// Ignore asioData since any error that may have occurred will have already been
	//   known because of asioCallback().
	if (file.good()) {
		if (inputBuffer.size() > 0) {
			fileBuf.write(inputBuffer.data(), inputBuffer.size());
		}
		fileBuf.flush();
	}
	file.close();
	DataReader::finish(asioData);
}


void FileReader::asioCallback(const boost::system::error_code& ec, std::size_t nBytes) {
	bytesReceived += nBytes;
	inputBuffer.setSize(inputBuffer.size() + nBytes);
	if (inputBuffer.full()) {
		// when inputBuffer is full, write contents to file buffer
		if (fileBuf.write(inputBuffer.data(), inputBuffer.size())) {
			// A file write was performed. Check file state.
			if (file.fail()) {
				goodFlag = false;
			}
		}
		inputBuffer.clear();
	}
	if (ec.value() != 0) {
		if (
			(ec == boost::asio::error::connection_reset)
			|| (ec == boost::asio::error::eof)
		) {
			// Client has closed data connection.
			doneFlag = true;
		}
		else {
			// unhandled error
			goodFlag = false;
			assert(false);
		}
	}
	doReadCallback(ec, nBytes);
}
