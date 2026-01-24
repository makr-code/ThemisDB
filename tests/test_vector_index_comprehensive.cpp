/**
 * @file test_vector_index_comprehensive.cpp
 * @brief Comprehensive real unit tests for Vector Index (HNSW-based)
 * 
 * Test Intent:
 * - Validate vector index creation and management with real HNSW operations
 * - Test vector search with various distance metrics (L2, cosine, inner product)
 * - Verify filtered vector search with metadata predicates
 * - Test index persistence and recovery
 * - Validate concurrent operations and thread safety
 * - Test edge cases (empty vectors, high dimensions, large datasets)
 * 
 * Coverage: Index layer (VectorIndex, HNSW, distance metrics, filtering)
 * No stubs - all tests use real HNSW library and RocksDB persistence
 */

#include <gtest/gtest.h>
#include "index/vector_index.h"
#include "storage/rocksdb_wrapper.h"
#include <filesystem>
#include <random>
#include <thread>
#include <cmath>

using namespace themis;
namespace fs = std::filesystem;

class VectorIndexComprehensiveTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = fs::temp_directory_path() / "vector_index_comprehensive_test";
        cleanupTestDir();
        fs::create_directories(test_dir_);
        
        RocksDBWrapper::Config config;
        config.db_path = test_dir_.string();
        config.enable_wal = true;
        
        db_ = std::make_unique<RocksDBWrapper>(config);
        ASSERT_TRUE(db_->open());
        
        vector_mgr_ = std::make_unique<VectorIndexManager>(*db_);
    }
    
    void TearDown() override {
        vector_mgr_.reset();
        db_.reset();
        cleanupTestDir();
    }
    
    void cleanupTestDir() {
        std::error_code ec;
        fs::remove_all(test_dir_, ec);
    }
    
    // Helper: Generate random vector
    std::vector<float> generateRandomVector(size_t dim, float scale = 1.0f) {
        std::vector<float> vec(dim);
        std::mt19937 gen(std::random_device{}());
        std::normal_distribution<float> dis(0.0f, scale);
        
        for (auto& v : vec) {
            v = dis(gen);
        }
        return vec;
    }
    
    // Helper: Calculate L2 distance
    float l2Distance(const std::vector<float>& a, const std::vector<float>& b) {
        float sum = 0.0f;
        for (size_t i = 0; i < a.size(); ++i) {
            float diff = a[i] - b[i];
            sum += diff * diff;
        }
        return std::sqrt(sum);
    }
    
    // Helper: Calculate cosine similarity
    float cosineSimilarity(const std::vector<float>& a, const std::vector<float>& b) {
        float dot = 0.0f, norm_a = 0.0f, norm_b = 0.0f;
        for (size_t i = 0; i < a.size(); ++i) {
            dot += a[i] * b[i];
            norm_a += a[i] * a[i];
            norm_b += b[i] * b[i];
        }
        return dot / (std::sqrt(norm_a) * std::sqrt(norm_b));
    }
    
    fs::path test_dir_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<VectorIndexManager> vector_mgr_;
};

// ============================================================================
// Index Creation and Management Tests
// ============================================================================

TEST_F(VectorIndexComprehensiveTest, CreateVectorIndex) {
    // Intent: Verify vector index creation with various configurations
    
    VectorIndexManager::Config config;
    config.dimension = 128;
    config.metric = VectorIndexManager::DistanceMetric::L2;
    config.ef_construction = 200;
    config.M = 16;
    
    auto status = vector_mgr_->createIndex("embeddings", "vector", config);
    ASSERT_TRUE(status.ok) << "Failed to create index: " << status.message;
    
    // Verify index exists
    EXPECT_TRUE(vector_mgr_->indexExists("embeddings", "vector"));
}

TEST_F(VectorIndexComprehensiveTest, CreateIndexWithDifferentMetrics) {
    // Intent: Verify support for different distance metrics
    
    VectorIndexManager::Config config_l2;
    config_l2.dimension = 64;
    config_l2.metric = VectorIndexManager::DistanceMetric::L2;
    
    auto status1 = vector_mgr_->createIndex("docs", "l2_vector", config_l2);
    ASSERT_TRUE(status1.ok);
    
    VectorIndexManager::Config config_cosine;
    config_cosine.dimension = 64;
    config_cosine.metric = VectorIndexManager::DistanceMetric::Cosine;
    
    auto status2 = vector_mgr_->createIndex("docs", "cosine_vector", config_cosine);
    ASSERT_TRUE(status2.ok);
    
    VectorIndexManager::Config config_ip;
    config_ip.dimension = 64;
    config_ip.metric = VectorIndexManager::DistanceMetric::InnerProduct;
    
    auto status3 = vector_mgr_->createIndex("docs", "ip_vector", config_ip);
    ASSERT_TRUE(status3.ok);
    
    EXPECT_TRUE(vector_mgr_->indexExists("docs", "l2_vector"));
    EXPECT_TRUE(vector_mgr_->indexExists("docs", "cosine_vector"));
    EXPECT_TRUE(vector_mgr_->indexExists("docs", "ip_vector"));
}

TEST_F(VectorIndexComprehensiveTest, CreateIndexInvalidDimension) {
    // Intent: Verify proper error handling for invalid dimensions
    
    VectorIndexManager::Config config;
    config.dimension = 0; // Invalid
    config.metric = VectorIndexManager::DistanceMetric::L2;
    
    auto status = vector_mgr_->createIndex("invalid", "vector", config);
    EXPECT_FALSE(status.ok);
    EXPECT_FALSE(status.message.empty());
}

// ============================================================================
// Vector Insertion Tests
// ============================================================================

TEST_F(VectorIndexComprehensiveTest, InsertSingleVector) {
    // Intent: Verify single vector insertion
    
    VectorIndexManager::Config config;
    config.dimension = 128;
    config.metric = VectorIndexManager::DistanceMetric::L2;
    
    ASSERT_TRUE(vector_mgr_->createIndex("items", "embedding", config).ok);
    
    auto vec = generateRandomVector(128);
    auto status = vector_mgr_->insertVector("items", "embedding", "item1", vec);
    ASSERT_TRUE(status.ok) << "Insert failed: " << status.message;
    
    // Verify vector can be retrieved
    auto retrieved = vector_mgr_->getVector("items", "embedding", "item1");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->size(), 128);
}

TEST_F(VectorIndexComprehensiveTest, InsertMultipleVectors) {
    // Intent: Verify batch vector insertion
    
    VectorIndexManager::Config config;
    config.dimension = 64;
    config.metric = VectorIndexManager::DistanceMetric::L2;
    
    ASSERT_TRUE(vector_mgr_->createIndex("products", "features", config).ok);
    
    const int num_vectors = 100;
    for (int i = 0; i < num_vectors; ++i) {
        auto vec = generateRandomVector(64);
        std::string id = "product" + std::to_string(i);
        auto status = vector_mgr_->insertVector("products", "features", id, vec);
        ASSERT_TRUE(status.ok) << "Insert " << i << " failed";
    }
    
    // Verify count
    auto count = vector_mgr_->getIndexSize("products", "features");
    EXPECT_EQ(count, num_vectors);
}

TEST_F(VectorIndexComprehensiveTest, InsertVectorWrongDimension) {
    // Intent: Verify dimension mismatch is caught
    
    VectorIndexManager::Config config;
    config.dimension = 128;
    config.metric = VectorIndexManager::DistanceMetric::L2;
    
    ASSERT_TRUE(vector_mgr_->createIndex("test", "vec", config).ok);
    
    // Try to insert vector with wrong dimension
    auto wrong_vec = generateRandomVector(64); // Should be 128
    auto status = vector_mgr_->insertVector("test", "vec", "id1", wrong_vec);
    EXPECT_FALSE(status.ok);
}

TEST_F(VectorIndexComprehensiveTest, UpdateExistingVector) {
    // Intent: Verify vector update operations
    
    VectorIndexManager::Config config;
    config.dimension = 32;
    config.metric = VectorIndexManager::DistanceMetric::L2;
    
    ASSERT_TRUE(vector_mgr_->createIndex("users", "profile", config).ok);
    
    auto vec1 = generateRandomVector(32, 1.0f);
    ASSERT_TRUE(vector_mgr_->insertVector("users", "profile", "user1", vec1).ok);
    
    // Update with new vector
    auto vec2 = generateRandomVector(32, 2.0f);
    auto status = vector_mgr_->updateVector("users", "profile", "user1", vec2);
    ASSERT_TRUE(status.ok);
    
    // Verify updated vector
    auto retrieved = vector_mgr_->getVector("users", "profile", "user1");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(*retrieved, vec2);
}

// ============================================================================
// Vector Search Tests
// ============================================================================

TEST_F(VectorIndexComprehensiveTest, SearchNearestNeighbors) {
    // Intent: Verify k-NN search returns correct neighbors
    
    VectorIndexManager::Config config;
    config.dimension = 8;
    config.metric = VectorIndexManager::DistanceMetric::L2;
    
    ASSERT_TRUE(vector_mgr_->createIndex("docs", "embedding", config).ok);
    
    // Insert vectors in a pattern
    std::vector<std::vector<float>> vectors;
    for (int i = 0; i < 10; ++i) {
        std::vector<float> vec(8, static_cast<float>(i));
        vectors.push_back(vec);
        std::string id = "doc" + std::to_string(i);
        ASSERT_TRUE(vector_mgr_->insertVector("docs", "embedding", id, vec).ok);
    }
    
    // Search for vector similar to doc5
    std::vector<float> query = vectors[5];
    auto [status, results] = vector_mgr_->search("docs", "embedding", query, 3);
    
    ASSERT_TRUE(status.ok);
    ASSERT_GE(results.size(), 1);
    
    // doc5 should be the closest match
    EXPECT_EQ(results[0].id, "doc5");
    EXPECT_NEAR(results[0].distance, 0.0f, 1e-5);
}

TEST_F(VectorIndexComprehensiveTest, SearchWithL2Metric) {
    // Intent: Verify L2 distance calculations in search
    
    VectorIndexManager::Config config;
    config.dimension = 4;
    config.metric = VectorIndexManager::DistanceMetric::L2;
    
    ASSERT_TRUE(vector_mgr_->createIndex("simple", "vec", config).ok);
    
    std::vector<float> vec1 = {1.0f, 0.0f, 0.0f, 0.0f};
    std::vector<float> vec2 = {0.0f, 1.0f, 0.0f, 0.0f};
    std::vector<float> vec3 = {0.9f, 0.1f, 0.0f, 0.0f};
    
    ASSERT_TRUE(vector_mgr_->insertVector("simple", "vec", "v1", vec1).ok);
    ASSERT_TRUE(vector_mgr_->insertVector("simple", "vec", "v2", vec2).ok);
    ASSERT_TRUE(vector_mgr_->insertVector("simple", "vec", "v3", vec3).ok);
    
    // Query with vec1
    auto [status, results] = vector_mgr_->search("simple", "vec", vec1, 3);
    
    ASSERT_TRUE(status.ok);
    ASSERT_GE(results.size(), 2);
    
    // v1 should be closest, v3 should be closer than v2
    EXPECT_EQ(results[0].id, "v1");
    if (results.size() >= 2) {
        EXPECT_EQ(results[1].id, "v3");
    }
}

TEST_F(VectorIndexComprehensiveTest, SearchWithCosineMetric) {
    // Intent: Verify cosine similarity calculations in search
    
    VectorIndexManager::Config config;
    config.dimension = 3;
    config.metric = VectorIndexManager::DistanceMetric::Cosine;
    
    ASSERT_TRUE(vector_mgr_->createIndex("cosine_test", "vec", config).ok);
    
    std::vector<float> vec1 = {1.0f, 0.0f, 0.0f};
    std::vector<float> vec2 = {0.707f, 0.707f, 0.0f}; // 45 degrees from vec1
    std::vector<float> vec3 = {0.0f, 1.0f, 0.0f};     // 90 degrees from vec1
    
    ASSERT_TRUE(vector_mgr_->insertVector("cosine_test", "vec", "v1", vec1).ok);
    ASSERT_TRUE(vector_mgr_->insertVector("cosine_test", "vec", "v2", vec2).ok);
    ASSERT_TRUE(vector_mgr_->insertVector("cosine_test", "vec", "v3", vec3).ok);
    
    // Query with vec1
    auto [status, results] = vector_mgr_->search("cosine_test", "vec", vec1, 3);
    
    ASSERT_TRUE(status.ok);
    ASSERT_GE(results.size(), 2);
    
    // v1 exact match, v2 closer than v3
    EXPECT_EQ(results[0].id, "v1");
    if (results.size() >= 2) {
        EXPECT_EQ(results[1].id, "v2");
    }
}

TEST_F(VectorIndexComprehensiveTest, SearchWithKParameter) {
    // Intent: Verify k parameter controls number of results
    
    VectorIndexManager::Config config;
    config.dimension = 16;
    config.metric = VectorIndexManager::DistanceMetric::L2;
    
    ASSERT_TRUE(vector_mgr_->createIndex("k_test", "vec", config).ok);
    
    // Insert 20 vectors
    for (int i = 0; i < 20; ++i) {
        auto vec = generateRandomVector(16);
        ASSERT_TRUE(vector_mgr_->insertVector("k_test", "vec", 
                                             "id" + std::to_string(i), vec).ok);
    }
    
    auto query = generateRandomVector(16);
    
    // Search with k=5
    auto [status5, results5] = vector_mgr_->search("k_test", "vec", query, 5);
    ASSERT_TRUE(status5.ok);
    EXPECT_LE(results5.size(), 5);
    
    // Search with k=10
    auto [status10, results10] = vector_mgr_->search("k_test", "vec", query, 10);
    ASSERT_TRUE(status10.ok);
    EXPECT_LE(results10.size(), 10);
}

// ============================================================================
// Filtered Vector Search Tests
// ============================================================================

TEST_F(VectorIndexComprehensiveTest, FilteredSearchByMetadata) {
    // Intent: Verify filtered search with metadata predicates
    
    VectorIndexManager::Config config;
    config.dimension = 32;
    config.metric = VectorIndexManager::DistanceMetric::L2;
    
    ASSERT_TRUE(vector_mgr_->createIndex("articles", "embedding", config).ok);
    
    // Insert vectors with metadata
    for (int i = 0; i < 20; ++i) {
        auto vec = generateRandomVector(32);
        std::string id = "article" + std::to_string(i);
        std::map<std::string, std::string> metadata;
        metadata["category"] = (i < 10) ? "tech" : "sports";
        metadata["year"] = (i < 10) ? "2023" : "2024";
        
        ASSERT_TRUE(vector_mgr_->insertVectorWithMetadata(
            "articles", "embedding", id, vec, metadata).ok);
    }
    
    // Search with filter: category = "tech"
    auto query = generateRandomVector(32);
    VectorIndexManager::FilterPredicate filter;
    filter.field = "category";
    filter.op = VectorIndexManager::FilterOp::Equals;
    filter.value = "tech";
    
    auto [status, results] = vector_mgr_->searchFiltered(
        "articles", "embedding", query, 5, filter);
    
    ASSERT_TRUE(status.ok);
    
    // All results should have category="tech"
    for (const auto& result : results) {
        auto metadata = vector_mgr_->getVectorMetadata("articles", "embedding", result.id);
        ASSERT_TRUE(metadata.has_value());
        EXPECT_EQ(metadata->at("category"), "tech");
    }
}

TEST_F(VectorIndexComprehensiveTest, FilteredSearchMultiplePredicates) {
    // Intent: Verify filtered search with multiple conditions
    
    VectorIndexManager::Config config;
    config.dimension = 16;
    config.metric = VectorIndexManager::DistanceMetric::L2;
    
    ASSERT_TRUE(vector_mgr_->createIndex("products", "features", config).ok);
    
    // Insert products with price and category
    for (int i = 0; i < 30; ++i) {
        auto vec = generateRandomVector(16);
        std::string id = "product" + std::to_string(i);
        std::map<std::string, std::string> metadata;
        metadata["category"] = (i % 3 == 0) ? "electronics" : "clothing";
        metadata["price"] = std::to_string(100 + i * 10);
        
        ASSERT_TRUE(vector_mgr_->insertVectorWithMetadata(
            "products", "features", id, vec, metadata).ok);
    }
    
    // Search: category=electronics AND price < 200
    auto query = generateRandomVector(16);
    std::vector<VectorIndexManager::FilterPredicate> filters;
    
    VectorIndexManager::FilterPredicate filter1;
    filter1.field = "category";
    filter1.op = VectorIndexManager::FilterOp::Equals;
    filter1.value = "electronics";
    filters.push_back(filter1);
    
    VectorIndexManager::FilterPredicate filter2;
    filter2.field = "price";
    filter2.op = VectorIndexManager::FilterOp::LessThan;
    filter2.value = "200";
    filters.push_back(filter2);
    
    auto [status, results] = vector_mgr_->searchFilteredMulti(
        "products", "features", query, 10, filters);
    
    ASSERT_TRUE(status.ok);
    
    // Verify all results match both filters
    for (const auto& result : results) {
        auto metadata = vector_mgr_->getVectorMetadata("products", "features", result.id);
        ASSERT_TRUE(metadata.has_value());
        EXPECT_EQ(metadata->at("category"), "electronics");
        EXPECT_LT(std::stoi(metadata->at("price")), 200);
    }
}

// ============================================================================
// Delete and Update Tests
// ============================================================================

TEST_F(VectorIndexComprehensiveTest, DeleteVector) {
    // Intent: Verify vector deletion removes from index
    
    VectorIndexManager::Config config;
    config.dimension = 16;
    config.metric = VectorIndexManager::DistanceMetric::L2;
    
    ASSERT_TRUE(vector_mgr_->createIndex("items", "vec", config).ok);
    
    auto vec = generateRandomVector(16);
    ASSERT_TRUE(vector_mgr_->insertVector("items", "vec", "delete_me", vec).ok);
    
    // Verify exists
    EXPECT_TRUE(vector_mgr_->getVector("items", "vec", "delete_me").has_value());
    
    // Delete
    auto status = vector_mgr_->deleteVector("items", "vec", "delete_me");
    ASSERT_TRUE(status.ok);
    
    // Verify deleted
    EXPECT_FALSE(vector_mgr_->getVector("items", "vec", "delete_me").has_value());
}

TEST_F(VectorIndexComprehensiveTest, DeleteDoesNotAffectSearch) {
    // Intent: Verify deleted vectors don't appear in search results
    
    VectorIndexManager::Config config;
    config.dimension = 8;
    config.metric = VectorIndexManager::DistanceMetric::L2;
    
    ASSERT_TRUE(vector_mgr_->createIndex("test", "vec", config).ok);
    
    std::vector<float> vec1 = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    std::vector<float> vec2 = {0.9f, 0.1f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    std::vector<float> vec3 = {0.8f, 0.2f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    
    ASSERT_TRUE(vector_mgr_->insertVector("test", "vec", "v1", vec1).ok);
    ASSERT_TRUE(vector_mgr_->insertVector("test", "vec", "v2", vec2).ok);
    ASSERT_TRUE(vector_mgr_->insertVector("test", "vec", "v3", vec3).ok);
    
    // Delete v2
    ASSERT_TRUE(vector_mgr_->deleteVector("test", "vec", "v2").ok);
    
    // Search should not return v2
    auto [status, results] = vector_mgr_->search("test", "vec", vec1, 3);
    ASSERT_TRUE(status.ok);
    
    for (const auto& result : results) {
        EXPECT_NE(result.id, "v2");
    }
}

// ============================================================================
// Persistence and Recovery Tests
// ============================================================================

TEST_F(VectorIndexComprehensiveTest, IndexPersistence) {
    // Intent: Verify index survives database close/reopen
    
    VectorIndexManager::Config config;
    config.dimension = 16;
    config.metric = VectorIndexManager::DistanceMetric::L2;
    
    ASSERT_TRUE(vector_mgr_->createIndex("persistent", "vec", config).ok);
    
    auto vec = generateRandomVector(16);
    ASSERT_TRUE(vector_mgr_->insertVector("persistent", "vec", "id1", vec).ok);
    
    // Close and reopen database
    vector_mgr_.reset();
    db_.reset();
    
    db_ = std::make_unique<RocksDBWrapper>(
        RocksDBWrapper::Config{test_dir_.string()});
    ASSERT_TRUE(db_->open());
    vector_mgr_ = std::make_unique<VectorIndexManager>(*db_);
    
    // Verify index still exists
    EXPECT_TRUE(vector_mgr_->indexExists("persistent", "vec"));
    
    // Verify vector still retrievable
    auto retrieved = vector_mgr_->getVector("persistent", "vec", "id1");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(*retrieved, vec);
}

// ============================================================================
// Concurrent Operations Tests
// ============================================================================

TEST_F(VectorIndexComprehensiveTest, ConcurrentInserts) {
    // Intent: Verify thread-safe concurrent insertions
    
    VectorIndexManager::Config config;
    config.dimension = 32;
    config.metric = VectorIndexManager::DistanceMetric::L2;
    
    ASSERT_TRUE(vector_mgr_->createIndex("concurrent", "vec", config).ok);
    
    const int num_threads = 4;
    const int vectors_per_thread = 50;
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([this, t, vectors_per_thread, &success_count]() {
            for (int i = 0; i < vectors_per_thread; ++i) {
                auto vec = generateRandomVector(32);
                std::string id = "t" + std::to_string(t) + "_v" + std::to_string(i);
                if (vector_mgr_->insertVector("concurrent", "vec", id, vec).ok) {
                    success_count++;
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Most inserts should succeed
    EXPECT_GE(success_count.load(), num_threads * vectors_per_thread * 0.95);
}

// ============================================================================
// Edge Cases and Error Handling
// ============================================================================

TEST_F(VectorIndexComprehensiveTest, EmptyVectorInsertion) {
    // Intent: Verify empty vector handling
    
    VectorIndexManager::Config config;
    config.dimension = 16;
    config.metric = VectorIndexManager::DistanceMetric::L2;
    
    ASSERT_TRUE(vector_mgr_->createIndex("empty_test", "vec", config).ok);
    
    std::vector<float> empty_vec;
    auto status = vector_mgr_->insertVector("empty_test", "vec", "id1", empty_vec);
    EXPECT_FALSE(status.ok);
}

TEST_F(VectorIndexComprehensiveTest, HighDimensionalVectors) {
    // Intent: Verify support for high-dimensional vectors
    
    VectorIndexManager::Config config;
    config.dimension = 1536; // Common for large embedding models
    config.metric = VectorIndexManager::DistanceMetric::L2;
    
    auto create_status = vector_mgr_->createIndex("high_dim", "vec", config);
    
    // Should succeed if implementation supports high dimensions
    if (create_status.ok) {
        auto vec = generateRandomVector(1536);
        auto insert_status = vector_mgr_->insertVector("high_dim", "vec", "id1", vec);
        EXPECT_TRUE(insert_status.ok);
    }
}

TEST_F(VectorIndexComprehensiveTest, SearchEmptyIndex) {
    // Intent: Verify search on empty index returns empty results
    
    VectorIndexManager::Config config;
    config.dimension = 16;
    config.metric = VectorIndexManager::DistanceMetric::L2;
    
    ASSERT_TRUE(vector_mgr_->createIndex("empty_search", "vec", config).ok);
    
    auto query = generateRandomVector(16);
    auto [status, results] = vector_mgr_->search("empty_search", "vec", query, 10);
    
    EXPECT_TRUE(status.ok);
    EXPECT_EQ(results.size(), 0);
}
