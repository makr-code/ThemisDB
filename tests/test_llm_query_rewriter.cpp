/**
 * @file test_llm_query_rewriter.cpp
 * @brief Unit tests for LlmQueryRewriter (v1.6.0)
 */

#include <gtest/gtest.h>
#include "search/llm_query_rewriter.h"
#include <stdexcept>
#include <string>
#include <vector>

using namespace themis;

// ============================================================================
// Config validation
// ============================================================================

TEST(LlmQueryRewriterConfig, DefaultConfigIsValid) {
    EXPECT_NO_THROW(LlmQueryRewriter{});
}

TEST(LlmQueryRewriterConfig, ZeroNumRewritesThrows) {
    LlmQueryRewriter::Config cfg;
    cfg.num_rewrites = 0;
    EXPECT_THROW(LlmQueryRewriter{cfg}, std::invalid_argument);
}

TEST(LlmQueryRewriterConfig, NegativeTemperatureThrows) {
    LlmQueryRewriter::Config cfg;
    cfg.temperature = -0.1f;
    EXPECT_THROW(LlmQueryRewriter{cfg}, std::invalid_argument);
}

TEST(LlmQueryRewriterConfig, TemperatureAboveTwoThrows) {
    LlmQueryRewriter::Config cfg;
    cfg.temperature = 2.1f;
    EXPECT_THROW(LlmQueryRewriter{cfg}, std::invalid_argument);
}

TEST(LlmQueryRewriterConfig, ConfigRoundtrip) {
    LlmQueryRewriter::Config cfg;
    cfg.num_rewrites = 5;
    cfg.max_tokens = 128;
    cfg.temperature = 0.5f;
    LlmQueryRewriter rw{cfg};
    EXPECT_EQ(rw.getConfig().num_rewrites, 5u);
    EXPECT_EQ(rw.getConfig().max_tokens, 128);
    EXPECT_FLOAT_EQ(rw.getConfig().temperature, 0.5f);
}

// ============================================================================
// rewrite() — no backend
// ============================================================================

TEST(LlmQueryRewriterNoBackend, EmptyQueryReturnsEmpty) {
    LlmQueryRewriter rw;
    auto result = rw.rewrite("");
    EXPECT_EQ(result.original, "");
    EXPECT_TRUE(result.rewrites.empty());
    EXPECT_FALSE(result.llm_used);
}

TEST(LlmQueryRewriterNoBackend, NullBackendFallsBackToOriginal) {
    LlmQueryRewriter rw;  // no backend
    auto result = rw.rewrite("fast database insert");
    EXPECT_EQ(result.original, "fast database insert");
    EXPECT_FALSE(result.llm_used);
    ASSERT_EQ(result.rewrites.size(), 1u);
    EXPECT_EQ(result.rewrites[0], "fast database insert");
}

TEST(LlmQueryRewriterNoBackend, FallbackDisabledYieldsNoRewrites) {
    LlmQueryRewriter::Config cfg;
    cfg.fallback_to_original = false;
    LlmQueryRewriter rw{cfg};
    auto result = rw.rewrite("some query");
    EXPECT_TRUE(result.rewrites.empty());
    EXPECT_FALSE(result.llm_used);
}

// ============================================================================
// rewrite() — with mock backend
// ============================================================================

static LlmQueryRewriter::LlmBackend makeMockBackend(const std::string& response) {
    return [response](const std::string& /*prompt*/) { return response; };
}

TEST(LlmQueryRewriterWithBackend, ParsesNumberedLines) {
    const std::string mock_response =
        "1. rapid database write\n"
        "2. high-throughput record insertion\n"
        "3. quick data persistence\n";
    LlmQueryRewriter rw{{}, makeMockBackend(mock_response)};
    auto result = rw.rewrite("fast db insert");

    EXPECT_TRUE(result.llm_used);
    EXPECT_EQ(result.original, "fast db insert");
    ASSERT_EQ(result.rewrites.size(), 3u);
    EXPECT_EQ(result.rewrites[0], "rapid database write");
    EXPECT_EQ(result.rewrites[1], "high-throughput record insertion");
    EXPECT_EQ(result.rewrites[2], "quick data persistence");
}

TEST(LlmQueryRewriterWithBackend, LimitsToNumRewrites) {
    const std::string mock_response =
        "1. rewrite one\n"
        "2. rewrite two\n"
        "3. rewrite three\n"
        "4. rewrite four\n"
        "5. rewrite five\n";
    LlmQueryRewriter::Config cfg;
    cfg.num_rewrites = 2;
    LlmQueryRewriter rw{cfg, makeMockBackend(mock_response)};
    auto result = rw.rewrite("query");
    EXPECT_LE(result.rewrites.size(), 2u);
}

TEST(LlmQueryRewriterWithBackend, DeduplicatesRewrites) {
    const std::string mock_response =
        "1. machine learning query\n"
        "2. machine learning query\n"   // exact duplicate
        "3. MACHINE LEARNING QUERY\n";  // case-insensitive duplicate
    LlmQueryRewriter rw{{}, makeMockBackend(mock_response)};
    auto result = rw.rewrite("ml search");

    // All three lines are the same (case-insensitively), so exactly one survives
    ASSERT_EQ(result.rewrites.size(), 1u);
    EXPECT_EQ(result.rewrites[0], "machine learning query");
}

TEST(LlmQueryRewriterWithBackend, DropsSameAsOriginal) {
    // LLM returns the original query verbatim — should be filtered out
    LlmQueryRewriter rw{{}, makeMockBackend("1. my exact query\n2. different phrasing\n")};
    auto result = rw.rewrite("my exact query");

    for (const auto& r : result.rewrites) {
        EXPECT_NE(r, "my exact query");
    }
}

TEST(LlmQueryRewriterWithBackend, HandlesParenthesesNumbering) {
    LlmQueryRewriter rw{{}, makeMockBackend("1) first rewrite\n2) second rewrite\n")};
    auto result = rw.rewrite("query");
    ASSERT_GE(result.rewrites.size(), 1u);
    EXPECT_EQ(result.rewrites[0], "first rewrite");
}

TEST(LlmQueryRewriterWithBackend, HandlesColonNumbering) {
    LlmQueryRewriter rw{{}, makeMockBackend("1: first rewrite\n2: second rewrite\n")};
    auto result = rw.rewrite("query");
    ASSERT_GE(result.rewrites.size(), 1u);
    EXPECT_EQ(result.rewrites[0], "first rewrite");
}

TEST(LlmQueryRewriterWithBackend, EmptyLlmResponseFallsBack) {
    LlmQueryRewriter rw{{}, makeMockBackend("")};
    auto result = rw.rewrite("some query");

    // LLM was called but returned nothing; fallback to original is on by default
    EXPECT_TRUE(result.llm_used);
    ASSERT_EQ(result.rewrites.size(), 1u);
    EXPECT_EQ(result.rewrites[0], "some query");
}

TEST(LlmQueryRewriterWithBackend, EmptyLlmResponseNoFallback) {
    LlmQueryRewriter::Config cfg;
    cfg.fallback_to_original = false;
    LlmQueryRewriter rw{cfg, makeMockBackend("")};
    auto result = rw.rewrite("some query");
    EXPECT_TRUE(result.llm_used);
    EXPECT_TRUE(result.rewrites.empty());
}

TEST(LlmQueryRewriterWithBackend, SkipsLinesExceedingMaxLength) {
    LlmQueryRewriter::Config cfg;
    cfg.max_rewrite_length = 20;  // short limit
    std::string long_line(30, 'a');  // 30 chars — too long
    const std::string mock_response = "1. short rewrite\n2. " + long_line + "\n";
    LlmQueryRewriter rw{cfg, makeMockBackend(mock_response)};
    auto result = rw.rewrite("query");

    for (const auto& r : result.rewrites) {
        EXPECT_LE(r.size(), 20u);
    }
}

TEST(LlmQueryRewriterWithBackend, BackendExceptionTriggersFallback) {
    LlmQueryRewriter::LlmBackend throwing_backend = [](const std::string&) -> std::string {
        throw std::runtime_error("LLM unavailable");
    };
    LlmQueryRewriter rw{{}, throwing_backend};
    auto result = rw.rewrite("test query");

    EXPECT_FALSE(result.llm_used);
    ASSERT_EQ(result.rewrites.size(), 1u);
    EXPECT_EQ(result.rewrites[0], "test query");
}

TEST(LlmQueryRewriterWithBackend, BackendExceptionNoFallback) {
    LlmQueryRewriter::Config cfg;
    cfg.fallback_to_original = false;
    LlmQueryRewriter::LlmBackend throwing_backend = [](const std::string&) -> std::string {
        throw std::runtime_error("LLM error");
    };
    LlmQueryRewriter rw{cfg, throwing_backend};
    auto result = rw.rewrite("test query");

    EXPECT_FALSE(result.llm_used);
    EXPECT_TRUE(result.rewrites.empty());
}

// ============================================================================
// setBackend() — dynamic backend replacement
// ============================================================================

TEST(LlmQueryRewriterSetBackend, ReplacesNullBackend) {
    LlmQueryRewriter rw;  // no backend
    rw.setBackend(makeMockBackend("1. better phrasing\n"));
    auto result = rw.rewrite("original query");
    EXPECT_TRUE(result.llm_used);
    ASSERT_EQ(result.rewrites.size(), 1u);
    EXPECT_EQ(result.rewrites[0], "better phrasing");
}

TEST(LlmQueryRewriterSetBackend, ClearingBackendFallsBack) {
    LlmQueryRewriter rw{{}, makeMockBackend("1. rewrite\n")};
    rw.setBackend(nullptr);  // remove backend
    auto result = rw.rewrite("query");
    EXPECT_FALSE(result.llm_used);
    // fallback_to_original is true by default
    ASSERT_EQ(result.rewrites.size(), 1u);
    EXPECT_EQ(result.rewrites[0], "query");
}

// ============================================================================
// OriginalPreserved — original field is always the verbatim input
// ============================================================================

TEST(LlmQueryRewriterOriginal, OriginalFieldAlwaysPreserved) {
    const std::string query = "Find all records with status=active AND region=EU";
    LlmQueryRewriter rw{{}, makeMockBackend("1. active EU records\n")};
    auto result = rw.rewrite(query);
    EXPECT_EQ(result.original, query);
}

// ============================================================================
// Blank-line and whitespace-only lines in LLM output are ignored
// ============================================================================

TEST(LlmQueryRewriterParsing, BlankLinesSkipped) {
    const std::string mock_response =
        "\n"
        "1. first rewrite\n"
        "\n"
        "2. second rewrite\n"
        "\n";
    LlmQueryRewriter rw{{}, makeMockBackend(mock_response)};
    auto result = rw.rewrite("test");
    ASSERT_EQ(result.rewrites.size(), 2u);
    EXPECT_EQ(result.rewrites[0], "first rewrite");
    EXPECT_EQ(result.rewrites[1], "second rewrite");
}

// ============================================================================
// buildPrompt() — prompt content validation
// ============================================================================

TEST(LlmQueryRewriterPrompt, PromptContainsQuery) {
    std::string captured_prompt = {};
    LlmQueryRewriter rw{{}, [&](const std::string& prompt) {
        captured_prompt = prompt;
        return "1. a rewrite\n";
    }};
    rw.rewrite("database indexing performance");
    EXPECT_NE(captured_prompt.find("database indexing performance"), std::string::npos);
}

TEST(LlmQueryRewriterPrompt, PromptContainsNumRewrites) {
    std::string captured_prompt = {};
    LlmQueryRewriter::Config cfg;
    cfg.num_rewrites = 5;
    LlmQueryRewriter rw{cfg, [&](const std::string& prompt) {
        captured_prompt = prompt;
        return "1. a rewrite\n";
    }};
    rw.rewrite("query");
    // The prompt should embed num_rewrites in context, e.g. "5 alternative phrasings"
    EXPECT_NE(captured_prompt.find("5 alternative"), std::string::npos);
}

TEST(LlmQueryRewriterPrompt, PromptContainsVocabularyStrategyGuidance) {
    std::string captured_prompt = {};
    LlmQueryRewriter rw{{}, [&](const std::string& prompt) {
        captured_prompt = prompt;
        return "1. a rewrite\n";
    }};
    rw.rewrite("fast db insert");
    // Prompt must instruct the LLM to apply distinct vocabulary strategies
    // so that rewrites cover different vocabulary spaces and improve recall.
    EXPECT_NE(captured_prompt.find("vocabulary"), std::string::npos);
    EXPECT_NE(captured_prompt.find("synonyms"), std::string::npos);
    EXPECT_NE(captured_prompt.find("abbreviations"), std::string::npos);
}

TEST(LlmQueryRewriterPrompt, PromptContainsNumberedLineInstruction) {
    std::string captured_prompt = {};
    LlmQueryRewriter rw{{}, [&](const std::string& prompt) {
        captured_prompt = prompt;
        return "1. a rewrite\n";
    }};
    rw.rewrite("test query");
    // Prompt must explicitly show the numbered-line format, e.g. "1. rewrite here"
    EXPECT_NE(captured_prompt.find("1. rewrite here"), std::string::npos);
}

TEST(LlmQueryRewriterPrompt, PromptContainsTemperatureHint) {
    std::string captured_prompt = {};
    LlmQueryRewriter::Config cfg;
    cfg.temperature = 0.3f;
    LlmQueryRewriter rw{cfg, [&](const std::string& prompt) {
        captured_prompt = prompt;
        return "1. a rewrite\n";
    }};
    rw.rewrite("test query");
    // Prompt must embed the temperature hint so backends can honour it
    EXPECT_NE(captured_prompt.find("temperature"), std::string::npos);
}
