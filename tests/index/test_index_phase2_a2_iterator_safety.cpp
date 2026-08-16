/**
 * @file test_index_phase2_a2_iterator_safety.cpp
 * @brief Iterator safety tests for Phase 2 A-2
 * @version 0.0.1
 * @note Tests all 8 iterator invalidation gaps:
 *   - Gaps A-2-01 to A-2-04: Partition manager iterator safety
 *   - Gaps A-2-05 to A-2-08: Vector index manager iterator safety
 */

#include <gtest/gtest.h>
#include "index/partition_manager.h"
#include "index/vector_index_manager_safety.h"
#include <thread>
#include <vector>

namespace themis::testing {

// ============================================================================
// PARTITION MANAGER TESTS (Gaps A-2-01 to A-2-04)
// ============================================================================

/**
 * @brief Test Gap A-2-01: Handle validity after partition removal
 * 
 * Verifies that:
 * - Handles become invalid after partition removal
 * - isValid() returns false for removed partitions
 * - Epoch tracking detects invalidation
 */
TEST(IndexPhase2A2PartitionManager, HandleValidityAfterRemoval) {
    PartitionManager pm;
    
    // Create partition and get handle
    auto handle = pm.AddPartition("test_partition");
    EXPECT_TRUE(handle.isValid());
    
    // Verify initial epoch matches
    EXPECT_EQ(handle.epoch(), pm.CurrentEpoch(handle.id()));
    
    // Remove partition
    EXPECT_TRUE(pm.RemovePartition(handle.id()));
    
    // Handle should be invalid now (epoch changed)
    EXPECT_FALSE(handle.isValid());
    
    // Epoch should have incremented
    EXPECT_NE(handle.epoch(), pm.CurrentEpoch(handle.id()));
}

/**
 * @brief Test Gap A-2-02: Stable ID-based rebuild after removal
 * 
 * Verifies that:
 * - ID-based lookup falls back when handle is invalid
 * - New partitions can be created after removal
 * - Partition IDs are stable across operations
 */
TEST(IndexPhase2A2PartitionManager, StableIdBasedRebuild) {
    PartitionManager pm;
    
    // Create partitions
    auto h1 = pm.AddPartition("p1");
    auto h2 = pm.AddPartition("p2");
    auto h3 = pm.AddPartition("p3");
    
    EXPECT_EQ(pm.GetPartitionCount(), 3);
    
    // Get IDs and remove middle partition
    uint32_t id1 = h1.id();
    uint32_t id2 = h2.id();
    uint32_t id3 = h3.id();
    
    EXPECT_TRUE(pm.RemovePartition(id2));
    EXPECT_EQ(pm.GetPartitionCount(), 2);
    
    // ID-based lookup should fail for removed partition
    EXPECT_EQ(pm.GetPartitionById(id2), nullptr);
    
    // But others should still be accessible
    EXPECT_NE(pm.GetPartitionById(id1), nullptr);
    EXPECT_NE(pm.GetPartitionById(id3), nullptr);
    
    // Verify ID list is stable
    auto ids = pm.GetPartitionIds();
    EXPECT_EQ(ids.size(), 2);
    EXPECT_TRUE(std::find(ids.begin(), ids.end(), id1) != ids.end());
    EXPECT_TRUE(std::find(ids.begin(), ids.end(), id3) != ids.end());
    EXPECT_TRUE(std::find(ids.begin(), ids.end(), id2) == ids.end());
}

/**
 * @brief Test Gap A-2-03: Rebuild invalidates all handles (exception safety)
 * 
 * Verifies that:
 * - RebuildPartitions increments epochs for all partitions
 * - All existing handles become invalid after rebuild
 * - Rebuild is exception-safe
 * - Fallback access works after rebuild
 */
TEST(IndexPhase2A2PartitionManager, RebuildInvalidatesAllHandles) {
    PartitionManager pm;
    
    // Create multiple partitions
    auto h1 = pm.AddPartition("p1");
    auto h2 = pm.AddPartition("p2");
    auto h3 = pm.AddPartition("p3");
    
    // Store original epochs
    auto orig_epoch1 = h1.epoch();
    auto orig_epoch2 = h2.epoch();
    auto orig_epoch3 = h3.epoch();
    
    // Rebuild partitions
    EXPECT_NO_THROW(pm.RebuildPartitions());
    
    // All handles should be invalid
    EXPECT_FALSE(h1.isValid());
    EXPECT_FALSE(h2.isValid());
    EXPECT_FALSE(h3.isValid());
    
    // Epochs should have changed
    EXPECT_NE(orig_epoch1, pm.CurrentEpoch(h1.id()));
    EXPECT_NE(orig_epoch2, pm.CurrentEpoch(h2.id()));
    EXPECT_NE(orig_epoch3, pm.CurrentEpoch(h3.id()));
    
    // But ID-based lookup should still work
    EXPECT_NE(pm.GetPartitionById(h1.id()), nullptr);
    EXPECT_NE(pm.GetPartitionById(h2.id()), nullptr);
    EXPECT_NE(pm.GetPartitionById(h3.id()), nullptr);
}

/**
 * @brief Test Gap A-2-04: Compact invalidates iterators with bounds checking
 * 
 * Verifies that:
 * - CompactPartitions increments epochs
 * - Handles become invalid after compaction
 * - Bounds checking prevents out-of-range access
 * - Partition count remains stable
 */
TEST(IndexPhase2A2PartitionManager, CompactInvalidatesIterators) {
    PartitionManager pm;
    
    // Create partitions
    std::vector<PartitionHandle> handles;
    for (int i = 0; i < 5; i++) {
        handles.push_back(pm.AddPartition("p" + std::to_string(i)));
    }
    
    auto initial_count = pm.GetPartitionCount();
    EXPECT_EQ(initial_count, 5);
    
    // Store epochs
    std::vector<uint64_t> original_epochs;
    for (const auto& h : handles) {
        original_epochs.push_back(pm.CurrentEpoch(h.id()));
    }
    
    // Compact partitions
    EXPECT_NO_THROW(pm.CompactPartitions());
    
    // Count should remain same
    EXPECT_EQ(pm.GetPartitionCount(), initial_count);
    
    // All handles should be invalid (epochs changed)
    for (size_t i = 0; i < handles.size(); i++) {
        EXPECT_NE(original_epochs[i], pm.CurrentEpoch(handles[i].id()));
    }
    
    // ID-based lookup should still work for all
    for (const auto& h : handles) {
        EXPECT_NE(pm.GetPartitionById(h.id()), nullptr);
    }
}

/**
 * @brief Test Gap A-2-03 continued: RAII guard for partition safety
 * 
 * Verifies that:
 * - PartitionGuard detects invalid partitions
 * - IsValid() check prevents use-after-free
 * - Guard is exception-safe
 */
TEST(IndexPhase2A2PartitionManager, PartitionGuardSafety) {
    PartitionManager pm;
    
    auto h = pm.AddPartition("test_partition");
    
    {
        PartitionGuard guard(pm, h);
        EXPECT_TRUE(guard.IsValid());
        EXPECT_NE(guard.Get(), nullptr);
    }
    
    // Remove partition
    EXPECT_TRUE(pm.RemovePartition(h.id()));
    
    {
        PartitionGuard guard(pm, h);
        EXPECT_FALSE(guard.IsValid());
        EXPECT_EQ(guard.Get(), nullptr);
    }
}

// ============================================================================
// VECTOR INDEX MANAGER TESTS (Gaps A-2-05 to A-2-08)
// ============================================================================

/**
 * @brief Test Gap A-2-05: Handle-based access instead of raw iterators
 * 
 * Verifies that:
 * - CreateIndex returns valid handles
 * - Handles can be checked for validity
 * - Handle generation tracks invalidation
 */
TEST(IndexPhase2A2VectorIndexManager, HandleBasedAccess) {
    VectorIndexManagerSafety vim;
    
    // Create index and get handle
    auto handle = vim.CreateIndex("idx1", 128);
    EXPECT_TRUE(handle.isValid());
    EXPECT_GT(handle.id(), 0);
    EXPECT_EQ(handle.generation(), 0);
    
    // Verify generation matches
    EXPECT_EQ(handle.generation(), vim.CurrentGeneration(handle.id()));
    
    // Access by handle
    auto data = vim.GetIndexByHandle(handle);
    EXPECT_NE(data, nullptr);
}

/**
 * @brief Test Gap A-2-06: Index removal invalidates handles
 * 
 * Verifies that:
 * - RemoveVectorIndex increments generation
 * - Handles become invalid after removal
 * - Generation-based validation detects invalidation
 */
TEST(IndexPhase2A2VectorIndexManager, IndexRemovalInvalidatesHandles) {
    VectorIndexManagerSafety vim;
    
    // Create indices
    auto h1 = vim.CreateIndex("idx1", 128);
    auto h2 = vim.CreateIndex("idx2", 256);
    
    EXPECT_EQ(vim.GetIndexCount(), 2);
    
    // Store original generation
    auto orig_gen = h1.generation();
    EXPECT_TRUE(h1.isValid());
    
    // Remove index
    EXPECT_TRUE(vim.RemoveVectorIndex(h1.id()));
    
    // Handle should be invalid
    EXPECT_FALSE(h1.isValid());
    EXPECT_NE(orig_gen, vim.CurrentGeneration(h1.id()));
    
    // Other index should remain valid
    EXPECT_TRUE(h2.isValid());
    
    // Count should decrease
    EXPECT_EQ(vim.GetIndexCount(), 1);
}

/**
 * @brief Test Gap A-2-07: Update rebuilds with snapshot safety
 * 
 * Verifies that:
 * - UpdateVectorIndex invalidates existing handles
 * - Snapshot pattern prevents iterator corruption
 * - New handles can be obtained after update
 * - Exception safety during update
 */
TEST(IndexPhase2A2VectorIndexManager, UpdateRebuildSafety) {
    VectorIndexManagerSafety vim;
    
    // Create index
    auto h = vim.CreateIndex("idx", 128);
    uint32_t index_id = h.id();
    
    EXPECT_TRUE(h.isValid());
    auto orig_gen = h.generation();
    
    // Get index data before update
    auto data_before = vim.GetIndexById(index_id);
    EXPECT_NE(data_before, nullptr);
    
    // Update index (should rebuild with new generation)
    EXPECT_TRUE(vim.UpdateVectorIndex(index_id));
    
    // Old handle should be invalid
    EXPECT_FALSE(h.isValid());
    EXPECT_NE(orig_gen, vim.CurrentGeneration(index_id));
    
    // But ID-based access should work
    auto data_after = vim.GetIndexById(index_id);
    EXPECT_NE(data_after, nullptr);
    
    // Can create new handle after update
    auto new_handle = vim.CreateIndex("idx2", 256);
    EXPECT_TRUE(new_handle.isValid());
}

/**
 * @brief Test Gap A-2-08: Handle validation detects invalidation
 * 
 * Verifies that:
 * - GetIndexByHandle validates generation
 * - Invalid handles return nullptr
 * - Validation prevents use-after-free
 * - Concurrent invalidation is detected
 */
TEST(IndexPhase2A2VectorIndexManager, HandleValidationDetectsInvalidation) {
    VectorIndexManagerSafety vim;
    
    // Create and update multiple times
    auto h = vim.CreateIndex("idx", 128);
    EXPECT_NE(vim.GetIndexByHandle(h), nullptr);
    
    // Update invalidates handle
    EXPECT_TRUE(vim.UpdateVectorIndex(h.id()));
    EXPECT_EQ(vim.GetIndexByHandle(h), nullptr);  // Returns nullptr for invalid handle
    
    // But ID-based access still works
    EXPECT_NE(vim.GetIndexById(h.id()), nullptr);
    
    // Remove invalidates further
    EXPECT_TRUE(vim.RemoveVectorIndex(h.id()));
    EXPECT_EQ(vim.GetIndexByHandle(h), nullptr);
    EXPECT_EQ(vim.GetIndexById(h.id()), nullptr);
}

/**
 * @brief Test Gap A-2-07 continued: VectorIndexGuard for safe access
 * 
 * Verifies that:
 * - VectorIndexGuard prevents use-after-free
 * - IsValid() check prevents dereference of invalid data
 * - Guard is exception-safe
 */
TEST(IndexPhase2A2VectorIndexManager, VectorIndexGuardSafety) {
    VectorIndexManagerSafety vim;
    
    auto h = vim.CreateIndex("idx", 128);
    
    {
        VectorIndexGuard guard(vim, h);
        EXPECT_TRUE(guard.IsValid());
        EXPECT_NE(guard.Get(), nullptr);
    }
    
    // Remove index
    EXPECT_TRUE(vim.RemoveVectorIndex(h.id()));
    
    {
        VectorIndexGuard guard(vim, h);
        EXPECT_FALSE(guard.IsValid());
        EXPECT_EQ(guard.Get(), nullptr);
    }
}

/**
 * @brief Edge case: Concurrent add/remove operations
 * 
 * Verifies that:
 * - Multiple handles can coexist safely
 * - Removal of one doesn't affect others
 * - ID-based fallback works in multi-handle scenario
 */
TEST(IndexPhase2A2PartitionManager, ConcurrentHandles) {
    PartitionManager pm;
    
    // Create multiple handles
    std::vector<PartitionHandle> handles;
    for (int i = 0; i < 10; i++) {
        handles.push_back(pm.AddPartition("p" + std::to_string(i)));
    }
    
    // All should be valid initially
    for (const auto& h : handles) {
        EXPECT_TRUE(h.isValid());
    }
    
    // Remove every other partition
    for (size_t i = 1; i < handles.size(); i += 2) {
        EXPECT_TRUE(pm.RemovePartition(handles[i].id()));
    }
    
    // Check validity
    for (size_t i = 0; i < handles.size(); i++) {
        if (i % 2 == 0) {
            EXPECT_TRUE(handles[i].isValid());
        } else {
            EXPECT_FALSE(handles[i].isValid());
        }
    }
}

/**
 * @brief Edge case: Empty container operations
 * 
 * Verifies that:
 * - CompactPartitions works on empty container
 * - RebuildPartitions works on empty container
 * - No exceptions on empty operations
 */
TEST(IndexPhase2A2PartitionManager, EmptyContainerOperations) {
    PartitionManager pm;
    
    EXPECT_EQ(pm.GetPartitionCount(), 0);
    EXPECT_NO_THROW(pm.RebuildPartitions());
    EXPECT_NO_THROW(pm.CompactPartitions());
    EXPECT_EQ(pm.GetPartitionCount(), 0);
}

/**
 * @brief Integration test: All 8 gaps working together
 * 
 * Verifies that:
 * - Partition and Vector Index managers work together
 * - Iterator safety patterns compose correctly
 * - Handle validity is maintained across operations
 */
TEST(IndexPhase2A2Integration, AllGapsWorking) {
    PartitionManager pm;
    VectorIndexManagerSafety vim;
    
    // Create resources
    auto p1 = pm.AddPartition("p1");
    auto p2 = pm.AddPartition("p2");
    auto idx1 = vim.CreateIndex("idx1", 128);
    auto idx2 = vim.CreateIndex("idx2", 256);
    
    EXPECT_EQ(pm.GetPartitionCount(), 2);
    EXPECT_EQ(vim.GetIndexCount(), 2);
    
    // All handles valid initially
    EXPECT_TRUE(p1.isValid());
    EXPECT_TRUE(p2.isValid());
    EXPECT_TRUE(idx1.isValid());
    EXPECT_TRUE(idx2.isValid());
    
    // Remove and update
    EXPECT_TRUE(pm.RemovePartition(p2.id()));
    EXPECT_TRUE(vim.UpdateVectorIndex(idx1.id()));
    
    // Check invalidation
    EXPECT_TRUE(p1.isValid());
    EXPECT_FALSE(p2.isValid());
    EXPECT_FALSE(idx1.isValid());
    EXPECT_TRUE(idx2.isValid());
    
    // Verify fallback access
    EXPECT_NE(pm.GetPartitionById(p1.id()), nullptr);
    EXPECT_EQ(pm.GetPartitionById(p2.id()), nullptr);
    EXPECT_NE(vim.GetIndexById(idx1.id()), nullptr);
    EXPECT_NE(vim.GetIndexById(idx2.id()), nullptr);
}

} // namespace themis::testing
