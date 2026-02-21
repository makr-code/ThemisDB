/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_content_security.cpp                          ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-02-21 07:18:15                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     444                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
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
