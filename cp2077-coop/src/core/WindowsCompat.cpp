#include <cstdint>

// MinGW compatibility with MSVC
// Provides ___chkstk_ms symbol needed by MinGW-compiled libraries
extern "C" {
    void ___chkstk_ms(void);
    void ___chkstk_ms(void) {
        // Stub implementation for MSVC compatibility
        // In a real implementation, this would check stack space
        // For our purposes, a no-op is sufficient
    }
}