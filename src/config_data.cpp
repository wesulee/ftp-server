#include "config_data.h"
#include "md5.h"
#include "utility.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <thread>	// hardware_concurrency
#include <utility>	// pair
#define BOOST_FILESYSTEM_NO_DEPRECATED
#include <boost/filesystem.hpp>
#include <yaml-cpp/yaml.h>


namespace fs = boost::filesystem;


namespace ConfigDataDefaults {
	constexpr char welcomeMessage[] = "Welcome!";
	constexpr char name[] = "anonymous";
	constexpr char homeDir[] = "public_ftp";
	constexpr int serverPort = 21;
	constexpr int saltLength = 16;
}


namespace ConfigKeys {
	constexpr char port[] = "port";
	constexpr char maxNumConcurrentUsers[] = "maxUsers";
	constexpr char numThreads[] = "numThreads";
	constexpr char passSaltLen[] = "saltLen";
	constexpr char welcomeMessage[] = "welcomeMessage";
	constexpr char users[] = "users";
	constexpr char user_name[] = "name";
	constexpr char user_passSalt[] = "passSalt";
	constexpr char user_passHash[] = "passHash";
	constexpr char user_homeDir[] = "homeDir";
}


namespace ReadUtil {
	constexpr char errorMsg[] = "invalid config file";
	std::string getFileContents(const std::string&);
	std::pair<int, bool> toInt(const std::string&);
	std::string errorStrKey(const char*);
	std::string errorStrIntVal(const char*, const std::string&);
	std::string getValueStr(const YAML::Node&, const char*);
	int getValueInt(const YAML::Node&, const char*);
}


namespace WriteUtil {
	template<class K, class V>
	void writePair(YAML::Emitter&, const K, const V&);
	void writeUser(YAML::Emitter&, const ConfigData::User&);
	std::string errorStrIntVal(const char*, const std::string&);
}


namespace ReadUtil {

std::string getFileContents(const std::string& path) {
	std::ifstream f{path};
	if (!f.good()) {
		throw std::runtime_error{"unable to open file " + Utility::quote(path)};
	}
	// read entire contents of file into string
	std::stringstream ss;
	ss << f.rdbuf();
	return ss.str();
}


std::pair<int, bool> toInt(const std::string& str) {
	try {
		return std::make_pair(std::stoi(str), true);
	}
	catch (std::exception& e) {
		return std::make_pair(0, false);
	}
}


std::string errorStrKey(const char* key) {
	return (std::string{ReadUtil::errorMsg} + ": missing key " + Utility::quote(key));
}


std::string errorStrIntVal(const char* key, const std::string& val) {
	return (
		std::string{ReadUtil::errorMsg} + ": key " + Utility::quote(key)
		+ " invalid value " + Utility::quote(val)
	);
}


// throws runtime_error if missing key
std::string getValueStr(const YAML::Node& node, const char* key) {
	if (!node[key]) {
		throw std::runtime_error{errorStrKey(key)};
	}
	return node[key].as<std::string>();
}


// throws runtime_error if missing key or if invalid int
int getValueInt(const YAML::Node& node, const char* key) {
	const std::string valStr = getValueStr(node, key);
	std::pair<int, bool> val = toInt(valStr);
	if (val.second) {
		return val.first;
	}
	else {
		throw std::runtime_error{errorStrIntVal(key, valStr)};
	}
}

}	// namespace ReadUtil


namespace WriteUtil {

template<class K, class V>
void writePair(YAML::Emitter& out, const K key, const V& val) {
	out << YAML::Key << key
	    << YAML::Value << val;
}


void writeUser(YAML::Emitter& out, const ConfigData::User& user) {
	out << YAML::BeginMap;
	writePair(out, ConfigKeys::user_name, user.name);
	writePair(out, ConfigKeys::user_passSalt, user.passSalt);
	writePair(out, ConfigKeys::user_passHash, user.passHash);
	writePair(out, ConfigKeys::user_homeDir, user.homeDir);
	out << YAML::EndMap;
}

}	// namespace WriteUtil


ConfigData ConfigData::getDefault() {
	ConfigData data;
	data.port = ConfigDataDefaults::serverPort;
	data.numThreads = static_cast<int>(std::thread::hardware_concurrency());
	data.passSaltLen = ConfigDataDefaults::saltLength;
	data.welcomeMessage = ConfigDataDefaults::welcomeMessage;
	data.users.emplace_back();
	data.users.back().name = ConfigDataDefaults::name;
	data.users.back().passSalt = Utility::getPasswordSalt(data.passSaltLen);
	data.users.back().passHash = MD5::getDigest(data.users.back().passSalt).str();
	data.users.back().homeDir = ConfigDataDefaults::homeDir;
	return data;
}


// throws runtime_error
ConfigData ConfigData::read(const std::string& path) {
	ConfigData data;
	YAML::Node node = YAML::Load(ReadUtil::getFileContents(path));
	if (!node.IsMap())
		throw std::runtime_error{ReadUtil::errorMsg};
	data.port = ReadUtil::getValueInt(node, ConfigKeys::port);
	data.maxNumConcurrentUsers = ReadUtil::getValueInt(node, ConfigKeys::maxNumConcurrentUsers);
	data.numThreads = ReadUtil::getValueInt(node, ConfigKeys::numThreads);
	data.passSaltLen = ReadUtil::getValueInt(node, ConfigKeys::passSaltLen);
	data.welcomeMessage = ReadUtil::getValueStr(node, ConfigKeys::welcomeMessage);
	// read users
	if (!node[ConfigKeys::users])
		throw std::runtime_error{ReadUtil::errorStrKey(ConfigKeys::users)};
	const YAML::Node& usersSeq = node[ConfigKeys::users];
	if (!usersSeq.IsSequence())
		throw std::runtime_error{ReadUtil::errorMsg};
	User tmpUser;
	for (YAML::const_iterator it = usersSeq.begin(); it != usersSeq.end(); ++it) {
		if (!it->IsMap())
			throw std::runtime_error{ReadUtil::errorMsg};
		tmpUser.name = ReadUtil::getValueStr(*it, ConfigKeys::user_name);
		tmpUser.passSalt = ReadUtil::getValueStr(*it, ConfigKeys::user_passSalt);
		tmpUser.passHash = ReadUtil::getValueStr(*it, ConfigKeys::user_passHash);
		tmpUser.homeDir = ReadUtil::getValueStr(*it, ConfigKeys::user_homeDir);
		data.users.push_back(tmpUser);
	}
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
		fTmpPath.close();
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


// throws invalid_argument if user already exists
void ConfigData::addUser(const std::string& username, const std::string& password,
const std::string& homeDir) {
	// check if user exist
	for (const auto& user : users) {
		if (user.name == username) {
			throw std::invalid_argument{"user already exists"};
		}
	}
	users.emplace_back();
	users.back().name = username;
	users.back().passSalt = Utility::getPasswordSalt(passSaltLen);
	const std::string saltedPass = (password + users.back().passSalt);
	users.back().passHash = MD5::getDigest(saltedPass).str();
	users.back().homeDir = homeDir;
}


// does the writing
// To simplify things, emitter will store contents in memory, and
//   then do one write to os.
void ConfigData::doWrite(std::ostream& os) {
	YAML::Emitter out;
	out << YAML::BeginMap;
	// general
	WriteUtil::writePair(out, ConfigKeys::port, port);
	WriteUtil::writePair(out, ConfigKeys::maxNumConcurrentUsers, maxNumConcurrentUsers);
	WriteUtil::writePair(out, ConfigKeys::numThreads, numThreads);
	WriteUtil::writePair(out, ConfigKeys::passSaltLen, passSaltLen);
	WriteUtil::writePair(out, ConfigKeys::welcomeMessage, welcomeMessage);
	// users
	out << YAML::Key << ConfigKeys::users << YAML::Value << YAML::BeginSeq;
	for (const User& user : users)
		WriteUtil::writeUser(out, user);
	out << YAML::EndSeq;
	// finish
	out << YAML::EndMap;
	os << out.c_str();
	os.flush();
}
