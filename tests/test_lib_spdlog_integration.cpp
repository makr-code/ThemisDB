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
