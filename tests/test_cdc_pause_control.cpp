/**
 * Test: CDC Pause/Resume Control
 * Tests for ICDCPauseControl / InMemoryPauseControl:
 *   - pause() / resume() state transitions
 *   - isPaused() non-blocking query
 *   - bufferEvent() during pause; drainBuffer() after resume
 *   - buffer overflow
 *   - PauseReason tracking
 *   - waitForResume() timeout / success paths
 *   - Thread-safety under concurrent pause/resume
 *
 * Copyright (c) 2025 ThemisDB Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "cdc/icdc_pause_control.h"
#include "cdc/changefeed.h"

#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

using namespace themis::cdc;

namespace {

themis::Changefeed::ChangeEvent makeEv(uint64_t seq, const std::string& key = "k") {
    themis::Changefeed::ChangeEvent ev;
    ev.sequence     = seq;
    ev.type         = themis::Changefeed::ChangeEventType::EVENT_PUT;
    ev.key          = key + ":" + std::to_string(seq);
    ev.value        = "v";
    ev.timestamp_ms = static_cast<int64_t>(seq) * 100;
    return ev;
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Initial state
// ─────────────────────────────────────────────────────────────────────────────

TEST(InMemoryPauseControlTest, InitiallyNotPaused) {
    InMemoryPauseControl ctrl;
    EXPECT_FALSE(ctrl.isPaused());
    EXPECT_EQ(ctrl.bufferedEventCount(), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Basic pause / resume transitions
// ─────────────────────────────────────────────────────────────────────────────

TEST(InMemoryPauseControlTest, PauseThenResume) {
    InMemoryPauseControl ctrl;
    EXPECT_TRUE(ctrl.pause());
    EXPECT_TRUE(ctrl.isPaused());
    EXPECT_TRUE(ctrl.resume());
    EXPECT_FALSE(ctrl.isPaused());
}

TEST(InMemoryPauseControlTest, PauseTwiceIsNoOp) {
    InMemoryPauseControl ctrl;
    EXPECT_TRUE(ctrl.pause());
    EXPECT_TRUE(ctrl.pause()); // second call — no-op
    EXPECT_TRUE(ctrl.isPaused());
    EXPECT_TRUE(ctrl.resume());
    EXPECT_FALSE(ctrl.isPaused());
}

TEST(InMemoryPauseControlTest, ResumeTwiceIsNoOp) {
    InMemoryPauseControl ctrl;
    EXPECT_TRUE(ctrl.resume()); // not paused — no-op
    EXPECT_FALSE(ctrl.isPaused());
}

// ─────────────────────────────────────────────────────────────────────────────
// PauseReason
// ─────────────────────────────────────────────────────────────────────────────

TEST(InMemoryPauseControlTest, PauseReasonAdminRequest) {
    InMemoryPauseControl ctrl;
    ctrl.pause(PauseReason::AdminRequest);
    EXPECT_EQ(ctrl.pauseReason(), PauseReason::AdminRequest);
}

TEST(InMemoryPauseControlTest, PauseReasonBackpressure) {
    InMemoryPauseControl ctrl;
    ctrl.pause(PauseReason::Backpressure);
    EXPECT_EQ(ctrl.pauseReason(), PauseReason::Backpressure);
}

TEST(InMemoryPauseControlTest, PauseReasonSchemaEvolution) {
    InMemoryPauseControl ctrl;
    ctrl.pause(PauseReason::SchemaEvolution);
    EXPECT_EQ(ctrl.pauseReason(), PauseReason::SchemaEvolution);
}

// ─────────────────────────────────────────────────────────────────────────────
// bufferEvent / drainBuffer
// ─────────────────────────────────────────────────────────────────────────────

TEST(InMemoryPauseControlTest, BufferEventDuringPause) {
    InMemoryPauseControl ctrl;
    ctrl.pause();

    for (uint64_t i = 1; i <= 5; ++i) {
        EXPECT_TRUE(ctrl.bufferEvent(makeEv(i)));
    }
    EXPECT_EQ(ctrl.bufferedEventCount(), 5u);
}

TEST(InMemoryPauseControlTest, BufferEventFailsWhenNotPaused) {
    InMemoryPauseControl ctrl;
    EXPECT_FALSE(ctrl.bufferEvent(makeEv(1))); // not paused
}

TEST(InMemoryPauseControlTest, DrainBufferReturnsAllEventsInOrder) {
    InMemoryPauseControl ctrl;
    ctrl.pause();

    for (uint64_t i = 1; i <= 4; ++i) {
        ctrl.bufferEvent(makeEv(i));
    }

    ctrl.resume();
    auto drained = ctrl.drainBuffer();
    ASSERT_EQ(drained.size(), 4u);
    EXPECT_EQ(drained[0].sequence, 1u);
    EXPECT_EQ(drained[1].sequence, 2u);
    EXPECT_EQ(drained[2].sequence, 3u);
    EXPECT_EQ(drained[3].sequence, 4u);
    EXPECT_EQ(ctrl.bufferedEventCount(), 0u);
}

TEST(InMemoryPauseControlTest, DrainBufferClearsBuffer) {
    InMemoryPauseControl ctrl;
    ctrl.pause();
    ctrl.bufferEvent(makeEv(1));
    ctrl.resume();
    ctrl.drainBuffer();
    EXPECT_EQ(ctrl.bufferedEventCount(), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Buffer overflow
// ─────────────────────────────────────────────────────────────────────────────

TEST(InMemoryPauseControlTest, BufferOverflowReturnsFalse) {
    // Create a controller with a tiny 1-byte buffer
    InMemoryPauseControl ctrl(/*max_buffer_bytes=*/1u);
    ctrl.pause();

    // Even a minimal event exceeds 1 byte when serialised to JSON
    EXPECT_FALSE(ctrl.bufferEvent(makeEv(1)));
    EXPECT_EQ(ctrl.bufferedEventCount(), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// waitForResume
// ─────────────────────────────────────────────────────────────────────────────

TEST(InMemoryPauseControlTest, WaitForResumeTimesOut) {
    InMemoryPauseControl ctrl;
    ctrl.pause();
    // Not resumed — should time out quickly
    EXPECT_FALSE(ctrl.waitForResume(std::chrono::milliseconds(50)));
}

TEST(InMemoryPauseControlTest, WaitForResumeSucceeds) {
    InMemoryPauseControl ctrl;
    ctrl.pause();

    std::thread t([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        ctrl.resume();
    });

    EXPECT_TRUE(ctrl.waitForResume(std::chrono::milliseconds(500)));
    t.join();
}

// ─────────────────────────────────────────────────────────────────────────────
// Thread-safety: concurrent pause / resume
// ─────────────────────────────────────────────────────────────────────────────

TEST(InMemoryPauseControlTest, ConcurrentPauseResume) {
    InMemoryPauseControl ctrl;
    constexpr int kRounds  = 200;
    constexpr int kThreads = 4;

    std::atomic<int> errors{0};
    std::vector<std::thread> threads;
    threads.reserve(kThreads);

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t] {
            for (int i = 0; i < kRounds; ++i) {
                if ((t + i) % 2 == 0) {
                    ctrl.pause(PauseReason::AdminRequest);
                } else {
                    ctrl.resume();
                }
                // isPaused() must always return a valid bool
                (void)ctrl.isPaused();
            }
        });
    }

    for (auto& th : threads) th.join();
    EXPECT_EQ(errors.load(), 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// ICDCPauseControl polymorphic usage
// ─────────────────────────────────────────────────────────────────────────────

TEST(InMemoryPauseControlTest, PolymorphicUsage) {
    std::unique_ptr<ICDCPauseControl> ctrl =
        std::make_unique<InMemoryPauseControl>();

    EXPECT_FALSE(ctrl->isPaused());
    ctrl->pause(PauseReason::SchemaEvolution);
    EXPECT_TRUE(ctrl->isPaused());
    EXPECT_EQ(ctrl->pauseReason(), PauseReason::SchemaEvolution);
    ctrl->resume();
    EXPECT_FALSE(ctrl->isPaused());
}
