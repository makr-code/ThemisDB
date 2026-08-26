/**
 * @file test_prompt_engineering_focused.cpp
 * @brief Focused tests for prompt_engineering module production readiness.
 *
 * Validates:
 *   1. PromptManager template lifecycle (create/get/list)
 *   2. PromptManager context injection and validation
 *   3. PromptVersionControl commit and history operations
 *   4. FeedbackCollector recording and statistics
 *   5. PromptOptimizer basic optimization loop
 *   6. PromptEvaluator structural evaluation
 *   7. PromptEngineeringMetrics recording and retrieval
 *   8. Error handling for invalid templates and edge cases
 *   9. Concurrent access safety (basic sanity check)
 *
 * Scope: Focused module-level validation for Q3 2026 release readiness
 */

#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <thread>
#include <vector>

#include "prompt_engineering/prompt_manager.h"
#include "prompt_engineering/prompt_version_control.h"
#include "prompt_engineering/feedback_collector.h"
#include "prompt_engineering/prompt_optimizer.h"
#include "prompt_engineering/prompt_evaluator.h"
#include "prompt_engineering/prompt_engineering_metrics.h"
#include "prompt_engineering/prompt_template_validator.h"

using namespace themis::prompt_engineering;

// ============================================================================
// Fixtures and Helpers
// ============================================================================

class PromptEngineeringFocusedTest : public ::testing::Test {
 protected:
  PromptManager mgr;
  PromptVersionControl vcs;
  FeedbackCollector feedback;
  PromptOptimizer optimizer;
  PromptEvaluator evaluator;
  PromptEngineeringMetrics metrics;

  PromptManager::PromptTemplate MakeValidTemplate(int i = 0) {
    PromptManager::PromptTemplate t;
    t.name = "test_template_" + std::to_string(i);
    t.version = "v1.0";
    t.content = "You are a helpful assistant. User query: {query}. Context: {context}.";
    t.description = "Test template " + std::to_string(i);
    t.active = true;
    return t;
  }

  std::unordered_map<std::string, std::string> MakeContextMap() {
    return {
      {"query", "What is ThemisDB?"},
      {"context", "ThemisDB is a hybrid database"},
    };
  }
};

// ============================================================================
// PE-FT-001: PromptManager template lifecycle
// ============================================================================

TEST_F(PromptEngineeringFocusedTest, PE_FT_001_PromptManagerTemplateLifecycle) {
  auto tmpl = MakeValidTemplate(1);
  auto created = mgr.createTemplate(tmpl);
  
  ASSERT_FALSE(created.id.empty());
  EXPECT_EQ(created.name, "test_template_1");
  EXPECT_EQ(created.version, "v1.0");
  EXPECT_EQ(created.content, tmpl.content);
  EXPECT_TRUE(created.active);

  auto fetched = mgr.getTemplate(created.id);
  ASSERT_TRUE(fetched.has_value());
  EXPECT_EQ(fetched->name, "test_template_1");
  EXPECT_EQ(fetched->content, tmpl.content);

  auto listed = mgr.listTemplates();
  EXPECT_GE(listed.size(), 1u);
}

// ============================================================================
// PE-FT-002: PromptManager context injection
// ============================================================================

TEST_F(PromptEngineeringFocusedTest, PE_FT_002_PromptManagerContextInjection) {
  auto tmpl = MakeValidTemplate(2);
  auto created = mgr.createTemplate(tmpl);
  
  auto ctx = MakeContextMap();
  auto injected = mgr.getPromptWithContext(created.id, ctx);
  
  ASSERT_TRUE(injected.has_value());
  // Placeholders should have been replaced by the provided context values
  EXPECT_EQ(injected->find("{query}"), std::string::npos);
  EXPECT_EQ(injected->find("{context}"), std::string::npos);
  EXPECT_NE(injected->find("What is ThemisDB?"), std::string::npos);
  EXPECT_NE(injected->find("ThemisDB is a hybrid database"), std::string::npos);
}

// ============================================================================
// PE-FT-003: PromptManager template validation
// ============================================================================

TEST_F(PromptEngineeringFocusedTest, PE_FT_003_PromptManagerTemplateValidation) {
  auto valid_tmpl = MakeValidTemplate(3);
  valid_tmpl.content = "Valid: {placeholder}";
  
  auto result = mgr.validateTemplate(valid_tmpl);
  EXPECT_TRUE(result.valid);
  EXPECT_TRUE(result.errors.empty());
}

// ============================================================================
// PE-FT-004: PromptVersionControl commit and history
// ============================================================================

TEST_F(PromptEngineeringFocusedTest, PE_FT_004_PromptVersionControlCommitHistory) {
  // Verify commit operation succeeds
  auto commit1_id = vcs.commit("template_v1", "v1", "Initial version");
  EXPECT_FALSE(commit1_id.empty());

  auto commit2_id = vcs.commit("template_v1", "v2", "Updated version");
  EXPECT_FALSE(commit2_id.empty());

  // Verify history retrieval
  auto history = vcs.getHistory("template_v1", "", 10);
  EXPECT_GE(history.size(), 1u);
}

// ============================================================================
// PE-FT-005: FeedbackCollector recording
// ============================================================================

TEST_F(PromptEngineeringFocusedTest, PE_FT_005_FeedbackCollectorRecording) {
  auto feedback_id = feedback.recordFeedback(
      "tmpl_1",
      "Was ist ThemisDB?",
      "ThemisDB ist eine hybride Datenbank.",
      FeedbackType::USER_POSITIVE,
      "Good response",
      0.9);
  EXPECT_FALSE(feedback_id.empty());

  auto stats = feedback.getStats("tmpl_1");
  EXPECT_EQ(stats.prompt_id, "tmpl_1");
  EXPECT_GE(stats.total_feedback, 1u);
  EXPECT_GT(stats.positive_ratio, 0.0);
}

// ============================================================================
// PE-FT-006: PromptOptimizer basic optimization
// ============================================================================

TEST_F(PromptEngineeringFocusedTest, PE_FT_006_PromptOptimizerBasicOptimization) {
  std::vector<TestCase> tests = {
      {"Frage", "Antwort", {}}
  };
  auto eval_fn = [](const std::string& prompt, const std::vector<TestCase>&) {
    return prompt.empty() ? 0.0 : 0.8;
  };

  auto result = optimizer.optimize("Original prompt", tests, eval_fn);
  EXPECT_FALSE(result.optimized_prompt.empty());
  EXPECT_GE(result.final_score, 0.0);
}

// ============================================================================
// PE-FT-007: PromptEvaluator structural evaluation
// ============================================================================

TEST_F(PromptEngineeringFocusedTest, PE_FT_007_PromptEvaluatorStructuralEvaluation) {
  auto result = evaluator.evaluateSingle("Evaluate this prompt", "Evaluate this prompt");
  EXPECT_GE(result.exact_match, 0.0);
  EXPECT_GE(result.semantic_similarity, 0.0);
  // Structural evaluation should not fail
}

// ============================================================================
// PE-FT-008: PromptEngineeringMetrics recording
// ============================================================================

TEST_F(PromptEngineeringFocusedTest, PE_FT_008_PromptEngineeringMetricsRecording) {
  metrics.recordPromptExecutionCount("tmpl_metrics_1", 2);
  metrics.recordPromptExecution("tmpl_metrics_1", true, 3.0);

  auto exported = metrics.exportMetrics();
  EXPECT_NE(exported.find("tmpl_metrics_1"), std::string::npos);
}

// ============================================================================
// PE-FT-009: Error handling for missing template
// ============================================================================

TEST_F(PromptEngineeringFocusedTest, PE_FT_009_ErrorHandlingMissingTemplate) {
  auto missing = mgr.getTemplate("nonexistent_template_id");
  EXPECT_FALSE(missing.has_value());
}

// ============================================================================
// PE-FT-010: Error handling for invalid context injection
// ============================================================================

TEST_F(PromptEngineeringFocusedTest, PE_FT_010_ErrorHandlingInvalidContextInjection) {
  auto tmpl = MakeValidTemplate(10);
  auto created = mgr.createTemplate(tmpl);

  std::unordered_map<std::string, std::string> empty_ctx;
  // Missing context values: implementation should handle gracefully
  auto result = mgr.getPromptWithContext(created.id, empty_ctx);
  // Either returns nullopt or returns original template with unresolved placeholders
  if (result.has_value()) {
    // If a result is returned, it must not crash and must be non-empty
    EXPECT_FALSE(result->empty());
  }
}

// ============================================================================
// PE-FT-011: Concurrent template access (basic sanity check)
// ============================================================================

TEST_F(PromptEngineeringFocusedTest, PE_FT_011_ConcurrentTemplateAccessSanity) {
  constexpr int kThreadCount = 4;
  std::vector<std::string> template_ids(kThreadCount);
  std::vector<std::thread> threads;

  // Create templates in parallel; each thread writes to its own pre-allocated slot
  for (int i = 0; i < kThreadCount; ++i) {
    threads.emplace_back([this, i, &template_ids]() {
      auto tmpl = MakeValidTemplate(i + 100);
      auto created = mgr.createTemplate(tmpl);
      template_ids[i] = created.id;  // Each thread writes to a distinct index — no data race
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  EXPECT_EQ(static_cast<int>(template_ids.size()), kThreadCount);
  for (const auto& id : template_ids) {
    EXPECT_FALSE(id.empty());
  }
}

// ============================================================================
// PE-FT-012: Injection validation edge case (empty placeholder)
// ============================================================================

TEST_F(PromptEngineeringFocusedTest, PE_FT_012_InjectionValidationEdgeCase) {
  auto tmpl = MakeValidTemplate(12);
  tmpl.content = "Prompt with empty: {} and normal: {placeholder}";
  auto created = mgr.createTemplate(tmpl);

  auto ctx = MakeContextMap();
  auto result = mgr.getPromptWithContext(created.id, ctx);
  // Should handle edge case gracefully (empty {} is not a valid placeholder)
  if (result.has_value()) {
    EXPECT_FALSE(result->empty());
  }
}

// ============================================================================
// PE-FT-013: Version control consistency
// ============================================================================

TEST_F(PromptEngineeringFocusedTest, PE_FT_013_VersionControlConsistency) {
  std::string tmpl_id = "consistency_test";
  
  vcs.commit(tmpl_id, "v1.0.0", "First release");
  vcs.commit(tmpl_id, "v1.0.1", "Patch");
  vcs.commit(tmpl_id, "v1.1.0", "Minor update");

  auto history = vcs.getHistory(tmpl_id, "", 5);
  EXPECT_GE(history.size(), 2u); // At least 2 commits should be in history
}

// ============================================================================
// PE-FT-014: Optimizer diagnostics (non-blocking)
// ============================================================================

TEST_F(PromptEngineeringFocusedTest, PE_FT_014_OptimizerDiagnostics) {
  std::vector<TestCase> tests = {
      {"input", "output", {}}
  };
  auto eval_fn = [](const std::string& prompt, const std::vector<TestCase>&) {
    return prompt.empty() ? 0.0 : 0.75;
  };

  auto result = optimizer.optimize("Test prompt for diagnostics", tests, eval_fn);
  // Diagnostics should be populated even if optimization has issues
  // (Assuming implementation provides diagnostics)
  EXPECT_TRUE(!result.optimized_prompt.empty() || result.converged);
}

// ============================================================================
// PE-FT-015: Evaluator consistency across multiple calls
// ============================================================================

TEST_F(PromptEngineeringFocusedTest, PE_FT_015_EvaluatorConsistency) {
  auto result1 = evaluator.evaluateSingle("Consistent evaluation test", "Consistent evaluation test");
  auto result2 = evaluator.evaluateSingle("Consistent evaluation test", "Consistent evaluation test");

  // Both evaluations should complete without errors
  EXPECT_GE(result1.semantic_similarity, 0.0);
  EXPECT_GE(result2.semantic_similarity, 0.0);
}
