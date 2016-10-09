#include "command.h"


std::unordered_map<std::string, Command::Name> Command::nameMap = {
	{"USER", Name::USER}, {"PASS", Name::PASS}, {"FEAT", Name::FEAT},
	{"PWD", Name::PWD}, {"TYPE", Name::TYPE}, {"PASV", Name::PASV},
	{"MLSD", Name::MLSD}, {"RETR", Name::RETR}, {"SYST", Name::SYST}
};


Command::Command(const std::string& str) : Command{} {
	const std::size_t spIndex = str.find(' ');
	if (spIndex == std::string::npos) {
		name = parseName(str);
		// arg is empty
	}
	else {
		name = parseName(str.substr(0, spIndex));
		arg = str.substr(spIndex + 1);
	}
}


Command::Name Command::parseName(const std::string& str) {
	auto it = nameMap.find(str);
	if (it == nameMap.end()) {
		return Name::_INVALID;
	}
	else {
		return it->second;
	}
}
