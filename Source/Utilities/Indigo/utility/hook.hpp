/*
*   This file is part of the Indigo library.
*
*   This program is licensed under the GNU General
*   Public License. To view the full license, check
*   LICENSE in the project root.
*/

#ifndef indigo_hook_hpp_
#define indigo_hook_hpp_

#include "../platform.h"
#if !defined(OS_WIN)
#error "Unsupported platform!"
#endif

#include "memory.hpp"
#include "../core/string.hpp"
#include "minhook/MinHook.h"

namespace indigo {
class Hook {
public:
	virtual ~Hook() {}

	virtual bool Install() = 0;
	virtual bool Remove() = 0;

	/**
	* \brief Static utility function to install a call hook, of which external use has been deprecated in favour of Hook::Create<CallHook>(...)
	* \param target Target address (being hooked)
	* \param function Pointer to replacement function (replacing target)
	* \param original Pointer to original function (saved for future usage)
	* \return Returns true if hooking was successful
	*/
	static bool Install(void *target, void *function, void **original = nullptr) {
		MH_STATUS status = MH_CreateHook(target, function, original);
		if (status == MH_ERROR_NOT_INITIALIZED) {
			MH_Initialize();
			MH_CreateHook(target, function, original);
		}

		status = MH_EnableHook(reinterpret_cast<void *>(target));

		return status == MH_OK;
	}

	/**
	* \brief Static utility function to remove a call hook, of which external use has been deprecated in favour of CallHook::Remove()
	* \param original The original function pointing to the original code
	* \return Returns true if removing the hook was successful
	*/
	static bool Remove(void **original) {
		if (MH_DisableHook(original) != MH_OK) {
			return false;
		}

		return MH_RemoveHook(original) == MH_OK;
	}

	/**
	* \brief Static utility function to install an export call hook, of which external use has been deprecated in favour of Hook::Create<EATHook>(...)
	* \param module_name The target module name
	* \param export_name The target export, ordinal (int) or named (const char *)
	* \param function Pointer to replacement function (replacing target)
	* \param original Pointer to original function (saved for future usage)
	* \return Returns true if hooking was successful
	*/
	static bool Install(const char *module_name, const char *export_name, void *function, void **original = nullptr) {
		HMODULE module = LoadLibraryA(module_name);
		if (module == nullptr) {
			return false;
		}

		void *target = GetProcAddress(module, export_name);
		if (target == nullptr) {
			return false;
		}

		return Install(target, function, original);
	}

	/**
	 * \brief Static utility function to install an import hook, of which external use has been deprecated in favour of Hook::Create<IATHook>(...)
	 * \param module The instance of which the hook is intended for
	 * \param module_name The target module name 
	 * \param import_name The target import, ordinal (int) or named (const char *)
	 * \param function Pointer to replacement function (replacing target)
	 * \param original Pointer to original function (saved for future usage)
	 * \return Returns true if hooking was successful
	 */
	static bool Install(void *module, const char *module_name, const char *import_name, void *function, void **original = nullptr) {
		IMAGE_NT_HEADERS *nt_headers = Memory::GetNTHeader(module);
		IMAGE_DATA_DIRECTORY *import_directory = &nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
		IMAGE_IMPORT_DESCRIPTOR *descriptor = Memory::GetRVA<IMAGE_IMPORT_DESCRIPTOR>(module, import_directory->VirtualAddress);

		// Search imports
		for (; descriptor->Name; descriptor++) {
			// Check the module name
			const char *current_module_name = Memory::GetRVA<char>(module, descriptor->Name);
			if (!String::Equals(current_module_name, module_name, true)) {
				continue;
			}

			// Pointer to name and address entries
			uintptr_t *thunk_ref = Memory::GetRVA<uintptr_t>(module, descriptor->OriginalFirstThunk);
			FARPROC *func_ref = Memory::GetRVA<FARPROC>(module, descriptor->FirstThunk);

			// Check if we have hints
			if (!descriptor->OriginalFirstThunk) {
				thunk_ref = reinterpret_cast<uintptr_t *>(func_ref);
			}

			// Loop through all the functions
			bool replace;
			for (; *thunk_ref; thunk_ref++, func_ref++) {
				if (IMAGE_SNAP_BY_ORDINAL(*thunk_ref)) {
					replace = reinterpret_cast<char *>(IMAGE_ORDINAL(*thunk_ref)) == import_name;
				} else {
					IMAGE_IMPORT_BY_NAME *import_by_name = Memory::GetRVA<IMAGE_IMPORT_BY_NAME>(module, *thunk_ref);
					replace = strcmp(import_by_name->Name, import_name) == 0;
				}

				if (replace) {
					// Unprotect function
					DWORD func_prot = 0;
					VirtualProtect(func_ref, sizeof(func_ref), PAGE_EXECUTE_READWRITE, &func_prot);

					// Store original
					if (original != nullptr) {
						*original = *func_ref;
					}

					// Change reference to our function
					*func_ref = static_cast<FARPROC>(function);

					// Protect function
					VirtualProtect(func_ref, sizeof(func_ref), func_prot, nullptr);

					return true;
				}			
			}
		}

		return false;
	}
};

class IATHook : public Hook {
	void *module_;
	const char *module_name_;
	const char *import_name_;
	void *target_;
	void *redirect_;
	void *original_;
	bool installed_;

public:
	IATHook()
		: module_(nullptr), module_name_(nullptr), import_name_(nullptr), target_(nullptr), redirect_(nullptr), original_(nullptr), installed_(false) {
	}

	~IATHook() {
		IATHook::Remove();
	}

	bool Install() override {
		if (installed_ || module_name_ == nullptr) {
			return false;
		}

		if (!Hook::Install(module_, module_name_, import_name_, redirect_, &original_)) {
			return false;
		}

		installed_ = true;

		return true;
	}

	template<typename _TImport>
	bool Install(void *source_module, const char *module, _TImport import_name, void *redirect) {
		if (installed_) {
			return false;
		}

		module_ = source_module;
		module_name_ = module;
		import_name_ = import_name;
		redirect_ = redirect;

		return Install();
	}

	bool Remove() override {
		if (!installed_) {
			return false;
		}

		if (!Hook::Install(module_, module_name_, import_name_, original_, nullptr)) {
			return false;
		}

		installed_ = false;

		return true;
	}

	template<typename _TFunction>
	_TFunction Get() {
		return static_cast<_TFunction>(original_);
	}
};

class EATHook : public Hook {
	const char *module_name_;
	const char *export_name_;
	void *redirect_;
	void *original_;
	bool installed_;

public:
	EATHook() 
		: module_name_(nullptr), export_name_(nullptr), redirect_(nullptr), original_(nullptr), installed_(false) {
	}

	~EATHook() {
		EATHook::Remove();
	}

	bool Install() override {
		if (installed_ || module_name_ == nullptr) {
			return false;
		}

		if (!Hook::Install(module_name_, export_name_, redirect_, &original_)) {
			return false;
		}

		installed_ = true;

		return true;
	}

	template<typename _TExport>
	bool Install(const char *module, _TExport export_name, void *redirect, void *original = nullptr) {
		if (installed_) {
			return false;
		}

		module_name_ = module;
		export_name_ = export_name;
		redirect_ = redirect;
		original_ = original;

		return Install();
	}

	bool Remove() override {
		if (!installed_) {
			return false;
		}

		if (!Hook::Remove(&original_)) {
			return false;
		}

		installed_ = false;

		return true;
	}

	template<typename _TFunction>
	_TFunction Get() {
		return static_cast<_TFunction>(original_);
	}
};

class CallHook : public Hook {
	void *target_;
	void *redirect_;
	void *original_;
	bool installed_;

public:
	CallHook()
		: target_(nullptr), redirect_(nullptr), original_(nullptr), installed_(false) {	
	}

	~CallHook() {
		CallHook::Remove();
	}

	bool Install() override {
		if (installed_ || target_ == nullptr) {
			return false;
		}

		if (!Hook::Install(target_, redirect_, &original_)) {
			return false;
		}

		installed_ = true;

		return true;
	}

	bool Install(void *target, void *redirect) {
		if (installed_) {
			return false;
		}

		target_ = target;
		redirect_ = redirect;

		return Install();
	}

	bool Remove() override {
		if (!installed_) {
			return false;
		}

		if (!Hook::Remove(&original_)) {
			return false;
		}

		installed_ = false;

		return true;
	}

	template<typename _TFunction>
	_TFunction Get() {
		return static_cast<_TFunction>(original_);
	}
};
}

#endif // indigo_hook_hpp_