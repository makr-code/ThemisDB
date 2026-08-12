/**
 * @file gpu_tensor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=6; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "llm/lora_framework/gpu_memory.h"
#include "llm/lora_framework/vram_allocator.h"
#include "llm/lora_framework/tensor_dtype.h"
#include <vector>
#include <memory>
#include <cstddef>
#include <functional>
#include <mutex>

namespace themis {
namespace llm {
namespace lora {

class Tensor;

/**
 * @brief GPU-enabled Tensor class for LoRA training
 * 
 * Extends the basic Tensor class to support GPU backends.
 * Tensors can reside on CPU or GPU (VRAM) and operations
 * are dispatched to the appropriate backend.
 */
class GPUTensor {
public:
    GPUTensor() = default;
    
    /**
     * @brief Construct tensor with shape on specified device
     * @param shape Tensor dimensions
     * @param device Target device (CPU, CUDA, HIP, Vulkan, DirectX)
     * @param dtype Data type (default: FP32)
     */
    GPUTensor(const std::vector<size_t>& shape, 
              const Device& device = Device::cpu(),
              DType dtype = DType::FLOAT32);
    
    /**
     * @brief Construct tensor with shape and initial value on device
     * @param shape Tensor dimensions
     * @param value Initial value
     * @param device Target device
     * @param dtype Data type (default: FP32)
     */
    GPUTensor(const std::vector<size_t>& shape, 
              float value, 
              const Device& device = Device::cpu(),
              DType dtype = DType::FLOAT32);
    
    ~GPUTensor();
    
    // Disable copy, enable move
    GPUTensor(const GPUTensor&) = delete;
    GPUTensor& operator=(const GPUTensor&) = delete;
    GPUTensor(GPUTensor&& other) noexcept;
    GPUTensor& operator=(GPUTensor&& other) noexcept;
    
    // ========== Device Management ==========
    
    /**
     * @brief Get current device location
     */
    Device device() const { return device_; }
    
    /**
     * @brief Move tensor to different device
     * @param target_device Destination device
     * @return New tensor on target device (this tensor remains valid)
     */
    GPUTensor to(const Device& target_device) const;
    
    /**
     * @brief Move tensor to device in-place
     * @param target_device Destination device
     */
    void to_inplace(const Device& target_device);
    
    /**
     * @brief Check if tensor is on CPU
     */
    bool is_cpu() const { return device_.type == DeviceType::CPU; }
    
    /**
     * @brief Check if tensor is on GPU
     */
    bool is_gpu() const { return !is_cpu(); }
    
    /**
     * @brief Get data type
     */
    DType dtype() const { return dtype_; }
    
    /**
     * @brief Check if tensor uses mixed precision
     */
    bool is_mixed_precision() const { return themis::llm::lora::is_mixed_precision(dtype_); }
    
    // ========== Shape and Data Access ==========
    
    const std::vector<size_t>& shape() const { return shape_; }
    size_t size() const;
    size_t ndim() const { return shape_.size(); }
    
    /**
     * @brief Get CPU data (downloads from GPU if needed)
     * Warning: This triggers CPU ↔ GPU transfer if tensor is on GPU
     */
    std::vector<float> cpu_data() const;
    
    /**
     * @brief Get GPU pointer (returns nullptr if on CPU)
     */
    void* gpu_ptr() const { return gpu_data_; }
    
    /**
     * @brief Upload data from CPU to current device
     */
    void upload(const float* data, size_t count);
    void upload(const std::vector<float>& data);
    
    /**
     * @brief Download data from current device to CPU
     */
    void download(float* data, size_t count) const;
    std::vector<float> download() const;
    
    // ========== Operations ==========
    
    /**
     * @brief Element-wise addition (dispatched to backend)
     */
    GPUTensor operator+(const GPUTensor& other) const;
    
    /**
     * @brief Element-wise subtraction
     */
    GPUTensor operator-(const GPUTensor& other) const;
    
    /**
     * @brief Scalar multiplication
     */
    GPUTensor operator*(float scalar) const;
    
    /**
     * @brief Element-wise multiplication
     */
    GPUTensor mul(const GPUTensor& other) const;
    
    /**
     * @brief Matrix multiplication (dispatched to backend)
     */
    GPUTensor matmul(const GPUTensor& other) const;
    
    /**
     * @brief Transpose (2D tensors only)
     */
    GPUTensor transpose() const;
    
    /**
     * @brief Fill with value
     */
    void fill(float value);
    
    /**
     * @brief Zero out tensor
     */
    void zero();
    
    /**
     * @brief Clone tensor (same device)
     */
    GPUTensor clone() const;
    
    // ========== Data Type Conversion ==========
    
    /**
     * @brief Convert tensor to FP32
     * @return New tensor in FP32
     */
    GPUTensor to_fp32() const;
    
    /**
     * @brief Convert tensor to FP16
     * @return New tensor in FP16
     */
    GPUTensor to_fp16() const;
    
    /**
     * @brief Convert tensor to BF16
     * @return New tensor in BF16
     */
    GPUTensor to_bf16() const;
    
    /**
     * @brief Convert tensor to specified dtype
     * @param target_dtype Target data type
     * @return New tensor in target dtype
     */
    GPUTensor to_dtype(DType target_dtype) const;
    
    // ========== Gradient Support ==========
    
    /**
     * @brief Gradient tensor (same device as data)
     */
    std::unique_ptr<GPUTensor> grad;
    bool requires_grad = false;
    
    /**
     * @brief Zero out gradient
     */
    void zero_grad();
    
    /**
     * @brief Allocate gradient tensor if not exists
     */
    void ensure_grad();
    
    // ========== Mixed Precision Support ==========
    
    /**
     * @brief Multiply tensor by scalar in-place (GPU-native)
     * Used for gradient unscaling in mixed precision training
     * @param scalar Scaling factor
     */
    void multiply_inplace(float scalar);
    
    /**
     * @brief Check if tensor contains NaN or Inf (GPU-native)
     * Used for overflow detection in mixed precision training
     * @return true if NaN or Inf detected
     */
    bool has_inf_or_nan() const;

    // ========== dtype-cast callback bridges (STUB #2/#3) ==========
    //
    // Allow injection of a real GPU dtype-cast kernel for CUDA (STUB #2) or
    // HIP/ROCm (STUB #3) builds, replacing the default CPU round-trip fallback.
    // The function receives the current element data as fp32, the source DType,
    // and the target DType; it returns the converted element data as fp32.
    // Passing nullptr reverts to the CPU round-trip fallback path.
    using DtypeCastFn = std::function<std::vector<float>(const std::vector<float>&, DType, DType)>;
    static void setCudaDtypeCastFn(DtypeCastFn fn);
    static void setHipDtypeCastFn(DtypeCastFn fn);

private:
    std::vector<size_t> shape_;
    Device device_;
    DType dtype_ = DType::FLOAT32;
    
    // CPU data (only used if device is CPU)
    std::vector<float> cpu_data_;
    
    // GPU data (only used if device is GPU)
    void* gpu_data_ = nullptr;
    VRAMAllocator* allocator_ = nullptr;
    
    // GPU memory manager (shared across all tensors)
    static GPUMemoryManager& get_memory_manager();
    
    // Backend operation dispatchers
    GPUTensor dispatch_add(const GPUTensor& other) const;
    GPUTensor dispatch_sub(const GPUTensor& other) const;
    GPUTensor dispatch_mul_scalar(float scalar) const;
    GPUTensor dispatch_mul_elementwise(const GPUTensor& other) const;
    GPUTensor dispatch_matmul(const GPUTensor& other) const;
    GPUTensor dispatch_transpose() const;
    
    // Helper to allocate GPU memory
    void allocate_gpu_memory();
    void free_gpu_memory();
};

// ========== Utility Functions ==========

namespace gpu_tensor_utils {
    /**
     * @brief Create tensor with random normal initialization
     */
    GPUTensor randn(const std::vector<size_t>& shape, 
                    float mean = 0.0f, 
                    float std = 1.0f,
                    const Device& device = Device::cpu(),
                    DType dtype = DType::FLOAT32);
    
    /**
     * @brief Xavier/Glorot initialization
     */
    GPUTensor xavier_uniform(const std::vector<size_t>& shape,
                            const Device& device = Device::cpu(),
                            DType dtype = DType::FLOAT32);
    
    /**
     * @brief Kaiming/He initialization
     */
    GPUTensor kaiming_uniform(const std::vector<size_t>& shape,
                             float a = 0.0f,
                             const Device& device = Device::cpu(),
                             DType dtype = DType::FLOAT32);
    
    /**
     * @brief Zero initialization
     */
    GPUTensor zeros(const std::vector<size_t>& shape,
                   const Device& device = Device::cpu(),
                   DType dtype = DType::FLOAT32);
    
    /**
     * @brief Ones initialization
     */
    GPUTensor ones(const std::vector<size_t>& shape,
                  const Device& device = Device::cpu(),
                  DType dtype = DType::FLOAT32);
    
    /**
     * @brief Convert legacy Tensor to GPUTensor
     */
    [[nodiscard]] GPUTensor from_legacy_tensor(const Tensor& tensor,
                                               const Device& device = Device::cpu(),
                                               DType dtype = DType::FLOAT32);
    
    /**
     * @brief Convert GPUTensor to legacy Tensor
     */
    [[nodiscard]] Tensor to_legacy_tensor(const GPUTensor& gpu_tensor);
}

} // namespace lora
} // namespace llm
} // namespace themis
