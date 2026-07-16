/**
 * @file test_cuda_operations_move_semantics.cpp
 * @brief Tests for CUDA operations use-after-move detection
 * @version 0.1.0
 */

#include <gtest/gtest.h>
#include "gpu/cuda_operations.h"
#include <utility>
#include <chrono>

using namespace themis::gpu;

class CudaOperationsTest : public ::testing::Test {
protected:
    // Note: Tests may require actual GPU; failures are expected without CUDA
};

// Test: CudaStream move constructor
TEST_F(CudaOperationsTest, CudaStreamMoveConstructor) {
    // This may fail without GPU
    try {
        CudaStream src(0, 0);
        CudaStream dst(std::move(src));

        EXPECT_TRUE(src.is_moved_from());
        EXPECT_TRUE(dst.is_valid());
    } catch (const std::runtime_error&) {
        SKIP();  // GPU not available
    }
}

// Test: CudaStream move assignment
TEST_F(CudaOperationsTest, CudaStreamMoveAssignment) {
    try {
        CudaStream src(0, 0);
        CudaStream dst(0, 0);

        dst = std::move(src);
        EXPECT_TRUE(src.is_moved_from());
        EXPECT_TRUE(dst.is_valid());
    } catch (const std::runtime_error&) {
        SKIP();
    }
}

// Test: Moved-from CudaStream throws on operations
TEST_F(CudaOperationsTest, MovedFromStreamThrowsOnGetHandle) {
    try {
        CudaStream src(0, 0);
        CudaStream dst(std::move(src));

        EXPECT_THROW({
            src.get_handle();
        }, std::logic_error);
    } catch (const std::runtime_error&) {
        SKIP();
    }
}

// Test: Moved-from CudaStream throws on synchronize
TEST_F(CudaOperationsTest, MovedFromStreamThrowsOnSynchronize) {
    try {
        CudaStream src(0, 0);
        CudaStream dst(std::move(src));

        EXPECT_THROW({
            src.synchronize();
        }, std::logic_error);
    } catch (const std::runtime_error&) {
        SKIP();
    }
}

// Test: Moved-from CudaStream returns true for is_ready()
TEST_F(CudaOperationsTest, MovedFromStreamIsReady) {
    try {
        CudaStream src(0, 0);
        CudaStream dst(std::move(src));

        EXPECT_TRUE(src.is_ready());
    } catch (const std::runtime_error&) {
        SKIP();
    }
}

// Test: Moved-from CudaStream is not valid
TEST_F(CudaOperationsTest, MovedFromStreamNotValid) {
    try {
        CudaStream src(0, 0);
        CudaStream dst(std::move(src));

        EXPECT_FALSE(src.is_valid());
    } catch (const std::runtime_error&) {
        SKIP();
    }
}

// Test: CudaOperation move constructor
TEST_F(CudaOperationsTest, CudaOperationMoveConstructor) {
    try {
        CudaStream stream(0, 0);
        CudaOperation src(stream, "test");
        CudaOperation dst(std::move(src));

        EXPECT_TRUE(src.is_moved_from());
        EXPECT_FALSE(dst.is_moved_from());
    } catch (const std::runtime_error&) {
        SKIP();
    }
}

// Test: CudaOperation move assignment
TEST_F(CudaOperationsTest, CudaOperationMoveAssignment) {
    try {
        CudaStream stream(0, 0);
        CudaOperation src(stream, "test");
        CudaOperation dst(stream, "test");

        dst = std::move(src);
        EXPECT_TRUE(src.is_moved_from());
        EXPECT_FALSE(dst.is_moved_from());
    } catch (const std::runtime_error&) {
        SKIP();
    }
}

// Test: Moved-from operation throws on record_event
TEST_F(CudaOperationsTest, MovedFromOperationThrowsOnRecordEvent) {
    try {
        CudaStream stream(0, 0);
        CudaOperation src(stream, "test");
        CudaOperation dst(std::move(src));

        EXPECT_THROW({
            src.record_event();
        }, std::logic_error);
    } catch (const std::runtime_error&) {
        SKIP();
    }
}

// Test: Moved-from operation throws on wait
TEST_F(CudaOperationsTest, MovedFromOperationThrowsOnWait) {
    try {
        CudaStream stream(0, 0);
        CudaOperation src(stream, "test");
        CudaOperation dst(std::move(src));

        EXPECT_THROW({
            src.wait();
        }, std::logic_error);
    } catch (const std::runtime_error&) {
        SKIP();
    }
}

// Test: Moved-from operation status is MOVED_FROM
TEST_F(CudaOperationsTest, MovedFromOperationStatus) {
    try {
        CudaStream stream(0, 0);
        CudaOperation src(stream, "test");
        CudaOperation dst(std::move(src));

        EXPECT_EQ(src.get_status(), CudaOperation::Status::MOVED_FROM);
    } catch (const std::runtime_error&) {
        SKIP();
    }
}

// Test: Moved-from operation returns empty name
TEST_F(CudaOperationsTest, MovedFromOperationEmptyName) {
    try {
        CudaStream stream(0, 0);
        CudaOperation src(stream, "test");
        CudaOperation dst(std::move(src));

        EXPECT_TRUE(src.get_name().empty());
    } catch (const std::runtime_error&) {
        SKIP();
    }
}

// Test: CudaOperationBatch move semantics
TEST_F(CudaOperationsTest, CudaOperationBatchMoveSemantics) {
    try {
        CudaStream stream(0, 0);
        CudaOperationBatch src(stream);
        CudaOperationBatch dst(std::move(src));

        EXPECT_TRUE(src.is_moved_from());
        EXPECT_TRUE(dst.is_valid());
    } catch (const std::runtime_error&) {
        SKIP();
    }
}

// Test: Moved-from batch throws on add_operation
TEST_F(CudaOperationsTest, MovedFromBatchThrowsOnAddOperation) {
    try {
        CudaStream stream(0, 0);
        CudaOperationBatch src(stream);
        CudaOperationBatch dst(std::move(src));

        CudaOperation op(stream, "test");
        EXPECT_THROW({
            src.add_operation(std::move(op));
        }, std::logic_error);
    } catch (const std::runtime_error&) {
        SKIP();
    }
}

// Test: Moved-from batch size is 0
TEST_F(CudaOperationsTest, MovedFromBatchSizeZero) {
    try {
        CudaStream stream(0, 0);
        CudaOperationBatch src(stream);
        CudaOperationBatch dst(std::move(src));

        EXPECT_EQ(src.size(), 0);
    } catch (const std::runtime_error&) {
        SKIP();
    }
}

// Test: Destructor safe on moved-from objects
TEST_F(CudaOperationsTest, DestructorSafeOnMovedFrom) {
    try {
        {
            CudaStream stream(0, 0);
            CudaStream moved(std::move(stream));
        }  // Should not crash
        
        {
            CudaStream stream(0, 0);
            CudaOperation op(stream, "test");
            CudaOperation moved(std::move(op));
        }  // Should not crash
    } catch (const std::runtime_error&) {
        SKIP();
    }
}
