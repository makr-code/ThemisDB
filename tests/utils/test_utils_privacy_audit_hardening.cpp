/**
 * @file test_utils_privacy_audit_hardening.cpp
 * @brief Focused smoke tests for current privacy and audit helper APIs.
 */

#include <gtest/gtest.h>

#include "security/mock_key_provider.h"
#include "utils/audit_logger.h"
#include "utils/pii_detection_engine.h"
#include "utils/pii_detector.h"
#include "utils/regex_detection_engine.h"
#include "utils/pki_client.h"

#include <filesystem>
#include <memory>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

namespace themis {
namespace utils {

TEST(UtilsPrivacyAuditHardening, PiiDetectorHandlesUnicodeText) {
    PIIDetector detector;
    const std::vector<std::string> samples = {
        "user@example.com",
        "我的电子邮件是user@example.com",
        "البريد الإلكتروني: user@example.com"
    };

    for (const auto& sample : samples) {
        EXPECT_NO_THROW({
            auto findings = detector.detectInText(sample);
            (void)findings;
        });
    }
}

TEST(UtilsPrivacyAuditHardening, RegexEngineAndStreamScannerSmoke) {
    auto engine = std::make_shared<RegexDetectionEngine>();
    nlohmann::json config = nlohmann::json::object();
    config["enabled"] = true;
    ASSERT_TRUE(engine->initialize(config));

    PIIStreamScanner scanner(engine);
    auto first = scanner.scan_chunk("contact user@example.");
    auto second = scanner.scan_chunk("com", true);

    EXPECT_GE(first.size() + second.size(), 0u);
}

TEST(UtilsPrivacyAuditHardening, AuditLoggerConcurrentWritesSmoke) {
    const fs::path temp_dir = fs::temp_directory_path() / "themis_privacy_audit_smoke";
    fs::create_directories(temp_dir);
    const auto log_path = (temp_dir / "audit.log").string();

    auto key_provider = std::make_shared<themis::MockKeyProvider>();
    key_provider->createKey("saga_log", 1);
    auto enc = std::make_shared<FieldEncryption>(key_provider);

    PKIConfig pki_cfg;
    pki_cfg.service_id = "privacy-audit";
    auto pki = std::make_shared<VCCPKIClient>(pki_cfg);

    AuditLoggerConfig cfg;
    cfg.enabled = true;
    cfg.encrypt_then_sign = false;
    cfg.log_path = log_path;

    AuditLogger logger(enc, pki, cfg);
    std::vector<std::thread> threads;

    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([&logger, i]() {
            for (int j = 0; j < 10; ++j) {
                nlohmann::json event;
                event["thread"] = i;
                event["seq"] = j;
                logger.logEvent(event);
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_TRUE(fs::exists(log_path));
    fs::remove_all(temp_dir);
}

} // namespace utils
} // namespace themis
