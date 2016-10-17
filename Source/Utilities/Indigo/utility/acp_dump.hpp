/*
*   This file is part of the Indigo library.
*
*   This program is licensed under the GNU General
*   Public License. To view the full license, check
*   LICENSE in the project root.
*/

#ifndef INDIGO_UTILITY_ACP_DUMP_H_
#define INDIGO_UTILITY_ACP_DUMP_H_

#include <stdint.h>
#include <mutex>

namespace indigo {
class ACPDump {
	FILE *file_;
	bool is_open_;
	std::mutex mutex_;

public:
	ACPDump();

	bool Open(std::string file_name);
	void Close();

	bool Write(int32_t type, int32_t protocol, uint32_t source_address, uint16_t source_port, 
		uint32_t destination_address, uint16_t destination_port, char *buffer, size_t length) const;
};
}

#endif // INDIGO_UTILITY_ACP_DUMP_H_