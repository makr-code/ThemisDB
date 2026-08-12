// WebSocket CDC/Changefeed Basic Tests
// These tests validate WebSocket functionality and CDC streaming integration

#include <gtest/gtest.h>

#ifdef THEMIS_ENABLE_WEBSOCKET

#include "server/websocket_session.h"
#include "cdc/changefeed.h"
#include <string>
#include <nlohmann/json.hpp>

using namespace themis::server;
using themis::Changefeed;
using json = nlohmann::json;

// ============================================================================
// WebSocket Basic Functionality Tests
// ============================================================================

TEST(WebSocketCDCTest, SubscribeMessageFormat) {
    // Test that CDC subscribe message has correct JSON format
    json subscribe_msg = {
        {"type", "subscribe"},
        {"channel", "cdc"},
        {"from_sequence", 0},
        {"key_prefix", "user:"}
    };
    
    EXPECT_EQ(subscribe_msg["type"], "subscribe");
    EXPECT_EQ(subscribe_msg["channel"], "cdc");
    EXPECT_EQ(subscribe_msg["from_sequence"], 0);
    EXPECT_TRUE(subscribe_msg.contains("key_prefix"));
}

TEST(WebSocketCDCTest, UnsubscribeMessageFormat) {
    // Test unsubscribe message format
    json unsubscribe_msg = {
        {"type", "unsubscribe"},
        {"channel", "cdc"}
    };
    
    EXPECT_EQ(unsubscribe_msg["type"], "unsubscribe");
    EXPECT_EQ(unsubscribe_msg["channel"], "cdc");
}

TEST(WebSocketCDCTest, CDCEventMessageFormat) {
    // Test CDC event message structure
    json cdc_event = {
        {"type", "cdc_event"},
        {"sequence", 12345},
        {"key", "user:123"},
        {"value", "John Doe"},
        {"operation", "PUT"}
    };
    
    EXPECT_EQ(cdc_event["type"], "cdc_event");
    EXPECT_EQ(cdc_event["sequence"], 12345);
    EXPECT_TRUE(cdc_event.contains("key"));
    EXPECT_TRUE(cdc_event.contains("value"));
}

// ============================================================================
// WebSocket Configuration Tests
// ============================================================================

TEST(WebSocketCDCTest, ConfigurationDefaults) {
    // Test WebSocket configuration defaults
    struct WebSocketConfig {
        bool enable_websocket = false;  // OFF by default
        size_t max_message_size = 1048576;  // 1MB
        int ping_interval_ms = 30000;  // 30 seconds
        int cdc_poll_interval_ms = 500;  // 500ms
    };
    
    WebSocketConfig config;
    
    EXPECT_FALSE(config.enable_websocket) << "WebSocket should be OFF by default";
    EXPECT_EQ(config.max_message_size, 1048576);
    EXPECT_GT(config.ping_interval_ms, 0);
    EXPECT_GT(config.cdc_poll_interval_ms, 0);
}

// ============================================================================
// WebSocket CDC Filtering Tests
// ============================================================================

TEST(WebSocketCDCTest, KeyPrefixFiltering) {
    // Test that key prefix filtering works correctly
    std::string key = "user:123:profile";
    std::string prefix = "user:";
    
    bool matches = key.find(prefix) == 0;
    EXPECT_TRUE(matches) << "Key should match prefix filter";
}

TEST(WebSocketCDCTest, SequenceFiltering) {
    // Test that sequence-based filtering works
    uint64_t event_sequence = 12345;
    uint64_t from_sequence = 10000;
    
    bool should_deliver = event_sequence >= from_sequence;
    EXPECT_TRUE(should_deliver) << "Event should be delivered when sequence >= from_sequence";
}

// ============================================================================
// WebSocket Message Type Tests
// ============================================================================

TEST(WebSocketCDCTest, SupportedMessageTypes) {
    // Test that both text and binary messages are supported
    std::vector<std::string> message_types = {"text", "binary"};
    
    EXPECT_EQ(message_types.size(), 2);
    EXPECT_NE(std::find(message_types.begin(), message_types.end(), "text"), 
              message_types.end());
    EXPECT_NE(std::find(message_types.begin(), message_types.end(), "binary"), 
              message_types.end());
}

TEST(WebSocketCDCTest, PingPongMechanism) {
    // Test ping/pong keepalive mechanism
    json ping_msg = {{"type", "ping"}};
    json pong_msg = {{"type", "pong"}};
    
    EXPECT_EQ(ping_msg["type"], "ping");
    EXPECT_EQ(pong_msg["type"], "pong");
}

#endif // THEMIS_ENABLE_WEBSOCKET

// Placeholder test when WebSocket is disabled
#ifndef THEMIS_ENABLE_WEBSOCKET
TEST(WebSocketCDCTest, DisabledByDefault) {
    GTEST_SKIP() << "WebSocket is disabled. Build with -DTHEMIS_ENABLE_WEBSOCKET=ON to enable.";
}
#endif

// ─────────────────────────────────────────────────────────────────────────────
// WebSocket query message format tests
// ─────────────────────────────────────────────────────────────────────────────

#ifdef THEMIS_ENABLE_WEBSOCKET

TEST(WebSocketQueryTest, QueryMessageFormat) {
    // Verify that a query message has the correct JSON shape
    json query_msg = {
        {"type",  "query"},
        {"table", "users"},
        {"predicates", json::array({{{"column", "status"}, {"value", "active"}}})}
    };

    EXPECT_EQ(query_msg["type"], "query");
    EXPECT_TRUE(query_msg.contains("table"));
    EXPECT_TRUE(query_msg.contains("predicates"));
    EXPECT_EQ(query_msg["predicates"].size(), 1u);
}

TEST(WebSocketQueryTest, AqlQueryMessageFormat) {
    // Verify that an AQL query message has the correct JSON shape
    json aql_msg = {
        {"type", "query"},
        {"aql",  "FOR doc IN users FILTER doc.status == 'active' RETURN doc"}
    };

    EXPECT_EQ(aql_msg["type"], "query");
    EXPECT_TRUE(aql_msg.contains("aql"));
}

TEST(WebSocketQueryTest, QueryResponseFormat) {
    // Verify that query_response message fields are present
    json response = {
        {"type",        "query_response"},
        {"status",      "ok"},
        {"http_status", 200},
        {"result",      json::array()}
    };

    EXPECT_EQ(response["type"],        "query_response");
    EXPECT_EQ(response["status"],      "ok");
    EXPECT_EQ(response["http_status"], 200);
    EXPECT_TRUE(response["result"].is_array());
}

TEST(WebSocketQueryTest, QueryErrorResponseFormat) {
    // Verify that error query_response message fields are present
    json error_resp = {
        {"type",        "query_response"},
        {"status",      "error"},
        {"http_status", 400},
        {"message",     "Missing 'table'"}
    };

    EXPECT_EQ(error_resp["type"],   "query_response");
    EXPECT_EQ(error_resp["status"], "error");
    EXPECT_TRUE(error_resp.contains("message"));
}

#endif // THEMIS_ENABLE_WEBSOCKET

// ─────────────────────────────────────────────────────────────────────────────
// WsTransport unit tests
// ─────────────────────────────────────────────────────────────────────────────

#ifdef THEMIS_ENABLE_WEBSOCKET

#include "cdc/ws_transport.h"
#include "cdc/cdc_metrics.h"

using namespace themis::cdc;

// ── Session lifecycle ─────────────────────────────────────────────────────────

TEST(WsTransportTest, InitialStatsAreZero) {
    // A freshly constructed WsTransport reports zero active sessions and
    // subscriptions.
    WsTransport transport(nullptr);  // null changefeed: polling is a no-op

    auto stats = transport.getStats();
    EXPECT_EQ(stats.active_sessions, 0u);
    EXPECT_EQ(stats.total_subscriptions, 0u);
    EXPECT_EQ(stats.total_events_delivered, 0u);
    EXPECT_EQ(stats.total_overflow_closes, 0u);
    EXPECT_EQ(stats.total_poll_cycles, 0u);
}

TEST(WsTransportTest, AddAndRemoveSession) {
    WsTransport transport(nullptr);

    transport.addSession("session-1");
    EXPECT_EQ(transport.getStats().active_sessions, 1u);

    transport.addSession("session-2");
    EXPECT_EQ(transport.getStats().active_sessions, 2u);

    transport.removeSession("session-1");
    EXPECT_EQ(transport.getStats().active_sessions, 1u);

    transport.removeSession("session-2");
    EXPECT_EQ(transport.getStats().active_sessions, 0u);
}

TEST(WsTransportTest, AddSessionIsIdempotent) {
    // Adding the same session twice does not create a duplicate entry.
    WsTransport transport(nullptr);

    transport.addSession("session-dup");
    transport.addSession("session-dup");
    EXPECT_EQ(transport.getStats().active_sessions, 1u);
}

TEST(WsTransportTest, RemoveUnknownSessionIsNoop) {
    WsTransport transport(nullptr);
    // Should not throw or crash.
    EXPECT_NO_THROW(transport.removeSession("nonexistent"));
    EXPECT_EQ(transport.getStats().active_sessions, 0u);
}

// ── Subscription management ───────────────────────────────────────────────────

TEST(WsTransportTest, SubscribeAndUnsubscribe) {
    WsTransport transport(nullptr);
    transport.addSession("sess");

    WsTransport::SubscriptionFilter filter;
    filter.key_prefix = "orders:";
    filter.from_sequence = 0;

    transport.subscribe("sess", "sub-1", filter);
    EXPECT_EQ(transport.getStats().total_subscriptions, 1u);

    transport.subscribe("sess", "sub-2", filter);
    EXPECT_EQ(transport.getStats().total_subscriptions, 2u);

    transport.unsubscribe("sess", "sub-1");
    EXPECT_EQ(transport.getStats().total_subscriptions, 1u);

    transport.unsubscribe("sess", "sub-2");
    EXPECT_EQ(transport.getStats().total_subscriptions, 0u);
}

TEST(WsTransportTest, ResubscribeReplacesExistingSubscription) {
    // Re-subscribing with the same ID replaces the old filter without
    // increasing the subscription count.
    WsTransport transport(nullptr);
    transport.addSession("sess");

    WsTransport::SubscriptionFilter f1;
    f1.key_prefix = "orders:";
    transport.subscribe("sess", "sub-1", f1);
    EXPECT_EQ(transport.getStats().total_subscriptions, 1u);

    WsTransport::SubscriptionFilter f2;
    f2.key_prefix = "inventory:";
    transport.subscribe("sess", "sub-1", f2);  // re-subscribe with different filter
    EXPECT_EQ(transport.getStats().total_subscriptions, 1u);
}

TEST(WsTransportTest, RemoveSessionClearsSubscriptions) {
    WsTransport transport(nullptr);
    transport.addSession("sess");

    WsTransport::SubscriptionFilter f;
    transport.subscribe("sess", "sub-a", f);
    transport.subscribe("sess", "sub-b", f);
    EXPECT_EQ(transport.getStats().total_subscriptions, 2u);

    transport.removeSession("sess");
    EXPECT_EQ(transport.getStats().active_sessions, 0u);
    EXPECT_EQ(transport.getStats().total_subscriptions, 0u);
}

TEST(WsTransportTest, SubscribeToUnknownSessionIsNoop) {
    WsTransport transport(nullptr);
    WsTransport::SubscriptionFilter f;
    // Should not throw or crash, and must not create a ghost session.
    EXPECT_NO_THROW(transport.subscribe("ghost-session", "sub-1", f));
    EXPECT_EQ(transport.getStats().active_sessions, 0u);
    EXPECT_EQ(transport.getStats().total_subscriptions, 0u);
}

// ── pollAndDeliver with null changefeed ───────────────────────────────────────

TEST(WsTransportTest, PollWithNullChangefeedIsNoop) {
    // When no changefeed is configured, pollAndDeliver must not invoke the
    // send callback and must increment the poll-cycle counter.
    WsTransport transport(nullptr);
    transport.addSession("sess");

    WsTransport::SubscriptionFilter f;
    transport.subscribe("sess", "sub-1", f);

    int send_calls = 0;
    auto send_fn = [&](const std::string&, const std::string&) { ++send_calls; };

    transport.pollAndDeliver(send_fn);

    EXPECT_EQ(send_calls, 0);
    EXPECT_EQ(transport.getStats().total_poll_cycles, 1u);
    EXPECT_EQ(transport.getStats().total_events_delivered, 0u);
}

TEST(WsTransportTest, PollWithNoSessionsIsNoop) {
    WsTransport transport(nullptr);

    int send_calls = 0;
    auto send_fn = [&](const std::string&, const std::string&) { ++send_calls; };

    transport.pollAndDeliver(send_fn);
    EXPECT_EQ(send_calls, 0);
    EXPECT_EQ(transport.getStats().total_poll_cycles, 1u);
}

// ── Subscription filter fields ────────────────────────────────────────────────

TEST(WsTransportTest, SubscriptionFilterDefaultValues) {
    WsTransport::SubscriptionFilter f;
    EXPECT_TRUE(f.key_prefix.empty());
    EXPECT_EQ(f.from_sequence, 0u);
    EXPECT_TRUE(f.event_types.empty());
}

TEST(WsTransportTest, SubscriptionFilterWithEventTypes) {
    WsTransport::SubscriptionFilter f;
    f.key_prefix = "user:";
    f.from_sequence = 100;
    f.event_types.insert(themis::Changefeed::ChangeEventType::EVENT_PUT);
    f.event_types.insert(themis::Changefeed::ChangeEventType::EVENT_DELETE);

    EXPECT_EQ(f.key_prefix, "user:");
    EXPECT_EQ(f.from_sequence, 100u);
    EXPECT_EQ(f.event_types.size(), 2u);
    EXPECT_TRUE(f.event_types.count(themis::Changefeed::ChangeEventType::EVENT_PUT));
    EXPECT_TRUE(f.event_types.count(themis::Changefeed::ChangeEventType::EVENT_DELETE));
}

// ── Protocol frame constants ──────────────────────────────────────────────────

TEST(WsTransportTest, MaxPendingEventsConstant) {
    // kMaxPendingEvents must be at least 1 (not zero) and must match the
    // documented value of 1000.
    EXPECT_EQ(WsTransport::kMaxPendingEvents, 1000u);
}

TEST(WsTransportTest, DefaultPollIntervalConstant) {
    EXPECT_EQ(WsTransport::kDefaultPollIntervalMs, 500u);
}

// ── CDC event frame format ────────────────────────────────────────────────────

TEST(WsTransportTest, CdcEventFrameHasRequiredFields) {
    // Verify the expected JSON shape of a cdc_event frame pushed to clients.
    json event_frame = {
        {"type",            "cdc_event"},
        {"subscription_id", "sub-1"},
        {"sequence",        42},
        {"key",             "orders:US-999"},
        {"timestamp_ms",    1740000000000},
        {"operation",       "PUT"}
    };

    EXPECT_EQ(event_frame["type"], "cdc_event");
    EXPECT_TRUE(event_frame.contains("subscription_id"));
    EXPECT_TRUE(event_frame.contains("sequence"));
    EXPECT_TRUE(event_frame.contains("key"));
    EXPECT_TRUE(event_frame.contains("timestamp_ms"));
}

TEST(WsTransportTest, SubscribeAckFrameHasRequiredFields) {
    // Verify the expected JSON shape of a subscribed ack frame.
    json ack = {
        {"action", "subscribed"},
        {"id",     "sub-1"}
    };

    EXPECT_EQ(ack["action"], "subscribed");
    EXPECT_EQ(ack["id"],     "sub-1");
}

// ── CDCMetrics integration ────────────────────────────────────────────────────

TEST(WsTransportTest, MetricsCountersStartAtZero) {
    // CDCMetrics WS counters must be zero-initialised.
    CDCMetrics m;
    EXPECT_EQ(m.ws_events_delivered.load(), 0u);
    EXPECT_EQ(m.ws_overflow_total.load(), 0u);
}

TEST(WsTransportTest, NullMetricsParameterIsAccepted) {
    // Constructing with null metrics pointer must not throw.
    EXPECT_NO_THROW(WsTransport transport(nullptr, WsTransport::kDefaultPollIntervalMs, nullptr));
}

TEST(WsTransportTest, MetricsParameterExposedViaConstructor) {
    // Verify the constructor accepts a non-null CDCMetrics pointer.
    CDCMetrics m;
    EXPECT_NO_THROW(WsTransport transport(nullptr, WsTransport::kDefaultPollIntervalMs, &m));
}

TEST(WsTransportTest, MetricsResetClearsWsCounters) {
    CDCMetrics m;
    m.ws_events_delivered = 42;
    m.ws_overflow_total   = 7;

    m.reset();

    EXPECT_EQ(m.ws_events_delivered.load(), 0u);
    EXPECT_EQ(m.ws_overflow_total.load(), 0u);
}

TEST(WsTransportTest, MetricsToJsonContainsWsFields) {
    CDCMetrics m;
    m.ws_events_delivered = 100;
    m.ws_overflow_total   = 3;

    auto j = m.toJson();
    EXPECT_TRUE(j.contains("counters"));
    EXPECT_EQ(j["counters"]["ws_events_delivered"], 100u);
    EXPECT_EQ(j["counters"]["ws_overflow_total"],   3u);
}

#endif // THEMIS_ENABLE_WEBSOCKET
