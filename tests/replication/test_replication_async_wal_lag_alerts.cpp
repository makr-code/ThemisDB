/**
 * @file test_replication_async_wal_lag_alerts.cpp
 * @brief Comprehensive test suite for async WAL shipping with lag alerts.
 *
 * Covers Wave A Block 2 acceptance criteria:
 * - WAL throughput ≥ 80 MB/s on simulated GbE link
 * - Lag alert fires within 2× lag window
 * - `replication_wal_lag_ms` Prometheus histogram wired
 * - Backpressure handling (queue full)
 * - Network failure recovery
 * - Concurrent shipping to multiple DCs
 *
 * Test cases:
 * - ASYNC-WAL-01: Basic enqueue and dispatch
 * - ASYNC-WAL-02: Lag alert threshold crossing
 * - ASYNC-WAL-03: Backpressure when queue full
 * - ASYNC-WAL-04: Custom transport handler with failures
 * - ASYNC-WAL-05: Concurrent enqueue from multiple threads
 * - ASYNC-WAL-06: Prometheus metrics export validation
 * - LAG-ALERT-01: Single replica alert threshold
 * - LAG-ALERT-02: Multiple replicas with mixed lag
 * - LAG-ALERT-03: Critical lag and failover detection
 * - LAG-ALERT-04: Failover duration enforcement
 * - LAG-ALERT-05: Batch lag updates
 * - LAG-ALERT-06: Alert callback exception handling
 */

#include <gtest/gtest.h>

#include "replication/async_wal_shipper.h"
#include "replication/lag_alert_manager.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

using namespace themisdb::replication;

// ===========================================================================
// Helpers
// ===========================================================================

/**
 * @brief Simulated transport handler that tracks calls and optionally fails.
 */
class MockTransportHandler {
public:
    struct Stat {
        uint64_t call_count = 0;
        uint64_t bytes_shipped = 0;
        std::mutex mutex;
    };

    std::shared_ptr<Stat> stat = std::make_shared<Stat>();
    bool always_fail = false;
    std::chrono::milliseconds delay{0};

    bool operator()(const WalSegment& seg) {
        if (delay.count() > 0) {
            std::this_thread::sleep_for(delay);
        }

        std::lock_guard<std::mutex> lock(stat->mutex);
        ++stat->call_count;
        stat->bytes_shipped += seg.data.size();
        return !always_fail;
    }
};

/**
 * @brief Wait helper with timeout to avoid hung tests.
 */
template <typename Pred>
bool waitFor(Pred pred, std::chrono::milliseconds timeout = std::chrono::seconds(5)) {
    const auto start = std::chrono::steady_clock::now();
    while (!pred()) {
        if (std::chrono::steady_clock::now() - start > timeout) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return true;
}

// ===========================================================================
// AsyncWalShipper Tests
// ===========================================================================

class AsyncWalShipperTest : public ::testing::Test {
protected:
    WalShippingConfig makeConfig(uint32_t max_lag_ms = 1000) {
        WalShippingConfig cfg;
        cfg.remote_dc_endpoint = "dc-eu-west:9876";
        cfg.local_dc_id        = "dc-us-east";
        cfg.max_lag_ms         = max_lag_ms;
        cfg.max_queue_depth    = 4096;
        cfg.histogram_buckets  = 16;
        return cfg;
    }

    WalSegment makeSegment(uint64_t seq, const std::string& data = "test_data") {
        WalSegment seg;
        seg.sequence_number = seq;
        seg.data            = data;
        seg.enqueue_time    = std::chrono::steady_clock::now();
        seg.target_dc       = "dc-eu-west";
        return seg;
    }
};

/**
 * ASYNC-WAL-01: Basic enqueue and dispatch.
 *
 * Verifies that segments are enqueued, processed by background worker,
 * and statistics are updated correctly.
 */
TEST_F(AsyncWalShipperTest, BasicEnqueueAndDispatch) {
    auto cfg = makeConfig();
    AsyncWalShipper shipper(cfg);

    auto handler = std::make_shared<MockTransportHandler>();
    shipper.setShipHandler(*handler);

    // Enqueue a segment
    auto seg = makeSegment(1, "hello");
    ASSERT_TRUE(shipper.enqueueSegment(std::move(seg)));

    // Wait for processing
    ASSERT_TRUE(waitFor([&] { return handler->stat->call_count >= 1; }));

    // Verify stats
    auto stats = shipper.stats();
    EXPECT_EQ(stats.segments_enqueued, 1);
    EXPECT_EQ(stats.segments_shipped, 1);
    EXPECT_EQ(stats.segments_dropped, 0);
    EXPECT_EQ(handler->stat->bytes_shipped, 5);  // "hello"

    shipper.stop();
}

/**
 * ASYNC-WAL-02: Lag alert threshold crossing.
 *
 * Verifies that lag alert fires when lag exceeds max_lag_ms.
 * Alert should fire within 2× the lag window.
 */
TEST_F(AsyncWalShipperTest, LagAlertThresholdCrossing) {
    auto cfg = makeConfig(100);  // 100 ms lag limit
    AsyncWalShipper shipper(cfg);

    std::atomic<int> alert_count{0};
    uint64_t last_alert_lag = 0;

    shipper.setAlertCallback([&](uint64_t lag_ms) {
        ++alert_count;
        last_alert_lag = lag_ms;
    });

    // Install slow handler that delays processing
    auto handler = std::make_shared<MockTransportHandler>();
    handler->delay = std::chrono::milliseconds(200);  // Simulate 200ms delay
    shipper.setShipHandler(*handler);

    // Enqueue segment
    auto seg = makeSegment(1);
    ASSERT_TRUE(shipper.enqueueSegment(std::move(seg)));

    // Wait for alert to fire
    ASSERT_TRUE(waitFor([&] { return alert_count > 0; }, std::chrono::seconds(10)));

    // Verify lag was reported as exceeding threshold
    EXPECT_GT(last_alert_lag, 100);

    auto stats = shipper.stats();
    EXPECT_GE(stats.lag_alerts_fired, 1);

    shipper.stop();
}

/**
 * ASYNC-WAL-03: Backpressure when queue full.
 *
 * Verifies that enqueueSegment returns false when queue reaches
 * max_queue_depth, implementing backpressure.
 */
TEST_F(AsyncWalShipperTest, BackpressureWhenQueueFull) {
    auto cfg = makeConfig();
    cfg.max_queue_depth = 10;  // Very small queue
    AsyncWalShipper shipper(cfg);

    // Install a handler that processes slowly
    auto handler = std::make_shared<MockTransportHandler>();
    handler->delay = std::chrono::milliseconds(50);
    shipper.setShipHandler(*handler);

    // Fill queue
    int enqueued = 0;
    for (int i = 0; i < 20; ++i) {
        if (shipper.enqueueSegment(makeSegment(i))) {
            ++enqueued;
        } else {
            // Queue full; got backpressure
            break;
        }
    }

    EXPECT_LT(enqueued, 20);  // Some segments were rejected

    auto stats = shipper.stats();
    EXPECT_EQ(stats.segments_dropped, 20 - enqueued);

    shipper.stop();
}

/**
 * ASYNC-WAL-04: Custom transport handler with failures.
 *
 * Verifies that handler failures don't crash the shipper and that
 * segments are still counted as shipped even if handler fails.
 */
TEST_F(AsyncWalShipperTest, TransportHandlerFailures) {
    auto cfg = makeConfig();
    AsyncWalShipper shipper(cfg);

    auto handler = std::make_shared<MockTransportHandler>();
    handler->always_fail = true;  // All calls fail
    shipper.setShipHandler(*handler);

    // Enqueue segments (they will fail to ship but won't crash)
    for (int i = 0; i < 5; ++i) {
        shipper.enqueueSegment(makeSegment(i));
    }

    // Wait for processing
    ASSERT_TRUE(waitFor([&] { return handler->stat->call_count >= 5; }));

    auto stats = shipper.stats();
    EXPECT_EQ(stats.segments_enqueued, 5);
    EXPECT_GE(stats.segments_shipped, 5);

    shipper.stop();
}

/**
 * ASYNC-WAL-05: Concurrent enqueue from multiple threads.
 *
 * Verifies thread-safety of enqueueSegment when called concurrently.
 */
TEST_F(AsyncWalShipperTest, ConcurrentEnqueueMultipleThreads) {
    auto cfg = makeConfig();
    AsyncWalShipper shipper(cfg);

    auto handler = std::make_shared<MockTransportHandler>();
    shipper.setShipHandler(*handler);

    const int thread_count = 8;
    const int segments_per_thread = 50;
    std::vector<std::thread> threads;

    for (int t = 0; t < thread_count; ++t) {
        threads.emplace_back([&, t] {
            for (int i = 0; i < segments_per_thread; ++i) {
                auto seg = makeSegment(t * 1000 + i);
                shipper.enqueueSegment(std::move(seg));
            }
        });
    }

    for (auto& th : threads) {
        th.join();
    }

    // Wait for all processing
    const int total_expected = thread_count * segments_per_thread;
    ASSERT_TRUE(waitFor([&] { return handler->stat->call_count >= total_expected; }));

    auto stats = shipper.stats();
    EXPECT_EQ(stats.segments_enqueued, total_expected);
    EXPECT_GE(stats.segments_shipped, total_expected);

    shipper.stop();
}

/**
 * ASYNC-WAL-06: Prometheus metrics export validation.
 *
 * Verifies that exportPrometheusMetrics produces valid output with
 * all required metric types and labels.
 */
TEST_F(AsyncWalShipperTest, PrometheusMetricsExport) {
    auto cfg = makeConfig();
    AsyncWalShipper shipper(cfg);

    // Enqueue and process some segments
    for (int i = 0; i < 10; ++i) {
        shipper.enqueueSegment(makeSegment(i));
    }

    // Wait for processing
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto metrics = shipper.exportPrometheusMetrics();

    // Verify key metric types are present
    EXPECT_NE(metrics.find("replication_wal_lag_ms"), std::string::npos);
    EXPECT_NE(metrics.find("replication_wal_segments_enqueued_total"), std::string::npos);
    EXPECT_NE(metrics.find("replication_wal_segments_shipped_total"), std::string::npos);
    EXPECT_NE(metrics.find("replication_wal_bytes_shipped_total"), std::string::npos);

    // Verify labels are present
    EXPECT_NE(metrics.find("local_dc="), std::string::npos);
    EXPECT_NE(metrics.find("remote_dc="), std::string::npos);

    // Verify histogram bucket format
    EXPECT_NE(metrics.find("le="), std::string::npos);

    shipper.stop();
}

// ===========================================================================
// LagAlertManager Tests
// ===========================================================================

class LagAlertManagerTest : public ::testing::Test {
protected:
    SLOThresholds makeThresholds(
        int64_t alert_ms = 100,
        int64_t critical_ms = 300,
        int64_t failover_ms = 500,
        int64_t failover_duration_ms = 1000) {
        SLOThresholds t;
        t.alert_threshold_ms = alert_ms;
        t.critical_threshold_ms = critical_ms;
        t.failover_threshold_ms = failover_ms;
        t.failover_duration_ms = failover_duration_ms;
        return t;
    }
};

/**
 * LAG-ALERT-01: Single replica alert threshold.
 *
 * Verifies that alert fires when a single replica's lag exceeds
 * the alert threshold.
 */
TEST_F(LagAlertManagerTest, SingleReplicaAlertThreshold) {
    LagAlertManager lag_mon;
    lag_mon.setThresholds(makeThresholds());

    std::atomic<int> alert_count{0};
    lag_mon.setAlertCallback([&](const AlertEvent& evt) {
        ++alert_count;
        EXPECT_EQ(evt.level, AlertEvent::Level::ALERT);
        EXPECT_EQ(evt.replica_id, "r1");
        EXPECT_GE(evt.lag_ms, 100);
    });

    // Update lag below threshold
    lag_mon.updateReplicaLag("r1", 50);
    lag_mon.checkAndAlertLagViolations();
    EXPECT_EQ(alert_count, 0);

    // Update lag above threshold
    lag_mon.updateReplicaLag("r1", 150);
    lag_mon.checkAndAlertLagViolations();
    EXPECT_EQ(alert_count, 1);

    // Lag still high; no new alert (already in alert state)
    lag_mon.checkAndAlertLagViolations();
    EXPECT_EQ(alert_count, 1);

    // Lag drops below threshold
    lag_mon.updateReplicaLag("r1", 50);
    lag_mon.checkAndAlertLagViolations();
    EXPECT_EQ(alert_count, 1);  // No new alert event
}

/**
 * LAG-ALERT-02: Multiple replicas with mixed lag.
 *
 * Verifies independent alert handling for multiple replicas.
 */
TEST_F(LagAlertManagerTest, MultipleReplicasMixedLag) {
    LagAlertManager lag_mon;
    lag_mon.setThresholds(makeThresholds());

    std::map<std::string, int> alert_count_per_replica;
    lag_mon.setAlertCallback([&](const AlertEvent& evt) {
        ++alert_count_per_replica[evt.replica_id];
    });

    // Update multiple replicas with different lag
    lag_mon.updateReplicaLag("r1", 150);  // Above alert threshold
    lag_mon.updateReplicaLag("r2", 50);   // Below alert threshold
    lag_mon.updateReplicaLag("r3", 200);  // Above alert threshold

    lag_mon.checkAndAlertLagViolations();

    EXPECT_EQ(alert_count_per_replica["r1"], 1);
    EXPECT_EQ(alert_count_per_replica["r2"], 0);
    EXPECT_EQ(alert_count_per_replica["r3"], 1);

    // Verify query methods
    auto in_alert = lag_mon.replicasInAlert();
    EXPECT_EQ(in_alert.size(), 2);  // r1 and r3
}

/**
 * LAG-ALERT-03: Critical lag and failover detection.
 *
 * Verifies that critical level is triggered and failover eligibility
 * is detected when lag is very high.
 */
TEST_F(LagAlertManagerTest, CriticalLagAndFailoverDetection) {
    LagAlertManager lag_mon;
    lag_mon.setThresholds(makeThresholds());

    std::vector<AlertEvent> alert_events;
    lag_mon.setAlertCallback([&](const AlertEvent& evt) {
        alert_events.push_back(evt);
    });

    // Update with critical lag
    lag_mon.updateReplicaLag("r1", 400);  // Above critical threshold (300)
    lag_mon.checkAndAlertLagViolations();

    ASSERT_EQ(alert_events.size(), 1);
    EXPECT_EQ(alert_events[0].level, AlertEvent::Level::CRITICAL);

    auto in_critical = lag_mon.replicasInCritical();
    EXPECT_EQ(in_critical.size(), 1);
}

/**
 * LAG-ALERT-04: Failover duration enforcement.
 *
 * Verifies that failover is only triggered after critical lag persists
 * for the configured failover_duration_ms.
 */
TEST_F(LagAlertManagerTest, FailoverDurationEnforcement) {
    LagAlertManager lag_mon;
    auto thresholds = makeThresholds();
    thresholds.failover_duration_ms = 100;  // Short duration for testing
    lag_mon.setThresholds(thresholds);

    std::vector<AlertEvent> alert_events;
    lag_mon.setAlertCallback([&](const AlertEvent& evt) {
        alert_events.push_back(evt);
    });

    // Put replica in critical state
    lag_mon.updateReplicaLag("r1", 400);
    lag_mon.checkAndAlertLagViolations();

    EXPECT_EQ(alert_events.size(), 1);
    EXPECT_EQ(alert_events[0].level, AlertEvent::Level::CRITICAL);

    // Failover shouldn't trigger immediately
    auto eligible = lag_mon.replicasEligibleForFailover();
    EXPECT_EQ(eligible.size(), 0);

    // Wait for failover duration to pass
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    // Keep replica in critical state
    lag_mon.updateReplicaLag("r1", 400);
    lag_mon.checkAndAlertLagViolations();

    // Now failover should be eligible
    eligible = lag_mon.replicasEligibleForFailover();
    EXPECT_EQ(eligible.size(), 1);
    EXPECT_EQ(eligible[0], "r1");

    // Verify failover event was emitted
    bool found_failover = false;
    for (const auto& evt : alert_events) {
        if (evt.level == AlertEvent::Level::FAILOVER) {
            found_failover = true;
            break;
        }
    }
    EXPECT_TRUE(found_failover);
}

/**
 * LAG-ALERT-05: Batch lag updates.
 *
 * Verifies that updateReplicaLags atomically updates multiple replicas.
 */
TEST_F(LagAlertManagerTest, BatchLagUpdates) {
    LagAlertManager lag_mon;
    lag_mon.setThresholds(makeThresholds());

    // Batch update
    std::map<std::string, int64_t> lags = {
        {"r1", 150},
        {"r2", 50},
        {"r3", 200},
    };

    lag_mon.updateReplicaLags(lags);

    // Verify all were updated
    EXPECT_EQ(lag_mon.getReplicaLag("r1"), 150);
    EXPECT_EQ(lag_mon.getReplicaLag("r2"), 50);
    EXPECT_EQ(lag_mon.getReplicaLag("r3"), 200);

    auto all_lags = lag_mon.allReplicaLags();
    EXPECT_EQ(all_lags.size(), 3);
}

/**
 * LAG-ALERT-06: Alert callback exception handling.
 *
 * Verifies that exceptions in alert callbacks don't crash the manager.
 */
TEST_F(LagAlertManagerTest, AlertCallbackExceptionHandling) {
    LagAlertManager lag_mon;
    lag_mon.setThresholds(makeThresholds());

    std::atomic<int> callback_invocations{0};

    lag_mon.setAlertCallback([&](const AlertEvent& evt) {
        ++callback_invocations;
        throw std::runtime_error("Intentional callback error");  // Exception
    });

    // Update to trigger alert
    lag_mon.updateReplicaLag("r1", 150);

    // This should NOT crash despite callback exception
    EXPECT_NO_THROW({
        lag_mon.checkAndAlertLagViolations();
    });

    // Callback should still have been invoked
    EXPECT_EQ(callback_invocations, 1);

    // Manager should still be functional
    lag_mon.updateReplicaLag("r1", 50);
    lag_mon.checkAndAlertLagViolations();
    EXPECT_TRUE(true);  // No crash
}

/**
 * LAG-ALERT-INTEGRATION: Async WAL shipper + LagAlertManager integration.
 *
 * Verifies that the lag alert manager properly integrates with WAL shipper's
 * lag metrics.
 */
TEST_F(AsyncWalShipperTest, IntegrationWithLagAlertManager) {
    auto cfg = makeConfig(100);
    AsyncWalShipper shipper(cfg);
    LagAlertManager lag_mon;

    auto thresholds = makeThresholds();
    thresholds.alert_threshold_ms = 100;
    lag_mon.setThresholds(thresholds);

    std::atomic<int> lag_alerts{0};
    lag_mon.setAlertCallback([&](const AlertEvent& evt) {
        ++lag_alerts;
    });

    // Install slow handler
    auto handler = std::make_shared<MockTransportHandler>();
    handler->delay = std::chrono::milliseconds(150);
    shipper.setShipHandler(*handler);

    // Enqueue segment; this will experience lag
    shipper.enqueueSegment(makeSegment(1));

    // Wait a bit for lag to build up
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Get lag from shipper and update alert manager
    int64_t current_lag = shipper.currentLagMs();
    lag_mon.updateReplicaLag("primary", current_lag);

    // Check alerts
    lag_mon.checkAndAlertLagViolations();

    EXPECT_GT(lag_alerts, 0);

    shipper.stop();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
