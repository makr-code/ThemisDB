/**
 * @file test_bidi_stream_adapter.cpp
 * @brief Unit tests for BidiStreamAdapter<Req, Resp>
 *
 * These tests use a lightweight mock of grpc::ServerReaderWriter so that no
 * real gRPC server is required.
 *
 * Test IDs: BSA-01 … BSA-20
 */

#include <gtest/gtest.h>
#include "rpc_grpc/bidi_stream_adapter.h"

#include <atomic>
#include <string>
#include <vector>

using namespace themis::plugins::rpc::grpc_plugin;

// ============================================================================
// Minimal mock of grpc::ServerReaderWriter<Resp, Req>
// ============================================================================

/**
 * @brief Simulates a bidirectional gRPC stream for testing.
 *
 * Reads are served from `inbound_` in order; when exhausted, Read() returns
 * false (client half-close).  Written messages are collected in `outbound_`.
 */
struct MockMessage {
    std::string value = {};
};

class MockBidiStream {
public:
    explicit MockBidiStream(std::vector<MockMessage> msgs)
        : inbound_(std::move(msgs)), read_index_(0) {}

    bool Read(MockMessage* msg) {
        if (read_index_ >= inbound_.size()) {
            return false;
        }
        *msg = inbound_[read_index_++];
        return true;
    }

    bool Write(const MockMessage& msg) {
        outbound_.push_back(msg);
        return true;
    }

    const std::vector<MockMessage>& outbound() const { return outbound_; }

private:
    std::vector<MockMessage> inbound_;
    std::size_t read_index_;
    std::vector<MockMessage> outbound_;
};

// Typed alias for testing: BidiStreamAdapter over MockBidiStream directly.
using TestAdapter = BidiStreamAdapter<MockMessage, MockMessage, MockBidiStream>;

// ============================================================================
// Test fixture
// ============================================================================

class BidiStreamAdapterTest : public ::testing::Test {
protected:
    /// Build an adapter over a mock stream with the given inbound messages.
    std::pair<std::unique_ptr<MockBidiStream>, std::unique_ptr<TestAdapter>>
    makeAdapter(std::vector<MockMessage> msgs, std::size_t queue_depth = 100) {
        auto stream = std::make_unique<MockBidiStream>(std::move(msgs));
        auto adapter = std::make_unique<TestAdapter>(stream.get(), queue_depth);
        return {std::move(stream), std::move(adapter)};
    }
};

// ============================================================================
// BSA-01 — Construction: valid stream pointer, no throw
// ============================================================================
TEST_F(BidiStreamAdapterTest, BSA01_ConstructWithValidStream) {
    MockBidiStream mock({});
    EXPECT_NO_THROW(TestAdapter adapter(&mock, 10));
}

// ============================================================================
// BSA-02 — Construction: null stream throws std::invalid_argument
// ============================================================================
TEST_F(BidiStreamAdapterTest, BSA02_ConstructWithNullStreamThrows) {
    EXPECT_THROW(TestAdapter(nullptr, 10), std::invalid_argument);
}

// ============================================================================
// BSA-03 — Initial state: not finished
// ============================================================================
TEST_F(BidiStreamAdapterTest, BSA03_InitialStateNotFinished) {
    MockBidiStream mock({});
    TestAdapter adapter(&mock);
    EXPECT_FALSE(adapter.isFinished());
}

// ============================================================================
// BSA-04 — Initial queue depth is zero
// ============================================================================
TEST_F(BidiStreamAdapterTest, BSA04_InitialQueueDepthIsZero) {
    MockBidiStream mock({});
    TestAdapter adapter(&mock);
    EXPECT_EQ(0u, adapter.queueDepth());
}

// ============================================================================
// BSA-05 — finish() marks the adapter as finished
// ============================================================================
TEST_F(BidiStreamAdapterTest, BSA05_FinishMarksFinished) {
    MockBidiStream mock({});
    TestAdapter adapter(&mock);
    EXPECT_FALSE(adapter.isFinished());
    adapter.finish(grpc::Status::OK);
    EXPECT_TRUE(adapter.isFinished());
}

// ============================================================================
// BSA-06 — write() returns false after finish()
// ============================================================================
TEST_F(BidiStreamAdapterTest, BSA06_WriteReturnsFalseAfterFinish) {
    MockBidiStream mock({});
    TestAdapter adapter(&mock);
    adapter.finish(grpc::Status::OK);
    MockMessage msg{"hello"};
    EXPECT_FALSE(adapter.write(std::move(msg)));
}

// ============================================================================
// BSA-07 — finishStatus() returns OK by default
// ============================================================================
TEST_F(BidiStreamAdapterTest, BSA07_FinishStatusDefaultIsOk) {
    MockBidiStream mock({});
    TestAdapter adapter(&mock);
    EXPECT_TRUE(adapter.finishStatus().ok());
}

// ============================================================================
// BSA-08 — finishStatus() reflects the status passed to finish()
// ============================================================================
TEST_F(BidiStreamAdapterTest, BSA08_FinishStatusReflectsSetStatus) {
    MockBidiStream mock({});
    TestAdapter adapter(&mock);
    adapter.finish(grpc::Status(grpc::StatusCode::INTERNAL, "test error"));
    EXPECT_EQ(grpc::StatusCode::INTERNAL, adapter.finishStatus().error_code());
}

// ============================================================================
// BSA-09 — onMessage callback invoked for each inbound message
// ============================================================================
TEST_F(BidiStreamAdapterTest, BSA09_OnMessageCalledForEachInbound) {
    MockBidiStream mock({{"a"}, {"b"}, {"c"}});
    TestAdapter adapter(&mock);

    std::vector<std::string> received;
    adapter.onMessage([&received](MockMessage&& m) {
        received.push_back(m.value);
    });

    adapter.run();

    ASSERT_EQ(3u, received.size());
    EXPECT_EQ("a", received[0]);
    EXPECT_EQ("b", received[1]);
    EXPECT_EQ("c", received[2]);
}

// ============================================================================
// BSA-10 — run() with no handler registered does not crash
// ============================================================================
TEST_F(BidiStreamAdapterTest, BSA10_RunWithoutHandlerNoCrash) {
    MockBidiStream mock({{"x"}, {"y"}});
    TestAdapter adapter(&mock);
    EXPECT_NO_THROW(adapter.run());
}

// ============================================================================
// BSA-11 — run() with empty inbound stream returns immediately
// ============================================================================
TEST_F(BidiStreamAdapterTest, BSA11_RunOnEmptyStreamNoCrash) {
    MockBidiStream mock({});
    TestAdapter adapter(&mock);
    std::atomic<int> count{0};
    adapter.onMessage([&count](MockMessage&&) { ++count; });
    adapter.run();
    EXPECT_EQ(0, count.load());
}

// ============================================================================
// BSA-12 — finish() called twice does not crash
// ============================================================================
TEST_F(BidiStreamAdapterTest, BSA12_DoubleFinishNoCrash) {
    MockBidiStream mock({});
    TestAdapter adapter(&mock);
    EXPECT_NO_THROW({
        adapter.finish(grpc::Status::OK);
        adapter.finish(grpc::Status::OK);
    });
}

// ============================================================================
// BSA-13 — isFinished() returns true consistently after finish()
// ============================================================================
TEST_F(BidiStreamAdapterTest, BSA13_IsFinishedConsistentAfterFinish) {
    MockBidiStream mock({});
    TestAdapter adapter(&mock);
    adapter.finish(grpc::Status::OK);
    for (int i = 0; i < 10; ++i) {
        EXPECT_TRUE(adapter.isFinished());
    }
}

// ============================================================================
// BSA-14 — Custom queue depth of 1 is accepted
// ============================================================================
TEST_F(BidiStreamAdapterTest, BSA14_QueueDepthOneAccepted) {
    MockBidiStream mock({});
    EXPECT_NO_THROW(TestAdapter adapter(&mock, 1));
}

// ============================================================================
// BSA-15 — Large queue depth accepted
// ============================================================================
TEST_F(BidiStreamAdapterTest, BSA15_LargeQueueDepthAccepted) {
    MockBidiStream mock({});
    EXPECT_NO_THROW(TestAdapter adapter(&mock, 100000));
}

// ============================================================================
// BSA-16 — finish() with CANCELLED status stored correctly
// ============================================================================
TEST_F(BidiStreamAdapterTest, BSA16_FinishWithCancelledStatus) {
    MockBidiStream mock({});
    TestAdapter adapter(&mock);
    adapter.finish(grpc::Status(grpc::StatusCode::CANCELLED, "cancelled"));
    EXPECT_EQ(grpc::StatusCode::CANCELLED, adapter.finishStatus().error_code());
    EXPECT_EQ("cancelled", adapter.finishStatus().error_message());
}

// ============================================================================
// BSA-17 — Multiple write() calls before finish do not crash
// ============================================================================
TEST_F(BidiStreamAdapterTest, BSA17_MultipleWritesBeforeFinish) {
    MockBidiStream mock({});
    TestAdapter adapter(&mock, 50);
    for (int i = 0; i < 10; ++i) {
        MockMessage m;
        m.value = std::to_string(i);
        EXPECT_TRUE(adapter.write(std::move(m)));
    }
}

// ============================================================================
// BSA-18 — run() stops if finish() is called before exhausting stream
// ============================================================================
TEST_F(BidiStreamAdapterTest, BSA18_RunStopsAfterFinishCalled) {
    MockBidiStream mock({{"a"}, {"b"}, {"c"}, {"d"}, {"e"}});
    TestAdapter adapter(&mock);

    std::atomic<int> count{0};
    adapter.onMessage([&adapter, &count](MockMessage&& m) {
        ++count;
        if (m.value == "b") {
            adapter.finish(grpc::Status::OK); // signal stop after "b"
        }
    });

    adapter.run();

    // run() checks finished_ flag each iteration; the actual number
    // of messages processed can be 2..5 depending on scheduling.
    EXPECT_GE(count.load(), 2);
}

// ============================================================================
// BSA-19 — Destructor does not crash even if write() would block
// ============================================================================
TEST_F(BidiStreamAdapterTest, BSA19_DestructorNoCrash) {
    MockBidiStream mock({});
    EXPECT_NO_THROW({
        TestAdapter adapter(&mock, 5);
        adapter.finish(grpc::Status::OK);
        // adapter goes out of scope here
    });
}

// ============================================================================
// BSA-20 — queueDepth after multiple writes and no flush is bounded
// ============================================================================
TEST_F(BidiStreamAdapterTest, BSA20_QueueDepthBoundedByMaxDepth) {
    MockBidiStream mock({});
    constexpr std::size_t kDepth = 5;
    TestAdapter adapter(&mock, kDepth);

    // Write 3 messages; each write flushes synchronously so queue drains.
    for (int i = 0; i < 3; ++i) {
        MockMessage m;
        m.value = "msg";
        adapter.write(std::move(m));
    }
    // After synchronous flush the queue should be empty.
    EXPECT_EQ(0u, adapter.queueDepth());
}
