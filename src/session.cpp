#include "session.h"
#include "representation_type.h"
#include "response.h"
#include "user.h"


namespace fs = boost::filesystem;


Session::Session(boost::asio::io_service& ios)
: socketPI{ios}, socketDTP{ios}, pi{*this}, dtp{*this}, user{nullptr} {
}


Session::~Session() {
	// do not delete user
}


void Session::setUser(User* usr) {
	assert(usr != nullptr);
	user = usr;
	cwd = user->home;
}


void Session::setRepresentationType(const RepresentationType reprType) {
	dtp.setRepresentationType(reprType);
}


void Session::closeDataConnection() {
	dtp.closeConnection();
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


void Session::setMLSDWriter(std::shared_ptr<DataResponse>& dataResp, const Path& p) {
	dtp.setMLSDWriter(dataResp, p);
}


void Session::setFileWriter(std::shared_ptr<DataResponse>& dataResp, const Path& p) {
	dtp.setFileWriter(dataResp, p);
}


// TODO better path handling
void Session::setFileReader(std::shared_ptr<DataResponse>& dataResp, const std::string& name) {
	std::string newName = name;
	if (!newName.empty() && (newName.front() == '/')) {
		newName = newName.substr(1);
	}
	dtp.setFileReader(dataResp, cwd, newName);

}
