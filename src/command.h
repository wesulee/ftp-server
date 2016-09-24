#pragma once

#include <string>
#include <unordered_map>


class Command {
public:
	enum class Name {_NONE, _INVALID, USER, PASS};

	Command();
	Command(const std::string&);
	~Command() = default;
	Name getName(void) const;
	const std::string& getArg(void) const;
	Command& operator=(const Command&);
	static Name parseName(const std::string&);
private:
	static std::unordered_map<std::string, Name> nameMap;
	Name name;
	std::string arg;
};


inline
Command::Name Command::getName() const {
	return name;
}


inline
const std::string& Command::getArg() const {
	return arg;
}


inline
Command& Command::operator=(const Command& o) {
	name = o.name;
	arg = o.arg;
	return *this;
}
