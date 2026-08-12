// Test: ChangefeedBuffer Unit Tests
// Covers: start/stop lifecycle, recordEvent buffering, flush(), flushFor(),
//         getStats(), setConfig(), compression, async flush, DLQ integration,
//         rate limiting config, empty-key guard, multiple event types.

#include <gtest/gtest.h>
#include "cdc/changefeed_buffer.h"
#include "cdc/changefeed.h"
#include "cdc/dead_letter_queue.h"
#include <rocksdb/utilities/transaction_db.h>
#include <filesystem>
#include <thread>
#include <chrono>
#include <atomic>

namespace fs = std::filesystem;
using namespace themis;
using namespace themis::cdc;

// ---------------------------------------------------------------------------
// Test fixture
// ---------------------------------------------------------------------------
class ChangefeedBufferTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_db_path_ = "/tmp/test_cfbuf_" +
                        std::to_string(std::chrono::steady_clock::now()
                                           .time_since_epoch()
                                           .count());
        fs::create_directories(test_db_path_);

        rocksdb::Options opts;
        opts.create_if_missing = true;
        rocksdb::TransactionDBOptions txn_opts;
        rocksdb::TransactionDB* raw = nullptr;
        auto s = rocksdb::TransactionDB::Open(opts, txn_opts, test_db_path_, &raw);
        ASSERT_TRUE(s.ok()) << s.ToString();
        db_.reset(raw);

        Changefeed::RetentionPolicy rp;
        rp.enabled = false;
        changefeed_ = std::make_unique<Changefeed>(db_.get(), nullptr, rp);
    }

    void TearDown() override {
        changefeed_.reset();
        db_.reset();
        fs::remove_all(test_db_path_);
    }

    Changefeed::ChangeEvent makePut(const std::string& key,
                                    const std::string& value = "{}") {
        Changefeed::ChangeEvent ev;
        ev.type  = Changefeed::ChangeEventType::EVENT_PUT;
        ev.key   = key;
        ev.value = value;
        return ev;
    }

    Changefeed::ChangeEvent makeDelete(const std::string& key) {
        Changefeed::ChangeEvent ev;
        ev.type  = Changefeed::ChangeEventType::EVENT_DELETE;
        ev.key   = key;
        return ev;
    }

    std::string                       test_db_path_;
    std::unique_ptr<rocksdb::TransactionDB> db_;
    std::unique_ptr<Changefeed>       changefeed_;
};

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

TEST_F(ChangefeedBufferTest, NullChangefeedThrows) {
    EXPECT_THROW(ChangefeedBuffer(nullptr), std::invalid_argument);
}

TEST_F(ChangefeedBufferTest, ValidConstructionDoesNotThrow) {
    ChangefeedBufferConfig cfg;
    EXPECT_NO_THROW(ChangefeedBuffer(changefeed_.get(), cfg));
}

// ---------------------------------------------------------------------------
// Lifecycle: start / stop
// ---------------------------------------------------------------------------

TEST_F(ChangefeedBufferTest, StartAndStopLifecycle) {
    ChangefeedBufferConfig cfg;
    cfg.async_flush = false;
    ChangefeedBuffer buf(changefeed_.get(), cfg);

    EXPECT_FALSE(buf.isRunning());
    buf.start();
    EXPECT_TRUE(buf.isRunning());
    buf.stop();
    EXPECT_FALSE(buf.isRunning());
}

TEST_F(ChangefeedBufferTest, DoubleStartIsIdempotent) {
    ChangefeedBufferConfig cfg;
    cfg.async_flush = false;
    ChangefeedBuffer buf(changefeed_.get(), cfg);

    buf.start();
    EXPECT_NO_THROW(buf.start()); // second start should be a no-op
    EXPECT_TRUE(buf.isRunning());
    buf.stop();
}

TEST_F(ChangefeedBufferTest, DestructorStopsBuffer) {
    ChangefeedBufferConfig cfg;
    cfg.async_flush = false;
    {
        ChangefeedBuffer buf(changefeed_.get(), cfg);
        buf.start();
        EXPECT_TRUE(buf.isRunning());
        // destructor should call stop() without throwing
    }
    // If we get here, destructor did not crash
    SUCCEED();
}

// ---------------------------------------------------------------------------
// recordEvent
// ---------------------------------------------------------------------------

TEST_F(ChangefeedBufferTest, RecordEventReturnsSequenceZeroWhileBuffered) {
    ChangefeedBufferConfig cfg;
    cfg.max_events_per_buffer = 100; // large so no auto-flush
    cfg.async_flush = false;
    ChangefeedBuffer buf(changefeed_.get(), cfg);
    buf.start();

    auto result = buf.recordEvent(makePut("k1", "v1"));
    // Sequence 0 signals the event is buffered, not yet recorded to changefeed
    EXPECT_EQ(result.sequence, 0u);

    buf.stop();
}

TEST_F(ChangefeedBufferTest, EmptyKeyReturnsEventWithoutRecording) {
    ChangefeedBufferConfig cfg;
    cfg.async_flush = false;
    ChangefeedBuffer buf(changefeed_.get(), cfg);
    buf.start();

    Changefeed::ChangeEvent ev;
    ev.type  = Changefeed::ChangeEventType::EVENT_PUT;
    ev.key   = ""; // intentionally empty
    ev.value = "ignored";

    EXPECT_NO_THROW({
        auto result = buf.recordEvent(ev);
        EXPECT_EQ(result.sequence, 0u);
    });

    buf.stop();
}

TEST_F(ChangefeedBufferTest, RecordMultipleEventsBeforeFlush) {
    ChangefeedBufferConfig cfg;
    cfg.max_events_per_buffer = 100;
    cfg.async_flush = false;
    ChangefeedBuffer buf(changefeed_.get(), cfg);
    buf.start();

    for (int i = 0; i < 10; i++) {
        buf.recordEvent(makePut("key_" + std::to_string(i)));
    }

    const auto& stats = buf.getStats();
    EXPECT_GE(stats.events_buffered.load(), 10u);

    buf.stop(); // stop performs final flush
}

// ---------------------------------------------------------------------------
// flush()
// ---------------------------------------------------------------------------

TEST_F(ChangefeedBufferTest, ExplicitFlushWritesToChangefeed) {
    ChangefeedBufferConfig cfg;
    cfg.max_events_per_buffer = 100;
    cfg.async_flush = false;
    ChangefeedBuffer buf(changefeed_.get(), cfg);
    buf.start();

    for (int i = 0; i < 5; i++) {
        buf.recordEvent(makePut("flush_key_" + std::to_string(i)));
    }

    size_t flushed = buf.flush();
    EXPECT_EQ(flushed, 5u);

    // Events should now be in the changefeed
    auto events = changefeed_->listEvents();
    EXPECT_EQ(events.size(), 5u);

    buf.stop();
}

TEST_F(ChangefeedBufferTest, FlushEmptyBufferReturnsZero) {
    ChangefeedBufferConfig cfg;
    cfg.async_flush = false;
    ChangefeedBuffer buf(changefeed_.get(), cfg);
    buf.start();

    size_t flushed = buf.flush();
    EXPECT_EQ(flushed, 0u);

    buf.stop();
}

TEST_F(ChangefeedBufferTest, StopPerformsFinalFlush) {
    ChangefeedBufferConfig cfg;
    cfg.max_events_per_buffer = 100;
    cfg.async_flush = false;
    ChangefeedBuffer buf(changefeed_.get(), cfg);
    buf.start();

    for (int i = 0; i < 8; i++) {
        buf.recordEvent(makePut("stop_flush_" + std::to_string(i)));
    }

    buf.stop(); // must flush remaining buffered events

    auto events = changefeed_->listEvents();
    EXPECT_EQ(events.size(), 8u);
}

// ---------------------------------------------------------------------------
// flushFor(event_type)
// ---------------------------------------------------------------------------

TEST_F(ChangefeedBufferTest, FlushForSpecificEventType) {
    ChangefeedBufferConfig cfg;
    cfg.max_events_per_buffer = 100;
    cfg.async_flush = false;
    ChangefeedBuffer buf(changefeed_.get(), cfg);
    buf.start();

    // Buffer a mix of PUT and DELETE events
    buf.recordEvent(makePut("put_key_0"));
    buf.recordEvent(makePut("put_key_1"));
    buf.recordEvent(makeDelete("del_key_0"));

    // Flush only PUT events
    size_t flushed = buf.flushFor(Changefeed::ChangeEventType::EVENT_PUT);
    EXPECT_EQ(flushed, 2u);

    buf.stop();
}

TEST_F(ChangefeedBufferTest, FlushForUnknownTypeReturnsZero) {
    ChangefeedBufferConfig cfg;
    cfg.async_flush = false;
    ChangefeedBuffer buf(changefeed_.get(), cfg);
    buf.start();

    // No DELETE events buffered
    size_t flushed = buf.flushFor(Changefeed::ChangeEventType::EVENT_DELETE);
    EXPECT_EQ(flushed, 0u);

    buf.stop();
}

// ---------------------------------------------------------------------------
// getStats()
// ---------------------------------------------------------------------------

TEST_F(ChangefeedBufferTest, StatsInitiallyZero) {
    ChangefeedBufferConfig cfg;
    cfg.async_flush = false;
    ChangefeedBuffer buf(changefeed_.get(), cfg);

    const auto& stats = buf.getStats();
    EXPECT_EQ(stats.events_buffered.load(), 0u);
    EXPECT_EQ(stats.events_flushed.load(),  0u);
    EXPECT_EQ(stats.flush_count.load(),     0u);
}

TEST_F(ChangefeedBufferTest, StatsTrackBufferedAndFlushedEvents) {
    ChangefeedBufferConfig cfg;
    cfg.max_events_per_buffer = 100;
    cfg.async_flush = false;
    ChangefeedBuffer buf(changefeed_.get(), cfg);
    buf.start();

    for (int i = 0; i < 6; i++) {
        buf.recordEvent(makePut("stats_key_" + std::to_string(i)));
    }

    EXPECT_GE(buf.getStats().events_buffered.load(), 6u);

    buf.flush();

    EXPECT_GE(buf.getStats().events_flushed.load(), 6u);
    EXPECT_GE(buf.getStats().flush_count.load(),    1u);

    buf.stop();
}

// ---------------------------------------------------------------------------
// setConfig()
// ---------------------------------------------------------------------------

TEST_F(ChangefeedBufferTest, SetConfigUpdatesFlushInterval) {
    ChangefeedBufferConfig cfg;
    cfg.async_flush = false;
    ChangefeedBuffer buf(changefeed_.get(), cfg);
    buf.start();

    ChangefeedBufferConfig new_cfg;
    new_cfg.flush_interval = std::chrono::milliseconds(500);
    new_cfg.async_flush    = false;

    EXPECT_NO_THROW(buf.setConfig(new_cfg));

    buf.stop();
}

// ---------------------------------------------------------------------------
// Auto-flush on size threshold
// ---------------------------------------------------------------------------

TEST_F(ChangefeedBufferTest, AutoFlushWhenSizeThresholdReached) {
    ChangefeedBufferConfig cfg;
    cfg.max_events_per_buffer = 5; // flush every 5 events
    cfg.async_flush = false;
    ChangefeedBuffer buf(changefeed_.get(), cfg);
    buf.start();

    // Record exactly max_events_per_buffer events — should trigger an auto-flush
    for (int i = 0; i < 5; i++) {
        buf.recordEvent(makePut("auto_" + std::to_string(i)));
    }

    // The in-process flush triggered by recordEvent should have written them
    auto events = changefeed_->listEvents();
    EXPECT_GE(events.size(), 5u);

    buf.stop();
}

// ---------------------------------------------------------------------------
// Compression
// ---------------------------------------------------------------------------

TEST_F(ChangefeedBufferTest, CompressionEnabledDoesNotDropEvents) {
    ChangefeedBufferConfig cfg;
    cfg.compress_payloads             = true;
    cfg.compression_threshold_bytes   = 10; // compress anything > 10 bytes
    cfg.max_events_per_buffer         = 100;
    cfg.async_flush                   = false;
    ChangefeedBuffer buf(changefeed_.get(), cfg);
    buf.start();

    // A payload well above the threshold
    buf.recordEvent(makePut("comp_key", std::string(200, 'Z')));

    size_t flushed = buf.flush();
    EXPECT_EQ(flushed, 1u);

    // The event should be persisted and readable
    auto events = changefeed_->listEvents();
    EXPECT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].key, "comp_key");

    buf.stop();
}

TEST_F(ChangefeedBufferTest, CompressionDisabledStillFlushes) {
    ChangefeedBufferConfig cfg;
    cfg.compress_payloads     = false;
    cfg.max_events_per_buffer = 100;
    cfg.async_flush           = false;
    ChangefeedBuffer buf(changefeed_.get(), cfg);
    buf.start();

    buf.recordEvent(makePut("nocomp_key", std::string(200, 'A')));
    size_t flushed = buf.flush();
    EXPECT_EQ(flushed, 1u);

    buf.stop();
}

// ---------------------------------------------------------------------------
// Rate limiting config (just ensure no crash when enabled)
// ---------------------------------------------------------------------------

TEST_F(ChangefeedBufferTest, RateLimitingConfigDoesNotCrash) {
    ChangefeedBufferConfig cfg;
    cfg.enable_rate_limiting     = true;
    cfg.max_events_per_second    = 1000;
    cfg.rate_limit_window        = std::chrono::milliseconds(100);
    cfg.max_events_per_buffer    = 100;
    cfg.async_flush              = false;
    ChangefeedBuffer buf(changefeed_.get(), cfg);
    buf.start();

    for (int i = 0; i < 3; i++) {
        EXPECT_NO_THROW(buf.recordEvent(makePut("rl_key_" + std::to_string(i))));
    }

    buf.stop();
}

// ---------------------------------------------------------------------------
// DLQ integration
// ---------------------------------------------------------------------------

TEST_F(ChangefeedBufferTest, SetDeadLetterQueueAttaches) {
    DeadLetterQueue dlq(db_.get(), nullptr);

    ChangefeedBufferConfig cfg;
    cfg.async_flush = false;
    ChangefeedBuffer buf(changefeed_.get(), cfg);

    buf.setDeadLetterQueue(&dlq);
    EXPECT_EQ(buf.getDeadLetterQueue(), &dlq);

    buf.start();
    buf.stop();
}

// ---------------------------------------------------------------------------
// Async flush (brief smoke-test — just verify no crash or hang)
// ---------------------------------------------------------------------------

TEST_F(ChangefeedBufferTest, AsyncFlushThreadStartsAndStops) {
    ChangefeedBufferConfig cfg;
    cfg.async_flush    = true;
    cfg.flush_interval = std::chrono::milliseconds(50);
    ChangefeedBuffer buf(changefeed_.get(), cfg);

    buf.start();

    for (int i = 0; i < 5; i++) {
        buf.recordEvent(makePut("async_" + std::to_string(i)));
    }

    // Give the flush thread a chance to run
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    buf.stop(); // must join flush thread

    // Verify events ended up in the changefeed
    auto events = changefeed_->listEvents();
    EXPECT_GE(events.size(), 5u);
}

// ---------------------------------------------------------------------------
// Multiple event-type buffers coexist
// ---------------------------------------------------------------------------

TEST_F(ChangefeedBufferTest, PutAndDeleteEventsBufferedSeparately) {
    ChangefeedBufferConfig cfg;
    cfg.max_events_per_buffer = 100;
    cfg.async_flush = false;
    ChangefeedBuffer buf(changefeed_.get(), cfg);
    buf.start();

    buf.recordEvent(makePut("key_a"));
    buf.recordEvent(makeDelete("key_b"));
    buf.recordEvent(makePut("key_c"));

    size_t flushed = buf.flush();
    EXPECT_EQ(flushed, 3u);

    auto events = changefeed_->listEvents();
    EXPECT_EQ(events.size(), 3u);

    buf.stop();
}
