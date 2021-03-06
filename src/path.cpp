#include "path.h"
#include <cassert>
#include <exception>


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


// throws boost::filesystem::filesystem_error if path does not exist
Path::Path(const fs::path& home) : path(fs::canonical(home)) {
}


// this is interpreted as the current working directory of a user, and
//   provided Path p is the user's home directory.
// Returns string of ftp response, excluding delimiting '"'
std::string Path::pwd(const Path& p) const {
	// TODO temporary implementation
	fs::path relPath = fs::relative(path, p.path);
	std::string str = relPath.string();
	if (relPath == fs::path{"."}) {
		str = "/";
	}
	return formatFTPStr(str);
}


// returns a new Path relative to this, where p is a relative path
// ret.first is the new Path if ret.second is true
// returns a new Path where this is interpreted as the root path and p is
//   either a relative or absolute path.
std::pair<Path, bool> Path::get(const std::string& p) const {
	assert(!p.empty());
	std::pair<Path, bool> ret = std::make_pair(Path{}, false);
	fs::path pPath{p};
	if (!pPath.is_relative()) {
		if (p.front() != '/') {
			// any non-relative path must begin with /
			return ret;
		}
		else {
			// attempt to make p relative
			pPath = fs::path{p.substr(1)};
		}
	}
	fs::path newPath = (path / pPath);
	try {
		ret.first = Path{newPath};
		ret.second = true;
	}
	catch (const std::exception&) {
		// path most likely does not exist
		// do nothing
	}
	return ret;
}


// Attempts to create a file with this as the base path and name being the desired
//   filename. this is assumed to be a directory.
// The provided ofstream should be only default constructed and not modified.
// create() will attempt to open the file in binary mode.
// If file successfully opened, the returned ret.second will be true, and
//   ret.first will be the Path to the created file.
std::pair<Path, bool> Path::create(const std::string& name, std::ofstream& file) const {
	assert(isDirectory());
	std::pair<Path, bool> ret = std::make_pair(Path{}, false);
	// make sure that name is a valid filename (that is, not an absolute path,
	//   does not begin with invalid sequence, has no parent path).
	const fs::path namePath{name};
	if (name.empty() || !namePath.is_relative() || !namePath.parent_path().string().empty())
		return ret;
	if (
		(name == ".")
		|| (name == "..")
		|| (name.substr(0, 2) == "./")
		|| (name.substr(0, 3) == "../")
	) {
		return ret;
	}
	// attempt to open file
	fs::path reqPath = (path / namePath);
	file.open(reqPath.string(), std::ofstream::binary | std::ofstream::trunc);
	if (file.is_open()) {
		try {
			ret.first = Path{reqPath};
			ret.second = true;
			assert(ret.first.childOf(*this));
		}
		catch (const std::exception&) {
			// unable to get Path...
		}
	}
	return ret;
}


// Is this a child of that? Or, in other words, is that a parent of this?
// that is assumed to be a directory.
// Note: returns false if this and that are equal.
bool Path::childOf(const Path& that) const {
	assert(that.isDirectory());
	if (path.size() < that.path.size())
		return false;
	fs::path::const_iterator itThis = path.begin();
	const fs::path::const_iterator itThisEnd = path.end();
	fs::path::const_iterator itThat = that.path.begin();
	const fs::path::const_iterator itThatEnd = that.path.end();
	// Compare each component of path lexically since this and that's member path are
	//   in canonical form.
	// Once either has reached their end, if that (the supposed root directory) reached
	//   its end first (or at the same time as this), we know this is at least as long
	//   as that and all its components up to that are equal.
	while (
		(itThis != itThisEnd)
		&& (itThat != itThatEnd)
		&& (*itThis == *itThat)
	) {
		++itThis;
		++itThat;
	}
	return ((itThat == itThatEnd) && (itThis != itThisEnd));
}
