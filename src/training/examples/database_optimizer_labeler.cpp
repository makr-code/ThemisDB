/**
 * @file database_optimizer_labeler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=6; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=3, Debt=0, C=0, H=1, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// SPDX-License-Identifier: Apache-2.0
// Example: DATABASE_OPTIMIZER domain labeling + federation gradient export
//
// Paper 1 — §5 Training Data Pipeline / §7.4 Golden Dataset Construction
// Related issues: IMPL-A1 (docs/issues/lora_loops/IMPL-A1-dataset-construction.md)
//                 IMPL-A3 (docs/issues/lora_loops/IMPL-A3-federation-hooks.md)
//
// This example demonstrates:
//   1. Labeling (query, explain_plan, delta_latency_ms) triples with the
//      DATABASE_OPTIMIZER domain labeler.
//   2. Running those samples through LoRADataSelectionPipeline quality filters.
//   3. Triggering an incremental LoRA training cycle (Loop 4).
//   4. Exporting the gradient as an EncryptedGradient blob for federation.
//   5. Applying a GlobalAdapterDelta received from the FedAvg coordinator.
//
// Build (conceptual — requires full ThemisDB cmake tree):
//   cmake --build build --target example_database_optimizer_labeler
//
// NOTE: This example uses the planned IMPL-A1/A3 APIs that are not yet
// implemented. It serves as the acceptance-criteria specification for those
// issues. The code is intentionally pseudocode-style where the API does not
// yet exist, marked with /* PLANNED */ comments.
//

#include <iostream>
#include <vector>
#include <string>
#include <cassert>

// Existing production headers
#include "training/auto_labeler.h"              // AutoLabeler, DomainType
#include "training/incremental_lora_trainer.h"  // IncrementalLoRATrainer
#include "training/lora_data_selection.h"        // LoRADataSelectionPipeline
#include "training/training_interfaces.h"        // TrainingSample, ITrainer

// PLANNED headers (IMPL-A1 / IMPL-A3 — not yet implemented)
// #include "training/training_interfaces.h"     // EncryptedGradient, GlobalAdapterDelta

namespace {

// ---------------------------------------------------------------------------
// Step 1: Simulate a batch of optimizer-log entries
// ---------------------------------------------------------------------------
struct OptimizerLogEntry {
    std::string query_text;
    std::string explain_plan_json;
    double      delta_latency_ms;  // positive = regression, negative = improvement
};

std::vector<OptimizerLogEntry> simulateOptimizerLog() {
    return {
        { "SELECT * FROM orders WHERE status = 'open'",
          R"({"type":"SeqScan","table":"orders","rows":50000})",
          +120.0 },   // major regression — high confidence label
        { "SELECT id FROM users LIMIT 10",
          R"({"type":"IndexScan","index":"users_pkey","rows":10})",
          -2.0 },     // tiny improvement — below confidence threshold, will be filtered
        { "SELECT SUM(amount) FROM invoices GROUP BY tenant_id",
          R"({"type":"HashAggregate","table":"invoices","rows":200000})",
          +350.0 },   // severe regression
        { "SELECT name FROM products WHERE category = 'db'",
          R"({"type":"IndexOnlyScan","index":"products_category_idx","rows":42})",
          +0.5 },     // noise — will be rejected
    };
}

// ---------------------------------------------------------------------------
// Step 2: DATABASE_OPTIMIZER confidence function (IMPL-A1 spec)
//   confidence = tanh(|Δlatency_ms| / 50.0)
//   Threshold: 0.85 (≈ |Δlatency| ≥ 50 ms)
// ---------------------------------------------------------------------------
double computeOptimizerConfidence([[maybe_unused]] double delta_latency_ms) {
    return std::tanh(std::abs(delta_latency_ms) / 50.0);
}

} // namespace

int main() {
    std::cout << "=== DATABASE_OPTIMIZER Labeler Example (IMPL-A1 + IMPL-A3) ===\n\n";

    // -----------------------------------------------------------------------
    // 1. Label optimizer-log triples
    // -----------------------------------------------------------------------
    std::cout << "Step 1: Labeling optimizer-log entries\n";

    const auto log_entries = simulateOptimizerLog();
    std::vector<TrainingSample> labeled_samples; // TrainingSample defined in training_interfaces.h

    for (const auto& entry : log_entries) {
        const double confidence = computeOptimizerConfidence(entry.delta_latency_ms);

        // Construct a TrainingSample for the DATABASE_OPTIMIZER domain
        TrainingSample sample;
        sample.text       = entry.query_text + "\n" + entry.explain_plan_json;
        sample.confidence = static_cast<float>(confidence);
        // sample.domain  = DomainType::DATABASE_OPTIMIZER;  /* PLANNED: IMPL-A1 */

        std::cout << "  query: \"" << entry.query_text.substr(0, 40) << "...\"\n"
                  << "  Δlatency: " << entry.delta_latency_ms << " ms"
                  << "  → confidence: " << confidence
                  << (confidence >= 0.85 ? "  ✓ accepted\n" : "  ✗ filtered\n");

        if (confidence >= 0.85) {
            labeled_samples.push_back(std::move(sample));
        }
    }

    std::cout << "\n  Accepted " <<static_cast<int>(labeled_samples.size())
              << " / " <<static_cast<int>(log_entries.size()) << " samples\n\n";
    assert(static_cast<int>(labeled_samples.size()) == 2 && "Expected exactly 2 high-confidence samples");

    // -----------------------------------------------------------------------
    // 2. Quality filter via LoRADataSelectionPipeline (dedup, min confidence)
    // -----------------------------------------------------------------------
    std::cout << "Step 2: Applying LoRADataSelectionPipeline quality filters\n";

    /* PLANNED (IMPL-A1):
    LoRADataSelectionConfig sel_cfg;
    sel_cfg.min_confidence     = 0.85f;
    sel_cfg.dedup_query_window = 100;
    LoRADataSelectionPipeline pipeline(sel_cfg);
    auto filtered = pipeline.filter(labeled_samples);
    std::cout << "  After dedup + confidence filter: " <<static_cast<int>(filtered.size()) << " samples\n\n";
    */
    std::cout << "  [PLANNED — LoRADataSelectionPipeline not yet wired for DATABASE_OPTIMIZER domain]\n\n";

    // -----------------------------------------------------------------------
    // 3. Incremental LoRA training (Loop 4)
    // -----------------------------------------------------------------------
    std::cout << "Step 3: Incremental LoRA training (Loop 4)\n";

    /* PLANNED (uses existing IncrementalLoRATrainer, extended in IMPL-A1):
    IncrementalTrainingConfig train_cfg;
    train_cfg.mode   = TrainingMode::INCREMENTAL;
    train_cfg.domain = DomainType::DATABASE_OPTIMIZER;
    train_cfg.lora_rank  = 16;
    train_cfg.max_epochs = 3;

    IncrementalLoRATrainer trainer(train_cfg);
    trainer.train(filtered);
    std::cout << "  Training complete — adapter version: " << trainer.activeVersion() << "\n\n";
    */
    std::cout << "  [PLANNED — full training cycle requires IMPL-A1 domain wiring]\n\n";

    // -----------------------------------------------------------------------
    // 4. Export gradient as EncryptedGradient (IMPL-A3)
    // -----------------------------------------------------------------------
    std::cout << "Step 4: Export gradient for federated aggregation (IMPL-A3)\n";

    /* PLANNED (IMPL-A3):
    EncryptedGradient grad = trainer.exportGradient();
    assert(!grad.blob.empty() && "Gradient blob must be non-empty");
    // Privacy invariant: raw query text must NOT appear in the blob
    const std::string blob_str(grad.blob.begin(), grad.blob.end());
    assert(blob_str.find("SELECT") == std::string::npos &&
           "Privacy violation: raw query text found in gradient blob");
    std::cout << "  Exported " <<static_cast<int>(grad.blob.size()) << " bytes (AES-256-GCM encrypted)\n\n";
    */
    std::cout << "  [PLANNED — exportGradient() to be implemented in IMPL-A3]\n\n";

    // -----------------------------------------------------------------------
    // 5. Apply GlobalAdapterDelta from FedAvg coordinator (IMPL-A3)
    // -----------------------------------------------------------------------
    std::cout << "Step 5: Apply GlobalAdapterDelta from federation coordinator (IMPL-A3)\n";

    /* PLANNED (IMPL-A3):
    GlobalAdapterDelta delta = federation_coordinator.getAggregatedDelta();
    trainer.applyGlobalDelta(delta);
    std::cout << "  Applied global FedAvg delta — adapter weights updated.\n\n";
    */
    std::cout << "  [PLANNED — applyGlobalDelta() to be implemented in IMPL-A3]\n\n";

    // -----------------------------------------------------------------------
    // Summary
    // -----------------------------------------------------------------------
    std::cout << "=== Summary ===\n"
              << "  Labeled samples accepted:  " <<static_cast<int>(labeled_samples.size()) << "\n"
              << "  Training cycle:            [PLANNED — IMPL-A1]\n"
              << "  Gradient export:           [PLANNED — IMPL-A3]\n"
              << "  FedAvg delta applied:      [PLANNED — IMPL-A3]\n"
              << "\nSee docs/issues/lora_loops/ for implementation specs.\n";

    return 0;
}
