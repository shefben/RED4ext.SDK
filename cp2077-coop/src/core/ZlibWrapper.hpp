#pragma once

// CRITICAL: Windows zlib compatibility wrapper
// This header ensures we use the correct zlib headers without CURL conflicts

#ifdef _WIN32
// Prevent all possible unistd.h inclusion attempts
#define _CRT_NONSTDC_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

// zlib configuration - prevent unistd.h usage
#define HAVE_ZLIB_H 1
#define NO_UNISTD_H
#define ZLIB_CONST
#define Z_SOLO
#define NO_GZIP

// Tell zlib/CURL we DON'T have unistd.h
#ifdef Z_HAVE_UNISTD_H
#undef Z_HAVE_UNISTD_H
#endif
#ifdef _LARGEFILE64_SOURCE
#undef _LARGEFILE64_SOURCE
#endif
#define CURL_DISABLE_UNISTD_H

// Include Windows replacements for POSIX functions
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <direct.h>

// Define unistd.h replacement functions to prevent any accidental inclusion
#ifndef _CRT_INTERNAL_NONSTDC_NAMES
#define access _access
#define close _close
#define open _open
#define read _read
#define write _write
#define lseek _lseek
#define mkdir _mkdir
#define rmdir _rmdir
#define unlink _unlink
#define getcwd _getcwd
#define chdir _chdir
#endif

#endif // _WIN32

// Now manually include zconf.h from the build directory first
// This ensures we get the Windows-configured version that doesn't try to include unistd.h
#include "../../third_party/zlib/build/zconf.h"

// Define the path to prevent zlib.h from including its own zconf.h
#define ZCONF_H

// Now include the main zlib.h header
// Since we already included zconf.h and defined ZCONF_H, zlib.h won't try to include it again
#include "../../third_party/zlib/zlib.h"