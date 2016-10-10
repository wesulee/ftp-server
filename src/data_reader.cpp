#include "data_reader.h"
#include "asio_data.h"
#include "data_response.h"
#include "dtp.h"
#include "session.h"


DataReader::DataReader(DataResponse& dr)
: dataResp{dr}, inputBuffer{dr.session.getDTP().getInputBuffer()}, bytesReceived{0} {
}


void DataReader::finish(const AsioData& asioData) {
	finishCallback(asioData, dataResp.getPtr());
}


// helper method
void DataReader::doReadCallback(const boost::system::error_code& ec, std::size_t nBytes) {
	readCallback(AsioData{ec, nBytes}, dataResp.getPtr());
}
