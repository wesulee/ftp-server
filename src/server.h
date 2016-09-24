#pragma once

#include "user.h"
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <boost/asio.hpp>


class Session;


class Server {
public:
	static std::shared_ptr<Server>& instance(void);
	Server(const int, const int);
	~Server() = default;
	void run(void);
	void stop(void);
	void setUsers(const std::vector<User>&);
	void setWelcomeMessage(const std::string&);
	const std::string& getWelcomeMessage(void) const;
	void beginAccept(void);
	void addSession(std::shared_ptr<Session>&);
	void removeSession(std::shared_ptr<Session>&);
	User* getUser(const std::string&, const std::string&);
private:
	void acceptCallback(const boost::system::error_code&, std::shared_ptr<Session>);

	static std::shared_ptr<Server> serverInstance;
	boost::asio::io_service ios;
	boost::asio::ip::tcp::acceptor acceptor;
	std::unique_ptr<boost::asio::io_service::work> ios_work;
	std::vector<std::thread> threads;
	std::unordered_set<std::shared_ptr<Session>> sessions;
	std::unordered_map<std::string, User> users;
	std::string welcomeMessage;
	std::mutex sessionsLock;
	bool running = false;
};


inline
std::shared_ptr<Server>& Server::instance() {
	return serverInstance;
}


inline
void Server::setWelcomeMessage(const std::string& str) {
	welcomeMessage = str;
}


inline
const std::string& Server::getWelcomeMessage() const {
	return welcomeMessage;
}
