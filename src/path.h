#pragma once

#include <fstream>
#include <string>
#include <utility>
#define BOOST_FILESYSTEM_NO_DEPRECATED
#include <boost/filesystem.hpp>


// A simple wrapper for boost::filesystem::path
class Path {
public:
	Path() = default;
	Path(const boost::filesystem::path&);
	Path(const Path&) = default;
	~Path() = default;
	std::string string(void) const;
	std::string fileName(void) const;
	std::string pwd(const Path&) const;
	std::pair<Path, bool> get(const std::string&) const;
	std::pair<Path, bool> create(const std::string&, std::ofstream&) const;
	bool childOf(const Path&) const;
	bool isFile(void) const;
	bool isDirectory(void) const;
	const boost::filesystem::path& getBoostPath(void) const;
	Path& operator=(const Path&) = default;
	bool operator==(const Path&) const;
private:
	boost::filesystem::path path;
};


inline
std::string Path::string() const {
	return path.string();
}


inline
std::string Path::fileName() const {
	return path.filename().string();
}


inline
bool Path::isFile() const {
	return (
		boost::filesystem::status(path).type()
		== boost::filesystem::file_type::regular_file
	);
}


inline
bool Path::isDirectory() const {
	return (
		boost::filesystem::status(path).type()
		== boost::filesystem::file_type::directory_file
	);
}


inline
const boost::filesystem::path& Path::getBoostPath() const {
	return path;
}


inline
bool Path::operator==(const Path& that) const {
	return (path == that.path);
}
