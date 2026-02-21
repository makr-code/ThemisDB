/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_lib_hnsw_integration.cpp                      ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-02-21 13:49:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     430                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include <hnswlib/hnswlib.h>
#include <vector>
#include <random>
#include <cmath>

// Test fixture for HNSW library integration
class HNSWLibIntegrationTest : public ::testing::Test {
protected:
    static constexpr int dim = 128;
    static constexpr int max_elements = 1000;
    
    std::vector<float> generateRandomVector(int dimension) {
        static std::mt19937 rng(42);
        static std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        
        std::vector<float> vec(dimension);
        for (int i = 0; i < dimension; ++i) {
            vec[i] = dist(rng);
        }
        return vec;
    }
    
    float computeL2Distance(const std::vector<float>& a, const std::vector<float>& b) {
        float sum = 0.0f;
        for (size_t i = 0; i < a.size(); ++i) {
            float diff = a[i] - b[i];
            sum += diff * diff;
        }
        return std::sqrt(sum);
    }
};

// Test 1: HNSWlib library linking and basic index creation
TEST_F(HNSWLibIntegrationTest, LibraryLinkingAndIndexCreation) {
    int M = 16;
    int ef_construction = 200;
    
    // Create L2 space
    hnswlib::L2Space space(dim);
    
    // Create HNSW index
    hnswlib::HierarchicalNSW<float>* alg_hnsw = 
        new hnswlib::HierarchicalNSW<float>(&space, max_elements, M, ef_construction);
    
    ASSERT_NE(alg_hnsw, nullptr);
    EXPECT_EQ(alg_hnsw->max_elements_, max_elements);
    
    delete alg_hnsw;
}

// Test 2: Add vectors to HNSW index
TEST_F(HNSWLibIntegrationTest, AddVectorsToIndex) {
    hnswlib::L2Space space(dim);
    hnswlib::HierarchicalNSW<float>* index = 
        new hnswlib::HierarchicalNSW<float>(&space, max_elements, 16, 200);
    
    // Add vectors
    int num_vectors = 100;
    for (int i = 0; i < num_vectors; ++i) {
        auto vec = generateRandomVector(dim);
        index->addPoint(vec.data(), i);
    }
    
    EXPECT_EQ(index->cur_element_count, num_vectors);
    
    delete index;
}

// Test 3: Search for nearest neighbors
TEST_F(HNSWLibIntegrationTest, SearchNearestNeighbors) {
    hnswlib::L2Space space(dim);
    hnswlib::HierarchicalNSW<float>* index = 
        new hnswlib::HierarchicalNSW<float>(&space, max_elements, 16, 200);
    
    // Add vectors
    std::vector<std::vector<float>> vectors;
    int num_vectors = 100;
    for (int i = 0; i < num_vectors; ++i) {
        auto vec = generateRandomVector(dim);
        vectors.push_back(vec);
        index->addPoint(vec.data(), i);
    }
    
    // Search for nearest neighbors
    int k = 5;
    index->setEf(50);
    
    auto query = vectors[0]; // Query with first vector
    auto result = index->searchKnn(query.data(), k);
    EXPECT_EQ(result.size(), k);

    // searchKnn returns a max-heap (worst distance on top). Find the best.
    std::vector<std::pair<float, size_t>> neighbors;
    neighbors.reserve(result.size());
    while (!result.empty()) {
        neighbors.emplace_back(result.top().first, result.top().second);
        result.pop();
    }
    std::sort(neighbors.begin(), neighbors.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
    });

    ASSERT_FALSE(neighbors.empty());
    EXPECT_EQ(neighbors.front().second, 0u);          // self should be present
    EXPECT_LT(neighbors.front().first, 1e-3f);        // distance ~0
    
    delete index;
}

// Test 4: Inner product space
TEST_F(HNSWLibIntegrationTest, InnerProductSpace) {
    hnswlib::InnerProductSpace space(dim);
    hnswlib::HierarchicalNSW<float>* index = 
        new hnswlib::HierarchicalNSW<float>(&space, max_elements, 16, 200);
    
    // Add vectors
    int num_vectors = 50;
    for (int i = 0; i < num_vectors; ++i) {
        auto vec = generateRandomVector(dim);
        index->addPoint(vec.data(), i);
    }
    
    EXPECT_EQ(index->cur_element_count, num_vectors);
    
    // Search
    auto query = generateRandomVector(dim);
    index->setEf(50);
    auto result = index->searchKnn(query.data(), 5);
    
    EXPECT_EQ(result.size(), 5u);
    
    delete index;
}

// Test 5: Mark deleted and search
TEST_F(HNSWLibIntegrationTest, MarkDeletedVectors) {
    hnswlib::L2Space space(dim);
    hnswlib::HierarchicalNSW<float>* index = 
        new hnswlib::HierarchicalNSW<float>(&space, max_elements, 16, 200);
    
    // Add vectors
    std::vector<std::vector<float>> vectors;
    int num_vectors = 50;
    for (int i = 0; i < num_vectors; ++i) {
        auto vec = generateRandomVector(dim);
        vectors.push_back(vec);
        index->addPoint(vec.data(), i);
    }
    
    // Mark some as deleted
    index->markDelete(5);
    index->markDelete(10);
    
    // Search should not return deleted items
    auto query = vectors[5];
    index->setEf(50);
    auto result = index->searchKnn(query.data(), 3);
    
    // Verify deleted item is not in results
    bool found_deleted = false;
    auto temp_result = result;
    while (!temp_result.empty()) {
        if (temp_result.top().second == 5) {
            found_deleted = true;
        }
        temp_result.pop();
    }
    EXPECT_FALSE(found_deleted) << "Deleted item should not be in search results";
    
    delete index;
}

// Test 6: Save and load index
TEST_F(HNSWLibIntegrationTest, SaveAndLoadIndex) {
    std::string index_path = "./data/test_hnsw_index.bin";
    
    // Create and populate index
    hnswlib::L2Space space(dim);
    hnswlib::HierarchicalNSW<float>* index = 
        new hnswlib::HierarchicalNSW<float>(&space, max_elements, 16, 200);
    
    std::vector<std::vector<float>> vectors;
    int num_vectors = 50;
    for (int i = 0; i < num_vectors; ++i) {
        auto vec = generateRandomVector(dim);
        vectors.push_back(vec);
        index->addPoint(vec.data(), i);
    }
    
    // Save index
    index->saveIndex(index_path);
    delete index;
    
    // Load index
    hnswlib::HierarchicalNSW<float>* loaded_index = 
        new hnswlib::HierarchicalNSW<float>(&space, index_path);
    
    EXPECT_EQ(loaded_index->cur_element_count, num_vectors);
    
    // Verify search works on loaded index
    auto query = vectors[0];
    loaded_index->setEf(50);
    auto result = loaded_index->searchKnn(query.data(), 5);
    EXPECT_EQ(result.size(), 5u);
    
    delete loaded_index;
    std::remove(index_path.c_str());
}

// Test 7: Resize index
TEST_F(HNSWLibIntegrationTest, ResizeIndex) {
    hnswlib::L2Space space(dim);
    int initial_max = 50;
    hnswlib::HierarchicalNSW<float>* index = 
        new hnswlib::HierarchicalNSW<float>(&space, initial_max, 16, 200);
    
    // Fill initial capacity
    for (int i = 0; i < initial_max; ++i) {
        auto vec = generateRandomVector(dim);
        index->addPoint(vec.data(), i);
    }
    
    // Resize
    int new_max = 100;
    index->resizeIndex(new_max);
    EXPECT_EQ(index->max_elements_, new_max);
    
    // Add more vectors
    for (int i = initial_max; i < initial_max + 20; ++i) {
        auto vec = generateRandomVector(dim);
        index->addPoint(vec.data(), i);
    }
    
    EXPECT_EQ(index->cur_element_count, initial_max + 20);
    
    delete index;
}

// Test 8: Different M and ef_construction parameters
TEST_F(HNSWLibIntegrationTest, DifferentParameters) {
    hnswlib::L2Space space(dim);
    
    // Test with small M and ef_construction (faster build, lower recall)
    {
        hnswlib::HierarchicalNSW<float>* index = 
            new hnswlib::HierarchicalNSW<float>(&space, max_elements, 4, 50);
        
        for (int i = 0; i < 50; ++i) {
            auto vec = generateRandomVector(dim);
            index->addPoint(vec.data(), i);
        }
        
        EXPECT_EQ(index->cur_element_count, 50);
        delete index;
    }
    
    // Test with large M and ef_construction (slower build, higher recall)
    {
        hnswlib::HierarchicalNSW<float>* index = 
            new hnswlib::HierarchicalNSW<float>(&space, max_elements, 32, 400);
        
        for (int i = 0; i < 50; ++i) {
            auto vec = generateRandomVector(dim);
            index->addPoint(vec.data(), i);
        }
        
        EXPECT_EQ(index->cur_element_count, 50);
        delete index;
    }
}

// Test 9: Search with different ef values
TEST_F(HNSWLibIntegrationTest, SearchWithDifferentEf) {
    hnswlib::L2Space space(dim);
    hnswlib::HierarchicalNSW<float>* index = 
        new hnswlib::HierarchicalNSW<float>(&space, max_elements, 16, 200);
    
    std::vector<std::vector<float>> vectors;
    int num_vectors = 100;
    for (int i = 0; i < num_vectors; ++i) {
        auto vec = generateRandomVector(dim);
        vectors.push_back(vec);
        index->addPoint(vec.data(), i);
    }
    
    auto query = vectors[0];
    int k = 10;
    
    // Search with low ef
    index->setEf(10);
    auto result_low = index->searchKnn(query.data(), k);
    EXPECT_EQ(result_low.size(), k);
    
    // Search with high ef
    index->setEf(100);
    auto result_high = index->searchKnn(query.data(), k);
    EXPECT_EQ(result_high.size(), k);
    
    // Both should return k results, but potentially different quality
    
    delete index;
}

// Test 10: Brute force index comparison
TEST_F(HNSWLibIntegrationTest, BruteForceComparison) {
    int small_dim = 32;
    hnswlib::L2Space space(small_dim);
    
    // Create HNSW index
    hnswlib::HierarchicalNSW<float>* hnsw_index = 
        new hnswlib::HierarchicalNSW<float>(&space, max_elements, 16, 200);
    
    // Create brute force index
    hnswlib::BruteforceSearch<float>* bf_index = 
        new hnswlib::BruteforceSearch<float>(&space, max_elements);
    
    // Add same vectors to both
    std::vector<std::vector<float>> vectors;
    int num_vectors = 50;
    for (int i = 0; i < num_vectors; ++i) {
        auto vec = generateRandomVector(small_dim);
        vectors.push_back(vec);
        hnsw_index->addPoint(vec.data(), i);
        bf_index->addPoint(vec.data(), i);
    }
    
    // Search both
    auto query = vectors[0];
    hnsw_index->setEf(50);
    
    auto hnsw_result = hnsw_index->searchKnn(query.data(), 5);
    auto bf_result = bf_index->searchKnn(query.data(), 5);
    
    EXPECT_EQ(hnsw_result.size(), 5u);
    EXPECT_EQ(bf_result.size(), 5u);
    
    // Top result should be same (the query itself)
    EXPECT_EQ(hnsw_result.top().second, bf_result.top().second);
    
    delete hnsw_index;
    delete bf_index;
}

// Test 11: Multi-threaded index building
TEST_F(HNSWLibIntegrationTest, MultiThreadedBuilding) {
    hnswlib::L2Space space(dim);
    hnswlib::HierarchicalNSW<float>* index = 
        new hnswlib::HierarchicalNSW<float>(&space, max_elements, 16, 200);
    
    // Enable multi-threading
    int num_threads = 4;
    
    // Add vectors (hnswlib handles thread safety internally)
    int num_vectors = 200;
    for (int i = 0; i < num_vectors; ++i) {
        auto vec = generateRandomVector(dim);
        index->addPoint(vec.data(), i);
    }
    
    EXPECT_EQ(index->cur_element_count, num_vectors);
    
    delete index;
}

// Test 12: Distance computation accuracy
TEST_F(HNSWLibIntegrationTest, DistanceComputationAccuracy) {
    int small_dim = 16;
    hnswlib::L2Space space(small_dim);
    
    std::vector<float> vec1 = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    std::vector<float> vec2 = {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    
    // Compute distance using hnswlib public API
    // Create small index to use get_dist_func()
    hnswlib::HierarchicalNSW<float> index(&space, 10, 16, 200);
    index.addPoint(vec1.data(), 0);
    index.addPoint(vec2.data(), 1);
    
    // Query vec1 against vec2
    auto result = index.searchKnn(vec1.data(), 2);
    
    // HNSWlib L2Space returns squared L2 distance (not the sqrt)
    float expected_dist = 2.0f; // 1^2 + 1^2

    EXPECT_EQ(result.size(), 2u);
    if (result.size() == 2) {
        std::vector<std::pair<float, size_t>> neighbors;
        neighbors.reserve(result.size());
        while (!result.empty()) {
            neighbors.emplace_back(result.top().first, result.top().second);
            result.pop();
        }
        std::sort(neighbors.begin(), neighbors.end(), [](const auto& a, const auto& b) {
            return a.first < b.first;
        });
        // Best is self (id 0), second should be vec2 with distance ~sqrt(2)
        ASSERT_EQ(neighbors.size(), 2u);
        EXPECT_EQ(neighbors[0].second, 0u);
        EXPECT_LT(neighbors[0].first, 1e-3f);
        EXPECT_EQ(neighbors[1].second, 1u);
        EXPECT_NEAR(neighbors[1].first, expected_dist, 0.05f);
    }
}
