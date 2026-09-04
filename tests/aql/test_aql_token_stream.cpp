/**
 * @file test_aql_token_stream.cpp
 * @brief Unit tests for AQLTokenStream – thread-safe generic token streaming.
 */

#include <gtest/gtest.h>
#include "aql/aql_token_stream.h"
#include <chrono>
#include <string>
#include <thread>
#include <vector>

using namespace themis::aql;

// ============================================================================
// Basic single-threaded tests
// ============================================================================

TEST(AQLTokenStreamTest, EmptyStreamReturnsNulloptImmediately) {
    AQLTokenStream stream;
    stream.close();
    auto token = stream.nextToken();
    EXPECT_FALSE(token.has_value());
}

TEST(AQLTokenStreamTest, PushAndDrainSingleToken) {
    AQLTokenStream stream;
    stream.push("hello");
    stream.close();

    auto t1 = stream.nextToken();
    ASSERT_TRUE(t1.has_value());
    EXPECT_EQ(*t1, "hello");

    auto t2 = stream.nextToken();
    EXPECT_FALSE(t2.has_value());  // end of stream
}

TEST(AQLTokenStreamTest, PushMultipleTokensPreservesOrder) {
    AQLTokenStream stream;
    std::vector<std::string> tokens = {"tok1", "tok2", "tok3", "tok4"};
    for (const auto& t : tokens) {
      stream.push(t);
    }
    stream.close();

    for (const auto& expected : tokens) {
        auto got = stream.nextToken();
        ASSERT_TRUE(got.has_value());
        EXPECT_EQ(*got, expected);
    }
    EXPECT_FALSE(stream.nextToken().has_value());
}

TEST(AQLTokenStreamTest, PushEmptyStringIsAllowed) {
    AQLTokenStream stream;
    stream.push("");
    stream.close();

    auto token = stream.nextToken();
    ASSERT_TRUE(token.has_value());
    EXPECT_EQ(*token, "");
}

TEST(AQLTokenStreamTest, CloseIsIdempotent) {
    AQLTokenStream stream;
    stream.push("a");
    stream.close();
    stream.close();  // should not throw or block

    auto t = stream.nextToken();
    ASSERT_TRUE(t.has_value());
    EXPECT_EQ(*t, "a");
}

// ============================================================================
// Cancellation tests
// ============================================================================

TEST(AQLTokenStreamTest, CancelReturnsFalseFromNextToken) {
    AQLTokenStream stream;
    stream.cancel();
    auto token = stream.nextToken();
    EXPECT_FALSE(token.has_value());
}

TEST(AQLTokenStreamTest, CancelIsIdempotent) {
    AQLTokenStream stream;
    stream.cancel();
    stream.cancel();  // should not throw
    EXPECT_TRUE(stream.isCancelled());
}

TEST(AQLTokenStreamTest, PushAfterCancelIsDiscarded) {
    AQLTokenStream stream;
    stream.cancel();
    stream.push("discarded");
    stream.close();
    auto token = stream.nextToken();
    EXPECT_FALSE(token.has_value());
}

TEST(AQLTokenStreamTest, CancelUnblocksWaitingConsumer) {
    auto stream = std::make_shared<AQLTokenStream>();

    std::optional<std::string> received;
    std::thread consumer([&] {
        received = stream->nextToken();
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    stream->cancel();  // should unblock the consumer
    consumer.join();

    EXPECT_FALSE(received.has_value());
}

// ============================================================================
// Producer/consumer concurrency tests
// ============================================================================

TEST(AQLTokenStreamTest, ProducerThreadPushesConsumerReceivesAll) {
    auto stream = std::make_shared<AQLTokenStream>();
    const int N = 100;

    std::thread producer([&] {
        for (int i = 0; i < N; ++i) {
            stream->push("token_" + std::to_string(i));
        }
        stream->close();
    });

    std::vector<std::string> received;
    while (auto t = stream->nextToken()) {
        received.push_back(*t);
    }
    producer.join();

    ASSERT_EQ(static_cast<int>(received.size()), N);
    for (int i = 0; i < N; ++i) {
        EXPECT_EQ(received[i], "token_" + std::to_string(i));
    }
}

TEST(AQLTokenStreamTest, RangeBasedForLoopDrainsStream) {
    auto stream = std::make_shared<AQLTokenStream>();

    std::thread producer([&] {
        for (int i = 0; i < 5; ++i) {
            stream->push(std::to_string(i));
        }
        stream->close();
    });

    std::vector<std::string> collected;
    for (const auto& token : *stream) {
        collected.push_back(token);
    }
    producer.join();

    ASSERT_EQ(collected.size(), std::size_t(5));
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(collected[i], std::to_string(i));
    }
}

TEST(AQLTokenStreamTest, CancelMidStreamStopsIteration) {
    auto stream = std::make_shared<AQLTokenStream>();

    std::thread producer([&] {
        for (int i = 0; i < 1000; ++i) {
            if (stream->isCancelled()) {
              break;
            }
            stream->push("t");
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        stream->close();
    });

    // Consumer reads a few tokens then cancels.
    int count = 0;
    while (auto t = stream->nextToken()) {
        ++count;
        if (count >= 3) {
            stream->cancel();
            break;
        }
    }

    // Drain any remaining tokens after cancel (nextToken returns nullopt).
    while (stream->nextToken().has_value()) { /* drain */ }
    producer.join();

    EXPECT_GE(count, 3);
    EXPECT_TRUE(stream->isCancelled());
}

// ============================================================================
// State query tests
// ============================================================================

TEST(AQLTokenStreamTest, IsClosedReflectsCloseCall) {
    AQLTokenStream stream;
    EXPECT_FALSE(stream.isClosed());
    stream.close();
    EXPECT_TRUE(stream.isClosed());
}

TEST(AQLTokenStreamTest, IsCancelledReflectsCancelCall) {
    AQLTokenStream stream;
    EXPECT_FALSE(stream.isCancelled());
    stream.cancel();
    EXPECT_TRUE(stream.isCancelled());
}
