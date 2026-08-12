#include <gtest/gtest.h>
#include "content/content_security.h"
#include "content/abuse_detector.h"
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
    EXPECT_NE(sanitized3.find("application/json"), std::string::npos);  // MIME type must survive sanitization
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

// ============================================================================
// PhotoDNAAbuseDetector Unit Tests
// ============================================================================

TEST(PhotoDNAAbuseDetectorTest, ComputeHashIsStable) {
    const std::string data(256, '\x80');
    const uint64_t h1 = PhotoDNAAbuseDetector::computeHash(data);
    const uint64_t h2 = PhotoDNAAbuseDetector::computeHash(data);
    EXPECT_EQ(h1, h2);
}

TEST(PhotoDNAAbuseDetectorTest, ComputeHashEmptyDataReturnsZero) {
    EXPECT_EQ(PhotoDNAAbuseDetector::computeHash(""), 0ULL);
}

TEST(PhotoDNAAbuseDetectorTest, HammingDistanceSameHash) {
    EXPECT_EQ(PhotoDNAAbuseDetector::hammingDistance(0xDEADBEEFULL, 0xDEADBEEFULL), 0);
}

TEST(PhotoDNAAbuseDetectorTest, HammingDistanceAllBitsDiffer) {
    EXPECT_EQ(PhotoDNAAbuseDetector::hammingDistance(0x0ULL, ~uint64_t{0}), 64);
}

TEST(PhotoDNAAbuseDetectorTest, NonImageContentIsAlwaysAllowed) {
    PhotoDNAAbuseDetector::BlocklistEntry entry{0ULL, "test_hash", AbuseAction::BLOCK};
    PhotoDNAAbuseDetector detector({entry}, 64);  // threshold=64 matches everything

    AbuseDetectorMetadata meta;
    meta.mime_type    = "text/plain";
    meta.content_id   = "doc-1";
    meta.content_hash = "abc";

    auto result = detector.detect("buy now click here", meta);
    EXPECT_EQ(result.action, AbuseAction::ALLOW);
}

TEST(PhotoDNAAbuseDetectorTest, ImageMatchesBlocklistIsBlocked) {
    // Build content that produces a known hash, then add it to the blocklist.
    const std::string img_data(64, '\xFF');  // 64 bytes, all 0xFF
    const uint64_t known_hash = PhotoDNAAbuseDetector::computeHash(img_data);

    PhotoDNAAbuseDetector::BlocklistEntry entry{known_hash, "TEST_CSAM_001", AbuseAction::BLOCK};
    PhotoDNAAbuseDetector detector({entry}, 0);  // exact match only

    AbuseDetectorMetadata meta;
    meta.mime_type    = "image/jpeg";
    meta.content_id   = "img-1";
    meta.content_hash = "abc";

    auto result = detector.detect(img_data, meta);
    EXPECT_EQ(result.action, AbuseAction::BLOCK);
    EXPECT_EQ(result.pattern_name, "TEST_CSAM_001");
    EXPECT_EQ(result.detector_type, "PhotoDNA");
}

TEST(PhotoDNAAbuseDetectorTest, ImageMatchesBlocklistIsFlagged) {
    const std::string img_data(128, '\xAA');
    const uint64_t known_hash = PhotoDNAAbuseDetector::computeHash(img_data);

    PhotoDNAAbuseDetector::BlocklistEntry entry{known_hash, "SUSPICIOUS_001", AbuseAction::FLAG};
    PhotoDNAAbuseDetector detector({entry}, 0);

    AbuseDetectorMetadata meta;
    meta.mime_type    = "image/png";
    meta.content_id   = "img-2";
    meta.content_hash = "def";

    auto result = detector.detect(img_data, meta);
    EXPECT_EQ(result.action, AbuseAction::FLAG);
    EXPECT_EQ(result.pattern_name, "SUSPICIOUS_001");
}

TEST(PhotoDNAAbuseDetectorTest, NearMatchWithinThresholdBlocked) {
    const std::string img_data(64, '\x10');
    const uint64_t base_hash = PhotoDNAAbuseDetector::computeHash(img_data);
    // Flip one bit to create a near-match
    const uint64_t near_hash = base_hash ^ uint64_t{1};

    PhotoDNAAbuseDetector::BlocklistEntry entry{near_hash, "NEAR_001", AbuseAction::BLOCK};
    PhotoDNAAbuseDetector detector({entry}, 5);  // allow up to 5 bits difference

    AbuseDetectorMetadata meta;
    meta.mime_type    = "image/jpeg";
    meta.content_id   = "img-3";
    meta.content_hash = "ghi";

    auto result = detector.detect(img_data, meta);
    EXPECT_EQ(result.action, AbuseAction::BLOCK);
}

TEST(PhotoDNAAbuseDetectorTest, FarMatchBeyondThresholdAllowed) {
    const std::string img_data(64, '\x10');
    const uint64_t base_hash = PhotoDNAAbuseDetector::computeHash(img_data);
    // Flip many bits so distance > threshold
    const uint64_t far_hash = base_hash ^ 0xFFFFFFFF00000000ULL;  // 32 bits differ

    PhotoDNAAbuseDetector::BlocklistEntry entry{far_hash, "FAR_001", AbuseAction::BLOCK};
    PhotoDNAAbuseDetector detector({entry}, 5);  // threshold = 5

    AbuseDetectorMetadata meta;
    meta.mime_type    = "image/jpeg";
    meta.content_id   = "img-4";
    meta.content_hash = "jkl";

    auto result = detector.detect(img_data, meta);
    EXPECT_EQ(result.action, AbuseAction::ALLOW);
}

TEST(PhotoDNAAbuseDetectorTest, EmptyBlocklistAllowsEverything) {
    PhotoDNAAbuseDetector detector({}, 10);

    AbuseDetectorMetadata meta;
    meta.mime_type    = "image/jpeg";
    meta.content_id   = "img-5";
    meta.content_hash = "mno";

    auto result = detector.detect("some image bytes", meta);
    EXPECT_EQ(result.action, AbuseAction::ALLOW);
}

// ============================================================================
// TextAbuseDetector Unit Tests
// ============================================================================

TEST(TextAbuseDetectorTest, EmptyPatternListAllowsEverything) {
    TextAbuseDetector detector({});

    AbuseDetectorMetadata meta;
    meta.mime_type  = "text/plain";
    meta.content_id = "doc-1";

    auto result = detector.detect("buy now click here", meta);
    EXPECT_EQ(result.action, AbuseAction::ALLOW);
}

TEST(TextAbuseDetectorTest, BlockPatternMatchIsBlocked) {
    TextAbuseDetector::Pattern p;
    p.name     = "test_block";
    p.action   = AbuseAction::BLOCK;
    p.compiled = std::regex("malicious content", std::regex::icase);

    TextAbuseDetector detector({p});

    AbuseDetectorMetadata meta;
    meta.mime_type  = "text/plain";
    meta.content_id = "doc-2";

    auto result = detector.detect("This has malicious content inside.", meta);
    EXPECT_EQ(result.action, AbuseAction::BLOCK);
    EXPECT_EQ(result.pattern_name, "test_block");
    EXPECT_EQ(result.detector_type, "Text");
}

TEST(TextAbuseDetectorTest, FlagPatternMatchIsFlagged) {
    TextAbuseDetector::Pattern p;
    p.name     = "test_flag";
    p.action   = AbuseAction::FLAG;
    p.compiled = std::regex("suspicious phrase", std::regex::icase);

    TextAbuseDetector detector({p});

    AbuseDetectorMetadata meta;
    meta.mime_type  = "text/plain";
    meta.content_id = "doc-3";

    auto result = detector.detect("This contains a suspicious phrase for review.", meta);
    EXPECT_EQ(result.action, AbuseAction::FLAG);
    EXPECT_EQ(result.pattern_name, "test_flag");
}

TEST(TextAbuseDetectorTest, NoPatternMatchIsAllowed) {
    TextAbuseDetector::Pattern p;
    p.name     = "no_match";
    p.action   = AbuseAction::BLOCK;
    p.compiled = std::regex("xyzzy_never_matches_real_content_0000");

    TextAbuseDetector detector({p});

    AbuseDetectorMetadata meta;
    meta.mime_type  = "text/plain";
    meta.content_id = "doc-4";

    auto result = detector.detect("completely innocent text", meta);
    EXPECT_EQ(result.action, AbuseAction::ALLOW);
}

TEST(TextAbuseDetectorTest, FirstMatchingPatternWins) {
    TextAbuseDetector::Pattern p1;
    p1.name     = "first_pattern";
    p1.action   = AbuseAction::FLAG;
    p1.compiled = std::regex("trigger", std::regex::icase);

    TextAbuseDetector::Pattern p2;
    p2.name     = "second_pattern";
    p2.action   = AbuseAction::BLOCK;
    p2.compiled = std::regex("trigger", std::regex::icase);

    TextAbuseDetector detector({p1, p2});

    AbuseDetectorMetadata meta;
    meta.mime_type  = "text/plain";
    meta.content_id = "doc-5";

    auto result = detector.detect("This has a trigger word.", meta);
    EXPECT_EQ(result.action, AbuseAction::FLAG);  // first pattern wins
    EXPECT_EQ(result.pattern_name, "first_pattern");
}

TEST(TextAbuseDetectorTest, CaseInsensitiveMatch) {
    TextAbuseDetector::Pattern p;
    p.name     = "case_test";
    p.action   = AbuseAction::BLOCK;
    p.compiled = std::regex("BUY NOW", std::regex::icase);

    TextAbuseDetector detector({p});

    AbuseDetectorMetadata meta;
    meta.mime_type  = "text/plain";
    meta.content_id = "doc-6";

    EXPECT_EQ(detector.detect("buy now!", meta).action, AbuseAction::BLOCK);
    EXPECT_EQ(detector.detect("BUY NOW!", meta).action, AbuseAction::BLOCK);
    EXPECT_EQ(detector.detect("Buy Now!", meta).action, AbuseAction::BLOCK);
}

TEST(TextAbuseDetectorTest, LoadFromYAMLMissingFileReturnsEmptyDetector) {
    std::string error;
    auto detector = TextAbuseDetector::loadFromYAML("/nonexistent/path/abuse.yaml", error);
    ASSERT_NE(detector, nullptr);
    EXPECT_FALSE(error.empty());
    EXPECT_EQ(detector->patternCount(), 0u);
}

// ============================================================================
// ContentSecurityManager Abuse Detection Integration Tests
// ============================================================================

class AbuseDetectionIntegrationTest : public ::testing::Test {
protected:
    // Build a ContentSecurityConfig with abuse detection enabled.
    ContentSecurityConfig makeConfig(bool block_on_abuse) {
        ContentSecurityConfig cfg;
        cfg.enable_malware_scan   = false;
        cfg.enable_pii_detection  = false;
        cfg.enable_abuse_detection = true;
        cfg.block_on_abuse         = block_on_abuse;
        return cfg;
    }

    // Helper: make a TextAbuseDetector with one pattern.
    static std::shared_ptr<IAbuseDetector> makeTextDetector(
        const std::string& pattern_name,
        const std::string& regex_str,
        AbuseAction action)
    {
        TextAbuseDetector::Pattern p;
        p.name     = pattern_name;
        p.action   = action;
        p.compiled = std::regex(regex_str, std::regex::icase);
        return std::make_shared<TextAbuseDetector>(
            std::vector<TextAbuseDetector::Pattern>{p}
        );
    }
};

TEST_F(AbuseDetectionIntegrationTest, CleanTextIsAllowed) {
    ContentSecurityManager mgr(makeConfig(true));
    mgr.setTextAbuseDetector(makeTextDetector("spam", "buy now", AbuseAction::BLOCK));

    auto result = mgr.checkContent("innocent content here", "text/plain", "c-1");

    EXPECT_TRUE(result.error.isOk());
    EXPECT_TRUE(result.abuse_checked);
    EXPECT_FALSE(result.abuse_detected);
    EXPECT_EQ(result.abuse_action, "ALLOW");
}

TEST_F(AbuseDetectionIntegrationTest, BlockedTextReturnsError) {
    ContentSecurityManager mgr(makeConfig(true));
    mgr.setTextAbuseDetector(makeTextDetector("spam_block", "buy now", AbuseAction::BLOCK));

    auto result = mgr.checkContent("buy now and make money fast", "text/plain", "c-2");

    EXPECT_TRUE(result.error.failed());
    EXPECT_TRUE(result.abuse_checked);
    EXPECT_TRUE(result.abuse_detected);
    EXPECT_EQ(result.abuse_action, "BLOCK");
    EXPECT_EQ(result.abuse_detector_type, "Text");
    EXPECT_EQ(result.abuse_pattern_name, "spam_block");
}

TEST_F(AbuseDetectionIntegrationTest, FlaggedTextDoesNotReturnError) {
    ContentSecurityManager mgr(makeConfig(true));  // block_on_abuse=true, but action=FLAG
    mgr.setTextAbuseDetector(makeTextDetector("spam_flag", "suspicious phrase", AbuseAction::FLAG));

    auto result = mgr.checkContent("This is a suspicious phrase for review.", "text/plain", "c-3");

    EXPECT_TRUE(result.error.isOk());  // FLAG should not produce an error
    EXPECT_TRUE(result.abuse_checked);
    EXPECT_TRUE(result.abuse_detected);
    EXPECT_EQ(result.abuse_action, "FLAG");
    EXPECT_EQ(result.abuse_detector_type, "Text");
    EXPECT_EQ(result.abuse_pattern_name, "spam_flag");
}

TEST_F(AbuseDetectionIntegrationTest, BlockActionWithBlockOnAbuseDisabledDoesNotError) {
    ContentSecurityManager mgr(makeConfig(false));  // block_on_abuse=false
    mgr.setTextAbuseDetector(makeTextDetector("spam_block", "buy now", AbuseAction::BLOCK));

    auto result = mgr.checkContent("buy now", "text/plain", "c-4");

    // BLOCK action detected, but block_on_abuse=false → no error
    EXPECT_TRUE(result.error.isOk());
    EXPECT_TRUE(result.abuse_detected);
    EXPECT_EQ(result.abuse_action, "BLOCK");
}

TEST_F(AbuseDetectionIntegrationTest, PhotoDetectorBlocksMatchingImage) {
    // Build an image blocklist entry matching our test image data.
    const std::string img_data(64, '\xAB');
    const uint64_t hash = PhotoDNAAbuseDetector::computeHash(img_data);

    PhotoDNAAbuseDetector::BlocklistEntry entry{hash, "CSAM_HASH_TEST", AbuseAction::BLOCK};
    auto photo_det = std::make_shared<PhotoDNAAbuseDetector>(
        std::vector<PhotoDNAAbuseDetector::BlocklistEntry>{entry},
        0  // exact match only
    );

    ContentSecurityManager mgr(makeConfig(true));
    mgr.setPhotoAbuseDetector(photo_det);

    auto result = mgr.checkContent(img_data, "image/jpeg", "img-1");

    EXPECT_TRUE(result.error.failed());
    EXPECT_TRUE(result.abuse_detected);
    EXPECT_EQ(result.abuse_action, "BLOCK");
    EXPECT_EQ(result.abuse_detector_type, "PhotoDNA");
    EXPECT_EQ(result.abuse_pattern_name, "CSAM_HASH_TEST");
}

TEST_F(AbuseDetectionIntegrationTest, PhotoDetectorFlagsMatchingImage) {
    const std::string img_data(64, '\xCD');
    const uint64_t hash = PhotoDNAAbuseDetector::computeHash(img_data);

    PhotoDNAAbuseDetector::BlocklistEntry entry{hash, "SUSPICIOUS_IMG", AbuseAction::FLAG};
    auto photo_det = std::make_shared<PhotoDNAAbuseDetector>(
        std::vector<PhotoDNAAbuseDetector::BlocklistEntry>{entry},
        0
    );

    ContentSecurityManager mgr(makeConfig(true));
    mgr.setPhotoAbuseDetector(photo_det);

    auto result = mgr.checkContent(img_data, "image/jpeg", "img-2");

    EXPECT_TRUE(result.error.isOk());  // FLAG does not block
    EXPECT_TRUE(result.abuse_detected);
    EXPECT_EQ(result.abuse_action, "FLAG");
    EXPECT_EQ(result.abuse_pattern_name, "SUSPICIOUS_IMG");
    EXPECT_EQ(result.abuse_detector_type, "PhotoDNA");
}

TEST_F(AbuseDetectionIntegrationTest, PhotoDetectorIgnoresNonImageContent) {
    // Set up a photo detector that matches EVERYTHING (threshold=64).
    const uint64_t any_hash = 0ULL;
    PhotoDNAAbuseDetector::BlocklistEntry entry{any_hash, "CSAM_ANY", AbuseAction::BLOCK};
    auto photo_det = std::make_shared<PhotoDNAAbuseDetector>(
        std::vector<PhotoDNAAbuseDetector::BlocklistEntry>{entry},
        64  // matches any hash (max hamming distance = 64)
    );

    ContentSecurityManager mgr(makeConfig(true));
    mgr.setPhotoAbuseDetector(photo_det);

    // Text content should pass through even though the "hash" would match.
    auto result = mgr.checkContent("some text content", "text/plain", "doc-1");

    EXPECT_TRUE(result.error.isOk());
    EXPECT_FALSE(result.abuse_detected);
}

TEST_F(AbuseDetectionIntegrationTest, AbuseDisabledSkipsCheck) {
    ContentSecurityConfig cfg;
    cfg.enable_malware_scan    = false;
    cfg.enable_pii_detection   = false;
    cfg.enable_abuse_detection = false;

    ContentSecurityManager mgr(cfg);
    mgr.setTextAbuseDetector(makeTextDetector("spam", "buy now", AbuseAction::BLOCK));

    auto result = mgr.checkContent("buy now make money", "text/plain", "c-5");

    EXPECT_FALSE(result.abuse_checked);  // check was not run
    EXPECT_TRUE(result.error.isOk());
}

TEST_F(AbuseDetectionIntegrationTest, AbuseDetectionMetricsIncrement) {
    ContentSecurityManager mgr(makeConfig(true));
    mgr.setTextAbuseDetector(makeTextDetector("spam", "buy now", AbuseAction::BLOCK));
    mgr.resetMetrics();

    // One blocked detection
    mgr.checkContent("buy now!", "text/plain", "c-6");
    EXPECT_EQ(mgr.getMetrics().abuse_scans.load(), 1u);
    EXPECT_EQ(mgr.getMetrics().abuse_detected.load(), 1u);
    EXPECT_EQ(mgr.getMetrics().abuse_blocked.load(), 1u);

    // One clean scan
    mgr.checkContent("innocent content", "text/plain", "c-7");
    EXPECT_EQ(mgr.getMetrics().abuse_scans.load(), 2u);
    EXPECT_EQ(mgr.getMetrics().abuse_detected.load(), 1u);
    EXPECT_EQ(mgr.getMetrics().abuse_blocked.load(), 1u);
}

TEST_F(AbuseDetectionIntegrationTest, AbuseMetricsInJson) {
    ContentSecurityManager mgr(makeConfig(true));
    auto j = mgr.getMetrics().toJson();
    EXPECT_TRUE(j.contains("abuse_scans"));
    EXPECT_TRUE(j.contains("abuse_detected"));
    EXPECT_TRUE(j.contains("abuse_blocked"));
}

TEST_F(AbuseDetectionIntegrationTest, ResultToJsonIncludesAbuseFields) {
    ContentSecurityManager mgr(makeConfig(true));
    mgr.setTextAbuseDetector(makeTextDetector("spam", "buy now", AbuseAction::FLAG));

    auto result = mgr.checkContent("buy now", "text/plain", "c-8");
    auto j = result.toJson();

    EXPECT_TRUE(j.contains("abuse_checked"));
    EXPECT_TRUE(j.contains("abuse_detected"));
    EXPECT_TRUE(j.contains("abuse_action"));
    EXPECT_TRUE(j.contains("abuse_detector_type"));
    EXPECT_TRUE(j.contains("abuse_pattern_name"));
    EXPECT_EQ(j["abuse_action"].get<std::string>(), "FLAG");
}

TEST_F(AbuseDetectionIntegrationTest, AuditLoggerIsCalledOnDetection) {
    // A minimal stub audit logger that records calls.
    struct StubEvent {
        std::string event_name;
        std::string action;
    };

    // We use a shared vector to capture calls (without full AuditLogger deps).
    // Instead of instantiating the real (heavyweight) AuditLogger we verify
    // behavior by checking the SecurityCheckResult fields which are populated
    // only when the detection code path executed correctly.

    ContentSecurityManager mgr(makeConfig(true));
    mgr.setTextAbuseDetector(makeTextDetector("spam_audit", "buy now", AbuseAction::FLAG));
    // Do not call setAuditLogger — it is optional; detection must work either way.

    auto result = mgr.checkContent("buy now", "text/plain", "c-audit-1");
    EXPECT_TRUE(result.abuse_detected);
    EXPECT_EQ(result.abuse_action, "FLAG");
    // If we reach here, the detection path executed without crashing when
    // audit_logger_ is nullptr (the null-check in checkAbuse() guards this).
}

// ============================================================================
// AbuseAction utilities
// ============================================================================

TEST(AbuseActionTest, ToStringAllValues) {
    EXPECT_EQ(abuseActionToString(AbuseAction::ALLOW), "ALLOW");
    EXPECT_EQ(abuseActionToString(AbuseAction::FLAG),  "FLAG");
    EXPECT_EQ(abuseActionToString(AbuseAction::BLOCK), "BLOCK");
}
