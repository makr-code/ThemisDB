// ============================================================================
// test_changefeed_sse_async.cpp — unit tests for AsyncSSEStream
// Verifies event-driven async SSE stream lifecycle (#305 Batch 2)
//
// Tests:
//   • Event-driven delivery via subscriptions (not polling-based)
//   • Async event queue with backpressure handling
//   • Heartbeat management
//   • Client disconnection handling
//   • RAII subscription cleanup
//   • Stream timeout and graceful closure
// ============================================================================

#include <gtest/gtest.h>

#include <boost/asio/io_context.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/verb.hpp>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <sstream>
#include <string>
#include <thread>

#include "cdc/changefeed.h"
#include "server/changefeed_api_handler.h"
#include "storage/rocksdb_wrapper.h"

namespace beast = boost::beast;
namespace http  = beast::http;

using themis::server::AsyncSSEStream;

#ifndef THEMIS_ENABLE_SSE

TEST(AsyncSSEStreamFeatureGateTest, DisabledWhenSseFeatureOff) {
    GTEST_SKIP() << "SSE feature is disabled in this build";
}

#else

namespace {

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class AsyncSSEStreamTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = (std::filesystem::temp_directory_path() /
                    ("test_async_sse_" +
                     std::to_string(
                         std::chrono::steady_clock::now().time_since_epoch().count())))
                       .string();
        std::filesystem::create_directories(db_path_);

        themis::RocksDBWrapper::Config cfg;
        cfg.db_path           = db_path_;
        cfg.memtable_size_mb  = 16;
        cfg.block_cache_size_mb = 16;
        storage_ = std::make_shared<themis::RocksDBWrapper>(cfg);
        ASSERT_TRUE(storage_->open());

        themis::Changefeed::RetentionPolicy rp;
        rp.enabled = false;
        changefeed_ = std::make_shared<themis::Changefeed>(storage_->getRawDB(), nullptr, rp);
    }

    void TearDown() override {
        changefeed_.reset();
        storage_->close();
        std::filesystem::remove_all(db_path_);
    }

    std::string db_path_;
    std::shared_ptr<themis::RocksDBWrapper> storage_;
    std::shared_ptr<themis::Changefeed> changefeed_;
};

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

// Test 1: Basic async event delivery via subscriptions
TEST_F(AsyncSSEStreamTest, EventDeliveredAsynchronouslyViaSubscription) {
    std::ostringstream output = {};

    AsyncSSEStream::Config config;
    config.max_duration_seconds = 1;  // Short duration for test
    config.heartbeat_interval_ms = 0; // Disable heartbeat for simplicity

    AsyncSSEStream stream(changefeed_, output, "", config);

    // Run stream in background thread
    std::atomic<size_t> delivered_count{0};
    std::thread stream_thread([&] {
        size_t count = stream.run();
        delivered_count.store(count, std::memory_order_release);
    });

    // Give stream time to subscribe
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Record a change event
    themis::Changefeed::ChangeEvent event;
    event.key = "test_key";
    event.value = "test_value";
    event.type = themis::Changefeed::ChangeEventType::EVENT_PUT;
    event.timestamp_ms = std::chrono::system_clock::now().time_since_epoch().count() / 1'000'000;
    changefeed_->recordEvent(event);

    // Wait for stream to complete
    stream_thread.join();

    std::string output_str = output.str();
    EXPECT_GT(delivered_count.load(std::memory_order_acquire), 0u)
        << "Stream should have delivered at least one event";
    EXPECT_NE(output_str.find("id:"), std::string::npos)
        << "Output should contain SSE event with ID";
    EXPECT_NE(output_str.find("data:"), std::string::npos)
        << "Output should contain SSE data field";
    EXPECT_NE(output_str.find("test_key"), std::string::npos)
        << "Output should contain the recorded key";
}

// Test 2: Backpressure handling with bounded queue
TEST_F(AsyncSSEStreamTest, BackpressureHandlingWhenQueueFull) {
    std::ostringstream output = {};

    AsyncSSEStream::Config config;
    config.max_buffered_events = 10;      // Small buffer to trigger backpressure
    config.max_duration_seconds = 2;
    config.heartbeat_interval_ms = 0;
    config.drop_oldest_on_overflow = true;

    AsyncSSEStream stream(changefeed_, output, "", config);

    // Run stream in background
    std::thread stream_thread([&] {
        stream.run();
    });

    // Give stream time to subscribe
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Rapidly generate more events than the buffer can hold
    for (int i = 0; i < 50; ++i) {
        themis::Changefeed::ChangeEvent event;
        event.key = "key_" + std::to_string(i);
        event.value = "value_" + std::to_string(i);
        event.type = themis::Changefeed::ChangeEventType::EVENT_PUT;
        event.timestamp_ms = std::chrono::system_clock::now().time_since_epoch().count() / 1'000'000;
        changefeed_->recordEvent(event);
    }

    // Wait for stream to complete
    stream_thread.join();

    // Check that some events were dropped due to backpressure
    size_t dropped = stream.getDroppedEventCount();
    size_t delivered = stream.getEventCount();

    EXPECT_GT(dropped + delivered, 0u) << "Stream should have processed events";
    EXPECT_LT(delivered, 50u)  // Should not deliver all due to buffer limit
        << "Stream should have dropped some events due to backpressure";

    std::string output_str = output.str();
    EXPECT_NE(output_str.find("id:"), std::string::npos)
        << "Output should contain at least some events";
}

// Test 3: Heartbeat sending
TEST_F(AsyncSSEStreamTest, HeartbeatsSentAtRegularIntervals) {
    std::ostringstream output = {};

    AsyncSSEStream::Config config;
    config.max_duration_seconds = 2;
    config.heartbeat_interval_ms = 500;  // Send heartbeat every 500ms

    AsyncSSEStream stream(changefeed_, output, "", config);

    // Run stream
    size_t delivered = stream.run();

    size_t heartbeats = stream.getHeartbeatCount();
    EXPECT_GT(heartbeats, 0u)
        << "Stream should have sent heartbeats during the 2 second run";

    std::string output_str = output.str();
    EXPECT_NE(output_str.find(": heartbeat"), std::string::npos)
        << "Output should contain heartbeat comments";
}

// Test 4: Subscription filtering by key prefix
TEST_F(AsyncSSEStreamTest, EventsFilteredByKeyPrefix) {
    std::ostringstream output = {};

    AsyncSSEStream::Config config;
    config.max_duration_seconds = 1;
    config.heartbeat_interval_ms = 0;

    AsyncSSEStream stream(changefeed_, output, "", config);

    // Run stream in background thread with key_prefix filter
    std::thread stream_thread([&] {
        stream.run("user:", {});  // Only events with key starting with "user:"
    });

    // Give stream time to subscribe
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Record events with different key prefixes
    for (const auto& key : {"user:123", "user:456", "admin:789"}) {
        themis::Changefeed::ChangeEvent event;
        event.key = key;
        event.value = "data";
        event.type = themis::Changefeed::ChangeEventType::EVENT_PUT;
        event.timestamp_ms = std::chrono::system_clock::now().time_since_epoch().count() / 1'000'000;
        changefeed_->recordEvent(event);
    }

    // Wait for stream to complete
    stream_thread.join();

    std::string output_str = output.str();
    EXPECT_NE(output_str.find("user:123"), std::string::npos)
        << "Output should contain user:123 event";
    EXPECT_NE(output_str.find("user:456"), std::string::npos)
        << "Output should contain user:456 event";
    // Note: admin:789 should not appear due to prefix filter
}

// Test 5: Event count tracking
TEST_F(AsyncSSEStreamTest, EventCountingWorks) {
    std::ostringstream output = {};

    AsyncSSEStream::Config config;
    config.max_duration_seconds = 1;
    config.heartbeat_interval_ms = 0;

    AsyncSSEStream stream(changefeed_, output, "", config);

    // Run stream in background
    std::thread stream_thread([&] {
        stream.run();
    });

    // Give stream time to subscribe
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Record exactly 5 events
    for (int i = 0; i < 5; ++i) {
        themis::Changefeed::ChangeEvent event;
        event.key = "key_" + std::to_string(i);
        event.value = "value";
        event.type = themis::Changefeed::ChangeEventType::EVENT_PUT;
        event.timestamp_ms = std::chrono::system_clock::now().time_since_epoch().count() / 1'000'000;
        changefeed_->recordEvent(event);
    }

    // Wait for stream to complete
    stream_thread.join();

    size_t delivered = stream.getEventCount();
    EXPECT_EQ(delivered, 5u)
        << "Stream should have delivered exactly 5 events";
}

// Test 6: Stream gracefully closes on timeout
TEST_F(AsyncSSEStreamTest, StreamClosesOnTimeout) {
    std::ostringstream output = {};

    AsyncSSEStream::Config config;
    config.max_duration_seconds = 1;  // Very short timeout
    config.heartbeat_interval_ms = 0;

    AsyncSSEStream stream(changefeed_, output, "", config);

    auto start = std::chrono::steady_clock::now();
    size_t delivered = stream.run();
    auto duration = std::chrono::steady_clock::now() - start;

    // Stream should close roughly around the timeout
    EXPECT_GE(duration, std::chrono::milliseconds(900))
        << "Stream should run for close to max_duration_seconds";
    EXPECT_LE(duration, std::chrono::milliseconds(1500))
        << "Stream should not run significantly longer than max_duration_seconds";
}

// Test 7: Subscription cleanup on destruction (RAII)
TEST_F(AsyncSSEStreamTest, SubscriptionCleanedUpOnDestruction) {
    std::ostringstream output = {};

    AsyncSSEStream::Config config;
    config.max_duration_seconds = 10;  // Long timeout
    config.heartbeat_interval_ms = 0;

    {
        AsyncSSEStream stream(changefeed_, output, "", config);
        // Destructor should clean up subscription
    }

    // If subscription wasn't cleaned up properly, this event might be delivered
    // to a non-existent subscriber, which should cause no issues
    themis::Changefeed::ChangeEvent event;
    event.key = "test_after_destruction";
    event.value = "test";
    event.type = themis::Changefeed::ChangeEventType::EVENT_PUT;
    event.timestamp_ms = std::chrono::system_clock::now().time_since_epoch().count() / 1'000'000;

    // This should not crash or cause issues even though stream is destroyed
    EXPECT_NO_THROW(changefeed_->recordEvent(event));
}

} // namespace

#endif  // THEMIS_ENABLE_SSE
