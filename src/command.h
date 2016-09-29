#pragma once

#include <string>
#include <unordered_map>


class Command {
public:
	enum class Name {_NONE, _INVALID, USER, PASS, FEAT};

	Command();
	Command(const Command&) = default;
	Command(const std::string&);
	~Command() = default;
	Name getName(void) const;
	const std::string& getArg(void) const;
	Command& operator=(const Command&) = default;
	static Name parseName(const std::string&);
private:
	static std::unordered_map<std::string, Name> nameMap;
	Name name;
	std::string arg;
};


inline
Command::Command() : name{Name::_NONE} {
}


inline
Command::Name Command::getName() const {
	return name;
}


inline
const std::string& Command::getArg() const {
	return arg;
}
