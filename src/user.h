#pragma once

#include "md5.h"
#include "path.h"
#include <string>


struct User {
	MD5Digest pass;
	std::string name;
	std::string salt;
	Path home;
};
