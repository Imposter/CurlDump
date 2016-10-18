/*
*   This file is part of the Indigo library.
*
*   This program is licensed under the GNU General
*   Public License. To view the full license, check
*   LICENSE in the project root.
*/

#ifndef indigo_console_hpp_
#define indigo_console_hpp_

#include "../platform.h"
#if !defined(OS_WIN)
#error "Unsupported platform!"
#endif

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace indigo {
class Console {
public:
	static void Show(std::string title = "Console") {
		AllocConsole();
		AttachConsole(GetCurrentProcessId());

		FILE *stream;
		freopen_s(&stream, "CONIN$", "r", stdin);
		freopen_s(&stream, "CONOUT$", "w", stdout);
		freopen_s(&stream, "CONOUT$", "w", stderr);

		SetConsoleTitleA(title.c_str());

		ShowWindow(GetConsoleWindow(), SW_SHOW);
	}

	static void Hide() {
		ShowWindow(GetConsoleWindow(), SW_HIDE);
		FreeConsole();
	}
};
}

#endif // indigo_console_hpp_