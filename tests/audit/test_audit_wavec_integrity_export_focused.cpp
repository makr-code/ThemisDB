// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_audit_wavec_integrity_export_focused.cpp
 * @brief Wave-C audit integrity and export tests against the production
 *        themis::utils::AuditLogger file-backed persistence path.
 *
 * Covers Wave C audit requirements with the real JSONL sink and chain-state
 * files:
 * - Tamper-evidence integrity validation under single-writer and concurrent load.
 * - High-volume persistence plus export-style enumeration with zero data loss.
 * - Recovery detection after persisted-log truncation/tampering.
 * - Backpressure enforcement through the production max_queued_events guard.
 * - Compliance and security trail validation using persisted production records.
 */

#include <gtest/gtest.h>

#include "utils/audit_logger.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

namespace fs = std::filesystem;
using namespace themis::utils;

namespace {

AuditLoggerConfig makeConfig(const fs::path& log_path,
                             const fs::path& chain_state_path,
                             bool enable_fsync = false) {
    AuditLoggerConfig cfg;
    cfg.enabled = true;
    cfg.encrypt_then_sign = false;
    cfg.log_path = log_path.string();
    cfg.enable_hash_chain = true;
    cfg.chain_state_file = chain_state_path.string();
    cfg.enable_siem = false;
    cfg.enable_fsync = enable_fsync;
    cfg.max_file_size_bytes = 0;
    cfg.max_rotated_files = 0;
    return cfg;
}

std::size_t countNonEmptyLines(const fs::path& path) {
    std::ifstream in(path);
    std::size_t count = 0;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty()) {
            ++count;
        }
    }
    return count;
}

std::vector<std::string> readLines(const fs::path& path) {
    std::ifstream in(path);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) {
        lines.push_back(line);
    }
    return lines;
}

void writeLines(const fs::path& path, const std::vector<std::string>& lines) {
    std::ofstream out(path, std::ios::trunc);
    for (const auto& line : lines) {
        out << line << '\n';
    }
}

std::string decodeBase64(std::string_view input) {
    static constexpr signed char kDecTable[256] = {
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,
        52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,
        -1,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,
        15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
        -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
        41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
    };

    std::string output;
    output.reserve((input.size() * 3) / 4);
    int value = 0;
    int bits = -8;

    for (unsigned char c : input) {
        if (c == '=') {
            break;
        }
        const int decoded = kDecTable[c];
        if (decoded < 0) {
            continue;
        }
        value = (value << 6) + decoded;
        bits += 6;
        if (bits >= 0) {
            output.push_back(static_cast<char>((value >> bits) & 0xFF));
            bits -= 8;
        }
    }

    return output;
}

nlohmann::json decodePayload(const nlohmann::json& record) {
    const auto& payload = record.at("payload");
    if (payload.contains("data")) {
        return payload.at("data");
    }
    return nlohmann::json::parse(
        decodeBase64(payload.at("data_b64").get_ref<const std::string&>()));
}

nlohmann::json makeEvent(std::string event_type,
                         std::string actor,
                         std::string resource,
                         std::string action,
                         std::string compliance_tags = {},
                         std::string severity = {}) {
    nlohmann::json event = {
        {"event_type", std::move(event_type)},
        {"user_id", std::move(actor)},
        {"resource", std::move(resource)},
        {"action", std::move(action)}
    };

    if (!compliance_tags.empty()) {
        event["compliance_tags"] = std::move(compliance_tags);
    }
    if (!severity.empty()) {
        event["severity"] = std::move(severity);
    }

    return event;
}

template <typename Sink>
std::pair<std::size_t, std::size_t> exportPersistedEntries(const AuditLogger& logger,
                                                           Sink&& sink,
                                                           std::size_t max_retries_per_entry) {
    const auto entries = logger.enumerateEntries();
    std::size_t exported = 0;
    std::size_t retries = 0;

    for (const auto& entry : entries) {
        bool delivered = false;
        std::size_t attempts = 0;
        while (!delivered && attempts <= max_retries_per_entry) {
            delivered = sink(entry.record);
            if (!delivered) {
                ++retries;
                ++attempts;
            }
        }
        if (delivered) {
            ++exported;
        }
    }

    return {exported, retries};
}

class AuditWaveCProductionTest : public ::testing::Test {
protected:
    void SetUp() override {
        const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        tmp_dir_ = fs::temp_directory_path() / "themis_audit_wavec_production"
                 / info->test_suite_name() / info->name();
        fs::remove_all(tmp_dir_);
        fs::create_directories(tmp_dir_);
        log_path_ = tmp_dir_ / "audit.jsonl";
        chain_state_path_ = tmp_dir_ / "audit_chain.json";
    }

    void TearDown() override {
        fs::remove_all(tmp_dir_);
    }

    AuditLogger makeLogger(bool enable_fsync = false) const {
        return AuditLogger(std::shared_ptr<themis::FieldEncryption>{},
                           std::shared_ptr<VCCPKIClient>{},
                           makeConfig(log_path_, chain_state_path_, enable_fsync));
    }

    fs::path tmp_dir_;
    fs::path log_path_;
    fs::path chain_state_path_;
};

TEST_F(AuditWaveCProductionTest, TamperEvidenceChainRemainsIntactWithSingleWriter) {
    auto logger = makeLogger(true);

    for (int i = 0; i < 100; ++i) {
        logger.logEvent(makeEvent("POLICY_UPDATED",
                                  "admin",
                                  "/policy/rbac/" + std::to_string(i),
                                  "modify",
                                  "ISO27001,GDPR",
                                  "LOW"));
    }

    logger.flush();

    ASSERT_TRUE(fs::exists(log_path_));
    ASSERT_TRUE(fs::exists(chain_state_path_));
    EXPECT_EQ(countNonEmptyLines(log_path_), 100u);
    EXPECT_EQ(logger.enumerateEntries().size(), 100u);
    EXPECT_TRUE(logger.verifyChainIntegrity());

    const auto state = logger.getChainState();
    EXPECT_EQ(state.value("entry_count", 0u), 100u);
    EXPECT_TRUE(state.value("chain_enabled", false));
}

TEST_F(AuditWaveCProductionTest, TamperEvidenceChainRemainsIntactUnderConcurrentWrites) {
    auto logger = makeLogger();
    constexpr int kThreads = 8;
    constexpr int kEventsPerThread = 500;

    std::vector<std::thread> writers;
    writers.reserve(kThreads);

    for (int thread_index = 0; thread_index < kThreads; ++thread_index) {
        writers.emplace_back([&logger, thread_index]() {
            for (int event_index = 0; event_index < kEventsPerThread; ++event_index) {
                logger.logEvent(makeEvent("KEY_ROTATED",
                                          "key_manager_" + std::to_string(thread_index),
                                          "/hsm/key/" + std::to_string(event_index),
                                          "rotate",
                                          "ISO27001,BSIC5",
                                          "LOW"));
            }
        });
    }

    for (auto& writer : writers) {
        writer.join();
    }

    const auto expected_total = static_cast<std::size_t>(kThreads * kEventsPerThread);
    const auto entries = logger.enumerateEntries();

    EXPECT_EQ(entries.size(), expected_total);
    EXPECT_EQ(countNonEmptyLines(log_path_), expected_total);
    EXPECT_TRUE(logger.verifyChainIntegrity());

    const auto report = logger.generateComplianceReport(
        std::chrono::system_clock::now() - std::chrono::hours(1),
        std::chrono::system_clock::now() + std::chrono::hours(1));
    EXPECT_EQ(report.total_events, expected_total);
    EXPECT_EQ(report.key_management_events, expected_total);
    EXPECT_EQ(report.event_counts_by_type.value("KEY_ROTATED", 0), kThreads * kEventsPerThread);
}

TEST_F(AuditWaveCProductionTest, RecoveryAfterPartialLogTruncationDetectsTamper) {
    auto logger = makeLogger();

    for (int i = 0; i < 20; ++i) {
        logger.logEvent(makeEvent("SUSPICIOUS_ACTIVITY",
                                  "anomaly_detector",
                                  "/query/" + std::to_string(i),
                                  "flag",
                                  "ISO27001,NIS2",
                                  "HIGH"));
    }

    auto lines = readLines(log_path_);
    ASSERT_GE(lines.size(), 10u);
    lines.erase(lines.begin() + 5);
    writeLines(log_path_, lines);

    auto verifier = makeLogger();
    EXPECT_FALSE(verifier.verifyChainIntegrity());
}

TEST_F(AuditWaveCProductionTest, HighVolumeExportHandlesSustainedLoad) {
    auto logger = makeLogger();
    constexpr int kTotalEvents = 10000;
    constexpr int kWriterThreads = 4;
    constexpr int kEventsPerWriter = kTotalEvents / kWriterThreads;

    auto start_time = std::chrono::steady_clock::now();
    std::vector<std::thread> writers;
    writers.reserve(kWriterThreads);

    for (int thread_index = 0; thread_index < kWriterThreads; ++thread_index) {
        writers.emplace_back([&logger, thread_index]() {
            for (int event_index = 0; event_index < kEventsPerWriter; ++event_index) {
                logger.logEvent(makeEvent("DATA_WRITE",
                                          "user_" + std::to_string(thread_index),
                                          "/query/" + std::to_string(event_index),
                                          "execute",
                                          "ISO27001,GDPR",
                                          "MEDIUM"));
            }
        });
    }

    for (auto& writer : writers) {
        writer.join();
    }

    std::size_t exported_count = 0;
    const auto [exported, retries] = exportPersistedEntries(
        logger,
        [&exported_count](const nlohmann::json& record) {
            const bool ok = !record.is_null();
            if (ok) {
                ++exported_count;
            }
            return ok;
        },
        0);

    const auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start_time).count();

    EXPECT_EQ(countNonEmptyLines(log_path_), static_cast<std::size_t>(kTotalEvents));
    EXPECT_EQ(exported, static_cast<std::size_t>(kTotalEvents));
    EXPECT_EQ(exported_count, static_cast<std::size_t>(kTotalEvents));
    EXPECT_EQ(retries, 0u);
    EXPECT_TRUE(logger.verifyChainIntegrity());
    EXPECT_GT(duration_ms, 0);
}

TEST_F(AuditWaveCProductionTest, ExportQueueBoundedGrowthUnderBackpressure) {
    AuditLoggerConfig cfg = makeConfig(log_path_, chain_state_path_);
    cfg.max_queued_events = 64;
    AuditLogger logger(std::shared_ptr<themis::FieldEncryption>{},
                       std::shared_ptr<VCCPKIClient>{},
                       cfg);

    bool overflow_detected = false;
    for (int i = 0; i < 500; ++i) {
        try {
            logger.logEvent(makeEvent("BULK_EXPORT",
                                      "load_generator",
                                      "/export/" + std::to_string(i),
                                      "export",
                                      "ISO27001,GDPR",
                                      "MEDIUM"));
        } catch (const std::runtime_error&) {
            overflow_detected = true;
            break;
        }
    }

    EXPECT_TRUE(overflow_detected);
    EXPECT_LT(countNonEmptyLines(log_path_), 500u);
}

TEST_F(AuditWaveCProductionTest, ExportRetryLogicHandlesTransientFailures) {
    auto logger = makeLogger();
    constexpr int kTestEvents = 1000;
    std::atomic<int> transient_failures{0};

    for (int i = 0; i < kTestEvents; ++i) {
        logger.logEvent(makeEvent("DATA_WRITE",
                                  "retry_tester",
                                  "/retry/" + std::to_string(i),
                                  "export",
                                  "ISO27001,GDPR",
                                  "MEDIUM"));
    }

    std::atomic<int> call_count{0};
    const auto [exported, retries] = exportPersistedEntries(
        logger,
        [&call_count, &transient_failures](const nlohmann::json&) {
            const int current = ++call_count;
            if (current % 10 == 0) {
                ++transient_failures;
                return false;
            }
            return true;
        },
        2);

    EXPECT_GT(transient_failures.load(), 0);
    EXPECT_GT(retries, 0u);
    EXPECT_EQ(exported, static_cast<std::size_t>(kTestEvents));
}

TEST_F(AuditWaveCProductionTest, AuditEventsTaggedWithComplianceFrameworks) {
    auto logger = makeLogger();

    logger.logEvent(makeEvent("KEY_ROTATED",
                              "compliance_officer",
                              "/access/policy",
                              "modify",
                              "ISO27001,ISO27018",
                              "LOW"));
    logger.logEvent(makeEvent("PII_ACCESSED",
                              "data_subject",
                              "/pii/user123",
                              "delete",
                              "GDPR,CCPA",
                              "HIGH"));
    logger.logEvent(makeEvent("UNAUTHORIZED_ACCESS",
                              "security_team",
                              "/incident/IR-2026-001",
                              "investigate",
                              "BSIC5,NIS2",
                              "HIGH"));

    const auto entries = logger.enumerateEntries();
    ASSERT_EQ(entries.size(), 3u);

    const auto first_payload = decodePayload(entries[0].record);
    const auto second_payload = decodePayload(entries[1].record);
    const auto third_payload = decodePayload(entries[2].record);

    EXPECT_NE(first_payload.value("compliance_tags", std::string{}).find("ISO27001"), std::string::npos);
    EXPECT_NE(second_payload.value("compliance_tags", std::string{}).find("GDPR"), std::string::npos);
    EXPECT_NE(third_payload.value("compliance_tags", std::string{}).find("BSIC5"), std::string::npos);
}

TEST_F(AuditWaveCProductionTest, SecurityEventTrailsAreAuditableAndTraceable) {
    auto logger = makeLogger();

    logger.logSecurityEvent(SecurityEventType::KEY_ROTATED,
                            "key_manager",
                            "/hsm/key/prod_master");
    logger.logSecurityEvent(SecurityEventType::POLICY_UPDATED,
                            "security_admin",
                            "/policy/access_control");
    logger.logSecurityEvent(SecurityEventType::UNAUTHORIZED_ACCESS,
                            "anomaly_detector",
                            "/query/suspicious");

    const auto report = logger.generateComplianceReport(
        std::chrono::system_clock::now() - std::chrono::hours(1),
        std::chrono::system_clock::now() + std::chrono::hours(1));
    const auto unauthorized = logger.searchEntries(AuditLogger::SearchQuery{
        .from = std::nullopt,
        .to = std::nullopt,
        .user_id = "anomaly_detector",
        .action = "UNAUTHORIZED_ACCESS",
        .resource_prefix = "/query",
        .max_results = 0
    });

    EXPECT_TRUE(logger.verifyChainIntegrity());
    EXPECT_EQ(report.total_events, 3u);
    EXPECT_EQ(report.key_management_events, 1u);
    EXPECT_EQ(report.security_events, 3u);
    ASSERT_EQ(unauthorized.size(), 1u);
    EXPECT_EQ(decodePayload(unauthorized.front().record).value("event_type", std::string{}),
              "UNAUTHORIZED_ACCESS");
}

} // namespace
