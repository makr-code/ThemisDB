/**
 * @file test_phase11_lifecycle.cpp
 * @brief Comprehensive tests for RAG Phase 11 components
 *
 * Tests model registry, retraining scheduler, evaluator, and promoter
 * for complete lifecycle from training through deployment.
 */

#include <cassert>
#include <iostream>
#include <sstream>

#include "rag/model_evaluator.h"
#include "rag/model_promoter.h"
#include "rag/model_registry.h"
#include "rag/retraining_scheduler.h"

using namespace themis::rag::lifecycle;

// ===== Test 1: ModelRegistry Basic Operations =====
void TestModelRegistry() {
  std::cout << "Test: ModelRegistry Basic Operations..." << std::endl;

  ModelRegistry registry("/tmp/test_registry.db");

  // Test 1.1: Register model
  uint32_t v1 = registry.RegisterModel("cost-optimizer", "dataset-1",
                                       R"({"ndcg@10": 0.75})", R"({"latency_ms": 42.5})",
                                       "/models/cost-v1", 0);
  assert(v1 == 1);
  assert(registry.Count() == 1);

  // Test 1.2: Register second model with lineage
  uint32_t v2 = registry.RegisterModel("cost-optimizer", "dataset-2",
                                       R"({"ndcg@10": 0.78})", R"({"latency_ms": 40.0})",
                                       "/models/cost-v2", v1);
  assert(v2 == 2);

  // Test 1.3: Get by version
  auto meta = registry.GetByVersion(v1);
  assert(meta.has_value());
  assert(meta->version == v1);
  assert(meta->status == ModelStatus::kDraft);

  // Test 1.4: Update status (draft → validated)
  bool ok = registry.UpdateModelStatus(v1, ModelStatus::kValidated, "Validation passed");
  assert(ok);
  meta = registry.GetByVersion(v1);
  assert(meta->status == ModelStatus::kValidated);

  // Test 1.5: Invalid transition should fail
  ok = registry.UpdateModelStatus(v1, ModelStatus::kValidated, "Should fail");
  assert(!ok);  // Can't transition from validated to validated

  // Test 1.6: Valid transition (validated → candidate)
  ok = registry.UpdateModelStatus(v1, ModelStatus::kCandidate, "Ready for canary");
  assert(ok);

  // Test 1.7: Get models by status
  auto candidates = registry.GetByStatus(ModelStatus::kCandidate);
  assert(candidates.size() == 1);
  assert(candidates[0].version == v1);

  // Test 1.8: Get lineage
  auto lineage = registry.GetLineage(v2);
  assert(lineage.size() == 2);
  assert(lineage[0].version == v2);  // Most recent first
  assert(lineage[1].version == v1);

  std::cout << "  ✅ ModelRegistry tests passed" << std::endl;
}

// ===== Test 2: RetariningScheduler Triggers =====
void TestRetariningScheduler() {
  std::cout << "Test: RetariningScheduler Triggers..." << std::endl;

  ModelRegistry registry("/tmp/test_registry2.db");
  RetariningScheduler scheduler(registry, 24, 0.15, 0.05);

  // Test 2.1: Manual retraining request
  bool ok = scheduler.RequestRetraining();
  assert(ok);
  assert(scheduler.IsRetariningInProgress());

  auto [trigger, reason] = scheduler.GetLastTriggerReason();
  assert(trigger == RetariningTrigger::kManualRequest);

  // Test 2.2: Cost model drift detection
  scheduler.ReportCostModelDrift(50.0, 40.0);  // 25% increase, above 15% threshold
  // Note: IsRetariningInProgress is true from manual request, so drift won't trigger immediately

  // Test 2.3: Quality regression detection
  scheduler.ReportQualityRegression("ndcg@10", 0.70, 0.75);  // 6.7% regression
  // Same - already in progress

  std::cout << "  ✅ RetariningScheduler tests passed" << std::endl;
}

// ===== Test 3: ModelEvaluator Statistical Validation =====
void TestModelEvaluator() {
  std::cout << "Test: ModelEvaluator Statistical Validation..." << std::endl;

  ModelRegistry registry("/tmp/test_registry3.db");
  ModelEvaluator evaluator(registry, 0.02, true, 0.3);

  // Register baseline and candidate models
  uint32_t baseline = registry.RegisterModel(
      "embedding-model", "dataset-1", R"({"ndcg@10": 0.75, "recall@10": 0.82})",
      R"({"latency_ms": 42.5, "cost_per_query": 0.01})", "/models/baseline", 0);

  uint32_t candidate = registry.RegisterModel(
      "embedding-model", "dataset-2", R"({"ndcg@10": 0.78, "recall@10": 0.85})",
      R"({"latency_ms": 40.0, "cost_per_query": 0.009})", "/models/candidate-v1", baseline);

  // Test 3.1: Evaluate candidate
  auto decision = evaluator.Evaluate(candidate, R"({"ndcg@10": 0.75, "recall@10": 0.82})",
                                     R"({"latency_ms": 42.5, "cost_per_query": 0.01})");

  assert(decision.approved);  // 4% ndcg improvement + 3% recall improvement > 2% threshold
  assert(decision.metric_results.size() >= 2);  // At least ndcg and recall metrics

  // Test 3.2: Threshold configuration
  evaluator.SetMinImprovementThreshold(0.10);  // Require 10% improvement
  decision = evaluator.Evaluate(candidate, R"({"ndcg@10": 0.75, "recall@10": 0.82})",
                               R"({"latency_ms": 42.5, "cost_per_query": 0.01})");
  assert(!decision.approved);  // Now requires > 10% improvement

  std::cout << "  ✅ ModelEvaluator tests passed" << std::endl;
}

// ===== Test 4: ModelPromoter Canary Deployment =====
void TestModelPromoter() {
  std::cout << "Test: ModelPromoter Canary Deployment..." << std::endl;

  ModelRegistry registry("/tmp/test_registry4.db");
  ModelEvaluator evaluator(registry, 0.02, true, 0.3);
  ModelPromoter promoter(registry, evaluator, 0.05, 1);

  // Register baseline (deployed) and candidate models
  uint32_t baseline = registry.RegisterModel("retriever", "dataset-1",
                                             R"({"ndcg@10": 0.75})", R"({"latency_ms": 42.5})",
                                             "/models/baseline", 0);
  registry.UpdateModelStatus(baseline, ModelStatus::kValidated);
  registry.UpdateModelStatus(baseline, ModelStatus::kCandidate);
  registry.UpdateModelStatus(baseline, ModelStatus::kDeployed);

  uint32_t candidate = registry.RegisterModel(
      "retriever", "dataset-2", R"({"ndcg@10": 0.77})", R"({"latency_ms": 40.0})",
      "/models/candidate-v1", baseline);

  // Test 4.1: Start canary
  bool ok = promoter.StartCanary(candidate);
  assert(ok);
  assert(promoter.IsCanaryActive());
  assert(promoter.GetCurrentPhase() == CanaryPhase::kShadow);

  // Test 4.2: Get traffic split (shadow = 0% traffic)
  auto split = promoter.GetTrafficSplit();
  assert(split.baseline_version == baseline);
  assert(split.canary_version == candidate);
  assert(split.canary_traffic_pct == 0.0);

  // Test 4.3: Advance through phases
  ok = promoter.AdvancePhase();
  assert(ok);
  assert(promoter.GetCurrentPhase() == CanaryPhase::kCanary5);

  split = promoter.GetTrafficSplit();
  assert(split.canary_traffic_pct == 5.0);

  // Test 4.4: Report good metrics (no regression)
  bool passes = promoter.ReportMetric("ndcg@10", 0.76, 0.75);
  assert(passes);  // 1.3% improvement, below 5% threshold

  // Test 4.5: Report bad metric (regression)
  passes = promoter.ReportMetric("ndcg@10", 0.70, 0.75);
  assert(!passes);  // 6.7% regression, above 5% threshold, triggers rollback
  assert(!promoter.IsCanaryActive());  // Rollback removes canary

  std::cout << "  ✅ ModelPromoter tests passed" << std::endl;
}

// ===== Test 5: End-to-End Lifecycle =====
void TestEndToEndLifecycle() {
  std::cout << "Test: End-to-End Lifecycle..." << std::endl;

  ModelRegistry registry("/tmp/test_registry_e2e.db");
  ModelEvaluator evaluator(registry, 0.02, true, 0.3);
  ModelPromoter promoter(registry, evaluator, 0.05, 1);

  // Step 1: Train new model (register as draft)
  uint32_t trained_model = registry.RegisterModel(
      "complete-rag", "dataset-prod", R"({"ndcg@10": 0.80, "recall@10": 0.85})",
      R"({"latency_ms": 45.0, "cost": 0.011})", "/models/complete-rag-v1", 0);
  assert(registry.GetByVersion(trained_model)->status == ModelStatus::kDraft);

  // Step 2: Validate model (draft → validated)
  registry.UpdateModelStatus(trained_model, ModelStatus::kValidated,
                             "Training completed successfully");
  assert(registry.GetByVersion(trained_model)->status == ModelStatus::kValidated);

  // Step 3: Approve for deployment (validated → candidate)
  auto decision = evaluator.Evaluate(trained_model, R"({"ndcg@10": 0.75, "recall@10": 0.80})",
                                     R"({"latency_ms": 50.0, "cost": 0.012})");
  if (decision.approved) {
    registry.UpdateModelStatus(trained_model, ModelStatus::kCandidate, "Evaluation approved");
  }
  assert(registry.GetByVersion(trained_model)->status == ModelStatus::kCandidate);

  // Step 4: Start canary deployment
  bool ok = promoter.StartCanary(trained_model);
  assert(ok);
  assert(promoter.GetCurrentPhase() == CanaryPhase::kShadow);

  // Step 5: Progress through canary phases
  for (int i = 0; i < 6; i++) {
    promoter.AdvancePhase();
    // Report healthy metrics at each phase
    promoter.ReportMetric("ndcg@10", 0.79, 0.75);
    promoter.ReportMetric("recall@10", 0.84, 0.80);
  }
  assert(promoter.GetCurrentPhase() == CanaryPhase::kDeployed);

  // Step 6: Finalize deployment
  ok = promoter.FinalizeDeployment();
  assert(ok);
  assert(!promoter.IsCanaryActive());

  // Verify final state
  auto deployed = registry.GetDeployedModel();
  assert(deployed.has_value());
  assert(deployed->version == trained_model);
  assert(deployed->status == ModelStatus::kDeployed);

  std::cout << "  ✅ End-to-End Lifecycle tests passed" << std::endl;
}

int main() {
  try {
    std::cout << "\n=== RAG Phase 11 Lifecycle Tests ===" << std::endl;

    TestModelRegistry();
    TestRetariningScheduler();
    TestModelEvaluator();
    TestModelPromoter();
    TestEndToEndLifecycle();

    std::cout << "\n✅ All Phase 11 tests passed!" << std::endl;
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "\n❌ Test failed: " << e.what() << std::endl;
    return 1;
  }
}
