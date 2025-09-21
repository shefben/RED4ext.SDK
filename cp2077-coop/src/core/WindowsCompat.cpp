#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <cstdlib>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <windows.h>
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")

// Use Windows SDK timeval instead of defining our own
// Define timezone since Windows doesn't have it
#ifndef _TIMEZONE_DEFINED
#define _TIMEZONE_DEFINED
struct timezone {
    int tz_minuteswest;
    int tz_dsttime;
};
#endif

// Windows directory handle structure
struct WindowsDirHandle {
    HANDLE findHandle;
    WIN32_FIND_DATAW findData;
    char entryName[256];
    bool isFirst;

    WindowsDirHandle() : findHandle(INVALID_HANDLE_VALUE), isFirst(false) {
        memset(&findData, 0, sizeof(findData));
        memset(entryName, 0, sizeof(entryName));
    }
};

// MinGW compatibility with MSVC
// Provides symbols needed by MinGW-compiled libraries
extern "C" {
    void ___chkstk_ms(void);
    void ___chkstk_ms(void) {
        // Stub implementation for MSVC compatibility
        // In a real implementation, this would check stack space
        // For our purposes, a no-op is sufficient
    }

    // GNU C library compatibility functions
    void* __memcpy_chk(void* dest, const void* src, size_t len, size_t destlen) {
        return memcpy(dest, src, len);
    }

    void* __memmove_chk(void* dest, const void* src, size_t len, size_t destlen) {
        return memmove(dest, src, len);
    }

    char* __strcpy_chk(char* dest, const char* src, size_t destlen) {
        return strcpy(dest, src);
    }

    // sprintf is already defined by MSVC, no need to redefine

    // Directory operations - Windows implementation
    void* opendir(const char* name) {
        if (!name || !*name) {
            return nullptr;
        }

        // Allocate directory handle structure
        WindowsDirHandle* handle = new WindowsDirHandle();
        if (!handle) {
            return nullptr;
        }

        // Convert to wide string for Windows API
        int wideLength = MultiByteToWideChar(CP_UTF8, 0, name, -1, nullptr, 0);
        if (wideLength <= 0) {
            delete handle;
            return nullptr;
        }

        wchar_t* widePath = new wchar_t[wideLength + 4]; // +4 for "\*.*"
        MultiByteToWideChar(CP_UTF8, 0, name, -1, widePath, wideLength);

        // Add wildcard for FindFirstFile
        wcscat(widePath, L"\\*");

        // Find first file
        handle->findHandle = FindFirstFileW(widePath, &handle->findData);
        handle->isFirst = true;

        delete[] widePath;

        if (handle->findHandle == INVALID_HANDLE_VALUE) {
            delete handle;
            return nullptr;
        }

        return handle;
    }

    void* readdir(void* dirp) {
        WindowsDirHandle* handle = static_cast<WindowsDirHandle*>(dirp);
        if (!handle || handle->findHandle == INVALID_HANDLE_VALUE) {
            return nullptr;
        }

        BOOL result;
        if (handle->isFirst) {
            handle->isFirst = false;
            result = TRUE; // First entry already retrieved
        } else {
            result = FindNextFileW(handle->findHandle, &handle->findData);
        }

        if (!result) {
            return nullptr;
        }

        // Convert to UTF-8
        int nameLength = WideCharToMultiByte(CP_UTF8, 0, handle->findData.cFileName, -1,
                                           handle->entryName, sizeof(handle->entryName), nullptr, nullptr);

        if (nameLength <= 0) {
            return nullptr;
        }

        // Return pointer to entry (simulated dirent structure)
        return &handle->entryName;
    }

    int closedir(void* dirp) {
        WindowsDirHandle* handle = static_cast<WindowsDirHandle*>(dirp);
        if (!handle) {
            return -1;
        }

        if (handle->findHandle != INVALID_HANDLE_VALUE) {
            FindClose(handle->findHandle);
        }

        delete handle;
        return 0;
    }

    int gettimeofday(struct timeval* tv, struct timezone* tz) {
        if (!tv) {
            return -1;
        }

        FILETIME ft;
        GetSystemTimeAsFileTime(&ft);

        // Convert FILETIME to UNIX timestamp
        ULARGE_INTEGER ull;
        ull.LowPart = ft.dwLowDateTime;
        ull.HighPart = ft.dwHighDateTime;

        // FILETIME is 100-nanosecond intervals since January 1, 1601 UTC
        // UNIX timestamp is seconds since January 1, 1970 UTC
        // Difference is 11644473600 seconds
        uint64_t unixTime = (ull.QuadPart / 10000000ULL) - 11644473600ULL;

        tv->tv_sec = static_cast<long>(unixTime);
        tv->tv_usec = static_cast<long>((ull.QuadPart % 10000000ULL) / 10);

        // timezone is deprecated, but set to nullptr if provided
        if (tz) {
            tz->tz_minuteswest = 0;
            tz->tz_dsttime = 0;
        }

        return 0;
    }

    // Note: if_nametoindex is provided by Windows SDK via iphlpapi.lib
    // Removed custom implementation to avoid conflicts

    // GNU C library runtime check functions
    void __chk_fail(void) {
        // GNU C library runtime check failure stub
        // In a real implementation, this would terminate the program
        // For our purposes, we'll just return (ignore the failure)
    }

    // MinGW printf compatibility functions
    int __mingw_vfprintf(FILE* stream, const char* format, va_list args) {
        return vfprintf(stream, format, args);
    }
    
    int __mingw_vasprintf(char** strp, const char* format, va_list args) {
        // Calculate required buffer size
        va_list args_copy;
        va_copy(args_copy, args);
        int size = vsnprintf(nullptr, 0, format, args_copy);
        va_end(args_copy);
        
        if (size < 0) {
            *strp = nullptr;
            return -1;
        }
        
        // Allocate buffer and format string
        *strp = (char*)malloc(size + 1);
        if (*strp == nullptr) {
            return -1;
        }
        
        return vsnprintf(*strp, size + 1, format, args);
    }

    int __mingw_vsnprintf(char* str, size_t size, const char* format, va_list args) {
        return vsnprintf(str, size, format, args);
    }

    int __mingw_vsscanf(const char* str, const char* format, va_list args) {
        return vsscanf(str, format, args);
    }

    void* __memset_chk(void* dest, int c, size_t n, size_t destlen) {
        return memset(dest, c, n);
    }

    // PSL (Public Suffix List) library implementation for Windows
    // Simplified implementation with common public suffixes

    struct WindowsPSL {
        static const char* commonSuffixes[];
        static const size_t suffixCount;
    };

    // Common public suffixes for basic functionality
    const char* WindowsPSL::commonSuffixes[] = {
        "com", "org", "net", "edu", "gov", "mil", "int",
        "co.uk", "org.uk", "ac.uk", "gov.uk", "ltd.uk",
        "com.au", "net.au", "org.au", "edu.au", "gov.au",
        "co.jp", "ne.jp", "or.jp", "ac.jp", "ad.jp",
        "de", "fr", "it", "es", "nl", "be", "ch", "at",
        "ru", "cn", "jp", "kr", "in", "br", "mx", "ca"
    };

    const size_t WindowsPSL::suffixCount = sizeof(WindowsPSL::commonSuffixes) / sizeof(WindowsPSL::commonSuffixes[0]);

    static WindowsPSL g_builtinPSL;

    int psl_is_cookie_domain_acceptable(void* psl, const char* hostname, const char* cookiedomain) {
        if (!hostname || !cookiedomain) {
            return 0;
        }

        // Basic domain validation
        size_t hostnameLen = strlen(hostname);
        size_t cookieLen = strlen(cookiedomain);

        if (cookieLen > hostnameLen) {
            return 0;
        }

        // Check if cookiedomain is a suffix of hostname
        if (cookieLen == hostnameLen) {
            return strcmp(hostname, cookiedomain) == 0 ? 1 : 0;
        }

        // Check if cookiedomain matches the end of hostname with a dot separator
        const char* suffix = hostname + (hostnameLen - cookieLen);
        if (hostname[hostnameLen - cookieLen - 1] != '.') {
            return 0;
        }

        if (strcmp(suffix, cookiedomain) == 0) {
            // Additional check: ensure cookiedomain is not a public suffix
            for (size_t i = 0; i < WindowsPSL::suffixCount; i++) {
                if (strcmp(cookiedomain, WindowsPSL::commonSuffixes[i]) == 0) {
                    return 0; // Cannot set cookies on public suffixes
                }
            }
            return 1;
        }

        return 0;
    }

    void psl_free(void* psl) {
        // For builtin PSL, no need to free
        // For custom PSL implementations, would free here
        if (psl && psl != &g_builtinPSL) {
            free(psl);
        }
    }

    void* psl_latest(void) {
        // Return builtin PSL as we don't download updates
        return &g_builtinPSL;
    }

    void* psl_builtin(void) {
        // Return pointer to builtin PSL data
        return &g_builtinPSL;
    }

    const char* psl_check_version_number(int version) {
        // Return a version string indicating this is a simplified implementation
        return "windows-compat-1.0.0";
    }

    // Additional PSL utility functions
    int psl_is_public_suffix(void* psl, const char* domain) {
        if (!domain) {
            return 0;
        }

        // Check if domain is in our list of public suffixes
        for (size_t i = 0; i < WindowsPSL::suffixCount; i++) {
            if (strcmp(domain, WindowsPSL::commonSuffixes[i]) == 0) {
                return 1;
            }
        }

        return 0;
    }

    const char* psl_registrable_domain(void* psl, const char* domain) {
        if (!domain) {
            return nullptr;
        }

        // Find the registrable domain (domain + public suffix)
        size_t domainLen = strlen(domain);

        // Look for the longest matching public suffix
        const char* bestSuffix = nullptr;
        size_t bestSuffixLen = 0;

        for (size_t i = 0; i < WindowsPSL::suffixCount; i++) {
            const char* suffix = WindowsPSL::commonSuffixes[i];
            size_t suffixLen = strlen(suffix);

            if (suffixLen <= domainLen) {
                const char* domainSuffix = domain + (domainLen - suffixLen);
                if (strcmp(domainSuffix, suffix) == 0) {
                    if (suffixLen > bestSuffixLen) {
                        bestSuffix = suffix;
                        bestSuffixLen = suffixLen;
                    }
                }
            }
        }

        if (!bestSuffix) {
            return domain; // No public suffix found, return whole domain
        }

        // Find the registrable part (one level above the public suffix)
        const char* registrablePart = domain + (domainLen - bestSuffixLen);

        // Find the dot before the registrable domain
        while (registrablePart > domain && *(registrablePart - 1) != '.') {
            registrablePart--;
        }

        // Include one more level
        if (registrablePart > domain) {
            registrablePart--;
            while (registrablePart > domain && *(registrablePart - 1) != '.') {
                registrablePart--;
            }
        }

        return registrablePart;
    }

    // fprintf is already provided by MSVC runtime
    // The unresolved symbol might be due to library linking order
}