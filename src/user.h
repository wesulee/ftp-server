#pragma once

#include "md5.h"
#include <string>


struct User {
	std::string name;
	std::string salt;
	MD5Digest pass;
};
