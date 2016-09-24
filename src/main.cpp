#include "server.h"
#include "utility.h"
#include <exception>
#include <iostream>
#include <limits>


int main() {
	Utility::RNG::init();
	try {
		Server::instance().reset(new Server{21, 1});
	}
	catch (const std::exception& e) {
		std::cerr << "Unable to start server." << std::endl
		          << "Reason: " << e.what() << std::endl;
		return 1;
	}
	Server::instance()->setWelcomeMessage("Welcome!");
	Server::instance()->run();
	std::this_thread::sleep_for(std::chrono::seconds(std::numeric_limits<int>::max()));
	Server::instance()->stop();

	return 0;
}
