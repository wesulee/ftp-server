#include "protocol_interpreter.h"
#include "server.h"
#include "session.h"
#include "utility.h"
#include <algorithm>	// copy
#include <cassert>
#include <stdexcept>


ProtocolInterpreter::ProtocolInterpreter(Session& s) : session{s} {
}


// Send 220 "Ready" reply
void ProtocolInterpreter::begin() {
	const std::string message = Utility::generateServerResponseStr(
		220, Server::instance()->getWelcomeMessage()
	);
	std::shared_ptr<LoginData> data{new LoginData};
	boost::asio::async_write(
		session.getPISocket(),
		boost::asio::buffer(message),
		[this, data](const boost::system::error_code& ec, std::size_t nBytes) {
			writeCallback(ec, nBytes, data);
		}
	);
}


void ProtocolInterpreter::setCmd() {
	cmd = Command{cmdStr};
	cmdStr.clear();
}


void ProtocolInterpreter::readCallback(const boost::system::error_code& ec, std::size_t nBytes) {
	if (ec.value() != 0) {
		// TODO
	}
	else {
		if (updateReadInput(nBytes)) {
			setCmd();
			switch (cmd.getName()) {
			case Command::Name::_INVALID:
				setOutputBuffer(
					ReturnCode::syntaxError,
					ResponseString::unknownCmd,
					sizeof(ResponseString::unknownCmd)
				);
				write();
				break;
			case Command::Name::USER:
				// changing users not implemented
				setOutputBuffer(
					ReturnCode::badSequence,
					ResponseString::cannotChangeUser,
					sizeof(ResponseString::cannotChangeUser)
				);
				write();
				break;
			case Command::Name::PASS:
				// user has already logged in, bad sequence
				setOutputBuffer(
					ReturnCode::badSequence,
					ResponseString::badSequence,
					sizeof(ResponseString::badSequence)
				);
				write();
				break;
			case Command::Name::FEAT:
				if (!cmd.getArg().empty()) {
					setOutputBuffer(
						ReturnCode::argumentSyntaxError,
						ResponseString::invalidCmd,
						sizeof(ResponseString::invalidCmd)
					);
					write();
				}
				else {
					const std::string featStr = getFeaturesResp();
					boost::asio::async_write(
						session.getPISocket(),
						boost::asio::buffer(featStr),
						[this](const boost::system::error_code& ec2, std::size_t nBytes2) {
							writeCallback(ec2, nBytes2);
						}
					);
				}
				break;
			default:
				break;
			}
		}
		else {
			readSome();
		}
	}
}


void ProtocolInterpreter::writeCallback(const boost::system::error_code& ec, std::size_t nBytes) {
	if (ec.value() != 0) {
		// TODO
	}
	else {
		readSome();
	}
}


void ProtocolInterpreter::readCallback(const boost::system::error_code& ec,
std::size_t nBytes, std::shared_ptr<LoginData> data) {
	if (ec.value() != 0) {
		// TODO
	}
	else {
		bool flag;
		switch (data->state) {
		case LoginData::State::READ_USER:
			flag = true;	// continue reading if true
			// check if input buffer contains a finished command
			if (updateReadInput(nBytes)) {
				flag = false;
				setCmd();
				if (cmd.getName() == Command::Name::USER) {
					data->username = cmd.getArg();
					data->state = LoginData::State::RESP_USER;
					setOutputBuffer(
						ReturnCode::userOkNeedPass,
						ResponseString::loginReqPass,
						sizeof(ResponseString::loginReqPass)
					);
				}
				else {
					// until logged in, ignore all commands except USER and PASS
					setOutputBuffer(
						ReturnCode::notLoggedIn,
						ResponseString::loginRequest,
						sizeof(ResponseString::loginRequest)
					);
				}
				write(data);
			}
			if (flag) {
				// continue reading
				readSome();
			}
			break;
		case LoginData::State::READ_PASS:
			// USER was provided, response was sent, now expecting PASS
			flag = true;	// continue reading if true
			if (updateReadInput(nBytes)) {
				flag = false;
				setCmd();
				if (cmd.getName() == Command::Name::PASS) {
					data->password = cmd.getArg();
					data->state = LoginData::State::RESP_PASS;
					// validate login
					data->user = Server::instance()->getUser(data->username, data->password);
					if (data->user != nullptr) {
						// valid login provided, set Session state
						session.setUser(data->user);
						setOutputBuffer(
							ReturnCode::loggedIn,
							ResponseString::loginSuccess,
							sizeof(ResponseString::loginSuccess)
						);
					}
					else {
						// incorrect login, reset state
						data->state = LoginData::State::READ_USER;
						setOutputBuffer(
							ReturnCode::notLoggedIn,
							ResponseString::loginIncorrect,
							sizeof(ResponseString::loginIncorrect)
						);
					}
				}
				else {
					// Reset state to beginning since PASS must be provided
					//   immediately after USER.
					data->state = LoginData::State::READ_USER;
					setOutputBuffer(
						ReturnCode::notLoggedIn,
						ResponseString::loginRequest,
						sizeof(ResponseString::loginRequest)
					);
				}
				write(data);
			}
			if (flag)
				readSome(data);
			break;
		case LoginData::State::RESP_USER:
		case LoginData::State::RESP_PASS:
			assert(false);
			throw std::logic_error{"invalid login state"};
			break;
		}
	}
}


void ProtocolInterpreter::writeCallback(const boost::system::error_code& ec,
std::size_t nBytes, std::shared_ptr<LoginData> data) {
	if (ec.value() != 0) {
		// TODO
	}
	else {
		switch (data->state) {
		case LoginData::State::READ_USER:
			// A response was sent (welcome message, invalid login, invalid command),
			//   so keep trying to read USER command.
			readSome(data);
			break;
		case LoginData::State::RESP_USER:
			// response to USER command was sent, now read PASS
			data->state = LoginData::State::READ_PASS;
			readSome(data);
			break;
		case LoginData::State::READ_PASS:
			// When state is set to READ_PASS, it stays in read loop
			//   and eventually changes state and responds, so this
			//   case should never happen.
			assert(false);
			throw std::logic_error{"invalid login state"};
			break;
		case LoginData::State::RESP_PASS:
			// PASS response was just sent and user is now logged in.
			// Read next command.
			readSome();
			break;
		}
	}
}


// Updates members inputBuffer and cmdStr.
// Returns true if there is a command read. When this happens,
//   cmdStr will contain the complete command and inputBuffer's
//   contents will be cleared of the command read.
bool ProtocolInterpreter::updateReadInput(std::size_t nBytes) {
	bool retVal = false;	// EOL flag
	// check if input buffer contains a finished command
	const std::size_t newBufSz = (inputBuffer.sz + nBytes);
	std::size_t i;
	for (i = inputBuffer.sz; i < newBufSz; ++i) {
		if (inputBuffer.buf[i] == '\n') {
			// expecting a '\r' before the '\n' to be considered EOL
			if (i == 0) {
				if (!cmdStr.empty() && (cmdStr.back() == '\r')) {
					// cmdStr contains an unnecessary '\r' at end, so remove it
					cmdStr.pop_back();
					retVal = true;
					break;
				}
			}
			else {
				if (inputBuffer.buf[i-1] == '\r') {
					retVal = true;
					break;
				}
			}
		}
	}
	// If EOL was found, copy the contents of the command to cmdStr and
	//   shift the remaining content in the buffer.
	if (retVal) {
		// copy
		if (i > 1) {
			cmdStr.append(inputBuffer.buf.data(), i-1);
		}
		// shift if needed
		if (newBufSz > i+1) {
			std::copy(
				inputBuffer.buf.begin() + i + 1,
				inputBuffer.buf.begin() + newBufSz,
				inputBuffer.buf.begin()
			);
			inputBuffer.sz = (newBufSz - i - 1);
		}
		else {
			inputBuffer.sz = 0;
		}
	}
	else {
		if (newBufSz == inputBuffer.buf.size())
			inputBuffer.sz = 0;
		else
			inputBuffer.sz = newBufSz;
	}
	return retVal;
}


// str should be a NULL terminated string
// sz is length of str (including NULL termination)
void ProtocolInterpreter::setOutputBuffer(int code, const char* str, const std::size_t sz) {
	assert(sz > 0);
	assert(sz <= outputBuffer.buf.size());
	assert(Utility::validReturnCode(code));
	outputBuffer.sz = 0;
	const std::string codeStr = std::to_string(code);
	assert(codeStr.size() == 3);
	outputBuffer.append(codeStr.c_str(), 3);
	outputBuffer.append(Constants::SP, 1);
	outputBuffer.append(str, sz-1);
	outputBuffer.append(Constants::EOL, sizeof(Constants::EOL)-1);
}


void ProtocolInterpreter::readSome() {
	session.getPISocket().async_read_some(
		boost::asio::buffer(
			inputBuffer.buf.data() + inputBuffer.sz,
			inputBuffer.buf.size() - inputBuffer.sz
		),
		[this](const boost::system::error_code& ec, std::size_t nBytes) {
			readCallback(ec, nBytes);
		}
	);
}


void ProtocolInterpreter::write() {
	boost::asio::async_write(
		session.getPISocket(),
		boost::asio::buffer(
			outputBuffer.buf.data(),
			outputBuffer.sz
		),
		[this](const boost::system::error_code& ec, std::size_t nBytes) {
			writeCallback(ec, nBytes);
		}
	);
}


void ProtocolInterpreter::readSome(std::shared_ptr<LoginData> data) {
	session.getPISocket().async_read_some(
		boost::asio::buffer(
			inputBuffer.buf.data() + inputBuffer.sz,
			inputBuffer.buf.size() - inputBuffer.sz
		),
		[this, data](const boost::system::error_code& ec, std::size_t nBytes) {
			readCallback(ec, nBytes, data);
		}
	);
}


void ProtocolInterpreter::write(std::shared_ptr<LoginData> data) {
	boost::asio::async_write(
		session.getPISocket(),
		boost::asio::buffer(
			outputBuffer.buf.data(),
			outputBuffer.sz
		),
		[this, data](const boost::system::error_code& ec, std::size_t nBytes) {
			writeCallback(ec, nBytes, data);
		}
	);
}


// https://tools.ietf.org/html/rfc2389
std::string ProtocolInterpreter::getFeaturesResp() {
	std::string str{"211-Features"};
	str.append(Constants::EOL);
	for (const auto feat : Constants::features) {
		str.append(Constants::SP);
		str.append(feat);
		str.append(Constants::EOL);
	}
	str.append("211 End");
	str.append(Constants::EOL);
	return str;
}
