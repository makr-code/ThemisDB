/**
 * @file test_aligned_vector_cache.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 87/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Test for cache-aligned vector allocator
// Verifies that aligned allocation and SIMD optimizations work correctly

#include <gtest/gtest.h>
#include "cache/aligned_vector_allocator.h"
#include "performance/alignment_helpers.h"
#include <vector>

using namespace themis;

// Test 1: Create aligned vector and verify 32-byte alignment
TEST(AlignedVectorAllocatorTest, BasicAlignment32) {
    cache::AlignedVector<float> vec;
    vec.resize(1536);  // OpenAI ada-002 dimension
    
    // Fill with test data
    for (size_t i = 0; i < vec.size(); ++i) {
        vec[i] = static_cast<float>(i) / 1536.0f;
    }
    
    // Verify alignment
    const void* ptr = vec.data();
    bool is_aligned_32 = performance::is_aligned<32>(ptr);
    
    EXPECT_TRUE(is_aligned_32) << "Vector should be 32-byte aligned at address " << ptr;
}

// Test 2: Test cache-line (64-byte) alignment
TEST(AlignedVectorAllocatorTest, CacheLineAlignment64) {
    cache::CacheLineVector<float> cache_aligned(64);
    const void* cache_ptr = cache_aligned.data();
    bool is_aligned_64 = performance::is_aligned<64>(cache_ptr);
    
    EXPECT_TRUE(is_aligned_64) << "Cache-line vector should be 64-byte aligned at address " << cache_ptr;
}

// Test 3: Verify data integrity after aligned allocation
TEST(AlignedVectorAllocatorTest, DataIntegrity) {
    cache::AlignedVector<float> vec(1536);
    
    // Fill with test data
    for (size_t i = 0; i < vec.size(); ++i) {
        vec[i] = static_cast<float>(i) / 1536.0f;
    }
    
    // Verify data
    EXPECT_FLOAT_EQ(vec[0], 0.0f);
    EXPECT_FLOAT_EQ(vec[1535], 1535.0f / 1536.0f);
}

// Test 4: Test SIMD alignment (16-byte for SSE/NEON)
TEST(AlignedVectorAllocatorTest, SimdAlignment16) {
    cache::SimdVector<float> simd_vec(128);
    const void* simd_ptr = simd_vec.data();
    bool is_aligned_16 = performance::is_aligned<16>(simd_ptr);
    
    EXPECT_TRUE(is_aligned_16) << "SIMD vector should be 16-byte aligned at address " << simd_ptr;
}

// Test 5: Test move semantics with aligned vectors
TEST(AlignedVectorAllocatorTest, MoveSemantics) {
    cache::AlignedVector<float> vec1(100);
    for (size_t i = 0; i < vec1.size(); ++i) {
        vec1[i] = static_cast<float>(i);
    }
    
    const void* original_ptr = vec1.data();
    
    // Move to vec2
    cache::AlignedVector<float> vec2 = std::move(vec1);
    
    EXPECT_EQ(vec2.data(), original_ptr) << "Move should transfer ownership";
    EXPECT_TRUE(performance::is_aligned<32>(vec2.data())) << "Alignment should be preserved after move";
    EXPECT_FLOAT_EQ(vec2[50], 50.0f);
}
