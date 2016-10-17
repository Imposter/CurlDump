/*
*   This file is part of the Indigo library.
*
*   This program is licensed under the GNU General
*   Public License. To view the full license, check
*   LICENSE in the project root.
*/

#ifndef indigo_string_hpp_
#define indigo_string_hpp_

#include <vector>
#include <stdarg.h>
#include <codecvt>
#include <locale>

#ifndef INDIGO_CORE_STRING_BUFFERSIZE
#define INDIGO_CORE_STRING_BUFFERSIZE 1024
#endif // INDIGO_CORE_STRING_BUFFERSIZE

namespace indigo {
class String {
public:
	static bool Equals(std::string str1, std::string str2, bool case_insensitive = false) {
		if (str1.size() != str2.size()) {
			return false;
		}
		size_t size = str1.size() > str2.size() ? str2.size() : str1.size();
		for (size_t i = 0; i < size; i++) {
			if ((case_insensitive && tolower(str1[i]) != tolower(str2[i])) || (!case_insensitive && str1[i] != str2[i])) {
				return false;
			} 
		}
		return true;
	}

	static bool Contains(std::string target, std::string contains) {
		return strstr(target.c_str(), contains.c_str()) != nullptr;
	}

	static bool IsNumber(const std::string &string) {
		return string.find_first_not_of("0123456789") == std::string::npos;
	}

	static std::string Format(std::string format, ...) {
		va_list arguments;
		va_start(arguments, format);

		int length = _vscprintf(format.c_str(), arguments) + 1;

		std::vector<char> output;
		output.resize(length);
		std::fill(output.begin(), output.end(), 0);

		vsprintf_s(output.data(), length, format.c_str(), arguments);

		va_end(arguments);

		return std::string(output.begin(), output.end());
	}

	static std::string PadLeft(std::string target, char character, size_t count) {
		std::string output;

		size_t length_to_add = count - target.size();
		output.append(length_to_add, character);
		output.append(target);

		return output;
	}
	
	static std::string PadRight(std::string target, char character, size_t count) {
		std::string output = target;

		size_t length_to_add = count - target.size();
		output.append(length_to_add, character);

		return output;
	}
	
	static std::string Replace(std::string source, std::string from, std::string to, bool ignore_case = false) {
		std::string output = source;

		for (size_t x = 0; x < output.size(); x++) {
			bool same = true;
			for (size_t y = 0; y < from.size(); y++) {
				char s = output[x + y];
				char f = from[y];

				if (ignore_case) {
					s = tolower(s);
					f = tolower(f);
				}

				if (s != f) {
					same = false;
					break;
				}
			}

			if (same) {
				output.replace(x, from.size(), to);
			}
		}

		return output;
	}
	
	static std::vector<std::string> Split(std::string source, std::string split) {
		std::string input = source;
		std::vector<std::string> output;

		size_t i;
		while ((i = input.find(split)) != std::string::npos) {
			output.push_back(input.substr(0, i));
			input.erase(0, i + split.size());
		}

		output.push_back(input);

		return output;
	}
	
	static std::wstring ToWideString(std::string string) {
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> utf16conv;
		return utf16conv.from_bytes(string);
	}
	
	static std::string ToString(std::wstring string) {
		std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> utf8conv;
		return utf8conv.to_bytes(string);
	}
};
}

#endif // indigo_string_hpp_