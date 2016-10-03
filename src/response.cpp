#include "response.h"
#include "asio_data.h"
#include "buffer.h"
#include "pi.h"
#include "session.h"
#include <algorithm>	// min
#include <utility>		// swap


namespace ResponseHelper {

template<class T, class V>
static void updateEOL(T& buf, V& target) {
	if (
		(buf[buf.size()-2] != Constants::EOL[0])
		|| (buf[buf.size()-1] != Constants::EOL[1])
	) {
		target.append(Constants::EOL, 2);
	}
}


template<class T, class V>
static void updateEOLSplitBuf(T& buf1, V& buf2) {
	if (
		(buf1[buf1.size()-1] != Constants::EOL[0])
		|| (buf2[0] != Constants::EOL[1])
	) {
		buf2.append(Constants::EOL, 2);
	}
}


template<class T, class V>
static void updateEOLSplitApp(T& buf1, V& buf2) {
	if (
		(buf1[buf1.size()-2] != Constants::EOL[0])
		|| (buf2[buf1.size()-1] != Constants::EOL[1])
	) {
		buf1.append(Constants::EOL, 1);
		buf2.append(Constants::EOL + 1, 1);
	}
}

}	// namespace ResponseHelper


Response::Response(Session& sess)
: session{sess}, outputBuffer{sess.getPI().getOutputBuffer()}, bufIndex{0},
bytesSent{0}, outputSz{0}, code{0}, doneFlag{false}, format{true} {
}


Response::Response(Session& sess, const std::string& cmdStr)
: Response{sess} {
	command = Command{cmdStr};
}


void Response::set(const std::string& str) {
	outputBuffer.clear();
	respTmp.clear();
	format = false;
	append2(str.c_str(), str.size());
}


void Response::send() {
	finalize();
	writeSome();
}


void Response::writeSome() {
	auto thisShared = getptr();
	session.getPISocket().async_write_some(
		boost::asio::buffer(
			outputBuffer.buf.data() + bufIndex,
			outputBuffer.size() - bufIndex
		),
		[thisShared, this](const boost::system::error_code& ec, std::size_t nBytes) {
			asioCallback(ec, nBytes);
		}
	);
}


void Response::append2(const char* str, const std::size_t sz) {
	assert(str && (sz > 0));
	if (!outputBuffer.full()) {
		if ((outputBuffer.size() + sz) > outputBuffer.capacity()) {
			// cannot append entire contents of str to buffer
			const std::size_t szBufApp = (outputBuffer.capacity() - outputBuffer.size());
			outputBuffer.append(str, szBufApp);
			respTmp.append(str + szBufApp, sz - szBufApp);
		}
		else {
			outputBuffer.append(str, sz);
		}
	}
	else {
		respTmp.append(str, sz);
	}
}


void Response::finalize() {
	assert(Utility::validReturnCode(code));
	assert(callback);
	assert(bytesSent == 0);
	if (format) {
		// do the text formatting
		outputBuffer.clear();
		std::string unformattedResp;
		std::swap(respTmp, unformattedResp);
		const std::string codeStr = std::to_string(code);
		assert(codeStr.size() == 3);
		outputBuffer.append(codeStr.c_str(), 3);
		if (!unformattedResp.empty()) {
			outputBuffer.append(Constants::SP, 1);
			append2(unformattedResp.c_str(), unformattedResp.size());
		}
	}
	updateEOL();
	outputSz = (outputBuffer.size() + respTmp.size());
}


// append EOL if does not end with EOL
void Response::updateEOL() {
	if (outputBuffer.full()) {
		// since outputBuffer is full, append EOL to respTmp if needed
		switch (respTmp.size()) {
		case 0:
			// EOL should be last 2 char of outputBuffer if there is EOL
			ResponseHelper::updateEOL(outputBuffer, respTmp);
			break;
		case 1:
			// EOL split over outputBuffer and respTmp if there is EOL
			ResponseHelper::updateEOLSplitBuf(outputBuffer, respTmp);
			break;
		default:
			// EOL should be last 2 char of respTmp if there is EOL
			ResponseHelper::updateEOL(respTmp, respTmp);
			break;
		}
	}
	else {
		// if EOL needs to be appended, it may or may not fit in outputBuffer
		if (outputBuffer.size() <= 1) {
			// outputBuffer cannot contain EOL
			outputBuffer.append(Constants::EOL, 2);
		}
		else if (outputBuffer.size() <= outputBuffer.capacity()-2) {
			// EOL should be last 2 char of outputBuffer if there is EOL
			// outputBuffer can contain EOL if it needs to be appended
			ResponseHelper::updateEOL(outputBuffer, outputBuffer);
		}
		else {
			// EOL should be last 2 char of outputBuffer if there is EOL
			// if EOL needs to be appended, split over outputBuffer and respTmp
			ResponseHelper::updateEOLSplitApp(outputBuffer, respTmp);
		}
	}
}


void Response::asioCallback(const boost::system::error_code& ec, std::size_t nBytes) {
	bytesSent += nBytes;
	bufIndex += nBytes;
	if (bytesSent >= outputSz) {
		assert(bytesSent == outputSz);
		doneFlag = true;
	}
	else if (bufIndex == outputBuffer.capacity()) {
		// reset outputBuffer
		bufIndex = 0;
		outputBuffer.clear();
		outputBuffer.append(
			respTmp.c_str(),
			std::min(respTmp.size(), outputBuffer.capacity())
		);
		respTmp = respTmp.substr(outputBuffer.size());
	}
	else {
		// nothing to do
	}
	callback(AsioData{ec, nBytes}, getptr());
}
