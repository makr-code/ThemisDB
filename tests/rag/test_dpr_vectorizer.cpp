/**
 * @file test_dpr_vectorizer.cpp
 * @brief Unit tests for DPRVectorizer (Wave A2: Dense Passage Retrieval)
 *
 * Test IDs: DPR-01 .. DPR-10
 *
 * Covers:
 *  - Initialization and configuration
 *  - Query encoding
 *  - Passage encoding (single and batch)
 *  - Embedding dimension reporting
 *  - Error handling for uninitialized state
 *  - Batch processing consistency
 */

#include <gtest/gtest.h>
#include "rag/dpr_vectorizer.h"

using namespace themis::rag;

// ─────────────────────────────────────────────────────────────────────────────
// DPR-01: DPRVectorizer constructs with config
// ─────────────────────────────────────────────────────────────────────────────
TEST(DPRVectorizer, DPR_01_ConstructsWithConfig) {
    DPRVectorizerConfig config;
    config.query_model_path = "/path/to/query_model";
    config.passage_model_path = "/path/to/passage_model";
    config.embedding_dimension = 384;

    DPRVectorizer vectorizer(config);
    EXPECT_FALSE(vectorizer.isInitialized());
    EXPECT_EQ(vectorizer.getConfig().embedding_dimension, 384u);
}

// ─────────────────────────────────────────────────────────────────────────────
// DPR-02: Initialize returns OK with valid config
// ─────────────────────────────────────────────────────────────────────────────
TEST(DPRVectorizer, DPR_02_InitializeSucceeds) {
    DPRVectorizerConfig config;
    config.query_model_path = "/path/to/query_model";
    config.passage_model_path = "/path/to/passage_model";

    DPRVectorizer vectorizer(config);
    EXPECT_NO_THROW(vectorizer.initialize());
    EXPECT_TRUE(vectorizer.isInitialized());
}

// ─────────────────────────────────────────────────────────────────────────────
// DPR-03: Initialize fails with empty query_model_path
// ─────────────────────────────────────────────────────────────────────────────
TEST(DPRVectorizer, DPR_03_InitializeFailsWithEmptyQueryModel) {
    DPRVectorizerConfig config;
    config.query_model_path = "";  // Empty
    config.passage_model_path = "/path/to/passage_model";

    DPRVectorizer vectorizer(config);
    
    EXPECT_THROW(vectorizer.initialize(), std::invalid_argument);
    EXPECT_FALSE(vectorizer.isInitialized());
}

// ─────────────────────────────────────────────────────────────────────────────
// DPR-04: Initialize fails with empty passage_model_path
// ─────────────────────────────────────────────────────────────────────────────
TEST(DPRVectorizer, DPR_04_InitializeFailsWithEmptyPassageModel) {
    DPRVectorizerConfig config;
    config.query_model_path = "/path/to/query_model";
    config.passage_model_path = "";  // Empty

    DPRVectorizer vectorizer(config);
    
    EXPECT_THROW(vectorizer.initialize(), std::invalid_argument);
    EXPECT_FALSE(vectorizer.isInitialized());
}

// ─────────────────────────────────────────────────────────────────────────────
// DPR-05: encodeQuery fails when not initialized
// ─────────────────────────────────────────────────────────────────────────────
TEST(DPRVectorizer, DPR_05_EncodeQueryFailsWhenNotInitialized) {
    DPRVectorizerConfig config;
    config.query_model_path = "/path/to/query_model";
    config.passage_model_path = "/path/to/passage_model";

    DPRVectorizer vectorizer(config);
    
    EXPECT_THROW(vectorizer.encodeQuery("test query"), std::runtime_error);
}

// ─────────────────────────────────────────────────────────────────────────────
// DPR-06: encodeQuery fails with empty query
// ─────────────────────────────────────────────────────────────────────────────
TEST(DPRVectorizer, DPR_06_EncodeQueryFailsWithEmptyQuery) {
    DPRVectorizerConfig config;
    config.query_model_path = "/path/to/query_model";
    config.passage_model_path = "/path/to/passage_model";

    DPRVectorizer vectorizer(config);
    vectorizer.initialize();
    
    EXPECT_THROW(vectorizer.encodeQuery(""), std::invalid_argument);
}

// ─────────────────────────────────────────────────────────────────────────────
// DPR-07: encodeQuery returns vector of correct dimension
// ─────────────────────────────────────────────────────────────────────────────
TEST(DPRVectorizer, DPR_07_EncodeQueryReturnsCorrectDimension) {
    DPRVectorizerConfig config;
    config.query_model_path = "/path/to/query_model";
    config.passage_model_path = "/path/to/passage_model";
    config.embedding_dimension = 384;

    DPRVectorizer vectorizer(config);
    vectorizer.initialize();
    
    auto embedding = vectorizer.encodeQuery("test query");
    EXPECT_EQ(embedding.size(), 384u);
    EXPECT_EQ(vectorizer.getEmbeddingDimension(), 384u);
}

// ─────────────────────────────────────────────────────────────────────────────
// DPR-08: encodePassage works correctly
// ─────────────────────────────────────────────────────────────────────────────
TEST(DPRVectorizer, DPR_08_EncodePassageWorks) {
    DPRVectorizerConfig config;
    config.query_model_path = "/path/to/query_model";
    config.passage_model_path = "/path/to/passage_model";
    config.embedding_dimension = 768;

    DPRVectorizer vectorizer(config);
    vectorizer.initialize();
    
    auto embedding = vectorizer.encodePassage("test passage");
    EXPECT_EQ(embedding.size(), 768u);
}

// ─────────────────────────────────────────────────────────────────────────────
// DPR-09: encodePassageBatch returns correct number of embeddings
// ─────────────────────────────────────────────────────────────────────────────
TEST(DPRVectorizer, DPR_09_EncodePassageBatchWorks) {
    DPRVectorizerConfig config;
    config.query_model_path = "/path/to/query_model";
    config.passage_model_path = "/path/to/passage_model";
    config.embedding_dimension = 384;

    DPRVectorizer vectorizer(config);
    vectorizer.initialize();
    
    std::vector<std::string> passages = {
        "passage one",
        "passage two",
        "passage three"
    };
    
    auto results = vectorizer.encodePassageBatch(passages);
    EXPECT_EQ(results.size(), 3u);
    for (const auto& embedding : results) {
        EXPECT_EQ(embedding.size(), 384u);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// DPR-10: getConfig returns configured parameters
// ─────────────────────────────────────────────────────────────────────────────
TEST(DPRVectorizer, DPR_10_GetConfigReturnsParameters) {
    DPRVectorizerConfig config;
    config.query_model_path = "/custom/query/path";
    config.passage_model_path = "/custom/passage/path";
    config.device = "cuda";
    config.batch_size = 64;
    config.embedding_dimension = 512;

    DPRVectorizer vectorizer(config);
    
    const auto& retrieved_config = vectorizer.getConfig();
    EXPECT_EQ(retrieved_config.query_model_path, "/custom/query/path");
    EXPECT_EQ(retrieved_config.passage_model_path, "/custom/passage/path");
    EXPECT_EQ(retrieved_config.device, "cuda");
    EXPECT_EQ(retrieved_config.batch_size, 64u);
    EXPECT_EQ(retrieved_config.embedding_dimension, 512u);
}
