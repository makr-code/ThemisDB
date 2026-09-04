/**
 * Integration tests for DecisionRecordYamlProcessor wiring in:
 *   - LoRARouter              → LORA_ADAPTER_SELECTION
 *   - AdapterLoadBalancer     → LORA_RANK_ADJUSTMENT
 *   - LoRAOrchestrator        → LOOP_TRIGGER
 *
 * These tests exercise the DecisionRecord struct directly and verify
 * the processor accepts well-formed records without crashing.  Full
 * end-to-end tests that write YAML files to disk are covered in
 * test_decision_record_yaml_processor.cpp.
 */

#include <gtest/gtest.h>

#include "llm/decision_record_yaml_processor.h"
#include "llm/lora_framework/lora_orchestrator.h"

#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>

namespace fs = std::filesystem;

using themis::llm::DecisionRecord;
using themis::llm::DecisionRecordYamlProcessor;
using themis::llm::lora::LoRAOrchestrator;

// ─── Helpers ──────────────────────────────────────────────────────────────────

static fs::path makeTempDir(const std::string& suffix) {
    auto tmp = fs::temp_directory_path() / ("dr_integ_test_" + suffix);
    fs::create_directories(tmp);
    return tmp;
}

// Count YAML files in a directory tree.
static size_t countYamlFiles(const fs::path& dir) {
    if (!fs::exists(dir)) {
      return 0;
    }
    size_t n = 0;
    for (const auto& e : fs::recursive_directory_iterator(dir)) {
        if (e.path().extension() == ".yaml") {
          ++n;
        }
    }
    return n;
}

// ─── LORA_ADAPTER_SELECTION ───────────────────────────────────────────────────

TEST(DrIntegration, LoraAdapterSelectionRecordFields) {
    // Build a LORA_ADAPTER_SELECTION record as LoRARouter::emitAdapterSelectionRecord
    // would produce it, and verify all expected fields are present.
    DecisionRecord rec;
    rec.decision_type               = "LORA_ADAPTER_SELECTION";
    rec.component                   = "LoRARouter";
    rec.outcome                     = "SUCCESS";
    rec.confidence                  = 0.85f;
    rec.latency_ms                  = 3;
    rec.parameters["adapter_id"]    = "lora_sql_expert";
    rec.parameters["base_model_id"] = "llama-2-7b";
    rec.parameters["gpu_device_id"] = "0";
    rec.parameters["similarity_score"] = "0.850000";
    rec.parameters["policy"]        = "1";   // LOAD_AWARE = 1
    rec.parameters["is_fallback"]   = "false";

    EXPECT_EQ(rec.decision_type, "LORA_ADAPTER_SELECTION");
    EXPECT_EQ(rec.component, "LoRARouter");
    EXPECT_EQ(rec.outcome, "SUCCESS");
    ASSERT_TRUE(rec.confidence.has_value());
    EXPECT_NEAR(*rec.confidence, 0.85f, 1e-5f);
    EXPECT_EQ(rec.latency_ms, 3);
    EXPECT_EQ(rec.parameters.at("adapter_id"), "lora_sql_expert");
    EXPECT_EQ(rec.parameters.at("is_fallback"), "false");
}

TEST(DrIntegration, LoraAdapterSelectionFallbackRecord) {
    // Fallback decisions should mark outcome = "FALLBACK".
    DecisionRecord rec;
    rec.decision_type               = "LORA_ADAPTER_SELECTION";
    rec.component                   = "LoRARouter";
    rec.outcome                     = "FALLBACK";
    rec.parameters["adapter_id"]    = "";
    rec.parameters["is_fallback"]   = "true";
    rec.parameters["reason"]        = "No semantic candidates found";

    EXPECT_EQ(rec.outcome, "FALLBACK");
    EXPECT_EQ(rec.parameters.at("is_fallback"), "true");
    EXPECT_FALSE(rec.parameters.at("reason").empty());
}

TEST(DrIntegration, LoraAdapterSelectionSubmitToProcessor) {
    auto tmp = makeTempDir("lora_adapter_sel");
    DecisionRecordYamlProcessor::Config cfg;
    cfg.log_dir              = tmp;
    cfg.create_daily_subdirs = false;
    cfg.max_queue_depth      = 100;

    auto proc = std::make_shared<DecisionRecordYamlProcessor>(cfg);

    DecisionRecord rec;
    rec.decision_type               = "LORA_ADAPTER_SELECTION";
    rec.component                   = "LoRARouter";
    rec.outcome                     = "SUCCESS";
    rec.parameters["adapter_id"]    = "lora_test_adapter";
    rec.parameters["is_fallback"]   = "false";

    EXPECT_TRUE(proc->submit(rec));
    proc->flush();

    auto stats = proc->getStats();
    EXPECT_EQ(stats.submitted, 1u);
    EXPECT_EQ(stats.written, 1u);
    EXPECT_EQ(stats.dropped, 0u);
    EXPECT_EQ(stats.errors, 0u);

    EXPECT_EQ(countYamlFiles(tmp), 1u);

    fs::remove_all(tmp);
}

// ─── LORA_RANK_ADJUSTMENT ─────────────────────────────────────────────────────

TEST(DrIntegration, LoraRankAdjustmentRecordFields) {
    // Build a LORA_RANK_ADJUSTMENT record as AdapterLoadBalancer::emitRebalanceRecord
    // would produce it.
    DecisionRecord rec;
    rec.decision_type                  = "LORA_RANK_ADJUSTMENT";
    rec.component                      = "AdapterLoadBalancer";
    rec.outcome                        = "SUCCESS";
    rec.parameters["migrations"]       = "3";
    rec.parameters["num_gpus"]         = "4";
    rec.parameters["avg_gpu_load"]     = "0.650000";
    rec.parameters["total_migrations"] = "12";
    rec.parameters["total_evictions"]  = "2";

    EXPECT_EQ(rec.decision_type, "LORA_RANK_ADJUSTMENT");
    EXPECT_EQ(rec.component, "AdapterLoadBalancer");
    EXPECT_EQ(rec.outcome, "SUCCESS");
    EXPECT_EQ(rec.parameters.at("migrations"), "3");
    EXPECT_EQ(rec.parameters.at("num_gpus"), "4");
}

TEST(DrIntegration, LoraRankAdjustmentSkippedRecord) {
    // When no migrations occur, outcome should be SKIPPED_BUDGET.
    DecisionRecord rec;
    rec.decision_type                  = "LORA_RANK_ADJUSTMENT";
    rec.component                      = "AdapterLoadBalancer";
    rec.outcome                        = "SKIPPED_BUDGET";
    rec.parameters["migrations"]       = "0";
    rec.parameters["num_gpus"]         = "1";

    EXPECT_EQ(rec.outcome, "SKIPPED_BUDGET");
    EXPECT_EQ(rec.parameters.at("migrations"), "0");
}

TEST(DrIntegration, LoraRankAdjustmentSubmitToProcessor) {
    auto tmp = makeTempDir("lora_rank_adj");
    DecisionRecordYamlProcessor::Config cfg;
    cfg.log_dir              = tmp;
    cfg.create_daily_subdirs = false;
    cfg.max_queue_depth      = 100;

    auto proc = std::make_shared<DecisionRecordYamlProcessor>(cfg);

    // Submit two records: one SUCCESS and one SKIPPED_BUDGET.
    for (int i = 0; i < 2; ++i) {
        DecisionRecord rec;
        rec.decision_type              = "LORA_RANK_ADJUSTMENT";
        rec.component                  = "AdapterLoadBalancer";
        rec.outcome                    = (i == 0) ? "SUCCESS" : "SKIPPED_BUDGET";
        rec.parameters["migrations"]   = std::to_string(i * 2);
        EXPECT_TRUE(proc->submit(rec));
    }
    proc->flush();

    auto stats = proc->getStats();
    EXPECT_EQ(stats.submitted, 2u);
    EXPECT_EQ(stats.written, 2u);
    EXPECT_EQ(stats.errors, 0u);

    EXPECT_EQ(countYamlFiles(tmp), 2u);

    fs::remove_all(tmp);
}

// ─── LOOP_TRIGGER ─────────────────────────────────────────────────────────────

TEST(DrIntegration, LoopTriggerRecordFields) {
    // Build a LOOP_TRIGGER record as LoRAOrchestrator::loadAdapter would produce it.
    DecisionRecord rec;
    rec.decision_type          = "LOOP_TRIGGER";
    rec.component              = "LoRAOrchestrator";
    rec.outcome                = "SUCCESS";
    rec.parameters["adapter_id"] = "lora_domain_adapter";
    rec.parameters["job_id"]     = "load-42";
    rec.parameters["async"]      = "false";

    EXPECT_EQ(rec.decision_type, "LOOP_TRIGGER");
    EXPECT_EQ(rec.component, "LoRAOrchestrator");
    EXPECT_EQ(rec.outcome, "SUCCESS");
    EXPECT_EQ(rec.parameters.at("adapter_id"), "lora_domain_adapter");
    EXPECT_EQ(rec.parameters.at("job_id"), "load-42");
    EXPECT_EQ(rec.parameters.at("async"), "false");
}

TEST(DrIntegration, LoopTriggerAsyncRecord) {
    DecisionRecord rec;
    rec.decision_type          = "LOOP_TRIGGER";
    rec.component              = "LoRAOrchestrator";
    rec.outcome                = "RUNNING";
    rec.parameters["adapter_id"] = "lora_async_adapter";
    rec.parameters["job_id"]     = "load-7";
    rec.parameters["async"]      = "true";

    EXPECT_EQ(rec.outcome, "RUNNING");
    EXPECT_EQ(rec.parameters.at("async"), "true");
}

TEST(DrIntegration, LoopTriggerViaOrchestrator) {
    // Use LoRAOrchestrator with a real DecisionRecordYamlProcessor to verify
    // that loadAdapter() emits a LOOP_TRIGGER file on disk.
    auto tmp = makeTempDir("loop_trigger");
    DecisionRecordYamlProcessor::Config cfg;
    cfg.log_dir              = tmp;
    cfg.create_daily_subdirs = false;
    cfg.max_queue_depth      = 100;

    auto proc = std::make_shared<DecisionRecordYamlProcessor>(cfg);

    LoRAOrchestrator orchestrator;
    orchestrator.setDecisionRecordProcessor(proc);

    auto job_id = orchestrator.loadAdapter("test_adapter", /*async=*/false);
    EXPECT_FALSE(job_id.empty());

    // Wait for the background thread to flush.
    proc->flush();

    auto stats = proc->getStats();
    EXPECT_EQ(stats.submitted, 1u);
    EXPECT_EQ(stats.written, 1u);
    EXPECT_EQ(stats.dropped, 0u);
    EXPECT_EQ(stats.errors, 0u);

    EXPECT_EQ(countYamlFiles(tmp), 1u);

    // Verify the YAML file contains expected fields.
    for (const auto& e : fs::directory_iterator(tmp)) {
        if (e.path().extension() == ".yaml") {
            std::ifstream f(e.path());
            std::string content((std::istreambuf_iterator<char>(f)),
                                 std::istreambuf_iterator<char>());
            EXPECT_NE(content.find("LOOP_TRIGGER"), std::string::npos);
            EXPECT_NE(content.find("LoRAOrchestrator"), std::string::npos);
            EXPECT_NE(content.find("test_adapter"), std::string::npos);
        }
    }

    fs::remove_all(tmp);
}

TEST(DrIntegration, LoopTriggerDisabledWhenNoProcessor) {
    // When no processor is set, loadAdapter must not crash.
    LoRAOrchestrator orchestrator;
    // No setDecisionRecordProcessor() call — processor remains null.
    auto job_id = orchestrator.loadAdapter("adapter_no_dr", /*async=*/true);
    EXPECT_FALSE(job_id.empty());
}

TEST(DrIntegration, SetProcessorToNullptrDisablesEmission) {
    // Setting processor to nullptr after enabling must silence emission.
    auto tmp = makeTempDir("null_proc");
    DecisionRecordYamlProcessor::Config cfg;
    cfg.log_dir              = tmp;
    cfg.create_daily_subdirs = false;

    auto proc = std::make_shared<DecisionRecordYamlProcessor>(cfg);

    LoRAOrchestrator orchestrator;
    orchestrator.setDecisionRecordProcessor(proc);
    orchestrator.setDecisionRecordProcessor(nullptr);  // disable

    orchestrator.loadAdapter("silenced_adapter");
    proc->flush();

    // No records should have been submitted after the processor was cleared.
    EXPECT_EQ(proc->getStats().submitted, 0u);
    EXPECT_EQ(countYamlFiles(tmp), 0u);

    fs::remove_all(tmp);
}
