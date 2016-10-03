#pragma once

#include "buffer.h"
#include "representation_type.h"
#include <memory>
#include <boost/asio.hpp>


class AsioData;
class DataResponse;
class Path;
class Response;
class Session;


// Data-transfer process
class DTP {
	typedef boost::asio::ip::tcp::socket socket_type;
public:
	enum class Mode {_NONE, ACTIVE, PASSIVE};

	DTP(Session&);
	~DTP() = default;
	void setRepresentationType(const RepresentationType);
	void closeConnection(void);
	void enablePassiveMode(std::shared_ptr<Response>);
	void passiveAccept(void);
	void setMLSDWriter(std::shared_ptr<DataResponse>&, const Path&);
	Buffer& getOutputBuffer(void);
private:
	void writeCallback(const AsioData&, std::shared_ptr<DataResponse>);
	void acceptCallback(const boost::system::error_code&, std::shared_ptr<socket_type>);

	std::unique_ptr<boost::asio::ip::tcp::acceptor> acceptor;
	Buffer inputBuffer;
	Buffer outputBuffer;
	Session& session;
	Mode mode;
	RepresentationType reprType;
};


inline
void DTP::setRepresentationType(const RepresentationType type) {
	reprType = type;
}


inline
Buffer& DTP::getOutputBuffer() {
	return outputBuffer;
}
