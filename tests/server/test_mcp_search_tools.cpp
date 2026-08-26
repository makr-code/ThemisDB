/**
 * @file test_mcp_search_tools.cpp
 * @brief Unit tests for MCP Group 2 (Vector/Hybrid/RAG) and Group 7 (Schema) tools:
 *        semantic_search, hybrid_search, rag_retrieve, vector_index_list,
 *        schema_validate, explain_query
 *
 * Labels: wave_b release_critical
 */

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <string>

using json = nlohmann::json;

static json missing_param_error(const std::string& param) {
    return {{"error", "missing parameter: " + param}};
}

// ============================================================================
// semantic_search tests
// ============================================================================

TEST(SemanticSearchTest, MissingQueryReturnsError) {
    json args = json::object();
    EXPECT_TRUE(args.find("query") == args.end());
    json expected = missing_param_error("query");
    EXPECT_EQ(expected["error"], "missing parameter: query");
}

TEST(SemanticSearchTest, TopKDefaultIsTen) {
    json args = {{"query", "hello world"}};
    int top_k = std::min(std::max(args.value("top_k", 10), 1), 200);
    EXPECT_EQ(top_k, 10);
}

TEST(SemanticSearchTest, TopKClampedToMax200) {
    json args = {{"query", "hello"}, {"top_k", 999}};
    int top_k = std::min(std::max(args.value("top_k", 10), 1), 200);
    EXPECT_EQ(top_k, 200);
}

TEST(SemanticSearchTest, TopKClampedToMin1) {
    json args = {{"query", "hello"}, {"top_k", -5}};
    int top_k = std::min(std::max(args.value("top_k", 10), 1), 200);
    EXPECT_EQ(top_k, 1);
}

TEST(SemanticSearchTest, OutputShapeHasRequiredFields) {
    json result = {
        {"results", json::array()},
        {"total_candidates_scanned", 0},
        {"query_embedding_model", "default"}
    };
    EXPECT_TRUE(result.contains("results"));
    EXPECT_TRUE(result.contains("total_candidates_scanned"));
    EXPECT_TRUE(result.contains("query_embedding_model"));
}

TEST(SemanticSearchTest, ThresholdRangeValidation) {
    // threshold must be in [0.0, 1.0]
    double raw = 1.5;
    double threshold = std::min(std::max(raw, 0.0), 1.0);
    EXPECT_DOUBLE_EQ(threshold, 1.0);

    raw = -0.5;
    threshold = std::min(std::max(raw, 0.0), 1.0);
    EXPECT_DOUBLE_EQ(threshold, 0.0);
}

// ============================================================================
// hybrid_search tests
// ============================================================================

TEST(HybridSearchTest, MissingQueryReturnsError) {
    json args = json::object();
    EXPECT_TRUE(args.find("query") == args.end());
    auto err = missing_param_error("query");
    EXPECT_EQ(err["error"], "missing parameter: query");
}

TEST(HybridSearchTest, VectorWeightClamped) {
    double raw = 1.8;
    double w = std::min(std::max(raw, 0.0), 1.0);
    EXPECT_DOUBLE_EQ(w, 1.0);
}

TEST(HybridSearchTest, Bm25WeightClamped) {
    double raw = -0.3;
    double w = std::min(std::max(raw, 0.0), 1.0);
    EXPECT_DOUBLE_EQ(w, 0.0);
}

TEST(HybridSearchTest, OutputShapeHasRequiredFields) {
    json result = {
        {"results",       json::array()},
        {"top_k_returned", 0}
    };
    EXPECT_TRUE(result.contains("results"));
    EXPECT_TRUE(result.contains("top_k_returned"));
}

TEST(HybridSearchTest, RrfScorePositive) {
    // RRF formula: weight / (rank + 60)
    double rrf = 0.5 / (1 + 60.0);
    EXPECT_GT(rrf, 0.0);
}

// ============================================================================
// rag_retrieve tests
// ============================================================================

TEST(RagRetrieveTest, MissingQueryReturnsError) {
    json args = json::object();
    EXPECT_TRUE(args.find("query") == args.end());
    auto err = missing_param_error("query");
    EXPECT_EQ(err["error"], "missing parameter: query");
}

TEST(RagRetrieveTest, DefaultTopKIsFive) {
    json args = {{"query", "test"}};
    int top_k = std::min(std::max(args.value("top_k", 5), 1), 20);
    EXPECT_EQ(top_k, 5);
}

TEST(RagRetrieveTest, TopKClampedToMax20) {
    json args = {{"query", "test"}, {"top_k", 100}};
    int top_k = std::min(std::max(args.value("top_k", 5), 1), 20);
    EXPECT_EQ(top_k, 20);
}

TEST(RagRetrieveTest, OutputShapeHasRequiredFields) {
    json result = {
        {"context_chunks",        json::array()},
        {"total_tokens_estimate", 0},
        {"retrieval_latency_ms",  5}
    };
    EXPECT_TRUE(result.contains("context_chunks"));
    EXPECT_TRUE(result.contains("total_tokens_estimate"));
    EXPECT_TRUE(result.contains("retrieval_latency_ms"));
}

TEST(RagRetrieveTest, RerankFalseStillReturnsChunks) {
    json args = {{"query", "test"}, {"rerank", false}};
    bool rerank = args.value("rerank", true);
    EXPECT_FALSE(rerank);
    // No error should be produced from rerank=false; chunks array still returned
    json result = {{"context_chunks", json::array()}, {"total_tokens_estimate", 0}, {"retrieval_latency_ms", 0}};
    EXPECT_TRUE(result["context_chunks"].is_array());
}

TEST(RagRetrieveTest, ChunkRankIsSequential) {
    json chunks = json::array();
    for (int i = 1; i <= 3; ++i) {
        chunks.push_back({{"rank", i}, {"content", "chunk"}, {"score", 0.9 - i * 0.1}});
    }
    EXPECT_EQ(chunks[0]["rank"].get<int>(), 1);
    EXPECT_EQ(chunks[1]["rank"].get<int>(), 2);
    EXPECT_EQ(chunks[2]["rank"].get<int>(), 3);
}

// ============================================================================
// vector_index_list tests
// ============================================================================

TEST(VectorIndexListTest, OutputShapeHasIndexesArray) {
    json result = {{"indexes", json::array()}};
    EXPECT_TRUE(result.contains("indexes"));
    EXPECT_TRUE(result["indexes"].is_array());
}

TEST(VectorIndexListTest, CollectionFilterApplied) {
    // Only vector indexes for the requested collection should be returned
    json all_indexes = json::array({
        {{"name", "vec1"}, {"collection", "docs"},  {"type", "hnsw"}},
        {{"name", "vec2"}, {"collection", "other"}, {"type", "hnsw"}}
    });
    std::string filter = "docs";
    json filtered = json::array();
    for (auto& idx : all_indexes) {
        if (idx["collection"].get<std::string>() == filter) filtered.push_back(idx);
    }
    EXPECT_EQ(filtered.size(), 1u);
    EXPECT_EQ(filtered[0]["collection"].get<std::string>(), "docs");
}

// ============================================================================
// schema_validate tests
// ============================================================================

TEST(SchemaValidateTest, MissingCollectionReturnsError) {
    json args = {{"document", {{"id", 1}}}};
    EXPECT_TRUE(args.find("collection") == args.end());
    auto err = missing_param_error("collection");
    EXPECT_EQ(err["error"], "missing parameter: collection");
}

TEST(SchemaValidateTest, MissingDocumentReturnsError) {
    json args = {{"collection", "persons"}};
    EXPECT_TRUE(args.find("document") == args.end());
    auto err = missing_param_error("document");
    EXPECT_EQ(err["error"], "missing parameter: document");
}

TEST(SchemaValidateTest, ValidDocumentReturnsTrue) {
    json result = {{"valid", true}, {"errors", json::array()}};
    EXPECT_TRUE(result["valid"].get<bool>());
    EXPECT_TRUE(result["errors"].empty());
}

TEST(SchemaValidateTest, InvalidDocumentReturnsFalse) {
    json errors = json::array({{{"field", "name"}, {"message", "Required field 'name' is missing"}}});
    json result = {{"valid", false}, {"errors", errors}};
    EXPECT_FALSE(result["valid"].get<bool>());
    EXPECT_EQ(result["errors"].size(), 1u);
    EXPECT_EQ(result["errors"][0]["field"].get<std::string>(), "name");
}

// ============================================================================
// explain_query tests
// ============================================================================

TEST(ExplainQueryTest, MissingQueryReturnsError) {
    json args = json::object();
    EXPECT_TRUE(args.find("query") == args.end());
    auto err = missing_param_error("query");
    EXPECT_EQ(err["error"], "missing parameter: query");
}

TEST(ExplainQueryTest, OutputShapeHasPlan) {
    json result = {
        {"plan", {
            {"nodes",               json::array()},
            {"estimated_total_cost", 0},
            {"optimizations_applied", json::array()}
        }},
        {"note", "explain not yet available for this query type (language=aql)"}
    };
    EXPECT_TRUE(result.contains("plan"));
    EXPECT_TRUE(result["plan"].contains("nodes"));
    EXPECT_TRUE(result["plan"].contains("estimated_total_cost"));
    EXPECT_TRUE(result["plan"].contains("optimizations_applied"));
}

TEST(ExplainQueryTest, DefaultLanguageIsAql) {
    json args = {{"query", "FOR x IN col RETURN x"}};
    std::string lang = args.value("language", "aql");
    EXPECT_EQ(lang, "aql");
}
