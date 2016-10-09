#pragma once

#include <array>
#include <random>
#include <string>


namespace Constants {
	constexpr char configName[] = "config.yaml";
	constexpr char EOL[] = "\r\n";	// CR LF
	constexpr char SP[] = " ";
	constexpr std::size_t CMD_BUF_SZ = 2048;
	constexpr std::size_t FILE_BUF_SZ = (64 * 1024);
	constexpr std::array<const char*, 2> features = {"PASV", "MLSD"};
}


namespace ResponseString {
	constexpr char loginRequest[] = "Please login with USER and PASS.";
	constexpr char loginReqPass[] = "Please specify the password.";
	constexpr char loginIncorrect[] = "Login incorrect.";
	constexpr char loginSuccess[] = "Login successful.";
	constexpr char unknownCmd[] = "Unknown command.";
	constexpr char cannotChangeUser[] = "Cannot change user.";
	constexpr char badSequence[] = "Bad sequence of commands.";
	constexpr char invalidCmd[] = "Invalid command.";
	constexpr char reqDataConnection[] = "Use PORT or PASV first.";
	constexpr char incomingDirList[] = "Here comes the directory listing.";
	constexpr char dirListSuccess[] = "Directory send OK.";
	constexpr char cannotOpenFile[] = "Failed to open file.";
	constexpr char transComplete[] = "Transfer complete.";
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
	constexpr int fileOkayDataConn = 150;	// File status okay; about to open data connection.
	constexpr int commandOkay = 200;
	constexpr int systemStatus = 211;
	constexpr int serviceReady = 220;
	constexpr int closeDataConn = 226;	// Closing data connection. Requested file action successful.
	constexpr int enterPassiveMode = 227;
	constexpr int loggedIn = 230;
	constexpr int pathnameCreated = 257;	// success of MKD or PWD
	constexpr int userOkNeedPass = 331;
	constexpr int noDataConnection = 425;
	constexpr int syntaxError = 500;	// or unknown command
	constexpr int argumentSyntaxError = 501;
	constexpr int badSequence = 503;	// Bad sequence of commands
	constexpr int notLoggedIn = 530;
	constexpr int fileUnavailable = 550;
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
	std::string quote(const std::string&);
}
