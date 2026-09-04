/**
 * @file gpu_tensor.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=11; TODO=1, Stub=5, Unimpl=0, Mock=1, Sim=4, Debt=0, C=0, H=3, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_framework/gpu_tensor.h"
#include "llm/lora_framework/lora_layers.h"
#include "llm/lora_framework/cuda_kernels.h"
#include "llm/lora_framework/hip_kernels.h"
#include <cstring>
#include <cmath>
#include <random>
#include <stdexcept>
#include <numeric>

namespace themis {
namespace llm {
namespace lora {

// Static memory manager instance
GPUMemoryManager& GPUTensor::get_memory_manager() {
    static GPUMemoryManager manager;
    return manager;
}

// ─── dtype-cast callback bridges (STUB #2 / STUB #3) ─────────────────────────
// These allow injection of real CUDA/HIP gpu-side dtype-cast kernels, replacing
// the default CPU round-trip fallback (download → convert → upload).

static std::mutex& cudaDtypeCastMutex() { static std::mutex m; return m; }
static GPUTensor::DtypeCastFn& cudaDtypeCastFnStorage() {
    static GPUTensor::DtypeCastFn fn;
    return fn;
}
void GPUTensor::setCudaDtypeCastFn(DtypeCastFn fn) {
    std::lock_guard<std::mutex> lk(cudaDtypeCastMutex());
    cudaDtypeCastFnStorage() = std::move(fn);
}

static std::mutex& hipDtypeCastMutex() { static std::mutex m; return m; }
static GPUTensor::DtypeCastFn& hipDtypeCastFnStorage() {
    static GPUTensor::DtypeCastFn fn;
    return fn;
}
void GPUTensor::setHipDtypeCastFn(DtypeCastFn fn) {
    std::lock_guard<std::mutex> lk(hipDtypeCastMutex());
    hipDtypeCastFnStorage() = std::move(fn);
}

// ============================================================================
// Constructors and Destructor
// ============================================================================

GPUTensor::GPUTensor(const std::vector<size_t>& shape, const Device& device, DType dtype)
    : shape_(shape), device_(device), dtype_(dtype) {
    
    if (is_cpu()) {
        // Allocate CPU memory (always FP32 on CPU for now)
        size_t total_size = std::accumulate(shape_.begin(), shape_.end(), 
                                           size_t(1), std::multiplies<size_t>());
        cpu_data_.resize(total_size, 0.0f);
    } else {
        // Allocate GPU memory with appropriate dtype
        allocate_gpu_memory();
    }
}

GPUTensor::GPUTensor(const std::vector<size_t>& shape, float value, const Device& device, DType dtype)
    : GPUTensor(shape, device, dtype) {
    fill(value);
}

GPUTensor::~GPUTensor() {
    if (is_gpu()) {
        free_gpu_memory();
    }
}

GPUTensor::GPUTensor(GPUTensor&& other) noexcept
    : shape_(std::move(other.shape_))
    , device_(other.device_)
    , dtype_(other.dtype_)
    , cpu_data_(std::move(other.cpu_data_))
    , gpu_data_(other.gpu_data_)
    , allocator_(other.allocator_)
    , grad(std::move(other.grad))
    , requires_grad(other.requires_grad) {
    
    other.gpu_data_ = nullptr;
    other.allocator_ = nullptr;
}

GPUTensor& GPUTensor::operator=(GPUTensor&& other) noexcept {
    if (this != &other) {
        if (is_gpu()) {
            free_gpu_memory();
        }
        
        shape_ = std::move(other.shape_);
        device_ = other.device_;
        dtype_ = other.dtype_;
        cpu_data_ = std::move(other.cpu_data_);
        gpu_data_ = other.gpu_data_;
        allocator_ = other.allocator_;
        grad = std::move(other.grad);
        requires_grad = other.requires_grad;
        
        other.gpu_data_ = nullptr;
        other.allocator_ = nullptr;
    }
    return *this;
}

// ============================================================================
// Device Management
// ============================================================================

GPUTensor GPUTensor::to(const Device& target_device) const {
    if (device_ == target_device) {
        return clone();
    }
    
    // Create new tensor on target device
    GPUTensor result(shape_, target_device);
    
    // Transfer data
    if (is_cpu() && !result.is_cpu()) {
        // CPU → GPU
        result.upload(cpu_data_);
    } else if (!is_cpu() && result.is_cpu()) {
        // GPU → CPU
        result.cpu_data_ = download();
    } else if (!is_cpu() && !result.is_cpu()) {
        // GPU → GPU (via CPU staging)
        auto temp_data = download();
        result.upload(temp_data);
    }
    
    return result;
}

void GPUTensor::to_inplace(const Device& target_device) {
    if (device_ == target_device) {
        return;
    }
    
    // Transfer data and change device
    auto temp = to(target_device);
    *this = std::move(temp);
}

// ============================================================================
// Data Access
// ============================================================================

size_t GPUTensor::size() const {
    return std::accumulate(shape_.begin(), shape_.end(), 
                          size_t(1), std::multiplies<size_t>());
}

std::vector<float> GPUTensor::cpu_data() const {
    if (is_cpu()) {
        return cpu_data_;
    } else {
        return download();
    }
}

void GPUTensor::upload(const float* data, size_t count) {
    if (count != size()) {
        throw std::invalid_argument("Upload size mismatch");
    }
    
    if (is_cpu()) {
        std::memcpy(cpu_data_.data(), data, count * sizeof(float));
    } else {
        if (!allocator_->upload(gpu_data_, data, count * sizeof(float))) {
            throw std::runtime_error("Failed to upload data to GPU");
        }
    }
}

void GPUTensor::upload(const std::vector<float>& data) {
    upload(data.data(),static_cast<int>(data.size()));
}

void GPUTensor::download(float* data, size_t count) const {
    if (count != size()) {
        throw std::invalid_argument("Download size mismatch");
    }
    
    if (is_cpu()) {
        std::memcpy(data, cpu_data_.data(), count * sizeof(float));
    } else {
        if (!allocator_->download(data, gpu_data_, count * sizeof(float))) {
            throw std::runtime_error("Failed to download data from GPU");
        }
    }
}

std::vector<float> GPUTensor::download() const {
    std::vector<float> result(size());
    download(result.data(),static_cast<int>(result.size()));
    return result;
}

// ============================================================================
// Operations (CPU fallback implementations)
// ============================================================================

GPUTensor GPUTensor::operator+(const GPUTensor& other) const {
    if (shape_ != other.shape_) {
        throw std::invalid_argument("Shape mismatch in addition");
    }
    
    if (device_ != other.device_) {
        throw std::invalid_argument("Device mismatch in addition");
    }
    
    return dispatch_add(other);
}

GPUTensor GPUTensor::operator-(const GPUTensor& other) const {
    if (shape_ != other.shape_) {
        throw std::invalid_argument("Shape mismatch in subtraction");
    }
    
    if (device_ != other.device_) {
        throw std::invalid_argument("Device mismatch in subtraction");
    }
    
    return dispatch_sub(other);
}

GPUTensor GPUTensor::operator*([[maybe_unused]] float scalar) const {
    return dispatch_mul_scalar(scalar);
}

GPUTensor GPUTensor::mul(const GPUTensor& other) const {
    if (shape_ != other.shape_) {
        throw std::invalid_argument("Shape mismatch in multiplication");
    }
    
    if (device_ != other.device_) {
        throw std::invalid_argument("Device mismatch in multiplication");
    }
    
    return dispatch_mul_elementwise(other);
}

GPUTensor GPUTensor::matmul(const GPUTensor& other) const {
    if (static_cast<int>(shape_.size()) != 2 || other.shape_.size() != 2) {
        throw std::invalid_argument("matmul requires 2D tensors");
    }
    
    if (shape_[1] != other.shape_[0]) {
        throw std::invalid_argument("Incompatible shapes for matmul");
    }
    
    if (device_ != other.device_) {
        throw std::invalid_argument("Device mismatch in matmul");
    }
    
    return dispatch_matmul(other);
}

GPUTensor GPUTensor::transpose() const {
    if (static_cast<int>(shape_.size()) != 2) {
        throw std::invalid_argument("transpose requires 2D tensor");
    }
    
    return dispatch_transpose();
}

void GPUTensor::fill([[maybe_unused]] float value) {
    if (is_cpu()) {
        std::fill(cpu_data_.begin(), cpu_data_.end(), value);
    } else {
        // For GPU, create temp CPU buffer, fill, and upload
        std::vector<float> temp(size(), value);
        upload(temp);
    }
}

void GPUTensor::zero() {
    fill(0.0f);
}

GPUTensor GPUTensor::clone() const {
    GPUTensor result(shape_, device_, dtype_);
    
    if (is_cpu()) {
        result.cpu_data_ = cpu_data_;
    } else {
        auto temp_data = download();
        result.upload(temp_data);
    }
    
    return result;
}

// ============================================================================
// Data Type Conversion
// ============================================================================

GPUTensor GPUTensor::to_fp32() const {
    return to_dtype(DType::FLOAT32);
}

GPUTensor GPUTensor::to_fp16() const {
    return to_dtype(DType::FLOAT16);
}

GPUTensor GPUTensor::to_bf16() const {
    return to_dtype(DType::BFLOAT16);
}

GPUTensor GPUTensor::to_dtype(DType target_dtype) const {
    if (dtype_ == target_dtype) {
        return clone();
    }
    
    // For CPU tensors, perform conversion on CPU
    if (is_cpu()) {
        GPUTensor result(shape_, device_, target_dtype);
        
        // Convert FP32 data to target dtype
        if (target_dtype == DType::FLOAT32) {
            // Source is FP16 or BF16, convert to FP32
            result.cpu_data_ = cpu_data_;  // Already stored as FP32
        } else if (target_dtype == DType::FLOAT16) {
            // Convert to FP16 (simulate precision loss)
            result.cpu_data_.resize(cpu_data_.size());
            for (size_t i = 0; i < cpu_data_.size(); ++i) {
                uint16_t fp16_bits = fp32_to_fp16_bits(cpu_data_[i]);
                result.cpu_data_[i] = fp16_bits_to_fp32(fp16_bits);
            }
        } else if (target_dtype == DType::BFLOAT16) {
            // Convert to BF16 (simulate precision loss)
            result.cpu_data_.resize(cpu_data_.size());
            for (size_t i = 0; i < cpu_data_.size(); ++i) {
                uint16_t bf16_bits = fp32_to_bf16_bits(cpu_data_[i]);
                result.cpu_data_[i] = bf16_bits_to_fp32(bf16_bits);
            }
        }
        
        return result;
    }
    
    // For GPU tensors, use GPU conversion kernels
    GPUTensor result(shape_, device_, target_dtype);

#ifdef THEMIS_ENABLE_CUDA
    if (device_.type == DeviceType::CUDA) {
        // STUB/SIMULATION NOTE:
        // Purpose: CPU round-trip fallback for dtype conversion on CUDA tensors when
        //          dedicated CUDA dtype-cast kernels are not yet implemented.
        // Activation: Active when no CudaDtypeCastFn is injected via
        //             GPUTensor::setCudaDtypeCastFn().  A real CUDA kernel can be
        //             injected at startup to replace this path.
        // Production Delta: download() + upload() incur PCIe round-trip overhead;
        //                   a native CUDA kernel (e.g. thrust::transform) is 10-50×
        //                   faster and avoids peak-VRAM doubling.
        // Removal Plan: Implement a CUDA __global__ cast kernel and register it via
        //               setCudaDtypeCastFn() at startup (Target: v1.7.0,
        //               FUTURE_ENHANCEMENTS.md §"CUDA dtype kernels").
        DtypeCastFn fn;
        {
            std::lock_guard<std::mutex> lk(cudaDtypeCastMutex());
            fn = cudaDtypeCastFnStorage();
        }
        if (fn) {
            try {
                auto converted_data = fn(download(), dtype_, target_dtype);
                result.upload(converted_data);
                return result;
            } catch (...) {
                // fall through to CPU round-trip
            }
        }
        auto cpu_data = download();
        GPUTensor temp(shape_, Device::cpu(), dtype_);
        temp.upload(cpu_data);
        auto converted = temp.to_dtype(target_dtype);
        result.upload(converted.download());
        return result;
    }
#endif

#ifdef THEMIS_ENABLE_HIP
    if (device_.type == DeviceType::HIP) {
        // STUB/SIMULATION NOTE:
        // Purpose: CPU round-trip fallback for dtype conversion on HIP/ROCm tensors.
        // Activation: Active when no HipDtypeCastFn is injected via
        //             GPUTensor::setHipDtypeCastFn().  A real HIP kernel can be
        //             injected at startup to replace this path.
        // Production Delta: Same PCIe round-trip overhead as the CUDA path above.
        // Removal Plan: Implement a HIP __global__ cast kernel and register it via
        //               setHipDtypeCastFn() at startup (Target: v1.7.0).
        DtypeCastFn fn;
        {
            std::lock_guard<std::mutex> lk(hipDtypeCastMutex());
            fn = hipDtypeCastFnStorage();
        }
        if (fn) {
            try {
                auto converted_data = fn(download(), dtype_, target_dtype);
                result.upload(converted_data);
                return result;
            } catch (...) {
                // fall through to CPU round-trip
            }
        }
        auto cpu_data = download();
        GPUTensor temp(shape_, Device::cpu(), dtype_);
        temp.upload(cpu_data);
        auto converted = temp.to_dtype(target_dtype);
        result.upload(converted.download());
        return result;
    }
#endif
    
    // Fallback: Download, convert on CPU, upload
    auto cpu_data = download();
    GPUTensor temp(shape_, Device::cpu(), dtype_);
    temp.upload(cpu_data);
    auto converted = temp.to_dtype(target_dtype);
    result.upload(converted.download());
    
    return result;
}

// ============================================================================
// Gradient Support
// ============================================================================

void GPUTensor::zero_grad() {
    if (grad) {
        grad->zero();
    }
}

void GPUTensor::ensure_grad() {
    if (!grad) {
        // Gradients are always computed in FP32 for numerical stability
        grad = std::make_unique<GPUTensor>(shape_, device_, DType::FLOAT32);
        grad->zero();
    }
}

// ============================================================================
// Memory Management Helpers
// ============================================================================

void GPUTensor::allocate_gpu_memory() {
    allocator_ = get_memory_manager().get_allocator(device_);
    if (!allocator_) {
        throw std::runtime_error("Failed to get allocator for device");
    }
    
    size_t bytes = size() * dtype_size(dtype_);
    gpu_data_ = allocator_->allocate(bytes);
    
    if (!gpu_data_) {
        throw std::runtime_error("Failed to allocate GPU memory");
    }
}

void GPUTensor::free_gpu_memory() {
    if (allocator_ && gpu_data_) {
        allocator_->deallocate(gpu_data_);
        gpu_data_ = nullptr;
    }
}

// ============================================================================
// Backend Dispatchers (CPU fallback for now)
// ============================================================================

GPUTensor GPUTensor::dispatch_add(const GPUTensor& other) const {
    GPUTensor result(shape_, device_, dtype_);
    
    if (is_cpu()) {
        // CPU path
        for (size_t i = 0; i < size(); i++) {
            result.cpu_data_[i] = cpu_data_[i] + other.cpu_data_[i];
        }
    } else {
        // GPU path: dispatch to appropriate backend
#ifdef THEMIS_ENABLE_CUDA
        if (device_.type == DeviceType::CUDA) {
            cuda::launch_add_kernel(
                static_cast<const float*>(gpu_data_),
                static_cast<const float*>(other.gpu_data_),
                static_cast<float*>(result.gpu_data_),
                size()
            );
            return result;
        }
#endif

#ifdef THEMIS_ENABLE_HIP
        if (device_.type == DeviceType::HIP) {
            hip::launch_add_kernel(
                static_cast<const float*>(gpu_data_),
                static_cast<const float*>(other.gpu_data_),
                static_cast<float*>(result.gpu_data_),
                size()
            );
            return result;
        }
#endif
        
        // Fallback for Vulkan/DirectX (not yet implemented)
        auto a_data = download();
        auto b_data = other.download();
        std::vector<float> c_data(size());
        
        for (size_t i = 0; i < size(); i++) {
            c_data[i] = a_data[i] + b_data[i];
        }
        
        result.upload(c_data);
    }
    
    return result;
}

GPUTensor GPUTensor::dispatch_sub(const GPUTensor& other) const {
    GPUTensor result(shape_, device_, dtype_);
    
    if (is_cpu()) {
        // CPU path
        for (size_t i = 0; i < size(); i++) {
            result.cpu_data_[i] = cpu_data_[i] - other.cpu_data_[i];
        }
    } else {
        // GPU path: For CUDA/HIP, use element-wise operations
        // Subtract: a - b = a + (-1 * b)
#ifdef THEMIS_ENABLE_CUDA
        if (device_.type == DeviceType::CUDA) {
            // Use multiply by -1, then add
            auto neg_b = other * (-1.0f);
            return *this + neg_b;
        }
#endif

#ifdef THEMIS_ENABLE_HIP
        if (device_.type == DeviceType::HIP) {
            auto neg_b = other * (-1.0f);
            return *this + neg_b;
        }
#endif
        
        // Fallback for other backends
        auto a_data = download();
        auto b_data = other.download();
        std::vector<float> c_data(size());
        
        for (size_t i = 0; i < size(); i++) {
            c_data[i] = a_data[i] - b_data[i];
        }
        
        result.upload(c_data);
    }
    
    return result;
}

GPUTensor GPUTensor::dispatch_mul_scalar([[maybe_unused]] float scalar) const {
    GPUTensor result(shape_, device_, dtype_);
    
    if (is_cpu()) {
        // CPU path
        for (size_t i = 0; i < size(); i++) {
            result.cpu_data_[i] = cpu_data_[i] * scalar;
        }
    } else {
        // GPU path: dispatch to appropriate backend
#ifdef THEMIS_ENABLE_CUDA
        if (device_.type == DeviceType::CUDA) {
            cuda::launch_scalar_multiply_kernel(
                static_cast<const float*>(gpu_data_),
                static_cast<float*>(result.gpu_data_),
                scalar,
                size()
            );
            return result;
        }
#endif

#ifdef THEMIS_ENABLE_HIP
        if (device_.type == DeviceType::HIP) {
            hip::launch_scalar_multiply_kernel(
                static_cast<const float*>(gpu_data_),
                static_cast<float*>(result.gpu_data_),
                scalar,
                size()
            );
            return result;
        }
#endif
        
        // Fallback for Vulkan/DirectX
        auto a_data = download();
        std::vector<float> c_data(size());
        
        for (size_t i = 0; i < size(); i++) {
            c_data[i] = a_data[i] * scalar;
        }
        
        result.upload(c_data);
    }
    
    return result;
}

GPUTensor GPUTensor::dispatch_mul_elementwise(const GPUTensor& other) const {
    GPUTensor result(shape_, device_, dtype_);
    
    if (is_cpu()) {
        // CPU path
        for (size_t i = 0; i < size(); i++) {
            result.cpu_data_[i] = cpu_data_[i] * other.cpu_data_[i];
        }
    } else {
        // GPU path: dispatch to appropriate backend
#ifdef THEMIS_ENABLE_CUDA
        if (device_.type == DeviceType::CUDA) {
            cuda::launch_multiply_kernel(
                static_cast<const float*>(gpu_data_),
                static_cast<const float*>(other.gpu_data_),
                static_cast<float*>(result.gpu_data_),
                size()
            );
            return result;
        }
#endif

#ifdef THEMIS_ENABLE_HIP
        if (device_.type == DeviceType::HIP) {
            hip::launch_multiply_kernel(
                static_cast<const float*>(gpu_data_),
                static_cast<const float*>(other.gpu_data_),
                static_cast<float*>(result.gpu_data_),
                size()
            );
            return result;
        }
#endif
        
        // Fallback for Vulkan/DirectX
        auto a_data = download();
        auto b_data = other.download();
        std::vector<float> c_data(size());
        
        for (size_t i = 0; i < size(); i++) {
            c_data[i] = a_data[i] * b_data[i];
        }
        
        result.upload(c_data);
    }
    
    return result;
}

GPUTensor GPUTensor::dispatch_matmul(const GPUTensor& other) const {
    size_t M = shape_[0];
    size_t K = shape_[1];
    size_t N = other.shape_[1];
    
    GPUTensor result({M, N}, device_);
    
    if (is_cpu()) {
        // CPU matrix multiplication
        auto a_data = cpu_data_.data();
        auto b_data = other.cpu_data_.data();
        auto c_data = result.cpu_data_.data();
        
        for (size_t i = 0; i < M; i++) {
            for (size_t j = 0; j < N; j++) {
                float sum = 0.0f;
                for (size_t k = 0; k < K; k++) {
                    sum += a_data[i * K + k] * b_data[k * N + j];
                }
                c_data[i * N + j] = sum;
            }
        }
    } else {
        // GPU path: dispatch to appropriate backend
#ifdef THEMIS_ENABLE_CUDA
        if (device_.type == DeviceType::CUDA) {
            // Use cuBLAS for optimal performance
            auto& manager = get_memory_manager();
            static cuda::CublasHandle cublas_handle;
            
            if (cublas_handle.is_valid()) {
                cuda::cublas_matmul(
                    cublas_handle.get(),
                    static_cast<const float*>(gpu_data_),
                    static_cast<const float*>(other.gpu_data_),
                    static_cast<float*>(result.gpu_data_),
                    M, K, N,
                    1.0f,  // alpha
                    0.0f   // beta
                );
            } else {
                // Fallback to custom kernel if cuBLAS not available
                cuda::launch_matmul_kernel(
                    static_cast<const float*>(gpu_data_),
                    static_cast<const float*>(other.gpu_data_),
                    static_cast<float*>(result.gpu_data_),
                    M, K, N,
                    1.0f
                );
            }
            return result;
        }
#endif

#ifdef THEMIS_ENABLE_HIP
        if (device_.type == DeviceType::HIP) {
            // Use rocBLAS for optimal performance
            static hip::RocblasHandle rocblas_handle;
            
            if (rocblas_handle.is_valid()) {
                hip::rocblas_matmul(
                    rocblas_handle.get(),
                    static_cast<const float*>(gpu_data_),
                    static_cast<const float*>(other.gpu_data_),
                    static_cast<float*>(result.gpu_data_),
                    M, K, N,
                    1.0f,
                    0.0f
                );
            } else {
                // Fallback to custom kernel
                hip::launch_matmul_kernel(
                    static_cast<const float*>(gpu_data_),
                    static_cast<const float*>(other.gpu_data_),
                    static_cast<float*>(result.gpu_data_),
                    M, K, N,
                    1.0f
                );
            }
            return result;
        }
#endif
        
        // Fallback for Vulkan/DirectX
        auto a_data = download();
        auto b_data = other.download();
        std::vector<float> c_data(M * N, 0.0f);
        
        for (size_t i = 0; i < M; i++) {
            for (size_t j = 0; j < N; j++) {
                float sum = 0.0f;
                for (size_t k = 0; k < K; k++) {
                    sum += a_data[i * K + k] * b_data[k * N + j];
                }
                c_data[i * N + j] = sum;
            }
        }
        
        result.upload(c_data);
    }
    
    return result;
}

GPUTensor GPUTensor::dispatch_transpose() const {
    size_t rows = shape_[0];
    size_t cols = shape_[1];
    
    GPUTensor result({cols, rows}, device_);
    
    if (is_cpu()) {
        // CPU transpose
        for (size_t i = 0; i < rows; i++) {
            for (size_t j = 0; j < cols; j++) {
                result.cpu_data_[j * rows + i] = cpu_data_[i * cols + j];
            }
        }
    } else {
        // GPU path: dispatch to appropriate backend
#ifdef THEMIS_ENABLE_CUDA
        if (device_.type == DeviceType::CUDA) {
            cuda::launch_transpose_kernel(
                static_cast<const float*>(gpu_data_),
                static_cast<float*>(result.gpu_data_),
                rows,
                cols
            );
            return result;
        }
#endif

#ifdef THEMIS_ENABLE_HIP
        if (device_.type == DeviceType::HIP) {
            hip::launch_transpose_kernel(
                static_cast<const float*>(gpu_data_),
                static_cast<float*>(result.gpu_data_),
                rows,
                cols
            );
            return result;
        }
#endif
        
        // Fallback for Vulkan/DirectX
        auto a_data = download();
        std::vector<float> c_data(rows * cols);
        
        for (size_t i = 0; i < rows; i++) {
            for (size_t j = 0; j < cols; j++) {
                c_data[j * rows + i] = a_data[i * cols + j];
            }
        }
        
        result.upload(c_data);
    }
    
    return result;
}

// ============================================================================
// Mixed Precision Support
// ============================================================================

void GPUTensor::multiply_inplace([[maybe_unused]] float scalar) {
    if (is_cpu()) {
        // CPU implementation
        for (size_t i = 0; i < cpu_data_.size(); ++i) {
            cpu_data_[i] *= scalar;
        }
    } else {
        // GPU path: dispatch to appropriate backend
#ifdef THEMIS_ENABLE_CUDA
        if (device_.type == DeviceType::CUDA) {
            cuda::launch_scalar_multiply_inplace_kernel(
                static_cast<float*>(gpu_data_),
                scalar,
                size()
            );
            return;
        }
#endif

#ifdef THEMIS_ENABLE_HIP
        if (device_.type == DeviceType::HIP) {
            hip::launch_scalar_multiply_inplace_kernel(
                static_cast<float*>(gpu_data_),
                scalar,
                size()
            );
            return;
        }
#endif
        
        // Fallback for Vulkan/DirectX: download, multiply, upload
        auto data = download();
        for (size_t i = 0; i < data.size(); ++i) {
            data[i] *= scalar;
        }
        upload(data);
    }
}

bool GPUTensor::has_inf_or_nan() const {
    if (is_cpu()) {
        // CPU implementation
        for (float val : cpu_data_) {
            if (std::isnan(val) || std::isinf(val)) {
                return true;
            }
        }
        return false;
    } else {
        // GPU path: dispatch to appropriate backend
#ifdef THEMIS_ENABLE_CUDA
        if (device_.type == DeviceType::CUDA) {
            bool has_overflow = false;
            cuda::launch_check_inf_nan_kernel(
                static_cast<const float*>(gpu_data_),
                size(),
                &has_overflow
            );
            return has_overflow;
        }
#endif

#ifdef THEMIS_ENABLE_HIP
        if (device_.type == DeviceType::HIP) {
            bool has_overflow = false;
            hip::launch_check_inf_nan_kernel(
                static_cast<const float*>(gpu_data_),
                size(),
                &has_overflow
            );
            return has_overflow;
        }
#endif
        
        // Fallback for Vulkan/DirectX: download and check
        auto data = download();
        for (float val : data) {
            if (std::isnan(val) || std::isinf(val)) {
                return true;
            }
        }
        return false;
    }
}

// ============================================================================
// Utility Functions
// ============================================================================

namespace gpu_tensor_utils {

GPUTensor randn(const std::vector<size_t>& shape, float mean, float std, const Device& device, DType dtype) {
    size_t total_size = std::accumulate(shape.begin(), shape.end(), 
                                       size_t(1), std::multiplies<size_t>());
    
    std::random_device rd = {};
    std::mt19937 gen(rd());
    std::normal_distribution<float> dist(mean, std);
    
    std::vector<float> data(total_size);
    for (auto& val : data) {
        val = dist(gen);
    }
    
    GPUTensor result(shape, device, dtype);
    result.upload(data);
    return result;
}

GPUTensor xavier_uniform(const std::vector<size_t>& shape, const Device& device, DType dtype) {
    if (static_cast<int>(shape.size()) != 2) {
        throw std::invalid_argument("Xavier init requires 2D tensor");
    }
    
    float fan_in = static_cast<float>(shape[0]);
    float fan_out = static_cast<float>(shape[1]);
    float limit = std::sqrt(6.0f / (fan_in + fan_out));
    
    size_t total_size = shape[0] * shape[1];
    
    std::random_device rd = {};
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-limit, limit);
    
    std::vector<float> data(total_size);
    for (auto& val : data) {
        val = dist(gen);
    }
    
    GPUTensor result(shape, device, dtype);
    result.upload(data);
    return result;
}

GPUTensor kaiming_uniform(const std::vector<size_t>& shape, float a, const Device& device, DType dtype) {
    if (static_cast<int>(shape.size()) != 2) {
        throw std::invalid_argument("Kaiming init requires 2D tensor");
    }
    
    float fan_in = static_cast<float>(shape[0]);
    float gain = std::sqrt(2.0f / (1.0f + a * a));
    float std = gain / std::sqrt(fan_in);
    float limit = std::sqrt(3.0f) * std;
    
    size_t total_size = shape[0] * shape[1];
    
    std::random_device rd = {};
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-limit, limit);
    
    std::vector<float> data(total_size);
    for (auto& val : data) {
        val = dist(gen);
    }
    
    GPUTensor result(shape, device, dtype);
    result.upload(data);
    return result;
}

GPUTensor zeros(const std::vector<size_t>& shape, const Device& device, DType dtype) {
    return GPUTensor(shape, 0.0f, device, dtype);
}

GPUTensor ones(const std::vector<size_t>& shape, const Device& device, DType dtype) {
    return GPUTensor(shape, 1.0f, device, dtype);
}

GPUTensor from_legacy_tensor(const Tensor& tensor, const Device& device, DType dtype) {
    GPUTensor result(tensor.shape(), device, dtype);
    result.upload(tensor.data());
    return result;
}

Tensor to_legacy_tensor(const GPUTensor& gpu_tensor) {
    Tensor result(gpu_tensor.shape());
    auto data = gpu_tensor.cpu_data();
    if (static_cast<int>(data.size()) == result.size()) {
        std::copy(data.begin(), data.end(), result.data().begin());
    }
    return result;
}

} // namespace gpu_tensor_utils

} // namespace lora
} // namespace llm
} // namespace themis

