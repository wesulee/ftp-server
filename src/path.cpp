#include "path.h"


namespace fs = boost::filesystem;


static std::string formatFTPStr(const std::string& str) {
	std::string ret;
	for (const auto c : str) {
		if (c == '"') {
			ret.append(2, '"');
		}
		else {
			ret.push_back(c);
		}
	}
	return ret;
}


Path::Path(const boost::filesystem::path& home) : path(fs::canonical(home)) {
}


// this is the current working directory, provided path p is the home
//   home directory.
// Returns string of ftp response, excluding delimiting '"'
std::string Path::pwd(const boost::filesystem::path& p) const {
	// TODO temporary implementation
	fs::path relPath = fs::relative(path, p);
	std::string str = relPath.string();
	if (relPath == fs::path{"."}) {
		str = "/";
	}
	return formatFTPStr(str);
}
