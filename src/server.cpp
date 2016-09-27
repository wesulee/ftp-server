#include "server.h"
#include "session.h"
#include <cassert>
#include <limits>
#include <stdexcept>


std::shared_ptr<Server> Server::serverInstance;


static constexpr bool validPort(const int port) {
	return ((port > 0) && (port <= static_cast<int>(std::numeric_limits<unsigned short>::max())));
}


static constexpr bool validNumThreads(const int numThreads) {
	return (numThreads > 0);
}


// throws boost::system::system_error, std::invalid_argument
Server::Server(const int port, const int numThreads)
: acceptor{ios}, ios_work{new boost::asio::io_service::work{ios}} {
	assert(validPort(port));
	assert(validNumThreads(numThreads));
	if (!validPort(port))
		throw std::invalid_argument{std::string{"invalid port: "} + std::to_string(port)};
	if (!validNumThreads(numThreads))
		throw std::invalid_argument{std::string{"invalid numThreads: "} + std::to_string(numThreads)};
	boost::asio::ip::tcp::endpoint ep{
		boost::asio::ip::address_v4::any(),
		static_cast<unsigned short>(port)
	};
	acceptor.open(ep.protocol());
	acceptor.bind(ep);
	threads.reserve(static_cast<std::size_t>(numThreads));
	for (int i = 0; i < numThreads; ++i) {
		threads.emplace_back(
			[this]() {
				ios.run();
			}
		);
	}
}


void Server::run() {
	running = true;
	acceptor.listen();
	beginAccept();
}


void Server::stop() {
	boost::system::error_code ec;
	running = false;
	ios_work.reset(nullptr);
	acceptor.close(ec);
	ios.stop();
	for (auto& thread : threads)
		thread.join();
}


// each call to this method accepts a new connection
void Server::beginAccept() {
	if (!running)
		return;
	std::shared_ptr<Session> session{new Session{ios}};
	// A worker thread will run acceptCallback(), which should call beginAccept()
	acceptor.async_accept(
		session->getPISocket(),
		[this, session](const boost::system::error_code& ec) {
			acceptCallback(ec, session);
		}
	);
}


// add session to list of active sessions and begin handling
void Server::addSession(std::shared_ptr<Session>& s) {
	sessionsLock.lock();
	auto insert = sessions.insert(s);
	(void)insert;	// remove warning
	assert(insert.second);
	sessionsLock.unlock();
	// Begin handling session
	s->run();
}


void Server::removeSession(std::shared_ptr<Session>& s) {
	sessionsLock.lock();
	auto it = sessions.find(s);
	if (it == sessions.end()) {
		assert(false);
	}
	else {
		sessions.erase(it);
	}
	sessionsLock.unlock();
}


// returns nullptr if invalid username or password
User* Server::getUser(const std::string& user, const std::string& pass) {
	auto it = users.find(user);
	if (it == users.end()) {
		return nullptr;
	}
	const std::string saltedPass = (pass + it->second.salt);
	MD5Digest digest = MD5::getDigest(MD5::strToByteArray(saltedPass));
	if (digest == it->second.pass) {
		return &it->second;
	}
	else {
		return nullptr;
	}
}


void Server::acceptCallback(const boost::system::error_code& ec, std::shared_ptr<Session> s) {
	if (ec.value() != 0) {
		// TODO
	}
	else {
		addSession(s);
	}
	// always call beginAccept() to initialize another new connection
	beginAccept();
}
