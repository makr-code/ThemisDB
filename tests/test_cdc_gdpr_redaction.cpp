/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_cdc_gdpr_redaction.cpp                        ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-04-13 04:36:23                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     267                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 33a346e4e2  2026-02-25  Refactor code structure and remove redundant code blocks ... ║
    • 7a2028071f  2026-02-24  feat(cdc): implement GDPR-aware change log redaction for ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Test: CDC GDPR-Aware Change Log Redaction
// Validates Changefeed::redactByKeyPrefix() and CDCAdmin::redactByKeyPrefix()
// per the GDPR "right to erasure" requirement for the CDC change log.

#include <gtest/gtest.h>
#include "cdc/changefeed.h"
#include "cdc/cdc_admin.h"
#include "storage/rocksdb_wrapper.h"
#include "cdc/cdc_error.h"
#include <filesystem>
#include <thread>

using namespace themis;
using namespace themis::cdc;

// ---------------------------------------------------------------------------
// Test fixture
// ---------------------------------------------------------------------------
class CDCGDPRRedactionTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto tid = std::hash<std::thread::id>{}(std::this_thread::get_id());
        test_db_path_ = "./data/themis_cdc_gdpr_" + std::to_string(tid) +
                        "_" + std::to_string(time(nullptr));
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }

        RocksDBWrapper::Config config;
        config.db_path            = test_db_path_;
        config.memtable_size_mb   = 64;
        config.block_cache_size_mb = 128;

        db_ = std::make_unique<RocksDBWrapper>(config);
        ASSERT_TRUE(db_->open());

        auto* raw_db = db_->getDB();
        ASSERT_NE(raw_db, nullptr);

        Changefeed::RetentionPolicy no_retention;
        no_retention.enabled = false;
        changefeed_ = std::make_unique<Changefeed>(raw_db, nullptr, no_retention);
        admin_      = std::make_unique<CDCAdmin>(changefeed_.get());
    }

    void TearDown() override {
        admin_.reset();
        changefeed_.reset();
        db_->close();
        db_.reset();
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }
    }

    Changefeed::ChangeEvent makePutEvent(const std::string& key,
                                         const std::string& value) {
        Changefeed::ChangeEvent ev;
        ev.type             = Changefeed::ChangeEventType::EVENT_PUT;
        ev.key              = key;
        ev.value            = value;
        ev.before_snapshot  = "{\"old\":\"state\"}";
        ev.after_snapshot   = "{\"new\":\"state\"}";
        return ev;
    }

    std::string test_db_path_;
    std::unique_ptr<RocksDBWrapper>   db_;
    std::unique_ptr<Changefeed>       changefeed_;
    std::unique_ptr<CDCAdmin>         admin_;
};

// ---------------------------------------------------------------------------
// Changefeed::redactByKeyPrefix – unit tests
// ---------------------------------------------------------------------------

TEST_F(CDCGDPRRedactionTest, RedactsMatchingEventValues) {
    changefeed_->recordEvent(makePutEvent("user:42:email", "alice@example.com"));
    changefeed_->recordEvent(makePutEvent("user:42:phone", "+1-800-555-0100"));
    changefeed_->recordEvent(makePutEvent("order:99:total", "149.99"));  // non-PII

    auto result = changefeed_->redactByKeyPrefix("user:42");

    EXPECT_EQ(result.events_scanned,  3u);
    EXPECT_EQ(result.events_redacted, 2u);

    // Re-read events and verify redaction
    Changefeed::ListOptions opts;
    opts.limit = 100;
    auto events = changefeed_->listEvents(opts);

    int redacted_count    = 0;
    int non_redacted_count = 0;
    for (const auto& ev : events) {
        if (ev.key.compare(0, 7, "user:42") == 0) {
            EXPECT_TRUE(ev.redacted)                 << "user:42 event must be redacted";
            EXPECT_EQ(ev.value, "[REDACTED]")        << "value must be [REDACTED]";
            EXPECT_FALSE(ev.before_snapshot.has_value()) << "before_snapshot must be cleared";
            EXPECT_FALSE(ev.after_snapshot.has_value())  << "after_snapshot must be cleared";
            redacted_count++;
        } else {
            EXPECT_FALSE(ev.redacted)                << "non-matching event must not be redacted";
            non_redacted_count++;
        }
    }
    EXPECT_EQ(redacted_count,     2);
    EXPECT_EQ(non_redacted_count, 1);
}

TEST_F(CDCGDPRRedactionTest, PreservesAuditFields) {
    auto stored = changefeed_->recordEvent(makePutEvent("user:7:name", "Bob"));

    auto result = changefeed_->redactByKeyPrefix("user:7");
    EXPECT_EQ(result.events_redacted, 1u);

    // Retrieve the redacted event and verify audit-critical fields are intact
    auto ev = changefeed_->getEvent(stored.sequence);

    EXPECT_EQ(ev.sequence,    stored.sequence)     << "sequence must be preserved";
    EXPECT_EQ(ev.key,         stored.key)          << "key must be preserved";
    EXPECT_EQ(ev.timestamp_ms, stored.timestamp_ms) << "timestamp_ms must be preserved";
    EXPECT_EQ(ev.type,        stored.type)         << "type must be preserved";
    EXPECT_TRUE(ev.redacted)                       << "redacted flag must be true";
    EXPECT_EQ(ev.value,       "[REDACTED]")        << "value must be [REDACTED]";
}

TEST_F(CDCGDPRRedactionTest, SkipsNonMatchingKeys) {
    changefeed_->recordEvent(makePutEvent("order:1:amount",  "50.00"));
    changefeed_->recordEvent(makePutEvent("product:7:title", "Widget"));

    auto result = changefeed_->redactByKeyPrefix("user:");

    EXPECT_EQ(result.events_scanned,  2u);
    EXPECT_EQ(result.events_redacted, 0u);
}

TEST_F(CDCGDPRRedactionTest, SkipsAlreadyRedactedEvents) {
    changefeed_->recordEvent(makePutEvent("user:5:ssn", "123-45-6789"));

    // First redaction
    auto r1 = changefeed_->redactByKeyPrefix("user:5");
    EXPECT_EQ(r1.events_redacted, 1u);

    // Second redaction – must not count already-redacted event
    auto r2 = changefeed_->redactByKeyPrefix("user:5");
    EXPECT_EQ(r2.events_redacted, 0u)
        << "Already-redacted events must not be counted a second time";
}

TEST_F(CDCGDPRRedactionTest, EmptyKeyPrefixThrows) {
    EXPECT_THROW(changefeed_->redactByKeyPrefix(""), CDCException);
}

TEST_F(CDCGDPRRedactionTest, EmptyChangelogReturnsZeroCounts) {
    auto result = changefeed_->redactByKeyPrefix("user:");

    EXPECT_EQ(result.events_scanned,  0u);
    EXPECT_EQ(result.events_redacted, 0u);
}

TEST_F(CDCGDPRRedactionTest, DeleteEventsAreAlsoRedacted) {
    Changefeed::ChangeEvent del_ev;
    del_ev.type  = Changefeed::ChangeEventType::EVENT_DELETE;
    del_ev.key   = "user:3:profile";
    // DELETE events have no value (nullopt) but should still be marked redacted
    changefeed_->recordEvent(del_ev);

    auto result = changefeed_->redactByKeyPrefix("user:3");
    EXPECT_EQ(result.events_redacted, 1u);

    Changefeed::ListOptions opts;
    opts.limit = 10;
    auto events = changefeed_->listEvents(opts);
    ASSERT_EQ(events.size(), 1u);
    EXPECT_TRUE(events[0].redacted);
}

// ---------------------------------------------------------------------------
// CDCAdmin::redactByKeyPrefix – integration tests
// ---------------------------------------------------------------------------

TEST_F(CDCGDPRRedactionTest, AdminRedactByKeyPrefixReturnsCorrectCounts) {
    changefeed_->recordEvent(makePutEvent("user:99:email", "dave@example.com"));
    changefeed_->recordEvent(makePutEvent("user:99:phone", "+44-20-1234-5678"));
    changefeed_->recordEvent(makePutEvent("session:abc",   "active"));

    GDPRRedactionResult result = admin_->redactByKeyPrefix("acme", "user:99", "dpo@acme.com");

    EXPECT_EQ(result.events_scanned,  3u);
    EXPECT_EQ(result.events_redacted, 2u);
    EXPECT_EQ(result.key_prefix,      "user:99");
    EXPECT_EQ(result.tenant_id,       "acme");
    EXPECT_EQ(result.operator_id,     "dpo@acme.com");
    EXPECT_GT(result.timestamp_ms,    0);
}

TEST_F(CDCGDPRRedactionTest, AdminRedactByKeyPrefixJsonSerialization) {
    changefeed_->recordEvent(makePutEvent("user:1:name", "Alice"));

    auto result = admin_->redactByKeyPrefix("tenant-x", "user:1", "admin");
    nlohmann::json j = result.toJson();

    EXPECT_TRUE(j.contains("events_scanned"));
    EXPECT_TRUE(j.contains("events_redacted"));
    EXPECT_TRUE(j.contains("elapsed_time_ms"));
    EXPECT_TRUE(j.contains("key_prefix"));
    EXPECT_TRUE(j.contains("tenant_id"));
    EXPECT_TRUE(j.contains("operator_id"));
    EXPECT_TRUE(j.contains("timestamp_ms"));

    EXPECT_EQ(j["key_prefix"],  "user:1");
    EXPECT_EQ(j["tenant_id"],   "tenant-x");
    EXPECT_EQ(j["operator_id"], "admin");
}

TEST_F(CDCGDPRRedactionTest, AdminRedactEmptyKeyPrefixThrows) {
    EXPECT_THROW(admin_->redactByKeyPrefix("t", "", "op"), CDCException);
}

TEST_F(CDCGDPRRedactionTest, AdminRedactWithDefaultOperatorId) {
    changefeed_->recordEvent(makePutEvent("user:2:data", "secret"));

    // operator_id defaults to "" – must not throw
    EXPECT_NO_THROW(admin_->redactByKeyPrefix("tenant", "user:2"));
}

TEST_F(CDCGDPRRedactionTest, RedactedEventsNotReturnedWithOriginalValue) {
    changefeed_->recordEvent(makePutEvent("user:10:email", "private@user.io"));

    admin_->redactByKeyPrefix("corp", "user:10", "gdpr-bot");

    Changefeed::ListOptions opts;
    opts.limit = 10;
    auto events = changefeed_->listEvents(opts);

    ASSERT_EQ(events.size(), 1u);
    ASSERT_TRUE(events[0].value.has_value())
        << "Redacted event must carry the [REDACTED] marker, not nullopt";
    EXPECT_EQ(*events[0].value, "[REDACTED]")
        << "Consumer must see [REDACTED] not the original PII value";
    // Ensure the original PII is gone
    EXPECT_EQ(events[0].value->find("private@user.io"), std::string::npos);
}
