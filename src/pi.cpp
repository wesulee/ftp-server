#include "pi.h"
#include "asio_data.h"
#include "data_response.h"
#include "data_writer.h"
#include "representation_type.h"
#include "response.h"
#include "server.h"
#include "session.h"
#include "utility.h"
#include <algorithm>	// copy
#include <cassert>
#include <stdexcept>
#include <utility>	// pair


namespace PIHelper {

static std::pair<RepresentationType, bool> parseReprType(const std::string& type) {
	std::pair<RepresentationType, bool> ret;
	ret.second = false;		// default value
	if (type.size() == 1) {
		switch (type[0]) {
		case 'A':
			ret.first = RepresentationType::ASCII;
			ret.second = true;
			break;
		case 'I':
			ret.first = RepresentationType::IMAGE;
			ret.second = true;
			break;
		default:
			break;
		}
	}
	return ret;
}

}	// namespace PIHelper


PI::PI(Session& s) : session{s} {
}


// Send 220 "Ready" reply
void PI::begin() {
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


std::shared_ptr<Response> PI::makeResponse() {
	std::shared_ptr<Response> resp{new Response{session, cmdStr}};
	cmdStr.clear();
	return resp;
}


void PI::setDefaultCallback(std::shared_ptr<Response>& resp) {
	resp->setCallback(
		[this](const AsioData& asioData, std::shared_ptr<Response> resp2) {
			writeCallback(asioData, resp2);
		}
	);
}


void PI::readCallback(const boost::system::error_code& ec, std::size_t nBytes) {
	if (ec.value() != 0) {
		// TODO
		assert(false);
		return;
	}
	if (!updateReadInput(nBytes)) {
		readSome();
		return;
	}
	std::shared_ptr<Response> resp = makeResponse();
	setDefaultCallback(resp);
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
	case Command::Name::TYPE:
		{
			const std::pair<RepresentationType, bool> reprType = PIHelper::parseReprType(
				resp->getCmd().getArg()
			);
			if (reprType.second) {
				session.setRepresentationType(reprType.first);
				resp->setCode(ReturnCode::commandOkay);
				resp->append("Switching to ");
				switch (reprType.first) {
				case RepresentationType::ASCII:
					resp->append("ASCII");
					break;
				case RepresentationType::IMAGE:
					resp->append("binary");
					break;
				}
				resp->append(" mode.");
			}
			else {
				resp->setCode(ReturnCode::syntaxError);
				resp->append(ResponseString::invalidCmd, sizeof(ResponseString::invalidCmd)-1);
			}
		}
		break;
	case Command::Name::PASV:
		if (resp->getCmd().getArg().empty()) {
			resp->setCode(ReturnCode::enterPassiveMode);
			session.passiveBegin(resp);
			// response message will be set by above call
		}
		else {
			resp->setCode(ReturnCode::argumentSyntaxError);
			resp->append(ResponseString::invalidCmd, sizeof(ResponseString::invalidCmd)-1);
		}
		break;
	case Command::Name::MLSD:
		// listing is sent through data connection
		if (!session.getDTPSocket().is_open()) {
			resp->setCode(ReturnCode::noDataConnection);
			resp->append(ResponseString::reqDataConnection, sizeof(ResponseString::reqDataConnection)-1);
		}
		else if (resp->getCmd().getArg().empty()) {
			// print cwd listing
			std::shared_ptr<DataResponse> dataResp{new DataResponse};
			dataResp->cmdResp = resp;
			session.setMLSDWriter(dataResp, session.getCWD());
			if (!dataResp->dataWriter) {
				// unable to get directory listing
				// TODO not implemented
				assert(false);
			}
			// DTP should have set the writeCallback of dataResp
			// PI should set the finish callback
			dataResp->dataWriter->setFinishCallback(
				[this](const AsioData& asioData, std::shared_ptr<DataResponse> dataResp2) {
					finishCallback(asioData, dataResp2);
				}
			);
			resp->setCallback(
				[this, dataResp](const AsioData& asioData, std::shared_ptr<Response> resp2) {
					(void)resp2;	// dataResp already contains associated Response
					writeCallback(asioData, dataResp);
				}
			);
			resp->setCode(ReturnCode::fileOkayDataConn);
			resp->append(ResponseString::incomingDirList, sizeof(ResponseString::incomingDirList)-1);
		}
		else {
			// argument should be directory path
			// TODO not implemented
			resp->setCode(ReturnCode::syntaxError);
			resp->append(ResponseString::invalidCmd, sizeof(ResponseString::invalidCmd)-1);
		}
		break;
	default:
		break;
	}
	resp->send();
}


void PI::writeCallback(const AsioData& asioData, std::shared_ptr<Response> resp) {
	if (asioData.ec.value() != 0) {
		// TODO
		assert(false);
		return;
	}
	if (!resp->done()) {
		resp->writeSome();
		return;
	}
	switch (resp->getCmd().getName()) {
	case Command::Name::PASV:
		// DTP has set up acceptor and is already listening.
		// Accept the expected incoming connection.
		session.passiveAccept();
		break;
	default:
		readSome();
		break;
	}
}


void PI::readCallback(const boost::system::error_code& ec,
std::size_t nBytes, std::shared_ptr<LoginData> data) {
	if (ec.value() != 0) {
		// TODO
		assert(false);
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


void PI::writeCallback(const AsioData& asioData, std::shared_ptr<Response> resp,
std::shared_ptr<LoginData> data) {
	if (asioData.ec.value() != 0) {
		// TODO
		assert(false);
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


void PI::writeCallback(const AsioData& asioData, std::shared_ptr<DataResponse> dataResp) {
	if (asioData.ec.value() != 0) {
		// TODO
		assert(false);
		return;
	}
	if (!dataResp->cmdResp->done()) {
		dataResp->cmdResp->writeSome();
		return;
	}
	switch (dataResp->cmdResp->getCmd().getName()) {
	case Command::Name::MLSD:
		// Initial response to MLSD has been sent. Now send listing.
		dataResp->dataWriter->send();
		break;
	default:
		assert(false);
		break;
	}
}


void PI::finishCallback(const AsioData& asioData, std::shared_ptr<DataResponse> dataResp) {
	if (asioData.ec.value() != 0) {
		// TODO
		assert(false);
		return;
	}
	switch (dataResp->cmdResp->getCmd().getName()) {
	case Command::Name::MLSD:
		// MLSD data response has been successfully sent.
		// Send success response over command connection.
		// Since finished writing over data connection, close data connection.
		session.closeDataConnection();
		{
			std::shared_ptr<Response> resp = dataResp->cmdResp;
			resp->clear();
			setDefaultCallback(resp);
			resp->setCode(ReturnCode::closeDataConn);
			resp->append(ResponseString::dirListSuccess, sizeof(ResponseString::dirListSuccess)-1);
			resp->send();
		}
		break;
	default:
		assert(false);
		break;
	}
}


// Updates members inputBuffer and cmdStr.
// Returns true if there is a command read. When this happens,
//   cmdStr will contain the complete command and inputBuffer's
//   contents will be cleared of the command read.
bool PI::updateReadInput(std::size_t nBytes) {
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


void PI::readSome() {
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


void PI::readSome(std::shared_ptr<LoginData> data) {
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
std::string PI::getFeaturesResp() {
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
