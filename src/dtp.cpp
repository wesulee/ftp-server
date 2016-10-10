#include "dtp.h"
#include "asio_data.h"
#include "data_response.h"
#include "file_reader.h"
#include "file_writer.h"
#include "mlsd_writer.h"
#include "path.h"
#include "response.h"
#include "server.h"
#include "session.h"
#include <algorithm>	// swap
#include <cassert>
#include <string>


namespace DTPHelper {

// note: address.to_bytes() returns byte array in network byte order
// h1,h2,h3,h4,p1,p2 where h1 is high order 8 bits of the internet host address
template<class T>
static std::string getPASVResponse(const T& bytes, unsigned short port) {
	const unsigned short portDiv = 256;
	std::string str{"Enter Passive Mode ("};
	assert(bytes.size() == 4);	// ipv4 address
	// address bytes
	for (const unsigned char n : bytes) {
		str.append(std::to_string(static_cast<unsigned int>(n)));
		str.append(",");
	}
	// port bytes
	str.append(std::to_string(static_cast<unsigned int>(port / portDiv)));
	str.append(",");
	str.append(std::to_string(static_cast<unsigned int>(port % portDiv)));
	str.append(")");
	return str;
}

}	// namespace DTPHelper


DTP::DTP(Session& sess)
: session{sess}, mode{Mode::_NONE}, reprType{RepresentationType::ASCII} {
}


void DTP::closeConnection() {
	assert(mode != Mode::_NONE);
	session.getDTPSocket().close();
	mode = Mode::_NONE;
}


void DTP::enablePassiveMode(std::shared_ptr<Response> resp) {
	// TODO reuse acceptor
	acceptor.reset(new acceptor_type{Server::instance()->getService()});
	const auto localAddress = session.getPISocket().local_endpoint().address();
	assert(localAddress.is_v4());
	boost::asio::ip::tcp::endpoint ep{localAddress, 0};
	acceptor->open(ep.protocol());
	acceptor->bind(ep);
	acceptor->listen();
	const unsigned short localPort = acceptor->local_endpoint().port();
	resp->append(DTPHelper::getPASVResponse(localAddress.to_v4().to_bytes(), localPort));
}


void DTP::passiveAccept() {
	std::shared_ptr<socket_type> sock{
		new socket_type{Server::instance()->getService()}
	};
	acceptor->async_accept(
		*sock,
		[this, sock](const boost::system::error_code& ec) {
			acceptCallback(ec, sock);
		}
	);
}


void DTP::setMLSDWriter(std::shared_ptr<DataResponse>& dataResp, const Path& p) {
	// TODO catch exceptions
	dataResp->dataWriter = std::shared_ptr<DataWriter>{
		new MLSDWriter{*dataResp, p}
	};
	setDefaultWriteCallback(dataResp->dataWriter);
	// PI will set appropriate finish callback
}


void DTP::setFileWriter(std::shared_ptr<DataResponse>& dataResp, const Path& p) {
	switch (mode) {
	case Mode::_NONE:
		// PI should have checked if data connection is active
		assert(false);
		break;
	case Mode::ACTIVE:
		// TODO not implemented
		assert(false);
		break;
	case Mode::PASSIVE:
		dataResp->dataWriter = std::shared_ptr<DataWriter>{
			new FileWriter{*dataResp, p}
		};
		setDefaultWriteCallback(dataResp->dataWriter);
		// PI will set appropriate finish callback
		break;
	}
}


void DTP::setFileReader(std::shared_ptr<DataResponse>& dataResp, const Path& p,
const std::string& name) {
	dataResp->dataReader = std::shared_ptr<DataReader>{
		new FileReader{*dataResp, p, name}
	};
	setDefaultReadCallback(dataResp->dataReader);
}


void DTP::setDefaultWriteCallback(std::shared_ptr<DataWriter>& writer) {
	writer->setWriteCallback(
		[this](const AsioData& asioData, std::shared_ptr<DataResponse> dataResp) {
			writeCallback(asioData, dataResp);
		}
	);
}


void DTP::setDefaultReadCallback(std::shared_ptr<DataReader>& reader) {
	reader->setReadCallback(
		[this](const AsioData& asioData, std::shared_ptr<DataResponse> dataResp) {
			readCallback(asioData, dataResp);
		}
	);
}


void DTP::writeCallback(const AsioData& asioData, std::shared_ptr<DataResponse> dataResp) {
	if ((asioData.ec.value() != 0) || !dataResp->dataWriter->good()) {
		dataResp->dataWriter->finish(asioData);
	}
	else if (!dataResp->dataWriter->done()) {
		dataResp->dataWriter->writeSome();
	}
	else {
		dataResp->dataWriter->finish(asioData);
	}
}


void DTP::readCallback(const AsioData& asioData, std::shared_ptr<DataResponse> dataResp) {
	if ((asioData.ec.value() != 0) || !dataResp->dataReader->good() || dataResp->dataReader->done()) {
		dataResp->dataReader->finish(asioData);
	}
	else {
		dataResp->dataReader->readSome();
	}
}


void DTP::acceptCallback(const boost::system::error_code& ec, std::shared_ptr<socket_type> sock) {
	if (ec.value() != 0) {
		// TODO
		assert(false);
		return;
	}
	if (sock->remote_endpoint().address() != session.getPISocket().remote_endpoint().address()) {
		// connection is not from correct address
		passiveAccept();	// retry
	}
	else {
		// success
		std::swap(session.getDTPSocket(), *sock);
		mode = Mode::PASSIVE;
		session.passiveEnabled();
	}
}
