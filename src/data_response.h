#pragma once

#include <memory>


class DataWriter;
class Response;


struct DataResponse : public std::enable_shared_from_this<DataResponse> {
	std::shared_ptr<DataResponse> getPtr(void);

	std::shared_ptr<Response> cmdResp;
	std::shared_ptr<DataWriter> dataWriter;
};


inline
std::shared_ptr<DataResponse> DataResponse::getPtr() {
	return shared_from_this();
}
