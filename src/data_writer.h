#pragma once

#include <functional>
#include <memory>
#include <boost/asio.hpp>


class AsioData;
class Buffer;
class DataResponse;


// Base class for writing to data connection (does the actual writing).
// Every DataWriter is associated with a DataResponse.
// good() is used to notify when an error has occurred.
// writeSome(), done(), and finish() may only be called after send() has
//   been called.
// good() may be called prior to send().
// If an error occurs during send() call, send() is still required to initiate
//   a call to writeCallback, either directly or through an async write.
//
// The writing sequence is:
//   - writeSome() should create an async write, with the callback being a
//     private callback.
//   - The private callback updates members (if finished, then calls to done() should
//     be true), and then calls writeCallback with the appropriate AsioData.
//   - writeCallback must first check for errors provided by AsioData. If an error
//     occurred, call finish(). writeCallback should then call good(). If good() is
//     false, call finish(). writeCallback should then call done(). If done() is false,
//     it should continue the write loop by calling writeSome(). If done() is true,
//     call finish(). In all the cases of calling finish(), writeCallback will pass the
//     provided AsioData.
//   - finish()'s last instruction must be a call to DataWriter::finish(), which will
//     call finishCallback, passing the error_code provided to finish().
class DataWriter {
public:
	typedef std::function<void(const AsioData&, std::shared_ptr<DataResponse>)> Callback;

	DataWriter(DataResponse&);
	virtual ~DataWriter() = default;
	void setWriteCallback(const Callback&);
	void setFinishCallback(const Callback&);
	virtual void send(void) = 0;			// begin write loop
	virtual bool good(void) const = 0;		// is the writer itself in good condition?
	virtual void writeSome(void) = 0;
	virtual bool done(void) const = 0;		// successfully finished writing?
	virtual void finish(const AsioData&);	// called after done() is true, or on error
protected:
	void doWriteCallback(const boost::system::error_code&, std::size_t);

	Callback writeCallback;
	Callback finishCallback;
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
