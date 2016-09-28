#include "config_data.h"
#include "server.h"
#include "user.h"
#include "utility.h"
#include <exception>
#include <iostream>
#include <limits>


static void initServer() {
	ConfigData config = ConfigData::read(Constants::configName);
	std::vector<User> users;
	for (const auto& user : config.getUsers()) {
		users.emplace_back();
		users.back().pass = MD5Digest{user.passHash};
		users.back().name = user.name;
		users.back().salt = user.passSalt;
		users.back().home = user.homeDir;
	}
	Server::instance().reset(new Server{
		config.getPort(),
		config.getNumThreads(),
		config.getWelcomeMessage()
	});
	Server::instance()->setUsers(users);
}


int main(int argc, char** argv) {
	(void)argc;
	try {
		Utility::init(argv[0]);
		initServer();
	}
	catch (const std::exception& e) {
		std::cerr << "Unable to start server." << std::endl
		          << "Reason: " << e.what() << std::endl;
		return 1;
	}

	Server::instance()->run();
	std::this_thread::sleep_for(std::chrono::seconds(std::numeric_limits<int>::max()));
	Server::instance()->stop();

	return 0;
}
