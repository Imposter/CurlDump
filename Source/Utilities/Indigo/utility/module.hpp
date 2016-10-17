/*
*   This file is part of the Indigo library.
*
*   This program is licensed under the GNU General
*   Public License. To view the full license, check
*   LICENSE in the project root.
*/

#ifndef indigo_module_hpp_
#define indigo_module_hpp_

#include "memory.hpp"
#include <stdlib.h>
#include <stdio.h>

namespace indigo {
class Module {
protected:
	void *memory_;
	size_t size_;

	void *module_;
	bool loaded_;

public:
	Module(void *memory, size_t size);
	Module(const Module &) = delete;
	~Module();

	bool Load();
	bool Unload();

	// This method does not return if successful
	bool Execute() const;

	void *GetExport(std::string name) const;
	void *GetImage() const;
};

class ModuleMemory : public Module {
public:
	ModuleMemory(void* memory, size_t size)
		: Module(memory, size) {
	}

	ModuleMemory(const Module &) = delete;
};

class ModuleFile : public Module {
	std::string path_;

public:
	ModuleFile(std::string path)
		: Module(nullptr, 0), path_(path) {
	}

	ModuleFile(const ModuleFile &) = delete;

	~ModuleFile() {
		Unload();
	}

	bool Load() {
		// Check if already open
		if (loaded_) {
			return false;
		}

		// Open the file
		FILE *file = fopen(path_.c_str(), "rb");
		if (file == nullptr) {
			return false;
		}

		// Get file size
		fseek(file, 0, SEEK_END);
		size_t size = ftell(file);
		fseek(file, 0, SEEK_SET);

		// Allocate the memory
		memory_ = malloc(size);
		size_ = size;

		// Read the file
		if (fread(memory_, sizeof(char), size_, file) != size) {
			fclose(file);
			free(memory_);
			return false;
		}

		return Module::Load();
	}

	bool Unload() {
		if (memory_ != nullptr) {
			// Free memory
			free(memory_);
			memory_ = nullptr;
			size_ = 0;
		}

		return Module::Unload();
	}
};
}

#endif // indigo_module_hpp_