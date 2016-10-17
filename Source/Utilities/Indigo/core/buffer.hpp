/*
*   This file is part of the Indigo library.
*
*   This program is licensed under the GNU General
*   Public License. To view the full license, check
*   LICENSE in the project root.
*/

#ifndef indigo_buffer_hpp_
#define indigo_buffer_hpp_

// Required libraries
#include <vector>
#include <stdint.h>

namespace indigo {
// Provides a way to write to a buffer with objects of any type or to serialize
// an object type to a byte array
// Example:
//    struct Person {
//      char FirstName[32];
//      char LastName[32];
//      int Age;
//    }
//    ...
//    Person p;
//    strcpy(p.FirstName, "John");
//    strcpy(p.LastName, "Doe");
//    p.Age = 25;
//
//    Buffer b;
//    b.Write(p);
//    b.Rewind();
//
//    Person p2;
//    b.Read(&p2);
//    printf("%s %s is %i years old.\n", p2.FirstName, p2.LastName, p2.Age);
class Buffer {
	// Internal buffer used for storing the data written. Used for later reading
	// or writing.
	std::vector<uint8_t> buffer_;

	// The currrent position of the buffer. Used for reading and writing to
	// determine if data needs to be appended or overwritten.
	size_t current_position_;

	// Used to flip endian order if required. In case two different hosts with
	// different endian orders have accessed/written to the buffer. For example,
	// AMD vs Intel CPUs or Network vs Host endian order.
	bool flip_endian_;

	template<typename _TData>
	static void FlipEndian(_TData *buffer, size_t size) {
		// Reverse the array
		uint8_t *p_buffer = *reinterpret_cast<uint8_t **>(&buffer);
		for (size_t i = 0; i < size / 2; i++) {
			uint8_t tmp = p_buffer[i];
			p_buffer[i] = p_buffer[size - 1 - i];
			p_buffer[size - 1 - i] = tmp;
		}
	}

public:
	Buffer(bool flip_endian = false) : current_position_(0),
		flip_endian_(flip_endian) {
	}

	Buffer(const uint8_t *buffer, size_t size, bool flip_endian = false)
		: current_position_(0), flip_endian_(flip_endian) {
		// Copy the data to the internal buffer
		for (size_t i = 0; i < size; i++) {
			buffer_.push_back(buffer[i]);
		}
	}

	~Buffer() {
		// Set all the data to 0 in case we had important data in the buffer
		for (size_t i = 0; i < buffer_.size(); i++) {
			buffer_[i] = 0;
		}
	}

	template<typename _TData>
	bool Read(_TData *obj) {
		// Check if there is enough data to read
		size_t size = sizeof(_TData);
		if (current_position_ + size > buffer_.size()) {
			return false;
		}
		// Read the data into the object buffer
		uint8_t *p_buffer = *reinterpret_cast<uint8_t **>(&obj);
		for (size_t i = 0; i < size; i++) {
			p_buffer[i] = buffer_.data()[current_position_];
			current_position_++;
		}
		// Flip the endian order of the object
		// if needed
		if (flip_endian_) {
			FlipEndian(obj, size);
		}
		return true;
	}

	template<typename _TData>
	bool ReadArray(_TData *obj, size_t size) {
		// Check if there is any data to read
		if (current_position_ + size * sizeof(_TData) - 1 > buffer_.size()) {
			return false;
		}
		// Read the array object by object
		for (size_t i = 0; i < size; i++) {
			if (!Read<_TData>(&obj[i])) {
				return false;
			}
		}
		return true;
	}

	template<typename _TData>
	void Write(_TData obj) {
		// Get the pointer to the data object and flip the object in
		// case the endian order needs changing
		uint8_t *p_buffer = reinterpret_cast<uint8_t *>(&obj);
		size_t size = sizeof(_TData);
		if (flip_endian_) {
			FlipEndian(p_buffer, size);
		}
		for (size_t i = 0; i < size; i++) {
			// Check if we need to append to the buffer or
			// replace existing data
			if (current_position_ + i >= this->buffer_.size()) {
				buffer_.push_back(p_buffer[i]);
			} else {
				buffer_[current_position_] = p_buffer[i];
			}
			current_position_++;
		}
	}

	template<typename _TData>
	void WriteArray(_TData *obj, size_t size) {
		// Write each object in the array
		for (size_t i = 0; i < size; i++) {
			Write(obj[i]);
		}
	}

	template<typename _TData>
	void WriteArray(const _TData *obj, size_t size) {
		// Write each object in the array
		for (size_t i = 0; i < size; i++) {
			Write(obj[i]);
		}
	}

	const size_t &GetPosition() const {
		return current_position_;
	}

	bool SetPosition(size_t current_position) {
		// If the specified position is past the end of the buffer
		// return false
		if (current_position > buffer_.size()) {
			return false;
		}
		current_position_ = current_position;
		return true;
	}

	const bool &IsFlippingEndian() const {
		return flip_endian_;
	}

	void SetFlipEndian(bool flip_endian) {
		flip_endian_ = flip_endian;
	}

	void Rewind() {
		current_position_ = 0;
	}

	void Resize(size_t size) {
		buffer_.resize(size);
	}

	const size_t &GetSize() const {
		return buffer_.size();
	}

	const uint8_t *GetBuffer() const {
		return buffer_.data();
	}

	void Clear() {
		// Set all the data to 0 in case we had important data in the buffer
		for (size_t i = 0; i < buffer_.size(); i++) {
			buffer_[i] = 0;
		}

		// Clear the buffer
		buffer_.clear();
		buffer_.shrink_to_fit();
		current_position_ = 0;
	}
};
}

#endif // indigo_buffer_hpp_