#include "protocol_interpreter.h"
#include "asio_data.h"
#include "response.h"
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
	std::shared_ptr<LoginData> data{new LoginData};
	std::shared_ptr<Response> resp{new Response{session}};
	resp->setCode(220);
	resp->set(
		Utility::generateServerResponseStr(220, Server::instance()->getWelcomeMessage())
	);
	resp->setCallback(
		[this, data](const AsioData& asioData, std::shared_ptr<Response> resp2) {
			writeCallback(asioData, resp2, data);
		}
	);
	resp->send();
}


std::shared_ptr<Response> ProtocolInterpreter::makeResponse() {
	std::shared_ptr<Response> resp{new Response{session, cmdStr}};
	cmdStr.clear();
	return resp;
}


void ProtocolInterpreter::readCallback(const boost::system::error_code& ec, std::size_t nBytes) {
	if (ec.value() != 0) {
		// TODO
		return;
	}
	if (!updateReadInput(nBytes)) {
		readSome();
		return;
	}
	std::shared_ptr<Response> resp = makeResponse();
	resp->setCallback(
		[this](const AsioData& asioData, std::shared_ptr<Response> resp2) {
			writeCallback(asioData, resp2);
		}
	);
	switch (resp->getCmd().getName()) {
	case Command::Name::_INVALID:
		resp->setCode(ReturnCode::syntaxError);
		resp->append(ResponseString::unknownCmd, sizeof(ResponseString::unknownCmd)-1);
		break;
	case Command::Name::USER:
		// changing users not implemented
		resp->setCode(ReturnCode::badSequence);
		resp->append(ResponseString::cannotChangeUser, sizeof(ResponseString::cannotChangeUser)-1);
		break;
	case Command::Name::PASS:
		// user has already logged in, bad sequence
		resp->setCode(ReturnCode::badSequence);
		resp->append(ResponseString::badSequence, sizeof(ResponseString::badSequence)-1);
		break;
	case Command::Name::FEAT:
		if (resp->getCmd().getArg().empty()) {
			resp->setCode(ReturnCode::systemStatus);
			resp->set(getFeaturesResp());
		}
		else {
			resp->setCode(ReturnCode::argumentSyntaxError);
			resp->append(ResponseString::invalidCmd, sizeof(ResponseString::invalidCmd)-1);
		}
		break;
	case Command::Name::PWD:
		if (resp->getCmd().getArg().empty()) {
			resp->setCode(ReturnCode::pathnameCreated);
			resp->append(Utility::quote(session.getCWD().pwd(session.getUser()->home)));
		}
		else {
			resp->setCode(ReturnCode::argumentSyntaxError);
			resp->append(ResponseString::invalidCmd, sizeof(ResponseString::invalidCmd)-1);
		}
		break;
	default:
		break;
	}
	resp->send();
}


void ProtocolInterpreter::writeCallback(const AsioData& asioData, std::shared_ptr<Response> resp) {
	if (asioData.ec.value() != 0) {
		// TODO
		return;
	}
	if (!resp->done()) {
		resp->writeSome();
		return;
	}
	readSome();
}


void ProtocolInterpreter::readCallback(const boost::system::error_code& ec,
std::size_t nBytes, std::shared_ptr<LoginData> data) {
	if (ec.value() != 0) {
		// TODO
		return;
	}
	if (!updateReadInput(nBytes)) {
		readSome(data);
		return;
	}
	std::shared_ptr<Response> resp = makeResponse();
	resp->setCallback(
		[this, data](const AsioData& asioData, std::shared_ptr<Response> resp2) {
			writeCallback(asioData, resp2, data);
		}
	);
	switch (data->state) {
	case LoginData::State::READ_USER:
		// check if input buffer contains a finished command
		if (resp->getCmd().getName() == Command::Name::USER) {
			data->username = resp->getCmd().getArg();
			data->state = LoginData::State::RESP_USER;
			resp->setCode(ReturnCode::userOkNeedPass);
			resp->append(ResponseString::loginReqPass, sizeof(ResponseString::loginReqPass)-1);
		}
		else {
			// until logged in, ignore all commands except USER and PASS
			resp->setCode(ReturnCode::notLoggedIn);
			resp->append(ResponseString::loginRequest, sizeof(ResponseString::loginRequest)-1);
		}
		break;
	case LoginData::State::READ_PASS:
		// USER was provided, response was sent, now expecting PASS
		if (resp->getCmd().getName() == Command::Name::PASS) {
			data->password = resp->getCmd().getArg();
			data->state = LoginData::State::RESP_PASS;
			// validate login
			data->user = Server::instance()->getUser(data->username, data->password);
			if (data->user != nullptr) {
				// valid login provided, set Session state
				session.setUser(data->user);
				resp->setCode(ReturnCode::loggedIn);
				resp->append(ResponseString::loginSuccess, sizeof(ResponseString::loginSuccess)-1);
			}
			else {
				// incorrect login, reset state
				data->state = LoginData::State::READ_USER;
				resp->setCode(ReturnCode::notLoggedIn);
				resp->append(ResponseString::loginIncorrect, sizeof(ResponseString::loginIncorrect)-1);
			}
		}
		else {
			// Reset state to beginning since PASS must be provided
			//   immediately after USER.
			data->state = LoginData::State::READ_USER;
			resp->setCode(ReturnCode::notLoggedIn);
			resp->append(ResponseString::loginRequest, sizeof(ResponseString::loginRequest)-1);
		}
		break;
	case LoginData::State::RESP_USER:
	case LoginData::State::RESP_PASS:
		assert(false);
		throw std::logic_error{"invalid login state"};
		break;
	}
	resp->send();
}


void ProtocolInterpreter::writeCallback(const AsioData& asioData, std::shared_ptr<Response> resp,
std::shared_ptr<LoginData> data) {
	if (asioData.ec.value() != 0) {
		// TODO
		return;
	}
	if (!resp->done()) {
		resp->writeSome();
		return;
	}
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


void ProtocolInterpreter::readSome() {
	session.getPISocket().async_read_some(
		boost::asio::buffer(
			inputBuffer.buf.data() + inputBuffer.size(),
			inputBuffer.capacity() - inputBuffer.size()
		),
		[this](const boost::system::error_code& ec, std::size_t nBytes) {
			readCallback(ec, nBytes);
		}
	);
}


void ProtocolInterpreter::readSome(std::shared_ptr<LoginData> data) {
	session.getPISocket().async_read_some(
		boost::asio::buffer(
			inputBuffer.buf.data() + inputBuffer.size(),
			inputBuffer.capacity() - inputBuffer.size()
		),
		[this, data](const boost::system::error_code& ec, std::size_t nBytes) {
			readCallback(ec, nBytes, data);
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
