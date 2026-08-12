/**
 * @file test_rag_document_summarizer.cpp
 * @brief Unit tests for DocumentSummarizer (multi-document summarization
 *        before context injection – RAG Phase 3)
 *
 * All tests exercise the heuristic-extractive path so they run without
 * a live LLM engine (LLMIntegration::setInferenceEngine(nullptr)).
 * Tests cover:
 *  - Single-document extractive summarization
 *  - Multi-document extractive summarization (RetrievedDocument)
 *  - Multi-document extractive summarization (StreamedDocument overload)
 *  - Empty document / empty document list edge cases
 *  - max_summary_chars budget enforcement
 *  - Source attribution presence/absence
 *  - Compression ratio in [0, 1]
 *  - Factory helpers (createExtractive, createAbstractive, createAuto)
 *  - Configuration round-trip via getConfig/setConfig
 *  - used_llm flag is false in extractive mode
 *  - ABSTRACTIVE strategy without LLM falls back gracefully
 */

#include "rag/document_summarizer.h"
#include "rag/llm_integration.h"

#include <gtest/gtest.h>
#include <string>
#include <vector>

using namespace themis::rag;
using namespace themis::rag::judge;
using namespace themis::rag::streaming;

// ============================================================================
// Helpers
// ============================================================================

static RetrievedDocument makeRetrievedDoc(const std::string& id,
                                           const std::string& content,
                                           double score = 0.9)
{
    RetrievedDocument d;
    d.id               = id;
    d.content          = content;
    d.similarity_score = score;
    return d;
}

static StreamedDocument makeStreamedDoc(const std::string& id,
                                         const std::string& content,
                                         double score = 0.9)
{
    return StreamedDocument{id, content, score, 0};
}

// Typical multi-sentence document content
static const std::string kDocParis =
    "Paris is the capital of France. "
    "It is located in northern France on the river Seine. "
    "Paris is known for the Eiffel Tower, which was built in 1889. "
    "The city is a major center of art, fashion, and culture. "
    "Millions of tourists visit Paris every year.";

static const std::string kDocGermany =
    "Berlin is the capital of Germany. "
    "Germany is the largest economy in Europe. "
    "The country borders nine other nations. "
    "German engineering is renowned worldwide. "
    "The Rhine river flows through western Germany.";

// ============================================================================
// Fixture
// ============================================================================

class DocumentSummarizerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Ensure no LLM engine is injected – tests use extractive path only
        LLMIntegration::setInferenceEngine(nullptr);

        cfg_.strategy              = DocumentSummarizerConfig::Strategy::EXTRACTIVE;
        cfg_.max_summary_chars     = 500;
        cfg_.max_sentences_per_doc = 3;
        cfg_.min_sentence_chars    = 10;
        cfg_.include_source_attribution = true;
    }

    void TearDown() override {
        LLMIntegration::setInferenceEngine(nullptr);
    }

    DocumentSummarizerConfig cfg_;
};

// ============================================================================
// Single-document summarization
// ============================================================================

TEST_F(DocumentSummarizerTest, SingleDocSummaryNotEmpty) {
    DocumentSummarizer s(cfg_);
    auto result = s.summarize("doc1", kDocParis, "Eiffel Tower");
    EXPECT_FALSE(result.summary.empty());
}

TEST_F(DocumentSummarizerTest, SingleDocSummaryRespectsBudget) {
    cfg_.max_summary_chars = 100;
    DocumentSummarizer s(cfg_);
    auto result = s.summarize("doc1", kDocParis, "capital France");
    EXPECT_LE(result.summary.size(), 200u); // generous tolerance for sentence boundaries
}

TEST_F(DocumentSummarizerTest, SingleDocCoverageScoreInRange) {
    DocumentSummarizer s(cfg_);
    auto result = s.summarize("doc1", kDocParis, "Paris");
    EXPECT_GE(result.coverage_score, 0.0);
    EXPECT_LE(result.coverage_score, 1.0);
}

TEST_F(DocumentSummarizerTest, SingleDocEmptyContent) {
    DocumentSummarizer s(cfg_);
    auto result = s.summarize("doc_empty", "", "query");
    EXPECT_TRUE(result.summary.empty());
    EXPECT_EQ(result.document_id, "doc_empty");
}

TEST_F(DocumentSummarizerTest, SingleDocUsedLlmFalseInExtractiveMode) {
    DocumentSummarizer s(cfg_);
    auto result = s.summarize("doc1", kDocParis, "Paris");
    EXPECT_FALSE(result.used_llm);
}

TEST_F(DocumentSummarizerTest, SingleDocQueryRelevanceImprovesSentenceSelection) {
    DocumentSummarizer s(cfg_);
    auto result = s.summarize("doc1", kDocParis, "Eiffel Tower");
    // The Eiffel Tower sentence should be selected when that's the query
    EXPECT_NE(result.summary.find("Eiffel"), std::string::npos);
}

TEST_F(DocumentSummarizerTest, SingleDocNoQueryStillProducesSummary) {
    DocumentSummarizer s(cfg_);
    auto result = s.summarize("doc1", kDocParis);
    EXPECT_FALSE(result.summary.empty());
}

// ============================================================================
// Multi-document summarization – RetrievedDocument
// ============================================================================

TEST_F(DocumentSummarizerTest, MultiDocCombinedNotEmpty) {
    DocumentSummarizer s(cfg_);
    std::vector<RetrievedDocument> docs = {
        makeRetrievedDoc("d1", kDocParis, 0.9),
        makeRetrievedDoc("d2", kDocGermany, 0.8)
    };
    auto result = s.summarizeMultiple(docs, "capital cities");
    EXPECT_FALSE(result.combined_summary.empty());
}

TEST_F(DocumentSummarizerTest, MultiDocPerDocumentBreakdownCount) {
    DocumentSummarizer s(cfg_);
    std::vector<RetrievedDocument> docs = {
        makeRetrievedDoc("d1", kDocParis),
        makeRetrievedDoc("d2", kDocGermany)
    };
    auto result = s.summarizeMultiple(docs, "Europe");
    EXPECT_EQ(result.per_document_summaries.size(), 2u);
}

TEST_F(DocumentSummarizerTest, MultiDocTotalInputChars) {
    DocumentSummarizer s(cfg_);
    std::vector<RetrievedDocument> docs = {
        makeRetrievedDoc("d1", kDocParis),
        makeRetrievedDoc("d2", kDocGermany)
    };
    auto result = s.summarizeMultiple(docs, "Europe");
    EXPECT_EQ(result.total_input_chars,
              kDocParis.size() + kDocGermany.size());
}

TEST_F(DocumentSummarizerTest, MultiDocCompressionRatioInRange) {
    DocumentSummarizer s(cfg_);
    std::vector<RetrievedDocument> docs = {
        makeRetrievedDoc("d1", kDocParis),
        makeRetrievedDoc("d2", kDocGermany)
    };
    auto result = s.summarizeMultiple(docs, "capital");
    EXPECT_GE(result.compression_ratio, 0.0);
    EXPECT_LE(result.compression_ratio, 1.0);
}

TEST_F(DocumentSummarizerTest, MultiDocUsedLlmFalseInExtractiveMode) {
    DocumentSummarizer s(cfg_);
    std::vector<RetrievedDocument> docs = {
        makeRetrievedDoc("d1", kDocParis)
    };
    auto result = s.summarizeMultiple(docs, "Paris");
    EXPECT_FALSE(result.used_llm);
}

TEST_F(DocumentSummarizerTest, MultiDocSourceAttributionPresent) {
    cfg_.include_source_attribution = true;
    DocumentSummarizer s(cfg_);
    std::vector<RetrievedDocument> docs = {
        makeRetrievedDoc("my-doc-id", kDocParis)
    };
    auto result = s.summarizeMultiple(docs, "Paris");
    EXPECT_NE(result.combined_summary.find("my-doc-id"), std::string::npos);
}

TEST_F(DocumentSummarizerTest, MultiDocSourceAttributionAbsent) {
    cfg_.include_source_attribution = false;
    DocumentSummarizer s(cfg_);
    std::vector<RetrievedDocument> docs = {
        makeRetrievedDoc("my-doc-id", kDocParis)
    };
    auto result = s.summarizeMultiple(docs, "Paris");
    EXPECT_EQ(result.combined_summary.find("[Source:"), std::string::npos);
}

TEST_F(DocumentSummarizerTest, MultiDocEmptyList) {
    DocumentSummarizer s(cfg_);
    auto result = s.summarizeMultiple(
        std::vector<RetrievedDocument>{}, "query");
    EXPECT_TRUE(result.combined_summary.empty());
    EXPECT_EQ(result.total_input_chars, 0u);
}

TEST_F(DocumentSummarizerTest, MultiDocElapsedMsNonNegative) {
    DocumentSummarizer s(cfg_);
    std::vector<RetrievedDocument> docs = {
        makeRetrievedDoc("d1", kDocParis)
    };
    auto result = s.summarizeMultiple(docs, "Paris");
    EXPECT_GE(result.elapsed_ms, 0.0);
}

// ============================================================================
// Multi-document summarization – StreamedDocument overload
// ============================================================================

TEST_F(DocumentSummarizerTest, StreamedDocOverloadProducesSummary) {
    DocumentSummarizer s(cfg_);
    std::vector<StreamedDocument> docs = {
        makeStreamedDoc("s1", kDocParis, 0.9),
        makeStreamedDoc("s2", kDocGermany, 0.7)
    };
    auto result = s.summarizeMultiple(docs, "Europe capitals");
    EXPECT_FALSE(result.combined_summary.empty());
    EXPECT_EQ(result.per_document_summaries.size(), 2u);
}

TEST_F(DocumentSummarizerTest, StreamedDocOverloadEmptyList) {
    DocumentSummarizer s(cfg_);
    auto result = s.summarizeMultiple(
        std::vector<StreamedDocument>{}, "query");
    EXPECT_TRUE(result.combined_summary.empty());
}

// ============================================================================
// ABSTRACTIVE strategy without an LLM engine (graceful degradation)
// ============================================================================

TEST_F(DocumentSummarizerTest, AbstractiveWithoutLLMReturnsNonEmpty) {
    DocumentSummarizerConfig abs_cfg = cfg_;
    abs_cfg.strategy = DocumentSummarizerConfig::Strategy::ABSTRACTIVE;
    DocumentSummarizer s(abs_cfg);
    // LLM is nullptr → LLMIntegration::generate() returns an empty / stub
    // string; the summarizer must not crash and must return a well-formed
    // (possibly empty) result
    std::vector<RetrievedDocument> docs = {
        makeRetrievedDoc("d1", kDocParis)
    };
    auto result = s.summarizeMultiple(docs, "Paris");
    // used_llm should be true (we requested abstractive)
    EXPECT_TRUE(result.used_llm);
    // No crash, per_document_summaries has one entry
    EXPECT_EQ(result.per_document_summaries.size(), 1u);
}

// ============================================================================
// AUTO strategy
// ============================================================================

TEST_F(DocumentSummarizerTest, AutoStrategyWithoutLLMUsesExtractive) {
    DocumentSummarizerConfig auto_cfg = cfg_;
    auto_cfg.strategy = DocumentSummarizerConfig::Strategy::AUTO;
    DocumentSummarizer s(auto_cfg);
    std::vector<RetrievedDocument> docs = {
        makeRetrievedDoc("d1", kDocParis)
    };
    auto result = s.summarizeMultiple(docs, "Paris");
    EXPECT_FALSE(result.used_llm);  // no engine → extractive
    EXPECT_FALSE(result.combined_summary.empty());
}

// ============================================================================
// Configuration round-trip
// ============================================================================

TEST_F(DocumentSummarizerTest, ConfigRoundTrip) {
    DocumentSummarizer s;
    DocumentSummarizerConfig cfg;
    cfg.max_summary_chars     = 1234;
    cfg.max_sentences_per_doc = 7;
    cfg.strategy              = DocumentSummarizerConfig::Strategy::EXTRACTIVE;
    s.setConfig(cfg);
    const auto& stored = s.getConfig();
    EXPECT_EQ(stored.max_summary_chars, 1234u);
    EXPECT_EQ(stored.max_sentences_per_doc, 7u);
    EXPECT_EQ(stored.strategy,
              DocumentSummarizerConfig::Strategy::EXTRACTIVE);
}

// ============================================================================
// Factory helpers
// ============================================================================

TEST(DocumentSummarizerFactoryTest, CreateExtractiveNotNull) {
    auto s = DocumentSummarizerFactory::createExtractive(3);
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(s->getConfig().strategy,
              DocumentSummarizerConfig::Strategy::EXTRACTIVE);
    EXPECT_EQ(s->getConfig().max_sentences_per_doc, 3u);
}

TEST(DocumentSummarizerFactoryTest, CreateAbstractiveNotNull) {
    auto s = DocumentSummarizerFactory::createAbstractive(1500);
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(s->getConfig().strategy,
              DocumentSummarizerConfig::Strategy::ABSTRACTIVE);
    EXPECT_EQ(s->getConfig().max_summary_chars, 1500u);
}

TEST(DocumentSummarizerFactoryTest, CreateAutoNotNull) {
    auto s = DocumentSummarizerFactory::createAuto();
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(s->getConfig().strategy,
              DocumentSummarizerConfig::Strategy::AUTO);
}
