#pragma once

#include <ostream>
#include <string>
#include <vector>


// Data from config file
// If User::homeDir is a relative path, it will be relative to executable
class ConfigData {
public:
	struct User {
		std::string name;
		std::string passSalt;
		std::string passHash;
		std::string homeDir;
	};

	ConfigData(const ConfigData&) = default;
	~ConfigData() = default;
	static ConfigData getDefault(void);
	static ConfigData read(const std::string&);
	void write(const std::string&);
	void addUser(const std::string&, const std::string&, const std::string&);
	int getPort(void) const;
	int getNumThreads(void) const;
	const std::string& getWelcomeMessage(void) const;
	const std::vector<User>& getUsers(void) const;
private:
	ConfigData() = default;
	void doWrite(std::ostream&);

	std::vector<User> users;
	std::string welcomeMessage;
	int port;
	int maxNumConcurrentUsers;
	int numThreads;
	int passSaltLen;
};


inline
int ConfigData::getPort() const {
	return port;
}


inline
int ConfigData::getNumThreads() const {
	return numThreads;
}


inline
const std::string& ConfigData::getWelcomeMessage() const {
	return welcomeMessage;
}


inline
const std::vector<ConfigData::User>& ConfigData::getUsers() const {
	return users;
}
