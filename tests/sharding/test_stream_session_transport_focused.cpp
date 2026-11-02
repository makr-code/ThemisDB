#include <atomic>
#include <stdexcept>

#include <gtest/gtest.h>

#include "sharding/stream_protocol.h"

using namespace themisdb::streaming;

TEST(StreamSessionTransport, MissingCallbackFailsClosed) {
    StreamSessionConfig config;
    config.direction = StreamDirection::OUTGOING;
    config.remote_endpoint = "shard-b:7000";

    StreamSession session(config);

    EXPECT_FALSE(session.initialize());
    EXPECT_EQ(session.getState(), StreamSessionState::ABORTED);
    EXPECT_EQ(session.getLastError(),
              StreamSessionErrorCode::TRANSPORT_NOT_CONFIGURED);
}

TEST(StreamSessionTransport, ConfiguredPrepareCallbackAllowsInitialization) {
    StreamSessionConfig config;
    config.direction = StreamDirection::OUTGOING;
    config.remote_endpoint = "shard-b:7000";

    StreamSession session(config);
    std::atomic<int> callback_count{0};
    session.setPrepareTransferCallback([&callback_count] {
        callback_count.fetch_add(1, std::memory_order_relaxed);
        return true;
    });

    EXPECT_TRUE(session.initialize());
    EXPECT_EQ(callback_count.load(), 1);
    EXPECT_EQ(session.getState(), StreamSessionState::PREPARING);
    EXPECT_EQ(session.getLastError(), StreamSessionErrorCode::NONE);
}

TEST(StreamSessionTransport, RejectedPrepareFailsClosed) {
    StreamSessionConfig config;
    config.direction = StreamDirection::OUTGOING;
    config.remote_endpoint = "shard-b:7000";

    StreamSession session(config);
    session.setPrepareTransferCallback([] { return false; });

    EXPECT_FALSE(session.initialize());
    EXPECT_EQ(session.getState(), StreamSessionState::ABORTED);
    EXPECT_EQ(session.getLastError(), StreamSessionErrorCode::PREPARE_FAILED);
}

TEST(StreamSessionTransport, PrepareExceptionFailsClosed) {
    StreamSessionConfig config;
    config.direction = StreamDirection::OUTGOING;
    config.remote_endpoint = "shard-b:7000";

    StreamSession session(config);
    session.setPrepareTransferCallback([]() -> bool {
        throw std::runtime_error("transport unavailable");
    });

    EXPECT_FALSE(session.initialize());
    EXPECT_EQ(session.getState(), StreamSessionState::ABORTED);
    EXPECT_EQ(session.getLastError(), StreamSessionErrorCode::PREPARE_FAILED);
}
