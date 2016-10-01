#pragma once

#include <boost/asio.hpp>


struct AsioData {
	AsioData(const boost::system::error_code&, const std::size_t);
	~AsioData() = default;

	const boost::system::error_code& ec;
	std::size_t nBytes;
};


inline
AsioData::AsioData(const boost::system::error_code& a1, const std::size_t a2)
: ec{a1}, nBytes{a2} {
}
