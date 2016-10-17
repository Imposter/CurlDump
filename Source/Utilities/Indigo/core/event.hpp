/*
*   This file is part of the Indigo library.
*
*   This program is licensed under the GNU General
*   Public License. To view the full license, check
*   LICENSE in the project root.
*/

#ifndef indigo_event_hpp_
#define indigo_event_hpp_

// Required libraries
#include <mutex>
#include <vector>
#include <functional>

namespace indigo {
template<typename... _TArgs>
class Event {
	std::mutex event_mutex_;
	std::vector<std::function<bool(_TArgs...)>> callbacks_;

public:
	Event() {}
	Event(const Event &event) {}

	template<typename _TFunction>
	Event &Add(const _TFunction &function) {
		event_mutex_.lock();
		callbacks_.push_back(function);
		event_mutex_.unlock();

		return *this;
	}

	template<typename _TFunction>
	Event &Remove(const std::function<_TFunction> &function) {
		event_mutex_.lock();
		for (auto it = callbacks_.begin(); it != callbacks_.end();) {
			if (it->template target<_TFunction>() == function.template target<_TFunction>()) {
				callbacks_.erase(it);
				break;
			} else {
				++it;
			}
		}
		event_mutex_.unlock();

		return *this;
	}

	bool operator()(_TArgs... arguments) {
		bool success = true;
		std::vector<std::function<bool(_TArgs...)>> callbacks(callbacks_.size());
		
		event_mutex_.lock();
		std::copy(callbacks_.begin(), callbacks_.end(), callbacks.begin());
		event_mutex_.unlock();

		for (auto iterator = callbacks.begin(); iterator != callbacks.end(); ++iterator) {
			if (!(*iterator)(arguments...)) {
				success = false;
			}
		}

		return success;
	}
};
}

#endif // indigo_event_hpp_