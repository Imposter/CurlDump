/*
*   This file is part of the Indigo library.
*
*   This program is licensed under the GNU General
*   Public License. To view the full license, check
*   LICENSE in the project root.
*/

#ifndef indigo_platform_h_
#define indigo_platform_h_

#if defined(__linux__)
#define OS_LINUX
#elif defined(__APPLE__)
#define OS_OSX
#elif defined(__unix)
#define OS_UNIX
#elif defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__) \
  || defined(__MINGW32__)
#define OS_WIN
#else
#error "Unsupported platform!"
#endif

#if defined(_WIN64) || defined(__CYGWIN64__) || defined(__MINGW64__)
#define OS_X64
#elif defined(_WIN32) || defined(__CYGWIN__) || defined(__MINGW32__)
#define OS_X86
#else
#error "Unsupported architecture!"
#endif

#endif // indigo_platform_h_