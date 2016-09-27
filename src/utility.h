#pragma once

#include <random>
#include <string>


namespace Constants {
	constexpr char configName[] = "config.yaml";
	constexpr char EOL[] = "\r\n";	// CR LF
	constexpr char SP[] = " ";
	constexpr std::size_t CMD_BUF_SZ = 2048;
}


namespace ResponseString {
	constexpr char loginRequest[] = "Please login with USER and PASS.";
	constexpr char loginReqPass[] = "Please specify the password.";
	constexpr char loginIncorrect[] = "Login incorrect.";
	constexpr char loginSuccess[] = "Login successful.";
	constexpr char unknownCmd[] = "Unknown command.";
	constexpr char cannotChangeUser[] = "Cannot change user.";
	constexpr char badSequence[] = "Bad sequence of commands.";
}


/*
1xx Positive Preliminary reply
2xx Positive Completion reply
3xx Positive Intermediate reply
4xx Transient Negative Completion reply
5xx Permanent Negative Completion reply

x0x Syntax
x1x Information
x2x Connections
x3x Authentication and accounting
x4x Unspecified
x5x File system
*/
namespace ReturnCode {
	constexpr int loggedIn = 230;
	constexpr int userOkNeedPass = 331;
	constexpr int syntaxError = 500;
	constexpr int badSequence = 503;	// Bad sequence of commands
	constexpr int notLoggedIn = 530;
}


namespace Utility {
	class RNG {
	public:
		static void init(void);
		static char getSaltChar(void);
	private:
		static std::minstd_rand generator;
	};

	void init(const char*);
	std::string getPasswordSalt(const int);
	std::string generateServerResponseStr(const int, const std::string&);
	bool validReturnCode(const int);
}
