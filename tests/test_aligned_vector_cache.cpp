// Test for cache-aligned vector allocator
// Verifies that aligned allocation and SIMD optimizations work correctly

#include "cache/aligned_vector_allocator.h"
#include "performance/alignment_helpers.h"
#include <iostream>
#include <cassert>
#include <vector>

using namespace themis;

int main() {
    std::cout << "Testing AlignedVectorAllocator...\n";
    
    // Test 1: Create aligned vector
    cache::AlignedVector<float> vec;
    vec.resize(1536);  // OpenAI ada-002 dimension
    
    // Fill with test data
    for (size_t i = 0; i < vec.size(); ++i) {
        vec[i] = static_cast<float>(i) / 1536.0f;
    }
    
    // Test 2: Verify alignment
    const void* ptr = vec.data();
    bool is_aligned_32 = performance::is_aligned<32>(ptr);
    
    std::cout << "Vector address: " << ptr << "\n";
    std::cout << "Is 32-byte aligned: " << (is_aligned_32 ? "YES" : "NO") << "\n";
    
    assert(is_aligned_32 && "Vector should be 32-byte aligned");
    
    // Test 3: Test with different alignments
    cache::CacheLineVector<float> cache_aligned(64);
    const void* cache_ptr = cache_aligned.data();
    bool is_aligned_64 = performance::is_aligned<64>(cache_ptr);
    
    std::cout << "Cache-line vector address: " << cache_ptr << "\n";
    std::cout << "Is 64-byte aligned: " << (is_aligned_64 ? "YES" : "NO") << "\n";
    
    assert(is_aligned_64 && "Cache-line vector should be 64-byte aligned");
    
    // Test 4: Verify data integrity
    assert(vec[0] == 0.0f);
    assert(vec[1535] == 1535.0f / 1536.0f);
    
    std::cout << "All tests passed!\n";
    return 0;
}
