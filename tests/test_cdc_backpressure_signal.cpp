/**
 * Test: CDC Backpressure Signal
 * Tests for ICDCBackpressureSignal / InMemoryBackpressureSignal:
 *   - Initial level is None
 *   - signalBackpressure() updates the current level
 *   - clearBackpressure() resets to None
 *   - Level-change callback is invoked on transitions
 *   - Callback not invoked when level does not change
 *   - setCallback() replaces previous callback
 *   - Thread-safety under concurrent signal calls
 *   - Polymorphic usage via ICDCBackpressureSignal*
 *
 * Copyright (c) 2025 ThemisDB Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "cdc/icdc_backpressure_signal.h"

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

using namespace themis::cdc;

// ─────────────────────────────────────────────────────────────────────────────
// Initial state
// ─────────────────────────────────────────────────────────────────────────────

TEST(InMemoryBackpressureSignalTest, InitialLevelIsNone) {
    InMemoryBackpressureSignal sig;
    EXPECT_EQ(sig.currentLevel(), BackpressureLevel::None);
}

// ─────────────────────────────────────────────────────────────────────────────
// signalBackpressure / clearBackpressure
// ─────────────────────────────────────────────────────────────────────────────

TEST(InMemoryBackpressureSignalTest, SignalLow) {
    InMemoryBackpressureSignal sig;
    sig.signalBackpressure(BackpressureLevel::Low);
    EXPECT_EQ(sig.currentLevel(), BackpressureLevel::Low);
}

TEST(InMemoryBackpressureSignalTest, SignalMedium) {
    InMemoryBackpressureSignal sig;
    sig.signalBackpressure(BackpressureLevel::Medium);
    EXPECT_EQ(sig.currentLevel(), BackpressureLevel::Medium);
}

TEST(InMemoryBackpressureSignalTest, SignalHigh) {
    InMemoryBackpressureSignal sig;
    sig.signalBackpressure(BackpressureLevel::High);
    EXPECT_EQ(sig.currentLevel(), BackpressureLevel::High);
}

TEST(InMemoryBackpressureSignalTest, SignalCritical) {
    InMemoryBackpressureSignal sig;
    sig.signalBackpressure(BackpressureLevel::Critical);
    EXPECT_EQ(sig.currentLevel(), BackpressureLevel::Critical);
}

TEST(InMemoryBackpressureSignalTest, ClearBackpressureResetsToNone) {
    InMemoryBackpressureSignal sig;
    sig.signalBackpressure(BackpressureLevel::High);
    sig.clearBackpressure();
    EXPECT_EQ(sig.currentLevel(), BackpressureLevel::None);
}

TEST(InMemoryBackpressureSignalTest, ReduceLevelOnRecovery) {
    InMemoryBackpressureSignal sig;
    sig.signalBackpressure(BackpressureLevel::Critical);
    sig.signalBackpressure(BackpressureLevel::Medium);
    EXPECT_EQ(sig.currentLevel(), BackpressureLevel::Medium);
}

// ─────────────────────────────────────────────────────────────────────────────
// Level-change callback
// ─────────────────────────────────────────────────────────────────────────────

TEST(InMemoryBackpressureSignalTest, CallbackInvokedOnChange) {
    std::atomic<int>    callback_count{0};
    BackpressureLevel   last_level{BackpressureLevel::None};

    InMemoryBackpressureSignal sig([&](BackpressureLevel lvl) {
        last_level = lvl;
        ++callback_count;
    });

    sig.signalBackpressure(BackpressureLevel::Low);
    EXPECT_EQ(callback_count.load(), 1);
    EXPECT_EQ(last_level, BackpressureLevel::Low);

    sig.signalBackpressure(BackpressureLevel::Critical);
    EXPECT_EQ(callback_count.load(), 2);
    EXPECT_EQ(last_level, BackpressureLevel::Critical);
}

TEST(InMemoryBackpressureSignalTest, CallbackNotInvokedWhenLevelUnchanged) {
    std::atomic<int> callback_count{0};

    InMemoryBackpressureSignal sig([&](BackpressureLevel) { ++callback_count; });

    sig.signalBackpressure(BackpressureLevel::High);
    EXPECT_EQ(callback_count.load(), 1);

    sig.signalBackpressure(BackpressureLevel::High); // same level — no callback
    EXPECT_EQ(callback_count.load(), 1);
}

TEST(InMemoryBackpressureSignalTest, ClearCallbackTriggeredWhenWasNonNone) {
    std::atomic<int> callback_count{0};
    InMemoryBackpressureSignal sig([&](BackpressureLevel) { ++callback_count; });

    sig.signalBackpressure(BackpressureLevel::Medium);
    EXPECT_EQ(callback_count.load(), 1);

    sig.clearBackpressure();
    EXPECT_EQ(callback_count.load(), 2); // None != Medium → callback fired
}

// ─────────────────────────────────────────────────────────────────────────────
// setCallback
// ─────────────────────────────────────────────────────────────────────────────

TEST(InMemoryBackpressureSignalTest, SetCallbackReplacesPrevious) {
    std::atomic<int> first_count{0};
    std::atomic<int> second_count{0};

    InMemoryBackpressureSignal sig([&](BackpressureLevel) { ++first_count; });

    sig.signalBackpressure(BackpressureLevel::Low);
    EXPECT_EQ(first_count.load(), 1);

    sig.setCallback([&](BackpressureLevel) { ++second_count; });
    sig.signalBackpressure(BackpressureLevel::High);
    EXPECT_EQ(first_count.load(), 1);  // old callback not called
    EXPECT_EQ(second_count.load(), 1); // new callback called
}

TEST(InMemoryBackpressureSignalTest, SetCallbackToNull) {
    std::atomic<int> count{0};
    InMemoryBackpressureSignal sig([&](BackpressureLevel) { ++count; });

    sig.setCallback(nullptr); // clear callback
    sig.signalBackpressure(BackpressureLevel::Critical);
    EXPECT_EQ(count.load(), 0); // no callback fired
    EXPECT_EQ(sig.currentLevel(), BackpressureLevel::Critical);
}

// ─────────────────────────────────────────────────────────────────────────────
// Thread-safety
// ─────────────────────────────────────────────────────────────────────────────

TEST(InMemoryBackpressureSignalTest, ConcurrentSignals) {
    InMemoryBackpressureSignal sig;
    constexpr int kIter    = 500;
    constexpr int kThreads = 4;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);

    const BackpressureLevel levels[] = {
        BackpressureLevel::None,
        BackpressureLevel::Low,
        BackpressureLevel::Medium,
        BackpressureLevel::High,
        BackpressureLevel::Critical,
    };

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t] {
            for (int i = 0; i < kIter; ++i) {
                sig.signalBackpressure(levels[(t + i) % 5]);
            }
        });
    }

    for (auto& th : threads) th.join();

    // Final level must be one of the valid enum values (no data race corruption)
    const auto lvl = sig.currentLevel();
    EXPECT_TRUE(
        lvl == BackpressureLevel::None    ||
        lvl == BackpressureLevel::Low     ||
        lvl == BackpressureLevel::Medium  ||
        lvl == BackpressureLevel::High    ||
        lvl == BackpressureLevel::Critical);
}

// ─────────────────────────────────────────────────────────────────────────────
// Polymorphic usage
// ─────────────────────────────────────────────────────────────────────────────

TEST(InMemoryBackpressureSignalTest, PolymorphicUsage) {
    std::unique_ptr<ICDCBackpressureSignal> sig =
        std::make_unique<InMemoryBackpressureSignal>();

    EXPECT_EQ(sig->currentLevel(), BackpressureLevel::None);
    sig->signalBackpressure(BackpressureLevel::High);
    EXPECT_EQ(sig->currentLevel(), BackpressureLevel::High);
    sig->clearBackpressure();
    EXPECT_EQ(sig->currentLevel(), BackpressureLevel::None);
}
