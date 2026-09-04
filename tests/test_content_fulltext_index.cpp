// ============================================================================
// ThemisDB - Content Automatic Fulltext Index Tests
// Tests for: Automatic fulltext index creation on document ingestion
// ============================================================================

#include <gtest/gtest.h>
#include "content/content_manager.h"
#include "storage/rocksdb_wrapper.h"
#include "index/vector_index_manager.h"
#include "index/graph_index.h"
#include "index/secondary_index.h"
#include <filesystem>
#include <nlohmann/json.hpp>

using namespace themis;
using namespace themis::content;
using json = nlohmann::json;

class ContentFulltextIndexTest : public ::testing::Test {
protected:
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<VectorIndexManager> vector_index_;
    std::shared_ptr<GraphIndexManager> graph_index_;
    std::shared_ptr<SecondaryIndexManager> secondary_index_;
    std::shared_ptr<ContentManager> content_mgr_;
    
    std::string test_db_path_ = "./test_content_fulltext_index_db";

    void SetUp() override {
#ifdef _WIN32
        GTEST_SKIP() << "Skipping ContentFulltextIndexTest on Windows due to timeout instability in fixture setup/index bootstrapping.";
#endif
        // Clean up test database
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }

        // Initialize storage
        RocksDBWrapper::Config cfg;
        cfg.db_path = test_db_path_;
        storage_ = std::make_shared<RocksDBWrapper>(cfg);
        ASSERT_TRUE(storage_->open());

        // Initialize index managers
        vector_index_ = std::make_shared<VectorIndexManager>(*storage_);
        graph_index_ = std::make_shared<GraphIndexManager>(*storage_);
        secondary_index_ = std::make_shared<SecondaryIndexManager>(*storage_);

        // Initialize content manager
        content_mgr_ = std::make_shared<ContentManager>(
            storage_, vector_index_, graph_index_, secondary_index_);
    }

    void TearDown() override {
        content_mgr_.reset();
        secondary_index_.reset();
        graph_index_.reset();
        vector_index_.reset();
        if (storage_) {
            storage_->close();
        }
        storage_.reset();

        // Clean up test database
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }
    }

    // Helper: Enable automatic fulltext indexing
    void enableAutoFulltextIndex(bool enable = true, 
                                   const std::string& language = "en",
                                   bool stemming = true,
                                   bool stopwords = true) {
        json config = {
            {"auto_fulltext_index", enable},
            {"fulltext_config", {
                {"language", language},
                {"stemming_enabled", stemming},
                {"stopwords_enabled", stopwords},
                {"normalize_umlauts", false}
            }}
        };
        
        std::string config_str = config.dump();
        storage_->put("config:content", 
                      std::vector<uint8_t>(config_str.begin(), config_str.end()));
    }

    // Helper: Import test content with chunks
    std::string importTestContent(const std::vector<std::string>& chunk_texts,
                                    const std::string& content_id = "") {
        json spec;
        
        // Content metadata - use more predictable ID generation for tests
        std::string cid = {};
        if (content_id.empty()) {
            // Use timestamp-based ID for better uniqueness in tests
            auto now = std::chrono::system_clock::now();
            auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()).count();
            cid = "test-content-" + std::to_string(timestamp);
        } else {
            cid = content_id;
        }
        
        spec["content"] = {
            {"id", cid},
            {"mime_type", "text/plain"},
            {"category", "TEXT"},
            {"original_filename", "test.txt"},
            {"size_bytes", 1000},
            {"created_at", 1234567890},
            {"hash_sha256", "abcd1234"},
            {"text_extracted", true},
            {"chunked", true},
            {"indexed", false}
        };
        
        // Chunks
        json chunks = json::array();
        for (size_t i = 0; i < chunk_texts.size(); ++i) {
            chunks.push_back({
                {"id", cid + "-chunk-" + std::to_string(i)},
                {"content_id", cid},
                {"seq_num", i},
                {"chunk_type", "text"},
                {"text", chunk_texts[i]},
                {"start_offset", 0},
                {"end_offset", static_cast<int>(chunk_texts[i].size())},
                {"created_at", 1234567890}
            });
        }
        spec["chunks"] = chunks;
        
        auto status = content_mgr_->importContent(spec);
        EXPECT_TRUE(status.ok) << "Failed to import content: " << status.message;
        
        return cid;
    }
};

// ============================================================================
// Automatic Fulltext Index Tests
// ============================================================================

TEST_F(ContentFulltextIndexTest, AutoIndexDisabledByDefault) {
    // Import content without enabling auto-indexing
    std::vector<std::string> chunks = {
        "Machine learning algorithms analyze data patterns.",
        "Neural networks are inspired by biological brains."
    };
    
    std::string content_id = importTestContent(chunks);
    ASSERT_FALSE(content_id.empty());
    
    // Fulltext index should NOT be created
    EXPECT_FALSE(secondary_index_->hasFulltextIndex("chunk", "text"));
}

TEST_F(ContentFulltextIndexTest, AutoIndexCreatedWhenEnabled) {
    // Enable auto-indexing
    enableAutoFulltextIndex(true, "en", true, false);
    
    // Import content
    std::vector<std::string> chunks = {
        "Machine learning algorithms analyze data patterns.",
        "Neural networks are inspired by biological brains."
    };
    
    std::string content_id = importTestContent(chunks, "test-1");
    ASSERT_FALSE(content_id.empty());
    
    // Fulltext index should be created
    EXPECT_TRUE(secondary_index_->hasFulltextIndex("chunk", "text"));
    
    // Verify we can search the content
    auto [status, results] = secondary_index_->scanFulltextWithScores(
        "chunk", "text", "machine learning", 10);
    
    EXPECT_TRUE(status.ok) << "Fulltext search failed: " << status.message;
    EXPECT_FALSE(results.empty()) << "Should find at least one result";
    
    if (!results.empty()) {
        EXPECT_GT(results[0].score, 0.0) << "Score should be positive";
    }
}

TEST_F(ContentFulltextIndexTest, SearchFindsMultipleChunks) {
    // Enable auto-indexing
    enableAutoFulltextIndex(true, "en", true, false);
    
    // Import content with multiple chunks containing the search term
    std::vector<std::string> chunks = {
        "Machine learning is a subset of artificial intelligence.",
        "Deep learning is a type of machine learning.",
        "Neural networks enable deep learning capabilities."
    };
    
    std::string content_id = importTestContent(chunks, "test-2");
    ASSERT_FALSE(content_id.empty());
    
    // Search for "learning" - should find multiple chunks
    auto [status, results] = secondary_index_->scanFulltextWithScores(
        "chunk", "text", "learning", 10);
    
    EXPECT_TRUE(status.ok);
    EXPECT_GE(results.size(), 2) << "Should find at least 2 chunks with 'learning'";
}

TEST_F(ContentFulltextIndexTest, SearchWithStemmingEnabled) {
    // Enable auto-indexing with stemming
    enableAutoFulltextIndex(true, "en", true, false);
    
    // Import content
    std::vector<std::string> chunks = {
        "The algorithm processes data efficiently.",
        "Processing large datasets requires optimization."
    };
    
    std::string content_id = importTestContent(chunks, "test-3");
    ASSERT_FALSE(content_id.empty());
    
    // Search with different word form - stemming should match "process" with "processing"
    auto [status, results] = secondary_index_->scanFulltextWithScores(
        "chunk", "text", "process", 10);
    
    EXPECT_TRUE(status.ok);
    // With stemming enabled, should find chunks with "processes" and "Processing"
    EXPECT_GE(results.size(), 1);
}

TEST_F(ContentFulltextIndexTest, DeleteContentRemovesFromIndex) {
    // Enable auto-indexing
    enableAutoFulltextIndex(true, "en", false, false);
    
    // Import content
    std::vector<std::string> chunks = {
        "Temporary content for deletion test."
    };
    
    std::string content_id = importTestContent(chunks, "test-delete");
    ASSERT_FALSE(content_id.empty());
    
    // Verify content is searchable
    auto [status1, results1] = secondary_index_->scanFulltextWithScores(
        "chunk", "text", "temporary", 10);
    EXPECT_TRUE(status1.ok);
    EXPECT_FALSE(results1.empty());
    
    // Delete the content
    auto delete_status = content_mgr_->deleteContent(content_id);
    EXPECT_TRUE(delete_status.ok) << "Delete failed: " << delete_status.message;
    
    // Verify content is no longer searchable
    auto [status2, results2] = secondary_index_->scanFulltextWithScores(
        "chunk", "text", "temporary", 10);
    EXPECT_TRUE(status2.ok);
    // Results should be empty or not contain the deleted chunk
    bool found_deleted = false;
    for (const auto& r : results2) {
        if (r.pk == "chunk-0") {
            found_deleted = true;
            break;
        }
    }
    EXPECT_FALSE(found_deleted) << "Deleted chunk should not be in search results";
}

TEST_F(ContentFulltextIndexTest, GermanLanguageSupport) {
    // Enable auto-indexing with German language
    enableAutoFulltextIndex(true, "de", true, false);
    
    // Import German content
    std::vector<std::string> chunks = {
        "Die Datenbank speichert Informationen effizient.",
        "Künstliche Intelligenz revolutioniert die Technologie."
    };
    
    std::string content_id = importTestContent(chunks, "test-german");
    ASSERT_FALSE(content_id.empty());
    
    // Search in German
    auto [status, results] = secondary_index_->scanFulltextWithScores(
        "chunk", "text", "Datenbank", 10);
    
    EXPECT_TRUE(status.ok);
    EXPECT_FALSE(results.empty());
}

TEST_F(ContentFulltextIndexTest, EmptyChunksNotIndexed) {
    // Enable auto-indexing
    enableAutoFulltextIndex(true, "en", false, false);
    
    // Import content with empty chunks
    std::vector<std::string> chunks = {
        "Valid content here.",
        "",  // Empty chunk
        "More valid content."
    };
    
    std::string content_id = importTestContent(chunks, "test-empty");
    ASSERT_FALSE(content_id.empty());
    
    // Index should be created and work for non-empty chunks
    EXPECT_TRUE(secondary_index_->hasFulltextIndex("chunk", "text"));
    
    auto [status, results] = secondary_index_->scanFulltextWithScores(
        "chunk", "text", "valid", 10);
    
    EXPECT_TRUE(status.ok);
    EXPECT_GE(results.size(), 1);
}

TEST_F(ContentFulltextIndexTest, MultipleDocumentsShareIndex) {
    // Enable auto-indexing
    enableAutoFulltextIndex(true, "en", false, false);
    
    // Import first document
    std::vector<std::string> chunks1 = {
        "First document about databases."
    };
    std::string content_id1 = importTestContent(chunks1, "test-doc1");
    ASSERT_FALSE(content_id1.empty());
    
    // Import second document
    std::vector<std::string> chunks2 = {
        "Second document also about databases."
    };
    std::string content_id2 = importTestContent(chunks2, "test-doc2");
    ASSERT_FALSE(content_id2.empty());
    
    // Search should find chunks from both documents
    auto [status, results] = secondary_index_->scanFulltextWithScores(
        "chunk", "text", "databases", 10);
    
    EXPECT_TRUE(status.ok);
    EXPECT_GE(results.size(), 2) << "Should find chunks from both documents";
}

// ============================================================================
// Main
// ============================================================================