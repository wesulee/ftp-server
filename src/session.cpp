#include "session.h"
#include "user.h"


Session::Session(boost::asio::io_service& ios) : socketPI{ios}, socketDTP{ios}, pi{*this}, user{nullptr} {
}


Session::~Session() {
	// do not delete user
}


void Session::setUser(User* usr) {
	assert(usr != nullptr);
	user = usr;
	cwd = Path{user->home};
}
