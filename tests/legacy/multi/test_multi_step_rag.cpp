/**
 * @file test_multi_step_rag.cpp
 * @brief Unit tests for MultiStepRAGOrchestrator (Map-Reduce + Iterative).
 *
 * Test suite: MultiStepRAGFocusedTests (16 tests)
 *   Group A (5)  – Map-Reduce: single-pass, multi-batch, empty docs, no infer fn
 *   Group B (5)  – Iterative: single iteration, max_iterations cap, no retriever
 *   Group C (6)  – Configuration, factory helpers, context_overflow flag
 */

#include <gtest/gtest.h>
#include "rag/multi_step_rag.h"
#include "llm/context_window_budget.h"

using namespace themis::rag;

// ── Stub InferenceFn ──────────────────────────────────────────────────────────

static InferenceFn echoInfer(const std::string& prefix = "ANSWER") {
    return [prefix](const std::string& /*prompt*/, int /*max_tokens*/) -> std::string {
        return prefix;
    };
}

[[maybe_unused]] static InferenceFn noneGapInfer() {
    // Always says "NONE" when called (no open aspects).
    return [](const std::string& /*prompt*/, int /*max_tokens*/) -> std::string {
        return "NONE";
    };
}

// Helper: make a chunk with given content and relevance.
static RetrievedChunk makeChunk(const std::string& content,
                                 float              score  = 1.0f,
                                 const std::string& source = "")
{
    RetrievedChunk c;
    c.content         = content;
    c.relevance_score = score;
    c.source          = source;
    return c;
}

// ── Group A – Map-Reduce ──────────────────────────────────────────────────────

TEST(MultiStepRAGFocusedTests, A1_SinglePassWhenDocsFit) {
    MultiStepRAGConfig cfg;
    cfg.assembler.model_context_tokens = 4096u;
    cfg.assembler.min_response_tokens  = 512u;
    MultiStepRAGOrchestrator orch(cfg);

    const auto result = orch.runMapReduce("query", {makeChunk("short doc")}, echoInfer("RESULT"));
    EXPECT_EQ("RESULT", result.final_answer);
    EXPECT_FALSE(result.context_overflow);
    EXPECT_EQ(1u, result.steps_executed);
}

TEST(MultiStepRAGFocusedTests, A2_EmptyDocumentsReturnsEmpty) {
    MultiStepRAGOrchestrator orch{};
    const auto result = orch.runMapReduce("q", {}, echoInfer());
    EXPECT_TRUE(result.final_answer.empty());
    EXPECT_EQ(0u, result.steps_executed);
}

TEST(MultiStepRAGFocusedTests, A3_NullInferFnReturnsEmpty) {
    MultiStepRAGOrchestrator orch{};
    const auto result = orch.runMapReduce("q", {makeChunk("doc")}, {});
    EXPECT_TRUE(result.final_answer.empty());
}

TEST(MultiStepRAGFocusedTests, A4_MultipleMapBatchesProduceOverflowFlag) {
    MultiStepRAGConfig cfg;
    cfg.assembler.model_context_tokens = 20u;  // tiny window → forces batching
    cfg.assembler.min_response_tokens  = 5u;
    cfg.max_map_steps                  = 4u;
    cfg.max_response_tokens            = 10;
    MultiStepRAGOrchestrator orch(cfg);

    // Large documents that won't fit in a 20-token window.
    std::vector<RetrievedChunk> docs;
    for (int i = 0; i < 4; ++i) {
        docs.push_back(makeChunk(std::string(200, static_cast<char>('a' + i))));
    }

    int infer_count = 0;
    InferenceFn infer = [&](const std::string&, int) -> std::string {
        ++infer_count;
        return "partial_" + std::to_string(infer_count);
    };

    const auto result = orch.runMapReduce("query", docs, infer);
    EXPECT_TRUE(result.context_overflow);
    EXPECT_GT(result.steps_executed, 1u);
    EXPECT_FALSE(result.final_answer.empty());
}

TEST(MultiStepRAGFocusedTests, A5_SingleBatchNoReduceStep) {
    // When only one batch is produced after partitioning, no reduce step is run.
    MultiStepRAGConfig cfg;
    cfg.assembler.model_context_tokens = 4096u;
    cfg.assembler.min_response_tokens  = 512u;
    MultiStepRAGOrchestrator orch(cfg);

    int call_count = 0;
    InferenceFn infer = [&](const std::string&, int) -> std::string {
        return "answer_" + std::to_string(++call_count);
    };
    const auto result = orch.runMapReduce("q", {makeChunk("doc1"), makeChunk("doc2")}, infer);
    // Should be a single-pass (both docs fit), so only 1 infer call.
    EXPECT_EQ(1, call_count);
    EXPECT_EQ(1u, result.steps_executed);
}

// ── Group B – Iterative ───────────────────────────────────────────────────────

TEST(MultiStepRAGFocusedTests, B1_SingleIterationWithNoneGap) {
    MultiStepRAGConfig cfg;
    cfg.assembler.model_context_tokens = 4096u;
    cfg.assembler.min_response_tokens  = 512u;
    cfg.max_iterations                 = 3u;
    MultiStepRAGOrchestrator orch(cfg);

    // Returns "NONE" for gap detection → stops after first answer generation.
    // infer is called: (1) for the answer, (2) for gap detection → NONE → stop.
    int calls = 0;
    InferenceFn infer = [&](const std::string&, int) -> std::string {
        ++calls;
        return (calls == 1) ? "The answer" : "NONE";
    };

    const auto result = orch.runIterative("q", {makeChunk("doc")}, infer,
        [](const std::string&, size_t) { return std::vector<RetrievedChunk>{}; });
    EXPECT_FALSE(result.final_answer.empty());
    EXPECT_GE(result.steps_executed, 1u);
}

TEST(MultiStepRAGFocusedTests, B2_MaxIterationsCaps) {
    MultiStepRAGConfig cfg;
    cfg.assembler.model_context_tokens = 4096u;
    cfg.assembler.min_response_tokens  = 512u;
    cfg.max_iterations                 = 2u;
    cfg.retrieval_top_k                = 2u;
    MultiStepRAGOrchestrator orch(cfg);

    // Gap detector always returns open aspects so we rely on max_iterations.
    InferenceFn infer = [](const std::string&, int) -> std::string {
        return "open aspect\nanother aspect";
    };
    // Retriever returns new docs every time.
    int retrieve_count = 0;
    RetrievalFn retrieve = [&](const std::string&, size_t k) -> std::vector<RetrievedChunk> {
        ++retrieve_count;
        std::vector<RetrievedChunk> docs;
        for (size_t i = 0; i < k; ++i) {
            docs.push_back(makeChunk("new_doc_" + std::to_string(retrieve_count),
                                      1.0f,
                                      "src_" + std::to_string(retrieve_count) + "_" + std::to_string(i)));
        }
        return docs;
    };

    const auto result = orch.runIterative("q", {makeChunk("initial")}, infer, retrieve);
    // max_iterations = 2, so no more than 2 full answer+gap cycles.
    EXPECT_LE(result.steps_executed, cfg.max_iterations * 2 + 1);
}

TEST(MultiStepRAGFocusedTests, B3_NoRetrieverBreaksAfterFirstIteration) {
    MultiStepRAGConfig cfg;
    cfg.assembler.model_context_tokens = 4096u;
    cfg.max_iterations                 = 5u;
    MultiStepRAGOrchestrator orch(cfg);

    int call_count = 0;
    InferenceFn infer = [&](const std::string&, int) -> std::string {
        ++call_count;
        return "answer";
    };

    // No retriever → should stop after the first inference call.
    const auto result = orch.runIterative("q", {makeChunk("doc")}, infer, {});
    EXPECT_EQ(1, call_count);
    EXPECT_EQ(1u, result.steps_executed);
}

TEST(MultiStepRAGFocusedTests, B4_ResultContainsFinalAnswer) {
    MultiStepRAGOrchestrator orch{};
    const auto result = orch.runIterative("q", {makeChunk("doc")}, echoInfer("MY_ANSWER"), {});
    EXPECT_EQ("MY_ANSWER", result.final_answer);
}

TEST(MultiStepRAGFocusedTests, B5_NullInferFnReturnsEmpty) {
    MultiStepRAGOrchestrator orch{};
    const auto result = orch.runIterative("q", {makeChunk("doc")}, {});
    EXPECT_TRUE(result.final_answer.empty());
}

TEST(MultiStepRAGFocusedTests, B6_IterativeUsesBudgetCappedTokensForAnswerAndGapDetection) {
    MultiStepRAGConfig cfg;
    cfg.assembler.model_context_tokens = 24u;
    cfg.assembler.min_response_tokens  = 5u;
    cfg.max_response_tokens            = 1000; // must be capped by computed budget
    cfg.max_iterations                 = 1u;
    MultiStepRAGOrchestrator orch(cfg);

    const auto budget = ::themis::llm::ContextWindowBudget::compute(
        cfg.assembler.model_context_tokens,
        cfg.system_prompt,
        "query",
        cfg.assembler.min_response_tokens);
    const int expected_answer_max_tokens =
        RAGContextAssembler::computeMaxTokens(budget, cfg.max_response_tokens);
    const int expected_gap_max_tokens = std::max(1, std::min(256, expected_answer_max_tokens));

    std::vector<int> observed_max_tokens;
    int call_index = 0;
    InferenceFn infer = [&](const std::string&, int max_tokens) -> std::string {
        observed_max_tokens.push_back(max_tokens);
        ++call_index;
        return (call_index == 1) ? "iterative-answer" : "NONE";
    };

    RetrievalFn retrieve = [](const std::string&, size_t) -> std::vector<RetrievedChunk> {
        return {};
    };

    const auto result = orch.runIterative("query", {makeChunk("doc")}, infer, retrieve);
    EXPECT_EQ("iterative-answer", result.final_answer);
    ASSERT_EQ(observed_max_tokens.size(), 2u);
    EXPECT_EQ(expected_answer_max_tokens, observed_max_tokens[0]);
    EXPECT_EQ(expected_gap_max_tokens, observed_max_tokens[1]);
}

// ── Group C – Configuration and factory helpers ───────────────────────────────

TEST(MultiStepRAGFocusedTests, C1_GetAndSetConfig) {
    MultiStepRAGOrchestrator orch{};
    MultiStepRAGConfig cfg;
    cfg.max_iterations = 7u;
    orch.setConfig(cfg);
    EXPECT_EQ(7u, orch.getConfig().max_iterations);
}

TEST(MultiStepRAGFocusedTests, C2_FactorySmallContextCreated) {
    const auto orch = MultiStepRAGFactory::createSmallContext();
    ASSERT_NE(nullptr, orch);
    EXPECT_EQ(4096u, orch->getConfig().assembler.model_context_tokens);
    EXPECT_EQ(3u, orch->getConfig().max_map_steps);
}

TEST(MultiStepRAGFocusedTests, C3_FactoryMediumContextCreated) {
    const auto orch = MultiStepRAGFactory::createMediumContext();
    ASSERT_NE(nullptr, orch);
    EXPECT_EQ(8192u, orch->getConfig().assembler.model_context_tokens);
}

TEST(MultiStepRAGFocusedTests, C4_FactoryLargeContextCreated) {
    const auto orch = MultiStepRAGFactory::createLargeContext();
    ASSERT_NE(nullptr, orch);
    EXPECT_EQ(32768u, orch->getConfig().assembler.model_context_tokens);
    EXPECT_EQ(8u, orch->getConfig().max_map_steps);
}

TEST(MultiStepRAGFocusedTests, C5_CustomFactoryConfig) {
    MultiStepRAGConfig cfg;
    cfg.assembler.model_context_tokens = 16384u;
    cfg.max_iterations                 = 10u;
    const auto orch = MultiStepRAGFactory::create(cfg);
    ASSERT_NE(nullptr, orch);
    EXPECT_EQ(16384u, orch->getConfig().assembler.model_context_tokens);
    EXPECT_EQ(10u,    orch->getConfig().max_iterations);
}

TEST(MultiStepRAGFocusedTests, C6_InvalidBudgetConfigIsSanitizedAtIngress) {
    MultiStepRAGConfig cfg;
    cfg.assembler.model_context_tokens = 0u;
    cfg.assembler.min_response_tokens  = 0u;
    cfg.max_response_tokens            = 0;
    cfg.max_map_steps                  = 0u;

    MultiStepRAGOrchestrator orch(cfg);
    const auto safe = orch.getConfig();

    EXPECT_EQ(4096u, safe.assembler.model_context_tokens);
    EXPECT_EQ(512u,  safe.assembler.min_response_tokens);
    EXPECT_EQ(1,     safe.max_response_tokens);
    EXPECT_EQ(1u,    safe.max_map_steps);

    MultiStepRAGConfig cfg2;
    cfg2.assembler.model_context_tokens = 32u;
    cfg2.assembler.min_response_tokens  = 999u;
    cfg2.max_response_tokens            = -7;
    cfg2.max_map_steps                  = 0u;
    orch.setConfig(cfg2);

    const auto safe2 = orch.getConfig();
    EXPECT_EQ(32u, safe2.assembler.model_context_tokens);
    EXPECT_EQ(32u, safe2.assembler.min_response_tokens);
    EXPECT_EQ(1,   safe2.max_response_tokens);
    EXPECT_EQ(1u,  safe2.max_map_steps);

    int observed_max_tokens = -1;
    InferenceFn infer = [&](const std::string&, int max_tokens) -> std::string {
        observed_max_tokens = max_tokens;
        return "sanitized";
    };

    const auto result = orch.runMapReduce("q", {makeChunk("doc")}, infer);
    EXPECT_EQ("sanitized", result.final_answer);
    EXPECT_EQ(1, observed_max_tokens);
}

TEST(MultiStepRAGFocusedTests, C7_MapReduceUsesBudgetCappedMaxTokensAcrossPhases) {
    MultiStepRAGConfig cfg;
    cfg.assembler.model_context_tokens = 20u;  // force overflow + batching
    cfg.assembler.min_response_tokens  = 5u;
    cfg.max_response_tokens            = 1000; // intentionally high, must be capped
    cfg.max_map_steps                  = 4u;
    cfg.enable_parallel_map            = false;

    MultiStepRAGOrchestrator orch(cfg);

    std::vector<RetrievedChunk> docs;
    for (int i = 0; i < 4; ++i) {
        docs.push_back(makeChunk(std::string(180, static_cast<char>('a' + i)), 1.0f, "src"));
    }

    const auto budget = ::themis::llm::ContextWindowBudget::compute(
        cfg.assembler.model_context_tokens,
        cfg.system_prompt,
        "query",
        cfg.assembler.min_response_tokens);
    const int expected_max_tokens =
        RAGContextAssembler::computeMaxTokens(budget, cfg.max_response_tokens);

    std::vector<int> observed_max_tokens;
    InferenceFn infer = [&](const std::string&, int max_tokens) -> std::string {
        observed_max_tokens.push_back(max_tokens);
        return "partial";
    };

    const auto result = orch.runMapReduce("query", docs, infer);
    EXPECT_TRUE(result.context_overflow);
    ASSERT_GE(observed_max_tokens.size(), 2u);
    for (int observed : observed_max_tokens) {
        EXPECT_EQ(expected_max_tokens, observed);
    }
}
