#include <gtest/gtest.h>
#include "prompt_engineering/rag_prompt_builder.h"

using namespace themis::prompt_engineering;

// ============================================================================
// Helpers
// ============================================================================

static std::vector<RetrievedChunk> makeChunks(size_t n = 3) {
    std::vector<RetrievedChunk> chunks;
    for (size_t i = 0; i < n; ++i) {
        RetrievedChunk c;
        c.content         = "Content of chunk " + std::to_string(i + 1) + ".";
        c.source          = "doc_" + std::to_string(i + 1) + ".pdf";
        c.relevance_score = 1.0 - (double(i) * 0.1);
        c.chunk_id        = "cid_" + std::to_string(i + 1);
        chunks.push_back(c);
    }
    return chunks;
}

// ============================================================================
// Configuration & construction
// ============================================================================

TEST(RAGPromptBuilderTest, DefaultConfigIsAccessible) {
    RAGPromptBuilder builder;
    const auto& cfg = builder.getConfig();
    EXPECT_GT(cfg.max_context_length, 0u);
    EXPECT_FALSE(cfg.template_placeholder.empty());
    EXPECT_TRUE(cfg.rank_by_relevance);
    EXPECT_TRUE(cfg.include_source_citations);
}

TEST(RAGPromptBuilderTest, SetConfigReplaces) {
    RAGPromptBuilder builder;
    RAGPromptConfig cfg;
    cfg.max_context_length = 100;
    builder.setConfig(cfg);
    EXPECT_EQ(builder.getConfig().max_context_length, 100u);
}

// ============================================================================
// buildContextSection
// ============================================================================

TEST(RAGPromptBuilderTest, ContextSectionEmptyChunks) {
    RAGPromptBuilder builder;
    EXPECT_TRUE(builder.buildContextSection({}).empty());
}

TEST(RAGPromptBuilderTest, ContextSectionContainsChunkContent) {
    RAGPromptBuilder builder;
    auto chunks = makeChunks(2);
    std::string ctx = builder.buildContextSection(chunks);

    EXPECT_NE(ctx.find("Content of chunk 1"), std::string::npos);
    EXPECT_NE(ctx.find("Content of chunk 2"), std::string::npos);
}

TEST(RAGPromptBuilderTest, ContextSectionIncludesHeader) {
    RAGPromptBuilder builder;
    auto chunks = makeChunks(1);
    std::string ctx = builder.buildContextSection(chunks);
    EXPECT_NE(ctx.find("Retrieved Context:"), std::string::npos);
}

TEST(RAGPromptBuilderTest, ContextSectionIncludesSourceCitation) {
    RAGPromptBuilder builder;
    auto chunks = makeChunks(1);
    std::string ctx = builder.buildContextSection(chunks);
    EXPECT_NE(ctx.find("doc_1.pdf"), std::string::npos);
}

TEST(RAGPromptBuilderTest, ContextSectionNoCitationsWhenDisabled) {
    RAGPromptConfig cfg;
    cfg.include_source_citations = false;
    RAGPromptBuilder builder(cfg);
    auto chunks = makeChunks(1);
    std::string ctx = builder.buildContextSection(chunks);
    EXPECT_EQ(ctx.find("doc_1.pdf"), std::string::npos);
}

TEST(RAGPromptBuilderTest, ContextSectionCustomHeader) {
    RAGPromptConfig cfg;
    cfg.context_header = "## Documents";
    RAGPromptBuilder builder(cfg);
    auto chunks = makeChunks(1);
    std::string ctx = builder.buildContextSection(chunks);
    EXPECT_NE(ctx.find("## Documents"), std::string::npos);
}

// ============================================================================
// selectChunks
// ============================================================================

TEST(RAGPromptBuilderTest, SelectChunksReturnsAllWhenBudgetLarge) {
    RAGPromptBuilder builder;
    auto chunks = makeChunks(3);
    auto selected = builder.selectChunks(chunks, 100000);
    EXPECT_EQ(selected.size(), 3u);
}

TEST(RAGPromptBuilderTest, SelectChunksRespectsBudget) {
    RAGPromptConfig cfg;
    cfg.include_source_citations = false;
    cfg.rank_by_relevance        = false;
    cfg.chunk_separator          = "\n";
    RAGPromptBuilder builder(cfg);

    // Create one very long chunk and one short chunk
    RetrievedChunk big;
    big.content         = std::string(500, 'A');
    big.relevance_score = 0.5;

    RetrievedChunk small;
    small.content         = "Short.";
    small.relevance_score = 0.9;

    // Budget should only fit the small chunk
    auto selected = builder.selectChunks({big, small}, 50);
    EXPECT_LE(selected.size(), 1u);
}

TEST(RAGPromptBuilderTest, SelectChunksSortsByRelevance) {
    RAGPromptConfig cfg;
    cfg.rank_by_relevance = true;
    RAGPromptBuilder builder(cfg);

    RetrievedChunk low, high;
    low.content          = "Low relevance.";
    low.relevance_score  = 0.1;
    high.content         = "High relevance.";
    high.relevance_score = 0.9;

    // Supply in low-then-high order; expect high to be selected first
    auto selected = builder.selectChunks({low, high}, 100000);
    ASSERT_GE(selected.size(), 1u);
    EXPECT_EQ(selected.front().content, "High relevance.");
}

TEST(RAGPromptBuilderTest, SelectChunksPreservesOrderWhenSortingDisabled) {
    RAGPromptConfig cfg;
    cfg.rank_by_relevance = false;
    RAGPromptBuilder builder(cfg);

    RetrievedChunk a, b;
    a.content          = "Alpha.";
    a.relevance_score  = 0.1;
    b.content          = "Beta.";
    b.relevance_score  = 0.9;

    auto selected = builder.selectChunks({a, b}, 100000);
    ASSERT_EQ(selected.size(), 2u);
    EXPECT_EQ(selected[0].content, "Alpha.");
    EXPECT_EQ(selected[1].content, "Beta.");
}

// ============================================================================
// build (template injection)
// ============================================================================

TEST(RAGPromptBuilderTest, BuildInjectsContextAndQuery) {
    RAGPromptBuilder builder;
    auto chunks = makeChunks(1);
    std::string tpl = "Context:\n{context}\n\nQuestion: {query}";

    std::string result = builder.build(tpl, "What is this about?", chunks);

    EXPECT_NE(result.find("Content of chunk 1"), std::string::npos);
    EXPECT_NE(result.find("What is this about?"), std::string::npos);
    // Placeholder tokens should be replaced
    EXPECT_EQ(result.find("{context}"), std::string::npos);
    EXPECT_EQ(result.find("{query}"), std::string::npos);
}

TEST(RAGPromptBuilderTest, BuildNoQueryPlaceholderLeavesQueryUnchanged) {
    RAGPromptBuilder builder;
    auto chunks = makeChunks(1);
    std::string tpl = "Answer based on: {context}";

    std::string result = builder.build(tpl, "My query.", chunks);
    EXPECT_NE(result.find("Content of chunk 1"), std::string::npos);
    // The query is NOT substituted because there is no {query} in tpl
    EXPECT_EQ(result.find("My query."), std::string::npos);
}

TEST(RAGPromptBuilderTest, BuildEmptyChunksReplacesPlaceholderWithEmpty) {
    RAGPromptBuilder builder;
    std::string tpl = "Docs:\n{context}\nQ: {query}";

    std::string result = builder.build(tpl, "Hello", {});
    EXPECT_NE(result.find("Hello"), std::string::npos);
    EXPECT_EQ(result.find("{context}"), std::string::npos);
}

// ============================================================================
// buildFullPrompt
// ============================================================================

TEST(RAGPromptBuilderTest, FullPromptContainsSystemInstructionAndQuery) {
    RAGPromptBuilder builder;
    auto chunks = makeChunks(2);

    std::string result = builder.buildFullPrompt(
        "You are a legal assistant.", "What is the case number?", chunks);

    EXPECT_NE(result.find("You are a legal assistant."), std::string::npos);
    EXPECT_NE(result.find("What is the case number?"), std::string::npos);
    EXPECT_NE(result.find("Content of chunk"), std::string::npos);
    EXPECT_NE(result.find("Answer:"), std::string::npos);
}

TEST(RAGPromptBuilderTest, FullPromptNoChunksStillProducesValidPrompt) {
    RAGPromptBuilder builder;
    std::string result = builder.buildFullPrompt("Be helpful.", "Why?", {});

    EXPECT_NE(result.find("Be helpful."), std::string::npos);
    EXPECT_NE(result.find("Why?"), std::string::npos);
    EXPECT_NE(result.find("Answer:"), std::string::npos);
}

TEST(RAGPromptBuilderTest, FullPromptEmptySystemInstruction) {
    RAGPromptBuilder builder;
    auto chunks = makeChunks(1);
    std::string result = builder.buildFullPrompt("", "Query?", chunks);

    // Should not start with a blank line caused by empty system instruction
    EXPECT_NE(result.find("Query?"), std::string::npos);
    EXPECT_NE(result.find("Content of chunk 1"), std::string::npos);
}
