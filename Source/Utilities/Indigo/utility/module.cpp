/*
*   This file is part of the Indigo library.
*
*   This program is licensed under the GNU General
*   Public License. To view the full license, check
*   LICENSE in the project root.
*/

#include "utility/module.hpp"
#include "memory_module.h"

namespace indigo {
typedef int (WINAPI *ExeEntryProc)(void);
typedef struct {
	PIMAGE_NT_HEADERS headers;
	unsigned char *codeBase;
	HCUSTOMMODULE *modules;
	int numModules;
	BOOL initialized;
	BOOL isDLL;
	BOOL isRelocated;
	CustomAllocFunc alloc;
	CustomFreeFunc free;
	CustomLoadLibraryFunc loadLibrary;
	CustomGetProcAddressFunc getProcAddress;
	CustomFreeLibraryFunc freeLibrary;
	void *userdata;
	ExeEntryProc exeEntry;
	DWORD pageSize;
} MEMORYMODULE, *PMEMORYMODULE;

Module::Module(void *memory, size_t size)
	: memory_(memory), size_(size), module_(nullptr), loaded_(false)  {
}

Module::~Module() {
	Unload();
}

bool Module::Load() {
	// Check if already loaded
	if (loaded_) {
		return false;
	}

	// Try to load the module
	if ((module_ = MemoryLoadLibraryEx(memory_, size_, MemoryDefaultAlloc, MemoryDefaultFree,
		MemoryDefaultLoadLibrary, MemoryDefaultGetProcAddress, MemoryDefaultFreeLibrary, nullptr)) == nullptr) {
		return false;
	}

	// Set as loaded
	loaded_ = true;

	return true;
}

bool Module::Unload() {
	// Check if not loaded
	if (!loaded_) {
		return false;
	}

	// Unload the module
	MemoryFreeLibrary(module_);

	// Set as unloaded
	loaded_ = false;

	return true;
}

bool Module::Execute() const {
	// Check if loaded
	if (!loaded_) {
		return false;
	}

	// Update the entrypoint
	MEMORYMODULE *module = reinterpret_cast<MEMORYMODULE *>(module_);
	IMAGE_NT_HEADERS *nt_headers = Memory::GetNTHeader(module->codeBase);
	module->exeEntry = reinterpret_cast<ExeEntryProc>(nt_headers->OptionalHeader.ImageBase + nt_headers->OptionalHeader.AddressOfEntryPoint);

	// Call the entrypoint
	if (MemoryCallEntryPoint(module_) == -1) {
		return false;
	}

	return true;
}

void *Module::GetExport(std::string name) const {
	return MemoryGetProcAddress(module_, name.c_str());
}

void *Module::GetImage() const {
	return reinterpret_cast<MEMORYMODULE *>(module_)->codeBase;
}
}
