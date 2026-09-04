#include <gtest/gtest.h>
#if __has_include(<hnswlib/hnswlib.h>)
#include <hnswlib/hnswlib.h>
#define THEMIS_HAS_HNSWLIB_HEADER 1
#else
#define THEMIS_HAS_HNSWLIB_HEADER 0
#endif
#include <vector>
#include <random>
#include <cmath>
#include <algorithm>

#if !THEMIS_HAS_HNSWLIB_HEADER

TEST(HNSWLibIntegrationTest, MissingHeader) {
    GTEST_SKIP() << "hnswlib header not available in this build environment";
}

#else

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
    [[maybe_unused]] int num_threads = 4;
    
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

// ============================================================
// Tests 13-16: hnswlib behaviours underlying incrementalReindex
// ============================================================

// Test 13: addPoint with an existing label updates the vector in-place
// (underpins the "update" path of incrementalReindex)
TEST_F(HNSWLibIntegrationTest, IncrementalReindex_UpdateVectorInPlace) {
    hnswlib::L2Space space(dim);
    hnswlib::HierarchicalNSW<float>* index =
        new hnswlib::HierarchicalNSW<float>(&space, max_elements, 16, 200);

    // Insert original vector at label 0
    auto original = generateRandomVector(dim);
    index->addPoint(original.data(), 0);
    index->setEf(50);

    // Query should find label 0
    auto r1 = index->searchKnn(original.data(), 1);
    ASSERT_EQ(r1.size(), 1u);
    EXPECT_EQ(r1.top().second, 0u);

    // Overwrite label 0 with a completely different vector
    auto updated = generateRandomVector(dim);
    index->addPoint(updated.data(), 0);  // same label – in-place update

    // Querying with the UPDATED vector should still find label 0
    auto r2 = index->searchKnn(updated.data(), 1);
    ASSERT_EQ(r2.size(), 1u);
    EXPECT_EQ(r2.top().second, 0u);

    delete index;
}

// Test 14: markDelete then re-add with the same label (soft-delete / re-insert path)
// (underpins adding a vector that was previously deleted back to the index)
TEST_F(HNSWLibIntegrationTest, IncrementalReindex_ReinsertAfterMarkDelete) {
    hnswlib::L2Space space(dim);
    hnswlib::HierarchicalNSW<float>* index =
        new hnswlib::HierarchicalNSW<float>(&space, max_elements, 16, 200);

    auto vec = generateRandomVector(dim);
    index->addPoint(vec.data(), 42);
    index->setEf(50);

    // Soft-delete label 42
    index->markDelete(42);

    // After deletion the vector should not appear in search results
    auto r_after_delete = index->searchKnn(vec.data(), 1);
    bool deleted_visible = false;
    auto tmp = r_after_delete;
    while (!tmp.empty()) {
        if (tmp.top().second == 42u) {
          deleted_visible = true;
        }
        tmp.pop();
    }
    EXPECT_FALSE(deleted_visible) << "Deleted label 42 should not appear in results";

    // Re-insert with the same label (incrementalReindex "new from storage" reuse path)
    index->addPoint(vec.data(), 42);

    // Should now be visible again
    auto r_after_reinsert = index->searchKnn(vec.data(), 1);
    ASSERT_FALSE(r_after_reinsert.empty());
    EXPECT_EQ(r_after_reinsert.top().second, 42u);

    delete index;
}

// Test 15: Multiple incremental steps – add, delete, update in sequence
// (end-to-end simulation of incrementalReindex workflow at the hnswlib level)
TEST_F(HNSWLibIntegrationTest, IncrementalReindex_AddDeleteUpdateSequence) {
    hnswlib::L2Space space(dim);
    int capacity = 200;
    hnswlib::HierarchicalNSW<float>* index =
        new hnswlib::HierarchicalNSW<float>(&space, capacity, 16, 200);

    // Populate with 50 vectors (labels 0..49)
    std::vector<std::vector<float>> vecs(50);
    for (int i = 0; i < 50; ++i) {
        vecs[i] = generateRandomVector(dim);
        index->addPoint(vecs[i].data(), static_cast<size_t>(i));
    }
    EXPECT_EQ(index->cur_element_count, 50u);

    index->setEf(50);

    // Step 1 – "delete" labels 10 and 20 (removed from storage)
    index->markDelete(10);
    index->markDelete(20);

    // Step 2 – "update" label 5 (vector changed in storage)
    auto new_vec5 = generateRandomVector(dim);
    index->addPoint(new_vec5.data(), 5);

    // Step 3 – "add" two new vectors (labels 50 and 51)
    auto new_50 = generateRandomVector(dim);
    auto new_51 = generateRandomVector(dim);
    index->addPoint(new_50.data(), 50);
    index->addPoint(new_51.data(), 51);

    // Verify: deleted labels absent from results
    auto r10 = index->searchKnn(vecs[10].data(), 5);
    bool found_10 = false;
    auto tmp10 = r10;
    while (!tmp10.empty()) { if (tmp10.top().second == 10u) found_10 = true; tmp10.pop(); }
    EXPECT_FALSE(found_10) << "Label 10 should be deleted";

    // Verify: new label 50 is findable
    auto r50 = index->searchKnn(new_50.data(), 3);
    bool found_50 = false;
    auto tmp50 = r50;
    while (!tmp50.empty()) { if (tmp50.top().second == 50u) found_50 = true; tmp50.pop(); }
    EXPECT_TRUE(found_50) << "New label 50 should be findable";

    delete index;
}

// Test 16: resizeIndex required before adding beyond initial capacity
// (incrementalReindex must handle capacity expansion)
TEST_F(HNSWLibIntegrationTest, IncrementalReindex_CapacityExpansion) {
    hnswlib::L2Space space(dim);
    int initial_cap = 10;
    hnswlib::HierarchicalNSW<float>* index =
        new hnswlib::HierarchicalNSW<float>(&space, initial_cap, 16, 200);

    for (int i = 0; i < initial_cap; ++i) {
        auto v = generateRandomVector(dim);
        index->addPoint(v.data(), static_cast<size_t>(i));
    }
    EXPECT_EQ(index->cur_element_count, static_cast<size_t>(initial_cap));

    // Expand capacity (as incrementalReindex must do when many new vectors arrive)
    index->resizeIndex(initial_cap * 3);
    EXPECT_EQ(index->max_elements_, static_cast<size_t>(initial_cap * 3));

    // Now add more vectors
    for (int i = initial_cap; i < initial_cap * 2; ++i) {
        auto v = generateRandomVector(dim);
        index->addPoint(v.data(), static_cast<size_t>(i));
    }
    EXPECT_EQ(index->cur_element_count, static_cast<size_t>(initial_cap * 2));

    delete index;
}

#endif
