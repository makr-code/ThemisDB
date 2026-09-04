/**
 * @file test_log_aggregator.cpp
 * @brief Unit tests for LogAggregator – structured log collection,
 *        ring buffer, level filtering, trace context, and callbacks.
 *
 * Tests cover:
 *  - Default construction
 *  - log() / info() / warn() / error() etc. all add entries
 *  - Level filtering: entries below min_level are dropped
 *  - setLevel() changes min_level dynamically
 *  - logStructured(): fields appear in LogEntry::fields
 *  - logWithContext(): trace_id / span_id / request_id injected as fields
 *  - LogEntry::toJson() produces valid JSON
 *  - Ring buffer cap: oldest entries evicted when full
 *  - clear() empties the buffer
 *  - size() reflects buffer length
 *  - entries() returns chronological copy
 *  - entriesAtLevel() filters correctly
 *  - stats(): total_entries, dropped_entries, entries_by_level
 *  - setEntryCallback() invoked for every accepted entry
 *  - isHealthy() returns healthy for MEMORY sink
 *  - shutdown() drops subsequent calls
 *  - Thread safety: concurrent logging
 */

#include <gtest/gtest.h>
#include "observability/log_aggregator.h"

#include <atomic>
#include <string>
#include <thread>
#include <vector>

using namespace themis::observability;
using Level = themis::core::concerns::ILogger::Level;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static LogAggregator makeAgg(Level min_level = Level::TRACE,
                              size_t max_entries = 256) {
    LogAggregatorConfig cfg;
    cfg.min_level           = min_level;
    cfg.sink_type           = LogSinkType::MEMORY;
    cfg.max_retained_entries = max_entries;
    cfg.publish_metrics     = false;
    return LogAggregator(cfg);
}

// ---------------------------------------------------------------------------
// Construction / configuration
// ---------------------------------------------------------------------------

TEST(LogAggregatorTest, DefaultConstruction) {
    LogAggregator agg;
    EXPECT_EQ(0u, agg.size());
    EXPECT_EQ(Level::INFO, agg.getLevel());
}

TEST(LogAggregatorTest, IsHealthyForMemorySink) {
    auto agg = makeAgg();
    EXPECT_TRUE(agg.isHealthy().ok);
}

TEST(LogAggregatorTest, GetConfigReflectsConstruction) {
    LogAggregatorConfig cfg;
    cfg.min_level            = Level::WARN;
    cfg.max_retained_entries = 512;
    LogAggregator agg(cfg);
    EXPECT_EQ(Level::WARN, agg.getConfig().min_level);
    EXPECT_EQ(512u,        agg.getConfig().max_retained_entries);
}

// ---------------------------------------------------------------------------
// Level helpers / logging methods
// ---------------------------------------------------------------------------

TEST(LogAggregatorTest, AllLevelMethodsAddEntries) {
    auto agg = makeAgg(Level::TRACE);
    agg.trace("t");
    agg.debug("d");
    agg.info("i");
    agg.warn("w");
    agg.error("e");
    agg.critical("c");
    EXPECT_EQ(6u, agg.size());
}

TEST(LogAggregatorTest, LevelFilterDropsBelowMinLevel) {
    auto agg = makeAgg(Level::WARN);
    agg.trace("dropped");
    agg.debug("dropped");
    agg.info("dropped");
    EXPECT_EQ(0u, agg.size());
    EXPECT_EQ(3, agg.stats().dropped_entries);
}

TEST(LogAggregatorTest, LevelFilterAcceptsAtOrAboveMinLevel) {
    auto agg = makeAgg(Level::WARN);
    agg.warn("w");
    agg.error("e");
    agg.critical("c");
    EXPECT_EQ(3u, agg.size());
}

TEST(LogAggregatorTest, SetLevelDynamicChange) {
    auto agg = makeAgg(Level::ERROR);
    agg.info("dropped before setLevel");
    EXPECT_EQ(0u, agg.size());

    agg.setLevel(Level::INFO);
    agg.info("accepted after setLevel");
    EXPECT_EQ(1u, agg.size());
}

// ---------------------------------------------------------------------------
// Structured logging
// ---------------------------------------------------------------------------

TEST(LogAggregatorTest, LogStructuredStoresFields) {
    auto agg = makeAgg();
    agg.logStructured(Level::INFO, "query done",
                      {{"query_id", "q-42"}, {"latency_ms", "12"}});
    ASSERT_EQ(1u, agg.size());
    auto entry = agg.entries()[0];
    EXPECT_EQ("q-42", entry.fields.at("query_id"));
    EXPECT_EQ("12",   entry.fields.at("latency_ms"));
}

TEST(LogAggregatorTest, LogWithContextInjectsTraceFields) {
    auto agg = makeAgg();
    themis::core::concerns::TraceContext ctx;
    ctx.trace_id   = "trace-abc";
    ctx.span_id    = "span-xyz";
    ctx.request_id = "req-001";

    agg.logWithContext(Level::INFO, "request handled", ctx,
                       {{"handler", "query"}});

    ASSERT_EQ(1u, agg.size());
    auto entry = agg.entries()[0];
    EXPECT_EQ("trace-abc",    entry.fields.at("trace_id"));
    EXPECT_EQ("span-xyz",     entry.fields.at("span_id"));
    EXPECT_EQ("req-001",      entry.fields.at("request_id"));
    EXPECT_EQ("query",        entry.fields.at("handler"));
}

TEST(LogAggregatorTest, LogWithContextPartialContext) {
    auto agg = makeAgg();
    themis::core::concerns::TraceContext ctx;
    ctx.trace_id = "trace-only";
    // span_id and request_id are empty

    agg.logWithContext(Level::WARN, "partial ctx", ctx);
    ASSERT_EQ(1u, agg.size());
    auto entry = agg.entries()[0];
    EXPECT_EQ("trace-only", entry.fields.at("trace_id"));
    EXPECT_EQ(entry.fields.end(), entry.fields.find("span_id"));
    EXPECT_EQ(entry.fields.end(), entry.fields.find("request_id"));
}

// ---------------------------------------------------------------------------
// LogEntry::toJson
// ---------------------------------------------------------------------------

TEST(LogAggregatorTest, ToJsonContainsRequiredFields) {
    auto agg = makeAgg();
    agg.logStructured(Level::WARN, "test msg", {{"k", "v"}});
    auto entry = agg.entries()[0];
    auto json  = entry.toJson();

    EXPECT_NE(std::string::npos, json.find("\"level\":\"warn\""));
    EXPECT_NE(std::string::npos, json.find("\"message\":\"test msg\""));
    EXPECT_NE(std::string::npos, json.find("\"timestamp\""));
    EXPECT_NE(std::string::npos, json.find("\"k\":\"v\""));
}

TEST(LogAggregatorTest, ToJsonEscapesSpecialChars) {
    auto agg = makeAgg();
    agg.info("line1\nline2\ttab\"quote");
    auto json = agg.entries()[0].toJson();
    // Newline, tab, and double-quote must be escaped
    EXPECT_EQ(std::string::npos, json.find('\n'));
    EXPECT_EQ(std::string::npos, json.find('\t'));
    EXPECT_NE(std::string::npos, json.find("\\n"));
}

// ---------------------------------------------------------------------------
// Ring buffer behaviour
// ---------------------------------------------------------------------------

TEST(LogAggregatorTest, RingBufferCapEvictsOldest) {
    auto agg = makeAgg(Level::TRACE, /*max_entries=*/3);
    for (int i = 0; i < 5; ++i) {
        agg.info("m" + std::to_string(i));
    }
    EXPECT_EQ(3u, agg.size());
    auto ents = agg.entries();
    EXPECT_EQ("m2", ents[0].message);
    EXPECT_EQ("m4", ents[2].message);
}

TEST(LogAggregatorTest, ClearEmptiesBuffer) {
    auto agg = makeAgg();
    agg.info("a");
    agg.info("b");
    EXPECT_EQ(2u, agg.size());
    agg.clear();
    EXPECT_EQ(0u, agg.size());
}

// ---------------------------------------------------------------------------
// entries() / entriesAtLevel()
// ---------------------------------------------------------------------------

TEST(LogAggregatorTest, EntriesIsChronological) {
    auto agg = makeAgg();
    agg.info("first");
    agg.warn("second");
    auto ents = agg.entries();
    ASSERT_EQ(2u, ents.size());
    EXPECT_EQ("first",  ents[0].message);
    EXPECT_EQ("second", ents[1].message);
}

TEST(LogAggregatorTest, EntriesAtLevelFiltersCorrectly) {
    auto agg = makeAgg(Level::TRACE);
    agg.trace("t");
    agg.info("i");
    agg.error("e");
    auto warn_and_above = agg.entriesAtLevel(Level::ERROR);
    ASSERT_EQ(1u, warn_and_above.size());
    EXPECT_EQ("e", warn_and_above[0].message);
}

// ---------------------------------------------------------------------------
// Statistics
// ---------------------------------------------------------------------------

TEST(LogAggregatorTest, StatsCountAcceptedEntries) {
    auto agg = makeAgg(Level::INFO);
    agg.info("a");
    agg.warn("b");
    agg.error("c");
    auto s = agg.stats();
    EXPECT_EQ(3, s.total_entries);
    // entries_by_level: INFO=1, WARN=1, ERROR=1
    EXPECT_EQ(1, s.entries_by_level[static_cast<int>(Level::INFO)]);
    EXPECT_EQ(1, s.entries_by_level[static_cast<int>(Level::WARN)]);
    EXPECT_EQ(1, s.entries_by_level[static_cast<int>(Level::ERROR)]);
}

TEST(LogAggregatorTest, StatsCountDroppedEntries) {
    auto agg = makeAgg(Level::ERROR);
    agg.trace("drop");
    agg.info("drop");
    auto s = agg.stats();
    EXPECT_EQ(0, s.total_entries);
    EXPECT_EQ(2, s.dropped_entries);
}

// ---------------------------------------------------------------------------
// Entry callback
// ---------------------------------------------------------------------------

TEST(LogAggregatorTest, EntryCallbackInvokedForEachAcceptedEntry) {
    auto agg = makeAgg();
    int count = 0;
    agg.setEntryCallback([&count](const LogEntry&) { ++count; });
    agg.info("one");
    agg.warn("two");
    EXPECT_EQ(2, count);
}

TEST(LogAggregatorTest, EntryCallbackNotInvokedForDroppedEntries) {
    auto agg = makeAgg(Level::ERROR);
    int count = 0;
    agg.setEntryCallback([&count](const LogEntry&) { ++count; });
    agg.debug("dropped");
    EXPECT_EQ(0, count);
}

TEST(LogAggregatorTest, SetCallbackNullRemovesCallback) {
    auto agg = makeAgg();
    int count = 0;
    agg.setEntryCallback([&count](const LogEntry&) { ++count; });
    agg.info("before");
    agg.setEntryCallback(nullptr);
    agg.info("after");
    EXPECT_EQ(1, count);
}

// ---------------------------------------------------------------------------
// Shutdown
// ---------------------------------------------------------------------------

TEST(LogAggregatorTest, ShutdownDropsSubsequentLogs) {
    auto agg = makeAgg();
    agg.info("before shutdown");
    EXPECT_EQ(1u, agg.size());
    agg.shutdown();
    agg.info("after shutdown — must be dropped");
    EXPECT_EQ(1u, agg.size());
}

// ---------------------------------------------------------------------------
// Thread safety
// ---------------------------------------------------------------------------

TEST(LogAggregatorTest, ConcurrentLogging) {
    LogAggregatorConfig cfg;
    cfg.min_level            = Level::INFO;
    cfg.sink_type            = LogSinkType::MEMORY;
    cfg.max_retained_entries = 1000;
    cfg.publish_metrics      = false;
    LogAggregator agg(cfg);

    constexpr int kThreads = 8;
    constexpr int kEach    = 25;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&agg, t] {
            for (int i = 0; i < kEach; ++i) {
                agg.info("thread-" + std::to_string(t) +
                         "-msg-" + std::to_string(i));
            }
        });
    }
    for (auto& th : threads) {
      th.join();
    }

    EXPECT_EQ(kThreads * kEach, agg.stats().total_entries);
}

// ---------------------------------------------------------------------------
// Async streaming
// ---------------------------------------------------------------------------

TEST(LogAggregatorTest, LogAsyncReturnsFutureThatCompletes) {
    auto agg = makeAgg();
    auto f = agg.logAsync(Level::INFO, "async message");
    f.get();  // must not block indefinitely
    EXPECT_GE(agg.size(), 0u); // entry may be in-flight; ensure no crash
}

TEST(LogAggregatorTest, LogAsyncEntryAppearsAfterFutureGet) {
    auto agg = makeAgg();
    agg.logAsync(Level::INFO, "async-info").get();
    EXPECT_EQ(1u, agg.size());
    EXPECT_EQ("async-info", agg.entries()[0].message);
}

TEST(LogAggregatorTest, LogStructuredAsyncStoresFields) {
    auto agg = makeAgg();
    agg.logStructuredAsync(Level::WARN, "async-structured",
                            {{"key", "val"}}).get();
    ASSERT_EQ(1u, agg.size());
    EXPECT_EQ("val", agg.entries()[0].fields.at("key"));
}

TEST(LogAggregatorTest, LogWithContextAsyncInjectsTraceFields) {
    auto agg = makeAgg();
    themis::core::concerns::TraceContext ctx;
    ctx.trace_id   = "trace-async";
    ctx.span_id    = "span-async";
    ctx.request_id = "req-async";
    agg.logWithContextAsync(Level::INFO, "async ctx", ctx,
                             {{"extra", "data"}}).get();
    ASSERT_EQ(1u, agg.size());
    auto entry = agg.entries()[0];
    EXPECT_EQ("trace-async", entry.fields.at("trace_id"));
    EXPECT_EQ("span-async",  entry.fields.at("span_id"));
    EXPECT_EQ("req-async",   entry.fields.at("request_id"));
    EXPECT_EQ("data",        entry.fields.at("extra"));
}

TEST(LogAggregatorTest, AsyncLevelFilterDropsBelowMinLevel) {
    auto agg = makeAgg(Level::WARN);
    agg.logAsync(Level::DEBUG, "should be dropped").get();
    EXPECT_EQ(0u, agg.size());
    EXPECT_EQ(1, agg.stats().dropped_entries);
}

TEST(LogAggregatorTest, AsyncQueueOverflowIncrementsCounter) {
    LogAggregatorConfig cfg;
    cfg.min_level             = Level::TRACE;
    cfg.sink_type             = LogSinkType::MEMORY;
    cfg.max_retained_entries  = 4096;
    cfg.async_queue_max_size  = 2;  // tiny queue to force overflow
    cfg.publish_metrics       = false;
    LogAggregator agg(cfg);

    // Submit many tasks without awaiting – some will overflow
    std::vector<std::future<void>> futures;
    for (int i = 0; i < 20; ++i) {
        futures.push_back(agg.logAsync(Level::INFO,
                                        "msg-" + std::to_string(i)));
    }
    for (auto& f : futures) {
      f.get();
    }

    auto s = agg.stats();
    // Total accepted + overflows must equal submitted
    EXPECT_EQ(20, s.total_entries + s.async_queue_overflows);
    // Some overflows must have occurred (queue size was 2, submitted 20)
    EXPECT_GT(s.async_queue_overflows, 0);
    // Only non-overflowed entries should appear in the buffer
    EXPECT_EQ(static_cast<int64_t>(agg.size()), s.total_entries);
}

TEST(LogAggregatorTest, ShutdownDrainsAsyncQueue) {
    auto agg = makeAgg();
    // Fire several async tasks and then immediately shut down
    std::vector<std::future<void>> futures;
    for (int i = 0; i < 10; ++i) {
        futures.push_back(agg.logAsync(Level::INFO,
                                        "drain-" + std::to_string(i)));
    }
    agg.shutdown();  // must drain queue and join worker
    for (auto& f : futures) {
        // All futures must be already resolved after shutdown
        EXPECT_EQ(std::future_status::ready,
                  f.wait_for(std::chrono::seconds(0)));
    }
}

TEST(LogAggregatorTest, AsyncAfterShutdownResolvesImmediately) {
    auto agg = makeAgg();
    agg.shutdown();
    auto f = agg.logAsync(Level::INFO, "post-shutdown");
    // Future must resolve without blocking
    EXPECT_EQ(std::future_status::ready,
              f.wait_for(std::chrono::seconds(1)));
    // Entry must NOT be logged after shutdown
    EXPECT_EQ(0u, agg.size());
}

TEST(LogAggregatorTest, ConcurrentAsyncLogging) {
    LogAggregatorConfig cfg;
    cfg.min_level            = Level::INFO;
    cfg.sink_type            = LogSinkType::MEMORY;
    cfg.max_retained_entries = 5000;
    cfg.async_queue_max_size = 5000;
    cfg.publish_metrics      = false;
    LogAggregator agg(cfg);

    constexpr int kThreads = 8;
    constexpr int kEach    = 25;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&agg, t] {
            std::vector<std::future<void>> futs;
            for (int i = 0; i < kEach; ++i) {
                futs.push_back(agg.logAsync(
                    Level::INFO,
                    "t" + std::to_string(t) + "-" + std::to_string(i)));
            }
            for (auto& f : futs) {
              f.get();
            }
        });
    }
    for (auto& th : threads) {
      th.join();
    }

    auto s = agg.stats();
    EXPECT_EQ(kThreads * kEach,
              s.total_entries + s.async_queue_overflows);
}
