#include <gtest/gtest.h>

#include "utils/audit_logger.h"
#include "security/mock_key_provider.h"
#include "themis/edition.h"

#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>

using namespace themis;
using namespace themis::utils;

class AuditLoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        key_provider_ = std::make_shared<MockKeyProvider>();
        // Create default key for saga_log
        key_provider_->createKey("saga_log", 1);
        
        enc_ = std::make_shared<FieldEncryption>(key_provider_);
        
        PKIConfig pki_cfg;
        pki_cfg.service_id = "test";
        pki_ = std::make_shared<VCCPKIClient>(pki_cfg);
        
        log_path_ = "data/logs/test_audit.jsonl";
        std::filesystem::remove(log_path_);
    }
    
    void TearDown() override {
        std::filesystem::remove(log_path_);
    }
    
    std::shared_ptr<MockKeyProvider> key_provider_;
    std::shared_ptr<FieldEncryption> enc_;
    std::shared_ptr<VCCPKIClient> pki_;
    std::string log_path_;
};

TEST_F(AuditLoggerTest, EncryptThenSignFlow) {
    if (!themis::edition::IsFeatureEnabled("field_encryption")) {
        GTEST_SKIP() << "field_encryption feature is unavailable in this edition";
    }

    AuditLoggerConfig cfg;
    cfg.enabled = true;
    cfg.encrypt_then_sign = true;
    cfg.log_path = log_path_;
    cfg.key_id = "saga_log";
    
    AuditLogger logger(enc_, pki_, cfg);
    
    nlohmann::json event = {
        {"user", "admin"},
        {"action", "read"},
        {"resource", "/content/doc123"},
        {"result", "success"}
    };
    
    logger.logEvent(event);
    
    // Verify log file exists and has encrypted payload
    ASSERT_TRUE(std::filesystem::exists(log_path_));
    
    std::ifstream ifs(log_path_);
    std::string line;
    ASSERT_TRUE(std::getline(ifs, line));
    
    auto record = nlohmann::json::parse(line);
    EXPECT_TRUE(record.contains("ts"));
    EXPECT_EQ(record["category"], "AUDIT");
    EXPECT_EQ(record["payload"]["type"], "ciphertext");
    EXPECT_TRUE(record["payload"].contains("iv_b64"));
    EXPECT_TRUE(record["payload"].contains("ciphertext_b64"));
    EXPECT_TRUE(record["payload"].contains("tag_b64"));
    EXPECT_TRUE(record["signature"]["ok"]);
    EXPECT_FALSE(record["signature"]["id"].get<std::string>().empty());
}

TEST_F(AuditLoggerTest, PlaintextSignFlow) {
    AuditLoggerConfig cfg;
    cfg.enabled = true;
    cfg.encrypt_then_sign = false; // disable encryption
    cfg.log_path = log_path_;
    
    AuditLogger logger(enc_, pki_, cfg);
    
    nlohmann::json event = {
        {"user", "user1"},
        {"action", "write"},
        {"resource", "/data/file.txt"}
    };
    
    logger.logEvent(event);
    
    // Verify log file exists with plaintext payload
    ASSERT_TRUE(std::filesystem::exists(log_path_));
    
    std::ifstream ifs(log_path_);
    std::string line;
    ASSERT_TRUE(std::getline(ifs, line));
    
    auto record = nlohmann::json::parse(line);
    EXPECT_EQ(record["payload"]["type"], "plaintext");
    EXPECT_TRUE(record["payload"].contains("data_b64"));
    ASSERT_TRUE(record.contains("signature"));
    EXPECT_TRUE(record["signature"].contains("ok"));
    EXPECT_TRUE(record["signature"]["ok"].is_boolean());
}

TEST_F(AuditLoggerTest, DisabledLogger) {
    AuditLoggerConfig cfg;
    cfg.enabled = false;
    cfg.log_path = log_path_;
    
    AuditLogger logger(enc_, pki_, cfg);
    
    nlohmann::json event = {{"action", "test"}};
    logger.logEvent(event);
    
    // No file should be created
    EXPECT_FALSE(std::filesystem::exists(log_path_));
}

TEST_F(AuditLoggerTest, MultipleEvents) {
    if (!themis::edition::IsFeatureEnabled("field_encryption")) {
        GTEST_SKIP() << "field_encryption feature is unavailable in this edition";
    }

    AuditLoggerConfig cfg;
    cfg.enabled = true;
    cfg.encrypt_then_sign = true;
    cfg.log_path = log_path_;
    cfg.key_id = "saga_log";
    
    AuditLogger logger(enc_, pki_, cfg);
    
    for (int i = 0; i < 5; ++i) {
        nlohmann::json event = {
            {"event_id", i},
            {"action", "test_action"}
        };
        logger.logEvent(event);
    }
    
    // Verify 5 lines in log file
    std::ifstream ifs(log_path_);
    int count = 0;
    std::string line;
    while (std::getline(ifs, line)) {
        auto record = nlohmann::json::parse(line);
        EXPECT_TRUE(record.contains("ts"));
        EXPECT_EQ(record["category"], "AUDIT");
        ++count;
    }
    EXPECT_EQ(count, 5);
}

// ============================================================================
// Retention Tests
// ============================================================================

TEST_F(AuditLoggerTest, EnumerateEntries) {
    AuditLoggerConfig cfg;
    cfg.enabled = true;
    cfg.encrypt_then_sign = false;  // Use plaintext for easier testing
    cfg.log_path = log_path_;
    
    AuditLogger logger(enc_, pki_, cfg);
    
    // Log some events with known timestamps
    for (int i = 0; i < 3; ++i) {
        nlohmann::json event = {
            {"event_id", i},
            {"action", "test_enumerate"}
        };
        logger.logEvent(event);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Enumerate entries
    auto entries = logger.enumerateEntries();
    
    EXPECT_EQ(entries.size(), 3);
    for (size_t i = 0; i < entries.size(); ++i) {
        EXPECT_EQ(entries[i].entry_number, i);
        EXPECT_TRUE(entries[i].record.contains("ts"));
    }
}

TEST_F(AuditLoggerTest, ArchiveOldEntries) {
    AuditLoggerConfig cfg;
    cfg.enabled = true;
    cfg.encrypt_then_sign = false;
    cfg.log_path = log_path_;
    
    AuditLogger logger(enc_, pki_, cfg);
    
    // Log events with different timestamps
    auto now = std::chrono::system_clock::now();
    auto old_ts = now - std::chrono::hours(24 * 365 * 6); // 6 years old
    auto recent_ts = now - std::chrono::hours(24 * 30);   // 1 month old
    
    // Manually create log entries with specific timestamps
    {
        std::ofstream ofs(log_path_);
        
        // Old entry (should be archived)
        nlohmann::json old_record = {
            {"ts", std::chrono::duration_cast<std::chrono::milliseconds>(
                old_ts.time_since_epoch()).count()},
            {"category", "AUDIT"},
            {"payload", {{"type", "plaintext"}, {"data", "old_event"}}}
        };
        ofs << old_record.dump() << "\n";
        
        // Recent entry (should be kept)
        nlohmann::json recent_record = {
            {"ts", std::chrono::duration_cast<std::chrono::milliseconds>(
                recent_ts.time_since_epoch()).count()},
            {"category", "AUDIT"},
            {"payload", {{"type", "plaintext"}, {"data", "recent_event"}}}
        };
        ofs << recent_record.dump() << "\n";
    }
    
    // Archive entries older than 5 years
    auto archive_threshold = now - std::chrono::hours(24 * 365 * 5);
    std::string archive_path = "data/logs/test_audit_archive.jsonl";
    std::filesystem::remove(archive_path);
    
    size_t archived = logger.archiveOldEntries(archive_threshold, archive_path);
    
    EXPECT_EQ(archived, 1);
    
    // Verify archive file exists and contains the old entry
    ASSERT_TRUE(std::filesystem::exists(archive_path));
    std::string line;
    {
        std::ifstream archive_ifs(archive_path);
        ASSERT_TRUE(std::getline(archive_ifs, line));
        auto archived_record = nlohmann::json::parse(line);
        EXPECT_EQ(archived_record["payload"]["data"], "old_event");
    }
    
    // Verify main log only contains recent entry
    int main_count = 0;
    {
        std::ifstream main_ifs(log_path_);
        while (std::getline(main_ifs, line)) {
            if (!line.empty()) {
                auto record = nlohmann::json::parse(line);
                EXPECT_EQ(record["payload"]["data"], "recent_event");
                ++main_count;
            }
        }
    }
    EXPECT_EQ(main_count, 1);
    
    std::filesystem::remove(archive_path);
}

TEST_F(AuditLoggerTest, PurgeOldEntries) {
    AuditLoggerConfig cfg;
    cfg.enabled = true;
    cfg.encrypt_then_sign = false;
    cfg.log_path = log_path_;
    
    AuditLogger logger(enc_, pki_, cfg);
    
    // Log events with different timestamps
    auto now = std::chrono::system_clock::now();
    auto very_old_ts = now - std::chrono::hours(24 * 365 * 8); // 8 years old
    auto recent_ts = now - std::chrono::hours(24 * 30);        // 1 month old
    
    // Manually create log entries with specific timestamps
    {
        std::ofstream ofs(log_path_);
        
        // Very old entry (should be purged)
        nlohmann::json old_record = {
            {"ts", std::chrono::duration_cast<std::chrono::milliseconds>(
                very_old_ts.time_since_epoch()).count()},
            {"category", "AUDIT"},
            {"payload", {{"type", "plaintext"}, {"data", "very_old_event"}}}
        };
        ofs << old_record.dump() << "\n";
        
        // Recent entry (should be kept)
        nlohmann::json recent_record = {
            {"ts", std::chrono::duration_cast<std::chrono::milliseconds>(
                recent_ts.time_since_epoch()).count()},
            {"category", "AUDIT"},
            {"payload", {{"type", "plaintext"}, {"data", "recent_event"}}}
        };
        ofs << recent_record.dump() << "\n";
    }
    
    // Purge entries older than 7 years
    auto purge_threshold = now - std::chrono::hours(24 * 365 * 7);
    
    size_t purged = logger.purgeOldEntries(purge_threshold);
    
    EXPECT_EQ(purged, 1);
    
    // Verify main log only contains recent entry
    std::ifstream main_ifs(log_path_);
    std::string line;
    int main_count = 0;
    while (std::getline(main_ifs, line)) {
        if (!line.empty()) {
            auto record = nlohmann::json::parse(line);
            EXPECT_EQ(record["payload"]["data"], "recent_event");
            ++main_count;
        }
    }
    EXPECT_EQ(main_count, 1);
}

TEST_F(AuditLoggerTest, RetentionWithNoOldEntries) {
    AuditLoggerConfig cfg;
    cfg.enabled = true;
    cfg.encrypt_then_sign = false;
    cfg.log_path = log_path_;
    
    AuditLogger logger(enc_, pki_, cfg);
    
    // Write only recent events with explicit timestamps
    auto now = std::chrono::system_clock::now();
    {
        std::ofstream ofs(log_path_);
        for (int i = 0; i < 3; ++i) {
            nlohmann::json event = {
                {"ts", std::chrono::duration_cast<std::chrono::milliseconds>(
                    now.time_since_epoch()).count()},
                {"category", "AUDIT"},
                {"payload", {{"type", "plaintext"}, {"data", "recent_event"}}},
                {"event_id", i}
            };
            ofs << event.dump() << "\n";
        }
    }
    
    // Try to archive/purge with thresholds that won't match anything
    auto recent_threshold = std::chrono::system_clock::now() - std::chrono::hours(24);
    std::string archive_path = "data/logs/test_audit_archive2.jsonl";
    std::filesystem::remove(archive_path);
    
    size_t archived = logger.archiveOldEntries(recent_threshold, archive_path);
    EXPECT_EQ(archived, 0);
    if (std::filesystem::exists(archive_path)) {
        EXPECT_EQ(std::filesystem::file_size(archive_path), 0u);
    }
    
    size_t purged = logger.purgeOldEntries(recent_threshold);
    EXPECT_EQ(purged, 0);
    
    // Verify all 3 entries are still present
    auto entries = logger.enumerateEntries();
    EXPECT_EQ(entries.size(), 3);
}

// ============================================================================
// Loss-Protection Tests
// ============================================================================

TEST_F(AuditLoggerTest, LogRotationOnSizeLimit) {
    AuditLoggerConfig cfg;
    cfg.enabled = true;
    cfg.encrypt_then_sign = false;
    cfg.log_path = log_path_;
    cfg.enable_hash_chain = false;
    // Set a size limit small enough that the first written entry (which is a JSON
    // object with timestamps, category, payload, etc. — well over 50 bytes) will
    // immediately trigger rotation when the second event is logged.
    cfg.max_file_size_bytes = 10; // 10 bytes: any JSONL audit entry exceeds this
    cfg.max_rotated_files = 2;

    AuditLogger logger(enc_, pki_, cfg);

    // First event: written to the primary log
    logger.logEvent({{"action", "event1"}});
    ASSERT_TRUE(std::filesystem::exists(log_path_));

    // Second event: primary log already exceeds 10 bytes, so rotation fires first
    logger.logEvent({{"action", "event2"}});

    // After rotation the old primary should now be log_path_.1
    std::string rotated = log_path_ + ".1";
    EXPECT_TRUE(std::filesystem::exists(rotated));
    // The new primary must exist and contain the second event
    ASSERT_TRUE(std::filesystem::exists(log_path_));

    // Clean up
    std::filesystem::remove(rotated);
}

TEST_F(AuditLoggerTest, SecondaryLogPathMirror) {
    std::string secondary_path = "data/logs/test_audit_secondary.jsonl";
    std::filesystem::remove(secondary_path);

    AuditLoggerConfig cfg;
    cfg.enabled = true;
    cfg.encrypt_then_sign = false;
    cfg.log_path = log_path_;
    cfg.enable_hash_chain = false;
    cfg.secondary_log_path = secondary_path;

    AuditLogger logger(enc_, pki_, cfg);

    logger.logEvent({{"user", "alice"}, {"action", "login"}});
    logger.logEvent({{"user", "bob"},   {"action", "read"}});

    // Both primary and secondary must exist
    ASSERT_TRUE(std::filesystem::exists(log_path_));
    ASSERT_TRUE(std::filesystem::exists(secondary_path));

    // Count lines in each file — they must match
    auto countLines = [](const std::string& path) {
        std::ifstream ifs(path);
        int n = 0;
        std::string line;
        while (std::getline(ifs, line)) {
          if (!line.empty()) ++n;
        }
        return n;
    };

    EXPECT_EQ(countLines(log_path_), 2);
    EXPECT_EQ(countLines(secondary_path), 2);

    // Verify the same content in both
    auto readLines = [](const std::string& path) {
        std::vector<std::string> lines;
        std::ifstream ifs(path);
        std::string line;
        while (std::getline(ifs, line)) {
          if (!line.empty()) lines.push_back(line);
        }
        return lines;
    };

    auto primary_lines   = readLines(log_path_);
    auto secondary_lines = readLines(secondary_path);
    ASSERT_EQ(primary_lines.size(), secondary_lines.size());
    for (size_t i = 0; i < primary_lines.size(); ++i) {
        EXPECT_EQ(primary_lines[i], secondary_lines[i]);
    }

    std::filesystem::remove(secondary_path);
}
