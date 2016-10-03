#include "session.h"
#include "representation_type.h"
#include "response.h"
#include "user.h"


Session::Session(boost::asio::io_service& ios)
: socketPI{ios}, socketDTP{ios}, pi{*this}, dtp{*this}, user{nullptr} {
}


Session::~Session() {
	// do not delete user
}


void Session::setUser(User* usr) {
	assert(usr != nullptr);
	user = usr;
	cwd = Path{user->home};
}


void Session::setRepresentationType(const RepresentationType reprType) {
	dtp.setRepresentationType(reprType);
}


// start the process of enabling passive mode
void Session::passiveBegin(std::shared_ptr<Response> resp) {
	dtp.enablePassiveMode(resp);
}


// accept connections to socket
void Session::passiveAccept() {
	dtp.passiveAccept();
}


// notification that pasive mode enabled
void Session::passiveEnabled() {
	pi.resume();
}
