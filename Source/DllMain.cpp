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

indigo::ACPDump acp_dump_;
std::map<void *, bool> redirect_set_;
std::mutex redirect_mutex_;
indigo::CallHook curl_setopt_hook_;
indigo::CallHook curl_close_hook_;

size_t curl_write_callback(char *data, size_t size, size_t bytes, void *userdata) {
	// actual size == size * bytes
	printf("CurlDump: Writing %d bytes\n", size);
	acp_dump_.Write(SOCK_STREAM, IPPROTO_TCP, static_cast<uint32_t>(inet_addr("127.0.0.1")),
		reinterpret_cast<uint16_t>(userdata), reinterpret_cast<uint32_t>(userdata), 1337, data, size * bytes);

	return size * bytes;
}

size_t curl_read_callback(char *data, size_t size, size_t bytes, void *userdata) {
	// actual size == size * bytes
	printf("CurlDump: Reading %d bytes\n", size);
	acp_dump_.Write(SOCK_STREAM, IPPROTO_TCP, reinterpret_cast<uint32_t>(userdata), 1337, 
		static_cast<uint32_t>(inet_addr("127.0.0.1")), reinterpret_cast<uint16_t>(userdata), data, size * bytes);

	return size * bytes;
}

// 0x0138F740 int __cdecl Curl_setopt(void *handle, signed int option, unsigned int *param)
int __cdecl curl_setopt_(void *handle, signed int option, unsigned int *param) {
	// Get the original
	auto curl_setopt = curl_setopt_hook_.Get<int(*__cdecl)(void *, signed int, unsigned int *)>();

	redirect_mutex_.lock();
	if (redirect_set_.find(handle) == redirect_set_.end()) {
		// Initialize
		printf("CurlDump: Redirecting handle 0x%08p\n", handle);

		// TODO/NOTE: curl_setopt takes a variadic argument, see curl_easy_setopt

		curl_setopt(handle, CURLOPT_VERBOSE, reinterpret_cast<unsigned int *>(1));
		curl_setopt(handle, CURLOPT_WRITEDATA, static_cast<unsigned int *>(handle));
		curl_setopt(handle, CURLOPT_READDATA, static_cast<unsigned int *>(handle));
		curl_setopt(handle, CURLOPT_WRITEFUNCTION, reinterpret_cast<unsigned int *>(&curl_write_callback));
		curl_setopt(handle, CURLOPT_READFUNCTION, reinterpret_cast<unsigned int *>(&curl_read_callback));

		redirect_set_[handle] = true;
	}

	redirect_mutex_.unlock();
	return curl_setopt(handle, option, param);
}

// 0x0138F330 int __cdecl Curl_close(void *handle)
int __cdecl curl_close_(void *handle) {
	redirect_mutex_.lock();
	for (auto it = redirect_set_.begin(); it != redirect_set_.end();) {
		if (it->first == handle) {
			it = redirect_set_.erase(it);
		} else {
			++it;
		}
	}
	redirect_mutex_.unlock();

	return curl_close_hook_.Get<int(*__cdecl)(void *)>()(handle);
}

extern "C" {
	EXPORT_ATTR void __cdecl onExtensionUnloading(void) {
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

		// Install hooks
		std::thread([=]() {
			std::this_thread::sleep_for(std::chrono::milliseconds(delay));

			while (true) {
				std::this_thread::sleep_for(std::chrono::milliseconds(10));

				if (!curl_setopt_hook_.Install(indigo::Memory::Find(setopt.c_str()).Get(0).Get<void *>(), &curl_setopt_)) {
					printf("CurlDump: Failed to install Curl_setopt hook\n");
					continue;
				}

				// "Optional"
				if (!close.empty()) {
					if (!curl_close_hook_.Install(indigo::Memory::Find(close.c_str()).Get(0).Get<void *>(), &curl_close_)) {
						printf("CurlDump: Failed to install Curl_close hook\n");
						continue;
					}
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
		va_list variadic;
		va_start(variadic, message);

		// Messages are a 32bit FNV1a hash of a string
		switch (message)
		{

		case FNV1a_Compiletime_32("DefaultCase"):
		default:
			break;
		}

		va_end(variadic);
	}
}

BOOLEAN WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
	if (reason == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(instance);
		DeleteLogfile();
	}

	return TRUE;
}