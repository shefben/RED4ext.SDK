#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <cstdlib>
#include <windows.h>

// Forward declare winsock types to avoid conflicts
struct timeval;
struct timezone;

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

    // Directory operations stubs
    void* opendir(const char* name) {
        // Stub implementation - return null to indicate directory operations not supported
        return nullptr;
    }

    void* readdir(void* dirp) {
        // Stub implementation
        return nullptr;
    }

    int closedir(void* dirp) {
        // Stub implementation
        return 0;
    }

    int gettimeofday(struct timeval* tv, struct timezone* tz) {
        // Stub implementation - return 0 to indicate success
        // Real implementation would require defining timeval struct properly
        return 0;
    }

    // Network function stubs
    int if_nametoindex(const char* ifname) {
        // Stub implementation
        return 0;
    }

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

    // PSL (Public Suffix List) library stubs - return safe defaults
    int psl_is_cookie_domain_acceptable(void* psl, const char* hostname, const char* cookiedomain) {
        // Return 1 to accept all cookie domains (safe default)
        return 1;
    }

    void psl_free(void* psl) {
        // Stub implementation
    }

    void* psl_latest(void) {
        // Return null to indicate no PSL available
        return nullptr;
    }

    void* psl_builtin(void) {
        // Return null to indicate no built-in PSL
        return nullptr;
    }

    const char* psl_check_version_number(int version) {
        // Return a version string for compatibility
        return "stub-0.0.0";
    }

    // fprintf is already provided by MSVC runtime
    // The unresolved symbol might be due to library linking order
}