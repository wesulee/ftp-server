#pragma once

#include "md5.h"
#include <string>


struct User {
	std::string name;
	MD5Digest pass;
};
