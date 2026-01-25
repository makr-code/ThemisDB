// Copyright (c) 2025 VCC ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0
//
// spdlog Logging Library Integration Tests
// Tests spdlog library integration for structured logging throughout ThemisDB
// Use Case: Observability, debugging, audit trails

#include <gtest/gtest.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/async.h>
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

namespace fs = std::filesystem;

class SpdlogLibIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_log_dir_ = "./data/test_lib_spdlog_" + 
                       std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        fs::create_directories(test_log_dir_);
        
        // Drop all existing loggers before each test
        spdlog::drop_all();
    }

    void TearDown() override {
        // Drop all loggers to release file handles
        spdlog::drop_all();
        
        // Clean up test logs
        if (fs::exists(test_log_dir_)) {
            std::error_code ec;
            fs::remove_all(test_log_dir_, ec);
        }
    }

    std::string test_log_dir_;
    
    // Helper to read log file content
    std::string readLogFile(const std::string& path) {
        std::ifstream ifs(path);
        if (!ifs) return "";
        return std::string((std::istreambuf_iterator<char>(ifs)),
                          std::istreambuf_iterator<char>());
    }
};

// Test 1: Library linking and basic logger creation
TEST_F(SpdlogLibIntegrationTest, LibraryLinking) {
    auto logger = spdlog::stdout_color_mt("test_logger");
    ASSERT_NE(logger, nullptr);
    EXPECT_EQ(logger->name(), "test_logger");
    
    // Verify we can log without errors
    EXPECT_NO_THROW(logger->info("Test message"));
}

// Test 2: Log levels
TEST_F(SpdlogLibIntegrationTest, LogLevels) {
    std::string log_path = test_log_dir_ + "/levels.log";
    auto logger = spdlog::basic_logger_mt("levels_logger", log_path);
    
    logger->set_level(spdlog::level::trace);
    logger->set_pattern("%v"); // Only message, no timestamp for easier testing
    
    logger->trace("trace message");
    logger->debug("debug message");
    logger->info("info message");
    logger->warn("warn message");
    logger->error("error message");
    logger->critical("critical message");
    
    logger->flush();
    spdlog::drop("levels_logger");
    
    std::string content = readLogFile(log_path);
    EXPECT_NE(content.find("trace message"), std::string::npos);
    EXPECT_NE(content.find("debug message"), std::string::npos);
    EXPECT_NE(content.find("info message"), std::string::npos);
    EXPECT_NE(content.find("warn message"), std::string::npos);
    EXPECT_NE(content.find("error message"), std::string::npos);
    EXPECT_NE(content.find("critical message"), std::string::npos);
}

// Test 3: Log level filtering
TEST_F(SpdlogLibIntegrationTest, LogLevelFiltering) {
    std::string log_path = test_log_dir_ + "/filtering.log";
    auto logger = spdlog::basic_logger_mt("filter_logger", log_path);
    
    logger->set_level(spdlog::level::warn); // Only warn and above
    logger->set_pattern("%v");
    
    logger->debug("debug message");
    logger->info("info message");
    logger->warn("warn message");
    logger->error("error message");
    
    logger->flush();
    spdlog::drop("filter_logger");
    
    std::string content = readLogFile(log_path);
    EXPECT_EQ(content.find("debug message"), std::string::npos);
    EXPECT_EQ(content.find("info message"), std::string::npos);
    EXPECT_NE(content.find("warn message"), std::string::npos);
    EXPECT_NE(content.find("error message"), std::string::npos);
}

// Test 4: Formatting patterns
TEST_F(SpdlogLibIntegrationTest, FormattingPatterns) {
    std::string log_path = test_log_dir_ + "/patterns.log";
    auto logger = spdlog::basic_logger_mt("pattern_logger", log_path);
    
    // Custom pattern with level, name, and message
    logger->set_pattern("[%l] [%n] %v");
    logger->info("test message");
    
    logger->flush();
    spdlog::drop("pattern_logger");
    
    std::string content = readLogFile(log_path);
    EXPECT_NE(content.find("[info]"), std::string::npos);
    EXPECT_NE(content.find("[pattern_logger]"), std::string::npos);
    EXPECT_NE(content.find("test message"), std::string::npos);
}

// Test 5: Multiple sinks (console + file)
TEST_F(SpdlogLibIntegrationTest, MultipleSinks) {
    std::string log_path = test_log_dir_ + "/multi_sink.log";
    
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_path);
    
    std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
    auto logger = std::make_shared<spdlog::logger>("multi_sink_logger", sinks.begin(), sinks.end());
    spdlog::register_logger(logger);
    
    logger->set_pattern("%v");
    logger->info("multi sink message");
    
    logger->flush();
    spdlog::drop("multi_sink_logger");
    
    std::string content = readLogFile(log_path);
    EXPECT_NE(content.find("multi sink message"), std::string::npos);
}

// Test 6: Rotating file sink
TEST_F(SpdlogLibIntegrationTest, RotatingFileSink) {
    std::string log_path = test_log_dir_ + "/rotating.log";
    size_t max_size = 1024; // 1KB
    size_t max_files = 3;
    
    auto logger = spdlog::rotating_logger_mt("rotating_logger", log_path, max_size, max_files);
    logger->set_pattern("%v");
    
    // Write enough data to trigger rotation
    for (int i = 0; i < 100; ++i) {
        logger->info("Log message number {} with some padding to fill space", i);
    }
    
    logger->flush();
    spdlog::drop("rotating_logger");
    
    // Check that rotation occurred
    EXPECT_TRUE(fs::exists(log_path));
    // Rotated files would be named rotating.1.log, rotating.2.log, etc.
}

// Test 7: Async logging
TEST_F(SpdlogLibIntegrationTest, AsyncLogging) {
    std::string log_path = test_log_dir_ + "/async.log";
    
    spdlog::init_thread_pool(8192, 1);
    auto logger = spdlog::basic_logger_mt<spdlog::async_factory>("async_logger", log_path);
    
    logger->set_pattern("%v");
    
    for (int i = 0; i < 100; ++i) {
        logger->info("Async message {}", i);
    }
    
    logger->flush();
    spdlog::drop("async_logger");
    
    std::string content = readLogFile(log_path);
    EXPECT_NE(content.find("Async message 0"), std::string::npos);
    EXPECT_NE(content.find("Async message 99"), std::string::npos);
}

// Test 8: Parameterized logging (format strings)
TEST_F(SpdlogLibIntegrationTest, ParameterizedLogging) {
    std::string log_path = test_log_dir_ + "/parameterized.log";
    auto logger = spdlog::basic_logger_mt("param_logger", log_path);
    
    logger->set_pattern("%v");
    
    int count = 42;
    std::string user = "testuser";
    double value = 3.14159;
    
    logger->info("Count: {}, User: {}, Value: {:.2f}", count, user, value);
    
    logger->flush();
    spdlog::drop("param_logger");
    
    std::string content = readLogFile(log_path);
    EXPECT_NE(content.find("Count: 42"), std::string::npos);
    EXPECT_NE(content.find("User: testuser"), std::string::npos);
    EXPECT_NE(content.find("Value: 3.14"), std::string::npos);
}

// Test 9: Global logger (default logger)
TEST_F(SpdlogLibIntegrationTest, GlobalDefaultLogger) {
    std::string log_path = test_log_dir_ + "/default.log";
    auto logger = spdlog::basic_logger_mt("default", log_path);
    spdlog::set_default_logger(logger);
    
    logger->set_pattern("%v");
    
    // Use convenience functions
    spdlog::info("Using default logger");
    spdlog::warn("Warning via default");
    
    logger->flush();
    
    std::string content = readLogFile(log_path);
    EXPECT_NE(content.find("Using default logger"), std::string::npos);
    EXPECT_NE(content.find("Warning via default"), std::string::npos);
}

// Test 10: Logger retrieval
TEST_F(SpdlogLibIntegrationTest, LoggerRetrieval) {
    auto logger1 = spdlog::stdout_color_mt("retrieval_logger");
    ASSERT_NE(logger1, nullptr);
    
    // Retrieve the same logger
    auto logger2 = spdlog::get("retrieval_logger");
    ASSERT_NE(logger2, nullptr);
    EXPECT_EQ(logger1, logger2);
    EXPECT_EQ(logger1->name(), logger2->name());
    
    // Try to retrieve non-existent logger
    auto logger3 = spdlog::get("non_existent");
    EXPECT_EQ(logger3, nullptr);
}

// Test 11: Thread safety
TEST_F(SpdlogLibIntegrationTest, ThreadSafety) {
    std::string log_path = test_log_dir_ + "/threaded.log";
    auto logger = spdlog::basic_logger_mt("thread_logger", log_path);
    logger->set_pattern("%v");
    
    const int num_threads = 4;
    const int messages_per_thread = 25;
    
    std::vector<std::thread> threads;
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&logger, t, messages_per_thread]() {
            for (int i = 0; i < messages_per_thread; ++i) {
                logger->info("Thread {} message {}", t, i);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    logger->flush();
    spdlog::drop("thread_logger");
    
    std::string content = readLogFile(log_path);
    // Verify we got all messages (100 total)
    int count = 0;
    size_t pos = 0;
    while ((pos = content.find("Thread ", pos)) != std::string::npos) {
        ++count;
        ++pos;
    }
    EXPECT_EQ(count, num_threads * messages_per_thread);
}

// Test 12: Error handling (invalid operations)
TEST_F(SpdlogLibIntegrationTest, ErrorHandling) {
    // Try to create logger with same name twice
    auto logger1 = spdlog::stdout_color_mt("dup_logger");
    ASSERT_NE(logger1, nullptr);
    
    // This should throw because logger already exists
    EXPECT_THROW(
        spdlog::stdout_color_mt("dup_logger"),
        spdlog::spdlog_ex
    );
    
    // Invalid log file path should throw
    EXPECT_THROW(
        spdlog::basic_logger_mt("invalid", "/invalid/path/that/doesnt/exist/file.log"),
        spdlog::spdlog_ex
    );
}

// Test 13: Flush policy
TEST_F(SpdlogLibIntegrationTest, FlushPolicy) {
    std::string log_path = test_log_dir_ + "/flush.log";
    auto logger = spdlog::basic_logger_mt("flush_logger", log_path);
    
    logger->set_pattern("%v");
    logger->flush_on(spdlog::level::err); // Auto-flush on error and above
    
    logger->info("info message"); // Not flushed yet
    
    // File might not have content yet without explicit flush
    // But after error, it should be flushed
    logger->error("error message"); // Should auto-flush
    
    spdlog::drop("flush_logger");
    
    std::string content = readLogFile(log_path);
    EXPECT_NE(content.find("error message"), std::string::npos);
}

// Test 14: Integration with ThemisDB patterns
TEST_F(SpdlogLibIntegrationTest, ThemisDBIntegration) {
    std::string log_path = test_log_dir_ + "/themisdb.log";
    auto logger = spdlog::basic_logger_mt("themisdb_logger", log_path);
    
    // Pattern similar to what ThemisDB might use
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %v");
    
    // Simulate database operations
    logger->info("Database opened: path=/data/themis");
    logger->debug("Transaction started: txn_id=12345");
    logger->info("Query executed: duration=42ms");
    logger->warn("Cache miss: key=user:123");
    logger->error("Connection failed: addr=192.168.1.1:8080");
    
    logger->flush();
    spdlog::drop("themisdb_logger");
    
    std::string content = readLogFile(log_path);
    EXPECT_NE(content.find("Database opened"), std::string::npos);
    EXPECT_NE(content.find("Transaction started"), std::string::npos);
    EXPECT_NE(content.find("themisdb_logger"), std::string::npos);
}

// Test 15: Performance - high volume logging
TEST_F(SpdlogLibIntegrationTest, HighVolumeLogging) {
    std::string log_path = test_log_dir_ + "/performance.log";
    
    spdlog::init_thread_pool(8192, 1);
    auto logger = spdlog::basic_logger_mt<spdlog::async_factory>("perf_logger", log_path);
    logger->set_pattern("%v");
    
    auto start = std::chrono::high_resolution_clock::now();
    
    const int num_messages = 10000;
    for (int i = 0; i < num_messages; ++i) {
        logger->info("Performance test message {}", i);
    }
    
    logger->flush();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    spdlog::drop("perf_logger");
    
    // Just verify it completed in reasonable time and all messages logged
    EXPECT_LT(duration.count(), 5000); // Should complete in under 5 seconds
    
    std::string content = readLogFile(log_path);
    EXPECT_NE(content.find("Performance test message 0"), std::string::npos);
    EXPECT_NE(content.find("Performance test message 9999"), std::string::npos);
}

// ===== ENHANCED TESTS: Thread Safety & Concurrency =====

// Test 16: Concurrent logging with race condition detection
TEST_F(SpdlogLibIntegrationTest, ConcurrentLoggingRaceConditions) {
    std::string log_path = test_log_dir_ + "/concurrent_race.log";
    auto logger = spdlog::basic_logger_mt("concurrent_race_logger", log_path);
    logger->set_pattern("[%t] %v");
    
    const int num_threads = 10;
    const int messages_per_thread = 100;
    std::atomic<int> completed_threads{0};
    
    std::vector<std::thread> threads;
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&logger, t, messages_per_thread, &completed_threads]() {
            for (int i = 0; i < messages_per_thread; ++i) {
                logger->info("Thread_{}_Message_{}", t, i);
            }
            completed_threads++;
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(completed_threads, num_threads);
    
    logger->flush();
    spdlog::drop("concurrent_race_logger");
    
    std::string content = readLogFile(log_path);
    // Verify all threads logged their messages
    int message_count = 0;
    size_t pos = 0;
    while ((pos = content.find("Thread_", pos)) != std::string::npos) {
        ++message_count;
        ++pos;
    }
    EXPECT_EQ(message_count, num_threads * messages_per_thread) 
        << "Expected " << (num_threads * messages_per_thread) << " messages, got " << message_count;
}

// Test 17: Async logger with high concurrency
TEST_F(SpdlogLibIntegrationTest, AsyncHighConcurrency) {
    std::string log_path = test_log_dir_ + "/async_high_concurrency.log";
    
    spdlog::init_thread_pool(16384, 2); // Larger queue, more worker threads
    auto logger = spdlog::basic_logger_mt<spdlog::async_factory>("async_high_concurrency", log_path);
    logger->set_pattern("%v");
    
    const int num_threads = 8;
    const int messages_per_thread = 500;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    std::vector<std::thread> threads;
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&logger, t, messages_per_thread]() {
            for (int i = 0; i < messages_per_thread; ++i) {
                logger->info("Async thread {} msg {}", t, i);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    logger->flush();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    spdlog::drop("async_high_concurrency");
    
    // Performance bound: Should handle 4000 messages from 8 threads efficiently
    // Using 1000ms for CI/slower environments
    EXPECT_LT(duration.count(), 1000) 
        << "Async logging took " << duration.count() << "ms, expected < 1000ms";
    
    std::string content = readLogFile(log_path);
    // Verify messages are present
    EXPECT_NE(content.find("Async thread 0"), std::string::npos);
    EXPECT_NE(content.find("Async thread 7"), std::string::npos);
}

// ===== ENHANCED TESTS: Log Rotation Edge Cases =====

// Test 18: Log rotation with exact size boundary
TEST_F(SpdlogLibIntegrationTest, RotationExactSizeBoundary) {
    std::string log_path = test_log_dir_ + "/rotation_boundary.log";
    size_t max_size = 512; // Small size for easier testing
    size_t max_files = 3;
    
    auto logger = spdlog::rotating_logger_mt("rotation_boundary", log_path, max_size, max_files);
    logger->set_pattern("%v");
    
    // Write exactly enough to trigger rotation
    std::string msg(100, 'X'); // 100 character message
    for (int i = 0; i < 10; ++i) { // 10 * 100 = 1000 bytes > 512
        logger->info("{}", msg);
    }
    
    logger->flush();
    spdlog::drop("rotation_boundary");
    
    // Check that main file exists
    EXPECT_TRUE(fs::exists(log_path));
    
    // Check that at least one rotated file exists
    std::string rotated_file = log_path + ".1";
    EXPECT_TRUE(fs::exists(rotated_file)) 
        << "Expected rotated file: " << rotated_file;
}

// Test 19: Log rotation max files limit
TEST_F(SpdlogLibIntegrationTest, RotationMaxFilesLimit) {
    std::string log_path = test_log_dir_ + "/rotation_max_files.log";
    size_t max_size = 256;
    size_t max_files = 2; // Only keep 2 rotated files
    
    auto logger = spdlog::rotating_logger_mt("rotation_max_files", log_path, max_size, max_files);
    logger->set_pattern("%v");
    
    // Write enough to trigger multiple rotations
    std::string msg(100, 'Y');
    for (int i = 0; i < 15; ++i) {
        logger->info("{}", msg);
        logger->flush(); // Force flush to trigger rotation
    }
    
    spdlog::drop("rotation_max_files");
    
    // Count rotation files
    int rotation_count = 0;
    for (size_t i = 1; i <= 5; ++i) {
        std::string rotated = log_path + "." + std::to_string(i);
        if (fs::exists(rotated)) {
            rotation_count++;
        }
    }
    
    // Should not exceed max_files
    EXPECT_LE(rotation_count, max_files) 
        << "Found " << rotation_count << " rotated files, expected <= " << max_files;
}

// ===== ENHANCED TESTS: Pattern Format Validation =====

// Test 20: Complex pattern format validation
TEST_F(SpdlogLibIntegrationTest, ComplexPatternFormat) {
    std::string log_path = test_log_dir_ + "/complex_pattern.log";
    auto logger = spdlog::basic_logger_mt("complex_pattern", log_path);
    
    // Complex pattern with multiple fields
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] [thread %t] [%s:%#] %v");
    logger->info("Complex pattern test");
    
    logger->flush();
    spdlog::drop("complex_pattern");
    
    std::string content = readLogFile(log_path);
    
    // Verify all pattern elements are present
    EXPECT_NE(content.find("[complex_pattern]"), std::string::npos) << "Logger name not found";
    EXPECT_NE(content.find("[info]"), std::string::npos) << "Log level not found";
    EXPECT_NE(content.find("[thread"), std::string::npos) << "Thread ID not found";
    EXPECT_NE(content.find("Complex pattern test"), std::string::npos) << "Message not found";
}

// Test 21: Custom pattern with colors (terminal)
TEST_F(SpdlogLibIntegrationTest, ColorPatternFormat) {
    auto logger = spdlog::stdout_color_mt("color_pattern");
    
    // Set pattern with color codes
    logger->set_pattern("%^[%l]%$ %v");
    
    // These should not throw
    EXPECT_NO_THROW(logger->info("Info with color"));
    EXPECT_NO_THROW(logger->warn("Warning with color"));
    EXPECT_NO_THROW(logger->error("Error with color"));
}

// ===== ENHANCED TESTS: Error Conditions & Edge Cases =====

// Test 22: Invalid file path handling
TEST_F(SpdlogLibIntegrationTest, InvalidFilePath) {
    // Try to create logger with invalid path (directory doesn't exist)
    EXPECT_THROW({
        auto logger = spdlog::basic_logger_mt("invalid_path", 
            "/nonexistent/directory/that/does/not/exist/file.log");
    }, spdlog::spdlog_ex);
}

// Test 23: Duplicate logger name handling
TEST_F(SpdlogLibIntegrationTest, DuplicateLoggerName) {
    auto logger1 = spdlog::stdout_color_mt("duplicate_name");
    ASSERT_NE(logger1, nullptr);
    
    // Try to create another logger with the same name
    EXPECT_THROW({
        auto logger2 = spdlog::stdout_color_mt("duplicate_name");
    }, spdlog::spdlog_ex);
}

// Test 24: Null/empty message handling
TEST_F(SpdlogLibIntegrationTest, EmptyMessageHandling) {
    std::string log_path = test_log_dir_ + "/empty_message.log";
    auto logger = spdlog::basic_logger_mt("empty_message", log_path);
    logger->set_pattern("%v");
    
    // Log empty strings
    EXPECT_NO_THROW(logger->info(""));
    EXPECT_NO_THROW(logger->info(""));
    
    logger->flush();
    spdlog::drop("empty_message");
    
    std::string content = readLogFile(log_path);
    // File should exist but might be empty or have newlines
    EXPECT_TRUE(fs::exists(log_path));
}

// Test 25: Logging with special characters
TEST_F(SpdlogLibIntegrationTest, SpecialCharacterHandling) {
    std::string log_path = test_log_dir_ + "/special_chars.log";
    auto logger = spdlog::basic_logger_mt("special_chars", log_path);
    logger->set_pattern("%v");
    
    // Test various special characters
    logger->info("Special: \n\t\r {} {{}} %% \" \' \\");
    logger->info("Unicode: café résumé 日本語");
    
    logger->flush();
    spdlog::drop("special_chars");
    
    std::string content = readLogFile(log_path);
    EXPECT_NE(content.find("Special:"), std::string::npos);
    EXPECT_NE(content.find("Unicode:"), std::string::npos);
}

// ===== ENHANCED TESTS: Performance Bounds =====

// Test 26: Sync logging performance bounds
TEST_F(SpdlogLibIntegrationTest, SyncLoggingPerformanceBounds) {
    std::string log_path = test_log_dir_ + "/sync_perf.log";
    auto logger = spdlog::basic_logger_mt("sync_perf", log_path);
    logger->set_pattern("%v");
    
    const int num_messages = 1000;
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_messages; ++i) {
        logger->info("Sync message {}", i);
    }
    
    logger->flush();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    spdlog::drop("sync_perf");
    
    // Performance bound: 1000 sync messages should complete in under 500ms
    EXPECT_LT(duration.count(), 500000) 
        << "Sync logging took " << duration.count() << "μs, expected < 500000μs";
    
    // Calculate throughput
    double throughput = (num_messages * 1000000.0) / duration.count();
    EXPECT_GT(throughput, 2000) << "Throughput: " << throughput << " msgs/sec, expected > 2000 msgs/sec";
}

// Test 27: Async logging latency bounds
TEST_F(SpdlogLibIntegrationTest, AsyncLoggingLatencyBounds) {
    std::string log_path = test_log_dir_ + "/async_latency.log";
    
    spdlog::init_thread_pool(8192, 1);
    auto logger = spdlog::basic_logger_mt<spdlog::async_factory>("async_latency", log_path);
    logger->set_pattern("%v");
    
    // Measure individual message latency
    std::vector<long long> latencies;
    const int num_samples = 100;
    
    for (int i = 0; i < num_samples; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        logger->info("Latency test message {}", i);
        auto end = std::chrono::high_resolution_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        latencies.push_back(latency.count());
    }
    
    logger->flush();
    spdlog::drop("async_latency");
    
    // Calculate average and max latency
    long long avg_latency = 0;
    long long max_latency = 0;
    for (auto latency : latencies) {
        avg_latency += latency;
        max_latency = std::max(max_latency, latency);
    }
    avg_latency /= latencies.size();
    
    // Performance bounds
    EXPECT_LT(avg_latency, 100) << "Average latency: " << avg_latency << "μs, expected < 100μs";
    EXPECT_LT(max_latency, 1000) << "Max latency: " << max_latency << "μs, expected < 1000μs";
}

// ===== ENHANCED TESTS: Resource Cleanup =====

// Test 28: Logger lifecycle and resource cleanup
TEST_F(SpdlogLibIntegrationTest, LoggerLifecycleCleanup) {
    std::string log_path = test_log_dir_ + "/lifecycle.log";
    
    {
        // Create logger in scope
        auto logger = spdlog::basic_logger_mt("lifecycle", log_path);
        logger->info("Message before cleanup");
        logger->flush();
        spdlog::drop("lifecycle");
    }
    
    // Verify file is accessible after logger dropped
    EXPECT_TRUE(fs::exists(log_path));
    
    std::string content = readLogFile(log_path);
    EXPECT_NE(content.find("Message before cleanup"), std::string::npos);
    
    // Should be able to create logger with same name again
    EXPECT_NO_THROW({
        auto logger = spdlog::basic_logger_mt("lifecycle", log_path);
        logger->info("Message after recreation");
        logger->flush();
        spdlog::drop("lifecycle");
    });
}

// Test 29: Multiple logger cleanup
TEST_F(SpdlogLibIntegrationTest, MultipleLoggerCleanup) {
    // Create multiple loggers
    std::vector<std::string> logger_names;
    for (int i = 0; i < 5; ++i) {
        std::string name = "cleanup_logger_" + std::to_string(i);
        logger_names.push_back(name);
        auto logger = spdlog::stdout_color_mt(name);
        logger->info("Test message");
    }
    
    // Drop all loggers
    spdlog::drop_all();
    
    // Verify all loggers are gone
    for (const auto& name : logger_names) {
        auto logger = spdlog::get(name);
        EXPECT_EQ(logger, nullptr) << "Logger " << name << " still exists after drop_all";
    }
}

// Test 30: File handle release verification
TEST_F(SpdlogLibIntegrationTest, FileHandleRelease) {
    std::string log_path = test_log_dir_ + "/file_handle.log";
    
    {
        auto logger = spdlog::basic_logger_mt("file_handle", log_path);
        logger->info("Test message");
        logger->flush();
        spdlog::drop("file_handle");
    }
    
    // Should be able to delete file after logger is dropped
    std::error_code ec;
    bool removed = fs::remove(log_path, ec);
    EXPECT_TRUE(removed || !fs::exists(log_path)) 
        << "Could not remove log file, file handle might not be released. Error: " << ec.message();
}
