/**
 * @file test_c5_key_lifecycle_audit_evidence.cpp
 * @brief Verify a production key-rotation operation is persisted by AuditLogger.
 */

#include <gtest/gtest.h>

#include "auth/jwt_key_rotation_manager.h"
#include "auth/jwt_validator.h"
#include "utils/audit_logger.h"

#include <nlohmann/json.hpp>
#include <openssl/evp.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace themis::auth;
using namespace themis::utils;

TEST(C5KeyLifecycleAuditEvidence, ProductionRotationPersistsAuditRecord) {
    const char* configured_path = std::getenv("THEMIS_C5_KEY_LIFECYCLE_AUDIT_PATH");
    const bool retain_evidence = configured_path != nullptr && configured_path[0] != '\0';
    const fs::path log_path = retain_evidence
        ? fs::path(configured_path)
        : fs::temp_directory_path() /
              ("themis-c5-key-lifecycle-" +
               std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) +
               ".jsonl");
    std::error_code ec;
    fs::remove(log_path, ec);
    ASSERT_FALSE(fs::exists(log_path))
        << "Could not clear previous audit evidence at " << log_path.string();
    fs::create_directories(log_path.parent_path());

    AuditLoggerConfig audit_config;
    audit_config.enabled = true;
    audit_config.encrypt_then_sign = false;
    audit_config.enable_hash_chain = false;
    audit_config.enable_fsync = true;
    audit_config.log_path = log_path.string();

    JWTValidatorConfig validator_config;
    validator_config.jwks_url = "http://localhost:8080/jwks";
    validator_config.require_issuer_validation = false;
    validator_config.require_audience_validation = false;

    JWTValidator validator(validator_config);
    AuditLogger logger(nullptr, nullptr, audit_config);
    JWTKeyRotationManager key_manager(validator);
    key_manager.setAuditLogger(&logger);

    // This is the real key-manager operation; the test does not inject events
    // directly into AuditLogger.
    key_manager.rotateActiveKey("c5-evidence-key");

    ASSERT_TRUE(fs::exists(log_path));
    ASSERT_GT(fs::file_size(log_path), 0u);

    std::ifstream audit_file(log_path);
    ASSERT_TRUE(audit_file.is_open());
    std::string line;
    ASSERT_TRUE(std::getline(audit_file, line));
    ASSERT_FALSE(line.empty());

    const auto persisted_record = nlohmann::json::parse(line);
    ASSERT_TRUE(persisted_record.contains("payload"));
    ASSERT_TRUE(persisted_record["payload"].contains("data_b64"));

    const std::string encoded =
        persisted_record["payload"]["data_b64"].get<std::string>();
    ASSERT_FALSE(encoded.empty());
    ASSERT_EQ(encoded.size() % 4, 0u);

    std::vector<unsigned char> decoded((encoded.size() / 4) * 3);
    const int decoded_size = EVP_DecodeBlock(
        decoded.data(),
        reinterpret_cast<const unsigned char*>(encoded.data()),
        static_cast<int>(encoded.size()));
    ASSERT_GE(decoded_size, 0);

    std::size_t padding = 0;
    if (encoded.ends_with("==")) {
        padding = 2;
    } else if (encoded.ends_with("=")) {
        padding = 1;
    }
    ASSERT_GE(static_cast<std::size_t>(decoded_size), padding);
    decoded.resize(static_cast<std::size_t>(decoded_size) - padding);

    const auto event = nlohmann::json::parse(
        std::string(decoded.begin(), decoded.end()));
    EXPECT_EQ(event.value("event_type", ""), "KEY_ROTATED");
    EXPECT_EQ(event.value("resource", ""), "jwt_key/c5-evidence-key");
    ASSERT_TRUE(event.contains("details"));
    EXPECT_EQ(event["details"].value("new_kid", ""), "c5-evidence-key");

    if (!retain_evidence) {
        fs::remove(log_path, ec);
    }
}
