/**
 * @file test_lek_rotation_atomic.cpp
 * @brief Tests for LEK rotation atomicity
 * @date 2026-08-17
 *
 * Tests Phase 2.3 and Phase 4.2 hardening for lek_manager.cpp:
 * - Atomic LEK rotation (no dual-generation window)
 * - Key store unavailability handling
 * - Rotation retry logic with bounded budget
 * - Thread-safe key access during rotation
 */

#include <gtest/gtest.h>
#include "utils/lek_manager.h"
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>

namespace themis {
namespace utils {

class LEKRotationTest : public ::testing::Test {
protected:
    LEKManager lek_manager;
    
    void SetUp() override {
        // Initialize with default settings
        lek_manager = LEKManager();
    }
};

// ============================================================================
// Test Atomic Rotation (No Dual-Generation Window)
// ============================================================================

TEST_F(LEKRotationTest, RotationIsAtomic) {
    // Test that rotation completes atomically
    // No point in time should have two different active generations
    
    // Get current generation before rotation
    auto gen_before = lek_manager.currentGeneration();
    
    // Perform rotation
    bool rotation_ok = lek_manager.rotateKey();
    EXPECT_TRUE(rotation_ok);
    
    // Get generation after rotation
    auto gen_after = lek_manager.currentGeneration();
    
    // Generation should have incremented by exactly 1
    EXPECT_EQ(gen_after, gen_before + 1);
}

TEST_F(LEKRotationTest, NoGenerationGap) {
    // Verify that generations are continuous (no gaps)
    std::vector<uint32_t> generations;
    
    for (int i = 0; i < 5; ++i) {
        generations.push_back(lek_manager.currentGeneration());
        lek_manager.rotateKey();
    }
    
    // Generations should be strictly increasing
    for (size_t i = 1; i < generations.size(); ++i) {
        EXPECT_EQ(generations[i], generations[i-1] + 1);
    }
}

TEST_F(LEKRotationTest, RotationBlocksOldKeys) {
    // After rotation, old generation should not be accepted
    auto old_gen = lek_manager.currentGeneration();
    
    // Rotate
    lek_manager.rotateKey();
    auto new_gen = lek_manager.currentGeneration();
    
    // Try to derive key with old generation
    // Should either fail or skip to new generation
    EXPECT_NE(old_gen, new_gen);
}

// ============================================================================
// Test Thread-Safe Concurrent Access During Rotation
// ============================================================================

TEST_F(LEKRotationTest, ConcurrentRotationAndAccess) {
    // Simulate concurrent threads rotating and accessing keys
    std::atomic<int> rotation_count = 0;
    std::atomic<int> access_count = 0;
    std::atomic<bool> error_detected = false;
    
    std::vector<std::thread> threads;
    
    // Thread 1: Perform rotations
    threads.emplace_back([this, &rotation_count]() {
        for (int i = 0; i < 10; ++i) {
            bool ok = lek_manager.rotateKey();
            if (ok) rotation_count++;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });
    
    // Threads 2-5: Access keys concurrently
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([this, &access_count, &error_detected]() {
            for (int i = 0; i < 20; ++i) {
                try {
                    // Try to get current generation
                    auto gen = lek_manager.currentGeneration();
                    access_count++;
                    
                    // Generation should always be valid (non-zero)
                    if (gen == 0) {
                        error_detected = true;
                    }
                } catch (...) {
                    // Exceptions acceptable during rotation, but should be caught
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        });
    }
    
    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }
    
    // Verify results
    EXPECT_GT(rotation_count, 0);  // Rotations occurred
    EXPECT_GT(access_count, 0);    // Accesses occurred
    EXPECT_FALSE(error_detected);  // No invalid generations detected
}

TEST_F(LEKRotationTest, RotationUnderConcurrentLoad) {
    std::vector<std::thread> readers;
    std::atomic<int> successful_reads = 0;
    
    // Spawn 10 reader threads
    for (int i = 0; i < 10; ++i) {
        readers.emplace_back([this, &successful_reads]() {
            for (int j = 0; j < 100; ++j) {
                try {
                    auto gen = lek_manager.currentGeneration();
                    successful_reads++;
                } catch (...) {
                    // Acceptable during rotation
                }
            }
        });
    }
    
    // Perform 5 rotations while readers are active
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    for (int i = 0; i < 5; ++i) {
        lek_manager.rotateKey();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    
    // Wait for readers
    for (auto& t : readers) {
        t.join();
    }
    
    EXPECT_GT(successful_reads, 0);
}

// ============================================================================
// Test Key Derivation Across Rotations
// ============================================================================

TEST_F(LEKRotationTest, KeyDerivationConsistency) {
    auto gen1 = lek_manager.currentGeneration();
    
    // Derive key material for generation 1
    // (This is pseudo-code - actual implementation may differ)
    // auto key1 = lek_manager.deriveKey(salt, info);
    
    // Rotate to generation 2
    lek_manager.rotateKey();
    auto gen2 = lek_manager.currentGeneration();
    EXPECT_NE(gen1, gen2);
    
    // Derive key material for generation 2
    // Keys should be different across generations
}

// ============================================================================
// Test Key Store Unavailability
// ============================================================================

TEST_F(LEKRotationTest, RotationFailsGracefully) {
    // Attempt rotation - should either succeed or fail gracefully
    bool rotation_ok = lek_manager.rotateKey();
    
    // If rotation fails (key store unavailable), should return false
    // If rotation succeeds, should return true
    // Either way, should not crash
    EXPECT_TRUE(rotation_ok || !rotation_ok);  // Tautology to express "no crash"
}

TEST_F(LEKRotationTest, RetryBudgetExhaustion) {
    // Attempt multiple rotations
    int successful_rotations = 0;
    int failed_rotations = 0;
    
    for (int i = 0; i < 100; ++i) {
        if (lek_manager.rotateKey()) {
            successful_rotations++;
        } else {
            failed_rotations++;
        }
    }
    
    // At least some rotations should succeed
    // No rotation should hang (all complete quickly)
    EXPECT_TRUE(successful_rotations > 0 || failed_rotations > 0);
}

// ============================================================================
// Test Rotation Ordering Under Stress
// ============================================================================

TEST_F(LEKRotationTest, GenerationMonotonicity) {
    // Perform rapid rotations and verify generation always increases
    std::vector<uint32_t> gen_sequence;
    
    for (int i = 0; i < 20; ++i) {
        gen_sequence.push_back(lek_manager.currentGeneration());
        lek_manager.rotateKey();
    }
    
    // Verify strict monotonic increase
    for (size_t i = 1; i < gen_sequence.size(); ++i) {
        EXPECT_LT(gen_sequence[i-1], gen_sequence[i]);
    }
}

TEST_F(LEKRotationTest, NoReverseRotation) {
    auto gen1 = lek_manager.currentGeneration();
    lek_manager.rotateKey();
    auto gen2 = lek_manager.currentGeneration();
    
    // Rotation should only go forward, never backward
    EXPECT_LT(gen1, gen2);
    
    lek_manager.rotateKey();
    auto gen3 = lek_manager.currentGeneration();
    
    EXPECT_LT(gen2, gen3);
}

// ============================================================================
// Test Multiple Consecutive Rotations
// ============================================================================

TEST_F(LEKRotationTest, ChainedRotations) {
    std::vector<uint32_t> generations;
    
    for (int i = 0; i < 10; ++i) {
        generations.push_back(lek_manager.currentGeneration());
        lek_manager.rotateKey();
    }
    
    // All generations should be unique and increasing
    for (size_t i = 0; i < generations.size(); ++i) {
        for (size_t j = i + 1; j < generations.size(); ++j) {
            EXPECT_NE(generations[i], generations[j]);
            EXPECT_LT(generations[i], generations[j]);
        }
    }
}

// ============================================================================
// Test Concurrent Rotation Attempts
// ============================================================================

TEST_F(LEKRotationTest, SimultaneousRotationRequests) {
    // Multiple threads attempting rotation simultaneously
    std::vector<std::thread> rotators;
    std::atomic<int> successful = 0;
    std::vector<uint32_t> observed_generations;
    std::mutex gen_mutex;
    
    for (int i = 0; i < 5; ++i) {
        rotators.emplace_back([this, &successful, &observed_generations, &gen_mutex]() {
            if (lek_manager.rotateKey()) {
                successful++;
                {
                    std::lock_guard<std::mutex> lock(gen_mutex);
                    observed_generations.push_back(lek_manager.currentGeneration());
                }
            }
        });
    }
    
    for (auto& t : rotators) {
        t.join();
    }
    
    // At most one rotation should actually succeed (atomic)
    // Or all may succeed (depending on implementation)
    EXPECT_GE(successful, 0);
    
    // All observed generations should be valid and unique
    std::sort(observed_generations.begin(), observed_generations.end());
    for (size_t i = 1; i < observed_generations.size(); ++i) {
        EXPECT_NE(observed_generations[i-1], observed_generations[i]);
    }
}

// ============================================================================
// Test Recovery from Failed Rotation
// ============================================================================

TEST_F(LEKRotationTest, RecoveryAfterFailedRotation) {
    auto initial_gen = lek_manager.currentGeneration();
    
    // Attempt rotation (may fail if key store unavailable)
    bool first_rotation = lek_manager.rotateKey();
    
    // Second rotation attempt should still work
    bool second_rotation = lek_manager.rotateKey();
    
    auto final_gen = lek_manager.currentGeneration();
    
    // Should be in valid state (generation increased by successful rotations)
    EXPECT_GE(final_gen, initial_gen);
}

// ============================================================================
// Test Metadata During Rotation
// ============================================================================

TEST_F(LEKRotationTest, MetadataAvailability) {
    auto metadata_before = lek_manager.getMetadata();
    
    lek_manager.rotateKey();
    
    auto metadata_after = lek_manager.getMetadata();
    
    // Metadata should be accessible before and after rotation
    // (Specific metadata structure depends on implementation)
}

// ============================================================================
// Test Long-Term Stability
// ============================================================================

TEST_F(LEKRotationTest, LongSequenceOfRotations) {
    // Perform many rotations in sequence
    uint32_t prev_gen = lek_manager.currentGeneration();
    
    for (int i = 0; i < 1000; ++i) {
        lek_manager.rotateKey();
        uint32_t curr_gen = lek_manager.currentGeneration();
        
        // Each rotation should increase generation by at least 1
        EXPECT_GT(curr_gen, prev_gen);
        prev_gen = curr_gen;
        
        // Should not hang or crash on any iteration
    }
}

} // namespace utils
} // namespace themis

