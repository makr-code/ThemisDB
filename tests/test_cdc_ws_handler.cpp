/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_cdc_ws_handler.cpp                            ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-03-02 04:02:44                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     311                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 33a346e4e  2026-02-25  Refactor code structure and remove redundant code blocks ... ║
    • 13aae88f8  2026-02-24  fix(cdc): audit fixes — cdc_ws_overflow_total metric, cdc... ║
    • 6d03c85c7  2026-02-24  feat(cdc): WebSocket transport for /v2/cdc/stream with at... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "cdc/cdc_ws_handler.h"
#include "cdc/changefeed.h"
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
        test_db_path_ = "./data/test_cdc_ws_handler_overflow";
        std::filesystem::remove_all(test_db_path_);

        RocksDBWrapper::Config cfg;
        cfg.db_path = test_db_path_;
        db_ = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());

        changefeed_ = std::make_unique<themis::Changefeed>(db_->getDB(), nullptr);
    }

    void TearDown() override {
        changefeed_.reset();
        db_->close();
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
