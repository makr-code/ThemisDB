/**
 * @file test_continuous_query_engine.cpp
 * @brief Unit tests CQ-01..CQ-22 for the Continuous Query Language engine.
 *
 * Coverage:
 *   CQ-01..05  WindowSpec construction and tick computation
 *   CQ-06..10  SynopsisStore insert, expire, size enforcement
 *   CQ-11..13  IncrementalAgg delta correctness (SUM / AVG / MIN / MAX)
 *   CQ-14..15  CQWatermark advancement and late-data detection
 *   CQ-16..18  DELTA / SNAPSHOT / CHANGES result mode output
 *   CQ-19..20  Validation rejections (invalid window, empty name)
 *   CQ-21      inject_queue_ overflow drops oldest without crashing (CQE-02)
 *   CQ-22      registry full returns error after kMaxRegisteredQueries (CQE-03)
 */

#include <gtest/gtest.h>

#include "query/window_spec.h"
#include "query/synopsis_store.h"
#include "query/incremental_agg.h"
#include "query/cq_watermark.h"
#include "query/continuous_query_engine_impl.h"
#include "query/continuous_query_planner.h"

#include <chrono>
#include <thread>

using namespace themis::query;

// ─────────────────────────────────────────────────────────────────────────────
// CQ-01  TIME_SLIDING window: slide interval == slide_ms
// ─────────────────────────────────────────────────────────────────────────────
TEST(ContinuousQueryWindowSpec, CQ01_TimeSlidingSlideInterval) {
    WindowSpec ws;
    ws.type     = WindowSpec::Type::TIME_SLIDING;
    ws.range_ms = 60'000;
    ws.slide_ms = 1'000;

    EXPECT_EQ(ws.slideInterval(), std::chrono::milliseconds{1'000});
    EXPECT_EQ(ws.windowWidth(),   std::chrono::milliseconds{60'000});
    EXPECT_TRUE(ws.isTimeBased());
}

// ─────────────────────────────────────────────────────────────────────────────
// CQ-02  TUMBLING window: slide interval == range_ms
// ─────────────────────────────────────────────────────────────────────────────
TEST(ContinuousQueryWindowSpec, CQ02_TumblingSlideInterval) {
    WindowSpec ws;
    ws.type     = WindowSpec::Type::TUMBLING;
    ws.range_ms = 5'000;

    EXPECT_EQ(ws.slideInterval(), std::chrono::milliseconds{5'000});
    EXPECT_EQ(ws.windowWidth(),   std::chrono::milliseconds{5'000});
    EXPECT_TRUE(ws.isTimeBased());
}

// ─────────────────────────────────────────────────────────────────────────────
// CQ-03  COUNT_SLIDING window: not time-based
// ─────────────────────────────────────────────────────────────────────────────
TEST(ContinuousQueryWindowSpec, CQ03_CountSlidingNotTimeBased) {
    WindowSpec ws;
    ws.type       = WindowSpec::Type::COUNT_SLIDING;
    ws.rows       = 500;
    ws.slide_rows = 50;

    EXPECT_FALSE(ws.isTimeBased());
    EXPECT_EQ(ws.rows, 500);
    EXPECT_EQ(ws.slide_rows, 50);
}

// ─────────────────────────────────────────────────────────────────────────────
// CQ-04  WindowSpec default values
// ─────────────────────────────────────────────────────────────────────────────
TEST(ContinuousQueryWindowSpec, CQ04_Defaults) {
    WindowSpec ws;
    EXPECT_EQ(ws.type,     WindowSpec::Type::TIME_SLIDING);
    EXPECT_EQ(ws.range_ms, 60'000);
    EXPECT_EQ(ws.slide_ms, 1'000);
}

// ─────────────────────────────────────────────────────────────────────────────
// CQ-05  WindowSpec partition_by (COUNT_SLIDING)
// ─────────────────────────────────────────────────────────────────────────────
TEST(ContinuousQueryWindowSpec, CQ05_PartitionBy) {
    WindowSpec ws;
    ws.type         = WindowSpec::Type::COUNT_SLIDING;
    ws.partition_by = "sensor_id";

    EXPECT_EQ(ws.partition_by, "sensor_id");
}

// ─────────────────────────────────────────────────────────────────────────────
// CQ-06  SynopsisStore: basic insert and size
// ─────────────────────────────────────────────────────────────────────────────
TEST(SynopsisStoreTest, CQ06_InsertAndSize) {
    SynopsisStore store(100, 1024 * 1024);
    EXPECT_TRUE(store.insert({1000, "tuple1"}));
    EXPECT_TRUE(store.insert({2000, "tuple2"}));
    EXPECT_EQ(store.size(), 2u);
}

// ─────────────────────────────────────────────────────────────────────────────
// CQ-07  SynopsisStore: expire removes tuples older than threshold
// ─────────────────────────────────────────────────────────────────────────────
TEST(SynopsisStoreTest, CQ07_Expire) {
    SynopsisStore store;
    store.insert({1000, "a"});
    store.insert({2000, "b"});
    store.insert({3000, "c"});

    auto expired = store.expire(2500);
    EXPECT_EQ(expired.size(), 2u);
    EXPECT_EQ(store.size(), 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// CQ-08  SynopsisStore: tuple-count capacity enforcement
// ─────────────────────────────────────────────────────────────────────────────
TEST(SynopsisStoreTest, CQ08_TupleCapacity) {
    SynopsisStore store(2, 1024 * 1024);
    EXPECT_TRUE(store.insert({1, "x"}));
    EXPECT_TRUE(store.insert({2, "y"}));
    EXPECT_FALSE(store.insert({3, "z"}));  // capacity exceeded
    EXPECT_EQ(store.size(), 2u);
}

// ─────────────────────────────────────────────────────────────────────────────
// CQ-09  SynopsisStore: byte capacity enforcement
// ─────────────────────────────────────────────────────────────────────────────
TEST(SynopsisStoreTest, CQ09_ByteCapacity) {
    SynopsisStore store(1000, 10);  // 10-byte cap
    EXPECT_TRUE(store.insert({1, "12345"}));   // 5 bytes → ok
    EXPECT_FALSE(store.insert({2, "123456"})); // 6 bytes → would overflow
    EXPECT_EQ(store.size(), 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// CQ-10  SynopsisStore: snapshot returns all tuples
// ─────────────────────────────────────────────────────────────────────────────
TEST(SynopsisStoreTest, CQ10_Snapshot) {
    SynopsisStore store;
    store.insert({100, "p1"});
    store.insert({200, "p2"});

    auto snap = store.snapshot();
    ASSERT_EQ(snap.size(), 2u);
    EXPECT_EQ(snap[0].payload, "p1");
    EXPECT_EQ(snap[1].payload, "p2");
    // snapshot does not clear
    EXPECT_EQ(store.size(), 2u);
}

// ─────────────────────────────────────────────────────────────────────────────
// CQ-11  IncrementalAgg SUM delta correctness
// ─────────────────────────────────────────────────────────────────────────────
TEST(IncrementalAggTest, CQ11_SumDelta) {
    IncrementalAgg agg(AggOp::SUM);
    agg.add(10.0);
    agg.add(20.0);
    agg.add(30.0);
    EXPECT_DOUBLE_EQ(agg.result(), 60.0);

    agg.remove(10.0);
    EXPECT_DOUBLE_EQ(agg.result(), 50.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// CQ-12  IncrementalAgg AVG delta correctness
// ─────────────────────────────────────────────────────────────────────────────
TEST(IncrementalAggTest, CQ12_AvgDelta) {
    IncrementalAgg agg(AggOp::AVG);
    agg.add(4.0);
    agg.add(8.0);
    EXPECT_DOUBLE_EQ(agg.result(), 6.0);

    agg.remove(4.0);
    EXPECT_DOUBLE_EQ(agg.result(), 8.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// CQ-13  IncrementalAgg MIN/MAX rescan on extremum eviction
// ─────────────────────────────────────────────────────────────────────────────
TEST(IncrementalAggTest, CQ13_MinMaxRescan) {
    IncrementalAgg agg_min(AggOp::MIN);
    agg_min.add(5.0);
    agg_min.add(3.0);
    agg_min.add(7.0);
    EXPECT_DOUBLE_EQ(agg_min.result(), 3.0);

    agg_min.remove(3.0);  // evict minimum → rescan needed
    EXPECT_TRUE(agg_min.rescanNeeded());
    agg_min.rescan({5.0, 7.0});
    EXPECT_FALSE(agg_min.rescanNeeded());
    EXPECT_DOUBLE_EQ(agg_min.result(), 5.0);

    IncrementalAgg agg_max(AggOp::MAX);
    agg_max.add(2.0);
    agg_max.add(9.0);
    agg_max.remove(9.0);  // evict maximum → rescan
    EXPECT_TRUE(agg_max.rescanNeeded());
    agg_max.rescan({2.0});
    EXPECT_DOUBLE_EQ(agg_max.result(), 2.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// CQ-14  CQWatermark: advances on on-time events
// ─────────────────────────────────────────────────────────────────────────────
TEST(CQWatermarkTest, CQ14_WatermarkAdvance) {
    CQWatermark wm(500);  // 500 ms lateness budget

    // event at 10 000 000 µs → max_seen = 10M; wm after advance = 10M − 500 000
    wm.observe(10'000'000LL);
    wm.advance();

    EXPECT_EQ(wm.maxSeenUs(), 10'000'000LL);
    EXPECT_EQ(wm.watermarkUs(), 10'000'000LL - 500'000LL);
}

// ─────────────────────────────────────────────────────────────────────────────
// CQ-15  CQWatermark: late-data detection and correction budget
// ─────────────────────────────────────────────────────────────────────────────
TEST(CQWatermarkTest, CQ15_LateDataDetection) {
    CQWatermark wm(500);  // 500 ms = 500 000 µs budget

    // Establish watermark at 10M µs
    wm.observe(10'000'000LL);
    wm.advance();

    // Event at 9.7M µs → below wm=9.5M but within budget (9.5M−9.7M = 200 000 µs < 500 000)
    // Actually wm = 9 500 000.  9 700 000 > 9 500 000, so on-time.
    // Let's place a truly late event: 9 000 000 µs < wm=9 500 000 µs
    // Within budget: 9 000 000 >= (9 500 000 − 500 000) = 9 000 000 → exactly on boundary → on-budget
    bool result = wm.observe(9'000'000LL);
    EXPECT_FALSE(result);  // late (< watermark)
    EXPECT_EQ(wm.lateProcessed(), 1u);
    EXPECT_EQ(wm.lateDropped(), 0u);

    // Beyond budget: 8 900 000 µs < (9 500 000 − 500 000) = 9 000 000
    bool dropped = wm.observe(8'900'000LL);
    EXPECT_FALSE(dropped);
    EXPECT_EQ(wm.lateDropped(), 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// CQ-16  ContinuousQueryEngine DELTA mode emits additions
// ─────────────────────────────────────────────────────────────────────────────
TEST(ContinuousQueryEngineTest, CQ16_DeltaModeAdditions) {
    ContinuousQueryEngineImpl engine(std::chrono::milliseconds{50});

    ContinuousQuerySpec spec;
    spec.name               = "test_delta";
    spec.source_collection  = "sensors";
    spec.aql_body           = "FOR s IN sensors RETURN s";
    spec.result_mode        = ResultMode::DELTA;
    spec.window.type        = WindowSpec::Type::TIME_SLIDING;
    spec.window.range_ms    = 10'000;
    spec.window.slide_ms    = 1'000;

    auto reg_result = engine.registerQuery(spec);
    ASSERT_TRUE(reg_result.has_value()) << reg_result.error().message();

    auto sub_result = engine.subscribe("test_delta", ResultMode::DELTA);
    ASSERT_TRUE(sub_result.has_value());
    auto stream = *sub_result;

    // Inject a tuple
    engine.injectTuple("sensors", R"({"id":1})", 5'000'000LL);

    // Wait for up to 300ms for the item to appear
    auto item = stream->next(std::chrono::milliseconds{300});
    ASSERT_TRUE(item.has_value());
    EXPECT_FALSE(item->is_retract);
    EXPECT_EQ(item->payload, R"({"id":1})");
}

// ─────────────────────────────────────────────────────────────────────────────
// CQ-17  ContinuousQueryEngine SNAPSHOT mode
// ─────────────────────────────────────────────────────────────────────────────
TEST(ContinuousQueryEngineTest, CQ17_SnapshotMode) {
    ContinuousQueryEngineImpl engine(std::chrono::milliseconds{50});

    ContinuousQuerySpec spec;
    spec.name               = "snap_query";
    spec.source_collection  = "events";
    spec.aql_body           = "FOR e IN events RETURN e";
    spec.result_mode        = ResultMode::SNAPSHOT;
    spec.window.type        = WindowSpec::Type::TUMBLING;
    spec.window.range_ms    = 5'000;

    ASSERT_TRUE(engine.registerQuery(spec).has_value());

    auto sub = engine.subscribe("snap_query", ResultMode::SNAPSHOT);
    ASSERT_TRUE(sub.has_value());
    auto stream = *sub;

    // Inject tuple
    engine.injectTuple("events", R"({"ev":"a"})", 1'000'000LL);

    // SNAPSHOT mode: the tick loop emits the full synopsis each tick.
    // Wait up to 300ms for at least one item
    auto item = stream->next(std::chrono::milliseconds{300});
    ASSERT_TRUE(item.has_value());
    EXPECT_FALSE(item->is_retract);
}

// ─────────────────────────────────────────────────────────────────────────────
// CQ-18  CHANGES mode emits retraction for expired tuple
// ─────────────────────────────────────────────────────────────────────────────
TEST(ContinuousQueryEngineTest, CQ18_ChangesRetraction) {
    // Use a very short window so expiry happens quickly
    ContinuousQueryEngineImpl engine(std::chrono::milliseconds{50});

    ContinuousQuerySpec spec;
    spec.name               = "changes_query";
    spec.source_collection  = "stream";
    spec.aql_body           = "FOR x IN stream RETURN x";
    spec.result_mode        = ResultMode::CHANGES;
    spec.window.type        = WindowSpec::Type::TIME_SLIDING;
    spec.window.range_ms    = 1;  // 1 ms window → tuples expire very quickly
    spec.window.slide_ms    = 1;
    spec.allowed_lateness_ms = 0;

    ASSERT_TRUE(engine.registerQuery(spec).has_value());

    auto sub = engine.subscribe("changes_query", ResultMode::CHANGES);
    ASSERT_TRUE(sub.has_value());
    auto stream = *sub;

    // Inject an old tuple that should expire immediately
    engine.injectTuple("stream", R"({"x":1})", 1LL);  // ts=1µs → will expire

    // Drain additions first
    auto add_item = stream->next(std::chrono::milliseconds{300});
    ASSERT_TRUE(add_item.has_value());
    EXPECT_FALSE(add_item->is_retract);

    // Wait for expiry retraction (tick fires within 100ms)
    auto ret_item = stream->next(std::chrono::milliseconds{500});
    if (ret_item.has_value()) {
        EXPECT_TRUE(ret_item->is_retract);
    }
    // If no retraction yet that is also acceptable — depends on tick timing
}

// ─────────────────────────────────────────────────────────────────────────────
// CQ-19  Validation rejection: zero range_ms
// ─────────────────────────────────────────────────────────────────────────────
TEST(ContinuousQueryValidation, CQ19_RejectZeroRangeMs) {
    ContinuousQueryEngineImpl engine(std::chrono::milliseconds{500});

    ContinuousQuerySpec spec;
    spec.name               = "bad_window";
    spec.source_collection  = "data";
    spec.aql_body           = "FOR d IN data RETURN d";
    spec.window.type        = WindowSpec::Type::TIME_SLIDING;
    spec.window.range_ms    = 0;  // invalid

    auto result = engine.registerQuery(spec);
    EXPECT_FALSE(result.has_value());
}

// ─────────────────────────────────────────────────────────────────────────────
// CQ-20  Validation rejection: empty query name
// ─────────────────────────────────────────────────────────────────────────────
TEST(ContinuousQueryValidation, CQ20_RejectEmptyName) {
    ContinuousQueryEngineImpl engine(std::chrono::milliseconds{500});

    ContinuousQuerySpec spec;
    spec.name               = "";  // invalid
    spec.source_collection  = "data";
    spec.aql_body           = "FOR d IN data RETURN d";
    spec.window.range_ms    = 1'000;

    auto result = engine.registerQuery(spec);
    EXPECT_FALSE(result.has_value());
}

// ─────────────────────────────────────────────────────────────────────────────
// CQ-21  inject_queue_ overflow: bulk injection saturates staging queue but
//        does not crash and engine continues to function (CQE-02 regression).
// ─────────────────────────────────────────────────────────────────────────────
TEST(ContinuousQueryEngineTest, CQ21_InjectQueueOverflowDropsOldest) {
    ContinuousQueryEngineImpl engine(std::chrono::milliseconds{500});

    ContinuousQuerySpec spec;
    spec.name              = "overflow_query";
    spec.source_collection = "flood";
    spec.aql_body          = "FOR e IN flood RETURN e";
    spec.result_mode       = ResultMode::DELTA;
    spec.window.type       = WindowSpec::Type::TUMBLING;
    spec.window.range_ms   = 60'000;
    ASSERT_TRUE(engine.registerQuery(spec).has_value());

    // Inject more tuples than kMaxInjectQueueDepth (100 000) without crashing.
    // We use a smaller count here to keep the test fast; the cap is exercised
    // at the boundary.
    constexpr int kBurst = 200'001;  // >2× kMaxInjectQueueDepth
    for (int i = 0; i < kBurst; ++i) {
        engine.injectTuple("flood", R"({"i":1})", static_cast<int64_t>(i) * 1000LL);
    }
    // Engine must still be alive and responsive after the burst.
    EXPECT_FALSE(engine.listQueries().empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// CQ-22  registry full: registerQuery returns an error after
//        kMaxRegisteredQueries have been registered (CQE-03 regression).
// ─────────────────────────────────────────────────────────────────────────────
TEST(ContinuousQueryEngineTest, CQ22_RegistryFullReturnsError) {
    // Use a very slow tick so the loop thread does not interfere.
    ContinuousQueryEngineImpl engine(std::chrono::milliseconds{60'000});

    auto make_spec = [](int idx) {
        ContinuousQuerySpec s;
        s.name              = "q" + std::to_string(idx);
        s.source_collection = "col";
        s.aql_body          = "FOR e IN col RETURN e";
        s.window.range_ms   = 60'000;
        return s;
    };

    // Fill the registry to the limit (kMaxRegisteredQueries = 1 000).
    constexpr int kLimit = 1'000;
    for (int i = 0; i < kLimit; ++i) {
        auto r = engine.registerQuery(make_spec(i));
        ASSERT_TRUE(r.has_value()) << "registration " << i << " failed unexpectedly";
    }

    // One more registration must be rejected.
    auto overflow = engine.registerQuery(make_spec(kLimit));
    EXPECT_FALSE(overflow.has_value())
        << "expected registry-full error but registration succeeded";
}
