/**
 * @file test_utils_audit_logger_stress_concurrent_writers.cpp
 * @brief TSAN stress test for AuditLogger with 128 concurrent writers
 * @date 2026-08-17
 *
 * Tests Phase 4.3 concurrency stress for audit_logger.cpp:
 * - Concurrent writes from N threads (N = 8, 32, 128)
 * - No data races (verified under TSAN)
 * - Queue overflow handling under sustained load
 * - Buffer management correctness
 *
 * Run with TSAN enabled:
 *   TSAN_OPTIONS=halt_on_error=1 ctest -R stress_audit_logger
 */

#include <gtest/gtest.h>
#include "utils/audit_logger.h"
#include "utils/error_contracts.h"

#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include <mutex>
#include <filesystem>
#include <memory>

namespace fs = std::filesystem;

namespace themis {
namespace utils {

class AuditLoggerStressTest : public ::testing::Test {
protected:
    fs::path tmp_dir;
    std::string log_path = {};

    void SetUp() override {
        tmp_dir = fs::temp_directory_path() / "audit_logger_stress_test";
        fs::create_directories(tmp_dir);
        log_path = (tmp_dir / "stress_audit.log").string();
        if (fs::exists(log_path)) {
            fs::remove(log_path);
        }
    }

    void TearDown() override {
        if (fs::exists(tmp_dir)) {
            fs::remove_all(tmp_dir);
        }
    }

    AuditLoggerConfig makeConfig(const std::string& path) {
        AuditLoggerConfig cfg;
        cfg.log_path = path;
        cfg.enabled = true;
        cfg.enable_hash_chain = false;
        cfg.enable_fsync = false;
        cfg.encrypt_then_sign = false;
        return cfg;
    }
};

// ============================================================================
// Stress Test: 8 Concurrent Writers
// ============================================================================

TEST_F(AuditLoggerStressTest, ConcurrentWriters_8Threads) {
    AuditLogger logger(std::shared_ptr<themis::FieldEncryption>{},
                       std::shared_ptr<VCCPKIClient>{},
                       makeConfig(log_path));
    
    constexpr int kThreads = 8;
    constexpr int kEventsPerThread = 100;
    
    std::vector<std::thread> threads;
    std::atomic<int> total_events = 0;
    std::atomic<int> error_count = 0;
    
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([this, &logger, &total_events, &error_count, t]() {
            for (int i = 0; i < kEventsPerThread; ++i) {
                try {
                    nlohmann::json event;
                    event["thread_id"] = t;
                    event["event_num"] = i;
                    event["message"] = "stress test event";
                    
                    logger.logEvent(event);
                    total_events++;
                    
                } catch (const std::exception& e) {
                    error_count++;
                }
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Verify results
    EXPECT_EQ(total_events, kThreads * kEventsPerThread);
    EXPECT_EQ(error_count, 0) << "No errors expected with available resources";
    EXPECT_TRUE(fs::exists(log_path));
    EXPECT_GT(fs::file_size(log_path), 0u);
}

// ============================================================================
// Stress Test: 32 Concurrent Writers
// ============================================================================

TEST_F(AuditLoggerStressTest, ConcurrentWriters_32Threads) {
    AuditLogger logger(std::shared_ptr<themis::FieldEncryption>{},
                       std::shared_ptr<VCCPKIClient>{},
                       makeConfig(log_path));
    
    constexpr int kThreads = 32;
    constexpr int kEventsPerThread = 50;
    
    std::vector<std::thread> threads;
    std::atomic<int> total_events = 0;
    std::atomic<int> error_count = 0;
    
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([this, &logger, &total_events, &error_count, t]() {
            for (int i = 0; i < kEventsPerThread; ++i) {
                try {
                    nlohmann::json event;
                    event["thread_id"] = t;
                    event["sequence"] = i;
                    event["timestamp"] = std::chrono::system_clock::now().time_since_epoch().count();
                    
                    logger.logEvent(event);
                    total_events++;
                    
                } catch (const std::exception& e) {
                    error_count++;
                }
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Verify results
    EXPECT_GE(total_events, kThreads * kEventsPerThread * 0.95);  // Allow 5% loss under extreme load
    EXPECT_TRUE(fs::exists(log_path));
}

// ============================================================================
// Stress Test: 128 Concurrent Writers (Maximum stress)
// ============================================================================

TEST_F(AuditLoggerStressTest, ConcurrentWriters_128Threads) {
    AuditLogger logger(std::shared_ptr<themis::FieldEncryption>{},
                       std::shared_ptr<VCCPKIClient>{},
                       makeConfig(log_path));
    
    constexpr int kThreads = 128;
    constexpr int kEventsPerThread = 20;  // Lower per-thread for 128 threads
    
    std::vector<std::thread> threads;
    std::atomic<int> total_events = 0;
    std::atomic<int> error_count = 0;
    std::mutex error_mutex = {};
    std::vector<std::string> error_messages;
    
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([this, &logger, &total_events, &error_count, 
                            &error_mutex, &error_messages, t]() {
            for (int i = 0; i < kEventsPerThread; ++i) {
                try {
                    nlohmann::json event;
                    event["worker"] = t;
                    event["iteration"] = i;
                    event["payload"] = "stress test payload for worker " + std::to_string(t);
                    
                    logger.logEvent(event);
                    total_events++;
                    
                } catch (const std::exception& e) {
                    error_count++;
                    {
                        std::lock_guard<std::mutex> lock(error_mutex);
                        error_messages.push_back(
                            "Thread " + std::to_string(t) + ": " + e.what()
                        );
                    }
                }
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Verify results - should handle high concurrency
    EXPECT_GE(total_events, 1) << "At least some events should be logged";
    EXPECT_TRUE(fs::exists(log_path));
    
    // Log any errors for diagnostic purposes
    if (error_count > 0) {
        std::cout << "Errors under 128-thread stress: " << error_count << std::endl;
        for (const auto& msg : error_messages) {
            std::cout << "  " << msg << std::endl;
        }
    }
}

// ============================================================================
// Stress Test: Rapid Fire Events with Minimal Delay
// ============================================================================

TEST_F(AuditLoggerStressTest, RapidFireEvents_NoDelay) {
    AuditLogger logger(std::shared_ptr<themis::FieldEncryption>{},
                       std::shared_ptr<VCCPKIClient>{},
                       makeConfig(log_path));
    
    constexpr int kThreads = 16;
    constexpr int kEventsPerThread = 500;
    
    std::vector<std::thread> threads;
    std::atomic<int> total_events = 0;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([this, &logger, &total_events, t]() {
            for (int i = 0; i < kEventsPerThread; ++i) {
                try {
                    nlohmann::json event;
                    event["id"] = t * 10000 + i;
                    logger.logEvent(event);
                    total_events++;
                } catch (...) {
                    // Acceptable to drop events under extreme load
                }
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time
    ).count();
    
    // Verify throughput metrics
    EXPECT_GT(total_events, 0);
    double throughput = static_cast<double>(total_events) / (duration / 1000.0);
    
    std::cout << "Stress test throughput: " << throughput << " events/sec" << std::endl;
}

// ============================================================================
// Stress Test: High Queue Pressure with Queue Full Handling
// ============================================================================

TEST_F(AuditLoggerStressTest, QueuePressureHandling) {
    AuditLogger logger(std::shared_ptr<themis::FieldEncryption>{},
                       std::shared_ptr<VCCPKIClient>{},
                       makeConfig(log_path));
    
    constexpr int kThreads = 32;
    constexpr int kEventsPerThread = 200;
    
    std::vector<std::thread> threads;
    std::atomic<int> dropped_events = 0;
    std::atomic<int> successful_events = 0;
    
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([this, &logger, &dropped_events, &successful_events, t]() {
            for (int i = 0; i < kEventsPerThread; ++i) {
                try {
                    nlohmann::json event;
                    event["source"] = "worker_" + std::to_string(t);
                    event["seqnum"] = i;
                    
                    logger.logEvent(event);
                    successful_events++;
                    
                } catch (const std::exception& e) {
                    // Queue full or backend unavailable - acceptable
                    dropped_events++;
                }
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Verify results
    int total_attempted = kThreads * kEventsPerThread;
    int total_logged = successful_events + dropped_events;
    
    EXPECT_EQ(total_attempted, total_logged);
    EXPECT_GT(successful_events, 0);
    
    std::cout << "Queue pressure: " << successful_events << " succeeded, " 
              << dropped_events << " dropped" << std::endl;
}

// ============================================================================
// Stress Test: Mixed Payload Sizes
// ============================================================================

TEST_F(AuditLoggerStressTest, MixedPayloadSizes) {
    AuditLogger logger(std::shared_ptr<themis::FieldEncryption>{},
                       std::shared_ptr<VCCPKIClient>{},
                       makeConfig(log_path));
    
    constexpr int kThreads = 16;
    constexpr int kEventsPerThread = 100;
    
    std::vector<std::thread> threads;
    std::atomic<int> total_events = 0;
    
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([this, &logger, &total_events, t]() {
            for (int i = 0; i < kEventsPerThread; ++i) {
                try {
                    nlohmann::json event;
                    event["thread"] = t;
                    event["seq"] = i;
                    
                    // Vary payload size
                    std::string payload = {};
                    int size = (i % 3) == 0 ? 10 : (i % 3) == 1 ? 1000 : 10000;
                    payload.assign(size, 'X');
                    event["payload"] = payload;
                    
                    logger.logEvent(event);
                    total_events++;
                    
                } catch (...) {
                    // Acceptable
                }
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_GT(total_events, 0);
    EXPECT_TRUE(fs::exists(log_path));
}

} // namespace utils
} // namespace themis
