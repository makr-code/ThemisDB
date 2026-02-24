/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_cdc_ws_handler.cpp                            ║
  Module:          cdc                                                ║
  Description:     Unit tests for CdcWebSocketHandler (named          ║
                   subscriptions + at-least-once delivery)             ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "cdc/cdc_ws_handler.h"
#include <nlohmann/json.hpp>

using namespace themis::cdc;
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
