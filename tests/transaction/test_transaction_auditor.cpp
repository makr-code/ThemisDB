/**
 * @file test_transaction_auditor.cpp
 * @brief Unit tests for TransactionAuditor — v1.8.0
 *
 * Acceptance criteria covered:
 *  AC-1  Auditing disabled by default; record() is a no-op before enableAuditing(true)
 *  AC-2  enableAuditing(true) switches auditing on; enableAuditing(false) switches it off
 *  AC-3  isEnabled() reflects the current auditing state
 *  AC-4  record() appends one AuditRecord when auditing is enabled
 *  AC-5  record() is a no-op when auditing is disabled
 *  AC-6  queryAuditLog() returns all records when no filters are applied
 *  AC-7  queryAuditLog() filters by user_id
 *  AC-8  queryAuditLog() filters by start_time (lower bound inclusive)
 *  AC-9  queryAuditLog() filters by end_time (upper bound inclusive)
 *  AC-10 queryAuditLog() applies start_time + end_time range together
 *  AC-11 queryAuditLog() respects limit (returns most-recent-first)
 *  AC-12 queryAuditLog() with limit=0 returns all matching records
 *  AC-13 queryAuditLog() returns records sorted most-recent-first
 *  AC-14 size() reflects the number of stored records
 *  AC-15 clear() removes all records; size() returns 0 afterwards
 *  AC-16 exportToKafka() without transport returns "transport not configured"
 *  AC-16b exportToKafka() with injected transport serialises NDJSON and forwards it
 *  AC-16c exportToKafka() on empty log returns OK without calling transport
 *  AC-17 exportToS3() without transport returns "transport not configured"
 *  AC-17b exportToS3() with injected transport builds correct S3 key and payload
 *  AC-17c setExportTransport(nullptr) disables export again
 *  AC-18 AuditRecord::Result enum has COMMITTED, ABORTED, DEADLOCK values
 *  AC-19 Operation::Type enum has PUT, DELETE, ADD_EDGE, DELETE_EDGE, ADD_VECTOR values
 *  AC-20 Concurrent record() calls from multiple threads are all stored correctly
 *  AC-21 record() stores all fields (txn_id, user_id, session_id, isolation, result, duration_us)
 *  AC-22 queryAuditLog() with combined user_id + time range returns correct subset
 *  AC-23 record() after clear() starts a fresh log
 *  AC-24 Operations list inside AuditRecord is preserved correctly
 *  AC-25 Default limit of 1000 is respected
 */

#include <gtest/gtest.h>
#include "transaction/transaction_auditor.h"

#include <array>
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

using namespace themis;
using namespace std::chrono_literals;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static TransactionAuditor::AuditRecord makeRecord(
    TransactionAuditor::TransactionId txn_id,
    std::string user_id,
    TransactionAuditor::AuditRecord::Result result =
        TransactionAuditor::AuditRecord::Result::COMMITTED,
    std::chrono::system_clock::time_point ts = std::chrono::system_clock::now(),
    IsolationLevel isolation = IsolationLevel::ReadCommitted,
    uint64_t duration_us = 1000)
{
    TransactionAuditor::AuditRecord rec;
    rec.txn_id      = txn_id;
    rec.user_id     = std::move(user_id);
    rec.session_id  = "session-" + std::to_string(txn_id);
    rec.timestamp   = ts;
    rec.isolation   = isolation;
    rec.result      = result;
    rec.duration_us = duration_us;
    return rec;
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-1  Default state: auditing disabled
// ─────────────────────────────────────────────────────────────────────────────

TEST(TransactionAuditorTest, AC1_DisabledByDefault) {
    TransactionAuditor auditor;
    auditor.record(makeRecord(1, "alice"));
    EXPECT_EQ(auditor.size(), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-2  enableAuditing toggle
// ─────────────────────────────────────────────────────────────────────────────

TEST(TransactionAuditorTest, AC2_EnableDisable) {
    TransactionAuditor auditor;
    auditor.enableAuditing(true);
    auditor.record(makeRecord(1, "alice"));
    EXPECT_EQ(auditor.size(), 1u);

    auditor.enableAuditing(false);
    auditor.record(makeRecord(2, "bob"));
    EXPECT_EQ(auditor.size(), 1u); // second record not stored
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-3  isEnabled()
// ─────────────────────────────────────────────────────────────────────────────

TEST(TransactionAuditorTest, AC3_IsEnabled) {
    TransactionAuditor auditor;
    EXPECT_FALSE(auditor.isEnabled());
    auditor.enableAuditing(true);
    EXPECT_TRUE(auditor.isEnabled());
    auditor.enableAuditing(false);
    EXPECT_FALSE(auditor.isEnabled());
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-4  record() appends when enabled
// ─────────────────────────────────────────────────────────────────────────────

TEST(TransactionAuditorTest, AC4_RecordAppendsWhenEnabled) {
    TransactionAuditor auditor;
    auditor.enableAuditing(true);
    auditor.record(makeRecord(42, "carol"));
    EXPECT_EQ(auditor.size(), 1u);
    auto log = auditor.queryAuditLog();
    ASSERT_EQ(log.size(), 1u);
    EXPECT_EQ(log[0].txn_id, 42u);
    EXPECT_EQ(log[0].user_id, "carol");
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-5  record() is no-op when disabled
// ─────────────────────────────────────────────────────────────────────────────

TEST(TransactionAuditorTest, AC5_RecordNoopWhenDisabled) {
    TransactionAuditor auditor;
    // auditing is off
    auditor.record(makeRecord(1, "dave"));
    EXPECT_EQ(auditor.size(), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-6  queryAuditLog() no filters returns all
// ─────────────────────────────────────────────────────────────────────────────

TEST(TransactionAuditorTest, AC6_QueryAllRecords) {
    TransactionAuditor auditor;
    auditor.enableAuditing(true);
    for (int i = 0; i < 5; ++i) {
        auditor.record(makeRecord(static_cast<uint64_t>(i), "user"));
    }
    auto log = auditor.queryAuditLog();
    EXPECT_EQ(log.size(), 5u);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-7  queryAuditLog() filters by user_id
// ─────────────────────────────────────────────────────────────────────────────

TEST(TransactionAuditorTest, AC7_FilterByUserId) {
    TransactionAuditor auditor;
    auditor.enableAuditing(true);
    auditor.record(makeRecord(1, "alice"));
    auditor.record(makeRecord(2, "bob"));
    auditor.record(makeRecord(3, "alice"));

    auto log = auditor.queryAuditLog("alice");
    ASSERT_EQ(log.size(), 2u);
    for (const auto& rec : log) {
        EXPECT_EQ(rec.user_id, "alice");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-8  queryAuditLog() filters by start_time
// ─────────────────────────────────────────────────────────────────────────────

TEST(TransactionAuditorTest, AC8_FilterByStartTime) {
    TransactionAuditor auditor;
    auditor.enableAuditing(true);

    auto now   = std::chrono::system_clock::now();
    auto older = now - std::chrono::hours(2);
    auto newer = now - std::chrono::hours(1);

    auditor.record(makeRecord(1, "u", TransactionAuditor::AuditRecord::Result::COMMITTED, older));
    auditor.record(makeRecord(2, "u", TransactionAuditor::AuditRecord::Result::COMMITTED, newer));
    auditor.record(makeRecord(3, "u", TransactionAuditor::AuditRecord::Result::COMMITTED, now));

    auto log = auditor.queryAuditLog(std::nullopt, newer);
    EXPECT_EQ(log.size(), 2u); // newer and now pass; older doesn't
    for (const auto& rec : log) {
        EXPECT_GE(rec.timestamp, newer);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-9  queryAuditLog() filters by end_time
// ─────────────────────────────────────────────────────────────────────────────

TEST(TransactionAuditorTest, AC9_FilterByEndTime) {
    TransactionAuditor auditor;
    auditor.enableAuditing(true);

    auto now   = std::chrono::system_clock::now();
    auto older = now - std::chrono::hours(2);
    auto newer = now - std::chrono::hours(1);

    auditor.record(makeRecord(1, "u", TransactionAuditor::AuditRecord::Result::COMMITTED, older));
    auditor.record(makeRecord(2, "u", TransactionAuditor::AuditRecord::Result::COMMITTED, newer));
    auditor.record(makeRecord(3, "u", TransactionAuditor::AuditRecord::Result::COMMITTED, now));

    auto log = auditor.queryAuditLog(std::nullopt, std::nullopt, newer);
    EXPECT_EQ(log.size(), 2u); // older and newer pass; now doesn't
    for (const auto& rec : log) {
        EXPECT_LE(rec.timestamp, newer);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-10 queryAuditLog() start + end time range
// ─────────────────────────────────────────────────────────────────────────────

TEST(TransactionAuditorTest, AC10_FilterByTimeRange) {
    TransactionAuditor auditor;
    auditor.enableAuditing(true);

    auto base = std::chrono::system_clock::now() - std::chrono::hours(10);
    auto t0   = base;
    auto t1   = base + std::chrono::hours(2);
    auto t2   = base + std::chrono::hours(4);
    auto t3   = base + std::chrono::hours(6);
    auto t4   = base + std::chrono::hours(8);

    std::array<std::chrono::system_clock::time_point, 5> times = {t0, t1, t2, t3, t4};
    for (size_t i = 0; i < times.size(); ++i) {
        auditor.record(makeRecord(static_cast<uint64_t>(i), "u",
                                  TransactionAuditor::AuditRecord::Result::COMMITTED, times[i]));
    }

    // Range [t1, t3]
    auto log = auditor.queryAuditLog(std::nullopt, t1, t3);
    EXPECT_EQ(log.size(), 3u);
    for (const auto& rec : log) {
        EXPECT_GE(rec.timestamp, t1);
        EXPECT_LE(rec.timestamp, t3);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-11 queryAuditLog() respects limit (most-recent-first)
// ─────────────────────────────────────────────────────────────────────────────

TEST(TransactionAuditorTest, AC11_LimitRespected) {
    TransactionAuditor auditor;
    auditor.enableAuditing(true);
    auto base = std::chrono::system_clock::now() - std::chrono::hours(10);
    for (int i = 0; i < 10; ++i) {
        auditor.record(makeRecord(static_cast<uint64_t>(i), "u",
                                  TransactionAuditor::AuditRecord::Result::COMMITTED,
                                  base + std::chrono::hours(i)));
    }
    auto log = auditor.queryAuditLog(std::nullopt, std::nullopt, std::nullopt, 3);
    ASSERT_EQ(log.size(), 3u);
    // Most recent first: txn_ids 9, 8, 7
    EXPECT_EQ(log[0].txn_id, 9u);
    EXPECT_EQ(log[1].txn_id, 8u);
    EXPECT_EQ(log[2].txn_id, 7u);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-12 queryAuditLog() limit=0 returns all
// ─────────────────────────────────────────────────────────────────────────────

TEST(TransactionAuditorTest, AC12_LimitZeroReturnsAll) {
    TransactionAuditor auditor;
    auditor.enableAuditing(true);
    for (int i = 0; i < 50; ++i) {
        auditor.record(makeRecord(static_cast<uint64_t>(i), "u"));
    }
    auto log = auditor.queryAuditLog(std::nullopt, std::nullopt, std::nullopt, 0);
    EXPECT_EQ(log.size(), 50u);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-13 queryAuditLog() sorts most-recent-first
// ─────────────────────────────────────────────────────────────────────────────

TEST(TransactionAuditorTest, AC13_MostRecentFirst) {
    TransactionAuditor auditor;
    auditor.enableAuditing(true);
    auto base = std::chrono::system_clock::now() - std::chrono::hours(5);
    for (int i = 0; i < 5; ++i) {
        auditor.record(makeRecord(static_cast<uint64_t>(i), "u",
                                  TransactionAuditor::AuditRecord::Result::COMMITTED,
                                  base + std::chrono::hours(i)));
    }
    auto log = auditor.queryAuditLog(std::nullopt, std::nullopt, std::nullopt, 0);
    ASSERT_EQ(log.size(), 5u);
    for (size_t i = 0; i + 1 < log.size(); ++i) {
        EXPECT_GE(log[i].timestamp, log[i + 1].timestamp);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-14 size() reflects count
// ─────────────────────────────────────────────────────────────────────────────

TEST(TransactionAuditorTest, AC14_Size) {
    TransactionAuditor auditor;
    auditor.enableAuditing(true);
    EXPECT_EQ(auditor.size(), 0u);
    auditor.record(makeRecord(1, "u"));
    EXPECT_EQ(auditor.size(), 1u);
    auditor.record(makeRecord(2, "u"));
    EXPECT_EQ(auditor.size(), 2u);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-15 clear() empties the log
// ─────────────────────────────────────────────────────────────────────────────

TEST(TransactionAuditorTest, AC15_Clear) {
    TransactionAuditor auditor;
    auditor.enableAuditing(true);
    auditor.record(makeRecord(1, "u"));
    auditor.record(makeRecord(2, "u"));
    EXPECT_EQ(auditor.size(), 2u);
    auditor.clear();
    EXPECT_EQ(auditor.size(), 0u);
    EXPECT_TRUE(auditor.queryAuditLog().empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-16 exportToKafka() without transport returns "transport not configured"
// ─────────────────────────────────────────────────────────────────────────────

TEST(TransactionAuditorTest, AC16_ExportToKafkaNoTransport) {
    TransactionAuditor auditor;
    auto st = auditor.exportToKafka("my-topic");
    EXPECT_FALSE(st.ok);
    // Must report missing transport, not "not yet implemented"
    EXPECT_NE(st.message.find("transport not configured"), std::string::npos)
        << "Unexpected message: " << st.message;
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-17 exportToS3() without transport returns "transport not configured"
// ─────────────────────────────────────────────────────────────────────────────

TEST(TransactionAuditorTest, AC17_ExportToS3NoTransport) {
    TransactionAuditor auditor;
    auto st = auditor.exportToS3("my-bucket", "logs/");
    EXPECT_FALSE(st.ok);
    EXPECT_NE(st.message.find("transport not configured"), std::string::npos)
        << "Unexpected message: " << st.message;
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-16b / AC-17b exportToKafka / exportToS3 with injected transport
// ─────────────────────────────────────────────────────────────────────────────

namespace {
/// Spy transport: records the last call's arguments and returns OK.
struct SpyTransport : TransactionAuditor::IAuditExportTransport {
    std::string last_kafka_topic;
    std::string last_kafka_payload;
    std::string last_s3_bucket;
    std::string last_s3_key;
    std::string last_s3_payload;
    int kafka_calls{0};
    int s3_calls{0};

    TransactionAuditor::Status sendKafka(const std::string& topic,
                                         const std::string& payload) override {
        last_kafka_topic   = topic;
        last_kafka_payload = payload;
        ++kafka_calls;
        return TransactionAuditor::Status::OK();
    }

    TransactionAuditor::Status writeS3(const std::string& bucket,
                                       const std::string& key,
                                       const std::string& payload) override {
        last_s3_bucket  = bucket;
        last_s3_key     = key;
        last_s3_payload = payload;
        ++s3_calls;
        return TransactionAuditor::Status::OK();
    }
};
} // anonymous namespace

TEST(TransactionAuditorTest, AC16b_ExportToKafkaWithTransport) {
    SpyTransport spy;
    TransactionAuditor auditor;
    auditor.setExportTransport(&spy);
    auditor.enableAuditing(true);
    auditor.record(makeRecord(42, "user1"));
    auditor.record(makeRecord(43, "user2"));

    auto st = auditor.exportToKafka("audit-topic");
    EXPECT_TRUE(st.ok) << st.message;
    EXPECT_EQ(spy.kafka_calls, 1);
    EXPECT_EQ(spy.last_kafka_topic, "audit-topic");
    // Payload must be non-empty NDJSON with both records
    EXPECT_FALSE(spy.last_kafka_payload.empty());
    // Each line is a JSON record; two records → at least 2 newlines
    size_t newlines = std::count(spy.last_kafka_payload.begin(),
                                  spy.last_kafka_payload.end(), '\n');
    EXPECT_EQ(newlines, 2u);
}

TEST(TransactionAuditorTest, AC17b_ExportToS3WithTransport) {
    SpyTransport spy;
    TransactionAuditor auditor;
    auditor.setExportTransport(&spy);
    auditor.enableAuditing(true);
    auditor.record(makeRecord(99, "admin"));

    auto st = auditor.exportToS3("my-bucket", "audit/");
    EXPECT_TRUE(st.ok) << st.message;
    EXPECT_EQ(spy.s3_calls, 1);
    EXPECT_EQ(spy.last_s3_bucket, "my-bucket");
    // Key should start with the prefix and contain the timestamp suffix
    EXPECT_EQ(spy.last_s3_key.substr(0, 6), "audit/")
        << "Key: " << spy.last_s3_key;
    EXPECT_NE(spy.last_s3_key.find(".ndjson"), std::string::npos)
        << "Key should end with .ndjson: " << spy.last_s3_key;
    EXPECT_FALSE(spy.last_s3_payload.empty());
}

TEST(TransactionAuditorTest, AC16c_ExportToKafkaEmptyLogReturnsOK) {
    SpyTransport spy;
    TransactionAuditor auditor;
    auditor.setExportTransport(&spy);
    // No records recorded — export should succeed without calling transport
    auto st = auditor.exportToKafka("empty-topic");
    EXPECT_TRUE(st.ok) << st.message;
    EXPECT_EQ(spy.kafka_calls, 0);  // transport not called for empty log
}

TEST(TransactionAuditorTest, AC17c_SetNullTransportDisablesExport) {
    SpyTransport spy;
    TransactionAuditor auditor;
    auditor.setExportTransport(&spy);
    auditor.enableAuditing(true);
    auditor.record(makeRecord(1, "u"));

    // Remove transport
    auditor.setExportTransport(nullptr);
    auto st = auditor.exportToKafka("topic");
    EXPECT_FALSE(st.ok);
    EXPECT_EQ(spy.kafka_calls, 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-18 AuditRecord::Result enum values
// ─────────────────────────────────────────────────────────────────────────────

TEST(TransactionAuditorTest, AC18_ResultEnumValues) {
    using Result = TransactionAuditor::AuditRecord::Result;
    // Just verify the enum values compile and are distinct
    EXPECT_NE(Result::COMMITTED, Result::ABORTED);
    EXPECT_NE(Result::ABORTED,   Result::DEADLOCK);
    EXPECT_NE(Result::COMMITTED, Result::DEADLOCK);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-19 Operation::Type enum values
// ─────────────────────────────────────────────────────────────────────────────

TEST(TransactionAuditorTest, AC19_OperationTypeEnumValues) {
    using Type = TransactionAuditor::Operation::Type;
    EXPECT_NE(Type::PUT,         Type::DELETE);
    EXPECT_NE(Type::ADD_EDGE,    Type::DELETE_EDGE);
    EXPECT_NE(Type::ADD_VECTOR,  Type::PUT);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-20 Concurrent record() is thread-safe
// ─────────────────────────────────────────────────────────────────────────────

TEST(TransactionAuditorTest, AC20_ConcurrentRecord) {
    TransactionAuditor auditor;
    auditor.enableAuditing(true);

    constexpr int kThreads  = 8;
    constexpr int kPerThread = 50;
    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&auditor, t]() {
            for (int i = 0; i < kPerThread; ++i) {
                auditor.record(makeRecord(
                    static_cast<uint64_t>(t * kPerThread + i),
                    "user-" + std::to_string(t)));
            }
        });
    }
    for (auto& th : threads) th.join();

    EXPECT_EQ(auditor.size(), static_cast<size_t>(kThreads * kPerThread));
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-21 All AuditRecord fields are stored and retrieved correctly
// ─────────────────────────────────────────────────────────────────────────────

TEST(TransactionAuditorTest, AC21_AllFieldsPreserved) {
    TransactionAuditor auditor;
    auditor.enableAuditing(true);

    auto now = std::chrono::system_clock::now();
    TransactionAuditor::AuditRecord rec;
    rec.txn_id      = 9999;
    rec.user_id     = "eve";
    rec.session_id  = "ses-42";
    rec.timestamp   = now;
    rec.isolation   = IsolationLevel::SERIALIZABLE;
    rec.result      = TransactionAuditor::AuditRecord::Result::DEADLOCK;
    rec.duration_us = 750000;

    TransactionAuditor::Operation op;
    op.type      = TransactionAuditor::Operation::Type::DELETE;
    op.table     = "orders";
    op.key       = "entity:orders:ord-1";
    op.old_value = "old_serialized_value";
    op.new_value = std::nullopt;
    rec.operations.push_back(op);

    auditor.record(rec);
    auto log = auditor.queryAuditLog();
    ASSERT_EQ(log.size(), 1u);
    const auto& got = log[0];
    EXPECT_EQ(got.txn_id,      9999u);
    EXPECT_EQ(got.user_id,     "eve");
    EXPECT_EQ(got.session_id,  "ses-42");
    EXPECT_EQ(got.timestamp,   now);
    EXPECT_EQ(got.isolation,   IsolationLevel::SERIALIZABLE);
    EXPECT_EQ(got.result,      TransactionAuditor::AuditRecord::Result::DEADLOCK);
    EXPECT_EQ(got.duration_us, 750000u);
    ASSERT_EQ(got.operations.size(), 1u);
    EXPECT_EQ(got.operations[0].type,      TransactionAuditor::Operation::Type::DELETE);
    EXPECT_EQ(got.operations[0].table,     "orders");
    EXPECT_EQ(got.operations[0].key,       "entity:orders:ord-1");
    EXPECT_EQ(got.operations[0].old_value, "old_serialized_value");
    EXPECT_FALSE(got.operations[0].new_value.has_value());
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-22 Combined user_id + time range filter
// ─────────────────────────────────────────────────────────────────────────────

TEST(TransactionAuditorTest, AC22_CombinedUserIdAndTimeRange) {
    TransactionAuditor auditor;
    auditor.enableAuditing(true);

    auto base = std::chrono::system_clock::now() - std::chrono::hours(10);

    auditor.record(makeRecord(1, "alice", TransactionAuditor::AuditRecord::Result::COMMITTED,
                              base + 1h));
    auditor.record(makeRecord(2, "bob",   TransactionAuditor::AuditRecord::Result::COMMITTED,
                              base + 2h));
    auditor.record(makeRecord(3, "alice", TransactionAuditor::AuditRecord::Result::COMMITTED,
                              base + 3h));
    auditor.record(makeRecord(4, "alice", TransactionAuditor::AuditRecord::Result::COMMITTED,
                              base + 5h)); // outside range

    auto start = base + 1h;
    auto end   = base + 4h;
    auto log   = auditor.queryAuditLog("alice", start, end);
    EXPECT_EQ(log.size(), 2u);
    for (const auto& rec : log) {
        EXPECT_EQ(rec.user_id, "alice");
        EXPECT_GE(rec.timestamp, start);
        EXPECT_LE(rec.timestamp, end);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-23 record() after clear() starts a fresh log
// ─────────────────────────────────────────────────────────────────────────────

TEST(TransactionAuditorTest, AC23_RecordAfterClear) {
    TransactionAuditor auditor;
    auditor.enableAuditing(true);
    auditor.record(makeRecord(1, "alice"));
    auditor.clear();
    auditor.record(makeRecord(2, "bob"));
    EXPECT_EQ(auditor.size(), 1u);
    auto log = auditor.queryAuditLog();
    ASSERT_EQ(log.size(), 1u);
    EXPECT_EQ(log[0].txn_id, 2u);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-24 Operations list is preserved
// ─────────────────────────────────────────────────────────────────────────────

TEST(TransactionAuditorTest, AC24_OperationsPreserved) {
    TransactionAuditor auditor;
    auditor.enableAuditing(true);

    TransactionAuditor::AuditRecord rec = makeRecord(1, "u");
    rec.operations.push_back({TransactionAuditor::Operation::Type::PUT,    "t", "k1", std::nullopt, "v1"});
    rec.operations.push_back({TransactionAuditor::Operation::Type::DELETE, "t", "k2", "old", std::nullopt});
    rec.operations.push_back({TransactionAuditor::Operation::Type::ADD_EDGE, "edges", "e1", std::nullopt, "edge_data"});
    auditor.record(rec);

    auto log = auditor.queryAuditLog();
    ASSERT_EQ(log.size(), 1u);
    ASSERT_EQ(log[0].operations.size(), 3u);
    EXPECT_EQ(log[0].operations[0].type,  TransactionAuditor::Operation::Type::PUT);
    EXPECT_EQ(log[0].operations[1].type,  TransactionAuditor::Operation::Type::DELETE);
    EXPECT_EQ(log[0].operations[2].type,  TransactionAuditor::Operation::Type::ADD_EDGE);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-25 Default limit of 1000 is respected
// ─────────────────────────────────────────────────────────────────────────────

TEST(TransactionAuditorTest, AC25_DefaultLimitOf1000) {
    TransactionAuditor auditor;
    auditor.enableAuditing(true);
    for (int i = 0; i < 1500; ++i) {
        auditor.record(makeRecord(static_cast<uint64_t>(i), "u"));
    }
    auto log = auditor.queryAuditLog(); // default limit = 1000
    EXPECT_EQ(log.size(), 1000u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Test suite registration
// ─────────────────────────────────────────────────────────────────────────────

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
