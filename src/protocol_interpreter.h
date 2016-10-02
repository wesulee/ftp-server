#pragma once

#include "buffer.h"
#include <array>
#include <memory>
#include <string>
#include <boost/asio.hpp>


class AsioData;
class Response;
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
	Buffer& getOutputBuffer(void);
private:
	std::shared_ptr<Response> makeResponse(void);
	void readCallback(const boost::system::error_code&, std::size_t);
	void writeCallback(const AsioData&, std::shared_ptr<Response>);
	void readCallback(const boost::system::error_code&, std::size_t, std::shared_ptr<LoginData>);
	void writeCallback(const AsioData&, std::shared_ptr<Response>, std::shared_ptr<LoginData>);
	bool updateReadInput(std::size_t);
	void readSome(void);
	void readSome(std::shared_ptr<LoginData>);
	static std::string getFeaturesResp(void);

	Session& session;
	Buffer inputBuffer;
	Buffer outputBuffer;
	std::string cmdStr;
};


inline
Buffer& ProtocolInterpreter::getOutputBuffer() {
	return outputBuffer;
}
