/**
 * @file test_tensor_move_semantics.cpp
 * @brief Tests for Tensor module move semantics
 * @version 1.0.0
 * @date 2026-07-05
 */

#include <gtest/gtest.h>
#include "distributed_tensor/tensor_data_move.h"
#include <memory>
#include <utility>

namespace themis {
namespace tensor {

class TensorModuleMoveTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// ============================================================================
// TensorData Tests (Gap 3.1)
// ============================================================================

TEST_F(TensorModuleMoveTest, TensorData_MoveConstruction) {
    TensorShape shape({2, 3, 4});
    auto tensor1 = std::make_unique<TensorData>(shape, DataType::FLOAT32);
    
    // Move construct
    TensorData tensor2(std::move(*tensor1));
    
    // Verify tensor2 has data
    EXPECT_EQ(tensor2.size(), 24);  // 2*3*4 = 24
    EXPECT_EQ(tensor2.dtype(), DataType::FLOAT32);
    EXPECT_FALSE(tensor2.empty());
    
    // Verify tensor1 is empty
    EXPECT_TRUE(tensor1->empty());
    EXPECT_EQ(tensor1->dtype(), DataType::UNKNOWN);
}

TEST_F(TensorModuleMoveTest, TensorData_MoveAssignment) {
    TensorShape shape1({2, 2});
    TensorShape shape2({3, 3});
    
    auto tensor1 = std::make_unique<TensorData>(shape1, DataType::FLOAT32);
    auto tensor2 = std::make_unique<TensorData>(shape2, DataType::FLOAT64);
    
    // Initial state
    EXPECT_EQ(tensor1->size(), 4);
    EXPECT_EQ(tensor2->size(), 9);
    
    // Move assign
    *tensor2 = std::move(*tensor1);
    
    // Verify tensor2 now has tensor1's data
    EXPECT_EQ(tensor2->size(), 4);
    EXPECT_EQ(tensor2->dtype(), DataType::FLOAT32);
    
    // Verify tensor1 is empty
    EXPECT_TRUE(tensor1->empty());
    EXPECT_EQ(tensor1->dtype(), DataType::UNKNOWN);
}

TEST_F(TensorModuleMoveTest, TensorData_MoveChain) {
    TensorShape shape({2, 2});
    auto a = std::make_unique<TensorData>(shape, DataType::FLOAT32);
    
    TensorData b(std::move(*a));
    TensorData c(std::move(b));
    
    // Verify final state
    EXPECT_EQ(c.size(), 4);
    EXPECT_EQ(c.dtype(), DataType::FLOAT32);
    
    // Verify intermediates are empty
    EXPECT_TRUE(b.empty());
    EXPECT_TRUE(a->empty());
}

TEST_F(TensorModuleMoveTest, TensorData_DeletesCopy) {
    static_assert(!std::is_copy_constructible_v<TensorData>,
                  "TensorData should not be copy constructible");
    static_assert(!std::is_copy_assignable_v<TensorData>,
                  "TensorData should not be copy assignable");
}

TEST_F(TensorModuleMoveTest, TensorData_IsMove) {
    static_assert(std::is_move_constructible_v<TensorData>,
                  "TensorData should be move constructible");
    static_assert(std::is_move_assignable_v<TensorData>,
                  "TensorData should be move assignable");
}

// ============================================================================
// TensorMetadata Tests (Gap 3.2)
// ============================================================================

TEST_F(TensorModuleMoveTest, TensorMetadata_MoveConstruction) {
    auto meta1 = std::make_unique<TensorMetadata>();
    
    TensorMetadata meta2(std::move(*meta1));
    
    // Verify valid state
    EXPECT_EQ(meta2.getDtype(), DataType::UNKNOWN);
    EXPECT_EQ(meta2.getMemoryOffset(), 0);
}

TEST_F(TensorModuleMoveTest, TensorMetadata_DeletesCopy) {
    static_assert(!std::is_copy_constructible_v<TensorMetadata>,
                  "TensorMetadata should not be copy constructible");
    static_assert(!std::is_copy_assignable_v<TensorMetadata>,
                  "TensorMetadata should not be copy assignable");
}

// ============================================================================
// ShardMetadata Tests (Gap 3.3)
// ============================================================================

TEST_F(TensorModuleMoveTest, ShardMetadata_MoveConstruction) {
    auto shard1 = std::make_unique<ShardMetadata>();
    
    ShardMetadata shard2(std::move(*shard1));
    
    // Verify valid state
    EXPECT_TRUE(shard2.getShardId().empty());
    EXPECT_TRUE(shard2.getShardRanges().empty());
}

TEST_F(TensorModuleMoveTest, ShardMetadata_DeletesCopy) {
    static_assert(!std::is_copy_constructible_v<ShardMetadata>,
                  "ShardMetadata should not be copy constructible");
    static_assert(!std::is_copy_assignable_v<ShardMetadata>,
                  "ShardMetadata should not be copy assignable");
}

// ============================================================================
// TensorRegistry Tests (Gap 3.4)
// ============================================================================

TEST_F(TensorModuleMoveTest, TensorRegistry_MoveConstruction) {
    auto registry1 = std::make_unique<TensorRegistry>();
    
    TensorRegistry registry2(std::move(*registry1));
    
    // Verify valid state
    EXPECT_EQ(registry2.size(), 0);
}

TEST_F(TensorModuleMoveTest, TensorRegistry_DeletesCopy) {
    static_assert(!std::is_copy_constructible_v<TensorRegistry>,
                  "TensorRegistry should not be copy constructible");
    static_assert(!std::is_copy_assignable_v<TensorRegistry>,
                  "TensorRegistry should not be copy assignable");
}

// ============================================================================
// Exception Safety Tests
// ============================================================================

TEST_F(TensorModuleMoveTest, TensorData_MoveNoexceptGuarantee) {
    static_assert(std::is_nothrow_move_constructible_v<TensorData>,
                  "TensorData move constructor must be noexcept");
    static_assert(std::is_nothrow_move_assignable_v<TensorData>,
                  "TensorData move assignment must be noexcept");
}

}  // namespace tensor
}  // namespace themis

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
