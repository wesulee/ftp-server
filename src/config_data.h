#pragma once

#include "user.h"
#include <vector>


// Data from config file
// If User::homeDir is a relative path, it will be relative to executable
class ConfigData {
public:
	struct User {
		std::string name;
		std::string passHash;
		std::string passSalt;
		std::string homeDir;
	};

	static ConfigData getDefault(void);
	~ConfigData() = default;
	static ConfigData read(const std::string&);
	void write(const std::string&);
	int getPort(void) const;
	int getNumThreads(void) const;
private:
	ConfigData() = default;

	std::vector<User> users;
	int port;
	int maxNumConcurrentUsers;
	int numThreads;
};


inline
int ConfigData::getPort() const {
	return port;
}


inline
int ConfigData::getNumThreads() const {
	return numThreads;
}
