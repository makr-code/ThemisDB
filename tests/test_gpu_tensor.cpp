#include <gtest/gtest.h>
#include "llm/lora_framework/gpu_tensor.h"
#include "llm/lora_framework/lora_layers.h"
#include <vector>
#include <cmath>

using namespace themis::llm::lora;

namespace {
    constexpr float EPSILON = 1e-5f;
}

class GPUTensorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Detect available devices
        auto backends = GPUMemoryManager::detect_backends();
        
        has_cuda_ = false;
        has_hip_ = false;
        
        for (const auto& backend : backends) {
            if (backend.type == themis::acceleration::BackendType::CUDA && backend.available) {
                has_cuda_ = true;
            }
            if (backend.type == themis::acceleration::BackendType::HIP && backend.available) {
                has_hip_ = true;
            }
        }
    }
    
    bool has_cuda_ = false;
    bool has_hip_ = false;
};

// ===== Construction Tests =====

TEST_F(GPUTensorTest, Construction_CPU) {
    GPUTensor tensor({2, 3}, Device::cpu());
    
    EXPECT_EQ(tensor.ndim(), 2);
    EXPECT_EQ(tensor.size(), 6);
    EXPECT_TRUE(tensor.is_cpu());
    EXPECT_FALSE(tensor.is_gpu());
}

TEST_F(GPUTensorTest, Construction_WithValue) {
    GPUTensor tensor({3, 4}, 5.0f, Device::cpu());
    
    auto data = tensor.cpu_data();
    EXPECT_EQ(data.size(), 12);
    
    for (auto val : data) {
        EXPECT_FLOAT_EQ(val, 5.0f);
    }
}

TEST_F(GPUTensorTest, MoveSemantics) {
    GPUTensor tensor1({2, 2}, Device::cpu());
    tensor1.fill(3.0f);
    
    // Move construct
    GPUTensor tensor2(std::move(tensor1));
    EXPECT_EQ(tensor2.size(), 4);
    
    auto data = tensor2.cpu_data();
    for (auto val : data) {
        EXPECT_FLOAT_EQ(val, 3.0f);
    }
}

// ===== Device Migration Tests =====

TEST_F(GPUTensorTest, DeviceMigration_CPUOnly) {
    GPUTensor cpu_tensor({2, 3}, 2.0f, Device::cpu());
    
    // CPU → CPU (should clone)
    auto cpu_tensor2 = cpu_tensor.to(Device::cpu());
    EXPECT_TRUE(cpu_tensor2.is_cpu());
    
    auto data = cpu_tensor2.cpu_data();
    for (auto val : data) {
        EXPECT_FLOAT_EQ(val, 2.0f);
    }
}

TEST_F(GPUTensorTest, DeviceMigration_CPUToCUDA) {
    if (!has_cuda_) {
        GTEST_SKIP() << "CUDA not available";
    }
    
    GPUTensor cpu_tensor({2, 2}, 1.5f, Device::cpu());
    
    // CPU → CUDA
    auto cuda_tensor = cpu_tensor.to(Device::cuda());
    EXPECT_TRUE(cuda_tensor.is_gpu());
    EXPECT_EQ(cuda_tensor.device().type, DeviceType::CUDA);
    
    // Verify data was transferred correctly
    auto data = cuda_tensor.cpu_data();
    for (auto val : data) {
        EXPECT_FLOAT_EQ(val, 1.5f);
    }
}

// ===== Upload/Download Tests =====

TEST_F(GPUTensorTest, UploadDownload_CPU) {
    GPUTensor tensor({3, 2}, Device::cpu());
    
    std::vector<float> input_data = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f};
    tensor.upload(input_data);
    
    auto output_data = tensor.download();
    EXPECT_EQ(output_data.size(), 6);
    
    for (size_t i = 0; i < input_data.size(); i++) {
        EXPECT_FLOAT_EQ(output_data[i], input_data[i]);
    }
}

// ===== Operation Tests =====

TEST_F(GPUTensorTest, Addition_CPU) {
    GPUTensor a({2, 2}, Device::cpu());
    GPUTensor b({2, 2}, Device::cpu());
    
    a.fill(2.0f);
    b.fill(3.0f);
    
    auto c = a + b;
    
    auto data = c.cpu_data();
    for (auto val : data) {
        EXPECT_FLOAT_EQ(val, 5.0f);
    }
}

TEST_F(GPUTensorTest, Subtraction_CPU) {
    GPUTensor a({2, 2}, Device::cpu());
    GPUTensor b({2, 2}, Device::cpu());
    
    a.fill(7.0f);
    b.fill(3.0f);
    
    auto c = a - b;
    
    auto data = c.cpu_data();
    for (auto val : data) {
        EXPECT_FLOAT_EQ(val, 4.0f);
    }
}

TEST_F(GPUTensorTest, ScalarMultiplication_CPU) {
    GPUTensor a({2, 2}, Device::cpu());
    a.fill(3.0f);
    
    auto c = a * 2.0f;
    
    auto data = c.cpu_data();
    for (auto val : data) {
        EXPECT_FLOAT_EQ(val, 6.0f);
    }
}

TEST_F(GPUTensorTest, ElementwiseMultiplication_CPU) {
    GPUTensor a({2, 2}, Device::cpu());
    GPUTensor b({2, 2}, Device::cpu());
    
    a.fill(2.0f);
    b.fill(3.0f);
    
    auto c = a.mul(b);
    
    auto data = c.cpu_data();
    for (auto val : data) {
        EXPECT_FLOAT_EQ(val, 6.0f);
    }
}

TEST_F(GPUTensorTest, MatrixMultiplication_CPU) {
    GPUTensor a({2, 3}, Device::cpu());
    GPUTensor b({3, 2}, Device::cpu());
    
    // A = [[1, 2, 3],
    //      [4, 5, 6]]
    std::vector<float> a_data = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f};
    a.upload(a_data);
    
    // B = [[1, 2],
    //      [3, 4],
    //      [5, 6]]
    std::vector<float> b_data = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f};
    b.upload(b_data);
    
    // C = A @ B = [[22, 28],
    //              [49, 64]]
    auto c = a.matmul(b);
    
    EXPECT_EQ(c.shape()[0], 2);
    EXPECT_EQ(c.shape()[1], 2);
    
    auto c_data = c.cpu_data();
    EXPECT_FLOAT_EQ(c_data[0], 22.0f);
    EXPECT_FLOAT_EQ(c_data[1], 28.0f);
    EXPECT_FLOAT_EQ(c_data[2], 49.0f);
    EXPECT_FLOAT_EQ(c_data[3], 64.0f);
}

TEST_F(GPUTensorTest, Transpose_CPU) {
    GPUTensor a({2, 3}, Device::cpu());
    
    // A = [[1, 2, 3],
    //      [4, 5, 6]]
    std::vector<float> a_data = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f};
    a.upload(a_data);
    
    // A^T = [[1, 4],
    //        [2, 5],
    //        [3, 6]]
    auto at = a.transpose();
    
    EXPECT_EQ(at.shape()[0], 3);
    EXPECT_EQ(at.shape()[1], 2);
    
    auto at_data = at.cpu_data();
    EXPECT_FLOAT_EQ(at_data[0], 1.0f);
    EXPECT_FLOAT_EQ(at_data[1], 4.0f);
    EXPECT_FLOAT_EQ(at_data[2], 2.0f);
    EXPECT_FLOAT_EQ(at_data[3], 5.0f);
    EXPECT_FLOAT_EQ(at_data[4], 3.0f);
    EXPECT_FLOAT_EQ(at_data[5], 6.0f);
}

TEST_F(GPUTensorTest, Fill_CPU) {
    GPUTensor tensor({3, 3}, Device::cpu());
    tensor.fill(7.5f);
    
    auto data = tensor.cpu_data();
    for (auto val : data) {
        EXPECT_FLOAT_EQ(val, 7.5f);
    }
}

TEST_F(GPUTensorTest, Zero_CPU) {
    GPUTensor tensor({2, 2}, 5.0f, Device::cpu());
    tensor.zero();
    
    auto data = tensor.cpu_data();
    for (auto val : data) {
        EXPECT_FLOAT_EQ(val, 0.0f);
    }
}

TEST_F(GPUTensorTest, Clone_CPU) {
    GPUTensor a({2, 2}, Device::cpu());
    a.fill(3.14f);
    
    auto b = a.clone();
    
    EXPECT_EQ(b.shape(), a.shape());
    EXPECT_EQ(b.device().type, a.device().type);
    
    auto b_data = b.cpu_data();
    for (auto val : b_data) {
        EXPECT_FLOAT_EQ(val, 3.14f);
    }
}

// ===== Gradient Tests =====

TEST_F(GPUTensorTest, Gradient_ZeroGrad) {
    GPUTensor tensor({2, 2}, Device::cpu());
    tensor.requires_grad = true;
    tensor.ensure_grad();
    
    tensor.grad->fill(5.0f);
    
    // Zero grad
    tensor.zero_grad();
    
    auto grad_data = tensor.grad->cpu_data();
    for (auto val : grad_data) {
        EXPECT_FLOAT_EQ(val, 0.0f);
    }
}

// ===== Utility Function Tests =====

TEST_F(GPUTensorTest, Utility_Zeros) {
    auto tensor = gpu_tensor_utils::zeros({3, 3}, Device::cpu());
    
    auto data = tensor.cpu_data();
    for (auto val : data) {
        EXPECT_FLOAT_EQ(val, 0.0f);
    }
}

TEST_F(GPUTensorTest, Utility_Ones) {
    auto tensor = gpu_tensor_utils::ones({2, 4}, Device::cpu());
    
    auto data = tensor.cpu_data();
    for (auto val : data) {
        EXPECT_FLOAT_EQ(val, 1.0f);
    }
}

TEST_F(GPUTensorTest, Utility_Randn) {
    auto tensor = gpu_tensor_utils::randn({10, 10}, 0.0f, 1.0f, Device::cpu());
    
    auto data = tensor.cpu_data();
    EXPECT_EQ(data.size(), 100);
    
    // Check that values are distributed (not all the same)
    float sum = 0.0f;
    for (auto val : data) {
        sum += val;
    }
    float mean = sum / data.size();
    
    // Mean should be close to 0 for large samples
    EXPECT_NEAR(mean, 0.0f, 0.5f);
}

TEST_F(GPUTensorTest, Utility_XavierUniform) {
    auto tensor = gpu_tensor_utils::xavier_uniform({64, 64}, Device::cpu());
    
    auto data = tensor.cpu_data();
    EXPECT_EQ(data.size(), 64 * 64);
    
    // Values should be within expected range
    float limit = std::sqrt(6.0f / (64.0f + 64.0f));
    for (auto val : data) {
        EXPECT_GE(val, -limit);
        EXPECT_LE(val, limit);
    }
}

TEST_F(GPUTensorTest, Utility_KaimingUniform) {
    auto tensor = gpu_tensor_utils::kaiming_uniform({32, 32}, 0.0f, Device::cpu());
    
    auto data = tensor.cpu_data();
    EXPECT_EQ(data.size(), 32 * 32);
    
    // Values should be initialized (not all zero)
    bool has_nonzero = false;
    for (auto val : data) {
        if (std::abs(val) > EPSILON) {
            has_nonzero = true;
            break;
        }
    }
    EXPECT_TRUE(has_nonzero);
}

// ===== CUDA Tests (conditional) =====

TEST_F(GPUTensorTest, CUDA_Construction) {
    if (!has_cuda_) {
        GTEST_SKIP() << "CUDA not available";
    }
    
    GPUTensor tensor({2, 3}, Device::cuda());
    
    EXPECT_TRUE(tensor.is_gpu());
    EXPECT_EQ(tensor.device().type, DeviceType::CUDA);
    EXPECT_EQ(tensor.size(), 6);
}

TEST_F(GPUTensorTest, CUDA_UploadDownload) {
    if (!has_cuda_) {
        GTEST_SKIP() << "CUDA not available";
    }
    
    GPUTensor tensor({2, 2}, Device::cuda());
    
    std::vector<float> input = {1.0f, 2.0f, 3.0f, 4.0f};
    tensor.upload(input);
    
    auto output = tensor.download();
    
    for (size_t i = 0; i < input.size(); i++) {
        EXPECT_FLOAT_EQ(output[i], input[i]);
    }
}

TEST_F(GPUTensorTest, CUDA_Operations) {
    if (!has_cuda_) {
        GTEST_SKIP() << "CUDA not available";
    }
    
    GPUTensor a({2, 2}, Device::cuda());
    GPUTensor b({2, 2}, Device::cuda());
    
    a.fill(2.0f);
    b.fill(3.0f);
    
    // Test addition on GPU
    auto c = a + b;
    
    auto data = c.cpu_data();
    for (auto val : data) {
        EXPECT_FLOAT_EQ(val, 5.0f);
    }
}

// ===== Legacy Tensor Conversion Tests =====

TEST_F(GPUTensorTest, LegacyConversion_ToGPU) {
    Tensor legacy({2, 2});
    legacy.fill(2.5f);
    
    auto gpu_tensor = gpu_tensor_utils::from_legacy_tensor(legacy, Device::cpu());
    
    EXPECT_EQ(gpu_tensor.shape(), legacy.shape());
    
    auto data = gpu_tensor.cpu_data();
    for (size_t i = 0; i < data.size(); i++) {
        EXPECT_FLOAT_EQ(data[i], legacy.data()[i]);
    }
}

TEST_F(GPUTensorTest, LegacyConversion_FromGPU) {
    GPUTensor gpu_tensor({2, 2}, 3.5f, Device::cpu());
    
    auto legacy = gpu_tensor_utils::to_legacy_tensor(gpu_tensor);
    
    EXPECT_EQ(legacy.shape(), gpu_tensor.shape());
    
    for (size_t i = 0; i < legacy.size(); i++) {
        EXPECT_FLOAT_EQ(legacy.data()[i], 3.5f);
    }
}
