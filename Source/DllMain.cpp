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
#include "Curl.h"
#include "Utilities/Indigo/utility/hook.hpp"
#include "Utilities/Indigo/utility/acp_dump.hpp"
#include "Utilities/Indigo/utility/config.hpp"

#ifdef _DEBUG
#include "Utilities/Indigo/utility/console.hpp"
#endif

#include <fstream>
#include <cstdarg>
#include <map>
#include <mutex>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

indigo::ACPDump acp_dump_;
std::map<void *, bool> redirect_set_;
std::mutex redirect_mutex_;
indigo::CallHook curl_setopt_hook_;
indigo::CallHook curl_close_hook_;

size_t curl_write_callback(char *data, size_t size, size_t bytes, void *userdata) {
	// actual size == size * nmemb
}

size_t curl_read_callback(char *buffer, size_t size, size_t bytes, void *userdata) {
	// actual size == size * nmemb
}

// 0x0138F740 int __cdecl Curl_setopt(void *handle, signed int option, unsigned int *param)
int __cdecl curl_setopt_(void *handle, signed int option, unsigned int *param) {
	// Get the original
	auto curl_setopt = curl_setopt_hook_.Get<int(*__cdecl)(void *, signed int, unsigned int *)>();

	redirect_mutex_.lock();
	if (redirect_set_.find(handle) == redirect_set_.end()) {
		// Initialize
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
		indigo::Console::Show();
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
		void *setopt = reinterpret_cast<void *>(config.GetInteger("CURL", "SetOpt"));
		void *close = reinterpret_cast<void *>(config.GetInteger("CURL", "Close"));

		// Install hooks
		if (!curl_setopt_hook_.Install(setopt, &curl_setopt_) || curl_close_hook_.Install(close, &curl_close_)) {
			printf("CurlDump: Failed to install hooks\n");
			return;
		}

		// Open dump
		std::string file_name = indigo::String::Format("curldump_%i.acp", time(nullptr));
		if (!acp_dump_.Open(file_name)) {
			printf("CurlDump: Failed to open %s\n", file_name.c_str());
			return;
		}

		printf("CurlDump: We're ready to go!\n");
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