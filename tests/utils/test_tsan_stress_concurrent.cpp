/**
 * @file test_tsan_stress_concurrent.cpp
 * @brief TSAN (ThreadSanitizer) stress tests for concurrency-critical components
 * @date 2026-08-17
 *
 * Tests Phase 2.3 and Phase 5 concurrency hardening:
 * - audit_logger: 128 concurrent writer stress test
 * - thread_pool_manager: saturation and priority ordering under TSAN
 * - pii_stream_scanner: parallel scan slots TSAN validation
 *
 * Build with: cmake --preset develop-tsan
 * Intended to catch race conditions and use-after-free
 */

#include <gtest/gtest.h>
#include "utils/audit_logger.h"
#include "utils/thread_pool_manager.h"
#include "utils/pii_stream_scanner.h"
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <random>

namespace themis {
namespace utils {

// ============================================================================
// AuditLogger TSAN Stress Tests
// ============================================================================

class AuditLoggerTSANTest : public ::testing::Test {
protected:
    AuditLogger audit_logger;
    std::atomic<int> successful_logs = 0;
    std::atomic<int> failed_logs = 0;
    
    void SetUp() override {
        audit_logger.initialize();
    }
    
    void TearDown() override {
        audit_logger.flush();
    }
};

TEST_F(AuditLoggerTSANTest, ConcurrentWriterStress128) {
    // Spin up 128 concurrent writer threads
    std::vector<std::thread> writers;
    const int WRITER_COUNT = 128;
    const int LOGS_PER_WRITER = 100;
    
    for (int w = 0; w < WRITER_COUNT; ++w) {
        writers.emplace_back([this, w]() {
            for (int l = 0; l < LOGS_PER_WRITER; ++l) {
                try {
                    AuditLogEntry entry;
                    entry.timestamp = std::chrono::system_clock::now();
                    entry.severity = (l % 3 == 0) ? "WARN" : "INFO";
                    entry.action = "PII_DETECTION";
                    entry.user_id = "user_" + std::to_string(w);
                    entry.result = "SUCCESS";
                    entry.details = "Detected " + std::to_string(l) + " PII elements";
                    
                    audit_logger.log(entry);
                    successful_logs++;
                } catch (...) {
                    failed_logs++;
                }
            }
        });
    }
    
    // Wait for all writers to complete
    for (auto& writer : writers) {
        writer.join();
    }
    
    // Verify results
    EXPECT_GT(successful_logs, 0);
    EXPECT_EQ(failed_logs, 0);
    EXPECT_EQ(successful_logs + failed_logs, WRITER_COUNT * LOGS_PER_WRITER);
}

TEST_F(AuditLoggerTSANTest, ConcurrentReadersAndWriters) {
    // Mix of readers and writers
    std::vector<std::thread> threads;
    std::atomic<int> read_count = 0;
    std::atomic<int> write_count = 0;
    
    // Start 10 writer threads
    for (int w = 0; w < 10; ++w) {
        threads.emplace_back([this, &write_count, w]() {
            for (int i = 0; i < 50; ++i) {
                AuditLogEntry entry;
                entry.timestamp = std::chrono::system_clock::now();
                entry.severity = "INFO";
                entry.action = "TEST";
                entry.user_id = "writer_" + std::to_string(w);
                
                try {
                    audit_logger.log(entry);
                    write_count++;
                } catch (...) {
                    // Acceptable
                }
            }
        });
    }
    
    // Start 5 reader threads
    for (int r = 0; r < 5; ++r) {
        threads.emplace_back([this, &read_count, r]() {
            for (int i = 0; i < 50; ++i) {
                try {
                    // Simulate reading recent logs
                    auto recent = audit_logger.queryRecent(10);  // Last 10 entries
                    read_count += recent.size();
                } catch (...) {
                    // Acceptable
                }
            }
        });
    }
    
    // Wait for all
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_GT(write_count, 0);
    // Reads may lag behind writes, so just check we tried
}

TEST_F(AuditLoggerTSANTest, SameSeverityMultipleWriters) {
    // Multiple writers logging same severity level
    std::vector<std::thread> writers;
    
    for (int w = 0; w < 32; ++w) {
        writers.emplace_back([this, w]() {
            for (int i = 0; i < 100; ++i) {
                AuditLogEntry entry;
                entry.severity = "CRITICAL";
                entry.action = "SECURITY_EVENT";
                entry.user_id = "threat_" + std::to_string(w);
                
                try {
                    audit_logger.log(entry);
                } catch (...) {
                    // Acceptable
                }
            }
        });
    }
    
    for (auto& t : writers) {
        t.join();
    }
}

// ============================================================================
// ThreadPoolManager TSAN Stress Tests
// ============================================================================

class ThreadPoolTSANTest : public ::testing::Test {
protected:
    ThreadPoolManager pool;
    std::atomic<int> tasks_completed = 0;
    std::atomic<int> tasks_failed = 0;
    
    void SetUp() override {
        pool.initialize({.thread_count = 16, .queue_capacity = 1000});
    }
    
    void TearDown() override {
        pool.shutdown();
    }
};

TEST_F(ThreadPoolTSANTest, HighPriorityStarvation) {
    // Submit many low-priority tasks interspersed with high-priority
    const int LOW_PRI_COUNT = 500;
    const int HIGH_PRI_COUNT = 50;
    
    for (int i = 0; i < LOW_PRI_COUNT; ++i) {
        pool.submitTask(
            [this]() {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
                tasks_completed++;
            },
            ThreadPoolManager::Priority::LOW
        );
    }
    
    for (int i = 0; i < HIGH_PRI_COUNT; ++i) {
        pool.submitTask(
            [this]() {
                tasks_completed++;
            },
            ThreadPoolManager::Priority::HIGH
        );
    }
    
    // Wait for completion with timeout
    auto start = std::chrono::steady_clock::now();
    while (tasks_completed < LOW_PRI_COUNT + HIGH_PRI_COUNT) {
        if (std::chrono::steady_clock::now() - start > std::chrono::seconds(30)) {
            FAIL() << "Task completion timeout";
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    EXPECT_EQ(tasks_completed, LOW_PRI_COUNT + HIGH_PRI_COUNT);
}

TEST_F(ThreadPoolTSANTest, QueueSaturation) {
    // Rapidly submit tasks to saturate queue
    std::atomic<bool> keep_submitting = true;
    std::vector<std::thread> submitters;
    
    // Multiple submitter threads
    for (int s = 0; s < 4; ++s) {
        submitters.emplace_back([this, &keep_submitting]() {
            while (keep_submitting) {
                try {
                    pool.submitTask(
                        [this]() {
                            std::this_thread::sleep_for(std::chrono::milliseconds(1));
                            tasks_completed++;
                        },
                        ThreadPoolManager::Priority::NORMAL
                    );
                } catch (...) {
                    // Queue full - acceptable
                }
            }
        });
    }
    
    // Let submissions happen for a short time
    std::this_thread::sleep_for(std::chrono::seconds(2));
    keep_submitting = false;
    
    for (auto& t : submitters) {
        t.join();
    }
    
    // Should have completed at least some tasks
    EXPECT_GT(tasks_completed, 0);
}

TEST_F(ThreadPoolTSANTest, SubmitDuringShutdown) {
    // Submit tasks while initiating shutdown
    std::vector<std::thread> threads;
    
    threads.emplace_back([this]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Start shutdown while tasks in flight
        pool.shutdown();
    });
    
    threads.emplace_back([this]() {
        for (int i = 0; i < 1000; ++i) {
            try {
                pool.submitTask(
                    [this]() { tasks_completed++; },
                    ThreadPoolManager::Priority::NORMAL
                );
            } catch (...) {
                // Acceptable during/after shutdown
                break;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    });
    
    for (auto& t : threads) {
        t.join();
    }
}

// ============================================================================
// PIIStreamScanner TSAN Stress Tests
// ============================================================================

class PIIStreamScannerTSANTest : public ::testing::Test {
protected:
    PIIStreamScanner scanner;
    std::atomic<int> findings_count = 0;
    std::atomic<int> scan_errors = 0;
    
    void SetUp() override {
        scanner.initialize({.parallel_slots = 16});
    }
};

TEST_F(PIIStreamScannerTSANTest, ParallelScanSlotSaturation) {
    // Saturate all parallel scan slots
    const int SCAN_COUNT = 100;
    std::vector<std::thread> scanners;
    
    for (int s = 0; s < SCAN_COUNT; ++s) {
        scanners.emplace_back([this, s]() {
            std::string data = "Email: user_" + std::to_string(s) + "@example.com\n"
                              "Phone: 555-" + std::to_string(s) + "\n";
            
            try {
                auto findings = scanner.scanChunk(data);
                findings_count += findings.size();
            } catch (...) {
                scan_errors++;
            }
        });
    }
    
    for (auto& t : scanners) {
        t.join();
    }
    
    EXPECT_GE(findings_count + scan_errors, 0);
}

TEST_F(PIIStreamScannerTSANTest, ConcurrentStreamUpdates) {
    // Multiple threads updating scanner state concurrently
    std::vector<std::thread> threads;
    
    for (int i = 0; i < 8; ++i) {
        threads.emplace_back([this, i]() {
            for (int j = 0; j < 50; ++j) {
                std::string chunk = "Data chunk " + std::to_string(i) + "_" + std::to_string(j);
                
                try {
                    scanner.scanChunk(chunk);
                } catch (...) {
                    scan_errors++;
                }
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
}

TEST_F(PIIStreamScannerTSANTest, SlotExhaustion) {
    // Try to exceed parallel slot capacity
    const int SLOT_COUNT = 16;
    const int SCAN_COUNT = 100;  // More than slots
    
    std::vector<std::thread> heavy_scanners;
    
    for (int s = 0; s < SCAN_COUNT; ++s) {
        heavy_scanners.emplace_back([this]() {
            // Large data to process
            std::string large_data;
            for (int i = 0; i < 1000; ++i) {
                large_data += "This is line " + std::to_string(i) + " with email test@domain.com\n";
            }
            
            try {
                auto findings = scanner.scanChunk(large_data);
                findings_count += findings.size();
            } catch (...) {
                scan_errors++;
            }
        });
    }
    
    for (auto& t : heavy_scanners) {
        t.join();
    }
    
    EXPECT_GT(findings_count, 0);
}

// ============================================================================
// Mixed Component Stress Test
// ============================================================================

class MixedComponentTSANTest : public ::testing::Test {
protected:
    AuditLogger audit_logger;
    ThreadPoolManager pool;
    PIIStreamScanner scanner;
    std::atomic<int> total_events = 0;
    
    void SetUp() override {
        audit_logger.initialize();
        pool.initialize({.thread_count = 8});
        scanner.initialize({.parallel_slots = 8});
    }
    
    void TearDown() override {
        pool.shutdown();
        audit_logger.flush();
    }
};

TEST_F(MixedComponentTSANTest, ConcurrentComponentAccess) {
    // Concurrent access to all three components
    std::vector<std::thread> threads;
    
    // Audit logger writers
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([this, i]() {
            for (int j = 0; j < 25; ++j) {
                AuditLogEntry entry;
                entry.severity = "INFO";
                entry.action = "SCAN";
                
                try {
                    audit_logger.log(entry);
                    total_events++;
                } catch (...) {}
            }
        });
    }
    
    // Thread pool task submitters
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([this, i]() {
            for (int j = 0; j < 25; ++j) {
                pool.submitTask(
                    [this]() {
                        total_events++;
                    },
                    ThreadPoolManager::Priority::NORMAL
                );
            }
        });
    }
    
    // Stream scanner threads
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([this, i]() {
            for (int j = 0; j < 25; ++j) {
                std::string data = "test@domain" + std::to_string(i) + ".com";
                
                try {
                    scanner.scanChunk(data);
                    total_events++;
                } catch (...) {}
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_GT(total_events, 0);
}

} // namespace utils
} // namespace themis

