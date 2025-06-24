#include <iostream>

// Captures crash information and copies recent network log.
void CaptureCrash(const char* reason)
{
    std::cout << "Crash captured: " << reason << std::endl;
    // Would copy last 1 MB of the network log to crash/netlog_last.txt
}
