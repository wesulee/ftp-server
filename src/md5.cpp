// Assumes little-endian (for now)
#include "md5.h"
#include <cassert>
#include <cmath>


namespace MD5Constants {
	constexpr std::array<std::uint32_t, 64> T = {
		0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
		0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
		0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
		0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
		0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
		0xd62f105d, 0x2441453, 0xd8a1e681, 0xe7d3fbc8,
		0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
		0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
		0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
		0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
		0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x4881d05,
		0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
		0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
		0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
		0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
		0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
	};
	constexpr std::array<std::uint32_t, 16> R2_k = {
		1, 6, 11, 0, 5, 10, 15, 4,
		9, 14, 3, 8, 13, 2, 7, 12
	};
	constexpr std::array<std::uint32_t, 16> R3_k = {
		5, 8, 11, 14, 1, 4, 7, 10,
		13, 0, 3, 6, 9, 12, 15, 2
	};
	constexpr std::array<std::uint32_t, 16> R4_k = {
		0, 7, 14, 5, 12, 3, 10, 1,
		8, 15, 6, 13, 4, 11, 2, 9
	};
	constexpr std::array<std::uint32_t, 4> R1_s = {7, 12, 17, 22};
	constexpr std::array<std::uint32_t, 4> R2_s = {5, 9, 14, 20};
	constexpr std::array<std::uint32_t, 4> R3_s = {4, 11, 16, 23};
	constexpr std::array<std::uint32_t, 4> R4_s = {6, 10, 15, 21};
	constexpr char hexDigits[] = "0123456789abcdef";
}


std::string MD5Digest::str() const {
	std::string digestStr;
	digestStr.reserve(32);
	for (std::size_t i = 0; i < 16; ++i) {
		digestStr.push_back(MD5Constants::hexDigits[digest[i] / 16]);
		digestStr.push_back(MD5Constants::hexDigits[digest[i] % 16]);
	}
	return digestStr;
}


template<class Array>
bool testTArray(const Array& T) {
	typedef long double FloatType;
	const FloatType multiplier = static_cast<FloatType>(4294967296);
	FloatType intPartF;
	std::uint32_t intPart;
	for (std::size_t i = 0; i < 64; ++i) {
		const FloatType radians = static_cast<FloatType>(i + 1);
		const FloatType result = (multiplier * std::abs(std::sin(radians)));
		std::modf(result, &intPartF);
		intPart = static_cast<std::uint32_t>(intPartF);
		assert(intPart == T[i]);
		if (intPart != T[i])
			return false;
	}
	return true;
}


template<class T>
void copyToByteArray(T& array, const std::size_t i, std::uint32_t value) {
	const std::uint32_t mask = static_cast<std::uint32_t>(UINT8_MAX);
	array[i] = static_cast<std::uint8_t>(value & mask);
	value >>= 8;
	array[i+1] = static_cast<std::uint8_t>(value & mask);
	value >>= 8;
	array[i+2] = static_cast<std::uint8_t>(value & mask);
	value >>= 8;
	array[i+3] = static_cast<std::uint8_t>(value);
}


// sz is message size in bits before padding
static void padMessageSz(std::vector<std::uint8_t>& message, std::uint64_t sz) {
	const std::uint64_t mask = static_cast<std::uint64_t>(UINT8_MAX);
	for (int i = 0; i < 7; ++i) {
		message.push_back(static_cast<std::uint8_t>(sz & mask));
		sz >>= 8;
	}
	message.push_back(static_cast<std::uint8_t>(sz));
	assert(message.size() % 64 == 0);
}


static void padMessage(std::vector<std::uint8_t>& message) {
	std::uint64_t messageSz = (static_cast<std::uint64_t>(message.size()) * 8);
	std::size_t padAmount = ((message.size() % 64) % 56);
	if (padAmount != 0) {
		padAmount = (56 - padAmount);
	}
	else {
		padAmount = 56;
	}
	message.reserve(message.size() + padAmount + 8);
	message.push_back(1 << 7);
	for (std::size_t i = 1; i < padAmount; ++i)
		message.push_back(0);
	assert(message.size() % 56 == 0);
	padMessageSz(message, messageSz);
}


static std::uint32_t getUint32(const std::vector<std::uint8_t>& message, const std::size_t i) {
	return (
		static_cast<std::uint32_t>(message[i])
		| (static_cast<std::uint32_t>(message[i+1]) << 8)
		| (static_cast<std::uint32_t>(message[i+2]) << 16)
		| (static_cast<std::uint32_t>(message[i+3]) << 24)
	);
}


static constexpr std::uint32_t rotateLeft(const std::uint32_t val, const std::uint32_t n) {
	return ((val << n) | (val >> (32 - n)));
}


static constexpr std::uint32_t F(const std::uint32_t X, const std::uint32_t Y, const std::uint32_t Z) {
	return ((X & Y) | ((~X) & Z));
}


static constexpr std::uint32_t G(const std::uint32_t X, const std::uint32_t Y, const std::uint32_t Z) {
	return ((X & Z) | (Y & (~Z)));
}


static constexpr std::uint32_t H(const std::uint32_t X, const std::uint32_t Y, const std::uint32_t Z) {
	return (X ^ Y ^ Z);
}


static constexpr std::uint32_t I(const std::uint32_t X, const std::uint32_t Y, const std::uint32_t Z) {
	return (Y ^ (X | (~Z)));
}


template<class Func>
static void R(Func f, std::uint32_t& A, const std::uint32_t B, const std::uint32_t C, const std::uint32_t D,
const std::uint32_t X, const std::uint32_t S, const std::uint32_t T) {
	A = (B + rotateLeft(A + f(B, C, D) + X + T, S));
}


MD5Digest MD5::getDigest(const std::vector<std::uint8_t>& message) {
	assert(testTArray(MD5Constants::T));
	std::array<std::uint32_t, 16> X;
	std::uint32_t A = 0x67452301;
	std::uint32_t B = 0xefcdab89;
	std::uint32_t C = 0x98badcfe;
	std::uint32_t D = 0x10325476;
	std::vector<std::uint8_t> input = message;
	padMessage(input);
	const std::size_t numBlocks = (input.size() / 64);
	for (std::size_t i = 0; i < numBlocks; ++i) {
		const std::uint32_t AA = A;
		const std::uint32_t BB = B;
		const std::uint32_t CC = C;
		const std::uint32_t DD = D;
		for (std::size_t j = 0; j < 16; ++j) {
			X[j] = getUint32(input, i*64 + j*4);
		}
		// Round 1
		std::size_t j;
		for (j = 0; j < 16; ++j) {
			const std::size_t k = (j % 4);
			const std::uint32_t X_val = X[j];
			const std::uint32_t S_val = MD5Constants::R1_s[k];
			const std::uint32_t T_val = MD5Constants::T[j];
			switch (k) {
			case 0:
				R(F, A, B, C, D, X_val, S_val, T_val);
				break;
			case 1:
				R(F, D, A, B, C, X_val, S_val, T_val);
				break;
			case 2:
				R(F, C, D, A, B, X_val, S_val, T_val);
				break;
			case 3:
				R(F, B, C, D, A, X_val, S_val, T_val);
				break;
			default:
				assert(false);
				break;
			}
		}
		// Round 2
		for (; j < 32; ++j) {
			const std::size_t k = (j % 4);
			const std::uint32_t X_val = X[MD5Constants::R2_k[j-16]];
			const std::uint32_t S_val = MD5Constants::R2_s[k];
			const std::uint32_t T_val = MD5Constants::T[j];
			switch (k) {
			case 0:
				R(G, A, B, C, D, X_val, S_val, T_val);
				break;
			case 1:
				R(G, D, A, B, C, X_val, S_val, T_val);
				break;
			case 2:
				R(G, C, D, A, B, X_val, S_val, T_val);
				break;
			case 3:
				R(G, B, C, D, A, X_val, S_val, T_val);
				break;
			default:
				assert(false);
				break;
			}
		}
		// Round 3
		for (; j < 48; ++j) {
			const std::size_t k = (j % 4);
			const std::uint32_t X_val = X[MD5Constants::R3_k[j-32]];
			const std::uint32_t S_val = MD5Constants::R3_s[k];
			const std::uint32_t T_val = MD5Constants::T[j];
			switch (k) {
			case 0:
				R(H, A, B, C, D, X_val, S_val, T_val);
				break;
			case 1:
				R(H, D, A, B, C, X_val, S_val, T_val);
				break;
			case 2:
				R(H, C, D, A, B, X_val, S_val, T_val);
				break;
			case 3:
				R(H, B, C, D, A, X_val, S_val, T_val);
				break;
			default:
				assert(false);
				break;
			}
		}
		// Round 4
		for (; j < 64; ++j) {
			const std::size_t k = (j % 4);
			const std::uint32_t X_val = X[MD5Constants::R4_k[j-48]];
			const std::uint32_t S_val = MD5Constants::R4_s[k];
			const std::uint32_t T_val = MD5Constants::T[j];
			switch (k) {
			case 0:
				R(I, A, B, C, D, X_val, S_val, T_val);
				break;
			case 1:
				R(I, D, A, B, C, X_val, S_val, T_val);
				break;
			case 2:
				R(I, C, D, A, B, X_val, S_val, T_val);
				break;
			case 3:
				R(I, B, C, D, A, X_val, S_val, T_val);
				break;
			default:
				assert(false);
				break;
			}
		}
		// Increment registers
		A += AA;
		B += BB;
		C += CC;
		D += DD;
	}
	// Copy digest
	MD5Digest::ArrayType digest;
	copyToByteArray(digest, 0, A);
	copyToByteArray(digest, 4, B);
	copyToByteArray(digest, 8, C);
	copyToByteArray(digest, 12, D);
	return MD5Digest(digest);
}


std::vector<std::uint8_t> MD5::strToByteArray(const std::string& str) {
	std::vector<std::uint8_t> array;
	array.reserve(str.size());
	for (const char c : str)
		array.push_back(static_cast<std::uint8_t>(c));
	return array;
}
