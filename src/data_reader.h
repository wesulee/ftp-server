#pragma once

#include <functional>
#include <memory>
#include <boost/asio.hpp>


class AsioData;
class Buffer;
class DataResponse;


// Base class for reading from data connection.
// Every DataReader is associated with a DataResponse.
class DataReader {
public:
	typedef std::function<void(const AsioData&, std::shared_ptr<DataResponse>)> Callback;

	DataReader(DataResponse&);
	virtual ~DataReader() = default;
	void setReadCallback(const Callback&);
	void setFinishCallback(const Callback&);
	virtual void receive(void) = 0;			// begin read loop
	virtual bool good(void) const = 0;		// is the reader itself in good condition?
	virtual void readSome(void) = 0;
	virtual bool done(void) const = 0;		// successfully finished reading?
	virtual void finish(const AsioData&);	// called after done() is true, or on error
protected:
	void doReadCallback(const boost::system::error_code&, std::size_t);

	Callback readCallback;
	Callback finishCallback;
	DataResponse& dataResp;
	Buffer& inputBuffer;
	std::size_t bytesReceived;
};


inline
void DataReader::setReadCallback(const Callback& c) {
	readCallback = c;
}


inline
void DataReader::setFinishCallback(const Callback& c) {
	finishCallback = c;
}
