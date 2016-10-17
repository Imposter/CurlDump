/*
*   This file is part of the Indigo library.
*
*   This program is licensed under the GNU General
*   Public License. To view the full license, check
*   LICENSE in the project root.
*/

#ifndef indigo_sys_function_hpp_
#define indigo_sys_function_hpp_

#include <functional>

namespace indigo {
class SysFunction {
	std::function<void()> shutdown_function_;

public:
	SysFunction(std::function<void()> startup_function) {
		startup_function();
	}

	SysFunction(std::function<void()> startup_function, std::function<void()> shutdown_function)
		: shutdown_function_(shutdown_function) {
		startup_function();
	}

	~SysFunction() {
		if (shutdown_function_) {
			shutdown_function_();
		}
	}
};
}

#endif // indigo_sys_function_hpp_