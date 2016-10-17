/*
*   This file is part of the Indigo library.
*
*   This program is licensed under the GNU General
*   Public License. To view the full license, check
*   LICENSE in the project root.
*/

#ifndef indigo_logger_hpp_
#define indigo_logger_hpp_

#include <ostream>
#include <iomanip>
#include <vector>
#include <mutex>
#include <stdarg.h>

namespace indigo {
enum LogType {
	kLogType_Error,
	kLogType_Warning,
	kLogType_Info,
	kLogType_Trace
};

class Logger {
	LogType level_;
	std::vector<std::ostream *> streams_;
	std::mutex streams_mutex_;

public:
	Logger() : level_(kLogType_Error) {}
	Logger(LogType level) : level_(level) {}

	void AddStream(std::ostream &stream) {
		streams_mutex_.lock();
		streams_.push_back(&stream);
		streams_mutex_.unlock();

		time_t current_time;
		time(&current_time);
		tm local_time;
		localtime_s(&local_time, &current_time);

		stream << "==================================================" << std::endl;
		stream << "Log created on " 
			<< std::setw(2) << std::setfill('0') << local_time.tm_mday << "/"
			<< std::setw(2) << std::setfill('0') << local_time.tm_mon + 1 << "/"
			<< std::setw(2) << std::setfill('0') << local_time.tm_year + 1900 << " "
			<< std::setw(2) << std::setfill('0') << local_time.tm_hour << ":"
			<< std::setw(2) << std::setfill('0') << local_time.tm_min << ":"
			<< std::setw(2) << std::setfill('0') << local_time.tm_sec
			<< std::endl;
		stream << "==================================================" << std::endl;
		stream << std::endl;
	}

	void RemoveStream(std::ostream &stream) {
		streams_mutex_.lock();
		for (auto it = streams_.begin(); it != streams_.end();) {
			if (*it == &stream) {
				it = streams_.erase(it);
			} else {
				++it;
			}
		}
		streams_mutex_.unlock();
	}

	void SetLevel(LogType level) {
		level_ = level;
	}

	void Write(LogType type, std::string class_name, std::string format, ...) {
		va_list arguments;
		va_start(arguments, format);

		int length = _vscprintf(format.c_str(), arguments) + 1;

		std::vector<char> message;
		message.resize(length);
		std::fill(message.begin(), message.end(), 0);

		vsprintf_s(message.data(), length, format.c_str(), arguments);

		va_end(arguments);

		const char *type_string = "";
		switch (type) {
		case indigo::kLogType_Error:
			type_string = "ERROR";
			break;
		case indigo::kLogType_Warning:
			type_string = "WARNING";
			break;
		case indigo::kLogType_Trace:
			type_string = "TRACE";
			break;
		case kLogType_Info: 
			type_string = "INFO";
			break;
		default:
			type_string = "UNKN";
		}

		time_t current_time;
		time(&current_time);
		tm local_time;
		localtime_s(&local_time, &current_time);

		streams_mutex_.lock();
		for (auto &stream : streams_) {
			*stream << "[" << std::setw(2) << std::setfill('0') << local_time.tm_hour
				<< ":" << std::setw(2) << std::setfill('0') << local_time.tm_min
				<< ":" << std::setw(2) << std::setfill('0') << local_time.tm_sec
				<< "][" << type_string << ":" << class_name.c_str() 
				<< "]: " << message.data() << std::endl;
		}
		streams_mutex_.unlock();
	}
};
}

#endif // indigo_logger_hpp_