#pragma once

#include "command.h"
#include "command_buffer.h"
#include <array>
#include <memory>
#include <string>
#include <boost/asio.hpp>


class Session;
class User;


class ProtocolInterpreter {
	struct LoginData {
		enum class State {READ_USER, RESP_USER, READ_PASS, RESP_PASS};

		std::string username;
		std::string password;
		State state = State::READ_USER;
		User* user = nullptr;
	};
public:
	ProtocolInterpreter(Session&);
	~ProtocolInterpreter() = default;
	void begin(void);
private:
	void setCmd(void);
	void readCallback(const boost::system::error_code&, std::size_t);
	void writeCallback(const boost::system::error_code&, std::size_t);
	void readCallback(const boost::system::error_code&, std::size_t, std::shared_ptr<LoginData>);
	void writeCallback(const boost::system::error_code&, std::size_t, std::shared_ptr<LoginData>);
	bool updateReadInput(std::size_t);
	void setOutputBuffer(const int, const char*, const std::size_t);
	void readSome(void);
	void write(void);
	void readSome(std::shared_ptr<LoginData>);
	void write(std::shared_ptr<LoginData>);

	Session& session;
	CommandBuffer inputBuffer;
	CommandBuffer outputBuffer;
	Command cmd;
	std::string cmdStr;
	std::string resp;
};
