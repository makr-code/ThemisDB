// ============================================================================
// test_changefeed_sse_writer.cpp — unit tests for ChangefeedApiHandler's
// SseStreamWriterFn bridge (stub #305 resolution, W1-S05 follow-up 5)
//
// Verifies:
//   • Path A: a registered SseStreamWriterFn is invoked for keep-alive SSE
//             requests when THEMIS_ENABLE_SSE is active.
//   • clearSseStreamWriterFn() resets the bridge so a subsequent writer can
//     be registered without stale state from a prior test.
//   • An exception thrown by the registered writer does not propagate to the
//     caller; the handler catches it and falls through to Path B (sync loop).
//   • Thread-safety of set/clear: concurrent writers and a clear do not
//     produce a crash.
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
#include <stdexcept>
#include <string>
#include <thread>

#include "cdc/changefeed.h"
#include "server/changefeed_api_handler.h"
#include "server/sse_connection_manager.h"
#include "storage/rocksdb_wrapper.h"

namespace beast = boost::beast;
namespace http  = beast::http;

using themis::server::ChangefeedApiHandler;
using themis::server::SseConnectionManager;

#ifndef THEMIS_ENABLE_SSE

TEST(ChangefeedSseWriterFeatureGateTest, DisabledWhenSseFeatureOff) {
    GTEST_SKIP() << "SSE feature is disabled in this build";
}

#else

namespace {

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class ChangefeedSseWriterTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = (std::filesystem::temp_directory_path() /
                    ("test_sse_writer_" +
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

        sse_manager_ = std::make_shared<SseConnectionManager>(changefeed_, ioc_);

        // auth = nullptr → all checkAuth calls return nullopt (no auth enforcement)
        handler_ = std::make_unique<ChangefeedApiHandler>(
            storage_, changefeed_, sse_manager_,
            /*auth=*/nullptr,
            /*feature_cdc=*/true);
    }

    void TearDown() override {
        // Always clear the static writer to prevent cross-test contamination.
        ChangefeedApiHandler::clearSseStreamWriterFn();

        handler_.reset();
        sse_manager_.reset();
        changefeed_.reset();
        storage_->close();
        std::filesystem::remove_all(db_path_);
    }

    // Build a minimal HTTP/1.1 GET request for /changefeed/stream.
    static http::request<http::string_body> makeStreamRequest(const std::string& query)
    {
        http::request<http::string_body> req{http::verb::get,
                                             "/changefeed/stream?" + query, 11};
        req.set(http::field::host, "localhost");
        return req;
    }

    std::string db_path_;
    boost::asio::io_context ioc_;
    std::shared_ptr<themis::RocksDBWrapper> storage_;
    std::shared_ptr<themis::Changefeed>     changefeed_;
    std::shared_ptr<SseConnectionManager>   sse_manager_;
    std::unique_ptr<ChangefeedApiHandler>   handler_;
};

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

#ifdef THEMIS_ENABLE_SSE

// Path A: a registered SseStreamWriterFn is called when keep_alive=true.
TEST_F(ChangefeedSseWriterTest, RegisteredWriterIsCalledForKeepAliveRequest) {
    std::atomic<bool> writer_called{false};

    ChangefeedApiHandler::setSseStreamWriterFn(
        [&](SseConnectionManager& /*mgr*/, uint64_t /*conn_id*/,
            std::ostream& body,
            std::chrono::seconds /*max_duration*/,
            uint32_t /*heartbeat_ms*/,
            size_t /*max_events_per_poll*/) {
            writer_called.store(true, std::memory_order_release);
            body << "data: {\"test\":true}\n\n";
        });

    auto req = makeStreamRequest("from_seq=0&keep_alive=true&max_seconds=1");
    auto res = handler_->handleStreamSse(req);

    EXPECT_EQ(res.result(), http::status::ok);
    EXPECT_TRUE(writer_called.load(std::memory_order_acquire))
        << "SseStreamWriterFn was not called for a keep_alive=true request";
}

// Path A: the writer receives the correct connection parameters.
TEST_F(ChangefeedSseWriterTest, WriterReceivesCorrectParameters) {
    std::chrono::seconds captured_max_duration{0};
    uint32_t             captured_heartbeat_ms = 0;
    size_t               captured_max_events   = 0;
    uint64_t             captured_conn_id      = 0;

    ChangefeedApiHandler::setSseStreamWriterFn(
        [&](SseConnectionManager& /*mgr*/, uint64_t conn_id,
            std::ostream& /*body*/,
            std::chrono::seconds max_duration,
            uint32_t heartbeat_ms,
            size_t max_events_per_poll) {
            captured_conn_id        = conn_id;
            captured_max_duration   = max_duration;
            captured_heartbeat_ms   = heartbeat_ms;
            captured_max_events     = max_events_per_poll;
        });

    auto req = makeStreamRequest(
        "from_seq=0&keep_alive=true&max_seconds=5"
        "&heartbeat_ms=2000&max_events_per_poll=50");
    (void)handler_->handleStreamSse(req);

    EXPECT_NE(captured_conn_id, 0u) << "conn_id should be a valid non-zero registration id";
    EXPECT_EQ(captured_max_duration, std::chrono::seconds(5));
    EXPECT_EQ(captured_heartbeat_ms, 2000u);
    EXPECT_EQ(captured_max_events, 50u);
}

// clearSseStreamWriterFn() removes the registered writer so a subsequent
// request does not call the old function.
TEST_F(ChangefeedSseWriterTest, ClearWriterPreventsCallOnNextRequest) {
    std::atomic<int> call_count{0};

    ChangefeedApiHandler::setSseStreamWriterFn(
        [&](SseConnectionManager&, uint64_t, std::ostream& body,
            std::chrono::seconds, uint32_t, size_t) {
            call_count.fetch_add(1, std::memory_order_relaxed);
            body << "data: {\"round\":1}\n\n";
        });

    auto req1 = makeStreamRequest("from_seq=0&keep_alive=true&max_seconds=1");
    (void)handler_->handleStreamSse(req1);
    ASSERT_EQ(call_count.load(), 1) << "Writer should be called on first request";

    // Clear and verify the next request does NOT call the old writer.
    ChangefeedApiHandler::clearSseStreamWriterFn();

    // Path B (sync loop) runs for max_seconds=1 when no writer is set.
    auto req2 = makeStreamRequest("from_seq=0&keep_alive=true&max_seconds=1");
    (void)handler_->handleStreamSse(req2);

    EXPECT_EQ(call_count.load(), 1)
        << "Cleared writer should not be called on subsequent requests";
}

// A writer that replaces a previous one via setSseStreamWriterFn is used on
// the next request (no stale function pointer is retained).
TEST_F(ChangefeedSseWriterTest, ReplaceWriterUsesNewFunction) {
    std::atomic<int> first_count{0};
    std::atomic<int> second_count{0};

    ChangefeedApiHandler::setSseStreamWriterFn(
        [&](SseConnectionManager&, uint64_t, std::ostream& body,
            std::chrono::seconds, uint32_t, size_t) {
            first_count.fetch_add(1, std::memory_order_relaxed);
            body << "data: {\"writer\":1}\n\n";
        });

    auto req1 = makeStreamRequest("from_seq=0&keep_alive=true&max_seconds=1");
    (void)handler_->handleStreamSse(req1);
    ASSERT_EQ(first_count.load(), 1);

    // Replace with a new writer.
    ChangefeedApiHandler::setSseStreamWriterFn(
        [&](SseConnectionManager&, uint64_t, std::ostream& body,
            std::chrono::seconds, uint32_t, size_t) {
            second_count.fetch_add(1, std::memory_order_relaxed);
            body << "data: {\"writer\":2}\n\n";
        });

    auto req2 = makeStreamRequest("from_seq=0&keep_alive=true&max_seconds=1");
    (void)handler_->handleStreamSse(req2);

    EXPECT_EQ(first_count.load(), 1)  << "Old writer must not be called after replacement";
    EXPECT_EQ(second_count.load(), 1) << "New writer must be called after replacement";
}

// An exception thrown by the registered writer must not propagate; the handler
// catches it and falls through to Path B.  The response status must still be OK.
TEST_F(ChangefeedSseWriterTest, ThrowingWriterFallsThroughToPathBWithoutCrash) {
    std::atomic<bool> writer_called{false};

    ChangefeedApiHandler::setSseStreamWriterFn(
        [&](SseConnectionManager&, uint64_t, std::ostream&,
            std::chrono::seconds, uint32_t, size_t) {
            writer_called.store(true, std::memory_order_release);
            throw std::runtime_error("simulated async writer failure");
        });

    // max_seconds=1 so Path B terminates quickly after the writer throws.
    auto req = makeStreamRequest("from_seq=0&keep_alive=true&max_seconds=1");
    http::response<http::string_body> res;
    ASSERT_NO_THROW(res = handler_->handleStreamSse(req));

    EXPECT_TRUE(writer_called.load(std::memory_order_acquire))
        << "Writer should have been invoked before throwing";
    EXPECT_EQ(res.result(), http::status::ok)
        << "Response must remain 200 OK even when the writer throws";
}

// Thread-safety: concurrent set and clear must not crash.
TEST_F(ChangefeedSseWriterTest, ConcurrentSetAndClearDoNotCrash) {
    std::atomic<bool> stop{false};

    // Writer thread: sets a no-op writer repeatedly.
    std::thread setter([&] {
        while (!stop.load(std::memory_order_relaxed)) {
            ChangefeedApiHandler::setSseStreamWriterFn(
                [](SseConnectionManager&, uint64_t, std::ostream& body,
                   std::chrono::seconds, uint32_t, size_t) {
                    body << "data: {}\n\n";
                });
            std::this_thread::yield();
        }
    });

    // Clearer thread: clears the writer repeatedly.
    std::thread clearer([&] {
        while (!stop.load(std::memory_order_relaxed)) {
            ChangefeedApiHandler::clearSseStreamWriterFn();
            std::this_thread::yield();
        }
    });

    // Run concurrent set/clear for a short period.
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    stop.store(true, std::memory_order_release);

    setter.join();
    clearer.join();
    // No assertion needed — the test passes if we reach here without crashing.
}

#else // !THEMIS_ENABLE_SSE

// When THEMIS_ENABLE_SSE is not defined the SSE path is compiled out.
// Ensure setSseStreamWriterFn / clearSseStreamWriterFn are still callable
// and handleStreamSse returns a well-formed (non-SSE) response.
TEST_F(ChangefeedSseWriterTest, SseDisabledSetClearAreNoOps) {
    ASSERT_NO_THROW(ChangefeedApiHandler::setSseStreamWriterFn(
        [](SseConnectionManager&, uint64_t, std::ostream&,
           std::chrono::seconds, uint32_t, size_t) {}));
    ASSERT_NO_THROW(ChangefeedApiHandler::clearSseStreamWriterFn());

    auto req = makeStreamRequest("from_seq=0&keep_alive=false");
    http::response<http::string_body> res;
    ASSERT_NO_THROW(res = handler_->handleStreamSse(req));
    // Response must still be well-formed (2xx or 4xx, but not a crash).
    EXPECT_GT(static_cast<unsigned>(res.result_int()), 0u);
}

#endif // THEMIS_ENABLE_SSE

} // namespace

#endif
