#pragma once

#include "command.h"
#include "utility.h"
#include <cassert>
#include <functional>
#include <memory>
#include <string>
#include <boost/asio.hpp>


class AsioData;
class Buffer;
class Session;


// The response to a command
class Response : public std::enable_shared_from_this<Response> {
public:
	typedef std::function<void(const AsioData&, std::shared_ptr<Response>)> Callback;

	Response(Session&);
	Response(Session&, const std::string&);
	~Response() = default;
	void setCode(const int);
	void setCallback(const Callback&);
	const Command& getCmd(void) const;
	void append(const char*, const std::size_t);
	void append(const std::string&);
	void set(const std::string&);
	void send(void);
	void writeSome(void);
	bool done(void) const;
	void clear(void);
private:
	std::shared_ptr<Response> getPtr(void);
	void append2(const char*, const std::size_t);
	void finalize(void);
	void updateEOL(void);
	void asioCallback(const boost::system::error_code&, std::size_t);

	Command command;
	Callback callback;
	std::string respTmp;
	Session& session;
	Buffer& outputBuffer;
	std::size_t bufIndex;
	std::size_t bytesSent;
	std::size_t outputSz;
	int code;
	bool doneFlag;
	bool format;
};


inline
void Response::setCode(const int c) {
	assert(Utility::validReturnCode(c));
	code = c;
}


inline
void Response::setCallback(const Callback& c) {
	callback = c;
}


inline
const Command& Response::getCmd() const {
	return command;
}


// appends sz bytes from str
// this is appending text that will be formatted automatically when send() called
// calling append() after set() will cause incorrectly formatted response
inline
void Response::append(const char* str, const std::size_t sz) {
	assert(format);
	// respTmp is used as temporary buffer until finalize() called
	respTmp.append(str, sz);
}


inline
void Response::append(const std::string& str) {
	append(str.c_str(), str.size());
}


// finished sending response?
inline
bool Response::done() const {
	return doneFlag;
}


inline
std::shared_ptr<Response> Response::getPtr() {
	return shared_from_this();
}
