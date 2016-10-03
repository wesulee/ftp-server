#pragma once

#include <functional>
#include <memory>
#include <boost/asio.hpp>


class AsioData;
class Buffer;
class DataResponse;
class Session;


// Base class for writing to data connection (does the actual writing).
// Every DataWriter is associated with a DataResponse.
// finish() is called when done() is true or something went wrong (indicated by error_code).
// The writing sequence is:
//   - writeSome() should create an async write, with the callback being a
//     private callback.
//   - The private callback updates members (if finished, then calls to done() should
//     be true), and then calls writeCallback with the appropriate AsioData.
//   - writeCallback must call done(). If done() is false and no error, it must call
//     writeSome(). Otherwise, writeCallback must call finish(), passing the provided
//     AsioData.
//   - finish()'s last instruction must be a call to DataWriter::finish(), which will
//     call finishCallback, passing the error_code provided to finish().
class DataWriter {
public:
	typedef std::function<void(const AsioData&, std::shared_ptr<DataResponse>)> Callback;

	DataWriter(Session&, DataResponse&);
	virtual ~DataWriter() = default;
	void setWriteCallback(const Callback&);
	void setFinishCallback(const Callback&);
	virtual void send(void) = 0;
	virtual void writeSome(void) = 0;
	virtual bool done(void) const = 0;
	virtual void finish(const AsioData&);
protected:
	Callback writeCallback;
	Callback finishCallback;
	Session& session;
	DataResponse& dataResp;	// the data response associated with this
	Buffer& outputBuffer;
	std::size_t bytesSent;
};


inline
void DataWriter::setWriteCallback(const Callback& c) {
	writeCallback = c;
}


inline
void DataWriter::setFinishCallback(const Callback& c) {
	finishCallback = c;
}
