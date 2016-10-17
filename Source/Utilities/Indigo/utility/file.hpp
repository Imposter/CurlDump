/*
*   This file is part of the Indigo library.
*
*   This program is licensed under the GNU General
*   Public License. To view the full license, check
*   LICENSE in the project root.
*/

#ifndef indigo_file_hpp_
#define indigo_file_hpp_

#include "../platform.h"
#include "../core/string.hpp"

#include <experimental/filesystem>

namespace indigo {
class File {
public:
	// Taken from http://stackoverflow.com/questions/1746136/how-do-i-normalize-a-pathname-using-boostfilesystem
	static std::experimental::filesystem::path Normalize(const std::experimental::filesystem::path &path) {
		std::experimental::filesystem::path absPath = absolute(path);
		std::experimental::filesystem::path::iterator it = absPath.begin();
		std::experimental::filesystem::path result = *it++;

		// Get canonical version of the existing part
		for (; exists(result / *it) && it != absPath.end(); ++it) {
			result /= *it;
		}
		result = canonical(result);

		// For the rest remove ".." and "." in a path with no symlinks
		for (; it != absPath.end(); ++it) {
			// Just move back on ../
			if (*it == "..") {
				result = result.parent_path();
			} else if (*it != ".") {
				// Just cat other path entries
				result /= *it;
			}
		}

		// Fix path
#if defined(OS_WIN)
		std::string final_path = String::Replace(result.string(), "/", "\\");
		final_path = String::Replace(final_path, "\\\\", "\\");
		result = std::experimental::filesystem::path(final_path);
#endif

		return result;
	}

	static std::experimental::filesystem::path GetRelativePath(const std::experimental::filesystem::path &path, 
		const std::experimental::filesystem::path &relative_to) {
		std::string source_path = path.string();
		std::string relative_path = relative_to.string();

		// Fix path
#if defined(OS_WIN)
		relative_path += "\\";

		source_path = String::Replace(source_path, "/", "\\");
		relative_path = String::Replace(relative_path, "/", "\\");

		source_path = String::Replace(source_path, "\\\\", "\\");
		relative_path = String::Replace(relative_path, "\\\\", "\\");
#endif

		return String::Replace(source_path, relative_path, "", true);
	}
};
}

#endif // indigo_file_hpp_