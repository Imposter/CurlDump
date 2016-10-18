/*
*   This file is part of the Indigo library.
*
*   This program is licensed under the GNU General
*   Public License. To view the full license, check
*   LICENSE in the project root.
*/

#ifndef indigo_config_hpp_
#define indigo_config_hpp_

#include "../core/string.hpp"
#include <stdint.h>
#include <map>
#include <mutex>

namespace indigo {
class Config {
	std::string buffer_;
	std::mutex mutex_;
	std::map<std::string, std::map<std::string, std::string>> data_;

public:
	bool Open(std::string buffer) {
		buffer_ = buffer;
		buffer_ = String::Replace(buffer_, "\r\n", "\n");

		std::vector<std::string> lines;
		if (String::Contains(buffer_, "\n")) {
			lines = String::Split(buffer_, "\n");
		} else {
			lines = String::Split(buffer_, "\n");
		}
		std::string section_name;
		std::map<std::string, std::string> section;
		for (auto &line : lines) {
			if (line.size() > 0) {
				if (line.find_first_of('[') == 0 && line.find_last_of(']') == line.size() - 1) {
					if (section_name.size() != 0) {
						data_.insert(std::make_pair(section_name, section));
						section.clear();
					}
					section_name = line.substr(1, line.size() - 2);
				} else if (line.find_first_of('=') != std::string::npos) {
					size_t key_end = line.find_first_of('=');
					std::string key = line.substr(0, key_end);
					std::string value = line.substr(key_end + 1);
					if (!key.empty()) {
						section.insert(std::make_pair(key, value));
					}
				}
			}
		}
		if (!section.empty()) {
			data_.insert(std::make_pair(section_name, section));
			section.clear();
		}

		return true;
	}

	void Close() {
		buffer_.clear();
		data_.clear();
	}

	bool KeyExists(std::string section, std::string key) {
		return !(data_.find(section) == data_.end() || data_[section].find(key) == data_[section].end());
	}

	std::string GetString(std::string section, std::string key, std::string default_value = std::string()) {
		if (data_.find(section) == data_.end() || data_[section].find(key) == data_[section].end()) {
			SetString(section, key, default_value);
			return default_value;
		}
		return data_[section][key];
	}

	int64_t GetInteger(std::string section, std::string key, int64_t default_value = 0) {
		if (data_.find(section) == data_.end() || data_[section].find(key) == data_[section].end()) {
			SetInteger(section, key, default_value);
			return default_value;
		}
		return strtoll(data_[section][key].c_str(), nullptr, 0);
	}

	float GetFloat(std::string section, std::string key, float default_value = 0) {
		if (data_.find(section) == data_.end() || data_[section].find(key) == data_[section].end()) {
			SetFloat(section, key, default_value);
			return default_value;
		}
		return stof(data_[section][key]);
	}

	std::vector<std::string> GetStringList(std::string section, std::string key, std::vector<std::string> default_value = std::vector<std::string>()) {
		if (data_.find(section) == data_.end() || data_[section].find(key) == data_[section].end()) {
			SetStringList(section, key, default_value);
			return default_value;
		}
		int size = std::atoi(data_[section][key].c_str());
		std::vector<std::string> result;
		for (int i = 0; i < size; i++) {
			result.push_back(data_[section][key + "." + std::to_string(i)]);
		}
		return result;
	}

	std::vector<int64_t> GetIntegerList(std::string section, std::string key, std::vector<int64_t> default_value = std::vector<int64_t>()) {
		if (data_.find(section) == data_.end() || data_[section].find(key) == data_[section].end()) {
			SetIntegerList(section, key, default_value);
			return default_value;
		}
		int size = std::atoi(data_[section][key].c_str());
		std::vector<int64_t> result;
		for (int i = 0; i < size; i++) {
			result.push_back(std::stoull(data_[section][key + "." + std::to_string(i)].c_str()));
		}
		return result;
	}

	std::vector<float> GetFloatList(std::string section, std::string key, std::vector<float> default_value = std::vector<float>()) {
		if (data_.find(section) == data_.end() || data_[section].find(key) == data_[section].end()) {
			SetFloatList(section, key, default_value);
			return default_value;
		}
		int size = std::atoi(data_[section][key].c_str());
		std::vector<float> result;
		for (int i = 0; i < size; i++) {
			result.push_back(std::stof(data_[section][key + "." + std::to_string(i)].c_str()));
		}
		return result;
	}

	std::map<std::string, std::string> GetStringMap(std::string section, std::string key, std::map<std::string, std::string> default_value 
		= std::map<std::string, std::string>()) {
		if (data_.find(section) == data_.end() || data_[section].find(key) == data_[section].end()) {
			SetStringMap(section, key, default_value);
			return default_value;
		}
		int32_t size = std::atoi(data_[section][key].c_str());
		std::map<std::string, std::string> result;
		do {
			for (auto &key_value_pair : data_[section]) {
				if (key_value_pair.first.find_first_of(key + ".") == 0) {
					result[key_value_pair.first.substr(key_value_pair.first.find_first_of('.'))] = key_value_pair.second;
				}
			}
		} while (result.size() != size);
		return result;
	}

	std::map<std::string, int64_t> GetIntegerMap(std::string section, std::string key, std::map<std::string, int64_t> default_value 
		= std::map<std::string, int64_t>()) {
		if (data_.find(section) == data_.end() || data_[section].find(key) == data_[section].end()) {
			SetIntegerMap(section, key, default_value);
			return default_value;
		}
		int32_t size = std::atoi(data_[section][key].c_str());
		std::map<std::string, int64_t> result;
		do {
			for (auto &key_value_pair : data_[section]) {
				if (key_value_pair.first.find_first_of(key + ".") == 0) {
					result[key_value_pair.first.substr(key_value_pair.first.find_first_of('.'))] = stoull(key_value_pair.second);
				}
			}
		} while (result.size() != size);
		return result;
	}

	std::map<std::string, float> GetFloatMap(std::string section, std::string key, std::map<std::string, float> default_value = std::map<std::string, float>()) {
		if (data_.find(section) == data_.end() || data_[section].find(key) == data_[section].end()) {
			SetFloatMap(section, key, default_value);
			return default_value;
		}
		int32_t size = std::atoi(data_[section][key].c_str());
		std::map<std::string, float> result;
		do {
			for (auto &key_value_pair : data_[section]) {
				if (key_value_pair.first.find_first_of(key + ".") == 0) {
					result[key_value_pair.first.substr(key_value_pair.first.find_first_of('.'))] = std::stof(key_value_pair.second);
				}
			}
		} while (result.size() != size);
		return result;
	}

	void SetString(std::string section, std::string key, std::string value) {
		data_[section][key] = value;
	}

	void SetInteger(std::string section, std::string key, int64_t value) {
		data_[section][key] = std::to_string(value);
	}

	void SetFloat(std::string section, std::string key, float value) {
		data_[section][key] = std::to_string(value);
	}

	void SetStringList(std::string section, std::string key, std::vector<std::string> value) {
		data_[section][key] = std::to_string(value.size());
		for (size_t i = 0; i < value.size(); i++) {
			data_[section][key + "." + std::to_string(i)] = value[i];
		}
	}

	void SetIntegerList(std::string section, std::string key, std::vector<int64_t> value) {
		data_[section][key] = std::to_string(value.size());
		for (size_t i = 0; i < value.size(); i++) {
			data_[section][key + "." + std::to_string(i)] = std::to_string(value[i]);
		}
	}

	void SetFloatList(std::string section, std::string key, std::vector<float> value) {
		data_[section][key] = std::to_string(value.size());
		for (size_t i = 0; i < value.size(); i++) {
			data_[section][key + "." + std::to_string(i)] = std::to_string(value[i]);
		}
	}

	void SetStringMap(std::string section, std::string key, std::map<std::string, std::string> value) {
		data_[section][key] = std::to_string(value.size());
		for (auto &pair : value) {
			data_[section][key + "." + pair.first] = pair.second;
		}
	}

	void SetIntegerMap(std::string section, std::string key, std::map<std::string, int64_t> value) {
		data_[section][key] = std::to_string(value.size());
		for (auto &pair : value) {
			data_[section][key + "." + pair.first] = std::to_string(pair.second);
		}
	}

	void SetFloatMap(std::string section, std::string key, std::map<std::string, float> value) {
		data_[section][key] = std::to_string(value.size());
		for (auto &pair : value) {
			data_[section][key + "." + pair.first] = std::to_string(pair.second);
		}
	}

	std::string GetBuffer() {
		buffer_.clear();
		for (auto &section : data_) {
			buffer_.append("[" + section.first + "]\n");
			for (auto &data : section.second) {
				buffer_.append(data.first + "=" + data.second + "\n");
			}
			buffer_.append("\n");
		}
		for (auto it = buffer_.begin(); it != buffer_.end();) {
			if (*it == '\0') {
				it = buffer_.erase(it);
			} else {
				++it;
			}
		}
		return buffer_;
	}
};
}

#endif // indigo_config_hpp_