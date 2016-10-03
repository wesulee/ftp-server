#pragma once

#include "representation_type.h"
#include <memory>
#include <boost/asio.hpp>


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
	void enablePassiveMode(std::shared_ptr<Response>);
	void passiveAccept(void);
private:
	void acceptCallback(const boost::system::error_code&, std::shared_ptr<socket_type>);
	std::string getPASVResponse(void);

	std::unique_ptr<boost::asio::ip::tcp::acceptor> acceptor;
	Session& session;
	Mode mode;
	RepresentationType reprType;
};


inline
void DTP::setRepresentationType(const RepresentationType type) {
	reprType = type;
}
