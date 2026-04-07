/**
 * @file test_multi_step_rag.cpp
 * @brief Unit tests for MultiStepRAGOrchestrator (Map-Reduce + Iterative).
 *
 * Test suite: MultiStepRAGFocusedTests (15 tests)
 *   Group A (5)  – Map-Reduce: single-pass, multi-batch, empty docs, no infer fn
 *   Group B (5)  – Iterative: single iteration, max_iterations cap, no retriever
 *   Group C (5)  – Configuration, factory helpers, context_overflow flag
 */

#include <gtest/gtest.h>
#include "rag/multi_step_rag.h"

using namespace themis::rag;

// ── Stub InferenceFn ──────────────────────────────────────────────────────────

static InferenceFn echoInfer(const std::string& prefix = "ANSWER") {
    return [prefix](const std::string& /*prompt*/, int /*max_tokens*/) -> std::string {
        return prefix;
    };
}

static InferenceFn noneGapInfer() {
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
