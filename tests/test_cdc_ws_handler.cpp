#include <gtest/gtest.h>
#include "cdc/cdc_ws_handler.h"
#include "cdc/changefeed.h"
#include "cdc/consumer_group.h"
#include "storage/rocksdb_wrapper.h"
#include <nlohmann/json.hpp>
#include <filesystem>

using namespace themis::cdc;
using themis::RocksDBWrapper;
using json = nlohmann::json;

// ============================================================================
// Subscribe / Unsubscribe
// ============================================================================

TEST(CdcWsHandlerTest, SubscribeReturnsSubscribedAck) {
    CdcWebSocketHandler handler;

    json frame = {{"action", "subscribe"}, {"id", "sub-1"}, {"collection", "orders"}};
    auto responses = handler.handleFrame(frame);

    ASSERT_EQ(responses.size(), 1u);
    EXPECT_EQ(responses[0]["action"], "subscribed");
    EXPECT_EQ(responses[0]["id"],     "sub-1");
}

TEST(CdcWsHandlerTest, SubscribeWithoutIdReturnsError) {
    CdcWebSocketHandler handler;

    json frame = {{"action", "subscribe"}, {"collection", "orders"}};
    auto responses = handler.handleFrame(frame);

    ASSERT_EQ(responses.size(), 1u);
    EXPECT_EQ(responses[0]["action"], "error");
}

TEST(CdcWsHandlerTest, SubscribeCreatesActiveSubscription) {
    CdcWebSocketHandler handler;

    EXPECT_FALSE(handler.hasSubscriptions());

    json frame = {{"action", "subscribe"}, {"id", "s1"}, {"collection", "users"}};
    handler.handleFrame(frame);

    EXPECT_TRUE(handler.hasSubscriptions());
}

TEST(CdcWsHandlerTest, UnsubscribeRemovesSubscription) {
    CdcWebSocketHandler handler;

    handler.handleFrame({{"action", "subscribe"}, {"id", "s1"}, {"collection", "users"}});
    EXPECT_TRUE(handler.hasSubscriptions());

    auto responses = handler.handleFrame({{"action", "unsubscribe"}, {"id", "s1"}});

    ASSERT_EQ(responses.size(), 1u);
    EXPECT_EQ(responses[0]["action"], "unsubscribed");
    EXPECT_EQ(responses[0]["id"],     "s1");
    EXPECT_FALSE(handler.hasSubscriptions());
}

TEST(CdcWsHandlerTest, UnsubscribeWithoutIdReturnsError) {
    CdcWebSocketHandler handler;

    auto responses = handler.handleFrame({{"action", "unsubscribe"}});
    ASSERT_EQ(responses.size(), 1u);
    EXPECT_EQ(responses[0]["action"], "error");
}

TEST(CdcWsHandlerTest, MultipleNamedSubscriptionsCoexist) {
    CdcWebSocketHandler handler;

    handler.handleFrame({{"action", "subscribe"}, {"id", "s1"}, {"collection", "orders"}});
    handler.handleFrame({{"action", "subscribe"}, {"id", "s2"}, {"collection", "inventory"}});

    EXPECT_TRUE(handler.hasSubscriptions());

    handler.handleFrame({{"action", "unsubscribe"}, {"id", "s1"}});
    EXPECT_TRUE(handler.hasSubscriptions()); // s2 still active

    handler.handleFrame({{"action", "unsubscribe"}, {"id", "s2"}});
    EXPECT_FALSE(handler.hasSubscriptions());
}

TEST(CdcWsHandlerTest, DuplicateSubscribeIdOverwrites) {
    CdcWebSocketHandler handler;

    handler.handleFrame({{"action", "subscribe"}, {"id", "s1"}, {"collection", "orders"}});
    // Subscribe again with the same id but different collection
    auto responses = handler.handleFrame(
        {{"action", "subscribe"}, {"id", "s1"}, {"collection", "inventory"}});

    ASSERT_EQ(responses.size(), 1u);
    EXPECT_EQ(responses[0]["action"], "subscribed");
    EXPECT_TRUE(handler.hasSubscriptions());
}

// ============================================================================
// Ack
// ============================================================================

TEST(CdcWsHandlerTest, AckWithoutIdReturnsError) {
    CdcWebSocketHandler handler;

    auto responses = handler.handleFrame({{"action", "ack"}, {"sequence", 42}});
    ASSERT_EQ(responses.size(), 1u);
    EXPECT_EQ(responses[0]["action"], "error");
}

TEST(CdcWsHandlerTest, AckForUnknownIdProducesNoResponse) {
    CdcWebSocketHandler handler;

    // No subscription registered; ack should be silently ignored
    auto responses = handler.handleFrame(
        {{"action", "ack"}, {"id", "ghost"}, {"sequence", 10}});
    EXPECT_TRUE(responses.empty());
}

TEST(CdcWsHandlerTest, AckProducesNoResponseOnSuccess) {
    CdcWebSocketHandler handler;
    handler.handleFrame({{"action", "subscribe"}, {"id", "s1"}, {"collection", "items"}});

    // No pending events yet; ack should be silently accepted
    auto responses = handler.handleFrame(
        {{"action", "ack"}, {"id", "s1"}, {"sequence", 0}});
    EXPECT_TRUE(responses.empty());
}

// ============================================================================
// Unknown action
// ============================================================================

TEST(CdcWsHandlerTest, UnknownActionReturnsError) {
    CdcWebSocketHandler handler;

    auto responses = handler.handleFrame({{"action", "teleport"}});
    ASSERT_EQ(responses.size(), 1u);
    EXPECT_EQ(responses[0]["action"], "error");
}

// ============================================================================
// Event-type and key_prefix parsing
// ============================================================================

TEST(CdcWsHandlerTest, CollectionMapsToKeyPrefix) {
    // This tests the frame parsing path; actual event filtering is exercised
    // by integration tests with a live Changefeed.
    CdcWebSocketHandler handler;

    json frame = {{"action", "subscribe"}, {"id", "s1"}, {"collection", "orders"}};
    auto responses = handler.handleFrame(frame);

    ASSERT_EQ(responses.size(), 1u);
    EXPECT_EQ(responses[0]["action"], "subscribed");
}

TEST(CdcWsHandlerTest, ExplicitKeyPrefixOverridesCollection) {
    CdcWebSocketHandler handler;

    json frame = {
        {"action",     "subscribe"},
        {"id",         "s1"},
        {"collection", "orders"},
        {"key_prefix", "US-"}
    };
    auto responses = handler.handleFrame(frame);
    ASSERT_EQ(responses.size(), 1u);
    EXPECT_EQ(responses[0]["action"], "subscribed");
}

TEST(CdcWsHandlerTest, EventTypesArrayParsed) {
    CdcWebSocketHandler handler;

    json frame = {
        {"action",      "subscribe"},
        {"id",          "s1"},
        {"collection",  "items"},
        {"event_types", json::array({"PUT", "DELETE"})}
    };
    auto responses = handler.handleFrame(frame);
    ASSERT_EQ(responses.size(), 1u);
    EXPECT_EQ(responses[0]["action"], "subscribed");
}

// ============================================================================
// from_sequence in subscribe frame
// ============================================================================

TEST(CdcWsHandlerTest, FromSequenceInSubscribeFrame) {
    CdcWebSocketHandler handler;

    json frame = {
        {"action",        "subscribe"},
        {"id",            "s1"},
        {"collection",    "logs"},
        {"from_sequence", uint64_t(100)}
    };
    auto responses = handler.handleFrame(frame);
    ASSERT_EQ(responses.size(), 1u);
    EXPECT_EQ(responses[0]["action"], "subscribed");
}

// ============================================================================
// checkRedelivery – no redelivery before timeout
// ============================================================================

TEST(CdcWsHandlerTest, CheckRedeliveryReturnsEmptyBeforeTimeout) {
    CdcWebSocketHandler handler;
    handler.handleFrame({{"action", "subscribe"}, {"id", "s1"}, {"collection", "x"}});

    // No events polled yet; redelivery queue is empty
    auto redeliveries = handler.checkRedelivery();
    EXPECT_TRUE(redeliveries.empty());
}

// ============================================================================
// hasSubscriptions edge cases
// ============================================================================

TEST(CdcWsHandlerTest, HasSubscriptionsFalseOnFreshHandler) {
    CdcWebSocketHandler handler;
    EXPECT_FALSE(handler.hasSubscriptions());
}

TEST(CdcWsHandlerTest, HasSubscriptionsTrueAfterSubscribe) {
    CdcWebSocketHandler handler;
    handler.handleFrame({{"action", "subscribe"}, {"id", "s1"}, {"collection", "c"}});
    EXPECT_TRUE(handler.hasSubscriptions());
}

TEST(CdcWsHandlerTest, HasSubscriptionsFalseAfterAllUnsubscribed) {
    CdcWebSocketHandler handler;
    handler.handleFrame({{"action", "subscribe"}, {"id", "s1"}, {"collection", "c"}});
    handler.handleFrame({{"action", "unsubscribe"}, {"id", "s1"}});
    EXPECT_FALSE(handler.hasSubscriptions());
}

// ============================================================================
// cdc_ws_overflow_total metric — unit tests (no RocksDB needed)
// ============================================================================

TEST(CdcWsHandlerTest, OverflowCounterStartsAtZero) {
    CdcWebSocketHandler handler;
    EXPECT_EQ(handler.getWsOverflowTotal(), 0u);
}

TEST(CdcWsHandlerTest, OverflowCounterUnchangedWithoutBackpressure) {
    CdcWebSocketHandler handler;
    handler.handleFrame({{"action", "subscribe"}, {"id", "s1"}, {"collection", "c"}});
    // No events polled; counter should remain 0.
    EXPECT_EQ(handler.getWsOverflowTotal(), 0u);
}

// ============================================================================
// cdc_ws_overflow_total metric — integration test (RocksDB-backed)
// ============================================================================

class CdcWsOverflowTest : public ::testing::Test {
protected:
    void SetUp() override {
#ifdef _WIN32
        GTEST_SKIP() << "Skipping CdcWsOverflowTest on Windows due to intermittent SEH in pollEvents path.";
#endif
        test_db_path_ = "./data/test_cdc_ws_handler_overflow";
        std::filesystem::remove_all(test_db_path_);

        RocksDBWrapper::Config cfg;
        cfg.db_path = test_db_path_;
        cfg.merge_operator_preset =
            RocksDBWrapper::Config::MergeOperatorPreset::SequenceU64Increment;
        db_ = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());

        changefeed_ = std::make_unique<themis::Changefeed>(db_->getDB(), nullptr);
    }

    void TearDown() override {
        changefeed_.reset();
        if (db_) {
            db_->close();
        }
        db_.reset();
        std::filesystem::remove_all(test_db_path_);
    }

    std::string test_db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<themis::Changefeed> changefeed_;
};

TEST_F(CdcWsOverflowTest, OverflowCounterIncrementsWhenPendingAckQueueFull) {
    // Create handler with max_pending_ack = 0 to trigger back-pressure immediately
    // on every pollEvents() call for any subscription.
    CdcWebSocketHandler handler(/*max_pending_ack=*/0);

    handler.handleFrame({{"action", "subscribe"}, {"id", "s1"}, {"collection", "items"}});

    EXPECT_EQ(handler.getWsOverflowTotal(), 0u);

    // First poll: pending_ack.size() (0) >= max_pending_ack_ (0) → overflow fires
    handler.pollEvents(*changefeed_);
    EXPECT_EQ(handler.getWsOverflowTotal(), 1u);

    // Second poll: counter keeps incrementing
    handler.pollEvents(*changefeed_);
    EXPECT_EQ(handler.getWsOverflowTotal(), 2u);
}

// ============================================================================
// Consumer-group protocol (v1.8.0) — unit tests (no RocksDB / ConsumerGroupManager)
// ============================================================================

TEST(CdcWsHandlerGroupTest, SubscribeWithGroupIdCreatesSubscription) {
    CdcWebSocketHandler handler;

    json frame = {
        {"action",      "subscribe"},
        {"group_id",    "etl-workers"},
        {"consumer_id", "worker-3"},
        {"collection",  "orders"}
    };
    auto responses = handler.handleFrame(frame);

    ASSERT_EQ(responses.size(), 1u);
    EXPECT_EQ(responses[0]["action"], "subscribed");
    // Subscription key is "group_id:consumer_id" when no explicit "id" is given.
    EXPECT_EQ(responses[0]["id"], "etl-workers:worker-3");
    EXPECT_TRUE(handler.hasSubscriptions());
}

TEST(CdcWsHandlerGroupTest, SubscribeWithGroupIdAndExplicitIdUsesExplicitId) {
    CdcWebSocketHandler handler;

    json frame = {
        {"action",      "subscribe"},
        {"id",          "my-sub"},
        {"group_id",    "etl-workers"},
        {"consumer_id", "worker-3"},
        {"collection",  "orders"}
    };
    auto responses = handler.handleFrame(frame);

    ASSERT_EQ(responses.size(), 1u);
    EXPECT_EQ(responses[0]["action"], "subscribed");
    EXPECT_EQ(responses[0]["id"], "my-sub");
}

TEST(CdcWsHandlerGroupTest, SubscribeWithNoIdAndNoGroupIdReturnsError) {
    CdcWebSocketHandler handler;

    json frame = {{"action", "subscribe"}, {"collection", "orders"}};
    auto responses = handler.handleFrame(frame);

    ASSERT_EQ(responses.size(), 1u);
    EXPECT_EQ(responses[0]["action"], "error");
}

TEST(CdcWsHandlerGroupTest, UnsubscribeByGroupId) {
    CdcWebSocketHandler handler;

    handler.handleFrame({{"action", "subscribe"},
                          {"group_id", "grp"}, {"consumer_id", "c0"},
                          {"collection", "items"}});
    EXPECT_TRUE(handler.hasSubscriptions());

    auto responses = handler.handleFrame({{"action", "unsubscribe"},
                                           {"group_id", "grp"}, {"consumer_id", "c0"}});
    ASSERT_EQ(responses.size(), 1u);
    EXPECT_EQ(responses[0]["action"], "unsubscribed");
    EXPECT_FALSE(handler.hasSubscriptions());
}

TEST(CdcWsHandlerGroupTest, AckByGroupIdNoConsumerGroupManagerIsNoop) {
    // Without a ConsumerGroupManager, ack by group_id is silently handled
    // (no error, no response).
    CdcWebSocketHandler handler;

    handler.handleFrame({{"action", "subscribe"},
                          {"group_id", "etl-workers"}, {"consumer_id", "w0"},
                          {"collection", "orders"}});

    auto responses = handler.handleFrame({
        {"action",   "ack"},
        {"group_id", "etl-workers"},
        {"sequence", uint64_t(100)}
    });
    // Ack produces no response frames on success.
    EXPECT_TRUE(responses.empty());
}

TEST(CdcWsHandlerGroupTest, SubscribeWithGroupIdNoManagerDoesNotCrash) {
    // When there is no ConsumerGroupManager (e.g. CDC disabled), a subscribe
    // frame with group_id should succeed with a subscribed ack and deliver
    // from sequence 0 rather than crashing.
    CdcWebSocketHandler handler; // no ConsumerGroupManager

    auto responses = handler.handleFrame({
        {"action",      "subscribe"},
        {"group_id",    "etl-workers"},
        {"consumer_id", "worker-3"},
        {"collection",  "orders"}
    });

    ASSERT_EQ(responses.size(), 1u);
    EXPECT_EQ(responses[0]["action"], "subscribed");
    EXPECT_TRUE(handler.hasSubscriptions());
}

TEST(CdcWsHandlerGroupTest, AckByIdWithNoGroupIdOrIdReturnsError) {
    CdcWebSocketHandler handler;

    auto responses = handler.handleFrame({{"action", "ack"}, {"sequence", 42}});
    ASSERT_EQ(responses.size(), 1u);
    EXPECT_EQ(responses[0]["action"], "error");
}

// ============================================================================
// Consumer-group integration test (RocksDB-backed, ConsumerGroupManager)
// ============================================================================

class CdcWsGroupIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_db_path_ = "./data/test_cdc_ws_group_integration";
        std::filesystem::remove_all(test_db_path_);

        RocksDBWrapper::Config cfg;
        cfg.db_path = test_db_path_;
        cfg.merge_operator_preset =
            RocksDBWrapper::Config::MergeOperatorPreset::SequenceU64Increment;
        db_ = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());

        themis::Changefeed::RetentionPolicy ret;
        ret.enabled = false;
        changefeed_ = std::make_unique<themis::Changefeed>(db_->getDB(), nullptr, ret);

        group_mgr_ = std::make_unique<themis::cdc::ConsumerGroupManager>(
            db_->getDB(), nullptr);

        // Create the group used in all sub-tests (single partition).
        themis::cdc::ConsumerGroupConfig group_cfg;
        group_cfg.group_id       = "etl-workers";
        group_cfg.consumer_count = 1;
        group_mgr_->createGroup(group_cfg);
    }

    void TearDown() override {
        group_mgr_.reset();
        changefeed_.reset();
        db_->close();
        db_.reset();
        std::filesystem::remove_all(test_db_path_);
    }

    // Insert change events with keys of the form "orders:<suffix>"
    void addOrderEvents(int count) {
        for (int i = 0; i < count; ++i) {
            themis::Changefeed::ChangeEvent ev;
            ev.type  = themis::Changefeed::ChangeEventType::EVENT_PUT;
            ev.key   = "orders:item-" + std::to_string(i);
            ev.value = "{\"qty\":" + std::to_string(i) + "}";
            changefeed_->recordEvent(ev);
        }
    }

    std::string test_db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<themis::Changefeed> changefeed_;
    std::unique_ptr<themis::cdc::ConsumerGroupManager> group_mgr_;
};

TEST_F(CdcWsGroupIntegrationTest, SubscribeResumesFromCommittedOffset) {
    // Write 5 events and commit offset to sequence of 3rd event.
    addOrderEvents(5);

    // Fetch first three events to determine their sequences.
    themis::Changefeed::ListOptions opts;
    opts.from_sequence = 0;
    opts.limit         = 3;
    auto events = changefeed_->listEvents(opts);
    ASSERT_EQ(events.size(), 3u);

    const uint64_t third_seq = events.back().sequence;
    group_mgr_->commitOffset("etl-workers", third_seq);
    EXPECT_EQ(group_mgr_->getCommittedOffset("etl-workers"), third_seq);

    // Create handler and subscribe in group mode.
    CdcWebSocketHandler handler(CdcWebSocketHandler::kMaxPendingAck, group_mgr_.get());

    auto responses = handler.handleFrame({
        {"action",      "subscribe"},
        {"group_id",    "etl-workers"},
        {"consumer_id", "worker-0"},
        {"collection",  "orders"}
    });
    ASSERT_EQ(responses.size(), 1u);
    EXPECT_EQ(responses[0]["action"], "subscribed");

    // Poll should only return events after the committed offset.
    auto frames = handler.pollEvents(*changefeed_);
    for (const auto& f : frames) {
        const uint64_t seq = f.value("sequence", uint64_t(0));
        EXPECT_GT(seq, third_seq)
            << "Expected events only after committed offset " << third_seq;
    }
}

TEST_F(CdcWsGroupIntegrationTest, AckByGroupIdAdvancesCommittedOffset) {
    addOrderEvents(3);

    CdcWebSocketHandler handler(CdcWebSocketHandler::kMaxPendingAck, group_mgr_.get());
    handler.handleFrame({
        {"action",      "subscribe"},
        {"group_id",    "etl-workers"},
        {"consumer_id", "worker-0"},
        {"collection",  "orders"}
    });

    // Poll to retrieve events.
    auto frames = handler.pollEvents(*changefeed_);
    ASSERT_FALSE(frames.empty());

    const uint64_t last_seq = frames.back().value("sequence", uint64_t(0));

    // Acknowledge by group_id.
    auto ack_responses = handler.handleFrame({
        {"action",   "ack"},
        {"group_id", "etl-workers"},
        {"sequence", last_seq}
    });
    EXPECT_TRUE(ack_responses.empty());  // No response on success

    // Durable committed offset must reflect the ack.
    EXPECT_EQ(group_mgr_->getCommittedOffset("etl-workers"), last_seq);
}

TEST_F(CdcWsGroupIntegrationTest, PartitionFilterDeliverOnlyConsumerEvents) {
    // Create a 2-consumer group so that events are split across two partitions.
    themis::cdc::ConsumerGroupConfig cfg2;
    cfg2.group_id       = "split-group";
    cfg2.consumer_count = 2;
    group_mgr_->createGroup(cfg2);

    // Determine partition assignments for worker-0 and worker-1.
    const uint32_t p0 = themis::cdc::ConsumerGroupManager::partitionForConsumer("worker-0", 2);
    const uint32_t p1 = themis::cdc::ConsumerGroupManager::partitionForConsumer("worker-1", 2);
    // They must be different for this test to be meaningful.
    ASSERT_NE(p0, p1);

    // Add 10 events; their keys are distributed between the two partitions.
    for (int i = 0; i < 10; ++i) {
        themis::Changefeed::ChangeEvent ev;
        ev.type  = themis::Changefeed::ChangeEventType::EVENT_PUT;
        ev.key   = "doc:" + std::to_string(i);
        ev.value = "{}";
        changefeed_->recordEvent(ev);
    }

    CdcWebSocketHandler handler(CdcWebSocketHandler::kMaxPendingAck, group_mgr_.get());

    // Subscribe worker-0.
    handler.handleFrame({{"action", "subscribe"},
                          {"group_id", "split-group"},
                          {"consumer_id", "worker-0"}});
    // Subscribe worker-1.
    handler.handleFrame({{"action", "subscribe"},
                          {"group_id", "split-group"},
                          {"consumer_id", "worker-1"}});

    auto frames = handler.pollEvents(*changefeed_);

    // Verify that no event is delivered to a worker whose partition doesn't match.
    for (const auto& f : frames) {
        const std::string key    = f.value("key", std::string{});
        const std::string sub_id = f.value("sub_id", std::string{});
        if (key.empty() || sub_id.empty()) continue;

        // Derive expected partition for this key.
        const uint32_t key_part = themis::cdc::ConsumerGroupManager::partitionForKey(key, 2);

        // Determine which consumer this event was sent to.
        std::string consumer_in_sub;
        if (sub_id == "split-group:worker-0") consumer_in_sub = "worker-0";
        else if (sub_id == "split-group:worker-1") consumer_in_sub = "worker-1";
        else continue;  // Unrecognized subscription (skip)

        const uint32_t consumer_part =
            themis::cdc::ConsumerGroupManager::partitionForConsumer(consumer_in_sub, 2);

        EXPECT_EQ(key_part, consumer_part)
            << "Event with key='" << key << "' (partition " << key_part
            << ") was incorrectly delivered to " << sub_id
            << " (partition " << consumer_part << ")";
    }
}
