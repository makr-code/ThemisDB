/*
 * ThemisDB | File: test_ts_auto_buffer_adaptive.cpp | Version: 0.0.13
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */
// RESTORED FROM HISTORY: 892fbc132819cf3446b54bb51b8b14ec2dd61db5


#include <gtest/gtest.h>
#include "timeseries/ts_auto_buffer_adaptive.h"
#include <thread>

namespace themis {
namespace {

// ─────────────────────────────────────────────────────────────────────────────
// Basic construction
// ─────────────────────────────────────────────────────────────────────────────

TEST(FlushController, DefaultConstructionHasReasonableBatchSize) {
    FlushController fc;
    EXPECT_GT(fc.recommendedBatchSize(), 0u);
}

TEST(FlushController, CustomConfigRespectsInitialBatchSize) {
    FlushControllerConfig cfg;
    cfg.initial_batch_size = 200;
    cfg.min_batch_size     = 50;
    cfg.max_batch_size     = 5000;
    FlushController fc(cfg);
    EXPECT_EQ(fc.recommendedBatchSize(), 200u);
}

// ─────────────────────────────────────────────────────────────────────────────
// EWMA update
// ─────────────────────────────────────────────────────────────────────────────

TEST(FlushController, FirstSampleSeedsEwma) {
    FlushControllerConfig cfg;
    cfg.ewma_alpha = 1.0;  // Pure last-sample for testing
    FlushController fc(cfg);
    fc.reportFlushLatency(30.0);
    EXPECT_NEAR(fc.ewmaLatencyMs(), 30.0, 1.0);
}

TEST(FlushController, HighLatencyShrinksBatchSize) {
    FlushControllerConfig cfg;
    cfg.slo_threshold_ms   = 50.0;
    cfg.initial_batch_size = 500;
    cfg.min_batch_size     = 10;
    cfg.warmup_samples     = 1;
    cfg.ewma_alpha         = 1.0;
    FlushController fc(cfg);

    // Report high latency to trigger shrink
    fc.reportFlushLatency(200.0);
    EXPECT_LT(fc.recommendedBatchSize(), 500u);
}

TEST(FlushController, LowLatencyGrowsBatchSize) {
    FlushControllerConfig cfg;
    cfg.slo_threshold_ms   = 50.0;
    cfg.initial_batch_size = 100;
    cfg.max_batch_size     = 5000;
    cfg.warmup_samples     = 1;
    cfg.ewma_alpha         = 1.0;
    cfg.headroom_factor    = 0.9;  // grow when below 0.9 * slo = 45ms
    FlushController fc(cfg);

    // Report very low latency to trigger grow
    fc.reportFlushLatency(5.0);
    EXPECT_GT(fc.recommendedBatchSize(), 100u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Backpressure
// ─────────────────────────────────────────────────────────────────────────────

TEST(FlushController, BackpressureNotActiveInitially) {
    FlushController fc;
    EXPECT_FALSE(fc.isBackpressureActive());
}

TEST(FlushController, BackpressureActivatedOnHighLatency) {
    FlushControllerConfig cfg;
    cfg.slo_threshold_ms   = 50.0;
    cfg.warmup_samples     = 1;
    cfg.ewma_alpha         = 1.0;
    FlushController fc(cfg);
    fc.reportFlushLatency(300.0);
    EXPECT_TRUE(fc.isBackpressureActive());
}

TEST(FlushController, BackpressureDeactivatedWhenLatencyDrops) {
    FlushControllerConfig cfg;
    cfg.slo_threshold_ms = 50.0;
    cfg.warmup_samples   = 1;
    cfg.ewma_alpha       = 1.0;
    FlushController fc(cfg);

    fc.reportFlushLatency(300.0);
    EXPECT_TRUE(fc.isBackpressureActive());

    fc.reportFlushLatency(5.0);
    EXPECT_FALSE(fc.isBackpressureActive());
}

TEST(FlushController, CheckBackpressureReturnsTrueWhenNotActive) {
    FlushController fc;
    bool ok = fc.checkBackpressure(10, std::chrono::milliseconds{100});
    EXPECT_TRUE(ok);
}

// ─────────────────────────────────────────────────────────────────────────────
// Stats
// ─────────────────────────────────────────────────────────────────────────────

TEST(FlushController, StatsBackpressureEventCounted) {
    FlushControllerConfig cfg;
    cfg.slo_threshold_ms = 50.0;
    cfg.warmup_samples   = 1;
    cfg.ewma_alpha       = 1.0;
    FlushController fc(cfg);

    fc.reportFlushLatency(200.0);
    auto s = fc.stats();
    EXPECT_GE(s.backpressure_events, 1u);
    EXPECT_TRUE(s.in_backpressure);
    EXPECT_EQ(s.samples, 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// notifyDrained releases backpressure waiter
// ─────────────────────────────────────────────────────────────────────────────

TEST(FlushController, NotifyDrainedReleasesProducer) {
    FlushControllerConfig cfg;
    cfg.slo_threshold_ms = 10.0;
    cfg.warmup_samples   = 1;
    cfg.ewma_alpha       = 1.0;
    cfg.low_water_mark   = 5;
    FlushController fc(cfg);

    // Activate backpressure
    fc.reportFlushLatency(500.0);
    ASSERT_TRUE(fc.isBackpressureActive());

    // Start a thread that waits on backpressure
    bool producer_unblocked = false;
    std::thread producer([&] {
        bool ok = fc.checkBackpressure(100, std::chrono::milliseconds{500});
        producer_unblocked = ok;
    });

    // Drain the queue below low-water mark
    std::this_thread::sleep_for(std::chrono::milliseconds{50});
    fc.reportFlushLatency(2.0);  // drops backpressure
    fc.notifyDrained(0);

    producer.join();
    EXPECT_TRUE(producer_unblocked);
}

// ─────────────────────────────────────────────────────────────────────────────
// Batch size stays within bounds
// ─────────────────────────────────────────────────────────────────────────────

TEST(FlushController, BatchSizeNeverExceedsMax) {
    FlushControllerConfig cfg;
    cfg.slo_threshold_ms   = 1000.0;  // very generous so we always grow
    cfg.initial_batch_size = 4000;
    cfg.max_batch_size     = 5000;
    cfg.min_batch_size     = 50;
    cfg.warmup_samples     = 1;
    cfg.ewma_alpha         = 1.0;
    FlushController fc(cfg);

    for (int i = 0; i < 100; ++i) {
        fc.reportFlushLatency(0.1);  // very low → keep growing
    }
    EXPECT_LE(fc.recommendedBatchSize(), cfg.max_batch_size);
}

TEST(FlushController, BatchSizeNeverDropsBelowMin) {
    FlushControllerConfig cfg;
    cfg.slo_threshold_ms   = 1.0;  // extremely tight so we always shrink
    cfg.initial_batch_size = 500;
    cfg.min_batch_size     = 50;
    cfg.max_batch_size     = 5000;
    cfg.warmup_samples     = 1;
    cfg.ewma_alpha         = 1.0;
    FlushController fc(cfg);

    for (int i = 0; i < 100; ++i) {
        fc.reportFlushLatency(1000.0);  // extremely high → keep shrinking
    }
    EXPECT_GE(fc.recommendedBatchSize(), cfg.min_batch_size);
}

} // anonymous namespace
} // namespace themis
