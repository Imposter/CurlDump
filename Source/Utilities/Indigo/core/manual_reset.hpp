/*
*   This file is part of the Indigo library.
*
*   This program is licensed under the GNU General
*   Public License. To view the full license, check
*   LICENSE in the project root.
*/

#ifndef indigo_manual_reset_hpp_
#define indigo_manual_reset_hpp_

#include <mutex>
#include <condition_variable>

namespace indigo {
class ManualReset {
	std::mutex mutex_;
	std::condition_variable condition_variable_;
	bool signaled_;

public:
	ManualReset(bool initial_state = false) 
		: signaled_(initial_state) {
	}

	void Set() {
		std::unique_lock<std::mutex> lock(mutex_);
		signaled_ = true;
		condition_variable_.notify_all();
	}

	void Reset() {
		std::unique_lock<std::mutex> lock(mutex_);
		signaled_ = false;
	}

	bool Wait(int timeout = 0) {
		std::unique_lock<std::mutex> lock(mutex_);
		while (!signaled_) {
			if (timeout == 0) {
				condition_variable_.wait(lock);
			} else {
				return condition_variable_.wait_for(lock, std::chrono::milliseconds(timeout)) != std::cv_status::timeout;
			}
		}
		return true;
	}
};
}

#endif // indigo_manual_reset_hpp_