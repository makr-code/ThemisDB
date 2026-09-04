// Unit tests for DiskANN (Phase 3)
// Based on "DiskANN: Fast Accurate Billion-point Nearest Neighbor Search" (NeurIPS'19)

#include <gtest/gtest.h>
#include "performance/phase3/diskann.h"
#include <filesystem>
#include <random>

using namespace themis::performance::phase3;

class DiskANNTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = std::filesystem::temp_directory_path() / "themis_diskann_test";
        std::filesystem::create_directories(test_dir_);
    }

    void TearDown() override {
        std::filesystem::remove_all(test_dir_);
    }

    std::filesystem::path test_dir_;
};

// ==================== LRUCache Tests ====================

TEST(DiskANNLRUCachePerformanceTest, BasicOperations) {
    LRUCache<int, std::string> cache(3);
    
    cache.put(1, "one");
    cache.put(2, "two");
    cache.put(3, "three");
    
    std::string value = {};
    EXPECT_TRUE(cache.get(1, value));
    EXPECT_EQ(value, "one");
    
    EXPECT_TRUE(cache.get(2, value));
    EXPECT_EQ(value, "two");
    
    EXPECT_FALSE(cache.get(99, value));
}

TEST(DiskANNLRUCachePerformanceTest, Eviction) {
    LRUCache<int, std::string> cache(2);
    
    cache.put(1, "one");
    cache.put(2, "two");
    cache.put(3, "three");  // Should evict one entry
    
    EXPECT_EQ(cache.size(), 2u);
}

// ==================== VantagePointTree Tests ====================

TEST(VantagePointTreeTest, Construction) {
    std::vector<std::pair<VectorID, std::vector<float>>> vectors = {
        {1, {1.0f, 0.0f}},
        {2, {0.0f, 1.0f}},
        {3, {1.0f, 1.0f}}
    };
    
    VantagePointTree vp_tree(vectors);
    
    // Find entry point for query
    std::vector<float> query = {0.9f, 0.1f};
    VectorID entry = vp_tree.find_entry_point(query);
    
    // Should return closest vector (ID 1)
    EXPECT_EQ(entry, 1u);
}

TEST(VantagePointTreeTest, EmptyTree) {
    std::vector<std::pair<VectorID, std::vector<float>>> vectors;
    VantagePointTree vp_tree(vectors);
    
    std::vector<float> query = {1.0f, 0.0f};
    EXPECT_THROW(vp_tree.find_entry_point(query), std::runtime_error);
}

// ==================== DiskANNIndex Tests ====================

TEST_F(DiskANNTest, ConstructionAndDestruction) {
    auto index_path = test_dir_ / "test_index.bin";
    
    {
        DiskANNIndex index(4, index_path.string(), 10);
        // Index should be created
        EXPECT_TRUE(std::filesystem::exists(index_path));
    }
    
    // File should exist after destruction
    EXPECT_TRUE(std::filesystem::exists(index_path));
}

TEST_F(DiskANNTest, BuildSmallIndex) {
    auto index_path = test_dir_ / "small_index.bin";
    DiskANNIndex index(2, index_path.string(), 10);
    
    // Build index with 5 vectors
    std::vector<std::pair<VectorID, std::vector<float>>> vectors = {
        {1, {1.0f, 0.0f}},
        {2, {0.0f, 1.0f}},
        {3, {1.0f, 1.0f}},
        {4, {0.5f, 0.5f}},
        {5, {0.8f, 0.2f}}
    };
    
    index.build(vectors);
    
    auto stats = index.get_stats();
    EXPECT_EQ(stats.num_vectors, 5u);
}

TEST_F(DiskANNTest, SearchNearest) {
    auto index_path = test_dir_ / "search_index.bin";
    DiskANNIndex index(2, index_path.string(), 10);
    
    // Build index
    std::vector<std::pair<VectorID, std::vector<float>>> vectors = {
        {1, {1.0f, 0.0f}},
        {2, {0.0f, 1.0f}},
        {3, {1.0f, 1.0f}},
        {4, {0.0f, 0.0f}}
    };
    
    index.build(vectors);
    
    // Search for nearest neighbor to (0.9, 0.1)
    std::vector<float> query = {0.9f, 0.1f};
    auto results = index.search(query, 2);
    
    EXPECT_GE(results.size(), 1u);
    EXPECT_LE(results.size(), 2u);
    
    // First result should be closest (ID 1)
    EXPECT_EQ(results[0].id, 1u);
}

TEST_F(DiskANNTest, AddVector) {
    auto index_path = test_dir_ / "add_index.bin";
    DiskANNIndex index(2, index_path.string(), 10);
    
    // Build initial index
    std::vector<std::pair<VectorID, std::vector<float>>> vectors = {
        {1, {1.0f, 0.0f}},
        {2, {0.0f, 1.0f}}
    };
    
    index.build(vectors);
    
    // Add new vector
    index.add(3, {0.5f, 0.5f});
    
    auto stats = index.get_stats();
    EXPECT_EQ(stats.num_vectors, 3u);
}

TEST_F(DiskANNTest, CacheStatistics) {
    auto index_path = test_dir_ / "cache_index.bin";
    DiskANNIndex index(2, index_path.string(), 1);  // Small cache
    
    // Build index
    std::vector<std::pair<VectorID, std::vector<float>>> vectors = {
        {1, {1.0f, 0.0f}},
        {2, {0.0f, 1.0f}},
        {3, {1.0f, 1.0f}}
    };
    
    index.build(vectors);
    
    // Perform searches to generate cache statistics
    std::vector<float> query = {0.5f, 0.5f};
    index.search(query, 1);
    index.search(query, 1);  // Should have some cache hits
    
    auto stats = index.get_stats();
    EXPECT_GT(stats.cache_hits + stats.cache_misses, 0u);
}

TEST_F(DiskANNTest, GraphEdgesTracked) {
    auto index_path = test_dir_ / "edges_index.bin";
    DiskANNIndex index(2, index_path.string(), 10);

    std::vector<std::pair<VectorID, std::vector<float>>> vectors = {
        {1, {1.0f, 0.0f}},
        {2, {0.0f, 1.0f}},
        {3, {1.0f, 1.0f}},
        {4, {0.5f, 0.5f}}
    };

    index.build(vectors);

    auto stats = index.get_stats();
    EXPECT_GT(stats.graph_edges, 0u);
}

TEST_F(DiskANNTest, SaveLoadMetadata) {
    auto index_path = test_dir_ / "meta_index.bin";
    DiskANNIndex index(2, index_path.string(), 10);

    std::vector<std::pair<VectorID, std::vector<float>>> vectors = {
        {1, {1.0f, 0.0f}},
        {2, {0.0f, 1.0f}},
        {3, {1.0f, 1.0f}}
    };

    index.build(vectors);
    ASSERT_TRUE(index.save(index_path.string()));

    // Load into a fresh index instance
    DiskANNIndex index2(2, index_path.string(), 10);
    ASSERT_TRUE(index2.load(index_path.string()));

    auto stats = index2.get_stats();
    EXPECT_EQ(stats.num_vectors, 3u);
    EXPECT_GT(stats.graph_edges, 0u);
}

TEST_F(DiskANNTest, LargerDataset) {
    auto index_path = test_dir_ / "large_index.bin";
    DiskANNIndex index(4, index_path.string(), 100);
    
    // Generate 100 random vectors
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    
    std::vector<std::pair<VectorID, std::vector<float>>> vectors;
    for (VectorID i = 1; i <= 100; i++) {
        std::vector<float> vec(4);
        for (int j = 0; j < 4; j++) {
            vec[j] = dist(rng);
        }
        vectors.push_back({i, vec});
    }
    
    index.build(vectors);
    
    // Search
    std::vector<float> query = {0.5f, 0.5f, 0.5f, 0.5f};
    auto results = index.search(query, 10);
    
    EXPECT_EQ(results.size(), 10u);
    
    // Results should be sorted by distance
    for (size_t i = 1; i < results.size(); i++) {
        EXPECT_LE(results[i-1].distance, results[i].distance);
    }
}

TEST_F(DiskANNTest, EmptyIndexSearch) {
    auto index_path = test_dir_ / "empty_index.bin";
    DiskANNIndex index(2, index_path.string(), 10);
    
    std::vector<float> query = {1.0f, 0.0f};
    auto results = index.search(query, 5);
    
    EXPECT_TRUE(results.empty());
}

TEST_F(DiskANNTest, DimensionMismatch) {
    auto index_path = test_dir_ / "dim_index.bin";
    DiskANNIndex index(2, index_path.string(), 10);
    
    // Try to add vector with wrong dimension
    EXPECT_THROW(index.add(1, {1.0f, 0.0f, 0.0f}), std::invalid_argument);
}


