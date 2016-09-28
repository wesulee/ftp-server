#include "config_data.h"
#include "server.h"
#include "user.h"
#include "utility.h"
#include <exception>
#include <iostream>
#include <limits>
#include <string>
#include <boost/program_options.hpp>


static void addUser() {
	ConfigData config = ConfigData::read(Constants::configName);
	std::string username;
	std::string password;
	std::string homeDir;

	std::cout << "user: ";
	std::getline(std::cin, username);
	std::cout << "password: ";
	std::getline(std::cin, password);
	std::cout << "home directory: ";
	std::getline(std::cin, homeDir);

	config.addUser(username, password, homeDir);
	config.write(Constants::configName);
}


// returns exit flag
static bool procArgs(const int argc, char** argv) {
	namespace po = boost::program_options;
	po::variables_map vm;
	po::options_description description{"Options"};
	description.add_options()
		("help", "print usage")
		("addUser", "add a new user")
	;
	try {
		po::store(po::parse_command_line(argc, argv, description), vm);
	}
	catch (po::error& e) {
		std::cerr << "Unable to process arguments: " << e.what() << std::endl;
		return true;
	}
	// process arguments
	if (vm.count("help")) {
		std::cout << description << std::endl;
		return true;
	}
	if (vm.count("addUser")) {
		addUser();
		return true;
	}
	return false;
}


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
	try {
		Utility::init(argv[0]);
		if (procArgs(argc, argv)) {
			return 0;
		}
	}
	catch (const std::exception& e) {
		std::cerr << "An error has occurred: " << e.what() << std::endl;
		return 1;
	}
	try {
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
