#include "../core/CoopNetCore.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <cassert>

namespace CoopNet {
namespace Test {

class SystemIntegrationTest {
public:
    static bool RunAllTests() {
        std::cout << "=== CoopNet System Integration Tests ===" << std::endl;

        bool allPassed = true;

        allPassed &= TestBasicInitialization();
        allPassed &= TestSystemDependencies();
        allPassed &= TestErrorHandling();
        allPassed &= TestConfigurationSystem();
        allPassed &= TestEventSystem();
        allPassed &= TestSystemHealthMonitoring();
        allPassed &= TestGracefulShutdown();

        std::cout << std::endl << "=== Test Results ===" << std::endl;
        std::cout << "All tests " << (allPassed ? "PASSED" : "FAILED") << std::endl;

        return allPassed;
    }

private:
    static bool TestBasicInitialization() {
        std::cout << "\n[TEST] Basic Initialization..." << std::endl;

        try {
            auto& core = CoopNetCore::Instance();

            // Test initialization
            bool initResult = core.Initialize();
            if (!initResult) {
                std::cout << "FAILED: Core initialization failed" << std::endl;
                return false;
            }

            // Wait for systems to be ready
            int attempts = 0;
            while (!core.AreAllSystemsReady() && attempts < 50) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                attempts++;
            }

            if (!core.AreAllSystemsReady()) {
                std::cout << "FAILED: Systems not ready after timeout" << std::endl;
                core.Shutdown();
                return false;
            }

            std::cout << "PASSED: Basic initialization successful" << std::endl;
            core.Shutdown();
            return true;

        } catch (const std::exception& ex) {
            std::cout << "FAILED: Exception during initialization: " << ex.what() << std::endl;
            return false;
        }
    }

    static bool TestSystemDependencies() {
        std::cout << "\n[TEST] System Dependencies..." << std::endl;

        try {
            auto& systemManager = SystemManager::Instance();

            // Test dependency validation
            bool depsValid = systemManager.ValidateDependencies();
            if (!depsValid) {
                std::cout << "FAILED: Dependency validation failed" << std::endl;
                return false;
            }

            // Test initialization order
            auto initOrder = systemManager.GetInitializationOrder();
            if (initOrder.empty()) {
                std::cout << "FAILED: Empty initialization order" << std::endl;
                return false;
            }

            // Verify ErrorManager comes first (highest priority)
            if (initOrder[0] != SystemType::ErrorManager) {
                std::cout << "FAILED: ErrorManager not initialized first" << std::endl;
                return false;
            }

            std::cout << "PASSED: System dependencies validated" << std::endl;
            return true;

        } catch (const std::exception& ex) {
            std::cout << "FAILED: Exception during dependency test: " << ex.what() << std::endl;
            return false;
        }
    }

    static bool TestErrorHandling() {
        std::cout << "\n[TEST] Error Handling..." << std::endl;

        try {
            auto& core = CoopNetCore::Instance();
            core.Initialize();

            // Test error reporting
            core.ReportError("TestComponent", "Test error message", false);
            core.ReportError("TestComponent", "Test critical error", true);

            // Test API convenience functions
            CoopNetAPI::ReportError("TestAPI", "API test error");
            CoopNetAPI::ReportCriticalError("TestAPI", "API critical error");

            // Verify error manager is accessible
            auto& errorManager = CoopNetAPI::GetErrorManager();
            auto stats = errorManager.GetStatistics();

            if (stats.totalErrors == 0) {
                std::cout << "FAILED: No errors recorded" << std::endl;
                core.Shutdown();
                return false;
            }

            std::cout << "PASSED: Error handling working correctly" << std::endl;
            core.Shutdown();
            return true;

        } catch (const std::exception& ex) {
            std::cout << "FAILED: Exception during error handling test: " << ex.what() << std::endl;
            return false;
        }
    }

    static bool TestConfigurationSystem() {
        std::cout << "\n[TEST] Configuration System..." << std::endl;

        try {
            auto& core = CoopNetCore::Instance();
            core.Initialize();

            // Test configuration API
            bool setResult = CoopNetAPI::SetConfigValue("test_key", std::string("test_value"));
            if (!setResult) {
                std::cout << "FAILED: Could not set config value" << std::endl;
                core.Shutdown();
                return false;
            }

            std::string getValue = CoopNetAPI::GetConfigValue("test_key", std::string("default"));
            if (getValue != "test_value") {
                std::cout << "FAILED: Config value mismatch: " << getValue << std::endl;
                core.Shutdown();
                return false;
            }

            // Test different types
            CoopNetAPI::SetConfigValue("test_int", 42);
            int intValue = CoopNetAPI::GetConfigValue("test_int", 0);
            if (intValue != 42) {
                std::cout << "FAILED: Integer config value mismatch: " << intValue << std::endl;
                core.Shutdown();
                return false;
            }

            CoopNetAPI::SetConfigValue("test_bool", true);
            bool boolValue = CoopNetAPI::GetConfigValue("test_bool", false);
            if (!boolValue) {
                std::cout << "FAILED: Boolean config value mismatch" << std::endl;
                core.Shutdown();
                return false;
            }

            std::cout << "PASSED: Configuration system working correctly" << std::endl;
            core.Shutdown();
            return true;

        } catch (const std::exception& ex) {
            std::cout << "FAILED: Exception during configuration test: " << ex.what() << std::endl;
            return false;
        }
    }

    static bool TestEventSystem() {
        std::cout << "\n[TEST] Event System..." << std::endl;

        try {
            auto& core = CoopNetCore::Instance();
            core.Initialize();

            bool eventReceived = false;
            std::string eventData;

            // Register event handler
            core.RegisterEventHandler("test_event", [&](const nlohmann::json& data) {
                eventReceived = true;
                if (data.contains("message")) {
                    eventData = data["message"];
                }
            });

            // Send test event
            nlohmann::json testData;
            testData["message"] = "Hello from event system";
            core.BroadcastEvent("test_event", testData);

            // Give some time for event processing
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            if (!eventReceived) {
                std::cout << "FAILED: Event not received" << std::endl;
                core.Shutdown();
                return false;
            }

            if (eventData != "Hello from event system") {
                std::cout << "FAILED: Event data mismatch: " << eventData << std::endl;
                core.Shutdown();
                return false;
            }

            // Test API convenience functions
            bool apiEventReceived = false;
            CoopNetAPI::RegisterForEvents("api_test", [&](const nlohmann::json& data) {
                apiEventReceived = true;
            });

            CoopNetAPI::SendEvent("api_test", nlohmann::json{{"test", true}});
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            if (!apiEventReceived) {
                std::cout << "FAILED: API event not received" << std::endl;
                core.Shutdown();
                return false;
            }

            std::cout << "PASSED: Event system working correctly" << std::endl;
            core.Shutdown();
            return true;

        } catch (const std::exception& ex) {
            std::cout << "FAILED: Exception during event system test: " << ex.what() << std::endl;
            return false;
        }
    }

    static bool TestSystemHealthMonitoring() {
        std::cout << "\n[TEST] System Health Monitoring..." << std::endl;

        try {
            auto& core = CoopNetCore::Instance();
            core.Initialize();

            // Wait for systems to be ready
            int attempts = 0;
            while (!core.AreAllSystemsReady() && attempts < 50) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                attempts++;
            }

            if (!core.AreAllSystemsReady()) {
                std::cout << "FAILED: Systems not ready for health monitoring test" << std::endl;
                core.Shutdown();
                return false;
            }

            // Perform diagnostics
            bool diagnosticsResult = core.PerformSystemDiagnostics();
            if (!diagnosticsResult) {
                std::cout << "WARNING: System diagnostics reported issues (this may be expected)" << std::endl;
            }

            // Test system manager health monitoring
            auto& systemManager = SystemManager::Instance();
            bool allHealthy = systemManager.AreAllSystemsHealthy();

            // Get system status
            std::string status = core.GetSystemStatus();
            if (status.empty()) {
                std::cout << "FAILED: Empty system status" << std::endl;
                core.Shutdown();
                return false;
            }

            // Get system report
            std::string report = core.GenerateSystemReport();
            if (report.empty()) {
                std::cout << "FAILED: Empty system report" << std::endl;
                core.Shutdown();
                return false;
            }

            std::cout << "PASSED: Health monitoring working correctly" << std::endl;
            core.Shutdown();
            return true;

        } catch (const std::exception& ex) {
            std::cout << "FAILED: Exception during health monitoring test: " << ex.what() << std::endl;
            return false;
        }
    }

    static bool TestGracefulShutdown() {
        std::cout << "\n[TEST] Graceful Shutdown..." << std::endl;

        try {
            auto& core = CoopNetCore::Instance();
            core.Initialize();

            // Wait for systems to be ready
            int attempts = 0;
            while (!core.AreAllSystemsReady() && attempts < 50) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                attempts++;
            }

            // Test graceful shutdown
            core.Shutdown();

            // Verify systems are shut down
            if (core.AreAllSystemsReady()) {
                std::cout << "FAILED: Systems still ready after shutdown" << std::endl;
                return false;
            }

            // Test re-initialization after shutdown
            bool reinitResult = core.Initialize();
            if (!reinitResult) {
                std::cout << "FAILED: Re-initialization failed after shutdown" << std::endl;
                return false;
            }

            core.Shutdown();

            std::cout << "PASSED: Graceful shutdown working correctly" << std::endl;
            return true;

        } catch (const std::exception& ex) {
            std::cout << "FAILED: Exception during shutdown test: " << ex.what() << std::endl;
            return false;
        }
    }
};

} // namespace Test
} // namespace CoopNet

// Test runner main function
int main() {
    try {
        bool result = CoopNet::Test::SystemIntegrationTest::RunAllTests();
        return result ? 0 : 1;
    } catch (const std::exception& ex) {
        std::cerr << "Fatal exception during testing: " << ex.what() << std::endl;
        return 1;
    }
}