/**
 * @file test_acceleration_move_semantics.cpp
 * @brief Tests for Acceleration module move semantics
 * @version 1.0.0
 * @date 2026-07-05
 */

#include <gtest/gtest.h>
#include "acceleration/acceleration_move_semantics.h"
#include <memory>
#include <utility>

namespace themis {
namespace acceleration {

class AccelerationModuleMoveTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(AccelerationModuleMoveTest, GPUKernelHandle_MoveConstruction) {
    auto kernel1 = std::make_unique<GPUKernelHandle>();
    GPUKernelHandle kernel2(std::move(*kernel1));
    
    EXPECT_FALSE(kernel2.isValid());
    EXPECT_EQ(kernel2.getDevice(), 0);
    EXPECT_FALSE(kernel1->isValid());
}

TEST_F(AccelerationModuleMoveTest, GPUKernelHandle_MoveAssignment) {
    auto kernel1 = std::make_unique<GPUKernelHandle>();
    auto kernel2 = std::make_unique<GPUKernelHandle>();
    
    *kernel2 = std::move(*kernel1);
    
    EXPECT_FALSE(kernel2->isValid());
    EXPECT_FALSE(kernel1->isValid());
}

TEST_F(AccelerationModuleMoveTest, GPUKernelHandle_DeletesCopy) {
    static_assert(!std::is_copy_constructible_v<GPUKernelHandle>);
    static_assert(!std::is_copy_assignable_v<GPUKernelHandle>);
}

TEST_F(AccelerationModuleMoveTest, GPUBatchProcessor_MoveConstruction) {
    auto processor1 = std::make_unique<GPUBatchProcessor>();
    GPUBatchProcessor processor2(std::move(*processor1));
    
    EXPECT_EQ(processor2.getKernelCount(), 0);
    EXPECT_EQ(processor1->getKernelCount(), 0);
}

TEST_F(AccelerationModuleMoveTest, GPUBatchProcessor_DeletesCopy) {
    static_assert(!std::is_copy_constructible_v<GPUBatchProcessor>);
    static_assert(!std::is_copy_assignable_v<GPUBatchProcessor>);
}

TEST_F(AccelerationModuleMoveTest, GPUMemoryPool_MoveConstruction) {
    auto pool1 = std::make_unique<GPUMemoryPool>();
    GPUMemoryPool pool2(std::move(*pool1));
    
    EXPECT_EQ(pool2.getTotalSize(), 0);
    EXPECT_EQ(pool2.getAllocatedSize(), 0);
    EXPECT_EQ(pool1->getTotalSize(), 0);
}

TEST_F(AccelerationModuleMoveTest, GPUMemoryPool_MoveAssignment) {
    auto pool1 = std::make_unique<GPUMemoryPool>();
    auto pool2 = std::make_unique<GPUMemoryPool>();
    
    *pool2 = std::move(*pool1);
    
    EXPECT_EQ(pool2->getTotalSize(), 0);
    EXPECT_EQ(pool1->getTotalSize(), 0);
}

TEST_F(AccelerationModuleMoveTest, GPUMemoryPool_DeletesCopy) {
    static_assert(!std::is_copy_constructible_v<GPUMemoryPool>);
    static_assert(!std::is_copy_assignable_v<GPUMemoryPool>);
}

TEST_F(AccelerationModuleMoveTest, GPUStreamWrapper_MoveConstruction) {
    auto stream1 = std::make_unique<GPUStreamWrapper>();
    GPUStreamWrapper stream2(std::move(*stream1));
    
    EXPECT_EQ(stream2.getDevice(), 0);
    EXPECT_FALSE(stream2.isRecording());
    EXPECT_EQ(stream1->getDevice(), 0);
}

TEST_F(AccelerationModuleMoveTest, GPUStreamWrapper_MoveAssignment) {
    auto stream1 = std::make_unique<GPUStreamWrapper>();
    auto stream2 = std::make_unique<GPUStreamWrapper>();
    
    *stream2 = std::move(*stream1);
    
    EXPECT_EQ(stream2->getDevice(), 0);
    EXPECT_EQ(stream1->getDevice(), 0);
}

TEST_F(AccelerationModuleMoveTest, GPUStreamWrapper_DeletesCopy) {
    static_assert(!std::is_copy_constructible_v<GPUStreamWrapper>);
    static_assert(!std::is_copy_assignable_v<GPUStreamWrapper>);
}

TEST_F(AccelerationModuleMoveTest, AccelerationModule_MoveNoexceptGuarantee) {
    static_assert(std::is_nothrow_move_constructible_v<GPUKernelHandle>);
    static_assert(std::is_nothrow_move_assignable_v<GPUKernelHandle>);
    static_assert(std::is_nothrow_move_constructible_v<GPUMemoryPool>);
    static_assert(std::is_nothrow_move_assignable_v<GPUMemoryPool>);
}

}  // namespace acceleration
}  // namespace themis

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
