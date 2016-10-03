#include "dtp.h"

#include "response.h"
#include "server.h"
#include "session.h"
#include <algorithm>	// swap
#include <cassert>
#include <string>


namespace DTPHelper {

// note: address.to_bytes() returns byte array in network byte order
// h1,h2,h3,h4,p1,p2 where h1 is high order 8 bits of the internet host address
template<class T>
static std::string getPASVResponse(const T& bytes, unsigned short port) {
	const unsigned short portDiv = 256;
	std::string str{"Enter Passive Mode ("};
	assert(bytes.size() == 4);	// ipv4 address
	// address bytes
	for (const unsigned char n : bytes) {
		str.append(std::to_string(static_cast<unsigned int>(n)));
		str.append(",");
	}
	// port bytes
	str.append(std::to_string(static_cast<unsigned int>(port / portDiv)));
	str.append(",");
	str.append(std::to_string(static_cast<unsigned int>(port % portDiv)));
	str.append(")");
	return str;
}

}	// namespace DTPHelper


DTP::DTP(Session& sess)
: session{sess}, mode{Mode::_NONE}, reprType{RepresentationType::ASCII} {
}


void DTP::enablePassiveMode(std::shared_ptr<Response> resp) {
	assert(!acceptor);
	acceptor.reset(new boost::asio::ip::tcp::acceptor{Server::instance()->getService()});
	const auto localAddress = session.getPISocket().local_endpoint().address();
	assert(localAddress.is_v4());
	boost::asio::ip::tcp::endpoint ep{localAddress, 0};
	acceptor->open(ep.protocol());
	acceptor->bind(ep);
	acceptor->listen();
	const unsigned short localPort = acceptor->local_endpoint().port();
	resp->append(DTPHelper::getPASVResponse(localAddress.to_v4().to_bytes(), localPort));
}


void DTP::passiveAccept() {
	std::shared_ptr<socket_type> sock{
		new socket_type{Server::instance()->getService()}
	};
	acceptor->async_accept(
		*sock,
		[this, sock](const boost::system::error_code& ec) {
			acceptCallback(ec, sock);
		}
	);
}


void DTP::acceptCallback(const boost::system::error_code& ec, std::shared_ptr<socket_type> sock) {
	if (ec.value() != 0) {
		// TODO
		return;
	}
	if (sock->remote_endpoint().address() != session.getPISocket().remote_endpoint().address()) {
		// connection is not from correct address
		passiveAccept();	// retry
	}
	else {
		// success
		std::swap(session.getDTPSocket(), *sock);
		mode = Mode::PASSIVE;
		session.passiveEnabled();
	}
}
