#pragma once

#include "path.h"
#include "dtp.h"
#include "pi.h"
#include <memory>
#include <string>
#include <boost/asio.hpp>


enum class RepresentationType;
class Response;
class User;


class Session {
public:
	Session(boost::asio::io_service&);
	~Session();
	PI& getPI(void);
	DTP& getDTP(void);
	boost::asio::ip::tcp::socket& getPISocket(void);
	boost::asio::ip::tcp::socket& getDTPSocket(void);
	void run(void);
	void setUser(User*);
	User* getUser(void);
	const Path& getCWD(void) const;
	void setRepresentationType(const RepresentationType);
	void closeDataConnection(void);
	void passiveBegin(std::shared_ptr<Response>);
	void passiveAccept(void);
	void passiveEnabled(void);
	void setMLSDWriter(std::shared_ptr<DataResponse>&, const Path&);
	void setFileWriter(std::shared_ptr<DataResponse>&, const Path&);
private:
	boost::asio::ip::tcp::socket socketPI;
	boost::asio::ip::tcp::socket socketDTP;
	PI pi;
	DTP dtp;
	Path cwd;	// absolute path of local directory
	User* user;
};


inline
PI& Session::getPI() {
	return pi;
}


inline
DTP& Session::getDTP() {
	return dtp;
}


inline
boost::asio::ip::tcp::socket& Session::getPISocket() {
	return socketPI;
}


inline
boost::asio::ip::tcp::socket& Session::getDTPSocket() {
	return socketDTP;
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
