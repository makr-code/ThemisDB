/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_content_security.cpp                          ║
  Version:         0.0.35                                             ║
  Last Modified:   2026-03-16 04:24:01                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     694                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 018e0013e  2026-03-11  audit: close all remaining CON-006 documentation gaps ║
    • 30cd78e12  2026-03-11  feat(content/security): enforce zip-bomb protection with ... ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "content/content_security.h"
#include "content/content_errors.h"
#include <nlohmann/json.hpp>

using namespace themis::content;
using namespace themis::security;
using json = nlohmann::json;

// ============================================================================
// ContentSecurityConfig Tests
// ============================================================================

TEST(ContentSecurityConfigTest, DefaultValues) {
    ContentSecurityConfig config;
    
    EXPECT_TRUE(config.enable_malware_scan);
    EXPECT_TRUE(config.block_on_malware);
    EXPECT_EQ(config.malware_block_threshold, ThreatLevel::MEDIUM);
    EXPECT_FALSE(config.enable_pii_detection);
    EXPECT_FALSE(config.block_on_pii);
    EXPECT_TRUE(config.redact_pii_in_logs);
    EXPECT_FALSE(config.enable_abuse_detection);
    EXPECT_FALSE(config.block_on_abuse);
    EXPECT_TRUE(config.sanitize_error_messages);
    EXPECT_TRUE(config.hide_internal_paths);
    EXPECT_TRUE(config.hide_system_info);

    // Zip-bomb protection defaults
    EXPECT_TRUE(config.enable_zip_bomb_check);
    EXPECT_EQ(config.max_zip_bomb_ratio, 100u);
    EXPECT_EQ(config.max_zip_file_count, 1000u);
}

TEST(ContentSecurityConfigTest, ToJson) {
    ContentSecurityConfig config;
    config.enable_malware_scan = false;
    config.block_on_pii = true;
    
    json j = config.toJson();
    
    EXPECT_FALSE(j["enable_malware_scan"]);
    EXPECT_TRUE(j["block_on_pii"]);
    EXPECT_TRUE(j["sanitize_error_messages"]);
}

TEST(ContentSecurityConfigTest, FromJson) {
    json j;
    j["enable_malware_scan"] = false;
    j["enable_pii_detection"] = true;
    j["block_on_pii"] = true;
    j["malware_block_threshold"] = static_cast<int>(ThreatLevel::HIGH);
    
    auto config = ContentSecurityConfig::fromJson(j);
    
    EXPECT_FALSE(config.enable_malware_scan);
    EXPECT_TRUE(config.enable_pii_detection);
    EXPECT_TRUE(config.block_on_pii);
    EXPECT_EQ(config.malware_block_threshold, ThreatLevel::HIGH);
}

TEST(ContentSecurityConfigTest, RoundTripSerialization) {
    ContentSecurityConfig config1;
    config1.enable_malware_scan = false;
    config1.enable_pii_detection = true;
    config1.block_on_abuse = true;
    config1.hide_internal_paths = false;
    
    json j = config1.toJson();
    auto config2 = ContentSecurityConfig::fromJson(j);
    
    EXPECT_EQ(config1.enable_malware_scan, config2.enable_malware_scan);
    EXPECT_EQ(config1.enable_pii_detection, config2.enable_pii_detection);
    EXPECT_EQ(config1.block_on_abuse, config2.block_on_abuse);
    EXPECT_EQ(config1.hide_internal_paths, config2.hide_internal_paths);
}

// ============================================================================
// SecurityCheckResult Tests
// ============================================================================

TEST(SecurityCheckResultTest, ToJson) {
    SecurityCheckResult result;
    result.error = ContentError::ok();
    result.malware_checked = true;
    result.malware_clean = false;
    result.malware_threat = "Win32.Trojan";
    result.pii_checked = true;
    result.pii_found = true;
    result.pii_types = {"EMAIL", "PHONE"};
    
    json j = result.toJson();
    
    EXPECT_TRUE(j["malware_checked"]);
    EXPECT_FALSE(j["malware_clean"]);
    EXPECT_EQ(j["malware_threat"], "Win32.Trojan");
    EXPECT_TRUE(j["pii_checked"]);
    EXPECT_TRUE(j["pii_found"]);
    EXPECT_EQ(j["pii_types"].size(), 2);
}

// ============================================================================
// ContentSecurityManager Basic Tests
// ============================================================================

TEST(ContentSecurityManagerTest, Construction) {
    ContentSecurityConfig config;
    ContentSecurityManager manager(config);
    
    EXPECT_EQ(manager.getConfig().enable_malware_scan, config.enable_malware_scan);
}

TEST(ContentSecurityManagerTest, SetConfig) {
    ContentSecurityManager manager;
    
    ContentSecurityConfig new_config;
    new_config.enable_malware_scan = false;
    new_config.enable_pii_detection = true;
    
    manager.setConfig(new_config);
    
    EXPECT_FALSE(manager.getConfig().enable_malware_scan);
    EXPECT_TRUE(manager.getConfig().enable_pii_detection);
}

TEST(ContentSecurityManagerTest, InitialMetrics) {
    ContentSecurityManager manager;
    
    const auto& metrics = manager.getMetrics();
    
    EXPECT_EQ(metrics.total_checks.load(), 0);
    EXPECT_EQ(metrics.malware_scans.load(), 0);
    EXPECT_EQ(metrics.pii_scans.load(), 0);
    EXPECT_EQ(metrics.abuse_scans.load(), 0);
}

TEST(ContentSecurityManagerTest, ResetMetrics) {
    ContentSecurityManager manager;
    
    // Simulate some checks to increment metrics
    ContentSecurityConfig config;
    config.enable_malware_scan = false;  // Disable to avoid needing actual scanner
    config.enable_pii_detection = false;
    manager.setConfig(config);
    
    manager.checkContent("test", "text/plain", "content-1");
    
    EXPECT_GT(manager.getMetrics().total_checks.load(), 0);
    
    manager.resetMetrics();
    
    EXPECT_EQ(manager.getMetrics().total_checks.load(), 0);
}

// ============================================================================
// Content Check Tests (without actual scanners)
// ============================================================================

TEST(ContentSecurityManagerTest, CheckContentNoScanners) {
    ContentSecurityConfig config;
    config.enable_malware_scan = false;
    config.enable_pii_detection = false;
    config.enable_abuse_detection = false;
    
    ContentSecurityManager manager(config);
    
    auto result = manager.checkContent("test data", "text/plain", "content-1");
    
    EXPECT_TRUE(result.error.isOk());
    EXPECT_FALSE(result.malware_checked);
    EXPECT_FALSE(result.pii_checked);
    EXPECT_FALSE(result.abuse_checked);
}

TEST(ContentSecurityManagerTest, CheckContentWithoutMalwareFilter) {
    ContentSecurityConfig config;
    config.enable_malware_scan = true;  // Enabled but no filter set
    config.enable_pii_detection = false;
    
    ContentSecurityManager manager(config);
    // Not setting malware filter
    
    auto result = manager.checkContent("test data", "text/plain", "content-1");
    
    EXPECT_TRUE(result.error.isOk());
    EXPECT_TRUE(result.malware_checked);
    EXPECT_TRUE(result.malware_clean);  // No filter means clean
}

TEST(ContentSecurityManagerTest, CheckTextForPiiWithoutDetector) {
    ContentSecurityConfig config;
    config.enable_pii_detection = true;  // Enabled but no detector set
    
    ContentSecurityManager manager(config);
    // Not setting PII detector
    
    auto result = manager.checkTextForPii("test@example.com", "content-1");
    
    EXPECT_TRUE(result.error.isOk());
    EXPECT_TRUE(result.pii_checked);
    EXPECT_FALSE(result.pii_found);  // No detector means no PII found
}

// ============================================================================
// Error Sanitization Tests
// ============================================================================

TEST(ContentSecurityManagerTest, SanitizeErrorWithPaths) {
    ContentSecurityConfig config;
    config.sanitize_error_messages = true;
    config.hide_internal_paths = true;
    
    ContentSecurityManager manager(config);
    
    ContentError error = ContentError::error(
        ContentErrorCode::CONTENT_PROCESSING_FAILED,
        "Failed to process file at /home/user/data/secret.txt"
    );
    error.details = "Internal stack trace: at /opt/app/lib/processor.cpp:42";
    
    auto sanitized = manager.sanitizeError(error);
    
    EXPECT_NE(sanitized.message.find("[PATH]"), std::string::npos);
    EXPECT_TRUE(sanitized.details.empty());  // Details always removed
}

TEST(ContentSecurityManagerTest, SanitizeErrorWithSystemInfo) {
    ContentSecurityConfig config;
    config.sanitize_error_messages = true;
    config.hide_system_info = true;
    
    ContentSecurityManager manager(config);
    
    ContentError error = ContentError::error(
        ContentErrorCode::CONTENT_DEPENDENCY_ERROR,
        "Failed to connect to service at server.internal.company.com"
    );
    
    auto sanitized = manager.sanitizeError(error);
    
    EXPECT_NE(sanitized.message.find("[HOSTNAME]"), std::string::npos);
}

TEST(ContentSecurityManagerTest, SanitizeErrorDisabled) {
    ContentSecurityConfig config;
    config.sanitize_error_messages = false;
    
    ContentSecurityManager manager(config);
    
    ContentError error = ContentError::error(
        ContentErrorCode::CONTENT_PROCESSING_FAILED,
        "Failed to process file at /home/user/data/secret.txt"
    );
    error.details = "Internal stack trace";
    
    auto sanitized = manager.sanitizeError(error);
    
    EXPECT_EQ(sanitized.message, error.message);  // Not sanitized
    EXPECT_EQ(sanitized.details, error.details);  // Not removed
}

TEST(ContentSecurityManagerTest, SanitizeErrorMetadata) {
    ContentSecurityConfig config;
    config.sanitize_error_messages = true;
    
    ContentSecurityManager manager(config);
    
    ContentError error = ContentError::error(
        ContentErrorCode::CONTENT_SIZE_EXCEEDED,
        "Size exceeded"
    );
    error.metadata = {
        {"size", 1000000},
        {"mime_type", "application/pdf"},
        {"internal_path", "/secret/path"},
        {"api_key", "secret123"}
    };
    
    auto sanitized = manager.sanitizeError(error);
    
    // Only safe metadata should be kept
    EXPECT_TRUE(sanitized.metadata.contains("size"));
    EXPECT_TRUE(sanitized.metadata.contains("mime_type"));
    EXPECT_FALSE(sanitized.metadata.contains("internal_path"));
    EXPECT_FALSE(sanitized.metadata.contains("api_key"));
}

TEST(ContentSecurityManagerTest, SanitizeErrorMessage) {
    ContentSecurityConfig config;
    config.hide_internal_paths = true;
    config.hide_system_info = true;
    
    ContentSecurityManager manager(config);
    
    std::string message = "Error processing /usr/local/data/file.txt from user: admin";
    std::string sanitized = manager.sanitizeErrorMessage(message);
    
    EXPECT_NE(sanitized.find("[PATH]"), std::string::npos);
    EXPECT_NE(sanitized.find("[USERNAME]"), std::string::npos);
}

// ============================================================================
// Metrics Tests
// ============================================================================

TEST(ContentSecurityManagerTest, MetricsIncrement) {
    ContentSecurityConfig config;
    config.enable_malware_scan = false;
    config.enable_pii_detection = false;
    
    ContentSecurityManager manager(config);
    
    manager.checkContent("data1", "text/plain", "content-1");
    manager.checkContent("data2", "text/plain", "content-2");
    manager.checkContent("data3", "text/plain", "content-3");
    
    EXPECT_EQ(manager.getMetrics().total_checks.load(), 3);
}

TEST(ContentSecurityManagerTest, MetricsToJson) {
    ContentSecurityManager manager;
    
    json j = manager.getMetrics().toJson();
    
    EXPECT_TRUE(j.contains("total_checks"));
    EXPECT_TRUE(j.contains("malware_scans"));
    EXPECT_TRUE(j.contains("pii_scans"));
    EXPECT_TRUE(j.contains("abuse_scans"));
    EXPECT_TRUE(j.contains("errors_sanitized"));
    EXPECT_TRUE(j.contains("zip_bomb_scans"));
    EXPECT_TRUE(j.contains("zip_bomb_blocked"));
}

// ============================================================================
// Path Sanitization Tests
// ============================================================================

TEST(ContentSecurityManagerTest, SanitizeUnixPaths) {
    ContentSecurityConfig config;
    config.hide_internal_paths = true;
    
    ContentSecurityManager manager(config);
    
    std::string msg1 = "Error at /home/user/file.txt";
    EXPECT_NE(manager.sanitizeErrorMessage(msg1).find("[PATH]"), std::string::npos);
    
    std::string msg2 = "Failed: /var/log/app.log";
    EXPECT_NE(manager.sanitizeErrorMessage(msg2).find("[PATH]"), std::string::npos);
    
    std::string msg3 = "File ~/documents/secret.pdf";
    EXPECT_NE(manager.sanitizeErrorMessage(msg3).find("[PATH]"), std::string::npos);
    
    // Should NOT sanitize dates or fractions
    std::string msg4 = "Date: 2024/01/15";
    EXPECT_EQ(manager.sanitizeErrorMessage(msg4), msg4);
    
    std::string msg5 = "Fraction: 3/4 of files processed";
    EXPECT_EQ(manager.sanitizeErrorMessage(msg5), msg5);
}

TEST(ContentSecurityManagerTest, SanitizeWindowsPaths) {
    ContentSecurityConfig config;
    config.hide_internal_paths = true;
    
    ContentSecurityManager manager(config);
    
    std::string msg = "Error at C:\\Users\\Admin\\file.txt";
    EXPECT_NE(manager.sanitizeErrorMessage(msg).find("[PATH]"), std::string::npos);
}

TEST(ContentSecurityManagerTest, SanitizeHostnames) {
    ContentSecurityConfig config;
    config.hide_system_info = true;
    
    ContentSecurityManager manager(config);
    
    // Should sanitize hostnames in context
    std::string msg1 = "Failed to connect to server.internal.company.com";
    std::string sanitized1 = manager.sanitizeErrorMessage(msg1);
    EXPECT_NE(sanitized1.find("[HOSTNAME]"), std::string::npos);
    
    std::string msg2 = "Error from api.example.com";
    std::string sanitized2 = manager.sanitizeErrorMessage(msg2);
    EXPECT_NE(sanitized2.find("[HOSTNAME]"), std::string::npos);
    
    // Should NOT sanitize MIME types
    std::string msg3 = "Content type: application/json";
    std::string sanitized3 = manager.sanitizeErrorMessage(msg3);
    EXPECT_EQ(sanitized3.find("application/json"), 0);  // Should still contain mime type
    EXPECT_EQ(sanitized3.find("[HOSTNAME]"), std::string::npos);
}

TEST(ContentSecurityManagerTest, SanitizeUsername) {
    ContentSecurityConfig config;
    config.hide_system_info = true;
    
    ContentSecurityManager manager(config);
    
    std::string msg = "Operation failed for user: john_doe";
    std::string sanitized = manager.sanitizeErrorMessage(msg);
    
    EXPECT_NE(sanitized.find("[USERNAME]"), std::string::npos);
    EXPECT_EQ(sanitized.find("john_doe"), std::string::npos);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST(ContentSecurityManagerTest, EmptyContent) {
    ContentSecurityConfig config;
    config.enable_malware_scan = false;
    config.enable_pii_detection = false;
    
    ContentSecurityManager manager(config);
    
    auto result = manager.checkContent("", "text/plain", "content-empty");
    
    EXPECT_TRUE(result.error.isOk());
}

TEST(ContentSecurityManagerTest, LargeContent) {
    ContentSecurityConfig config;
    config.enable_malware_scan = false;
    config.enable_pii_detection = false;
    
    ContentSecurityManager manager(config);
    
    std::string large_data(10000000, 'x');  // 10MB
    auto result = manager.checkContent(large_data, "application/octet-stream", "content-large");
    
    EXPECT_TRUE(result.error.isOk());
}

TEST(ContentSecurityManagerTest, SpecialCharactersInErrorMessage) {
    ContentSecurityConfig config;
    config.sanitize_error_messages = true;
    
    ContentSecurityManager manager(config);
    
    ContentError error = ContentError::error(
        ContentErrorCode::CONTENT_CORRUPT,
        "Error: <tag> & \"quotes\" in /path/file"
    );
    
    auto sanitized = manager.sanitizeError(error);
    
    // Should still contain special chars but sanitize path
    EXPECT_NE(sanitized.message.find("<tag>"), std::string::npos);
    EXPECT_NE(sanitized.message.find("[PATH]"), std::string::npos);
}

// ============================================================================
// Zip-Bomb Protection Tests (CON-006)
// ============================================================================

class ZipBombProtectionTest : public ::testing::Test {
protected:
    // Default config has enable_zip_bomb_check=true, max_zip_bomb_ratio=100, max_zip_file_count=1000
    ContentSecurityConfig default_config_;
};

TEST_F(ZipBombProtectionTest, AllowsNormalArchive) {
    ContentSecurityManager manager(default_config_);
    // 100 bytes compressed -> 500 bytes uncompressed: ratio 5x, well under 100x
    auto result = manager.checkZipBomb(100, 500, 10, "archive.zip");
    EXPECT_FALSE(result.error.failed());
    EXPECT_TRUE(result.zip_bomb_checked);
    EXPECT_FALSE(result.zip_bomb_detected);
}

TEST_F(ZipBombProtectionTest, BlocksExcessiveCompressionRatio) {
    ContentSecurityManager manager(default_config_);
    // 1 byte compressed -> 200 bytes uncompressed: ratio 200x, exceeds 100x limit
    auto result = manager.checkZipBomb(1, 200, 1, "bomb.zip");
    EXPECT_TRUE(result.error.failed());
    EXPECT_TRUE(result.zip_bomb_checked);
    EXPECT_TRUE(result.zip_bomb_detected);
    EXPECT_EQ(result.error.code, ContentErrorCode::CONTENT_MALWARE_DETECTED);
    EXPECT_NE(result.error.message.find("compression ratio"), std::string::npos);
}

TEST_F(ZipBombProtectionTest, AllowsExactlyAtRatioLimit) {
    ContentSecurityManager manager(default_config_);
    // Ratio exactly 100x should pass (limit is strictly >, not >=)
    auto result = manager.checkZipBomb(1, 100, 1, "archive.zip");
    EXPECT_FALSE(result.error.failed());
    EXPECT_FALSE(result.zip_bomb_detected);
}

TEST_F(ZipBombProtectionTest, BlocksOneOverRatioLimit) {
    ContentSecurityManager manager(default_config_);
    // Ratio 101x must be rejected
    auto result = manager.checkZipBomb(1, 101, 1, "archive.zip");
    EXPECT_TRUE(result.error.failed());
    EXPECT_TRUE(result.zip_bomb_detected);
}

TEST_F(ZipBombProtectionTest, BlocksExcessiveFileCount) {
    ContentSecurityManager manager(default_config_);
    // 1001 files exceeds the 1000-file limit
    auto result = manager.checkZipBomb(1000, 2000, 1001, "archive.zip");
    EXPECT_TRUE(result.error.failed());
    EXPECT_TRUE(result.zip_bomb_detected);
    EXPECT_EQ(result.error.code, ContentErrorCode::CONTENT_SIZE_EXCEEDED);
    EXPECT_NE(result.error.message.find("file count"), std::string::npos);
}

TEST_F(ZipBombProtectionTest, AllowsExactlyMaxFileCount) {
    ContentSecurityManager manager(default_config_);
    // Exactly 1000 files must pass (limit is strictly >)
    auto result = manager.checkZipBomb(1000, 2000, 1000, "archive.zip");
    EXPECT_FALSE(result.error.failed());
    EXPECT_FALSE(result.zip_bomb_detected);
}

TEST_F(ZipBombProtectionTest, SkipsCheckWhenDisabled) {
    ContentSecurityConfig cfg;
    cfg.enable_zip_bomb_check = false;
    ContentSecurityManager manager(cfg);
    // Would fail both ratio and file-count limits, but check is disabled
    auto result = manager.checkZipBomb(1, 200, 2000, "bomb.zip");
    EXPECT_FALSE(result.error.failed());
    EXPECT_FALSE(result.zip_bomb_checked);   // check was skipped
    EXPECT_FALSE(result.zip_bomb_detected);
}

TEST_F(ZipBombProtectionTest, HandlesZeroCompressedSize) {
    ContentSecurityManager manager(default_config_);
    // Zero compressed size must not divide-by-zero; only file count is checked
    auto result = manager.checkZipBomb(0, 1000000, 5, "archive.zip");
    EXPECT_FALSE(result.error.failed());
}

TEST_F(ZipBombProtectionTest, RatioCheckUsesIntegerDivision) {
    ContentSecurityManager manager(default_config_);
    // 2 bytes compressed -> 201 bytes uncompressed: integer ratio = 100x, must pass
    auto result = manager.checkZipBomb(2, 201, 1, "archive.zip");
    EXPECT_FALSE(result.error.failed());
}

TEST_F(ZipBombProtectionTest, CustomRatioThreshold) {
    ContentSecurityConfig cfg;
    cfg.max_zip_bomb_ratio = 10;    // stricter: only 10x ratio allowed
    cfg.max_zip_file_count = 10000;
    ContentSecurityManager manager(cfg);

    // 11x ratio must be blocked
    auto r1 = manager.checkZipBomb(1, 11, 1, "archive.zip");
    EXPECT_TRUE(r1.error.failed());
    EXPECT_TRUE(r1.zip_bomb_detected);

    // 10x ratio must pass
    auto r2 = manager.checkZipBomb(1, 10, 1, "archive.zip");
    EXPECT_FALSE(r2.error.failed());
    EXPECT_FALSE(r2.zip_bomb_detected);
}

TEST_F(ZipBombProtectionTest, CustomFileCountThreshold) {
    ContentSecurityConfig cfg;
    cfg.max_zip_bomb_ratio = 10000; // lenient ratio
    cfg.max_zip_file_count = 5;     // strict file count
    ContentSecurityManager manager(cfg);

    // 6 files must be blocked
    auto r1 = manager.checkZipBomb(100, 200, 6, "archive.zip");
    EXPECT_TRUE(r1.error.failed());
    EXPECT_EQ(r1.error.code, ContentErrorCode::CONTENT_SIZE_EXCEEDED);

    // 5 files must pass
    auto r2 = manager.checkZipBomb(100, 200, 5, "archive.zip");
    EXPECT_FALSE(r2.error.failed());
}

TEST_F(ZipBombProtectionTest, ErrorIncludesRatioMetadata) {
    ContentSecurityManager manager(default_config_);
    auto result = manager.checkZipBomb(1, 200, 1, "bomb.zip");
    ASSERT_TRUE(result.error.failed());
    ASSERT_TRUE(result.error.metadata.contains("ratio"));
    ASSERT_TRUE(result.error.metadata.contains("max_ratio"));
    ASSERT_TRUE(result.error.metadata.contains("compressed_size"));
    ASSERT_TRUE(result.error.metadata.contains("uncompressed_size"));
    EXPECT_EQ(result.error.metadata["ratio"].get<uint64_t>(), 200u);
    EXPECT_EQ(result.error.metadata["max_ratio"].get<uint64_t>(), 100u);
    EXPECT_EQ(result.error.metadata["compressed_size"].get<uint64_t>(), 1u);
    EXPECT_EQ(result.error.metadata["uncompressed_size"].get<uint64_t>(), 200u);
}

TEST_F(ZipBombProtectionTest, ErrorIncludesFileCountMetadata) {
    ContentSecurityManager manager(default_config_);
    auto result = manager.checkZipBomb(1000, 2000, 1001, "bomb.zip");
    ASSERT_TRUE(result.error.failed());
    ASSERT_TRUE(result.error.metadata.contains("file_count"));
    ASSERT_TRUE(result.error.metadata.contains("max_file_count"));
    EXPECT_EQ(result.error.metadata["file_count"].get<size_t>(), 1001u);
    EXPECT_EQ(result.error.metadata["max_file_count"].get<size_t>(), 1000u);
}

TEST_F(ZipBombProtectionTest, ErrorContentIdSet) {
    ContentSecurityManager manager(default_config_);
    auto result = manager.checkZipBomb(1, 500, 1, "my_archive_id");
    ASSERT_TRUE(result.error.failed());
    EXPECT_EQ(result.error.content_id, "my_archive_id");
}

TEST_F(ZipBombProtectionTest, MetricsIncrementOnScan) {
    ContentSecurityManager manager(default_config_);
    manager.resetMetrics();
    manager.checkZipBomb(100, 500, 10, "archive.zip");
    EXPECT_EQ(manager.getMetrics().zip_bomb_scans.load(), 1u);
    EXPECT_EQ(manager.getMetrics().zip_bomb_blocked.load(), 0u);
}

TEST_F(ZipBombProtectionTest, MetricsIncrementOnBlock) {
    ContentSecurityManager manager(default_config_);
    manager.resetMetrics();
    manager.checkZipBomb(1, 200, 1, "bomb.zip");  // ratio 200x, blocked
    EXPECT_EQ(manager.getMetrics().zip_bomb_scans.load(), 1u);
    EXPECT_EQ(manager.getMetrics().zip_bomb_blocked.load(), 1u);
}

TEST_F(ZipBombProtectionTest, MetricsNotIncrementedWhenDisabled) {
    ContentSecurityConfig cfg;
    cfg.enable_zip_bomb_check = false;
    ContentSecurityManager manager(cfg);
    manager.resetMetrics();
    manager.checkZipBomb(1, 200, 2000, "bomb.zip");
    EXPECT_EQ(manager.getMetrics().zip_bomb_scans.load(), 0u);
    EXPECT_EQ(manager.getMetrics().zip_bomb_blocked.load(), 0u);
}

TEST_F(ZipBombProtectionTest, ResultToJsonContainsZipBombFields) {
    ContentSecurityManager manager(default_config_);
    auto result = manager.checkZipBomb(1, 200, 1, "bomb.zip");
    auto j = result.toJson();
    EXPECT_TRUE(j.contains("zip_bomb_checked"));
    EXPECT_TRUE(j.contains("zip_bomb_detected"));
    EXPECT_TRUE(j["zip_bomb_checked"].get<bool>());
    EXPECT_TRUE(j["zip_bomb_detected"].get<bool>());
}

TEST_F(ZipBombProtectionTest, ConfigJsonRoundTrip) {
    ContentSecurityConfig cfg1;
    cfg1.enable_zip_bomb_check = false;
    cfg1.max_zip_bomb_ratio = 50;
    cfg1.max_zip_file_count = 500;

    auto j = cfg1.toJson();
    EXPECT_TRUE(j.contains("enable_zip_bomb_check"));
    EXPECT_TRUE(j.contains("max_zip_bomb_ratio"));
    EXPECT_TRUE(j.contains("max_zip_file_count"));

    auto cfg2 = ContentSecurityConfig::fromJson(j);
    EXPECT_EQ(cfg2.enable_zip_bomb_check, false);
    EXPECT_EQ(cfg2.max_zip_bomb_ratio, 50u);
    EXPECT_EQ(cfg2.max_zip_file_count, 500u);
}

TEST_F(ZipBombProtectionTest, MultipleScansAccumulateMetrics) {
    ContentSecurityManager manager(default_config_);
    manager.resetMetrics();

    // Three clean scans
    manager.checkZipBomb(100, 500, 5, "a.zip");
    manager.checkZipBomb(200, 1000, 3, "b.zip");
    manager.checkZipBomb(50, 250, 8, "c.zip");
    // One blocked scan (ratio 200x)
    manager.checkZipBomb(1, 200, 1, "bomb.zip");

    EXPECT_EQ(manager.getMetrics().zip_bomb_scans.load(), 4u);
    EXPECT_EQ(manager.getMetrics().zip_bomb_blocked.load(), 1u);
}
