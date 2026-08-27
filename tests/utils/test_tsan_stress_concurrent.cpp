/**
 * @file test_tsan_stress_concurrent.cpp
 * @brief Focused concurrency smoke tests for utils components.
 */

#include <gtest/gtest.h>

#include "security/mock_key_provider.h"
#include "utils/audit_logger.h"
#include "utils/pii_detection_engine.h"
#include "utils/regex_detection_engine.h"
#include "utils/thread_pool_manager.h"
#include "utils/pki_client.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

namespace themis {
namespace utils {

TEST(UtilsTsanStressConcurrent, AuditLoggerConcurrentWritersSmoke) {
    const fs::path temp_dir = fs::temp_directory_path() / "themis_tsan_audit";
    fs::create_directories(temp_dir);
    const auto log_path = (temp_dir / "audit.log").string();

    auto key_provider = std::make_shared<themis::MockKeyProvider>();
    key_provider->createKey("saga_log", 1);
    auto enc = std::make_shared<FieldEncryption>(key_provider);

    PKIConfig pki_cfg;
    pki_cfg.service_id = "tsan-stress";
    auto pki = std::make_shared<VCCPKIClient>(pki_cfg);

    AuditLoggerConfig cfg;
    cfg.enabled = true;
    cfg.encrypt_then_sign = false;
    cfg.log_path = log_path;

    AuditLogger logger(enc, pki, cfg);
    std::atomic<int> written{0};

    std::vector<std::thread> threads;
    for (int i = 0; i < 8; ++i) {
        threads.emplace_back([&logger, &written, i]() {
            for (int j = 0; j < 25; ++j) {
                nlohmann::json event;
                event["thread"] = i;
                event["seq"] = j;
                logger.logEvent(event);
                ++written;
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_GT(written.load(), 0);
    EXPECT_TRUE(fs::exists(log_path));
    fs::remove_all(temp_dir);
}

TEST(UtilsTsanStressConcurrent, ThreadPoolManagerSubmitTaskSmoke) {
    ThreadPoolManager::Config config;
    config.cpu_pool.min_threads = 2;
    config.cpu_pool.max_threads = 2;
    config.cpu_pool.queue_size = 32;
    config.cpu_pool.name = "tsan-cpu";

    ThreadPoolManager manager(config);
    std::atomic<int> executed{0};

    for (int i = 0; i < 32; ++i) {
        EXPECT_TRUE(manager.submitTask(
            ThreadPoolManager::PoolType::CPU,
            [&executed]() {
                executed.fetch_add(1, std::memory_order_relaxed);
            },
            "tsan-task",
            Task::Priority::NORMAL));
    }

    manager.shutdown();
    EXPECT_GT(executed.load(), 0);
}

TEST(UtilsTsanStressConcurrent, PiiScannerSmokeAcrossThreads) {
    auto engine = std::make_shared<RegexDetectionEngine>();
    nlohmann::json config = nlohmann::json::object();
    config["enabled"] = true;
    ASSERT_TRUE(engine->initialize(config));

    std::atomic<int> findings_seen{0};
    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([engine, &findings_seen, i]() {
            PIIStreamScanner scanner(engine);
            const std::string text = "user" + std::to_string(i) + "@example.com";
            auto findings = scanner.scan_chunk(text, true);
            findings_seen.fetch_add(static_cast<int>(findings.size()), std::memory_order_relaxed);
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_GE(findings_seen.load(), 0);
}

} // namespace utils
} // namespace themis
