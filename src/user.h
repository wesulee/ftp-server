#pragma once

#include "md5.h"
#include <string>


struct User {
	MD5Digest pass;
	std::string name;
	std::string salt;
	std::string home;
};
