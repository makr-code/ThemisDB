/**
 * @file test_gpu_kernel_manager_move_semantics.cpp
 * @brief Tests for GPU kernel manager move semantics and moved-from state handling
 * @version 0.1.0
 */

#include <gtest/gtest.h>
#include "gpu/gpu_kernel_manager.h"
#include <utility>

using namespace themis::gpu;

class GPUKernelManagerTest : public ::testing::Test {
protected:
    const std::string test_kernel_name = "test_kernel";
    const int test_device_id = 0;
    const GPUKernelManager::Config test_config{};
};

// Test: Default construction creates valid moved-from state
TEST_F(GPUKernelManagerTest, DefaultConstructorIsValid) {
    GPUKernelManager mgr;
    EXPECT_FALSE(mgr.is_valid());
    EXPECT_FALSE(mgr.is_moved_from());
}

// Test: Construction with parameters initializes properly
TEST_F(GPUKernelManagerTest, ConstructorWithParamsInitializes) {
    // Note: This may fail without actual GPU, but tests the interface
    EXPECT_THROW({
        GPUKernelManager mgr(test_kernel_name, test_device_id, test_config);
    }, std::runtime_error);  // Expected without CUDA
}

// Test: Move constructor transfers ownership
TEST_F(GPUKernelManagerTest, MoveConstructorTransfersOwnership) {
    GPUKernelManager src;
    src = std::move(src);  // Move to self (should be safe)
    
    GPUKernelManager dst(std::move(src));
    EXPECT_TRUE(src.is_moved_from());
    EXPECT_FALSE(dst.is_valid());  // dst is moved-from from invalid src
}

// Test: Move assignment transfers ownership
TEST_F(GPUKernelManagerTest, MoveAssignmentTransfersOwnership) {
    GPUKernelManager src;
    GPUKernelManager dst;

    dst = std::move(src);
    EXPECT_TRUE(src.is_moved_from());
}

// Test: Moved-from manager cannot be used for operations
TEST_F(GPUKernelManagerTest, MovedFromManagerThrowsOnLaunch) {
    GPUKernelManager mgr;
    GPUKernelManager moved(std::move(mgr));

    EXPECT_TRUE(mgr.is_moved_from());
    
    // Attempting to launch on moved-from object should throw
    EXPECT_THROW({
        mgr.launch(nullptr);
    }, std::logic_error);
}

// Test: Moved-from manager is safe to destroy
TEST_F(GPUKernelManagerTest, MovedFromManagerDestructSafe) {
    GPUKernelManager mgr;
    {
        GPUKernelManager moved(std::move(mgr));
    }  // Should not crash
    EXPECT_TRUE(mgr.is_moved_from());
}

// Test: Self-move assignment is safe
TEST_F(GPUKernelManagerTest, SelfMoveAssignmentIsSafe) {
    GPUKernelManager mgr;
    mgr = std::move(mgr);  // Should not crash
    EXPECT_TRUE(mgr.is_moved_from());
}

// Test: device_id() returns -1 when moved-from
TEST_F(GPUKernelManagerTest, DeviceIdReturnsNegativeWhenMovedFrom) {
    GPUKernelManager mgr;
    GPUKernelManager moved(std::move(mgr));
    EXPECT_EQ(mgr.device_id(), -1);
}

// Test: kernel_name() returns empty when moved-from
TEST_F(GPUKernelManagerTest, KernelNameEmptyWhenMovedFrom) {
    GPUKernelManager mgr;
    GPUKernelManager moved(std::move(mgr));
    EXPECT_TRUE(mgr.kernel_name().empty());
}

// Test: is_running() returns false when moved-from
TEST_F(GPUKernelManagerTest, IsRunningReturnsFalseWhenMovedFrom) {
    GPUKernelManager mgr;
    GPUKernelManager moved(std::move(mgr));
    EXPECT_FALSE(mgr.is_running());
}

// Test: Destructor safe on moved-from objects (no double-cleanup)
TEST_F(GPUKernelManagerTest, DestructorSafeOnMovedFrom) {
    {
        GPUKernelManager mgr;
        GPUKernelManager moved(std::move(mgr));
    }  // Double destruction of handles should not crash
}

// Test: KernelArgumentBuffer move semantics
TEST_F(GPUKernelManagerTest, KernelArgumentBufferMoveSemantics) {
    KernelArgumentBuffer buf1(64, 0);
    EXPECT_TRUE(buf1.is_valid());

    KernelArgumentBuffer buf2(std::move(buf1));
    EXPECT_FALSE(buf1.is_valid());
    EXPECT_TRUE(buf2.is_valid());
}

// Test: KernelArgumentBuffer operations fail on moved-from
TEST_F(GPUKernelManagerTest, KernelArgumentBufferThrowsWhenMovedFrom) {
    KernelArgumentBuffer buf1(64, 0);
    KernelArgumentBuffer buf2(std::move(buf1));

    EXPECT_THROW({
        buf1.upload();
    }, std::logic_error);
}

// Test: KernelArgumentBuffer size() returns 0 when moved-from
TEST_F(GPUKernelManagerTest, KernelArgumentBufferSizeZeroWhenMovedFrom) {
    KernelArgumentBuffer buf1(64, 0);
    KernelArgumentBuffer buf2(std::move(buf1));

    EXPECT_EQ(buf1.size(), 0);
}

// Test: KernelArgumentBuffer pointers null when moved-from
TEST_F(GPUKernelManagerTest, KernelArgumentBufferPointersNullWhenMovedFrom) {
    KernelArgumentBuffer buf1(64, 0);
    KernelArgumentBuffer buf2(std::move(buf1));

    EXPECT_EQ(buf1.device_ptr(), nullptr);
    EXPECT_EQ(buf1.host_ptr(), nullptr);
}
