#pragma once

#include "protocol_interpreter.h"
#include <string>
#include <boost/asio.hpp>


class User;


class Session {
public:
	Session(boost::asio::io_service&);
	~Session();
	boost::asio::ip::tcp::socket& getPISocket(void);
	void run(void);
	void setUser(User*);
	const std::string& getCWD(void) const;
private:
	boost::asio::ip::tcp::socket socketPI;
	boost::asio::ip::tcp::socket socketDTP;
	ProtocolInterpreter pi;
	std::string cwd;
	User* user;
};


inline
boost::asio::ip::tcp::socket& Session::getPISocket() {
	return socketPI;
}


inline
void Session::run() {
	pi.begin();
}


inline
const std::string& Session::getCWD() const {
	return cwd;
}
