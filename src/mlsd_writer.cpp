#include "mlsd_writer.h"
#include "path.h"
#include "session.h"
#include "utility.h"
#include <algorithm>	// min
#include <cassert>
#include <cstdint>		// uintmax_t
#include <sstream>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>


namespace fs = boost::filesystem;
namespace ps = boost::posix_time;


/* MLSD format (https://tools.ietf.org/html/rfc3659)
data-response    = *( entry CRLF )
entry            = [ facts ] SP pathname
facts            = 1*( fact ";" )
fact             = factname "=" value
factname         = "Size" / "Modify" / "Create" /
                   "Type" / "Unique" / "Perm" /
                   "Lang" / "Media-Type" / "CharSet" /
                   os-depend-fact / local-fact
os-depend-fact   = <IANA assigned OS name> "." token
local-fact       = "X." token
token            = 1*RCHAR
value            = *SCHAR
SCHAR          = RCHAR / "=" ;
RCHAR          = ALPHA / DIGIT / "," / "." / ":" / "!" /
                 "@" / "#" / "$" / "%" / "^" /
                 "&" / "(" / ")" / "-" / "_" /
                 "+" / "?" / "/" / "\" / "'" /
                 DQUOTE   ; <"> -- double quote character (%x22)
ALPHA          =  %x41-5A / %x61-7A   ; A-Z / a-z
DIGIT          =  %x30-39   ; 0-9

Fact names are case-insensitive.

Five values are possible for the type fact:

      file         -- a file entry
      cdir         -- the listed directory
      pdir         -- a parent directory
      dir          -- a directory or sub-directory
      OS.name=type -- an OS or file system dependent file type

The perm fact is used to indicate access rights the current FTP user
   has over the object listed.  Its value is always an unordered
   sequence of alphabetic characters.

      perm-fact    = "Perm" "=" *pvals
      pvals        = "a" / "c" / "d" / "e" / "f" /
                     "l" / "m" / "p" / "r" / "w"

The syntax of a time value is:
      time-val       = 14DIGIT [ "." 1*DIGIT ]

The leading, mandatory, fourteen digits are to be interpreted as, in
   order from the leftmost, four digits giving the year, with a range of
   1000--9999, two digits giving the month of the year, with a range of
   01--12, two digits giving the day of the month, with a range of
   01--31, two digits giving the hour of the day, with a range of
   00--23, two digits giving minutes past the hour, with a range of
   00--59, and finally, two digits giving seconds past the minute, with
   a range of 00--60 (with 60 being used only at a leap second).
*/


namespace MLSDConstants {
	constexpr char factType[] = "type";
	constexpr char factTypeFile[] = "file";
	constexpr char factTypeDir[] = "dir";
	constexpr char factSize[] = "size";
	constexpr char factModify[] = "modify";
}


namespace MLSDUtil {

template<class T>
static void appendFact(std::string& dst, const char* name, const T& value) {
	dst.append(name);
	dst.push_back('=');
	dst.append(value);
	dst.push_back(';');
}


static std::string genTime(const std::time_t timeT) {
	ps::ptime posixTime = ps::from_time_t(timeT);
	// Note: ownership of facet is given to locale
	// TODO save locale
	ps::time_facet* facet = new ps::time_facet{"%Y%m%d%H%M%S"};
	std::stringstream ss;
	ss.imbue(std::locale(ss.getloc(), facet));
	ss << posixTime;
	std::string ret = ss.str();
	assert(ret.size() == 14);
	return ret;
}


// generate a single entry (which excludes CRLF)
// invalid file returns empty string
// TODO add permissions (provided by status)
static std::string genEntry(const fs::directory_entry& entry) {
	std::string str;
	fs::file_status status = entry.status();
	// make sure entry is a regular file or directory
	switch (status.type()) {
	case fs::file_type::regular_file:
	case fs::file_type::directory_file:
		break;
	default:
		return str;
	}
	if (status.type() == fs::file_type::regular_file) {
		const std::uintmax_t fileSz = fs::file_size(entry.path());
		const std::time_t modifyTimeInt = fs::last_write_time(entry.path());
		appendFact(str, MLSDConstants::factType, MLSDConstants::factTypeFile);
		appendFact(str, MLSDConstants::factSize, std::to_string(fileSz));
		appendFact(str, MLSDConstants::factModify, genTime(modifyTimeInt));
	}
	else {
		// directory
		assert(status.type() == fs::file_type::directory_file);
		appendFact(str, MLSDConstants::factType, MLSDConstants::factTypeDir);
	}
	str.push_back(' ');
	str.append(entry.path().filename().string());
	return str;
}

}	// namespace MLSDUtil


MLSDWriter::MLSDWriter(Session& sess, DataResponse& dr, const Path& dirPath)
: DataWriter{sess, dr}, entriesIndex{0}, doneFlag{false} {
	fs::directory_iterator it{dirPath.getBoostPath()};
	fs::directory_iterator end;
	// deferrencing it returns type const fs::directory_entry&
	for (; it != end; ++it) {
		entries.emplace_back(MLSDUtil::genEntry(*it));
		if (entries.back().empty()) {
			entries.pop_back();
		}
		else {
			// to simplify things, append EOL to each entry
			entries.back().append(Constants::EOL);
		}
	}
}


// TODO what to do when empty?
void MLSDWriter::send() {
	assert(!entries.empty());
	setupEntry();
	writeSome();
}


bool MLSDWriter::good() const {
	return !entries.empty();
}


void MLSDWriter::writeSome() {
	session.getDTPSocket().async_write_some(
		boost::asio::buffer(
			outputBuffer.data() + bufIndex,
			outputBuffer.size() - bufIndex
		),
		[this](const boost::system::error_code& ec, std::size_t nBytes) {
			asioCallback(ec, nBytes);
		}
	);
}


bool MLSDWriter::done() const {
	return doneFlag;
}


void MLSDWriter::finish(const AsioData& asioData) {
	// After MLSD is successfully sent over data connection, we close data connection
	//   and send 226 response over command connection.
	// This is done in PI, so we do nothing in this method.
	DataWriter::finish(asioData);
}


// copy new entry to outputBuffer
void MLSDWriter::setupEntry() {
	// setup entry entriesIndex
	assert(entriesIndex < entries.size());
	entryBytes = 0;
	bufIndex = 0;
	outputBuffer.clear();
	const std::size_t appendSz = std::min(
		entries[entriesIndex].size(),
		outputBuffer.capacity()
	);
	outputBuffer.append(entries[entriesIndex].c_str(), appendSz);
}


void MLSDWriter::updateEntry() {
	assert(bufIndex == outputBuffer.capacity());
	outputBuffer.clear();
	bufIndex = 0;
	const std::size_t appendSz = std::min(
		entries[entriesIndex].size() - entryBytes,
		outputBuffer.capacity()
	);
	outputBuffer.append(entries[entriesIndex].c_str() + entryBytes, appendSz);
}


void MLSDWriter::asioCallback(const boost::system::error_code& ec, std::size_t nBytes) {
	bytesSent += nBytes;
	entryBytes += nBytes;
	bufIndex += nBytes;
	if (entryBytes == entries[entriesIndex].size()) {
		// finished writing current entry
		++entriesIndex;
		if (entriesIndex == entries.size()) {
			doneFlag = true;
		}
		else {
			setupEntry();
		}
	}
	else if (bufIndex == outputBuffer.capacity()) {
		// copy remaining contents of entry to outputBuffer
		updateEntry();
	}
	else {
		// still not finished writing contents of current entry in outputBuffer
		// do nothing
	}
	doWriteCallback(ec, nBytes);
}
