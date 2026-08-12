/**
 * @file test_metrics_stream_server.cpp
 * @brief Unit tests for MetricsStreamServer — real-time metric streaming.
 *
 * Acceptance criteria (from issue #82):
 * - MetricsStreamServer can be started and stopped.
 * - subscribe() / unsubscribe() manage subscriptions correctly.
 * - pushMetrics() delivers updates to matching subscribers.
 * - Label filters (AND semantics) and metric-name filtering are enforced.
 * - Per-subscription rate limiting (update_interval) is enforced.
 * - Stats counters are accurate.
 * - formatWebSocketMessage() and formatSseMessage() produce correct output.
 * - Thread safety: concurrent push from multiple threads.
 */

#include <gtest/gtest.h>
#include "observability/metrics_stream_server.h"

#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

using namespace themis::observability;
using namespace std::chrono_literals;

// ============================================================================
// Fixture
// ============================================================================

class MetricsStreamServerTest : public ::testing::Test {
protected:
    void SetUp() override {
        srv_.resetStats();
    }
    void TearDown() override {
        if (srv_.isRunning()) srv_.stop();
    }

    MetricsStreamServer srv_;
};

// ============================================================================
// Lifecycle
// ============================================================================

TEST_F(MetricsStreamServerTest, InitiallyNotRunning) {
    EXPECT_FALSE(srv_.isRunning());
    EXPECT_EQ(0u, srv_.port());
    EXPECT_TRUE(srv_.bindAddress().empty());
}

TEST_F(MetricsStreamServerTest, StartSetsRunningState) {
    srv_.start("0.0.0.0", 8001);
    EXPECT_TRUE(srv_.isRunning());
    EXPECT_EQ(8001u, srv_.port());
    EXPECT_EQ("0.0.0.0", srv_.bindAddress());
}

TEST_F(MetricsStreamServerTest, StopClearsRunningState) {
    srv_.start("127.0.0.1", 9000);
    EXPECT_TRUE(srv_.isRunning());
    srv_.stop();
    EXPECT_FALSE(srv_.isRunning());
}

TEST_F(MetricsStreamServerTest, StartThrowsOnEmptyBindAddress) {
    EXPECT_THROW(srv_.start("", 8001), std::runtime_error);
}

TEST_F(MetricsStreamServerTest, StartThrowsOnZeroPort) {
    EXPECT_THROW(srv_.start("0.0.0.0", 0), std::runtime_error);
}

TEST_F(MetricsStreamServerTest, StartCanBeCalledTwice) {
    srv_.start("0.0.0.0", 8001);
    // Calling start() again (e.g. to update bind coordinates) must not throw.
    EXPECT_NO_THROW(srv_.start("127.0.0.1", 8002));
    EXPECT_EQ(8002u, srv_.port());
    EXPECT_EQ("127.0.0.1", srv_.bindAddress());
}

// ============================================================================
// Subscription management
// ============================================================================

TEST_F(MetricsStreamServerTest, SubscribeIncreasesCount) {
    EXPECT_EQ(0u, srv_.subscriptionCount());
    StreamSubscription sub;
    sub.client_id = "client-1";
    srv_.subscribe(sub);
    EXPECT_EQ(1u, srv_.subscriptionCount());
}

TEST_F(MetricsStreamServerTest, DuplicateSubscribeOverwrites) {
    StreamSubscription sub;
    sub.client_id = "client-1";
    sub.metric_names = {"metric_a"};
    srv_.subscribe(sub);

    sub.metric_names = {"metric_b"};
    srv_.subscribe(sub);

    EXPECT_EQ(1u, srv_.subscriptionCount());
    EXPECT_TRUE(srv_.hasSubscription("client-1"));
}

TEST_F(MetricsStreamServerTest, UnsubscribeDecreasesCount) {
    StreamSubscription sub;
    sub.client_id = "client-1";
    srv_.subscribe(sub);
    EXPECT_EQ(1u, srv_.subscriptionCount());

    srv_.unsubscribe("client-1");
    EXPECT_EQ(0u, srv_.subscriptionCount());
    EXPECT_FALSE(srv_.hasSubscription("client-1"));
}

TEST_F(MetricsStreamServerTest, UnsubscribeNonExistentIsNoOp) {
    EXPECT_NO_THROW(srv_.unsubscribe("ghost-client"));
}

TEST_F(MetricsStreamServerTest, SubscribeThrowsOnEmptyClientId) {
    StreamSubscription sub;
    sub.client_id = "";
    EXPECT_THROW(srv_.subscribe(sub), std::invalid_argument);
}

TEST_F(MetricsStreamServerTest, HasSubscriptionReturnsTrueForKnownClient) {
    StreamSubscription sub;
    sub.client_id = "cli-xyz";
    srv_.subscribe(sub);
    EXPECT_TRUE(srv_.hasSubscription("cli-xyz"));
    EXPECT_FALSE(srv_.hasSubscription("cli-unknown"));
}

// ============================================================================
// Delivery — basic dispatch
// ============================================================================

TEST_F(MetricsStreamServerTest, PushDeliversToSubscriber) {
    srv_.start("0.0.0.0", 8001);

    std::vector<std::string> received;
    srv_.setDeliveryCallback([&](const std::string& /*client_id*/,
                                 const std::string& payload) {
        received.push_back(payload);
    });

    StreamSubscription sub;
    sub.client_id = "client-1";
    sub.metric_names = {"query_latency_ms"};
    srv_.subscribe(sub);

    MetricUpdate upd;
    upd.metric_name = "query_latency_ms";
    upd.value = 42.5;
    srv_.pushMetrics(upd);

    ASSERT_EQ(1u, received.size());
    EXPECT_NE(std::string::npos, received[0].find("query_latency_ms"));
    EXPECT_NE(std::string::npos, received[0].find("42.5"));
}

TEST_F(MetricsStreamServerTest, PushDoesNotDeliverWhenStopped) {
    // Do not call start(); server is not running.
    bool called = false;
    srv_.setDeliveryCallback([&](const std::string&, const std::string&) {
        called = true;
    });

    StreamSubscription sub;
    sub.client_id = "client-1";
    srv_.subscribe(sub);

    MetricUpdate upd;
    upd.metric_name = "any_metric";
    upd.value = 1.0;
    srv_.pushMetrics(upd);

    EXPECT_FALSE(called);
}

TEST_F(MetricsStreamServerTest, PushDeliversToAllSubscribers) {
    srv_.start("0.0.0.0", 8001);

    std::atomic<int> count{0};
    srv_.setDeliveryCallback([&](const std::string&, const std::string&) {
        count.fetch_add(1, std::memory_order_relaxed);
    });

    for (int i = 0; i < 5; ++i) {
        StreamSubscription sub;
        sub.client_id = "client-" + std::to_string(i);
        srv_.subscribe(sub);
    }

    MetricUpdate upd;
    upd.metric_name = "cache_hit_rate";
    upd.value = 0.95;
    srv_.pushMetrics(upd);

    EXPECT_EQ(5, count.load());
}

TEST_F(MetricsStreamServerTest, PushCountedInStats) {
    srv_.start("0.0.0.0", 8001);
    srv_.setDeliveryCallback([](const std::string&, const std::string&) {});

    StreamSubscription sub;
    sub.client_id = "client-1";
    srv_.subscribe(sub);

    MetricUpdate upd;
    upd.metric_name = "any_metric";
    upd.value = 1.0;
    srv_.pushMetrics(upd);
    srv_.pushMetrics(upd);

    auto stats = srv_.getStats();
    EXPECT_EQ(2u, stats.total_updates_pushed);
    EXPECT_EQ(2u, stats.total_deliveries);
}

// ============================================================================
// Metric name filtering
// ============================================================================

TEST_F(MetricsStreamServerTest, EmptyMetricNamesSubscribesToAll) {
    srv_.start("0.0.0.0", 8001);

    int count = 0;
    srv_.setDeliveryCallback([&](const std::string&, const std::string&) {
        ++count;
    });

    StreamSubscription sub;
    sub.client_id = "catch-all";
    // metric_names is empty → subscribe to all
    srv_.subscribe(sub);

    for (const char* name : {"metric_a", "metric_b", "metric_c"}) {
        MetricUpdate upd;
        upd.metric_name = name;
        upd.value = 1.0;
        srv_.pushMetrics(upd);
    }

    EXPECT_EQ(3, count);
}

TEST_F(MetricsStreamServerTest, MetricNameFilterSkipsNonMatchingUpdates) {
    srv_.start("0.0.0.0", 8001);

    std::vector<std::string> received_metrics;
    srv_.setDeliveryCallback([&](const std::string&, const std::string& payload) {
        received_metrics.push_back(payload);
    });

    StreamSubscription sub;
    sub.client_id = "selective";
    sub.metric_names = {"query_latency_ms"};
    srv_.subscribe(sub);

    MetricUpdate upd1;
    upd1.metric_name = "query_latency_ms";
    upd1.value = 10.0;

    MetricUpdate upd2;
    upd2.metric_name = "cache_hit_rate";
    upd2.value = 0.9;

    srv_.pushMetrics(upd1);
    srv_.pushMetrics(upd2);

    ASSERT_EQ(1u, received_metrics.size());
    EXPECT_NE(std::string::npos, received_metrics[0].find("query_latency_ms"));

    auto stats = srv_.getStats();
    EXPECT_EQ(1u, stats.filtered_deliveries);
}

// ============================================================================
// Label filtering
// ============================================================================

TEST_F(MetricsStreamServerTest, LabelFilterMatchesCorrectLabel) {
    srv_.start("0.0.0.0", 8001);

    int count = 0;
    srv_.setDeliveryCallback([&](const std::string&, const std::string&) { ++count; });

    StreamSubscription sub;
    sub.client_id = "tenant-acme";
    sub.filters = {{"tenant_id", "acme"}};
    srv_.subscribe(sub);

    MetricUpdate upd_acme;
    upd_acme.metric_name = "query_latency_ms";
    upd_acme.value = 5.0;
    upd_acme.labels = {{"tenant_id", "acme"}};

    MetricUpdate upd_other;
    upd_other.metric_name = "query_latency_ms";
    upd_other.value = 7.0;
    upd_other.labels = {{"tenant_id", "globex"}};

    srv_.pushMetrics(upd_acme);
    srv_.pushMetrics(upd_other);

    EXPECT_EQ(1, count);
    EXPECT_EQ(1u, srv_.getStats().filtered_deliveries);
}

TEST_F(MetricsStreamServerTest, LabelFilterWithEmptyValueMatchesAny) {
    srv_.start("0.0.0.0", 8001);

    int count = 0;
    srv_.setDeliveryCallback([&](const std::string&, const std::string&) { ++count; });

    StreamSubscription sub;
    sub.client_id = "any-tenant";
    sub.filters = {{"tenant_id", ""}};  // empty value = wildcard
    srv_.subscribe(sub);

    for (const char* tenant : {"acme", "globex", "initech"}) {
        MetricUpdate upd;
        upd.metric_name = "query_latency_ms";
        upd.value = 1.0;
        upd.labels = {{"tenant_id", tenant}};
        srv_.pushMetrics(upd);
    }

    EXPECT_EQ(3, count);
}

TEST_F(MetricsStreamServerTest, AllLabelFiltersMustMatch) {
    srv_.start("0.0.0.0", 8001);

    int count = 0;
    srv_.setDeliveryCallback([&](const std::string&, const std::string&) { ++count; });

    StreamSubscription sub;
    sub.client_id = "strict";
    sub.filters = {{"region", "us-east"}, {"env", "prod"}};
    srv_.subscribe(sub);

    // Matches both filters.
    MetricUpdate upd_match;
    upd_match.metric_name = "cpu_usage";
    upd_match.value = 60.0;
    upd_match.labels = {{"region", "us-east"}, {"env", "prod"}};

    // Matches only region.
    MetricUpdate upd_partial;
    upd_partial.metric_name = "cpu_usage";
    upd_partial.value = 70.0;
    upd_partial.labels = {{"region", "us-east"}, {"env", "staging"}};

    srv_.pushMetrics(upd_match);
    srv_.pushMetrics(upd_partial);

    EXPECT_EQ(1, count);
}

TEST_F(MetricsStreamServerTest, MissingLabelFailsFilter) {
    srv_.start("0.0.0.0", 8001);

    int count = 0;
    srv_.setDeliveryCallback([&](const std::string&, const std::string&) { ++count; });

    StreamSubscription sub;
    sub.client_id = "labeled-only";
    sub.filters = {{"tenant_id", "acme"}};
    srv_.subscribe(sub);

    // Update has no labels at all.
    MetricUpdate upd;
    upd.metric_name = "query_latency_ms";
    upd.value = 1.0;
    srv_.pushMetrics(upd);

    EXPECT_EQ(0, count);
    EXPECT_EQ(1u, srv_.getStats().filtered_deliveries);
}

// ============================================================================
// Rate limiting
// ============================================================================

TEST_F(MetricsStreamServerTest, ThrottleSkipsEarlyDelivery) {
    srv_.start("0.0.0.0", 8001);

    int count = 0;
    srv_.setDeliveryCallback([&](const std::string&, const std::string&) { ++count; });

    StreamSubscription sub;
    sub.client_id = "throttled";
    sub.update_interval = 500ms;
    srv_.subscribe(sub);

    MetricUpdate upd;
    upd.metric_name = "query_latency_ms";
    upd.value = 1.0;

    srv_.pushMetrics(upd);  // delivered (first push)
    srv_.pushMetrics(upd);  // throttled

    EXPECT_EQ(1, count);
    EXPECT_EQ(1u, srv_.getStats().throttled_deliveries);
}

TEST_F(MetricsStreamServerTest, ThrottleAllowsDeliveryAfterInterval) {
    srv_.start("0.0.0.0", 8001);

    int count = 0;
    srv_.setDeliveryCallback([&](const std::string&, const std::string&) { ++count; });

    StreamSubscription sub;
    sub.client_id = "throttled";
    sub.update_interval = 50ms;
    srv_.subscribe(sub);

    MetricUpdate upd;
    upd.metric_name = "cpu_usage";
    upd.value = 55.0;

    srv_.pushMetrics(upd);
    EXPECT_EQ(1, count);

    // Use a generous sleep to avoid CI timing flakiness: interval is 50 ms,
    // sleep for 5× that to ensure steady_clock advances past the threshold
    // on any scheduler.
    std::this_thread::sleep_for(250ms);
    srv_.pushMetrics(upd);
    EXPECT_EQ(2, count);
}

TEST_F(MetricsStreamServerTest, ZeroIntervalNeverThrottles) {
    srv_.start("0.0.0.0", 8001);

    int count = 0;
    srv_.setDeliveryCallback([&](const std::string&, const std::string&) { ++count; });

    StreamSubscription sub;
    sub.client_id = "unlimited";
    sub.update_interval = 0ms;
    srv_.subscribe(sub);

    MetricUpdate upd;
    upd.metric_name = "any";
    upd.value = 1.0;

    for (int i = 0; i < 10; ++i) {
        srv_.pushMetrics(upd);
    }
    EXPECT_EQ(10, count);
    EXPECT_EQ(0u, srv_.getStats().throttled_deliveries);
}

// ============================================================================
// Serialisation
// ============================================================================

TEST_F(MetricsStreamServerTest, FormatWebSocketMessageContainsRequiredFields) {
    MetricUpdate upd;
    upd.metric_name = "cache_hit_rate";
    upd.value = 0.95;
    upd.labels = {{"region", "us-east"}};

    const std::string msg = MetricsStreamServer::formatWebSocketMessage(upd);

    EXPECT_NE(std::string::npos, msg.find(R"("type":"metric_update")"));
    EXPECT_NE(std::string::npos, msg.find(R"("metric_name":"cache_hit_rate")"));
    EXPECT_NE(std::string::npos, msg.find(R"("value":0.95)"));
    EXPECT_NE(std::string::npos, msg.find(R"("region":"us-east")"));
    EXPECT_NE(std::string::npos, msg.find("timestamp_ms"));
}

TEST_F(MetricsStreamServerTest, FormatWebSocketMessageEmptyLabels) {
    MetricUpdate upd;
    upd.metric_name = "mem_usage_bytes";
    upd.value = 1024.0;

    const std::string msg = MetricsStreamServer::formatWebSocketMessage(upd);
    EXPECT_NE(std::string::npos, msg.find(R"("labels":{})"));
}

TEST_F(MetricsStreamServerTest, FormatWebSocketMessageEscapesSpecialChars) {
    MetricUpdate upd;
    upd.metric_name = "metric\"with\\special\nchars";
    upd.value = 1.0;
    upd.labels = {{"key\"1", "val\tue"}};

    const std::string msg = MetricsStreamServer::formatWebSocketMessage(upd);

    // metric_name: double-quote, backslash and newline must be escaped.
    EXPECT_NE(std::string::npos,
              msg.find("\"metric_name\":\"metric\\\"with\\\\special\\nchars\""));
    // Label key and value must be escaped too.
    EXPECT_NE(std::string::npos, msg.find("\"key\\\"1\":\"val\\tue\""));
    // The raw special characters must NOT appear unescaped in the output.
    EXPECT_EQ(std::string::npos, msg.find('\n'));
}

TEST_F(MetricsStreamServerTest, FormatSseMessageHasDataPrefix) {
    MetricUpdate upd;
    upd.metric_name = "disk_iops";
    upd.value = 500.0;

    const std::string sse = MetricsStreamServer::formatSseMessage(upd);
    EXPECT_EQ(0u, sse.find("data: "));
    EXPECT_NE(std::string::npos, sse.find("disk_iops"));
    // SSE events end with double newline.
    EXPECT_EQ("\n\n", sse.substr(sse.size() - 2));
}

// ============================================================================
// Stats
// ============================================================================

TEST_F(MetricsStreamServerTest, InitialStatsAreZero) {
    auto stats = srv_.getStats();
    EXPECT_EQ(0u, stats.active_subscriptions);
    EXPECT_EQ(0u, stats.total_updates_pushed);
    EXPECT_EQ(0u, stats.total_deliveries);
    EXPECT_EQ(0u, stats.throttled_deliveries);
    EXPECT_EQ(0u, stats.filtered_deliveries);
}

TEST_F(MetricsStreamServerTest, ResetStatsZeroesCounters) {
    srv_.start("0.0.0.0", 8001);
    srv_.setDeliveryCallback([](const std::string&, const std::string&) {});

    StreamSubscription sub;
    sub.client_id = "c1";
    srv_.subscribe(sub);

    MetricUpdate upd;
    upd.metric_name = "x";
    upd.value = 1.0;
    srv_.pushMetrics(upd);

    srv_.resetStats();
    auto stats = srv_.getStats();
    EXPECT_EQ(0u, stats.total_updates_pushed);
    EXPECT_EQ(0u, stats.total_deliveries);
    // active_subscriptions is live state, not a resettable counter.
    EXPECT_EQ(1u, stats.active_subscriptions);
}

TEST_F(MetricsStreamServerTest, StatsActiveSubscriptionsIsAccurate) {
    StreamSubscription sub;
    sub.client_id = "a";
    srv_.subscribe(sub);
    sub.client_id = "b";
    srv_.subscribe(sub);

    EXPECT_EQ(2u, srv_.getStats().active_subscriptions);

    srv_.unsubscribe("a");
    EXPECT_EQ(1u, srv_.getStats().active_subscriptions);
}

// ============================================================================
// Thread safety
// ============================================================================

TEST_F(MetricsStreamServerTest, ConcurrentPushFromMultipleThreads) {
    srv_.start("0.0.0.0", 8001);

    std::atomic<int> delivered{0};
    srv_.setDeliveryCallback([&](const std::string&, const std::string&) {
        delivered.fetch_add(1, std::memory_order_relaxed);
    });

    StreamSubscription sub;
    sub.client_id = "concurrent-listener";
    srv_.subscribe(sub);

    constexpr int kThreads = 4;
    constexpr int kPushesPerThread = 25;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&] {
            MetricUpdate upd;
            upd.metric_name = "counter";
            upd.value = 1.0;
            for (int j = 0; j < kPushesPerThread; ++j) {
                srv_.pushMetrics(upd);
            }
        });
    }
    for (auto& t : threads) t.join();

    EXPECT_EQ(kThreads * kPushesPerThread, delivered.load());
    EXPECT_EQ(static_cast<uint64_t>(kThreads * kPushesPerThread),
              srv_.getStats().total_updates_pushed);
}

TEST_F(MetricsStreamServerTest, ConcurrentSubscribeAndPush) {
    srv_.start("0.0.0.0", 8001);
    srv_.setDeliveryCallback([](const std::string&, const std::string&) {});

    // Pre-subscribe one client so there is at least one subscriber.
    StreamSubscription base_sub;
    base_sub.client_id = "base";
    srv_.subscribe(base_sub);

    // Producer thread: bounded to avoid pegging the CPU on CI.
    std::thread producer([&] {
        MetricUpdate upd;
        upd.metric_name = "throughput";
        upd.value = 1.0;
        constexpr int kProducerPushes = 200;
        for (int push = 0; push < kProducerPushes; ++push) {
            srv_.pushMetrics(upd);
            std::this_thread::yield();
        }
    });

    // Subscriber churn thread.
    std::thread churn([&] {
        for (int i = 0; i < 50; ++i) {
            StreamSubscription sub;
            sub.client_id = "churn-" + std::to_string(i);
            srv_.subscribe(sub);
            srv_.unsubscribe("churn-" + std::to_string(i));
        }
    });

    churn.join();
    producer.join();

    // Should not have crashed or deadlocked.
    EXPECT_EQ(200u, srv_.getStats().total_updates_pushed);
}
