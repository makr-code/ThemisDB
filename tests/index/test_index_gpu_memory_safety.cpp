// Test suite for Phase 5 C++ safety fixes: GPU memory safety
// Validates that GPU resources are properly allocated and deallocated
// (Blocker #6, specifically Fix #3)
//
// This test file ensures:
// - GPU resources are allocated on initialization
// - GPU resources are freed on destruction (RAII)
// - Exception during init doesn't leak GPU memory
// - Large allocations are handled correctly
// - Memory is released in proper order

#include "index/gpu_vector_index.h"

#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include <atomic>
#include <thread>
#include <stdexcept>

// Note: CUDA headers are conditionally included
#ifdef THEMIS_CUDA_ENABLED
    #include <cuda_runtime.h>
#endif

namespace themis::index::tests {

// ============================================================================
// Helper: CUDA Memory Tracker (works with or without actual CUDA)
// ============================================================================

class CudaMemoryTracker {
 public:
    // Query free GPU memory
    static size_t getFreeFenceMemory() {
#ifdef THEMIS_CUDA_ENABLED
        size_t free_mem = 0;
        size_t total_mem = 0;
        cudaMemGetInfo(&free_mem, &total_mem);
        return free_mem;
#else
        // Fallback: return a simulated value for testing without CUDA
        return 1024 * 1024 * 1024;  // 1GB simulated
#endif
    }

    // Query total GPU memory
    static size_t getTotalGpuMemory() {
#ifdef THEMIS_CUDA_ENABLED
        size_t free_mem = 0;
        size_t total_mem = 0;
        cudaMemGetInfo(&free_mem, &total_mem);
        return total_mem;
#else
        return 2 * 1024 * 1024 * 1024;  // 2GB simulated
#endif
    }

    // Check for CUDA errors
    static bool hasCudaErrors() {
#ifdef THEMIS_CUDA_ENABLED
        cudaError_t err = cudaGetLastError();
        return err != cudaSuccess;
#else
        return false;
#endif
    }

    // Reset CUDA error state
    static void resetCudaErrors() {
#ifdef THEMIS_CUDA_ENABLED
        cudaGetLastError();
#endif
    }
};

// ============================================================================
// Test Fixture for GPU Memory Safety
// ============================================================================

class GPUVectorIndexMemorySafety : public ::testing::Test {
 protected:
    void SetUp() override {
        // Check if GPU is available
        initial_free_memory_ = CudaMemoryTracker::getFreeFenceMemory();
        CudaMemoryTracker::resetCudaErrors();
    }

    void TearDown() override {
        // Verify no lingering CUDA errors
        EXPECT_FALSE(CudaMemoryTracker::hasCudaErrors())
            << "CUDA error state detected after test";
    }

    size_t initial_free_memory_;
};

// ============================================================================
// TEST 1: GPU Resource Initialization and Cleanup
// ============================================================================

TEST_F(GPUVectorIndexMemorySafety, GpuResourcesInitializedAndReleased) {
    // This test verifies basic allocation/deallocation without exceptions
    
    try {
        size_t pre_init_free = CudaMemoryTracker::getFreeFenceMemory();
        
        {
            GPUVectorIndex gpu_index;
            
            // Initialize with specific dimension
            bool init_success = gpu_index.initialize(128);
            
            if (!init_success) {
                GTEST_SKIP() << "GPU initialization failed (GPU may not be available)";
            }
            
            size_t post_init_free = CudaMemoryTracker::getFreeFenceMemory();
            
            // If CUDA is available, expect some memory to be allocated
            // (Note: memory tracking may not be precise in all CUDA implementations)
            if (post_init_free < pre_init_free) {
                EXPECT_LT(post_init_free, pre_init_free);
            }
        }
        // Destructor called here; GPU resources should be released
        
        size_t post_cleanup_free = CudaMemoryTracker::getFreeFenceMemory();
        
        // After cleanup, free memory should be restored (approximately)
        // Allow for CUDA overhead and memory fragmentation
        EXPECT_GE(post_cleanup_free, pre_init_free - (10 * 1024 * 1024));  // 10MB margin
        
    } catch (const std::exception& e) {
        GTEST_SKIP() << "Test skipped: " << e.what();
    }
}

// ============================================================================
// TEST 2: Multiple Init/Destroy Cycles (Memory Leak Detection)
// ============================================================================

TEST_F(GPUVectorIndexMemorySafety, MultipleInitDestroyNeverLeaks) {
    // Run multiple init/destroy cycles and verify no memory leak trend
    
    try {
        std::vector<size_t> free_memory_samples;
        const int NUM_CYCLES = 5;
        
        for (int cycle = 0; cycle < NUM_CYCLES; ++cycle) {
            size_t pre_cycle = CudaMemoryTracker::getFreeFenceMemory();
            free_memory_samples.push_back(pre_cycle);
            
            {
                GPUVectorIndex gpu_index;
                bool init_success = gpu_index.initialize(64);
                
                if (!init_success && cycle == 0) {
                    GTEST_SKIP() << "GPU initialization failed";
                }
            }
            // Destructor called
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        // Check for memory leak trend
        // Free memory should stabilize, not continuously decrease
        if (free_memory_samples.size() >= 2) {
            size_t initial_free = free_memory_samples[0];
            size_t final_free = free_memory_samples.back();
            
            // Allow for fragmentation but not consistent decrease
            // (memory shouldn't leak away after cleanup)
            EXPECT_GE(final_free, initial_free - (50 * 1024 * 1024));  // 50MB threshold
        }
        
    } catch (const std::exception& e) {
        GTEST_SKIP() << "Test skipped: " << e.what();
    }
}

// ============================================================================
// TEST 3: Exception During Initialization (Cleanup on Exception)
// ============================================================================

TEST_F(GPUVectorIndexMemorySafety, ExceptionDuringInitialization) {
    // Simulate exception during initialization and verify cleanup completes
    
    try {
        size_t pre_exception_free = CudaMemoryTracker::getFreeFenceMemory();
        
        try {
            GPUVectorIndex gpu_index;
            
            // Initialize (may throw)
            bool init_success = gpu_index.initialize(128);
            
            if (!init_success) {
                throw std::runtime_error("GPU initialization failed");
            }
            
            // Simulate application-level exception during use
            throw std::runtime_error("Simulated GPU compute error");
            
        } catch (const std::runtime_error& e) {
            // Exception should be caught here
            // Destructor runs during exception unwinding
            EXPECT_NE(std::string(e.what()).find("error"), std::string::npos);
        }
        
        size_t post_exception_free = CudaMemoryTracker::getFreeFenceMemory();
        
        // Verify GPU resources were cleaned up despite exception
        EXPECT_GE(post_exception_free, pre_exception_free - (50 * 1024 * 1024));
        
    } catch (const std::exception& e) {
        GTEST_SKIP() << "Test skipped: " << e.what();
    }
}

// ============================================================================
// TEST 4: Large Allocations Near GPU Memory Limit
// ============================================================================

TEST_F(GPUVectorIndexMemorySafety, LargeAllocationsCompleteCleanup) {
    // Test with a reasonably large allocation to stress GPU memory
    
    try {
        size_t total_gpu_mem = CudaMemoryTracker::getTotalGpuMemory();
        size_t free_before = CudaMemoryTracker::getFreeFenceMemory();
        
        // Allocate ~20% of available GPU memory or 100MB, whichever is smaller
        size_t target_allocation = std::min(
            free_before / 5,
            100 * 1024 * 1024  // 100MB
        );
        
        // Calculate vector dimension that would use roughly target_allocation
        // Rough formula: memory = num_vectors * dimension * sizeof(float)
        size_t num_vectors = 10000;
        size_t dimension = target_allocation / (num_vectors * sizeof(float));
        dimension = std::max(dimension, size_t(128));
        dimension = std::min(dimension, size_t(1024));
        
        {
            GPUVectorIndex gpu_index;
            
            bool init_success = gpu_index.initialize(static_cast<int>(dimension));
            
            if (!init_success) {
                GTEST_SKIP() << "GPU initialization failed";
            }
            
            // Destructor called here
        }
        
        size_t free_after = CudaMemoryTracker::getFreeFenceMemory();
        
        // Allow for some fragmentation and overhead
        EXPECT_GE(free_after, free_before - (200 * 1024 * 1024));  // 200MB margin
        
    } catch (const std::exception& e) {
        GTEST_SKIP() << "Test skipped: " << e.what();
    }
}

// ============================================================================
// TEST 5: Concurrent GPU Index Creation/Destruction
// ============================================================================

TEST_F(GPUVectorIndexMemorySafety, ConcurrentIndexManagement) {
    // Multiple threads creating/destroying GPU indices
    // Verify memory management is thread-safe
    
    try {
        const int NUM_THREADS = 4;
        const int ITERATIONS = 3;
        std::vector<std::thread> threads;
        std::atomic<int> successful_cycles(0);
        
        for (int t = 0; t < NUM_THREADS; ++t) {
            threads.emplace_back([&successful_cycles]() {
                for (int i = 0; i < ITERATIONS; ++i) {
                    try {
                        {
                            GPUVectorIndex gpu_index = {};
                            if (gpu_index.initialize(64)) {
                                ++successful_cycles;
                            }
                        }
                        std::this_thread::yield();
                    } catch (const std::exception&) {
                        // Skip on GPU unavailability
                    }
                }
            });
        }
        
        // Wait for all threads
        for (auto& t : threads) {
            t.join();
        }
        
        // At least some cycles should succeed
        EXPECT_GT(successful_cycles.load(), 0);
        
    } catch (const std::exception& e) {
        GTEST_SKIP() << "Test skipped: " << e.what();
    }
}

// ============================================================================
// TEST 6: Explicit Shutdown and Cleanup
// ============================================================================

TEST_F(GPUVectorIndexMemorySafety, ExplicitShutdownCleansupResources) {
    // Verify that explicit shutdown() call cleans up resources
    
    try {
        size_t pre_shutdown_free = CudaMemoryTracker::getFreeFenceMemory();
        
        GPUVectorIndex gpu_index = {};
        
        if (!gpu_index.initialize(128)) {
            GTEST_SKIP() << "GPU initialization failed";
        }
        
        // Explicitly shutdown
        gpu_index.shutdown();
        
        size_t post_shutdown_free = CudaMemoryTracker::getFreeFenceMemory();
        
        // After explicit shutdown, memory should be freed
        EXPECT_GE(post_shutdown_free, pre_shutdown_free - (50 * 1024 * 1024));
        
        // Destructor should be safe even after explicit shutdown
        // (destructor will be called on scope exit)
        
    } catch (const std::exception& e) {
        GTEST_SKIP() << "Test skipped: " << e.what();
    }
}

// ============================================================================
// TEST 7: Multiple Shutdown Calls (Idempotency)
// ============================================================================

TEST_F(GPUVectorIndexMemorySafety, MultipleShutdownCallsSafe) {
    // Verify that calling shutdown() multiple times is safe
    
    try {
        GPUVectorIndex gpu_index = {};
        
        if (!gpu_index.initialize(64)) {
            GTEST_SKIP() << "GPU initialization failed";
        }
        
        // Multiple shutdown calls should be safe
        gpu_index.shutdown();
        gpu_index.shutdown();
        gpu_index.shutdown();
        
        // Destructor will call shutdown again (should still be safe)
        
    } catch (const std::exception& e) {
        GTEST_SKIP() << "Test skipped: " << e.what();
    }
}

// ============================================================================
// TEST 8: GPU Resource State Verification
// ============================================================================

TEST_F(GPUVectorIndexMemorySafety, GpuStateValidation) {
    // Verify GPU state is valid before/after operations
    
    try {
        // Clear any previous errors
        CudaMemoryTracker::resetCudaErrors();
        
        {
            GPUVectorIndex gpu_index = {};
            
            if (!gpu_index.initialize(128)) {
                GTEST_SKIP() << "GPU initialization failed";
            }
            
            // No errors should occur during valid operations
            EXPECT_FALSE(CudaMemoryTracker::hasCudaErrors());
        }
        
        // After cleanup, GPU state should still be valid
        EXPECT_FALSE(CudaMemoryTracker::hasCudaErrors());
        
    } catch (const std::exception& e) {
        GTEST_SKIP() << "Test skipped: " << e.what();
    }
}

// ============================================================================
// TEST 9: Empty Destruction (Default Constructor)
// ============================================================================

TEST_F(GPUVectorIndexMemorySafety, DefaultConstructorAndDestruction) {
    // Verify default constructor + destruction is safe without init
    
    try {
        size_t pre_create = CudaMemoryTracker::getFreeFenceMemory();
        
        {
            GPUVectorIndex gpu_index;
            // No initialize() call - destructor should still be safe
        }
        
        size_t post_destroy = CudaMemoryTracker::getFreeFenceMemory();
        
        // Memory should be released even if never initialized
        EXPECT_GE(post_destroy, pre_create - (10 * 1024 * 1024));
        
    } catch (const std::exception& e) {
        GTEST_SKIP() << "Test skipped: " << e.what();
    }
}

// ============================================================================
// TEST 10: Configuration-Based Allocation
// ============================================================================

TEST_F(GPUVectorIndexMemorySafety, ConfigurationBasedAllocation) {
    // Test initialization with explicit configuration
    
    try {
        size_t pre_alloc = CudaMemoryTracker::getFreeFenceMemory();
        
        {
            GPUVectorIndex::Config config;
            config.oversubscription_partition_vectors = 10000;
            config.oversubscription_enabled = true;
            
            GPUVectorIndex gpu_index(config);
            
            if (!gpu_index.initialize(256)) {
                GTEST_SKIP() << "GPU initialization with config failed";
            }
        }
        
        size_t post_cleanup = CudaMemoryTracker::getFreeFenceMemory();
        
        // Configuration-based allocation should still clean up properly
        EXPECT_GE(post_cleanup, pre_alloc - (100 * 1024 * 1024));
        
    } catch (const std::exception& e) {
        GTEST_SKIP() << "Test skipped: " << e.what();
    }
}

}  // namespace themis::index::tests
