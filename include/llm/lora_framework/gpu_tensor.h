/**
 * @file gpu_tensor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class GPUTensor {
public:
    GPUTensor() = default;
    
    GPUTensor(const std::vector<size_t>& shape, 
              const Device& device = Device::cpu(),
              DType dtype = DType::FLOAT32);
    
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
    
    Device device() const { return device_; }
    
    /**
     * @brief To.
     * @param[in] target_device Input parameter.
     * @return Return value.
     */
    GPUTensor to(const Device& target_device) const;
    
    /**
     * @brief To inplace.
     * @param[in] target_device Input parameter.
     */
    void to_inplace(const Device& target_device);
    
    bool is_cpu() const { return device_.type == DeviceType::CPU; }
    
    bool is_gpu() const { return !is_cpu(); }
    
    DType dtype() const { return dtype_; }
    
    bool is_mixed_precision() const { return themis::llm::lora::is_mixed_precision(dtype_); }
    
    // ========== Shape and Data Access ==========
    
    const std::vector<size_t>& shape() const { return shape_; }
    /**
     * @brief Size.
     * @return Return value.
     */
    size_t size() const;
    size_t ndim() const { return shape_.size(); }
    
    /**
     * @brief Cpu data.
     * @return Return value.
     */
    std::vector<float> cpu_data() const;
    
    void* gpu_ptr() const { return gpu_data_; }
    
    /**
     * @brief Upload.
     * @param[in] data Input parameter.
     * @param[in] count Input parameter.
     */
    void upload(const float* data, size_t count);
    /**
     * @brief Upload.
     * @param[in] data Input parameter.
     */
    void upload(const std::vector<float>& data);
    
    /**
     * @brief Download.
     * @param[in,out] data Input/output parameter.
     * @param[in] count Input parameter.
     */
    void download(float* data, size_t count) const;
    /**
     * @brief Download.
     * @return Return value.
     */
    std::vector<float> download() const;
    
    // ========== Operations ==========
    
    GPUTensor operator+(const GPUTensor& other) const;
    
    GPUTensor operator-(const GPUTensor& other) const;
    
    GPUTensor operator*(float scalar) const;
    
    /**
     * @brief Mul.
     * @param[in] other Input parameter.
     * @return Return value.
     */
    GPUTensor mul(const GPUTensor& other) const;
    
    /**
     * @brief Matmul.
     * @param[in] other Input parameter.
     * @return Return value.
     */
    GPUTensor matmul(const GPUTensor& other) const;
    
    /**
     * @brief Transpose.
     * @return Return value.
     */
    GPUTensor transpose() const;
    
    /**
     * @brief Fill.
     * @param[in] value Input parameter.
     */
    void fill(float value);
    
    /**
     * @brief Zero.
     */
    void zero();
    
    /**
     * @brief Clone.
     * @return Return value.
     */
    GPUTensor clone() const;
    
    /**
     * @brief ========== Data Type Conversion ==========
     * @return Return value.
     */
    
    GPUTensor to_fp32() const;
    
    /**
     * @brief To fp16.
     * @return Return value.
     */
    GPUTensor to_fp16() const;
    
    /**
     * @brief To bf16.
     * @return Return value.
     */
    GPUTensor to_bf16() const;
    
    /**
     * @brief To dtype.
     * @param[in] target_dtype Input parameter.
     * @return Return value.
     */
    GPUTensor to_dtype(DType target_dtype) const;
    
    // ========== Gradient Support ==========
    
    std::unique_ptr<GPUTensor> grad;
    bool requires_grad = false;
    
    /**
     * @brief Zero grad.
     */
    void zero_grad();
    
    /**
     * @brief Ensure grad.
     */
    void ensure_grad();
    
    /**
     * @brief ========== Mixed Precision Support ==========
     * @param[in] scalar Input parameter.
     */
    
    void multiply_inplace(float scalar);
    
    /**
     * @brief Has inf or nan.
     * @return True when the operation succeeds.
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
    /**
     * @brief Set Cuda Dtype Cast Fn.
     * @param[in] fn Input parameter.
     */
    static void setCudaDtypeCastFn(DtypeCastFn fn);
    /**
     * @brief Set Hip Dtype Cast Fn.
     * @param[in] fn Input parameter.
     */
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
    
    /**
     * @brief GPU memory manager (shared across all tensors)
     * @return Return value.
     */
    static GPUMemoryManager& get_memory_manager();
    
    // Backend operation dispatchers
    /**
     * @brief Dispatch add.
     * @param[in] other Input parameter.
     * @return Return value.
     */
    GPUTensor dispatch_add(const GPUTensor& other) const;
    /**
     * @brief Dispatch sub.
     * @param[in] other Input parameter.
     * @return Return value.
     */
    GPUTensor dispatch_sub(const GPUTensor& other) const;
    /**
     * @brief Dispatch mul scalar.
     * @param[in] scalar Input parameter.
     * @return Return value.
     */
    GPUTensor dispatch_mul_scalar(float scalar) const;
    /**
     * @brief Dispatch mul elementwise.
     * @param[in] other Input parameter.
     * @return Return value.
     */
    GPUTensor dispatch_mul_elementwise(const GPUTensor& other) const;
    /**
     * @brief Dispatch matmul.
     * @param[in] other Input parameter.
     * @return Return value.
     */
    GPUTensor dispatch_matmul(const GPUTensor& other) const;
    /**
     * @brief Dispatch transpose.
     * @return Return value.
     */
    GPUTensor dispatch_transpose() const;
    
    /**
     * @brief Helper to allocate GPU memory
     */
    void allocate_gpu_memory();
    /**
     * @brief Free gpu memory.
     */
    void free_gpu_memory();
};

// ========== Utility Functions ==========

namespace gpu_tensor_utils {
    GPUTensor randn(const std::vector<size_t>& shape, 
                    float mean = 0.0f, 
                    float std = 1.0f,
                    const Device& device = Device::cpu(),
                    DType dtype = DType::FLOAT32);
    
    GPUTensor xavier_uniform(const std::vector<size_t>& shape,
                            const Device& device = Device::cpu(),
                            DType dtype = DType::FLOAT32);
    
    GPUTensor kaiming_uniform(const std::vector<size_t>& shape,
                             float a = 0.0f,
                             const Device& device = Device::cpu(),
                             DType dtype = DType::FLOAT32);
    
    GPUTensor zeros(const std::vector<size_t>& shape,
                   const Device& device = Device::cpu(),
                   DType dtype = DType::FLOAT32);
    
    GPUTensor ones(const std::vector<size_t>& shape,
                  const Device& device = Device::cpu(),
                  DType dtype = DType::FLOAT32);
    
    [[nodiscard]] GPUTensor from_legacy_tensor(const Tensor& tensor,
                                               const Device& device = Device::cpu(),
                                               DType dtype = DType::FLOAT32);
    
    [[nodiscard]] Tensor to_legacy_tensor(const GPUTensor& gpu_tensor);
}

} // namespace lora
} // namespace llm
} // namespace themis
