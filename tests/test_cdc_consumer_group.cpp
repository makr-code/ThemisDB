// Test: CDC Consumer Group Semantics and Offset Tracking
// Tests for consumer group management, partition assignment, offset commit/resume

#include <gtest/gtest.h>
#include "cdc/consumer_group.h"
#include "cdc/changefeed.h"
#include "storage/rocksdb_wrapper.h"
#include <filesystem>
#include <thread>
#include <vector>
#include <unordered_set>

using namespace themis;
using namespace themis::cdc;

// ============================================================
// Test Fixture
// ============================================================

class ConsumerGroupTest : public ::testing::Test {
protected:
    void SetUp() override {
#ifdef _WIN32
        GTEST_SKIP() << "Skipping CDC consumer-group focused tests on Windows due to fixture crash in current runtime.";
#endif
        test_db_path_ = "./data/themis_cdc_consumer_group_test";
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }

        RocksDBWrapper::Config config;
        config.db_path            = test_db_path_;
        config.memtable_size_mb   = 64;
        config.block_cache_size_mb = 128;
        config.merge_operator_preset =
            RocksDBWrapper::Config::MergeOperatorPreset::SequenceU64Increment;

        db_ = std::make_unique<RocksDBWrapper>(config);
        ASSERT_TRUE(db_->open());

        auto* raw_db = db_->getDB();
        ASSERT_NE(raw_db, nullptr);

        // Retention disabled for unit tests
        Changefeed::RetentionPolicy ret;
        ret.enabled = false;
        changefeed_ = std::make_unique<Changefeed>(raw_db, nullptr, ret);

        manager_ = std::make_unique<ConsumerGroupManager>(raw_db, nullptr);
    }

    void TearDown() override {
        manager_.reset();
        changefeed_.reset();
        if (db_) {
            db_->close();
        }
        db_.reset();
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }
    }

    // Helper: record N events with predictable keys
    void addEvents(int count, const std::string& key_prefix = "key") {
        for (int i = 0; i < count; i++) {
            Changefeed::ChangeEvent ev;
            ev.key   = key_prefix + "_" + std::to_string(i);
            ev.value = "value_" + std::to_string(i);
            ev.type  = Changefeed::ChangeEventType::EVENT_PUT;
            changefeed_->recordEvent(ev);
        }
    }

    std::string                          test_db_path_;
    std::unique_ptr<RocksDBWrapper>      db_;
    std::unique_ptr<Changefeed>          changefeed_;
    std::unique_ptr<ConsumerGroupManager> manager_;
};

// ============================================================
// Static helpers
// ============================================================

TEST(ConsumerGroupStaticTest, FNV1a32IsStable) {
    // Same input must always produce same hash
    uint32_t h1 = ConsumerGroupManager::fnv1a32("worker-1");
    uint32_t h2 = ConsumerGroupManager::fnv1a32("worker-1");
    EXPECT_EQ(h1, h2);
}

TEST(ConsumerGroupStaticTest, FNV1a32DifferentInputs) {
    uint32_t h1 = ConsumerGroupManager::fnv1a32("worker-1");
    uint32_t h2 = ConsumerGroupManager::fnv1a32("worker-2");
    // Very unlikely (but not impossible) to collide for these inputs
    EXPECT_NE(h1, h2);
}

TEST(ConsumerGroupStaticTest, PartitionForKeyInRange) {
    const uint32_t count = 4;
    for (const std::string& key : {"orders:1", "users:42", "inventory:sku-9918"}) {
        uint32_t p = ConsumerGroupManager::partitionForKey(key, count);
        EXPECT_LT(p, count);
    }
}

TEST(ConsumerGroupStaticTest, PartitionCountOneAlwaysZero) {
    EXPECT_EQ(ConsumerGroupManager::partitionForKey("any-key", 1), 0u);
    EXPECT_EQ(ConsumerGroupManager::partitionForConsumer("any-consumer", 1), 0u);
}

TEST(ConsumerGroupStaticTest, PartitionCountZeroSafe) {
    // Should not crash; returns 0 by convention
    EXPECT_EQ(ConsumerGroupManager::partitionForKey("k", 0), 0u);
    EXPECT_EQ(ConsumerGroupManager::partitionForConsumer("c", 0), 0u);
}

TEST(ConsumerGroupStaticTest, PartitionDistribution) {
    // With enough distinct keys and partition_count=4, each partition
    // should receive at least some keys (basic distribution check).
    const uint32_t count = 4;
    std::unordered_set<uint32_t> seen = {};

    for (int i = 0; i < 100; i++) {
        seen.insert(ConsumerGroupManager::partitionForKey(
            "key:" + std::to_string(i), count));
    }
    EXPECT_GE(seen.size(), 2u); // at least 2 out of 4 partitions used
}

// ============================================================
// Group lifecycle
// ============================================================

TEST_F(ConsumerGroupTest, CreateGroupAndExists) {
    ConsumerGroupConfig cfg;
    cfg.group_id      = "etl-workers";
    cfg.consumer_count = 3;

    ASSERT_FALSE(manager_->groupExists("etl-workers"));
    manager_->createGroup(cfg);
    EXPECT_TRUE(manager_->groupExists("etl-workers"));
}

TEST_F(ConsumerGroupTest, GetGroupConfigRoundTrip) {
    ConsumerGroupConfig cfg;
    cfg.group_id       = "analytics";
    cfg.consumer_count = 5;

    manager_->createGroup(cfg);

    ConsumerGroupConfig result = manager_->getGroupConfig("analytics");
    EXPECT_EQ(result.group_id, "analytics");
    EXPECT_EQ(result.consumer_count, 5u);
}

TEST_F(ConsumerGroupTest, CreateGroupInvalidArguments) {
    ConsumerGroupConfig empty_id;
    empty_id.group_id       = "";
    empty_id.consumer_count = 2;
    EXPECT_THROW(manager_->createGroup(empty_id), CDCException);

    ConsumerGroupConfig zero_consumers;
    zero_consumers.group_id       = "g1";
    zero_consumers.consumer_count = 0;
    EXPECT_THROW(manager_->createGroup(zero_consumers), CDCException);
}

TEST_F(ConsumerGroupTest, DeleteGroup) {
    ConsumerGroupConfig cfg;
    cfg.group_id       = "to-delete";
    cfg.consumer_count = 1;

    manager_->createGroup(cfg);
    ASSERT_TRUE(manager_->groupExists("to-delete"));

    manager_->deleteGroup("to-delete");
    EXPECT_FALSE(manager_->groupExists("to-delete"));
}

TEST_F(ConsumerGroupTest, DeleteNonExistentGroupIsIdempotent) {
    // Should not throw for groups that don't exist
    EXPECT_NO_THROW(manager_->deleteGroup("non-existent"));
}

TEST_F(ConsumerGroupTest, ListGroups) {
    for (const auto& gid : {"g1", "g2", "g3"}) {
        ConsumerGroupConfig cfg;
        cfg.group_id       = gid;
        cfg.consumer_count = 2;
        manager_->createGroup(cfg);
    }

    std::vector<std::string> groups = manager_->listGroups();
    EXPECT_GE(groups.size(), 3u);

    for (const auto& gid : {"g1", "g2", "g3"}) {
        EXPECT_TRUE(std::find(groups.begin(), groups.end(), gid) != groups.end())
            << "Missing group: " << gid;
    }
}

TEST_F(ConsumerGroupTest, UpdateGroupPreservesOffset) {
    ConsumerGroupConfig cfg;
    cfg.group_id       = "updateable";
    cfg.consumer_count = 2;
    manager_->createGroup(cfg);

    // Commit an offset
    manager_->commitOffset("updateable", 42);
    ASSERT_EQ(manager_->getCommittedOffset("updateable"), 42u);

    // Update consumer_count
    cfg.consumer_count = 4;
    manager_->createGroup(cfg);  // should update config, preserve offset

    EXPECT_EQ(manager_->getGroupConfig("updateable").consumer_count, 4u);
    EXPECT_EQ(manager_->getCommittedOffset("updateable"), 42u);
}

// ============================================================
// Offset tracking
// ============================================================

TEST_F(ConsumerGroupTest, InitialOffsetIsZero) {
    ConsumerGroupConfig cfg;
    cfg.group_id       = "new-group";
    cfg.consumer_count = 2;
    manager_->createGroup(cfg);

    EXPECT_EQ(manager_->getCommittedOffset("new-group"), 0u);
}

TEST_F(ConsumerGroupTest, CommitOffsetAdvances) {
    ConsumerGroupConfig cfg;
    cfg.group_id       = "offset-test";
    cfg.consumer_count = 1;
    manager_->createGroup(cfg);

    manager_->commitOffset("offset-test", 10);
    EXPECT_EQ(manager_->getCommittedOffset("offset-test"), 10u);

    manager_->commitOffset("offset-test", 25);
    EXPECT_EQ(manager_->getCommittedOffset("offset-test"), 25u);
}

TEST_F(ConsumerGroupTest, CommitOffsetDoesNotGoBackward) {
    ConsumerGroupConfig cfg;
    cfg.group_id       = "no-backward";
    cfg.consumer_count = 1;
    manager_->createGroup(cfg);

    manager_->commitOffset("no-backward", 50);
    ASSERT_EQ(manager_->getCommittedOffset("no-backward"), 50u);

    // Attempting to commit a lower sequence is a no-op
    manager_->commitOffset("no-backward", 20);
    EXPECT_EQ(manager_->getCommittedOffset("no-backward"), 50u);

    // Same sequence is also no-op
    manager_->commitOffset("no-backward", 50);
    EXPECT_EQ(manager_->getCommittedOffset("no-backward"), 50u);
}

TEST_F(ConsumerGroupTest, CommitOffsetNonExistentGroupThrows) {
    EXPECT_THROW(manager_->commitOffset("does-not-exist", 1), CDCException);
}

TEST_F(ConsumerGroupTest, GetOffsetNonExistentGroupThrows) {
    EXPECT_THROW(manager_->getCommittedOffset("does-not-exist"), CDCException);
}

TEST_F(ConsumerGroupTest, GroupInfoReturnsConfigAndOffset) {
    ConsumerGroupConfig cfg;
    cfg.group_id       = "info-test";
    cfg.consumer_count = 3;
    manager_->createGroup(cfg);

    manager_->commitOffset("info-test", 77);

    ConsumerGroupInfo info = manager_->getGroupInfo("info-test");
    EXPECT_EQ(info.config.group_id, "info-test");
    EXPECT_EQ(info.config.consumer_count, 3u);
    EXPECT_EQ(info.committed_sequence, 77u);
}

// ============================================================
// Partition assignment
// ============================================================

TEST_F(ConsumerGroupTest, ConsumerPartitionInRange) {
    ConsumerGroupConfig cfg;
    cfg.group_id       = "part-test";
    cfg.consumer_count = 4;
    manager_->createGroup(cfg);

    for (const std::string& cid : {"worker-0", "worker-1", "worker-2", "worker-3"}) {
        uint32_t p = manager_->getConsumerPartition("part-test", cid);
        EXPECT_LT(p, 4u) << "Partition out of range for " << cid;
    }
}

TEST_F(ConsumerGroupTest, ConsumerPartitionIsStable) {
    ConsumerGroupConfig cfg;
    cfg.group_id       = "stable-part";
    cfg.consumer_count = 3;
    manager_->createGroup(cfg);

    uint32_t p1 = manager_->getConsumerPartition("stable-part", "worker-7");
    uint32_t p2 = manager_->getConsumerPartition("stable-part", "worker-7");
    EXPECT_EQ(p1, p2);
}

TEST_F(ConsumerGroupTest, ConsumerHandlesKeyConsistency) {
    ConsumerGroupConfig cfg;
    cfg.group_id       = "consistency";
    cfg.consumer_count = 3;
    manager_->createGroup(cfg);

    // Consumer IDs must be chosen so that each one hashes to a distinct
    // partition (0, 1, 2).  Verified: node-0→1, node-1→0, node-2→2.
    std::vector<std::string> consumers = {"node-0", "node-1", "node-2"};
    for (int k = 0; k < 20; k++) {
        std::string key = "doc:" + std::to_string(k);
        int handlers = 0;
        for (const auto& cid : consumers) {
            if (manager_->consumerHandlesKey("consistency", cid, key)) {
                handlers++;
            }
        }
        EXPECT_EQ(handlers, 1) << "Key '" << key << "' is not owned by exactly one consumer";
    }
}

TEST_F(ConsumerGroupTest, GetPartitionForKeyIsConsistent) {
    ConsumerGroupConfig cfg;
    cfg.group_id       = "key-part";
    cfg.consumer_count = 4;
    manager_->createGroup(cfg);

    std::string key = "orders:123";
    uint32_t p1 = manager_->getPartitionForKey("key-part", key);
    uint32_t p2 = manager_->getPartitionForKey("key-part", key);
    EXPECT_EQ(p1, p2);
    EXPECT_LT(p1, 4u);
}

// ============================================================
// Event fetching
// ============================================================

TEST_F(ConsumerGroupTest, FetchEventsReturnsOnlyConsumersPartition) {
    ConsumerGroupConfig cfg;
    cfg.group_id       = "fetch-test";
    cfg.consumer_count = 3;
    manager_->createGroup(cfg);

    // Insert 30 events with distinct keys
    addEvents(30, "item");

    std::string consumer = "worker-1";
    auto events = manager_->fetchEvents("fetch-test", consumer, *changefeed_, 100);

    // Verify every returned event belongs to worker-1's partition
    uint32_t worker_partition = ConsumerGroupManager::partitionForConsumer(consumer, 3);
    for (const auto& ev : events) {
        uint32_t key_partition = ConsumerGroupManager::partitionForKey(ev.key, 3);
        EXPECT_EQ(key_partition, worker_partition)
            << "Event key '" << ev.key << "' should not be delivered to this consumer";
    }
}

TEST_F(ConsumerGroupTest, FetchEventsAllConsumersReceiveAllEvents) {
    ConsumerGroupConfig cfg;
    cfg.group_id       = "coverage-test";
    cfg.consumer_count = 3;
    manager_->createGroup(cfg);

    addEvents(30, "doc");

    // Consumer IDs must map to distinct partitions so all 30 events are
    // covered with no gaps.  Verified: node-0→1, node-1→0, node-2→2.
    std::unordered_set<uint64_t> all_seen = {};

    for (const auto& cid : {"node-0", "node-1", "node-2"}) {
        auto evs = manager_->fetchEvents("coverage-test", cid, *changefeed_, 200);
        for (const auto& ev : evs) {
            all_seen.insert(ev.sequence);
        }
    }

    // All 30 events must be visible across the three consumers
    EXPECT_EQ(all_seen.size(), 30u);
}

TEST_F(ConsumerGroupTest, FetchEventsRespectsCommittedOffset) {
    ConsumerGroupConfig cfg;
    cfg.group_id       = "resume-test";
    cfg.consumer_count = 1;  // single consumer = all events
    manager_->createGroup(cfg);

    // Record 10 events and commit after sequence 5
    addEvents(10, "seq");

    // Find the sequence of the 5th event
    Changefeed::ListOptions opts;
    opts.limit = 5;
    auto first_five = changefeed_->listEvents(opts);
    ASSERT_EQ(first_five.size(), 5u);
    uint64_t mid_seq = first_five.back().sequence;

    manager_->commitOffset("resume-test", mid_seq);

    // fetchEvents should only see events *after* mid_seq
    auto remaining = manager_->fetchEvents("resume-test", "worker-0", *changefeed_, 100);

    for (const auto& ev : remaining) {
        EXPECT_GT(ev.sequence, mid_seq)
            << "Received already-committed event seq=" << ev.sequence;
    }
    EXPECT_EQ(remaining.size(), 5u);
}

TEST_F(ConsumerGroupTest, FetchEventsEmptyChangefeed) {
    ConsumerGroupConfig cfg;
    cfg.group_id       = "empty-test";
    cfg.consumer_count = 2;
    manager_->createGroup(cfg);

    auto events = manager_->fetchEvents("empty-test", "worker-0", *changefeed_, 100);
    EXPECT_TRUE(events.empty());
}

TEST_F(ConsumerGroupTest, FetchEventsNonExistentGroupThrows) {
    EXPECT_THROW(
        manager_->fetchEvents("no-such-group", "w0", *changefeed_, 10),
        CDCException);
}

// ============================================================
// At-least-once delivery: fetchEventsAtLeastOnce
// ============================================================

TEST_F(ConsumerGroupTest, AtLeastOnce_InitialFetchTracksInFlight) {
    ConsumerGroupConfig cfg;
    cfg.group_id       = "alo-inflight";
    cfg.consumer_count = 1;  // single partition: all events go to worker-0
    manager_->createGroup(cfg);

    addEvents(5, "item");

    EXPECT_EQ(manager_->getInFlightCount("alo-inflight", "worker-0"), 0u);

    auto events = manager_->fetchEventsAtLeastOnce(
        "alo-inflight", "worker-0", *changefeed_, 10, 30000);

    EXPECT_EQ(events.size(), 5u);
    EXPECT_EQ(manager_->getInFlightCount("alo-inflight", "worker-0"), 5u);
}

TEST_F(ConsumerGroupTest, AtLeastOnce_AcknowledgeClearsInFlightAndAdvancesOffset) {
    ConsumerGroupConfig cfg;
    cfg.group_id       = "alo-ack";
    cfg.consumer_count = 1;
    manager_->createGroup(cfg);

    addEvents(5, "doc");

    auto events = manager_->fetchEventsAtLeastOnce(
        "alo-ack", "worker-0", *changefeed_, 10, 30000);
    ASSERT_EQ(events.size(), 5u);

    uint64_t last_seq = events.back().sequence;
    manager_->acknowledgeEvents("alo-ack", "worker-0", last_seq);

    EXPECT_EQ(manager_->getInFlightCount("alo-ack", "worker-0"), 0u);
    EXPECT_EQ(manager_->getCommittedOffset("alo-ack"), last_seq);
}

TEST_F(ConsumerGroupTest, AtLeastOnce_AcknowledgePartialBatch) {
    ConsumerGroupConfig cfg;
    cfg.group_id       = "alo-partial";
    cfg.consumer_count = 1;
    manager_->createGroup(cfg);

    addEvents(6, "p");

    auto events = manager_->fetchEventsAtLeastOnce(
        "alo-partial", "worker-0", *changefeed_, 10, 30000);
    ASSERT_EQ(events.size(), 6u);

    // Ack first 3 events
    uint64_t mid_seq = events[2].sequence;
    manager_->acknowledgeEvents("alo-partial", "worker-0", mid_seq);

    // 3 events should remain in-flight
    EXPECT_EQ(manager_->getInFlightCount("alo-partial", "worker-0"), 3u);
    EXPECT_EQ(manager_->getCommittedOffset("alo-partial"), mid_seq);
}

TEST_F(ConsumerGroupTest, AtLeastOnce_RedeliveryOnTimeout) {
    ConsumerGroupConfig cfg;
    cfg.group_id       = "alo-redeliver";
    cfg.consumer_count = 1;
    manager_->createGroup(cfg);

    addEvents(3, "r");

    // Fetch with a 1 ms ack timeout so events expire immediately
    auto first = manager_->fetchEventsAtLeastOnce(
        "alo-redeliver", "worker-0", *changefeed_, 10, 1 /*ack_timeout_ms*/);
    ASSERT_EQ(first.size(), 3u);

    // Sleep to ensure the timeout has elapsed
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    // Second fetch should redeliver the same events
    auto second = manager_->fetchEventsAtLeastOnce(
        "alo-redeliver", "worker-0", *changefeed_, 10, 1 /*ack_timeout_ms*/);
    ASSERT_EQ(second.size(), 3u);

    // Redelivered sequences must match the originals
    std::vector<uint64_t> first_seqs, second_seqs;
    for (const auto& ev : first) {
      first_seqs.push_back(ev.sequence);
    }
    for (const auto& ev : second) {
      second_seqs.push_back(ev.sequence);
    }
    std::sort(first_seqs.begin(),  first_seqs.end());
    std::sort(second_seqs.begin(), second_seqs.end());
    EXPECT_EQ(first_seqs, second_seqs);
}

TEST_F(ConsumerGroupTest, AtLeastOnce_NoRedeliveryBeforeTimeout) {
    ConsumerGroupConfig cfg;
    cfg.group_id       = "alo-no-redeliver";
    cfg.consumer_count = 1;
    manager_->createGroup(cfg);

    addEvents(3, "nr");

    // Long timeout — events should not be redelivered
    auto first = manager_->fetchEventsAtLeastOnce(
        "alo-no-redeliver", "worker-0", *changefeed_, 10, 60000);
    ASSERT_EQ(first.size(), 3u);

    // Immediate second fetch should return zero new events and zero redeliveries
    auto second = manager_->fetchEventsAtLeastOnce(
        "alo-no-redeliver", "worker-0", *changefeed_, 10, 60000);
    EXPECT_TRUE(second.empty());
}

TEST_F(ConsumerGroupTest, AtLeastOnce_AcknowledgedEventsNotRedelivered) {
    ConsumerGroupConfig cfg;
    cfg.group_id       = "alo-no-redel-acked";
    cfg.consumer_count = 1;
    manager_->createGroup(cfg);

    addEvents(3, "ack");

    // Fetch with 1 ms timeout
    auto events = manager_->fetchEventsAtLeastOnce(
        "alo-no-redel-acked", "worker-0", *changefeed_, 10, 1);
    ASSERT_EQ(events.size(), 3u);

    // Acknowledge all events
    manager_->acknowledgeEvents("alo-no-redel-acked", "worker-0",
                                 events.back().sequence);

    // Wait for timeout
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    // No redelivery expected since all events were acknowledged
    auto recheck = manager_->fetchEventsAtLeastOnce(
        "alo-no-redel-acked", "worker-0", *changefeed_, 10, 1);
    EXPECT_TRUE(recheck.empty());
}

TEST_F(ConsumerGroupTest, AtLeastOnce_GetInFlightStats) {
    ConsumerGroupConfig cfg;
    cfg.group_id       = "alo-stats";
    cfg.consumer_count = 1;
    manager_->createGroup(cfg);

    addEvents(4, "s");

    manager_->fetchEventsAtLeastOnce("alo-stats", "worker-0", *changefeed_, 10, 1);

    // Sleep so all in-flight events become overdue
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    InFlightStats stats = manager_->getInFlightStats("alo-stats", "worker-0", 1);
    EXPECT_EQ(stats.group_id, "alo-stats");
    EXPECT_EQ(stats.consumer_id, "worker-0");
    EXPECT_EQ(stats.inflight_count, 4u);
    EXPECT_EQ(stats.overdue_count, 4u);
    EXPECT_GT(stats.oldest_inflight_sequence, 0u);
}

TEST_F(ConsumerGroupTest, AtLeastOnce_NonExistentGroupThrows) {
    EXPECT_THROW(
        manager_->fetchEventsAtLeastOnce("ghost", "w0", *changefeed_, 10),
        CDCException);
    EXPECT_THROW(
        manager_->acknowledgeEvents("ghost", "w0", 1),
        CDCException);
}

TEST_F(ConsumerGroupTest, AtLeastOnce_EmptyConsumerIdThrows) {
    ConsumerGroupConfig cfg;
    cfg.group_id       = "alo-empty-consumer";
    cfg.consumer_count = 1;
    manager_->createGroup(cfg);

    EXPECT_THROW(
        manager_->fetchEventsAtLeastOnce("alo-empty-consumer", "", *changefeed_, 10),
        CDCException);
    EXPECT_THROW(
        manager_->acknowledgeEvents("alo-empty-consumer", "", 1),
        CDCException);
}

TEST_F(ConsumerGroupTest, AtLeastOnce_DeleteGroupClearsInflightState) {
    ConsumerGroupConfig cfg;
    cfg.group_id       = "alo-delete-group";
    cfg.consumer_count = 1;
    manager_->createGroup(cfg);

    addEvents(3, "dg");

    // Fetch so there is in-flight state
    manager_->fetchEventsAtLeastOnce("alo-delete-group", "worker-0", *changefeed_, 10);
    ASSERT_EQ(manager_->getInFlightCount("alo-delete-group", "worker-0"), 3u);

    // Delete group: in-flight state must be cleared
    manager_->deleteGroup("alo-delete-group");
    EXPECT_EQ(manager_->getInFlightCount("alo-delete-group", "worker-0"), 0u);
}

TEST_F(ConsumerGroupTest, AtLeastOnce_AcknowledgeDoesNotGoBackward) {
    ConsumerGroupConfig cfg;
    cfg.group_id       = "alo-no-backward";
    cfg.consumer_count = 1;
    manager_->createGroup(cfg);

    addEvents(5, "nb");

    auto events = manager_->fetchEventsAtLeastOnce(
        "alo-no-backward", "worker-0", *changefeed_, 10, 30000);
    ASSERT_GE(events.size(), 5u);

    uint64_t last_seq = events.back().sequence;
    manager_->acknowledgeEvents("alo-no-backward", "worker-0", last_seq);
    ASSERT_EQ(manager_->getCommittedOffset("alo-no-backward"), last_seq);

    // Acknowledge an earlier sequence — offset must not regress
    manager_->acknowledgeEvents("alo-no-backward", "worker-0", 1);
    EXPECT_EQ(manager_->getCommittedOffset("alo-no-backward"), last_seq);
}

// ============================================================
// JSON serialisation helpers
// ============================================================

TEST(ConsumerGroupConfigTest, JsonRoundTrip) {
    ConsumerGroupConfig cfg;
    cfg.group_id       = "json-test";
    cfg.consumer_count = 7;

    nlohmann::json j = cfg.toJson();
    ConsumerGroupConfig cfg2 = ConsumerGroupConfig::fromJson(j);

    EXPECT_EQ(cfg2.group_id, "json-test");
    EXPECT_EQ(cfg2.consumer_count, 7u);
}

TEST(ConsumerGroupInfoTest, JsonContainsExpectedFields) {
    ConsumerGroupInfo info;
    info.config.group_id       = "info-json";
    info.config.consumer_count = 2;
    info.committed_sequence    = 99;

    nlohmann::json j = info.toJson();
    EXPECT_TRUE(j.contains("config"));
    EXPECT_TRUE(j.contains("committed_sequence"));
    EXPECT_EQ(j["committed_sequence"].get<uint64_t>(), 99u);
}
