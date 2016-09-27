#include "config_data.h"
#include "md5.h"
#include "utility.h"
#include <fstream>
#define BOOST_FILESYSTEM_NO_DEPRECATED
#include <boost/filesystem.hpp>
#include <yaml-cpp/yaml.h>


namespace fs = boost::filesystem;


namespace ConfigDataDefaults {
	constexpr char name[] = "anonymous";
	constexpr char homeDir[] = "public_ftp";
	constexpr int serverPort = 21;
	constexpr int numServerThreads = 2;
	constexpr int saltLength = 16;
}


namespace ConfigKeys {
	constexpr char port[] = "port";
	constexpr char maxNumConcurrentUsers[] = "maxUsers";
	constexpr char numThreads[] = "numThreads";
	constexpr char passSaltLen[] = "saltLen";
	constexpr char users[] = "users";
	constexpr char user_name[] = "name";
	constexpr char user_passSalt[] = "passSalt";
	constexpr char user_passHash[] = "passHash";
	constexpr char user_homeDir[] = "homeDir";
}


template<class K, class V>
static void writePair(YAML::Emitter& out, const K key, const V& val) {
	out << YAML::Key << key
	    << YAML::Value << val;
}


static void writeUser(YAML::Emitter& out, const ConfigData::User& user) {
	out << YAML::BeginMap;
	writePair(out, ConfigKeys::user_name, user.name);
	writePair(out, ConfigKeys::user_passSalt, user.passSalt);
	writePair(out, ConfigKeys::user_passHash, user.passHash);
	writePair(out, ConfigKeys::user_homeDir, user.homeDir);
	out << YAML::EndMap;
}


ConfigData ConfigData::getDefault() {
	ConfigData data;
	data.port = ConfigDataDefaults::serverPort;
	data.numThreads = ConfigDataDefaults::numServerThreads;
	data.passSaltLen = ConfigDataDefaults::saltLength;
	data.users.emplace_back();
	data.users.back().name = ConfigDataDefaults::name;
	data.users.back().passSalt = Utility::getPasswordSalt(data.passSaltLen);
	data.users.back().passHash = MD5::getDigest(
		MD5::strToByteArray(data.users.back().passSalt)
	).str();
	data.users.back().homeDir = ConfigDataDefaults::homeDir;
	return data;
}


// Write config file to pathStr
// If pathStr already exists, writes to temporary file (appends .tmp to pathStr),
//   renames existing file (appends .old to pathStr), and renames temporary file
//   to pathStr.
void ConfigData::write(const std::string& pathStr) {
	const fs::path path{pathStr};
	fs::file_status pathStatus = fs::status(path);
	if (fs::exists(pathStatus)) {
		const fs::path tmpPath = (pathStr + ".tmp");
		const fs::path oldPath = (pathStr + ".old");
		std::ofstream fTmpPath{tmpPath.lexically_normal().string()};
		doWrite(fTmpPath);
		// if oldPath already exists, delete it
		fs::remove(oldPath);
		// rename
		fs::rename(path, oldPath);
		fs::rename(tmpPath, path);
	}
	else {
		std::ofstream fPath{path.lexically_normal().string()};
		doWrite(fPath);
	}
}


// does the writing
// To simplify things, emitter will store contents in memory, and
//   then do one write to os.
void ConfigData::doWrite(std::ostream& os) {
	YAML::Emitter out;
	out << YAML::BeginMap;
	// general
	writePair(out, ConfigKeys::port, port);
	writePair(out, ConfigKeys::maxNumConcurrentUsers, maxNumConcurrentUsers);
	writePair(out, ConfigKeys::numThreads, numThreads);
	writePair(out, ConfigKeys::passSaltLen, passSaltLen);
	// users
	out << YAML::Key << ConfigKeys::users << YAML::Value << YAML::BeginSeq;
	for (const User& user : users)
		writeUser(out, user);
	out << YAML::EndSeq;
	// finish
	out << YAML::EndMap;
	os << out.c_str();
	os.flush();
}
