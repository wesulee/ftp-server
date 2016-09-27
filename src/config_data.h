#pragma once

#include <ostream>
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
	int getPort(void) const;
	int getNumThreads(void) const;
private:
	ConfigData() = default;
	void doWrite(std::ostream&);

	std::vector<User> users;
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
