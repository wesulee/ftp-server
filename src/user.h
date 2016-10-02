#pragma once

#include "md5.h"
#include <string>
#define BOOST_FILESYSTEM_NO_DEPRECATED
#include <boost/filesystem.hpp>


struct User {
	MD5Digest pass;
	std::string name;
	std::string salt;
	boost::filesystem::path home;	// canonical
};
