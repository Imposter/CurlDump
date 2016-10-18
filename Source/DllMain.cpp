/*
*
*   Title: CurlDump
*   Authors: Eyaz Rehman [http://github.com/Imposter]
*   Date: 2/18/2016
*
*   Copyright (C) 2016 Eyaz Rehman. All Rights Reserved.
*
*/

#include "Configuration/All.h"
#include "Utilities/Indigo/utility/hook.hpp"
#include "Utilities/Indigo/utility/acp_dump.hpp"
#include "Utilities/Indigo/utility/config.hpp"
#include "Curl.h"

#ifdef _DEBUG
#include "Utilities/Indigo/utility/console.hpp"
#endif

#include <fstream>
#include <cstdarg>
#include <map>
#include <mutex>
#include <thread>
#include <chrono>
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>

#pragma comment(lib, "ws2_32.lib")

typedef size_t (*__cdecl CurlIOCallback)(char *data, size_t size, size_t bytes, void *userdata);

struct CurlInstance {
	void *Handle;
	void *WriteData;
	void *ReadData;
	CurlIOCallback WriteCallback;
	CurlIOCallback ReadCallback;
};

indigo::ACPDump acp_dump_;
std::map<void *, CurlInstance *> redirect_set_;
std::mutex redirect_mutex_;
indigo::CallHook curl_setopt_hook_;
indigo::CallHook curl_close_hook_;

size_t curl_write_callback(char *data, size_t size, size_t bytes, CurlInstance *instance) {
	// actual size == size * bytes
	printf("CurlDump: (0x%08p) Writing %d bytes\n", instance->Handle, size * bytes);
	acp_dump_.Write(SOCK_STREAM, IPPROTO_TCP, static_cast<uint32_t>(inet_addr("127.0.0.1")),
		reinterpret_cast<uint16_t>(instance->Handle), reinterpret_cast<uint32_t>(instance->Handle), 1337, data, size * bytes);

	return instance->WriteCallback != nullptr ? instance->WriteCallback(data, size, bytes, instance->WriteData) : 0;
}

size_t curl_read_callback(char *data, size_t size, size_t bytes, CurlInstance *instance) {
	// actual size == size * bytes
	printf("CurlDump: (0x%08p) Reading %d bytes\n", instance->Handle, size * bytes);
	acp_dump_.Write(SOCK_STREAM, IPPROTO_TCP, reinterpret_cast<uint32_t>(instance->Handle), 1337,
		static_cast<uint32_t>(inet_addr("127.0.0.1")), reinterpret_cast<uint16_t>(instance->Handle), data, size * bytes);

	return instance->ReadCallback != nullptr ? instance->ReadCallback(data, size, bytes, instance->ReadData) : 0;
}

// int __cdecl Curl_setopt(void *handle, signed int option, va_list param)
int __cdecl curl_setopt_(void *handle, signed int option, va_list param) {
	printf("CurlDump: Curl_setopt(0x%08p, %d, 0x%08p)\n", handle, option, param);

	// Curl instance
	CurlInstance *instance;

	// Get the original
	auto curl_setopt = curl_setopt_hook_.Get<int(*__cdecl)(void *, signed int, va_list)>();

	redirect_mutex_.lock();
	if (redirect_set_.find(handle) == redirect_set_.end()) {
		// Initialize
		printf("CurlDump: Monitoring easy handle 0x%08p\n", handle);

		// Create curl instance
		instance = new CurlInstance{ nullptr };
		instance->Handle = handle;

		if (option == CURLOPT_WRITEDATA) {
			instance->WriteData = va_arg(param, void *);
		} else if (option == CURLOPT_READDATA) {
			instance->ReadData = va_arg(param, void *);
		} else if (option == CURLOPT_WRITEFUNCTION) {
			instance->WriteCallback = va_arg(param, CurlIOCallback);
		} else if (option == CURLOPT_READFUNCTION) {
			instance->ReadCallback = va_arg(param, CurlIOCallback);
		}

		auto setopt = [=](CURLoption opt, ...) {
			printf("CurlDump: Setting option %d for easy handle 0x%08p\n", opt, handle);

			va_list va;
			va_start(va, opt);

			curl_setopt(handle, opt, va);

			va_end(va);
		};

#ifdef _DEBUG
		setopt(CURLOPT_VERBOSE, 1);
#endif
		setopt(CURLOPT_WRITEDATA, instance);
		setopt(CURLOPT_READDATA, instance);
		setopt(CURLOPT_WRITEFUNCTION, &curl_write_callback);
		setopt(CURLOPT_READFUNCTION, &curl_read_callback);

		redirect_set_[handle] = instance;
	}

	instance = redirect_set_[handle];
	redirect_mutex_.unlock();
	if (option == CURLOPT_WRITEDATA) {
		instance->WriteData = va_arg(param, void *);
		return 0;
	} 
	if (option == CURLOPT_READDATA) {
		instance->ReadData = va_arg(param, void *);
		return 0;
	} 
	if (option == CURLOPT_WRITEFUNCTION) {
		instance->WriteCallback = va_arg(param, CurlIOCallback);
		return 0;
	} 
	if (option == CURLOPT_READFUNCTION) {
		instance->ReadCallback = va_arg(param, CurlIOCallback);
		return 0;
	}

	return curl_setopt(handle, option, param);
}

// int __cdecl Curl_close(void *handle)
int __cdecl curl_close_(void *handle) {
	redirect_mutex_.lock();
	for (auto it = redirect_set_.begin(); it != redirect_set_.end();) {
		if (it->first == handle) {
			delete it->second;
			it = redirect_set_.erase(it);
			break;
		} 
		++it;
	}
	redirect_mutex_.unlock();

	return curl_close_hook_.Get<int(*__cdecl)(void *)>()(handle);
}

extern "C" {
	EXPORT_ATTR void __cdecl onExtensionUnloading(void) {
		// Remove curl instances
		redirect_mutex_.lock();
		for (auto it = redirect_set_.begin(); it != redirect_set_.end();) {
			delete it->second;
			it = redirect_set_.erase(it);
		}
		redirect_mutex_.unlock();

		// Close dump
		acp_dump_.Close();

#ifdef _DEBUG
		indigo::Console::Hide();
#endif
	}

	EXPORT_ATTR void __cdecl onInitializationStart(void) {
#ifdef _DEBUG
		indigo::Console::Show("Ayria Console");

		MessageBoxA(nullptr, "Attach debugger now!", "CurlDump - Debug", MB_OK);
#endif

		// Open the config file
		std::ifstream config_file;
		config_file.open("curldump.ini");
		if (!config_file.is_open()) {
			printf("CurlDump: Failed to open curldump.ini\n");
			return;
		}

		config_file.seekg(0, std::ios::end);
		size_t config_size = config_file.tellg();
		config_file.seekg(0, std::ios::beg);

		std::string config_buffer;
		config_buffer.resize(config_size);
		config_file.read(const_cast<char *>(config_buffer.c_str()), config_size);

		indigo::Config config;
		if (!config.Open(config_buffer)) {
			printf("CurlDump: Failed to read curldump.ini\n");
			return;
		}

		// Get addresses for Curl_setopt and Curl_close
		std::string setopt = config.GetString("CURL", "SetOpt");
		std::string close = config.GetString("CURL", "Close");
		int32_t delay = config.GetInteger("CURL", "HookDelay", 1);

		if (setopt.empty()) {
			printf("CurlDump: Invalid Curl_setopt pattern\n");
			return;
		}

		if (close.empty()) {
			printf("CurlDump: Invalid Curl_close pattern\n");
			return;
		}

		// Install hooks
		std::thread([=]() {
			std::this_thread::sleep_for(std::chrono::milliseconds(delay));

			while (true) {
				std::this_thread::sleep_for(std::chrono::milliseconds(10));

				if (!curl_setopt_hook_.Install(indigo::Memory::Find(setopt.c_str()).Get(0).Get<void *>(), &curl_setopt_)) {
					printf("CurlDump: Failed to install Curl_setopt hook\n");
					continue;
				}

				if (!curl_close_hook_.Install(indigo::Memory::Find(close.c_str()).Get(0).Get<void *>(), &curl_close_)) {
					printf("CurlDump: Failed to install Curl_close hook\n");
					continue;
				}

				// Open dump
				std::string file_name = indigo::String::Format("curldump_%i.acp", time(nullptr));
				if (!acp_dump_.Open(file_name)) {
					printf("CurlDump: Failed to open %s\n", file_name.c_str());
					return;
				}

				break;
			}

			printf("CurlDump: We're ready to go!\n");
		}).detach();
	}

	EXPORT_ATTR void __cdecl onInitializationComplete(void) {
	}

	EXPORT_ATTR void __cdecl onMessage(uint32_t message, ...) {
	}
}

BOOLEAN WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
	if (reason == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(instance);
		DeleteLogfile();
	}

	return TRUE;
}