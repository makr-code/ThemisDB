/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_rag_batch_evaluator.cpp                       ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-04-15 18:17:06                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     322                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 6efaebce20  2026-03-09  feat(rag): implement BatchEvaluator, CalibrationManager, ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_rag_batch_evaluator.cpp
 * @brief Unit tests for BatchEvaluator
 *
 * Tests cover:
 *  - Default construction
 *  - Custom config stored correctly
 *  - evaluateBatch(EvaluationInputs) returns aggregated statistics
 *  - evaluateBatch(RAGTestCases) delegates to EvaluationInput overload
 *  - Average scores are in [0, 1]
 *  - passed_quality_threshold counting
 *  - evaluateAsync returns usable handle; get() returns result
 *  - submit() + waitForAll() processes queued evaluations
 *  - getQueueSize() reflects pending items
 *  - stop() + resume() restarts workers
 *  - Progress callback invoked during synchronous batch
 *  - setConfig / getConfig round-trip
 *  - Empty input returns zero-item result
 */

#include "rag/batch_evaluator.h"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <vector>

using namespace themis::rag::judge;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static std::shared_ptr<RAGJudge> makeJudge() {
    RAGJudgeConfig cfg;
    cfg.mode = EvaluationMode::FAST;
    cfg.enable_ethical_evaluation = false;
    cfg.use_nli_verifier           = false;
    cfg.use_geval_scoring          = false;
    cfg.cache_evaluations          = false;
    return std::make_shared<RAGJudge>(cfg);
}

static EvaluationInput makeInput(const std::string& query = "What is the capital of France?",
                                 const std::string& answer = "Paris") {
    EvaluationInput in;
    in.query            = query;
    in.generated_answer = answer;
    RetrievedDocument doc;
    doc.id               = "doc1";
    doc.content          = "Paris is the capital of France.";
    doc.similarity_score = 0.95;
    in.documents.push_back(doc);
    return in;
}

static RAGTestCase makeTestCase(const std::string& id = "tc1") {
    RAGTestCase tc;
    tc.test_id           = id;
    tc.query             = "What is the capital of France?";
    tc.generated_answer  = "Paris";
    RetrievedDocument doc;
    doc.id               = "doc1";
    doc.content          = "Paris is the capital of France.";
    doc.similarity_score = 0.95;
    tc.documents.push_back(doc);
    return tc;
}

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────

TEST(BatchEvaluatorTest, DefaultConstruction) {
    auto judge = makeJudge();
    BatchEvaluator eval(judge);
    auto cfg = eval.getConfig();
    EXPECT_EQ(cfg.batch_size,   8u);
    EXPECT_EQ(cfg.num_workers,  4u);
}

TEST(BatchEvaluatorTest, CustomConfigStored) {
    auto judge = makeJudge();
    BatchEvaluatorConfig cfg;
    cfg.batch_size  = 16;
    cfg.num_workers = 2;
    BatchEvaluator eval(judge, cfg);
    auto got = eval.getConfig();
    EXPECT_EQ(got.batch_size,  16u);
    EXPECT_EQ(got.num_workers,  2u);
}

// ─────────────────────────────────────────────────────────────────────────────
// evaluateBatch(EvaluationInputs)
// ─────────────────────────────────────────────────────────────────────────────

TEST(BatchEvaluatorTest, EvaluateBatchInputsReturnsResults) {
    auto judge = makeJudge();
    BatchEvaluatorConfig cfg;
    cfg.num_workers = 1;
    BatchEvaluator eval(judge, cfg);

    std::vector<EvaluationInput> inputs = {makeInput(), makeInput()};
    auto result = eval.evaluateBatch(inputs);

    EXPECT_EQ(result.results.size(), 2u);
    EXPECT_EQ(result.progress.total_items, 2u);
    EXPECT_DOUBLE_EQ(result.progress.progress_percentage, 100.0);
}

TEST(BatchEvaluatorTest, AverageScoresInRange) {
    auto judge = makeJudge();
    BatchEvaluatorConfig cfg;
    cfg.num_workers = 1;
    BatchEvaluator eval(judge, cfg);

    std::vector<EvaluationInput> inputs = {makeInput(), makeInput(), makeInput()};
    auto result = eval.evaluateBatch(inputs);

    EXPECT_GE(result.average_faithfulness,  0.0);
    EXPECT_LE(result.average_faithfulness,  1.0);
    EXPECT_GE(result.average_relevance,     0.0);
    EXPECT_LE(result.average_relevance,     1.0);
    EXPECT_GE(result.average_overall_score, 0.0);
    EXPECT_LE(result.average_overall_score, 1.0);
}

TEST(BatchEvaluatorTest, PassedFailedCountsAddUp) {
    auto judge = makeJudge();
    BatchEvaluatorConfig cfg;
    cfg.num_workers = 1;
    BatchEvaluator eval(judge, cfg);

    std::vector<EvaluationInput> inputs = {makeInput(), makeInput()};
    auto result = eval.evaluateBatch(inputs);

    EXPECT_EQ(result.passed_quality_threshold + result.failed_quality_threshold,
              result.results.size());
}

TEST(BatchEvaluatorTest, EmptyInputReturnsEmptyResult) {
    auto judge = makeJudge();
    BatchEvaluator eval(judge);
    auto result = eval.evaluateBatch(std::vector<EvaluationInput>{});
    EXPECT_TRUE(result.results.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// evaluateBatch(RAGTestCases)
// ─────────────────────────────────────────────────────────────────────────────

TEST(BatchEvaluatorTest, EvaluateBatchTestCases) {
    auto judge = makeJudge();
    BatchEvaluatorConfig cfg;
    cfg.num_workers = 1;
    BatchEvaluator eval(judge, cfg);

    std::vector<RAGTestCase> cases = {makeTestCase("tc1"), makeTestCase("tc2")};
    auto result = eval.evaluateBatch(cases);

    EXPECT_EQ(result.results.size(), 2u);
}

// ─────────────────────────────────────────────────────────────────────────────
// evaluateAsync
// ─────────────────────────────────────────────────────────────────────────────

TEST(BatchEvaluatorTest, EvaluateAsyncReturnsHandle) {
    auto judge = makeJudge();
    BatchEvaluatorConfig cfg;
    cfg.num_workers = 2;
    BatchEvaluator eval(judge, cfg);

    auto handle = eval.evaluateAsync(makeInput());
    ASSERT_NE(handle, nullptr);

    // Wait at most 5 seconds
    bool done = handle->wait(std::chrono::milliseconds(5000));
    EXPECT_TRUE(done);

    auto result = handle->get();
    EXPECT_GE(result.overall_score, 0.0);
    EXPECT_LE(result.overall_score, 1.0);
}

TEST(BatchEvaluatorTest, EvaluateAsyncMultipleHandles) {
    auto judge = makeJudge();
    BatchEvaluatorConfig cfg;
    cfg.num_workers = 2;
    BatchEvaluator eval(judge, cfg);

    std::vector<EvaluationInput> inputs = {makeInput(), makeInput(), makeInput()};
    auto handles = eval.evaluateAsync(inputs);

    EXPECT_EQ(handles.size(), 3u);
    for (auto& h : handles) {
        ASSERT_NE(h, nullptr);
        bool done = h->wait(std::chrono::milliseconds(5000));
        EXPECT_TRUE(done);
    }
}

TEST(BatchEvaluatorTest, CancelledHandleThrows) {
    auto judge = makeJudge();
    BatchEvaluatorConfig cfg;
    cfg.num_workers = 1;
    BatchEvaluator eval(judge, cfg);

    auto handle = eval.evaluateAsync(makeInput());
    handle->cancel();
    EXPECT_TRUE(handle->isDone());
    EXPECT_THROW(handle->get(), std::runtime_error);
}

// ─────────────────────────────────────────────────────────────────────────────
// submit() + waitForAll()
// ─────────────────────────────────────────────────────────────────────────────

TEST(BatchEvaluatorTest, SubmitAndWaitForAll) {
    auto judge = makeJudge();
    BatchEvaluatorConfig cfg;
    cfg.num_workers = 2;
    BatchEvaluator eval(judge, cfg);

    std::atomic<int> callback_count{0};
    for (int i = 0; i < 4; ++i) {
        eval.submit(makeInput(), [&](const EvaluationResult&) {
            ++callback_count;
        });
    }

    bool all_done = eval.waitForAll(std::chrono::milliseconds(10000));
    EXPECT_TRUE(all_done);
    EXPECT_EQ(callback_count.load(), 4);
}

// ─────────────────────────────────────────────────────────────────────────────
// Progress callback
// ─────────────────────────────────────────────────────────────────────────────

TEST(BatchEvaluatorTest, ProgressCallbackInvoked) {
    auto judge = makeJudge();
    BatchEvaluatorConfig cfg;
    cfg.num_workers            = 1;
    cfg.enable_progress_tracking = true;

    std::atomic<int> progress_calls{0};
    cfg.progress_callback = [&](const BatchProgress& p) {
        ++progress_calls;
        EXPECT_LE(p.progress_percentage, 100.0);
        EXPECT_GE(p.progress_percentage, 0.0);
    };

    BatchEvaluator eval(judge, cfg);
    std::vector<EvaluationInput> inputs = {makeInput(), makeInput(), makeInput()};
    eval.evaluateBatch(inputs);

    EXPECT_GT(progress_calls.load(), 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// setConfig / getConfig
// ─────────────────────────────────────────────────────────────────────────────

TEST(BatchEvaluatorTest, SetGetConfig) {
    auto judge = makeJudge();
    BatchEvaluator eval(judge);

    BatchEvaluatorConfig cfg;
    cfg.batch_size  = 32;
    cfg.num_workers = 8;
    cfg.fail_fast   = true;
    eval.setConfig(cfg);

    auto got = eval.getConfig();
    EXPECT_EQ(got.batch_size,  32u);
    EXPECT_EQ(got.num_workers,  8u);
    EXPECT_TRUE(got.fail_fast);
}

// ─────────────────────────────────────────────────────────────────────────────
// stop() + resume()
// ─────────────────────────────────────────────────────────────────────────────

TEST(BatchEvaluatorTest, StopAndResume) {
    auto judge = makeJudge();
    BatchEvaluatorConfig cfg;
    cfg.num_workers = 2;
    BatchEvaluator eval(judge, cfg);

    eval.stop();
    // After stop, we can resume and evaluate again
    eval.resume();

    auto result = eval.evaluateBatch(std::vector<EvaluationInput>{makeInput()});
    EXPECT_EQ(result.results.size(), 1u);
}
