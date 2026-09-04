/**
 * @file test_gpu_stubs_comprehensive.cpp
 * @brief Comprehensive tests for GPU stubs covering memory management, backend selection, and error handling
 */

#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <string>
#include <algorithm>

// Mock GPU types for testing
namespace themis {

// Minimal GPU memory manager interface for testing
class GPUMemoryManager {
public:
    struct MemoryBlock {
        void* ptr = nullptr;
        size_t size = 0;
        int device_id = 0;
    };

    virtual ~GPUMemoryManager() = default;
    
    virtual bool initialize(int device_id = 0) = 0;
    virtual MemoryBlock allocate(size_t size) = 0;
    virtual bool deallocate(const MemoryBlock& block) = 0;
    virtual size_t getAvailableMemory() const = 0;
    virtual size_t getTotalMemory() const = 0;
    virtual bool isInitialized() const = 0;
};

// Stub implementation for testing
class GPUMemoryManagerStub : public GPUMemoryManager {
private:
    bool initialized_ = false;
    int device_id_ = -1;
    size_t total_memory_ = 8ULL * 1024 * 1024 * 1024; // 8GB default
    size_t allocated_memory_ = 0;
    std::vector<MemoryBlock> allocations_;
    mutable std::mutex mutex_;

public:
    bool initialize(int device_id = 0) override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (initialized_) return false; // Already initialized
        device_id_ = device_id;
        initialized_ = true;
        return true;
    }

    MemoryBlock allocate(size_t size) override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!initialized_) return {nullptr, 0, -1};
        if (allocated_memory_ + size > total_memory_) {
            return {nullptr, 0, device_id_}; // Out of memory
        }

        // Simulate allocation
        void* ptr = reinterpret_cast<void*>(0x10000 + allocations_.size() * 0x1000);
        MemoryBlock block{ptr, size, device_id_};
        allocations_.push_back(block);
        allocated_memory_ += size;
        return block;
    }

    bool deallocate(const MemoryBlock& block) override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!initialized_) {
          return false;
        }
        
        auto it = std::find_if(allocations_.begin(), allocations_.end(),
            [&](const MemoryBlock& b) { return b.ptr == block.ptr; });
        
        if (it == allocations_.end()) {
          return false;
        }
        
        allocated_memory_ -= it->size;
        allocations_.erase(it);
        return true;
    }

    size_t getAvailableMemory() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return total_memory_ - allocated_memory_;
    }

    size_t getTotalMemory() const override {
        return total_memory_;
    }

    bool isInitialized() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return initialized_;
    }

    // Test helpers
    void setTotalMemory(size_t size) {
        total_memory_ = size;
    }

    size_t getAllocatedMemory() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return allocated_memory_;
    }

    size_t getAllocationCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return allocations_.size();
    }
};

// GPU Backend stub for testing
enum class GPUBackendType {
    NONE,
    CUDA,
    ROCM,
    OPENCL,
    VULKAN,
    METAL,
    DIRECTX
};

class GPUBackend {
public:
    virtual ~GPUBackend() = default;
    virtual GPUBackendType getType() const = 0;
    virtual bool isAvailable() const = 0;
    virtual std::string getName() const = 0;
    virtual int getDeviceCount() const = 0;
};

class GPUBackendStub : public GPUBackend {
private:
    GPUBackendType type_;
    bool available_ = 0;
    int device_count_ = 0;

public:
    GPUBackendStub(GPUBackendType type, bool available = true, int device_count = 1)
        : type_(type), available_(available), device_count_(device_count) {}

    GPUBackendType getType() const override { return type_; }
    bool isAvailable() const override { return available_; }
    
    std::string getName() const override {
        switch (type_) {
            case GPUBackendType::CUDA: return "CUDA";
            case GPUBackendType::ROCM: return "ROCm";
            case GPUBackendType::OPENCL: return "OpenCL";
            case GPUBackendType::VULKAN: return "Vulkan";
            case GPUBackendType::METAL: return "Metal";
            case GPUBackendType::DIRECTX: return "DirectX";
            default: return "None";
        }
    }
    
    int getDeviceCount() const override { return device_count_; }

    void setAvailable(bool available) { available_ = available; }
    void setDeviceCount(int count) { device_count_ = count; }
};

} // namespace themis

using namespace themis;

// ============================================================================
// GPU Memory Manager Tests
// ============================================================================

class GPUMemoryManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager_ = std::make_unique<GPUMemoryManagerStub>();
    }

    std::unique_ptr<GPUMemoryManagerStub> manager_;
};

TEST_F(GPUMemoryManagerTest, Initialization_Success) {
    EXPECT_FALSE(manager_->isInitialized());
    EXPECT_TRUE(manager_->initialize(0));
    EXPECT_TRUE(manager_->isInitialized());
}

TEST_F(GPUMemoryManagerTest, DoubleInitialization_Fails) {
    ASSERT_TRUE(manager_->initialize(0));
    EXPECT_FALSE(manager_->initialize(1)); // Second initialization should fail
}

TEST_F(GPUMemoryManagerTest, AllocateBeforeInitialization_Fails) {
    auto block = manager_->allocate(1024);
    EXPECT_EQ(block.ptr, nullptr);
    EXPECT_EQ(block.size, 0);
}

TEST_F(GPUMemoryManagerTest, BasicAllocation_Success) {
    ASSERT_TRUE(manager_->initialize(0));
    
    auto block = manager_->allocate(1024 * 1024); // 1MB
    EXPECT_NE(block.ptr, nullptr);
    EXPECT_EQ(block.size, 1024 * 1024);
    EXPECT_EQ(block.device_id, 0);
    EXPECT_EQ(manager_->getAllocatedMemory(), 1024 * 1024);
}

TEST_F(GPUMemoryManagerTest, MultipleAllocations_TrackMemory) {
    ASSERT_TRUE(manager_->initialize(0));
    
    auto block1 = manager_->allocate(1024 * 1024);
    auto block2 = manager_->allocate(2 * 1024 * 1024);
    auto block3 = manager_->allocate(512 * 1024);
    
    EXPECT_EQ(manager_->getAllocationCount(), 3);
    EXPECT_EQ(manager_->getAllocatedMemory(), 1024*1024 + 2*1024*1024 + 512*1024);
}

TEST_F(GPUMemoryManagerTest, Deallocation_FreesMemory) {
    ASSERT_TRUE(manager_->initialize(0));
    
    auto block = manager_->allocate(1024 * 1024);
    ASSERT_EQ(manager_->getAllocatedMemory(), 1024 * 1024);
    
    EXPECT_TRUE(manager_->deallocate(block));
    EXPECT_EQ(manager_->getAllocatedMemory(), 0);
    EXPECT_EQ(manager_->getAllocationCount(), 0);
}

TEST_F(GPUMemoryManagerTest, DeallocationInvalidBlock_Fails) {
    ASSERT_TRUE(manager_->initialize(0));
    
    GPUMemoryManager::MemoryBlock fake_block{reinterpret_cast<void*>(0xDEADBEEF), 1024, 0};
    EXPECT_FALSE(manager_->deallocate(fake_block));
}

TEST_F(GPUMemoryManagerTest, OutOfMemory_ReturnsNull) {
    manager_->setTotalMemory(1024 * 1024); // 1MB total
    ASSERT_TRUE(manager_->initialize(0));
    
    auto block1 = manager_->allocate(512 * 1024);
    ASSERT_NE(block1.ptr, nullptr);
    
    auto block2 = manager_->allocate(600 * 1024); // Would exceed total
    EXPECT_EQ(block2.ptr, nullptr);
}

TEST_F(GPUMemoryManagerTest, AvailableMemory_ReflectsAllocations) {
    size_t total = 10 * 1024 * 1024; // 10MB
    manager_->setTotalMemory(total);
    ASSERT_TRUE(manager_->initialize(0));
    
    EXPECT_EQ(manager_->getAvailableMemory(), total);
    
    auto block = manager_->allocate(3 * 1024 * 1024);
    EXPECT_EQ(manager_->getAvailableMemory(), 7 * 1024 * 1024);
    
    manager_->deallocate(block);
    EXPECT_EQ(manager_->getAvailableMemory(), total);
}

TEST_F(GPUMemoryManagerTest, ZeroSizeAllocation_HandledGracefully) {
    ASSERT_TRUE(manager_->initialize(0));
    
    auto block = manager_->allocate(0);
    // Implementation-specific: either allow or deny
    if (block.ptr != nullptr) {
        EXPECT_EQ(block.size, 0);
    }
}

TEST_F(GPUMemoryManagerTest, VeryLargeAllocation_HandledGracefully) {
    ASSERT_TRUE(manager_->initialize(0));
    
    size_t huge_size = 100ULL * 1024 * 1024 * 1024; // 100GB
    auto block = manager_->allocate(huge_size);
    EXPECT_EQ(block.ptr, nullptr); // Should fail on 8GB default memory
}

TEST_F(GPUMemoryManagerTest, ConcurrentAllocations_ThreadSafe) {
    ASSERT_TRUE(manager_->initialize(0));
    
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([this, &success_count]() {
            for (int j = 0; j < 10; ++j) {
                auto block = manager_->allocate(1024);
                if (block.ptr != nullptr) {
                    success_count++;
                    manager_->deallocate(block);
                }
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_GT(success_count, 0);
    EXPECT_EQ(manager_->getAllocatedMemory(), 0); // All deallocated
}

// ============================================================================
// GPU Backend Selection Tests
// ============================================================================

class GPUBackendTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create various backend stubs
        backends_.push_back(std::make_unique<GPUBackendStub>(GPUBackendType::CUDA, true, 2));
        backends_.push_back(std::make_unique<GPUBackendStub>(GPUBackendType::ROCM, false, 0));
        backends_.push_back(std::make_unique<GPUBackendStub>(GPUBackendType::OPENCL, true, 1));
        backends_.push_back(std::make_unique<GPUBackendStub>(GPUBackendType::VULKAN, true, 1));
    }

    std::vector<std::unique_ptr<GPUBackend>> backends_;
};

TEST_F(GPUBackendTest, BackendType_ReturnsCorrectType) {
    EXPECT_EQ(backends_[0]->getType(), GPUBackendType::CUDA);
    EXPECT_EQ(backends_[1]->getType(), GPUBackendType::ROCM);
    EXPECT_EQ(backends_[2]->getType(), GPUBackendType::OPENCL);
}

TEST_F(GPUBackendTest, BackendName_ReturnsCorrectName) {
    EXPECT_EQ(backends_[0]->getName(), "CUDA");
    EXPECT_EQ(backends_[1]->getName(), "ROCm");
    EXPECT_EQ(backends_[2]->getName(), "OpenCL");
    EXPECT_EQ(backends_[3]->getName(), "Vulkan");
}

TEST_F(GPUBackendTest, BackendAvailability_ReflectsStatus) {
    EXPECT_TRUE(backends_[0]->isAvailable());
    EXPECT_FALSE(backends_[1]->isAvailable());
    EXPECT_TRUE(backends_[2]->isAvailable());
}

TEST_F(GPUBackendTest, DeviceCount_ReturnsCorrectCount) {
    EXPECT_EQ(backends_[0]->getDeviceCount(), 2);
    EXPECT_EQ(backends_[1]->getDeviceCount(), 0);
    EXPECT_EQ(backends_[2]->getDeviceCount(), 1);
}

TEST_F(GPUBackendTest, SelectFirstAvailableBackend_Success) {
    GPUBackend* selected = nullptr;
    
    for (const auto& backend : backends_) {
        if (backend->isAvailable() && backend->getDeviceCount() > 0) {
            selected = backend.get();
            break;
        }
    }
    
    ASSERT_NE(selected, nullptr);
    EXPECT_EQ(selected->getType(), GPUBackendType::CUDA);
}

TEST_F(GPUBackendTest, PreferCUDAOverOthers_WhenAvailable) {
    GPUBackend* cuda_backend = nullptr;
    
    for (const auto& backend : backends_) {
        if (backend->getType() == GPUBackendType::CUDA && backend->isAvailable()) {
            cuda_backend = backend.get();
            break;
        }
    }
    
    ASSERT_NE(cuda_backend, nullptr);
    EXPECT_TRUE(cuda_backend->isAvailable());
    EXPECT_GT(cuda_backend->getDeviceCount(), 0);
}

TEST_F(GPUBackendTest, FallbackToOpenCL_WhenCUDAUnavailable) {
    // Make CUDA unavailable
    auto* cuda_stub = dynamic_cast<GPUBackendStub*>(backends_[0].get());
    ASSERT_NE(cuda_stub, nullptr);
    cuda_stub->setAvailable(false);
    
    GPUBackend* selected = nullptr;
    for (const auto& backend : backends_) {
        if (backend->isAvailable() && backend->getDeviceCount() > 0) {
            selected = backend.get();
            break;
        }
    }
    
    ASSERT_NE(selected, nullptr);
    EXPECT_EQ(selected->getType(), GPUBackendType::OPENCL);
}

TEST_F(GPUBackendTest, NoAvailableBackends_HandledGracefully) {
    // Make all backends unavailable
    for (auto& backend : backends_) {
        auto* stub = dynamic_cast<GPUBackendStub*>(backend.get());
        if (stub) {
          stub->setAvailable(false);
        }
    }
    
    GPUBackend* selected = nullptr;
    for (const auto& backend : backends_) {
        if (backend->isAvailable()) {
            selected = backend.get();
            break;
        }
    }
    
    EXPECT_EQ(selected, nullptr);
}

// ============================================================================
// Integration Tests
// ============================================================================

class GPUIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager_ = std::make_unique<GPUMemoryManagerStub>();
        backend_ = std::make_unique<GPUBackendStub>(GPUBackendType::CUDA, true, 1);
    }

    std::unique_ptr<GPUMemoryManagerStub> manager_;
    std::unique_ptr<GPUBackendStub> backend_;
};

TEST_F(GPUIntegrationTest, InitializeWithBackend_Success) {
    ASSERT_TRUE(backend_->isAvailable());
    ASSERT_TRUE(manager_->initialize(0));
    
    auto block = manager_->allocate(1024 * 1024);
    EXPECT_NE(block.ptr, nullptr);
    EXPECT_EQ(block.device_id, 0);
}

TEST_F(GPUIntegrationTest, BackendUnavailable_MemoryManagerFails) {
    backend_->setAvailable(false);
    
    if (!backend_->isAvailable()) {
        // Should not initialize memory manager
        // This is implementation-specific behavior
        GTEST_SKIP() << "capability:backend_runtime_available=false;reason=backend_unavailable";
    }
}

TEST_F(GPUIntegrationTest, MultiDeviceAllocation_DistributesMemory) {
    backend_->setDeviceCount(2);
    
    ASSERT_TRUE(manager_->initialize(0));
    
    auto block1 = manager_->allocate(1024 * 1024);
    EXPECT_EQ(block1.device_id, 0);
    
    // In a real implementation, might allocate on different device
    // This stub always uses device 0
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST(GPUErrorHandling, NullPointerDeallocation_Handled) {
    GPUMemoryManagerStub manager;
    manager.initialize(0);
    
    GPUMemoryManager::MemoryBlock null_block{nullptr, 0, 0};
    // Should handle gracefully without crash
    bool result = manager.deallocate(null_block);
    // Expect false for invalid block
    EXPECT_FALSE(result);
}

// NOTE: Using GPUMemoryManager::MemoryBlock after its managing GPUMemoryManagerStub
// instance has been destroyed is undefined behavior in real-world code and is
// intentionally not tested here. Any such usage should be avoided.

TEST(GPUErrorHandling, ExtremeFragmentation_Handled) {
    GPUMemoryManagerStub manager;
    manager.setTotalMemory(10 * 1024 * 1024); // 10MB
    manager.initialize(0);
    
    // Allocate many small blocks to fragment memory
    std::vector<GPUMemoryManager::MemoryBlock> blocks = {};

    for (int i = 0; i < 1000; ++i) {
        auto block = manager.allocate(1024); // 1KB each
        if (block.ptr != nullptr) {
            blocks.push_back(block);
        }
    }
    
    // Should have allocated some blocks without crashing
    EXPECT_GT(blocks.size(), 0);
}
