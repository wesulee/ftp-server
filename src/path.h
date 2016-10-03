#pragma once

#include <string>
#define BOOST_FILESYSTEM_NO_DEPRECATED
#include <boost/filesystem.hpp>


// A simple wrapper for boost::filesystem::path
class Path {
public:
	Path() = default;
	Path(const boost::filesystem::path&);
	Path(const Path&) = default;
	~Path() = default;
	std::string pwd(const boost::filesystem::path&) const;
	const boost::filesystem::path& getBoostPath(void) const;
private:
	boost::filesystem::path path;
};


inline
const boost::filesystem::path& Path::getBoostPath() const {
	return path;
}
