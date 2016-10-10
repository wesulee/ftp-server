#pragma once

#include <memory>


class DataReader;
class DataWriter;
class Response;
class Session;


// Every command where the data connection will be used will create an
//   instance of this class.
struct DataResponse : public std::enable_shared_from_this<DataResponse> {
	DataResponse(Session&);
	~DataResponse() = default;
	std::shared_ptr<DataResponse> getPtr(void);

	std::shared_ptr<Response> cmdResp;
	std::shared_ptr<DataReader> dataReader;
	std::shared_ptr<DataWriter> dataWriter;
	Session& session;
};


inline
DataResponse::DataResponse(Session& sess) : session{sess} {
}


inline
std::shared_ptr<DataResponse> DataResponse::getPtr() {
	return shared_from_this();
}
