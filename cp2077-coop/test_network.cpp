// Simple test runner for network functionality validation
// This can be compiled and run independently to test the networking system

#include <iostream>
#include <cassert>

// Forward declaration of the test function
extern "C" int RunNetworkTests();

int main() {
    std::cout << "CP2077-Coop Network System Test Runner" << std::endl;
    std::cout << "=======================================" << std::endl;

    try {
        int result = RunNetworkTests();

        if (result == 0) {
            std::cout << "\n🎉 All tests completed successfully!" << std::endl;
            std::cout << "The network and player synchronization systems are working correctly." << std::endl;
        } else {
            std::cout << "\n💥 Tests failed!" << std::endl;
            std::cout << "There are issues with the network implementation that need to be addressed." << std::endl;
        }

        return result;
    } catch (const std::exception& e) {
        std::cout << "\n💥 Test execution failed with exception: " << e.what() << std::endl;
        return 1;
    }
}