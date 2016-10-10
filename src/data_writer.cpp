#include "data_writer.h"
#include "asio_data.h"
#include "data_response.h"
#include "dtp.h"
#include "session.h"


DataWriter::DataWriter(DataResponse& dr)
: dataResp{dr}, outputBuffer{dr.session.getDTP().getOutputBuffer()},
bytesSent{0} {
}


void DataWriter::finish(const AsioData& asioData) {
	finishCallback(asioData, dataResp.getPtr());
}


// helper method
void DataWriter::doWriteCallback(const boost::system::error_code& ec, std::size_t nBytes) {
	writeCallback(AsioData{ec, nBytes}, dataResp.getPtr());
}
