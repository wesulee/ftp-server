#pragma once

#include "path.h"
#include "protocol_interpreter.h"
#include <boost/asio.hpp>


class User;


class Session {
public:
	Session(boost::asio::io_service&);
	~Session();
	ProtocolInterpreter& getPI(void);
	boost::asio::ip::tcp::socket& getPISocket(void);
	void run(void);
	void setUser(User*);
	User* getUser(void);
	const Path& getCWD(void) const;
private:
	boost::asio::ip::tcp::socket socketPI;
	boost::asio::ip::tcp::socket socketDTP;
	ProtocolInterpreter pi;
	Path cwd;	// absolute path of local directory
	User* user;
};


inline
ProtocolInterpreter& Session::getPI() {
	return pi;
}


inline
boost::asio::ip::tcp::socket& Session::getPISocket() {
	return socketPI;
}


inline
void Session::run() {
	pi.begin();
}


inline
User* Session::getUser() {
	return user;
}


inline
const Path& Session::getCWD() const {
	return cwd;
}
