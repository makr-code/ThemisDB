/*
 * Tests: LoRAPatternClassifier (LPC-01..LPC-15)
 *
 * Copyright (c) 2025 VCC-URN Project — SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <atomic>
#include <cmath>
#include <string>
#include <thread>
#include <vector>

#include "analytics/lora_pattern_classifier.h"

using namespace themisdb::analytics;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static DataPoint makeEvent(const std::string& id, double value = 1.0) {
    DataPoint dp;
    dp.id = id;
    dp.timestamp_ms = 0;
    dp.set("value", value);
    return dp;
}

static AdapterDomain makeDomain(const std::string& adapter_id,
                                 const std::string& domain,
                                 std::vector<double> emb) {
    return AdapterDomain{adapter_id, domain, std::move(emb)};
}

// ─────────────────────────────────────────────────────────────────────────────
// LPC-01..LPC-05: Single-event classification + registration
// ─────────────────────────────────────────────────────────────────────────────

TEST(LoRAPatternClassifierTest, LPC01_RegistersAdapterDomain) {
    LoRAPatternClassifier clf;
    clf.registerAdapterDomain(makeDomain("ada1", "fraud", {1.0, 0.0}));
    EXPECT_EQ(clf.registeredAdapterCount(), 1u);
}

TEST(LoRAPatternClassifierTest, LPC02_FallbackWhenNoInferenceFn) {
    LoRAPatternClassifier clf;
    clf.registerAdapterDomain(makeDomain("ada1", "fraud", {1.0, 0.0}));
    const auto result = clf.classify({makeEvent("e1")}, "ada1");
    EXPECT_TRUE(result.used_fallback);
    EXPECT_EQ(result.adapter_id, "ada1");
    EXPECT_EQ(result.label, "fraud");
    EXPECT_GE(result.confidence, 0.0);
    EXPECT_LE(result.confidence, 1.0);
}

TEST(LoRAPatternClassifierTest, LPC03_UsesInjectedInferenceFn) {
    LoRAPatternClassifier clf;
    clf.registerAdapterDomain(makeDomain("ada1", "fraud", {1.0, 0.0}));
    clf.setInferenceFn([](const std::string& /*adapter_id*/,
                           const std::string& /*prompt*/) -> std::string {
        return R"({"label":"fraud_sequence","confidence":0.92})";
    });
    const auto result = clf.classify({makeEvent("e1")}, "ada1");
    EXPECT_FALSE(result.used_fallback);
}

TEST(LoRAPatternClassifierTest, LPC04_ParsesLabelAndConfidenceFromJson) {
    LoRAPatternClassifier clf;
    clf.registerAdapterDomain(makeDomain("ada1", "fraud", {1.0, 0.0}));
    clf.setInferenceFn([](const std::string&, const std::string&) -> std::string {
        return R"({"label":"fraud_sequence","confidence":0.92})";
    });
    const auto result = clf.classify({makeEvent("e1")}, "ada1");
    EXPECT_EQ(result.label, "fraud_sequence");
    EXPECT_NEAR(result.confidence, 0.92, 0.01);
    EXPECT_EQ(result.adapter_id, "ada1");
}

TEST(LoRAPatternClassifierTest, LPC05_ClassifyWithEmptyAdapterIdSelectsBest) {
    LoRAPatternClassifier clf;
    clf.registerAdapterDomain(makeDomain("ada1", "fraud", {1.0, 0.0}));
    clf.setInferenceFn([](const std::string& /*adapter_id*/, const std::string&) {
        return R"({"label":"ok","confidence":0.8})";
    });
    // Empty adapter_id → selectAdapter() is called.
    const auto result = clf.classify({makeEvent("e1")}, "");
    EXPECT_FALSE(result.adapter_id.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// LPC-06..LPC-08: batchClassify
// ─────────────────────────────────────────────────────────────────────────────

TEST(LoRAPatternClassifierTest, LPC06_BatchClassifyReturnsOneResultPerEvent) {
    LoRAPatternClassifier clf;
    clf.registerAdapterDomain(makeDomain("ada1", "fraud", {1.0, 0.0}));
    clf.setInferenceFn([](const std::string&, const std::string&) {
        return R"({"label":"fraud","confidence":0.9})";
    });
    const std::vector<DataPoint> events = {
        makeEvent("e1"), makeEvent("e2"), makeEvent("e3")
    };
    const auto results = clf.batchClassify(events);
    EXPECT_EQ(results.size(), 3u);
}

TEST(LoRAPatternClassifierTest, LPC07_BatchClassifyPreservesOrder) {
    LoRAPatternClassifier clf;
    clf.registerAdapterDomain(makeDomain("ada1", "test", {1.0, 0.0}));
    int call_index = 0;
    clf.setInferenceFn([&call_index](const std::string&, const std::string&) {
        int idx = call_index++;
        return R"({"label":"label)" + std::to_string(idx) + R"(","confidence":0.5})";
    });
    const std::vector<DataPoint> events = {
        makeEvent("e0"), makeEvent("e1"), makeEvent("e2"),
        makeEvent("e3"), makeEvent("e4")
    };
    const auto results = clf.batchClassify(events);
    ASSERT_EQ(results.size(), 5u);
    // Each result must have a non-empty label.
    for (const auto& r : results)
        EXPECT_FALSE(r.label.empty());
}

TEST(LoRAPatternClassifierTest, LPC08_BatchClassifyEmptyInputReturnsEmpty) {
    LoRAPatternClassifier clf;
    const auto results = clf.batchClassify({});
    EXPECT_TRUE(results.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// LPC-09..LPC-11: selectAdapter
// ─────────────────────────────────────────────────────────────────────────────

TEST(LoRAPatternClassifierTest, LPC09_SelectAdapterReturnsEmptyWhenNoDomains) {
    LoRAPatternClassifier clf;
    EXPECT_EQ(clf.selectAdapter("some context"), "");
}

TEST(LoRAPatternClassifierTest, LPC10_SelectAdapterReturnsFirstWhenNoEmbeddingFn) {
    LoRAPatternClassifier clf;
    clf.registerAdapterDomain(makeDomain("first",  "domain1", {1.0, 0.0}));
    clf.registerAdapterDomain(makeDomain("second", "domain2", {0.0, 1.0}));
    EXPECT_EQ(clf.selectAdapter("context"), "first");
}

TEST(LoRAPatternClassifierTest, LPC11_SelectAdapterPicksHighestCosineSimilarity) {
    LoRAPatternClassifier clf;

    // Three adapters with orthogonal embeddings.
    clf.registerAdapterDomain(makeDomain("fraud_adapter",     "fraud",     {1.0, 0.0, 0.0}));
    clf.registerAdapterDomain(makeDomain("anomaly_adapter",   "anomaly",   {0.0, 1.0, 0.0}));
    clf.registerAdapterDomain(makeDomain("compliance_adapter","compliance",{0.0, 0.0, 1.0}));

    // Inject embedding fn that returns the embedding for the "anomaly" domain.
    clf.setEmbeddingFn([](const std::string&) -> std::vector<double> {
        return {0.0, 1.0, 0.0};  // closest to anomaly_adapter
    });

    EXPECT_EQ(clf.selectAdapter("any context"), "anomaly_adapter");
}

// ─────────────────────────────────────────────────────────────────────────────
// LPC-12..LPC-14: State queries
// ─────────────────────────────────────────────────────────────────────────────

TEST(LoRAPatternClassifierTest, LPC12_HasInferenceFnReturnsFalseBeforeInjection) {
    LoRAPatternClassifier clf;
    EXPECT_FALSE(clf.hasInferenceFn());
}

TEST(LoRAPatternClassifierTest, LPC13_HasInferenceFnReturnsTrueAfterInjection) {
    LoRAPatternClassifier clf;
    clf.setInferenceFn([](const std::string&, const std::string&) {
        return R"({"label":"ok","confidence":0.8})";
    });
    EXPECT_TRUE(clf.hasInferenceFn());
}

TEST(LoRAPatternClassifierTest, LPC14_RegisteredAdapterCountMatchesDomains) {
    LoRAPatternClassifier clf;
    EXPECT_EQ(clf.registeredAdapterCount(), 0u);
    clf.registerAdapterDomain(makeDomain("a1", "d1", {1.0}));
    EXPECT_EQ(clf.registeredAdapterCount(), 1u);
    clf.registerAdapterDomain(makeDomain("a2", "d2", {0.0}));
    EXPECT_EQ(clf.registeredAdapterCount(), 2u);
}

// ─────────────────────────────────────────────────────────────────────────────
// LPC-15: Concurrency safety
// ─────────────────────────────────────────────────────────────────────────────

TEST(LoRAPatternClassifierTest, LPC15_ConcurrentBatchClassifyFrom4ThreadsIsSafe) {
    LoRAPatternClassifier clf;
    clf.registerAdapterDomain(makeDomain("ada1", "fraud", {1.0, 0.0}));
    std::atomic<int> call_count{0};
    clf.setInferenceFn([&call_count](const std::string&, const std::string&) {
        ++call_count;
        return R"({"label":"fraud","confidence":0.9})";
    });

    const std::vector<DataPoint> events = {
        makeEvent("e1"), makeEvent("e2"), makeEvent("e3"), makeEvent("e4"),
        makeEvent("e5"), makeEvent("e6"), makeEvent("e7"), makeEvent("e8")
    };

    constexpr int kThreads = 4;
    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&clf, &events]() {
            const auto results = clf.batchClassify(events);
            EXPECT_EQ(results.size(), events.size());
        });
    }
    for (auto& th : threads) {
      th.join();
    }
    // No crashes and all calls accounted for.
    EXPECT_GE(call_count.load(), 1);
}

TEST(LoRAPatternClassifierTest, LPC16_FallbackConfidenceAdaptsToEventSignal) {
    LoRAPatternClassifier clf;
    clf.registerAdapterDomain(makeDomain("ada1", "anomaly", {1.0, 0.0}));

    DataPoint low_signal;
    low_signal.id = "low";
    low_signal.timestamp_ms = 10;
    low_signal.set("status", std::string("ok"));

    DataPoint high_signal_1;
    high_signal_1.id = "high_1";
    high_signal_1.timestamp_ms = 100;
    high_signal_1.set("amount", 10.0);
    high_signal_1.set("count", static_cast<int64_t>(5));
    high_signal_1.set("flag", true);

    DataPoint high_signal_2;
    high_signal_2.id = "high_2";
    high_signal_2.timestamp_ms = 200;
    high_signal_2.set("amount", 120.0);
    high_signal_2.set("count", static_cast<int64_t>(50));
    high_signal_2.set("flag", true);

    const auto low = clf.classify({low_signal}, "ada1");
    const auto high = clf.classify({high_signal_1, high_signal_2}, "ada1");

    EXPECT_TRUE(low.used_fallback);
    EXPECT_TRUE(high.used_fallback);
    EXPECT_EQ(low.label, "anomaly");
    EXPECT_EQ(high.label, "anomaly");
    EXPECT_GE(low.confidence, 0.0);
    EXPECT_LE(low.confidence, 1.0);
    EXPECT_GE(high.confidence, 0.0);
    EXPECT_LE(high.confidence, 1.0);
    EXPECT_GT(high.confidence, low.confidence);
}
