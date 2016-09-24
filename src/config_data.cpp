#include "config_data.h"
#include "utility.h"


namespace ConfigDataDefaults {
	constexpr char name[] = "anonymous";
	constexpr char homeDir[] = "public_ftp";
}


ConfigData ConfigData::getDefault() {
	ConfigData data;
	ConfigData::User defaultUser;
	defaultUser.name = ConfigDataDefaults::name;
	defaultUser.homeDir = ConfigDataDefaults::homeDir;
	data.users.push_back(defaultUser);
	data.port = Constants::defaultServerPort;
	data.numThreads = Constants::defaultNumServerThreads;
	return data;
}
