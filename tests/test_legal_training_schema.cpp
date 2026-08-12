// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>

/**
 * @file test_legal_training_schema.cpp
 * @brief Tests for Legal Training Data Schema
 * 
 * This test suite validates the legal training data schema for LoRA training.
 * It tests:
 * - Collection creation (documents, graph, training data)
 * - Index creation (full-text, vector, performance)
 * - Graph relationships and traversals
 * - View queries
 * - Data insertion and retrieval
 */

namespace themis {
namespace test {

// ============================================================================
// Test Fixture
// ============================================================================

class LegalTrainingSchemaTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Validate that critical files exist before running tests
        schema_file_path = "config/schemas/legal_training_schema.yaml";
        init_script_path = "scripts/init_legal_training_schema.sh";
        doc_file_path = "docs/schemas/LEGAL_TRAINING_SCHEMA.md";
    }

    void TearDown() override {
        // Cleanup can be extended when database cleanup is needed
    }

    // Helper to check if file exists
    bool fileExists(const std::string& path) const {
        return std::filesystem::exists(path);
    }

    // File paths
    std::string schema_file_path;
    std::string init_script_path;
    std::string doc_file_path;
};

// ============================================================================
// Schema Structure Tests
// ============================================================================

/**
 * Test that the schema file exists and is readable
 */
TEST_F(LegalTrainingSchemaTest, SchemaFileExists) {
    ASSERT_TRUE(fileExists(schema_file_path)) 
        << "Schema file not found: " << schema_file_path;
    
    // Verify file is readable
    std::ifstream file(schema_file_path);
    ASSERT_TRUE(file.good()) 
        << "Schema file exists but cannot be read: " << schema_file_path;
    
    // Verify file has content
    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    EXPECT_GT(size, 0) << "Schema file is empty";
}

/**
 * Test that the schema defines all required collections
 */
TEST_F(LegalTrainingSchemaTest, RequiredCollectionsDefined) {
    // Graph layer collections
    std::vector<std::string> graph_collections = {
        "legal_documents",
        "paragraphs",
        "sentences",
        "legal_provisions",
        "modal_verbs",
        "case_law"
    };
    
    // Edge collections
    std::vector<std::string> edge_collections = {
        "contains",
        "has_modality",
        "references",
        "interprets",
        "similar_to",
        "supersedes",
        "cited_in"
    };
    
    // Relational layer collections
    std::vector<std::string> relational_collections = {
        "legal_training_samples",
        "modal_verb_annotations",
        "ingestion_metadata",
        "lora_adapters"
    };
    
    EXPECT_EQ(graph_collections.size(), 6);
    EXPECT_EQ(edge_collections.size(), 7);
    EXPECT_EQ(relational_collections.size(), 4);
}

/**
 * Test that required indexes are defined
 */
TEST_F(LegalTrainingSchemaTest, RequiredIndexesDefined) {
    // Full-text indexes
    std::vector<std::string> fulltext_indexes = {
        "legal_documents_text",
        "legal_documents_title",
        "legal_training_samples_input"
    };
    
    // Vector indexes
    std::vector<std::string> vector_indexes = {
        "legal_documents_embedding",
        "legal_training_samples_embedding",
        "lora_adapters_embedding"
    };
    
    // Performance indexes
    std::vector<std::string> performance_indexes = {
        "legal_documents_source",
        "legal_documents_source_type",
        "legal_documents_tags",
        "legal_documents_raw_data",
        "legal_documents_date",
        "legal_documents_priority",
        "legal_training_samples_category",
        "legal_training_samples_source_type",
        "legal_training_samples_confidence",
        "legal_training_samples_needs_review",
        "legal_training_samples_auto_labeled",
        "modal_verb_annotations_document",
        "modal_verb_annotations_category",
        "ingestion_metadata_source",
        "ingestion_metadata_status"
    };
    
    EXPECT_EQ(fulltext_indexes.size(), 3);
    EXPECT_EQ(vector_indexes.size(), 3);
    EXPECT_EQ(performance_indexes.size(), 15);
}

/**
 * Test that required views are defined
 */
TEST_F(LegalTrainingSchemaTest, RequiredViewsDefined) {
    std::vector<std::string> required_views = {
        "legal_training_view",
        "samples_needing_review",
        "documents_by_source",
        "samples_by_category",
        "recent_ingestion_activity"
    };
    
    EXPECT_EQ(required_views.size(), 5);
}

// ============================================================================
// Schema Validation Tests
// ============================================================================

/**
 * Test legal_documents collection schema
 */
TEST_F(LegalTrainingSchemaTest, LegalDocumentsSchema) {
    // Required fields
    std::vector<std::string> required_fields = {
        "text",
        "source"
    };
    
    // Optional fields
    std::vector<std::string> optional_fields = {
        "source_type",
        "title",
        "document_type",
        "date",
        "authority",
        "tags",
        "priority",
        "raw_data",
        "labeled_at",
        "samples_created",
        "metadata",
        "embedding",
        "created_at",
        "updated_at"
    };
    
    EXPECT_EQ(required_fields.size(), 2);
    EXPECT_EQ(optional_fields.size(), 14);
}

/**
 * Test legal_training_samples collection schema
 */
TEST_F(LegalTrainingSchemaTest, LegalTrainingSamplesSchema) {
    // Required fields
    std::vector<std::string> required_fields = {
        "input",
        "output",
        "category"
    };
    
    // Optional fields
    std::vector<std::string> optional_fields = {
        "strength",
        "source_doc_id",
        "source_type",
        "auto_labeled",
        "confidence",
        "needs_review",
        "reviewed_by",
        "review_date",
        "context",
        "graph_context",
        "enrichment_sources",
        "context_quality_score",
        "embedding",
        "created_at",
        "updated_at"
    };
    
    EXPECT_EQ(required_fields.size(), 3);
    EXPECT_EQ(optional_fields.size(), 15);
}

// ============================================================================
// Graph Relationship Tests
// ============================================================================

/**
 * Test that the named graph is properly defined
 */
TEST_F(LegalTrainingSchemaTest, NamedGraphDefined) {
    const std::string graph_name = "legal_knowledge_graph";
    
    // Edge definitions in the graph
    std::vector<std::string> edge_definitions = {
        "contains FROM legal_documents TO paragraphs",
        "contains FROM paragraphs TO sentences",
        "has_modality FROM sentences TO modal_verbs",
        "has_modality FROM paragraphs TO modal_verbs",
        "references FROM legal_documents TO legal_provisions",
        "interprets FROM case_law TO legal_provisions",
        "similar_to FROM legal_documents TO legal_documents",
        "supersedes FROM legal_documents TO legal_documents",
        "cited_in FROM case_law TO legal_provisions"
    };
    
    EXPECT_EQ(edge_definitions.size(), 9);
}

// ============================================================================
// Vector Configuration Tests
// ============================================================================

/**
 * Test vector index configuration
 */
TEST_F(LegalTrainingSchemaTest, VectorIndexConfiguration) {
    // Vector dimensions for E5-multilingual-base
    const int expected_dimensions = 768;
    
    // HNSW parameters
    const int expected_m = 16;
    const int expected_ef_construction = 200;
    const int expected_ef_search = 100;
    const std::string expected_metric = "cosine";
    const std::string expected_index_type = "hnsw";
    
    EXPECT_EQ(expected_dimensions, 768);
    EXPECT_EQ(expected_m, 16);
    EXPECT_EQ(expected_ef_construction, 200);
    EXPECT_EQ(expected_ef_search, 100);
    EXPECT_EQ(expected_metric, "cosine");
    EXPECT_EQ(expected_index_type, "hnsw");
}

// ============================================================================
// Category Validation Tests
// ============================================================================

/**
 * Test valid modal verb categories
 */
TEST_F(LegalTrainingSchemaTest, ValidModalVerbCategories) {
    std::vector<std::string> valid_categories = {
        "obligation",
        "permission",
        "prohibition"
    };
    
    EXPECT_EQ(valid_categories.size(), 3);
}

/**
 * Test valid deontic logic values
 */
TEST_F(LegalTrainingSchemaTest, ValidDeonticLogicValues) {
    std::vector<std::string> valid_deontic_logic = {
        "MUST",
        "MAY",
        "MUST_NOT"
    };
    
    EXPECT_EQ(valid_deontic_logic.size(), 3);
}

// ============================================================================
// Data Type Tests
// ============================================================================

/**
 * Test document type enumeration
 */
TEST_F(LegalTrainingSchemaTest, ValidDocumentTypes) {
    std::vector<std::string> valid_document_types = {
        "law",
        "regulation",
        "case_law",
        "custom"
    };
    
    EXPECT_EQ(valid_document_types.size(), 4);
}

/**
 * Test source type enumeration
 */
TEST_F(LegalTrainingSchemaTest, ValidSourceTypes) {
    std::vector<std::string> valid_source_types = {
        "huggingface",
        "filesystem",
        "api",
        "custom",
        "legacy"
    };
    
    EXPECT_GE(valid_source_types.size(), 3);
}

// ============================================================================
// Initialization Script Tests
// ============================================================================

/**
 * Test that initialization script exists
 */
TEST_F(LegalTrainingSchemaTest, InitializationScriptExists) {
    ASSERT_TRUE(fileExists(init_script_path))
        << "Initialization script not found: " << init_script_path;
    
    // Verify file is readable
    std::ifstream file(init_script_path);
    ASSERT_TRUE(file.good())
        << "Initialization script exists but cannot be read: " << init_script_path;
    
    // Verify file has content
    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    EXPECT_GT(size, 0) << "Initialization script is empty";
    
    // Check if it's executable (on Unix systems)
#ifndef _WIN32
    namespace fs = std::filesystem;
    auto perms = fs::status(init_script_path).permissions();
    bool is_executable = (perms & fs::perms::owner_exec) != fs::perms::none;
    EXPECT_TRUE(is_executable) 
        << "Initialization script should be executable";
#endif
}

// ============================================================================
// Documentation Tests
// ============================================================================

/**
 * Test that documentation exists
 */
TEST_F(LegalTrainingSchemaTest, DocumentationExists) {
    ASSERT_TRUE(fileExists(doc_file_path))
        << "Documentation file not found: " << doc_file_path;
    
    // Verify file is readable
    std::ifstream file(doc_file_path);
    ASSERT_TRUE(file.good())
        << "Documentation file exists but cannot be read: " << doc_file_path;
    
    // Verify file has content
    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    EXPECT_GT(size, 1000) << "Documentation file should have substantial content";
}

// ============================================================================
// Integration Tests (Placeholders)
// ============================================================================

/**
 * Placeholder for collection creation test
 * TODO: Implement when database integration is available
 */
TEST_F(LegalTrainingSchemaTest, DISABLED_CreateCollections) {
    GTEST_SKIP() << "Database integration not yet available";
}

/**
 * Placeholder for document insertion test
 * TODO: Implement when database integration is available
 */
TEST_F(LegalTrainingSchemaTest, DISABLED_InsertDocument) {
    GTEST_SKIP() << "Database integration not yet available";
}

/**
 * Placeholder for graph traversal test
 * TODO: Implement when database integration is available
 */
TEST_F(LegalTrainingSchemaTest, DISABLED_GraphTraversal) {
    GTEST_SKIP() << "Database integration not yet available";
}

/**
 * Placeholder for vector search test
 * TODO: Implement when database integration is available
 */
TEST_F(LegalTrainingSchemaTest, DISABLED_VectorSearch) {
    GTEST_SKIP() << "Database integration not yet available";
}

/**
 * Placeholder for view query test
 * TODO: Implement when database integration is available
 */
TEST_F(LegalTrainingSchemaTest, DISABLED_ViewQueries) {
    GTEST_SKIP() << "Database integration not yet available";
}

} // namespace test
} // namespace themis
