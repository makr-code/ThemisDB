/**
 * @file test_training_failsafe_edge_cases_focused.cpp
 * @brief Focused edge-case regressions for training fail-safe handlers (Phase 4).
 *
 * Test identifiers: TFE-01 through TFE-12
 *
 * Coverage:
 *  TFE-01  CheckpointFaultHandler — corruption fault with fallback → ROLLBACK
 *  TFE-02  CheckpointFaultHandler — corruption fault without fallback → ABORT
 *  TFE-03  CheckpointFaultHandler — transient I/O fault during save, no fallback → SKIP_SAVE
 *  TFE-04  CheckpointFaultHandler — disk exhaustion during save → SKIP_SAVE
 *  TFE-05  CheckpointFaultHandler — disk exhaustion during load with fallback → ROLLBACK
 *  TFE-06  CheckpointFaultHandler — incident emitter receives event for ABORT case
 *  TFE-07  AdapterMergeFailsafe   — merge fault with snapshot → ROLLBACK, snapshot restored
 *  TFE-08  AdapterMergeFailsafe   — merge fault without snapshot → ABORT_MERGE
 *  TFE-09  AdapterMergeFailsafe   — partial merge detected flag is true when layers > 0
 *  TFE-10  EnrichmentGapHandler   — coverage above threshold → no incident emitted
 *  TFE-11  EnrichmentGapHandler   — coverage below threshold → incident emitted, meets_threshold=false
 *  TFE-12  EnrichmentGapHandler   — total_items == 0 → coverage 1.0, no incident
 */

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include "training/training_failsafe.h"
#include "training/training_incident_emitter.h"

#include <gtest/gtest.h>

#include <memory>
#include <mutex>
#include <vector>

namespace themis {
namespace training {
namespace {

// ---------------------------------------------------------------------------
// Capture listener for emitter tests
// ---------------------------------------------------------------------------

struct CapturedIncident {
    TrainingIncidentClass cls;
    TrainingErrorCode     code;
    std::string           component;
    std::string           operation;
    std::string           message;
    bool                  recoverable;
    std::string           context;
};

class CapturingListener final : public TrainingIncidentListener {
public:
    void onIncident(const TrainingIncident& incident) override {
        std::lock_guard<std::mutex> lk(mu_);
        incidents_.push_back({
            incident.incident_class,
            incident.error_code,
            incident.component,
            incident.operation,
            incident.message,
            incident.is_recoverable,
            incident.context
        });
    }

    size_t count() const {
        std::lock_guard<std::mutex> lk(mu_);
        return incidents_.size();
    }

    CapturedIncident last() const {
        std::lock_guard<std::mutex> lk(mu_);
        return incidents_.back();
    }

    void clear() {
        std::lock_guard<std::mutex> lk(mu_);
        incidents_.clear();
    }

private:
    mutable std::mutex mu_;
    std::vector<CapturedIncident> incidents_;
};

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class TrainingFailsafeEdgeCasesTest : public ::testing::Test {
protected:
    void SetUp() override {
        listener_ = std::make_shared<CapturingListener>();
        emitter_  = std::make_unique<TrainingIncidentEmitter>();
        emitter_->addListener(listener_);
    }

    CheckpointFaultHandler    checkpoint_handler_;
    AdapterMergeFailsafe      merge_failsafe_;
    EnrichmentGapHandler      enrichment_handler_;

    std::shared_ptr<CapturingListener> listener_;
    std::unique_ptr<TrainingIncidentEmitter> emitter_;
};

// ---------------------------------------------------------------------------
// TFE-01: corruption fault with fallback → ROLLBACK
// ---------------------------------------------------------------------------

TEST_F(TrainingFailsafeEdgeCasesTest, TFE01_CorruptionWithFallback_Rollback) {
    CheckpointFaultContext ctx;
    ctx.fault_code       = TrainingErrorCode::CHECKPOINT_SHA256_MISMATCH;
    ctx.checkpoint_path  = "/checkpoints/step_100";
    ctx.has_fallback     = true;
    ctx.fallback_path    = "/checkpoints/step_050";

    auto result = checkpoint_handler_.handle(ctx, emitter_.get());

    EXPECT_EQ(result.strategy, CheckpointFaultResult::RecoveryStrategy::ROLLBACK)
        << "TFE-01: corruption with fallback must trigger ROLLBACK";
    EXPECT_EQ(result.recovery_path, "/checkpoints/step_050")
        << "TFE-01: recovery_path must match fallback_path";
    EXPECT_TRUE(result.incident_emitted) << "TFE-01: incident must be emitted";
    EXPECT_EQ(listener_->count(), 1u) << "TFE-01: exactly one incident emitted";
    EXPECT_TRUE(listener_->last().recoverable)
        << "TFE-01: ROLLBACK incident must be marked recoverable";
}

// ---------------------------------------------------------------------------
// TFE-02: corruption fault without fallback → ABORT
// ---------------------------------------------------------------------------

TEST_F(TrainingFailsafeEdgeCasesTest, TFE02_CorruptionWithoutFallback_Abort) {
    CheckpointFaultContext ctx;
    ctx.fault_code      = TrainingErrorCode::CHECKPOINT_TRUNCATED;
    ctx.checkpoint_path = "/checkpoints/step_100";
    ctx.has_fallback    = false;

    auto result = checkpoint_handler_.handle(ctx, emitter_.get());

    EXPECT_EQ(result.strategy, CheckpointFaultResult::RecoveryStrategy::ABORT)
        << "TFE-02: corruption without fallback must trigger ABORT";
    EXPECT_TRUE(result.recovery_path.empty())
        << "TFE-02: recovery_path must be empty for ABORT";
    EXPECT_FALSE(listener_->last().recoverable)
        << "TFE-02: ABORT incident must be marked non-recoverable";
}

// ---------------------------------------------------------------------------
// TFE-03: transient I/O fault during save, no fallback → SKIP_SAVE
// ---------------------------------------------------------------------------

TEST_F(TrainingFailsafeEdgeCasesTest, TFE03_IoFaultDuringSave_NoFallback_SkipSave) {
    CheckpointFaultContext ctx;
    ctx.fault_code      = TrainingErrorCode::CHECKPOINT_WRITE_FAILED;
    ctx.checkpoint_path = "/checkpoints/step_200";
    ctx.has_fallback    = false;
    ctx.during_save     = true;

    auto result = checkpoint_handler_.handle(ctx, emitter_.get());

    EXPECT_EQ(result.strategy, CheckpointFaultResult::RecoveryStrategy::SKIP_SAVE)
        << "TFE-03: transient I/O during save without fallback must be SKIP_SAVE";
    EXPECT_TRUE(listener_->last().recoverable)
        << "TFE-03: SKIP_SAVE incident must be marked recoverable";
}

// ---------------------------------------------------------------------------
// TFE-04: disk exhaustion during save → SKIP_SAVE
// ---------------------------------------------------------------------------

TEST_F(TrainingFailsafeEdgeCasesTest, TFE04_DiskExhaustionDuringSave_SkipSave) {
    CheckpointFaultContext ctx;
    ctx.fault_code      = TrainingErrorCode::CHECKPOINT_DISK_SPACE_EXHAUSTED;
    ctx.checkpoint_path = "/checkpoints/step_300";
    ctx.has_fallback    = true;
    ctx.fallback_path   = "/checkpoints/step_250";
    ctx.during_save     = true;

    auto result = checkpoint_handler_.handle(ctx, emitter_.get());

    EXPECT_EQ(result.strategy, CheckpointFaultResult::RecoveryStrategy::SKIP_SAVE)
        << "TFE-04: disk exhaustion during save must be SKIP_SAVE even if fallback exists";
}

// ---------------------------------------------------------------------------
// TFE-05: disk exhaustion during load with fallback → ROLLBACK
// ---------------------------------------------------------------------------

TEST_F(TrainingFailsafeEdgeCasesTest, TFE05_DiskExhaustionDuringLoad_Rollback) {
    CheckpointFaultContext ctx;
    ctx.fault_code      = TrainingErrorCode::CHECKPOINT_DISK_SPACE_EXHAUSTED;
    ctx.checkpoint_path = "/checkpoints/step_400";
    ctx.has_fallback    = true;
    ctx.fallback_path   = "/checkpoints/step_350";
    ctx.during_save     = false;

    auto result = checkpoint_handler_.handle(ctx, emitter_.get());

    EXPECT_EQ(result.strategy, CheckpointFaultResult::RecoveryStrategy::ROLLBACK)
        << "TFE-05: disk exhaustion during load with fallback must be ROLLBACK";
    EXPECT_EQ(result.recovery_path, "/checkpoints/step_350");
}

// ---------------------------------------------------------------------------
// TFE-06: emitter receives incident for ABORT case (nullptr passthrough test)
// ---------------------------------------------------------------------------

TEST_F(TrainingFailsafeEdgeCasesTest, TFE06_AbortCase_EmitterReceivesEvent) {
    CheckpointFaultContext ctx;
    ctx.fault_code      = TrainingErrorCode::CHECKPOINT_INVALID_FORMAT;
    ctx.checkpoint_path = "/checkpoints/step_500";
    ctx.has_fallback    = false;

    // With emitter
    listener_->clear();
    auto result_with = checkpoint_handler_.handle(ctx, emitter_.get());
    EXPECT_TRUE(result_with.incident_emitted) << "TFE-06: incident_emitted must be true with emitter";
    EXPECT_EQ(listener_->count(), 1u) << "TFE-06: exactly one incident when emitter provided";

    // Without emitter (nullptr)
    listener_->clear();
    auto result_null = checkpoint_handler_.handle(ctx, nullptr);
    EXPECT_FALSE(result_null.incident_emitted) << "TFE-06: incident_emitted must be false with nullptr";
    EXPECT_EQ(listener_->count(), 0u) << "TFE-06: no incident when emitter is nullptr";
}

// ---------------------------------------------------------------------------
// TFE-07: merge fault with valid snapshot → ROLLBACK, snapshot restored
// ---------------------------------------------------------------------------

TEST_F(TrainingFailsafeEdgeCasesTest, TFE07_MergeFaultWithSnapshot_Rollback) {
    AdapterMergeFaultContext ctx;
    ctx.fault_code                  = TrainingErrorCode::MERGE_ADAPTER_DIMENSION_MISMATCH;
    ctx.pre_merge_snapshot.adapter_id       = "legal_adapter_v3";
    ctx.pre_merge_snapshot.checkpoint_path  = "/adapters/legal_v3_ckpt";
    ctx.pre_merge_snapshot.adapter_version  = "3.0.0";
    ctx.pre_merge_snapshot.layer_count      = 12;
    ctx.layers_merged_before_fault  = 0;
    ctx.total_layers                = 12;

    auto result = merge_failsafe_.handle(ctx, emitter_.get());

    EXPECT_EQ(result.strategy, AdapterMergeFaultResult::RecoveryStrategy::ROLLBACK)
        << "TFE-07: merge fault with snapshot must be ROLLBACK";
    EXPECT_EQ(result.restore_snapshot.adapter_id, "legal_adapter_v3");
    EXPECT_EQ(result.restore_snapshot.checkpoint_path, "/adapters/legal_v3_ckpt");
    EXPECT_FALSE(result.partial_merge_detected)
        << "TFE-07: no layers merged → partial_merge_detected must be false";
    EXPECT_TRUE(result.incident_emitted);
}

// ---------------------------------------------------------------------------
// TFE-08: merge fault without snapshot → ABORT_MERGE
// ---------------------------------------------------------------------------

TEST_F(TrainingFailsafeEdgeCasesTest, TFE08_MergeFaultWithoutSnapshot_Abort) {
    AdapterMergeFaultContext ctx;
    ctx.fault_code = TrainingErrorCode::MERGE_WEIGHTS_INVALID;
    // Empty snapshot (adapter_id is empty)
    ctx.layers_merged_before_fault = 5;
    ctx.total_layers               = 10;

    auto result = merge_failsafe_.handle(ctx, emitter_.get());

    EXPECT_EQ(result.strategy, AdapterMergeFaultResult::RecoveryStrategy::ABORT_MERGE)
        << "TFE-08: merge fault without snapshot must be ABORT_MERGE";
    EXPECT_TRUE(result.incident_emitted);
}

// ---------------------------------------------------------------------------
// TFE-09: partial merge detected flag
// ---------------------------------------------------------------------------

TEST_F(TrainingFailsafeEdgeCasesTest, TFE09_PartialMerge_FlagSet) {
    AdapterMergeFaultContext ctx;
    ctx.fault_code = TrainingErrorCode::MERGE_WEIGHTS_INVALID;
    ctx.pre_merge_snapshot.adapter_id = "merge_test_v1";
    ctx.layers_merged_before_fault    = 7;
    ctx.total_layers                  = 10;

    auto result = merge_failsafe_.handle(ctx, nullptr);

    EXPECT_TRUE(result.partial_merge_detected)
        << "TFE-09: partial merge must be flagged when layers_merged > 0";
    EXPECT_EQ(result.strategy, AdapterMergeFaultResult::RecoveryStrategy::ROLLBACK);
}

// ---------------------------------------------------------------------------
// TFE-10: enrichment coverage above threshold → no incident
// ---------------------------------------------------------------------------

TEST_F(TrainingFailsafeEdgeCasesTest, TFE10_CoverageAboveThreshold_NoIncident) {
    EnrichmentGapContext ctx;
    ctx.total_items                = 100;
    ctx.enriched_items             = 80;
    ctx.minimum_coverage_threshold = 0.7;
    ctx.source_component           = "KGEnricher";
    ctx.operation                  = "enrich_batch";

    listener_->clear();
    auto summary = enrichment_handler_.evaluate(ctx, emitter_.get());

    EXPECT_TRUE(summary.meets_threshold)
        << "TFE-10: 80% coverage > 70% threshold must meet threshold";
    EXPECT_NEAR(summary.coverage_ratio, 0.8, 1e-9)
        << "TFE-10: coverage_ratio must be 0.8";
    EXPECT_EQ(listener_->count(), 0u)
        << "TFE-10: no incident must be emitted when coverage meets threshold";
    EXPECT_FALSE(summary.incident_emitted);
}

// ---------------------------------------------------------------------------
// TFE-11: enrichment coverage below threshold → incident emitted
// ---------------------------------------------------------------------------

TEST_F(TrainingFailsafeEdgeCasesTest, TFE11_CoverageBelowThreshold_IncidentEmitted) {
    EnrichmentGapContext ctx;
    ctx.total_items                = 200;
    ctx.enriched_items             = 60;
    ctx.minimum_coverage_threshold = 0.5;
    ctx.source_component           = "KGEnricher";
    ctx.operation                  = "enrich_domain_batch";

    listener_->clear();
    auto summary = enrichment_handler_.evaluate(ctx, emitter_.get());

    EXPECT_FALSE(summary.meets_threshold)
        << "TFE-11: 30% coverage < 50% threshold must not meet threshold";
    EXPECT_NEAR(summary.coverage_ratio, 0.3, 1e-9);
    EXPECT_EQ(summary.gap_items, 140u);
    EXPECT_EQ(listener_->count(), 1u)
        << "TFE-11: exactly one incident must be emitted below threshold";
    EXPECT_TRUE(summary.incident_emitted);
    EXPECT_EQ(listener_->last().cls, TrainingIncidentClass::DATASET);
    EXPECT_TRUE(listener_->last().recoverable)
        << "TFE-11: enrichment gap incident must be marked recoverable";
}

// ---------------------------------------------------------------------------
// TFE-12: empty batch (total_items == 0) → coverage 1.0, no incident
// ---------------------------------------------------------------------------

TEST_F(TrainingFailsafeEdgeCasesTest, TFE12_EmptyBatch_FullCoverage_NoIncident) {
    EnrichmentGapContext ctx;
    ctx.total_items                = 0;
    ctx.enriched_items             = 0;
    ctx.minimum_coverage_threshold = 0.5;
    ctx.source_component           = "KGEnricher";

    listener_->clear();
    auto summary = enrichment_handler_.evaluate(ctx, emitter_.get());

    EXPECT_TRUE(summary.meets_threshold)
        << "TFE-12: empty batch must have coverage 1.0 and meet threshold";
    EXPECT_NEAR(summary.coverage_ratio, 1.0, 1e-9);
    EXPECT_EQ(summary.gap_items, 0u);
    EXPECT_EQ(listener_->count(), 0u)
        << "TFE-12: no incident for empty batch";
    EXPECT_FALSE(summary.incident_emitted);
}

} // namespace
} // namespace training
} // namespace themis
