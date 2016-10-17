/*
*   This file is part of the Indigo library.
*
*   This program is licensed under the GNU General
*   Public License. To view the full license, check
*   LICENSE in the project root.
*/

#ifndef indigo_hash_hpp_
#define indigo_hash_hpp_

#include "../core/string.hpp"
#include <iostream>

namespace indigo {
class Hash {
public:
	static const uint32_t FNV1A_Prime32 = 0x01000193;
	static const uint32_t FNV1A_Offset32 = 0x811C9DC5;
	static const uint64_t FNV1A_Prime64 = 0x00000100000001B3;
	static const uint64_t FNV1A_Offset64 = 0xCBF29CE484222325;

	static uint32_t FNV1A_32(uint8_t *obj, size_t size, uint32_t prime = FNV1A_Prime32, uint32_t offset = FNV1A_Offset32) {
		uint32_t hash = offset;
		for (size_t i = 0; i < size; i++) {
			hash *= prime;
			hash ^= obj[i];
		}
		return hash;
	}

	static uint64_t FNV1A_64(uint8_t *obj, size_t size, uint64_t prime = FNV1A_Prime64, uint64_t offset = FNV1A_Offset64) {
		uint64_t hash = offset;
		for (size_t i = 0; i < size; i++) {
			hash *= prime;
			hash ^= obj[i];
		}
		return hash;
	}

	static uint32_t FNV1A_32(std::string &obj, uint32_t prime = FNV1A_Prime32, uint32_t offset = FNV1A_Offset32) {
		return FNV1A_32(reinterpret_cast<uint8_t *>(const_cast<char *>(obj.c_str())), obj.size(), prime, offset);
	}

	static uint64_t FNV1A_64(std::string &obj, uint64_t prime = FNV1A_Prime64, uint64_t offset = FNV1A_Offset64) {
		return FNV1A_64(reinterpret_cast<uint8_t *>(const_cast<char *>(obj.c_str())), obj.size(), prime, offset);
	}

	static uint32_t FNV1A_32(std::istream &stream, uint32_t prime = FNV1A_Prime32, uint32_t offset = FNV1A_Offset32) {
		size_t buffer_size = 1024 * 1024;
		char *buffer = new char[buffer_size];
		uint32_t hash = offset;
		while (!stream.eof()) {
			stream.read(buffer, buffer_size);
			hash = FNV1A_32(reinterpret_cast<uint8_t *>(buffer), stream.gcount(), prime, hash);
		}
		delete[] buffer;

		return hash;
	}

	static uint64_t FNV1A_64(std::istream &stream, uint32_t prime = FNV1A_Prime64, uint32_t offset = FNV1A_Offset64) {
		size_t buffer_size = 1024 * 1024;
		char *buffer = new char[buffer_size];
		uint64_t hash = offset;
		while (!stream.eof()) {
			stream.read(buffer, buffer_size);
			hash = FNV1A_64(reinterpret_cast<uint8_t *>(buffer), stream.gcount(), prime, hash);
		}
		delete[] buffer;

		return hash;
	}

	template<typename _TData>
	static std::string GetString(_TData hash) {
		std::string result;
		result.resize(sizeof(hash) * 2);
		fill(result.begin(), result.end(), 0);
		uint8_t *src = reinterpret_cast<uint8_t *>(&hash);
		for (size_t i = 0; i < sizeof(hash); i++) {
			sprintf_s(const_cast<char *>(result.c_str() + i * 2), result.size(), "%02X", src[i]);
		}
		return result;
	}
};
}

#endif // indigo_hash_hpp_