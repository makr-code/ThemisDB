/**
 * Test: CDC Server-Side Filter Pipeline
 *
 * Tests for ICDCFilterPipeline / InMemoryFilterPipeline /
 * IEventFilter / PredicateFilter / KeyPrefixFilter / EventTypeFilter:
 *
 *   AC-FP1  Empty pipeline passes all events
 *   AC-FP2  addFilter() returns false for duplicate name
 *   AC-FP3  removeFilter() returns false for unknown name
 *   AC-FP4  hasFilter() correctly reports presence
 *   AC-FP5  size() and empty() reflect pipeline state
 *   AC-FP6  PredicateFilter passes / drops correctly
 *   AC-FP7  KeyPrefixFilter passes matching prefix; drops others
 *   AC-FP8  EventTypeFilter passes matching types; drops others
 *   AC-FP9  Multiple stages: first Drop short-circuits remaining stages
 *   AC-FP10 applyBatch() returns only passing events (order preserved)
 *   AC-FP11 filterNames() returns stage names in insertion order
 *   AC-FP12 totalPassed / totalDropped / resetCounters
 *   AC-FP13 Removing a stage affects subsequent apply() calls
 *   AC-FP14 Thread-safety: concurrent addFilter / apply
 *   AC-FP15 ICDCFilterPipeline polymorphic usage
 *
 * Copyright (c) 2025 ThemisDB Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "cdc/icdc_filter_pipeline.h"
#include "cdc/changefeed.h"

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace themis;
using namespace themis::cdc;

namespace {

Changefeed::ChangeEvent makeEv(
    uint64_t seq,
    const std::string& key = "col:k",
    Changefeed::ChangeEventType type = Changefeed::ChangeEventType::EVENT_PUT)
{
    Changefeed::ChangeEvent ev;
    ev.sequence     = seq;
    ev.type         = type;
    ev.key          = key + std::to_string(seq);
    ev.value        = "v";
    ev.timestamp_ms = static_cast<int64_t>(seq) * 1000;
    return ev;
}

} // namespace

// ── AC-FP1  Empty pipeline passes all events ──────────────────────────────────

TEST(InMemoryFilterPipelineTest, EmptyPipelinePassesAll) {
    InMemoryFilterPipeline pipeline;
    EXPECT_TRUE(pipeline.empty());
    auto ev = makeEv(1);
    EXPECT_EQ(pipeline.apply(ev), FilterResult::Pass);
}

// ── AC-FP2  addFilter() returns false for duplicate name ─────────────────────

TEST(InMemoryFilterPipelineTest, AddFilterReturnsFalseForDuplicate) {
    InMemoryFilterPipeline pipeline;
    auto f1 = std::make_unique<PredicateFilter>("f", [](const auto&) { return true; });
    auto f2 = std::make_unique<PredicateFilter>("f", [](const auto&) { return true; });
    EXPECT_TRUE(pipeline.addFilter(std::move(f1)));
    EXPECT_FALSE(pipeline.addFilter(std::move(f2)));
    EXPECT_EQ(pipeline.size(), 1u);
}

// ── AC-FP3  removeFilter() returns false for unknown name ────────────────────

TEST(InMemoryFilterPipelineTest, RemoveFilterReturnsFalseForUnknown) {
    InMemoryFilterPipeline pipeline;
    EXPECT_FALSE(pipeline.removeFilter("nonexistent"));
}

// ── AC-FP4  hasFilter() reports presence ─────────────────────────────────────

TEST(InMemoryFilterPipelineTest, HasFilterReportsPresence) {
    InMemoryFilterPipeline pipeline;
    EXPECT_FALSE(pipeline.hasFilter("f1"));
    pipeline.addFilter(
        std::make_unique<PredicateFilter>("f1", [](const auto&) { return true; }));
    EXPECT_TRUE(pipeline.hasFilter("f1"));
}

// ── AC-FP5  size() and empty() ────────────────────────────────────────────────

TEST(InMemoryFilterPipelineTest, SizeAndEmpty) {
    InMemoryFilterPipeline pipeline;
    EXPECT_EQ(pipeline.size(), 0u);
    EXPECT_TRUE(pipeline.empty());

    pipeline.addFilter(
        std::make_unique<PredicateFilter>("a", [](const auto&) { return true; }));
    EXPECT_EQ(pipeline.size(), 1u);
    EXPECT_FALSE(pipeline.empty());

    pipeline.addFilter(
        std::make_unique<PredicateFilter>("b", [](const auto&) { return true; }));
    EXPECT_EQ(pipeline.size(), 2u);

    pipeline.removeFilter("a");
    EXPECT_EQ(pipeline.size(), 1u);
}

// ── AC-FP6  PredicateFilter pass / drop ──────────────────────────────────────

TEST(InMemoryFilterPipelineTest, PredicateFilterPassDrop) {
    InMemoryFilterPipeline pipeline;
    // Drop events with sequence > 3
    pipeline.addFilter(std::make_unique<PredicateFilter>(
        "seq-cap", [](const Changefeed::ChangeEvent& ev) {
            return ev.sequence <= 3;
        }));

    EXPECT_EQ(pipeline.apply(makeEv(1)), FilterResult::Pass);
    EXPECT_EQ(pipeline.apply(makeEv(3)), FilterResult::Pass);
    EXPECT_EQ(pipeline.apply(makeEv(4)), FilterResult::Drop);
}

// ── AC-FP7  KeyPrefixFilter ───────────────────────────────────────────────────

TEST(InMemoryFilterPipelineTest, KeyPrefixFilterPassesMatchingPrefix) {
    InMemoryFilterPipeline pipeline;
    pipeline.addFilter(
        std::make_unique<KeyPrefixFilter>("prefix", "orders:"));

    EXPECT_EQ(pipeline.apply(makeEv(1, "orders:k")), FilterResult::Pass);
    EXPECT_EQ(pipeline.apply(makeEv(2, "products:k")), FilterResult::Drop);
}

TEST(InMemoryFilterPipelineTest, EmptyKeyPrefixPassesAll) {
    InMemoryFilterPipeline pipeline;
    pipeline.addFilter(std::make_unique<KeyPrefixFilter>("any-key", ""));
    EXPECT_EQ(pipeline.apply(makeEv(1, "anything:k")), FilterResult::Pass);
}

// ── AC-FP8  EventTypeFilter ───────────────────────────────────────────────────

TEST(InMemoryFilterPipelineTest, EventTypeFilterPassesMatchingTypes) {
    InMemoryFilterPipeline pipeline;
    pipeline.addFilter(std::make_unique<EventTypeFilter>(
        "deletes-only",
        std::vector<Changefeed::ChangeEventType>{
            Changefeed::ChangeEventType::EVENT_DELETE}));

    EXPECT_EQ(
        pipeline.apply(makeEv(1, "k:", Changefeed::ChangeEventType::EVENT_DELETE)),
        FilterResult::Pass);
    EXPECT_EQ(
        pipeline.apply(makeEv(2, "k:", Changefeed::ChangeEventType::EVENT_PUT)),
        FilterResult::Drop);
}

TEST(InMemoryFilterPipelineTest, EmptyEventTypeListPassesAll) {
    InMemoryFilterPipeline pipeline;
    pipeline.addFilter(std::make_unique<EventTypeFilter>(
        "all-types",
        std::vector<Changefeed::ChangeEventType>{}));
    EXPECT_EQ(
        pipeline.apply(makeEv(1, "k:", Changefeed::ChangeEventType::EVENT_DELETE)),
        FilterResult::Pass);
}

// ── AC-FP9  Multiple stages: first Drop short-circuits ───────────────────────

TEST(InMemoryFilterPipelineTest, FirstDropShortCircuitsRemainingStages) {
    std::atomic<int> second_stage_calls{0};

    InMemoryFilterPipeline pipeline;
    pipeline.addFilter(std::make_unique<PredicateFilter>(
        "always-drop", [](const auto&) { return false; }));
    pipeline.addFilter(std::make_unique<PredicateFilter>(
        "counting", [&](const auto&) {
            second_stage_calls.fetch_add(1);
            return true;
        }));

    auto ev = makeEv(1);
    EXPECT_EQ(pipeline.apply(ev), FilterResult::Drop);
    EXPECT_EQ(second_stage_calls.load(), 0);
}

// ── AC-FP10 applyBatch() ─────────────────────────────────────────────────────

TEST(InMemoryFilterPipelineTest, ApplyBatchReturnOnlyPassingEventsInOrder) {
    InMemoryFilterPipeline pipeline;
    pipeline.addFilter(std::make_unique<PredicateFilter>(
        "even-seq", [](const Changefeed::ChangeEvent& ev) {
            return ev.sequence % 2 == 0;
        }));

    std::vector<Changefeed::ChangeEvent> batch;
    for (uint64_t i = 1; i <= 6; ++i) batch.push_back(makeEv(i));

    auto result = pipeline.applyBatch(batch);
    ASSERT_EQ(result.size(), 3u);
    EXPECT_EQ(result[0].sequence, 2u);
    EXPECT_EQ(result[1].sequence, 4u);
    EXPECT_EQ(result[2].sequence, 6u);
}

// ── AC-FP11 filterNames() in insertion order ─────────────────────────────────

TEST(InMemoryFilterPipelineTest, FilterNamesInInsertionOrder) {
    InMemoryFilterPipeline pipeline;
    pipeline.addFilter(
        std::make_unique<PredicateFilter>("alpha", [](const auto&) { return true; }));
    pipeline.addFilter(
        std::make_unique<PredicateFilter>("beta",  [](const auto&) { return true; }));
    pipeline.addFilter(
        std::make_unique<PredicateFilter>("gamma", [](const auto&) { return true; }));

    auto names = pipeline.filterNames();
    ASSERT_EQ(names.size(), 3u);
    EXPECT_EQ(names[0], "alpha");
    EXPECT_EQ(names[1], "beta");
    EXPECT_EQ(names[2], "gamma");
}

// ── AC-FP12 totalPassed / totalDropped / resetCounters ───────────────────────

TEST(InMemoryFilterPipelineTest, PassDropCountersAndReset) {
    InMemoryFilterPipeline pipeline;
    pipeline.addFilter(std::make_unique<PredicateFilter>(
        "odd", [](const Changefeed::ChangeEvent& ev) {
            return ev.sequence % 2 != 0;
        }));

    for (uint64_t i = 1; i <= 6; ++i) pipeline.apply(makeEv(i));

    EXPECT_EQ(pipeline.totalPassed(),  3u); // 1, 3, 5
    EXPECT_EQ(pipeline.totalDropped(), 3u); // 2, 4, 6

    pipeline.resetCounters();
    EXPECT_EQ(pipeline.totalPassed(),  0u);
    EXPECT_EQ(pipeline.totalDropped(), 0u);
}

// ── AC-FP13 Removing a stage affects apply() ─────────────────────────────────

TEST(InMemoryFilterPipelineTest, RemoveStageAffectsApply) {
    InMemoryFilterPipeline pipeline;
    pipeline.addFilter(std::make_unique<PredicateFilter>(
        "block-all", [](const auto&) { return false; }));

    EXPECT_EQ(pipeline.apply(makeEv(1)), FilterResult::Drop);

    pipeline.removeFilter("block-all");
    EXPECT_EQ(pipeline.apply(makeEv(1)), FilterResult::Pass);
}

// ── AC-FP14 Thread-safety: concurrent addFilter / apply ──────────────────────

TEST(InMemoryFilterPipelineTest, ConcurrentAddFilterAndApplyAreThreadSafe) {
    InMemoryFilterPipeline pipeline;
    constexpr int kThreads = 4;
    constexpr int kOps     = 200;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t] {
            for (int i = 0; i < kOps; ++i) {
                const std::string name = "f" + std::to_string(t) + "_" + std::to_string(i);
                pipeline.addFilter(std::make_unique<PredicateFilter>(
                    name, [](const auto&) { return true; }));
                pipeline.apply(makeEv(static_cast<uint64_t>(i + 1)));
            }
        });
    }
    for (auto& th : threads) th.join();
    // No crash; pipeline is consistent
    EXPECT_LE(pipeline.size(), static_cast<std::size_t>(kThreads * kOps));
}

// ── AC-FP15 ICDCFilterPipeline polymorphic usage ─────────────────────────────

TEST(InMemoryFilterPipelineTest, PolymorphicUsage) {
    std::unique_ptr<ICDCFilterPipeline> pl =
        std::make_unique<InMemoryFilterPipeline>();

    pl->addFilter(std::make_unique<KeyPrefixFilter>("pfx", "user:"));

    auto pass = makeEv(1, "user:k");
    auto drop = makeEv(2, "order:k");

    EXPECT_EQ(pl->apply(pass), FilterResult::Pass);
    EXPECT_EQ(pl->apply(drop), FilterResult::Drop);
    EXPECT_EQ(pl->totalPassed(),  1u);
    EXPECT_EQ(pl->totalDropped(), 1u);
}
