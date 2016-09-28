#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>


class MD5Digest {
public:
	typedef std::array<std::uint8_t, 16> ArrayType;

	MD5Digest();
	MD5Digest(const MD5Digest&) = default;
	MD5Digest(const std::string&);
	MD5Digest(const ArrayType&);
	~MD5Digest() = default;
	std::string str(void) const;
	MD5Digest& operator=(const MD5Digest&) = default;
	bool operator==(const MD5Digest&) const;
private:
	ArrayType digest;
};


// https://tools.ietf.org/html/rfc1321
class MD5 {
public:
	static MD5Digest getDigest(const std::string&);
	static MD5Digest getDigest(const std::vector<std::uint8_t>&);
	static std::vector<std::uint8_t> strToByteArray(const std::string&);
};


inline
MD5Digest::MD5Digest() {
	digest.fill(0);
}


inline
MD5Digest::MD5Digest(const ArrayType& d) : digest(d) {
}


inline
bool MD5Digest::operator==(const MD5Digest& o) const {
	return (digest == o.digest);
}


inline
MD5Digest MD5::getDigest(const std::string& str) {
	return getDigest(strToByteArray(str));
}
