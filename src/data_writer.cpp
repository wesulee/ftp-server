#include "data_writer.h"
#include "data_response.h"
#include "dtp.h"
#include "session.h"


DataWriter::DataWriter(Session& sess, DataResponse& dr)
: session{sess}, dataResp{dr}, outputBuffer{sess.getDTP().getOutputBuffer()},
bytesSent{0} {
}


void DataWriter::finish(const AsioData& asioData) {
	finishCallback(asioData, dataResp.getPtr());
}
