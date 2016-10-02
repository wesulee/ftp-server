#include "utility.h"
#include "config_data.h"
#include <cctype>
#include <chrono>
#define BOOST_FILESYSTEM_NO_DEPRECATED
#include <boost/filesystem.hpp>


std::minstd_rand Utility::RNG::generator;


namespace Utility {

void RNG::init() {
	const auto seedValue = static_cast<decltype(generator)::result_type>(
		std::chrono::duration_cast<std::chrono::seconds>(
			std::chrono::system_clock::now().time_since_epoch()
		).count()
	);
	generator.seed(seedValue);
}


char RNG::getSaltChar() {
	std::uniform_int_distribution<int> dist{48, 122};
	int c;
	do {
		c = dist(generator);
	} while (!std::isalnum(c));
	return static_cast<char>(c);
}


void init(const char* argv0) {
	namespace fs = boost::filesystem;
	RNG::init();
	// Update current working directory to executable directory
	const fs::path pathExe{fs::system_complete(fs::path{argv0})};
	const fs::path pathExeParent{pathExe.parent_path()};
	fs::current_path(pathExeParent);
	// Create default config file if one does not exist
	const fs::path configPath{std::string{Constants::configName}};
	fs::file_status configPathStatus = fs::status(configPath);
	if (!fs::exists(configPathStatus)) {
		ConfigData defaultConfig = ConfigData::getDefault();
		defaultConfig.write(configPath.string());
	}
}


std::string getPasswordSalt(const int length) {
	std::string str;
	str.reserve(static_cast<std::size_t>(length));
	for (int i = 0; i < length; ++i)
		str.push_back(RNG::getSaltChar());
	return str;
}


// if multi-line response, assumes new lines delimited by '\n'
std::string generateServerResponseStr(const int code, const std::string& response) {
	const std::string codeStr = std::to_string(code);
	if (response.empty())
		return (codeStr + Constants::EOL);
	// TODO
	return (codeStr + " " + response + Constants::EOL);
}


bool validReturnCode(const int code) {
	if ((code < 100) || (code >= 700))
		return false;
	const int y = ((code / 10) % 10);
	if (y > 5)
		return false;
	return true;
}


std::string quote(const std::string& str) {
	std::string ret;
	ret.reserve(str.size() + 2);
	ret.push_back('"');
	ret.append(str);
	ret.push_back('"');
	return ret;
}

}	// namespace Utility
