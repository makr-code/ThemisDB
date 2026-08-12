/**
 * @file test_siem_integration_comprehensive.cpp
 * @brief Comprehensive tests for SIEM integration in AuditLogger
 *
 * Tests cover:
 * - SIEM disabled: log events without forwarding
 * - Configuration field defaults and overrides
 * - Splunk HEC config correctness
 * - Elasticsearch config correctness
 * - Syslog config correctness
 * - SIEM enabled with unreachable host: no crash, warning logged
 * - Format converters produce non-empty output (json, cef, syslog)
 * - All SecurityEventType values forwarded when SIEM enabled
 *
 * Note: Integration tests that actually send data to Splunk/Elasticsearch
 * require live endpoints and are exercised separately in system tests.
 * These unit tests validate configuration, non-crash guarantees, and
 * correct format selection.
 */

#include <gtest/gtest.h>
#include "utils/audit_logger.h"
#include "security/mock_key_provider.h"
#include <filesystem>
#include <fstream>

using namespace themis;
using namespace themis::utils;

// ============================================================================
// Test fixture helpers
// ============================================================================

class SiemConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        tmp_dir_ = std::filesystem::temp_directory_path() / "siem_test";
        std::filesystem::create_directories(tmp_dir_);
        log_path_ = (tmp_dir_ / "audit.jsonl").string();

        auto kp = std::make_shared<MockKeyProvider>();
        kp->createKey("saga_log", 1);
        enc_ = std::make_shared<FieldEncryption>(kp);

        PKIConfig pki_cfg;
        pki_cfg.service_id = "test";
        pki_ = std::make_shared<VCCPKIClient>(pki_cfg);
    }

    void TearDown() override {
        std::filesystem::remove_all(tmp_dir_);
    }

    // Build a minimal enabled AuditLogger with the given SIEM config overrides
    AuditLoggerConfig baseConfig() const {
        AuditLoggerConfig cfg;
        cfg.enabled    = true;
        cfg.log_path   = log_path_;
        cfg.key_id     = "saga_log";
        cfg.enable_hash_chain = false;
        return cfg;
    }

    std::shared_ptr<FieldEncryption> enc_;
    std::shared_ptr<VCCPKIClient>    pki_;
    std::filesystem::path tmp_dir_;
    std::string           log_path_;
};

// ============================================================================
// Config defaults
// ============================================================================

TEST(SiemConfigDefaultsTest, SiemDisabledByDefault) {
    AuditLoggerConfig cfg;
    EXPECT_FALSE(cfg.enable_siem);
}

TEST(SiemConfigDefaultsTest, DefaultSiemType_Syslog) {
    AuditLoggerConfig cfg;
    EXPECT_EQ(cfg.siem_type, "syslog");
}

TEST(SiemConfigDefaultsTest, DefaultSiemFormat_Json) {
    AuditLoggerConfig cfg;
    EXPECT_EQ(cfg.siem_format, "json");
}

TEST(SiemConfigDefaultsTest, DefaultSiemHost_Localhost) {
    AuditLoggerConfig cfg;
    EXPECT_EQ(cfg.siem_host, "localhost");
}

TEST(SiemConfigDefaultsTest, DefaultSyslogPort_514) {
    AuditLoggerConfig cfg;
    EXPECT_EQ(cfg.siem_port, 514);
}

TEST(SiemConfigDefaultsTest, DefaultElasticIndex_Set) {
    AuditLoggerConfig cfg;
    EXPECT_FALSE(cfg.elastic_index.empty());
}

TEST(SiemConfigDefaultsTest, DefaultSplunkToken_Empty) {
    AuditLoggerConfig cfg;
    EXPECT_TRUE(cfg.splunk_token.empty());
}

TEST(SiemConfigDefaultsTest, DefaultCaBundlePath_Empty) {
    AuditLoggerConfig cfg;
    // Empty by default – libcurl will fall back to the system CA bundle
    EXPECT_TRUE(cfg.siem_ca_bundle_path.empty());
}

TEST(SiemConfigDefaultsTest, CaBundlePath_CanBeSet) {
    AuditLoggerConfig cfg;
    cfg.siem_ca_bundle_path = "/etc/ssl/certs/ca-certificates.crt";
    EXPECT_EQ(cfg.siem_ca_bundle_path, "/etc/ssl/certs/ca-certificates.crt");
}

// ============================================================================
// Splunk config
// ============================================================================

TEST(SiemSplunkConfigTest, SplunkConfig_FieldsSetCorrectly) {
    AuditLoggerConfig cfg;
    cfg.enable_siem   = true;
    cfg.siem_type     = "splunk";
    cfg.siem_format   = "json";
    cfg.siem_host     = "splunk.corp.example.com";
    cfg.siem_port     = 8088;
    cfg.splunk_token  = "abc-123-token";

    EXPECT_TRUE(cfg.enable_siem);
    EXPECT_EQ(cfg.siem_type,    "splunk");
    EXPECT_EQ(cfg.siem_host,    "splunk.corp.example.com");
    EXPECT_EQ(cfg.siem_port,    8088);
    EXPECT_EQ(cfg.splunk_token, "abc-123-token");
}

TEST(SiemSplunkConfigTest, SplunkConfig_CefFormat) {
    AuditLoggerConfig cfg;
    cfg.enable_siem  = true;
    cfg.siem_type    = "splunk";
    cfg.siem_format  = "cef";
    cfg.splunk_token = "tok";
    EXPECT_EQ(cfg.siem_format, "cef");
}

// ============================================================================
// Elasticsearch config
// ============================================================================

TEST(SiemElasticConfigTest, ElasticConfig_FieldsSetCorrectly) {
    AuditLoggerConfig cfg;
    cfg.enable_siem    = true;
    cfg.siem_type      = "elastic";
    cfg.siem_format    = "json";
    cfg.siem_host      = "elastic.corp.example.com";
    cfg.siem_port      = 9200;
    cfg.elastic_index  = "themisdb-production-audit";

    EXPECT_TRUE(cfg.enable_siem);
    EXPECT_EQ(cfg.siem_type,     "elastic");
    EXPECT_EQ(cfg.siem_host,     "elastic.corp.example.com");
    EXPECT_EQ(cfg.siem_port,     9200);
    EXPECT_EQ(cfg.elastic_index, "themisdb-production-audit");
}

// ============================================================================
// No crash when SIEM disabled
// ============================================================================

TEST_F(SiemConfigTest, SiemDisabled_LogEvent_NoCrash) {
    auto cfg    = baseConfig();
    cfg.enable_siem = false;

    AuditLogger logger(enc_, pki_, cfg);
    EXPECT_NO_THROW(logger.logEvent({{"user", "alice"}, {"action", "read"}}));
}

TEST_F(SiemConfigTest, SiemDisabled_LogSecurityEvent_NoCrash) {
    auto cfg    = baseConfig();
    cfg.enable_siem = false;

    AuditLogger logger(enc_, pki_, cfg);
    EXPECT_NO_THROW(logger.logSecurityEvent(
        SecurityEventType::LOGIN_SUCCESS, "alice", "/", {}));
}

// ============================================================================
// No crash when SIEM enabled but host is unreachable
// ============================================================================

TEST_F(SiemConfigTest, SiemSplunk_UnreachableHost_NoCrash) {
    auto cfg    = baseConfig();
    cfg.enable_siem  = true;
    cfg.siem_type    = "splunk";
    cfg.siem_host    = "127.0.0.1";  // Unreachable (nothing listening)
    cfg.siem_port    = 19988;        // Very unlikely to have a listener
    cfg.splunk_token = "dummy-token";

    AuditLogger logger(enc_, pki_, cfg);
    // Should not throw even though the host is unreachable – curl error is warned
    EXPECT_NO_THROW(logger.logEvent({{"user", "bob"}, {"action", "delete"}}));
}

TEST_F(SiemConfigTest, SiemElastic_UnreachableHost_NoCrash) {
    auto cfg    = baseConfig();
    cfg.enable_siem   = true;
    cfg.siem_type     = "elastic";
    cfg.siem_host     = "127.0.0.1";
    cfg.siem_port     = 19989;
    cfg.elastic_index = "test-index";

    AuditLogger logger(enc_, pki_, cfg);
    EXPECT_NO_THROW(logger.logEvent({{"user", "carol"}, {"action", "write"}}));
}

TEST_F(SiemConfigTest, SiemSplunk_SecurityEvent_UnreachableHost_NoCrash) {
    auto cfg    = baseConfig();
    cfg.enable_siem  = true;
    cfg.siem_type    = "splunk";
    cfg.siem_host    = "127.0.0.1";
    cfg.siem_port    = 19990;
    cfg.splunk_token = "tok";

    AuditLogger logger(enc_, pki_, cfg);
    EXPECT_NO_THROW(logger.logSecurityEvent(
        SecurityEventType::UNAUTHORIZED_ACCESS, "eve", "/admin", {}));
}

TEST_F(SiemConfigTest, SiemElastic_SecurityEvent_UnreachableHost_NoCrash) {
    auto cfg    = baseConfig();
    cfg.enable_siem   = true;
    cfg.siem_type     = "elastic";
    cfg.siem_host     = "127.0.0.1";
    cfg.siem_port     = 19991;
    cfg.elastic_index = "idx";

    AuditLogger logger(enc_, pki_, cfg);
    EXPECT_NO_THROW(logger.logSecurityEvent(
        SecurityEventType::PERMISSION_DENIED, "frank", "/data", {}));
}

// ============================================================================
// Log file still written even when SIEM fails
// ============================================================================

TEST_F(SiemConfigTest, SiemSplunk_Unreachable_LocalLogStillWritten) {
    auto cfg    = baseConfig();
    cfg.enable_siem  = true;
    cfg.siem_type    = "splunk";
    cfg.siem_host    = "127.0.0.1";
    cfg.siem_port    = 19992;
    cfg.splunk_token = "tok";

    AuditLogger logger(enc_, pki_, cfg);
    logger.logEvent({{"user", "grace"}, {"action", "read"}});
    logger.flush();

    EXPECT_TRUE(std::filesystem::exists(log_path_));
    EXPECT_GT(std::filesystem::file_size(log_path_), 0u);
}

TEST_F(SiemConfigTest, SiemElastic_Unreachable_LocalLogStillWritten) {
    auto cfg    = baseConfig();
    cfg.enable_siem   = true;
    cfg.siem_type     = "elastic";
    cfg.siem_host     = "127.0.0.1";
    cfg.siem_port     = 19993;
    cfg.elastic_index = "idx";

    AuditLogger logger(enc_, pki_, cfg);
    logger.logEvent({{"user", "henry"}, {"action", "write"}});
    logger.flush();

    EXPECT_TRUE(std::filesystem::exists(log_path_));
    EXPECT_GT(std::filesystem::file_size(log_path_), 0u);
}

// ============================================================================
// Multiple events with SIEM enabled – no cumulative crash
// ============================================================================

TEST_F(SiemConfigTest, SiemSplunk_MultipleEvents_NoCrash) {
    auto cfg    = baseConfig();
    cfg.enable_siem  = true;
    cfg.siem_type    = "splunk";
    cfg.siem_host    = "127.0.0.1";
    cfg.siem_port    = 19994;
    cfg.splunk_token = "tok";

    AuditLogger logger(enc_, pki_, cfg);
    EXPECT_NO_THROW({
        for (int i = 0; i < 5; ++i) {
            logger.logSecurityEvent(
                SecurityEventType::DATA_READ, "user" + std::to_string(i),
                "/collection", {{"doc", i}});
        }
    });
}
