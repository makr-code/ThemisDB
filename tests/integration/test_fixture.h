/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_fixture.h                                     ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 07:09:09                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     111                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_fixture.h
 * @brief Base fixture for integration tests
 * 
 * Provides common setup, teardown, and helper methods for integration testing.
 * All integration tests should inherit from IntegrationTestFixture for consistency.
 */

#pragma once

#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <thread>
#include <filesystem>

namespace themis {
namespace test {

/**
 * @brief Base fixture for all integration tests
 * 
 * Provides:
 * - Automatic test database setup/cleanup
 * - Helper methods for common test operations
 * - Timeout and async operation support
 * - Consistent test data generation
 */
class IntegrationTestFixture : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary directory for test data
        temp_dir_ = std::filesystem::temp_directory_path() / 
                    ("themis_test_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));
        std::filesystem::create_directories(temp_dir_);
    }
    
    void TearDown() override {
        // Clean up temporary directory
        if (std::filesystem::exists(temp_dir_)) {
            std::filesystem::remove_all(temp_dir_);
        }
    }
    
    /**
     * @brief Wait for a condition to become true with timeout
     * @param condition Function that returns true when condition is met
     * @param timeout Maximum time to wait
     * @param check_interval How often to check the condition
     * @return true if condition was met, false if timeout
     */
    bool WaitForCondition(
        const std::function<bool()>& condition,
        std::chrono::seconds timeout = std::chrono::seconds(10),
        std::chrono::milliseconds check_interval = std::chrono::milliseconds(100)
    ) {
        auto start = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - start < timeout) {
            if (condition()) {
                return true;
            }
            std::this_thread::sleep_for(check_interval);
        }
        return false;
    }
    
    /**
     * @brief Get the temporary directory path for this test
     */
    const std::filesystem::path& GetTempDir() const {
        return temp_dir_;
    }
    
    /**
     * @brief Create a unique test database path
     */
    std::filesystem::path CreateTestDbPath(const std::string& name = "test_db") {
        return temp_dir_ / name;
    }

protected:
    std::filesystem::path temp_dir_;
};

} // namespace test
} // namespace themis
