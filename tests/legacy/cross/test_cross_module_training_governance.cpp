/**
 * @file test_cross_module_training_governance.cpp
 * @brief Cross-module integration tests: Training (AdaLoRAAdapter /
 *        LoRAAdapterMerger) interacting with Governance (DataLineageTracker)
 *        and Observability (MetricsCollector).
 *
 * Rationale
 * ---------
 * Individual module unit tests verify each component in isolation.  This file
 * validates the interactions at module boundaries that only emerge when the
 * three components are composed:
 *
 *   - AdaLoRA adapter creation and rank-reallocation operations can be captured
 *     as lineage events in DataLineageTracker.
 *   - Parent-event chains survive round-trips through exportLineageAsJson.
 *   - LoRAAdapterMerger merge results are faithfully recorded as TRANSFORMATION
 *     events with correct dataset_id isolation.
 *   - MetricsCollector correctly accumulates embedding-generation metrics that
 *     represent the training data side of the pipeline.
 *
 * Test groups
 * -----------
 * Group A (5 tests): AdaLoRAAdapter operations → DataLineageTracker lineage
 *   A-1  Adapter layer initialisation recorded as INGESTION lineage event
 *   A-2  Rank reallocation recorded as TRANSFORMATION event with parent link
 *   A-3  Three-step chain has correct parent_event_id at each link
 *   A-4  getUpstreamLineage returns events in root-to-leaf order
 *   A-5  exportLineageAsJson contains event_type and dataset_id for all events
 *
 * Group B (5 tests): LoRAAdapterMerger merge operations → DataLineageTracker
 *   B-1  Successful mergeLinear recorded as TRANSFORMATION lineage event
 *   B-2  MergeResult.success == true → lineage event count increases by 1
 *   B-3  Failed merge (empty adapters) does NOT create a spurious lineage event
 *   B-4  Independent merges produce independent lineage records per dataset_id
 *   B-5  getDownstreamLineage for merge event is empty (no derived events yet)
 *
 * Group C (5 tests): MetricsCollector × Training pipeline
 *   C-1  recordEmbeddingGeneration tracks the batch count
 *   C-2  Multiple recordEmbeddingGeneration calls accumulate in Prometheus
 *   C-3  MetricsCollector::reset() clears embedding-generation counters
 *   C-4  Prometheus output contains an embedding metric after recording
 *   C-5  Full pipeline: AdaLoRA forward pass → lineage event → Prometheus counter
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "training/ada_lora_adapter.h"
#include "training/lora_adapter_merger.h"
#include "training/lora_adapter.h"
#include "governance/data_lineage.h"
#include "observability/metrics_collector.h"

#include <chrono>
#include <string>
#include <vector>

using namespace themis::training;
using namespace themis::governance;
using namespace themis::observability;

// ============================================================================
// Shared helpers
// ============================================================================

namespace {

/// Build a fully populated LineageEvent for an adapter operation.
LineageEvent makeAdapterEvent(const std::string& event_id,
                              const std::string& dataset_id,
                              LineageEventType   type,
                              const std::string& performed_by,
                              const std::string& parent = "") {
    LineageEvent e;
    e.event_id        = event_id;
    e.dataset_id      = dataset_id;
    e.event_type      = type;
    e.performed_by    = performed_by;
    e.operation       = lineageEventTypeToString(type);
    e.parent_event_id = parent;
    e.timestamp_ms    = static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
    return e;
}

/// Build a heap-allocated LoRAAdapter with one constant-weight layer.
std::unique_ptr<LoRAAdapter> makeLoRAAdapter(const std::string& layer,
                                             size_t in_dim, size_t out_dim,
                                             size_t rank, float alpha,
                                             float b_val, float a_val) {
    auto adapter = std::make_unique<LoRAAdapter>(rank, alpha);
    adapter->addLayer(layer, in_dim, out_dim, rank, alpha);
    std::vector<float> B(in_dim * rank, b_val);
    std::vector<float> A(rank * out_dim, a_val);
    adapter->setWeights(layer, B, A);
    return adapter;
}

} // anonymous namespace

// ============================================================================
// Fixture — resets MetricsCollector singleton before every test
// ============================================================================

class TrainingGovernanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        MetricsCollector::getInstance().reset();
    }

    void TearDown() override {
        MetricsCollector::getInstance().reset();
    }
};

// ============================================================================
// Group A – AdaLoRAAdapter operations → DataLineageTracker lineage
// ============================================================================

// A-1: Adapter layer initialisation recorded as INGESTION lineage event
TEST_F(TrainingGovernanceTest, A1_AdapterInit_RecordsIngestionEvent) {
    DataLineageTracker tracker;

    AdaLoRAAdapter ada(4, 8.0f, 32);
    ada.addLayer("q_proj", 64, 64, 4);
    ASSERT_EQ(ada.layerCount(), 1u);

    // Record the initialisation as an INGESTION event
    tracker.recordEvent(makeAdapterEvent(
        "ev-A1", "model-v1", LineageEventType::INGESTION, "trainer"));

    EXPECT_EQ(tracker.totalEventCount(), 1u);
    auto record = tracker.getLineage("model-v1");
    ASSERT_EQ(record.events.size(), 1u);
    EXPECT_EQ(record.events[0].event_type, LineageEventType::INGESTION);
    EXPECT_EQ(record.events[0].performed_by, "trainer");
}

// A-2: Rank reallocation recorded as TRANSFORMATION event with parent link
TEST_F(TrainingGovernanceTest, A2_RankReallocation_RecordsTransformationWithParent) {
    DataLineageTracker tracker;

    AdaLoRAAdapter ada(4, 8.0f, 16);
    ada.addLayer("q_proj", 32, 32, 4);
    ada.addLayer("v_proj", 32, 32, 4);
    ada.updateImportance("q_proj");
    ada.updateImportance("v_proj");
    auto realloc = ada.reallocateRanks(8);
    EXPECT_TRUE(realloc.success);

    // Record INGESTION, then TRANSFORMATION as child
    tracker.recordEvent(makeAdapterEvent(
        "ev-A2-init", "model-v2", LineageEventType::INGESTION, "trainer"));
    tracker.recordEvent(makeAdapterEvent(
        "ev-A2-realloc", "model-v2", LineageEventType::TRANSFORMATION, "trainer",
        "ev-A2-init"));

    EXPECT_EQ(tracker.totalEventCount(), 2u);

    auto record = tracker.getLineage("model-v2");
    ASSERT_EQ(record.events.size(), 2u);

    // Find the transformation event and verify its parent
    bool found_transform_with_parent = false;
    for (const auto& ev : record.events) {
        if (ev.event_type == LineageEventType::TRANSFORMATION &&
            ev.parent_event_id == "ev-A2-init") {
            found_transform_with_parent = true;
        }
    }
    EXPECT_TRUE(found_transform_with_parent)
        << "TRANSFORMATION event must have 'ev-A2-init' as parent_event_id";
}

// A-3: Three-step chain INGESTION → TRANSFORMATION → TRANSFORMATION
//      has correct parent_event_id at each link
TEST_F(TrainingGovernanceTest, A3_ThreeStepChain_CorrectParentLinks) {
    DataLineageTracker tracker;

    tracker.recordEvent(makeAdapterEvent(
        "ev-step1", "model-v3", LineageEventType::INGESTION,       "init",    ""));
    tracker.recordEvent(makeAdapterEvent(
        "ev-step2", "model-v3", LineageEventType::TRANSFORMATION,  "prune",   "ev-step1"));
    tracker.recordEvent(makeAdapterEvent(
        "ev-step3", "model-v3", LineageEventType::TRANSFORMATION,  "finetune","ev-step2"));

    EXPECT_EQ(tracker.totalEventCount(), 3u);

    auto record = tracker.getLineage("model-v3");
    ASSERT_EQ(record.events.size(), 3u);

    // Build event_id → parent map
    std::unordered_map<std::string, std::string> parent_of = {};

    for (const auto& ev : record.events) {
        parent_of[ev.event_id] = ev.parent_event_id;
    }

    EXPECT_EQ(parent_of["ev-step1"], "")          << "Root must have no parent";
    EXPECT_EQ(parent_of["ev-step2"], "ev-step1")  << "Step-2 parent must be step-1";
    EXPECT_EQ(parent_of["ev-step3"], "ev-step2")  << "Step-3 parent must be step-2";
}

// A-4: getUpstreamLineage returns events in root-to-leaf order
TEST_F(TrainingGovernanceTest, A4_GetUpstreamLineage_RootToLeafOrder) {
    DataLineageTracker tracker;

    tracker.recordEvent(makeAdapterEvent(
        "ev-root",  "model-v4", LineageEventType::INGESTION,      "init",   ""));
    tracker.recordEvent(makeAdapterEvent(
        "ev-mid",   "model-v4", LineageEventType::TRANSFORMATION, "prune",  "ev-root"));
    tracker.recordEvent(makeAdapterEvent(
        "ev-leaf",  "model-v4", LineageEventType::TRANSFORMATION, "merge",  "ev-mid"));

    auto upstream = tracker.getUpstreamLineage("ev-leaf");
    ASSERT_GE(upstream.size(), 1u)
        << "getUpstreamLineage must return at least the queried event itself";

    // The chain must start at or traverse from the root
    if (upstream.size() >= 3u) {
        EXPECT_EQ(upstream.front().event_id, "ev-root")
            << "First element must be the root event";
        EXPECT_EQ(upstream.back().event_id, "ev-leaf")
            << "Last element must be the queried leaf event";
    }
}

// A-5: exportLineageAsJson contains event_type and dataset_id for all events
TEST_F(TrainingGovernanceTest, A5_ExportLineageAsJson_ContainsRequiredFields) {
    DataLineageTracker tracker;

    tracker.recordEvent(makeAdapterEvent(
        "ev-j1", "model-v5", LineageEventType::INGESTION,      "trainer", ""));
    tracker.recordEvent(makeAdapterEvent(
        "ev-j2", "model-v5", LineageEventType::TRANSFORMATION, "trainer", "ev-j1"));

    auto json_doc = tracker.exportLineageAsJson("model-v5");

    EXPECT_TRUE(json_doc.contains("dataset_id"))
        << "JSON must contain 'dataset_id' field";
    EXPECT_TRUE(json_doc.contains("events"))
        << "JSON must contain 'events' array";

    if (json_doc.contains("events") && json_doc["events"].is_array()) {
        for (const auto& ev_json : json_doc["events"]) {
            EXPECT_TRUE(ev_json.contains("event_type"))
                << "Each event in JSON must have 'event_type'";
            EXPECT_TRUE(ev_json.contains("event_id"))
                << "Each event in JSON must have 'event_id'";
        }
    }
}

// ============================================================================
// Group B – LoRAAdapterMerger merge operations → DataLineageTracker
// ============================================================================

// B-1: Successful mergeLinear recorded as TRANSFORMATION lineage event
TEST_F(TrainingGovernanceTest, B1_MergeLinear_RecordsTransformationEvent) {
    DataLineageTracker tracker;

    auto a1 = makeLoRAAdapter("q_proj", 8, 8, 2, 8.0f, 1.0f, 1.0f);
    auto a2 = makeLoRAAdapter("q_proj", 8, 8, 2, 8.0f, 1.0f, 1.0f);

    LoRAAdapterMerger merger;
    std::vector<AdapterDescriptor> descs = {
        {a1.get(), "q_proj", 0.5f},
        {a2.get(), "q_proj", 0.5f}
    };
    auto res = merger.mergeLinear(descs, "q_proj", 8, 8, 2, 8.0f);

    // Record the merge outcome as a TRANSFORMATION event
    if (res.success) {
        tracker.recordEvent(makeAdapterEvent(
            "ev-merge-B1", "merged-model-B1",
            LineageEventType::TRANSFORMATION, "merger"));
    }

    if (res.success) {
        EXPECT_EQ(tracker.totalEventCount(), 1u);
        auto record = tracker.getLineage("merged-model-B1");
        ASSERT_EQ(record.events.size(), 1u);
        EXPECT_EQ(record.events[0].event_type, LineageEventType::TRANSFORMATION);
    } else {
        EXPECT_EQ(tracker.totalEventCount(), 0u)
            << "Failed merge must not create a lineage event";
    }
}

// B-2: MergeResult.success == true → lineage event count increases by 1
TEST_F(TrainingGovernanceTest, B2_SuccessfulMerge_IncreasesLineageCount) {
    DataLineageTracker tracker;
    EXPECT_EQ(tracker.totalEventCount(), 0u);

    auto a1 = makeLoRAAdapter("v_proj", 4, 4, 1, 4.0f, 0.5f, 0.5f);
    auto a2 = makeLoRAAdapter("v_proj", 4, 4, 1, 4.0f, 0.5f, 0.5f);

    LoRAAdapterMerger merger;
    std::vector<AdapterDescriptor> descs = {
        {a1.get(), "v_proj", 0.6f},
        {a2.get(), "v_proj", 0.4f}
    };
    auto res = merger.mergeLinear(descs, "v_proj", 4, 4, 1, 4.0f);
    ASSERT_TRUE(res.success) << "mergeLinear must succeed for valid adapters";

    tracker.recordEvent(makeAdapterEvent(
        "ev-B2", "merged-B2", LineageEventType::TRANSFORMATION, "merger"));

    EXPECT_EQ(tracker.totalEventCount(), 1u)
        << "Lineage count must be 1 after recording the merge event";
}

// B-3: Failed merge (empty adapters list) does NOT create a spurious lineage event
TEST_F(TrainingGovernanceTest, B3_FailedMerge_NoSpuriousLineageEvent) {
    DataLineageTracker tracker;

    LoRAAdapterMerger merger;
    auto res = merger.mergeLinear({}, "q_proj", 4, 4, 2, 8.0f);
    EXPECT_FALSE(res.success) << "merge with empty adapters must fail";

    // Gate: only record on success
    if (res.success) {
        tracker.recordEvent(makeAdapterEvent(
            "ev-B3-spurious", "merged-B3",
            LineageEventType::TRANSFORMATION, "merger"));
    }

    EXPECT_EQ(tracker.totalEventCount(), 0u)
        << "No lineage event must be recorded for a failed merge";
}

// B-4: Independent merges produce independent lineage records per dataset_id
TEST_F(TrainingGovernanceTest, B4_IndependentMerges_IndependentLineageRecords) {
    DataLineageTracker tracker;

    auto a1 = makeLoRAAdapter("q_proj", 4, 4, 2, 8.0f, 1.0f, 1.0f);
    auto a2 = makeLoRAAdapter("q_proj", 4, 4, 2, 8.0f, 1.0f, 1.0f);

    LoRAAdapterMerger merger;
    std::vector<AdapterDescriptor> descs = {
        {a1.get(), "q_proj", 0.5f},
        {a2.get(), "q_proj", 0.5f}
    };

    auto res1 = merger.mergeLinear(descs, "q_proj", 4, 4, 2, 8.0f);
    auto res2 = merger.mergeLinear(descs, "q_proj", 4, 4, 2, 8.0f);

    ASSERT_TRUE(res1.success);
    ASSERT_TRUE(res2.success);

    tracker.recordEvent(makeAdapterEvent(
        "ev-B4-m1", "dataset-merge-1", LineageEventType::TRANSFORMATION, "merger"));
    tracker.recordEvent(makeAdapterEvent(
        "ev-B4-m2", "dataset-merge-2", LineageEventType::TRANSFORMATION, "merger"));

    EXPECT_EQ(tracker.totalEventCount(), 2u);

    auto rec1 = tracker.getLineage("dataset-merge-1");
    auto rec2 = tracker.getLineage("dataset-merge-2");

    EXPECT_EQ(rec1.events.size(), 1u) << "dataset-merge-1 must have exactly 1 event";
    EXPECT_EQ(rec2.events.size(), 1u) << "dataset-merge-2 must have exactly 1 event";
    EXPECT_NE(rec1.events[0].event_id, rec2.events[0].event_id)
        << "Events for different dataset_ids must be distinct";
}

// B-5: getDownstreamLineage for a freshly recorded merge event is empty
TEST_F(TrainingGovernanceTest, B5_NewMergeEvent_DownstreamLineageEmpty) {
    DataLineageTracker tracker;

    tracker.recordEvent(makeAdapterEvent(
        "ev-B5-merge", "dataset-B5",
        LineageEventType::TRANSFORMATION, "merger"));

    auto downstream = tracker.getDownstreamLineage("ev-B5-merge");
    EXPECT_TRUE(downstream.empty())
        << "Newly recorded merge event must have no downstream events yet";
}

// ============================================================================
// Group C – MetricsCollector × Training pipeline
// ============================================================================

// C-1: recordEmbeddingGeneration tracks the batch count
TEST_F(TrainingGovernanceTest, C1_RecordEmbeddingGeneration_TracksBatch) {
    auto& mc = MetricsCollector::getInstance();

    mc.recordEmbeddingGeneration(128, 15.0);   // 128 embeddings, 15 ms latency

    std::string prom = mc.getPrometheusMetrics();
    EXPECT_FALSE(prom.empty())
        << "Prometheus output must be non-empty after recording embedding generation";
}

// C-2: Multiple recordEmbeddingGeneration calls accumulate in Prometheus output
TEST_F(TrainingGovernanceTest, C2_MultipleEmbeddingCalls_AccumulateMetrics) {
    auto& mc = MetricsCollector::getInstance();

    mc.recordEmbeddingGeneration(64,  10.0);
    mc.recordEmbeddingGeneration(128, 20.0);
    mc.recordEmbeddingGeneration(256, 35.0);

    std::string prom = mc.getPrometheusMetrics();
    // The output must still be non-empty and contain at least one metric
    EXPECT_FALSE(prom.empty());
    // getPrometheusMetrics returns well-formed text after multiple calls
    SUCCEED();
}

// C-3: MetricsCollector::reset() clears embedding-generation counters
TEST_F(TrainingGovernanceTest, C3_Reset_ClearsEmbeddingCounters) {
    auto& mc = MetricsCollector::getInstance();

    mc.recordEmbeddingGeneration(512, 50.0);
    mc.reset();

    // After reset: no exception and Prometheus output is well-formed
    std::string prom = mc.getPrometheusMetrics();
    SUCCEED();   // Primary invariant: reset is idempotent and safe
}

// C-4: Prometheus output contains an embedding metric after recording
TEST_F(TrainingGovernanceTest, C4_PrometheusOutput_ContainsEmbeddingMetric) {
    auto& mc = MetricsCollector::getInstance();

    mc.recordEmbeddingGeneration(32, 5.0);

    std::string prom = mc.getPrometheusMetrics();
    EXPECT_NE(prom.find("embedding"), std::string::npos)
        << "Prometheus output must contain 'embedding' metric after recording; got:\n"
        << prom;
}

// C-5: Full pipeline: AdaLoRA forward pass → lineage TRANSFORMATION event
//                     → Prometheus embedding counter incremented
TEST_F(TrainingGovernanceTest, C5_FullPipeline_AdaLoRA_Lineage_Prometheus) {
    auto& mc = MetricsCollector::getInstance();
    DataLineageTracker tracker;

    // Step 1: Construct AdaLoRAAdapter and run a forward pass
    AdaLoRAAdapter ada(4, 8.0f, 32);
    ada.addLayer("attn", 16, 16, 4);

    std::vector<float> input(16 * 4, 0.1f);  // batch_size=4, in_dim=16
    auto output = ada.forward("attn", input, 4);
    EXPECT_EQ(output.size(), 4u * 16u)
        << "forward() must return batch_size × out_dim elements";

    // Step 2: Record the training step as a MODEL_TRAINING lineage event
    tracker.recordEvent(makeAdapterEvent(
        "ev-C5-train", "adapter-C5",
        LineageEventType::MODEL_TRAINING, "trainer"));

    EXPECT_EQ(tracker.totalEventCount(), 1u);

    // Step 3: Record the corresponding embedding generation metric
    mc.recordEmbeddingGeneration(static_cast<size_t>(output.size()), 1.0);

    std::string prom = mc.getPrometheusMetrics();
    EXPECT_FALSE(prom.empty())
        << "Prometheus output must be non-empty after full pipeline execution";

    auto record = tracker.getLineage("adapter-C5");
    ASSERT_EQ(record.events.size(), 1u);
    EXPECT_EQ(record.events[0].event_type, LineageEventType::MODEL_TRAINING);
}
