/**
 * @file gpu_lora_layers.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=34, H=36, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_framework/gpu_lora_layers.h"
#include "llm/lora_framework/flash_lora.h"
#include "performance/alignment_helpers.h"
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <cmath>
#include <cassert>

// Include fused kernel headers
#ifdef THEMIS_ENABLE_CUDA
#include "llm/lora_framework/cuda_fused_kernels.h"
#endif

#ifdef THEMIS_ENABLE_HIP
#include "llm/lora_framework/hip_fused_kernels.h"
#endif

#ifdef THEMIS_ENABLE_VULKAN
#include "llm/lora_framework/vulkan_kernels.h"
#endif

namespace themis {
namespace llm {
namespace lora {

// ============================================================================
// GPULoRALayer Implementation
// ============================================================================

GPULoRALayer::GPULoRALayer(size_t in_dim, size_t out_dim, size_t rank, 
                           float scaling, const Device& device, bool use_fused_kernels,
                           bool use_flash_lora)
    : in_dim_(in_dim)
    , out_dim_(out_dim)
    , rank_(rank)
    , scaling_(scaling)
    , device_(device)
    , use_fused_kernels_(use_fused_kernels)
    , use_flash_lora_(use_flash_lora) {
    if (in_dim_ == 0 || out_dim_ == 0 || rank_ == 0) {
        throw std::invalid_argument("GPULoRALayer requires non-zero in_dim/out_dim/rank");
    }
    if (!std::isfinite(scaling_)) {
        throw std::invalid_argument("GPULoRALayer scaling must be finite");
    }
    
    // Check FlashLoRA availability
    if (use_flash_lora_ && !FlashLoRA::is_available(device)) {
        spdlog::warn("FlashLoRA requested but not available on device, disabling");
        use_flash_lora_ = false;
    }
    
    // Initialize B with Kaiming uniform
    B_ = std::make_unique<GPUTensor>(
        gpu_tensor_utils::kaiming_uniform({in_dim, rank}, 0.0f, device)
    );
    B_->requires_grad = true;
    
    // Initialize A with zeros (standard LoRA initialization)
    A_ = std::make_unique<GPUTensor>(
        gpu_tensor_utils::zeros({rank, out_dim}, device)
    );
    A_->requires_grad = true;
    
    spdlog::debug("GPULoRALayer created: in_dim={}, out_dim={}, rank={}, scaling={}, device={}, fused={}, flash={}",
                  in_dim_, out_dim_, rank_, scaling_, static_cast<int>(device.type), 
                  use_fused_kernels_, use_flash_lora_);
}

GPUTensor GPULoRALayer::forward(const GPUTensor& input) {
    // Verify input is on the same device
    if (input.device() != device_) {
        throw std::runtime_error("Input device mismatch in GPULoRALayer::forward");
    }
    if (input.shape().size() != 2) {
        throw std::invalid_argument("GPULoRALayer::forward expects a 2D input tensor");
    }
    if (input.shape()[1] != in_dim_) {
        throw std::invalid_argument("GPULoRALayer::forward input feature dimension mismatch");
    }
    
    // Gradient checkpointing: Only cache input if NOT checkpointing
    // When checkpointing, we save minimal data and recompute during backward
    if (!use_checkpointing_) {
        cached_input_ = input.clone();
    }
    
    // Try FlashLoRA first if enabled (CUDA only)
    if (use_flash_lora_ && device_.type == DeviceType::CUDA) {
        try {
            // Note: B is [in_dim, rank], but FlashLoRA expects [rank, in_dim]
            // Note: A is [rank, out_dim], but FlashLoRA expects [out_dim, rank]
            GPUTensor B_T = B_->transpose();  // [rank, in_dim]
            GPUTensor A_T = A_->transpose();  // [out_dim, rank]
            
            auto output = FlashLoRA::forward(input, B_T, A_T, scaling_);
            
            // Only cache intermediate if not checkpointing
            if (!use_checkpointing_) {
                // NOTE: For backward pass compatibility with existing code, we still
                // need to cache intermediate h = input @ B. This partially defeats
                // FlashLoRA's memory optimization. A future improvement would be to
                // implement FlashLoRA::backward_cached() that recomputes h on-the-fly
                // during backward pass, eliminating this extra matmul and storage.
                // For now, we prioritize API compatibility and correctness.
                cached_h_ = input.matmul(*B_);
            }
            
            return output;
        } catch (const std::exception& e) {
            spdlog::warn("FlashLoRA forward failed: {}, falling back to standard", e.what());
            use_flash_lora_ = false;  // Disable for future calls
        }
    }
    
    // Try to use fused kernels if enabled and on CUDA/HIP
    if (use_fused_kernels_ &&
        (device_.type == DeviceType::CUDA || device_.type == DeviceType::HIP || device_.type == DeviceType::VULKAN)) {
#ifdef THEMIS_ENABLE_CUDA
        if (device_.type == DeviceType::CUDA) {
            // Use fused CUDA kernel
            auto batch_size = input.shape()[0];
            GPUTensor output({batch_size, out_dim_}, device_);
            
            // Get raw pointers for GPU kernel
            // Safety: GPUTensor guarantees proper float alignment for GPU memory
            // All GPU tensors are allocated with cudaMalloc which provides proper alignment
            assert(performance::is_aligned<alignof(float)>(input.data()) && 
                   "Input tensor must be float-aligned for GPU operations");
            assert(performance::is_aligned<alignof(float)>(B_->data()) && 
                   "B tensor must be float-aligned for GPU operations");
            assert(performance::is_aligned<alignof(float)>(A_->data()) && 
                   "A tensor must be float-aligned for GPU operations");
            assert(performance::is_aligned<alignof(float)>(output.data()) && 
                   "Output tensor must be float-aligned for GPU operations");
            
            const float* input_ptr = reinterpret_cast<const float*>(input.data());
            const float* B_ptr = reinterpret_cast<const float*>(B_->data());
            const float* A_ptr = reinterpret_cast<const float*>(A_->data());
            float* output_ptr = reinterpret_cast<float*>(output.data());
            
            cudaError_t err = cuda::fused::launch_fused_lora_forward(
                input_ptr, B_ptr, A_ptr, output_ptr,
                batch_size, in_dim_, rank_, out_dim_, scaling_);
            
            if (err == cudaSuccess) {
                // Only cache h for backward if not checkpointing
                if (!use_checkpointing_) {
                    cached_h_ = input.matmul(*B_);
                }
                return output;
            }
            // Fall back to unfused on error
            spdlog::warn("Fused CUDA kernel failed, falling back to unfused");
        }
#endif
#ifdef THEMIS_ENABLE_HIP
        if (device_.type == DeviceType::HIP) {
            // Use fused HIP kernel
            auto batch_size = input.shape()[0];
            GPUTensor output({batch_size, out_dim_}, device_);
            
            // Get raw pointers for GPU kernel
            // Safety: GPUTensor guarantees proper float alignment for GPU memory
            // All GPU tensors are allocated with hipMalloc which provides proper alignment
            assert(performance::is_aligned<alignof(float)>(input.data()) && 
                   "Input tensor must be float-aligned for GPU operations");
            assert(performance::is_aligned<alignof(float)>(B_->data()) && 
                   "B tensor must be float-aligned for GPU operations");
            assert(performance::is_aligned<alignof(float)>(A_->data()) && 
                   "A tensor must be float-aligned for GPU operations");
            assert(performance::is_aligned<alignof(float)>(output.data()) && 
                   "Output tensor must be float-aligned for GPU operations");
            
            const float* input_ptr = reinterpret_cast<const float*>(input.data());
            const float* B_ptr = reinterpret_cast<const float*>(B_->data());
            const float* A_ptr = reinterpret_cast<const float*>(A_->data());
            float* output_ptr = reinterpret_cast<float*>(output.data());
            
            hipError_t err = hip::fused::launch_fused_lora_forward(
                input_ptr, B_ptr, A_ptr, output_ptr,
                batch_size, in_dim_, rank_, out_dim_, scaling_);
            
            if (err == hipSuccess) {
                // Only cache h for backward if not checkpointing
                if (!use_checkpointing_) {
                    cached_h_ = input.matmul(*B_);
                }
                return output;
            }
            // Fall back to unfused on error
            spdlog::warn("Fused HIP kernel failed, falling back to unfused");
        }
#endif
#ifdef THEMIS_ENABLE_VULKAN
        if (device_.type == DeviceType::VULKAN) {
            auto batch_size = input.shape()[0];
            GPUTensor output({batch_size, out_dim_}, device_);

            static bool vulkan_ready = false;
            if (!vulkan_ready) {
                if (!::themis::lora::vulkan::is_vulkan_available() ||
                    !::themis::lora::vulkan::initialize_vulkan_lora(device_.device_id)) {
                    spdlog::warn("Vulkan fused forward init failed, falling back to unfused");
                } else {
                    vulkan_ready = true;
                }
            }

            if (vulkan_ready) {
                auto input_host = input.download();
                auto b_host = B_->download();
                auto a_host = A_->download();
                std::vector<float> output_host(batch_size * out_dim_);

                try {
                    ::themis::lora::vulkan::launch_fused_lora_forward(
                        input_host.data(),
                        b_host.data(),
                        a_host.data(),
                        output_host.data(),
                        batch_size,
                        in_dim_,
                        rank_,
                        out_dim_,
                        scaling_);

                    output.upload(output_host);
                    if (!use_checkpointing_) {
                        // Keep backward compatibility with existing checkpoint logic.
                        cached_h_ = input.matmul(*B_);
                    }
                    return output;
                } catch (const std::exception& e) {
                    spdlog::warn("Fused Vulkan forward kernel failed, falling back to unfused: {}", e.what());
                }
            }
        }
#endif
    }
    
    // Unfused path (original implementation)
    // Forward: output = input @ B @ A * scaling
    // Step 1: h = input @ B
    GPUTensor h = input.matmul(*B_);
    
    // Only cache h if not checkpointing (save memory)
    if (!use_checkpointing_) {
        cached_h_ = h.clone();
    }
    
    // Step 2: output = h @ A
    auto output = h.matmul(*A_);
    
    // Step 3: Scale output
    if (std::abs(scaling_ - 1.0f) > 1e-6f) {
        output = output * scaling_;
    }
    
    return output;
}

GPUTensor GPULoRALayer::backward(const GPUTensor& grad_output) {
    // Verify grad_output is on the same device
    if (grad_output.device() != device_) {
        throw std::runtime_error("Gradient device mismatch in GPULoRALayer::backward");
    }
    if (grad_output.shape().size() != 2) {
        throw std::invalid_argument("GPULoRALayer::backward expects a 2D grad_output tensor");
    }
    if (grad_output.shape()[1] != out_dim_) {
        throw std::invalid_argument("GPULoRALayer::backward grad_output feature dimension mismatch");
    }
    if (cached_input_.shape().empty()) {
        throw std::runtime_error("GPULoRALayer::backward called before a successful forward pass");
    }
    
    // Try to use fused kernels if enabled and on CUDA/HIP
    if (use_fused_kernels_ &&
        (device_.type == DeviceType::CUDA || device_.type == DeviceType::HIP || device_.type == DeviceType::VULKAN)) {
#ifdef THEMIS_ENABLE_VULKAN
        if (device_.type == DeviceType::VULKAN) {
            if (cached_input_.shape().empty()) {
                throw std::runtime_error("No cached input for Vulkan fused backward pass");
            }

            auto batch_size = grad_output.shape()[0];
            A_->ensure_grad();
            B_->ensure_grad();
            GPUTensor grad_input({batch_size, in_dim_}, device_);

            static bool vulkan_ready = false;
            if (!vulkan_ready) {
                if (!::themis::lora::vulkan::is_vulkan_available() ||
                    !::themis::lora::vulkan::initialize_vulkan_lora(device_.device_id)) {
                    spdlog::warn("Vulkan fused backward init failed, falling back to unfused");
                } else {
                    vulkan_ready = true;
                }
            }

            if (vulkan_ready) {
                auto input_host = cached_input_.download();
                auto b_host = B_->download();
                auto a_host = A_->download();
                auto grad_output_host = grad_output.download();

                std::vector<float> grad_a_host(rank_ * out_dim_);
                std::vector<float> grad_b_host(in_dim_ * rank_);
                std::vector<float> grad_input_host(batch_size * in_dim_);

                try {
                    ::themis::lora::vulkan::launch_fused_lora_backward(
                        input_host.data(),
                        b_host.data(),
                        a_host.data(),
                        grad_output_host.data(),
                        grad_a_host.data(),
                        grad_b_host.data(),
                        grad_input_host.data(),
                        batch_size,
                        in_dim_,
                        rank_,
                        out_dim_,
                        scaling_);

                    A_->grad->upload(grad_a_host);
                    B_->grad->upload(grad_b_host);
                    grad_input.upload(grad_input_host);
                    return grad_input;
                } catch (const std::exception& e) {
                    spdlog::warn("Fused Vulkan backward kernel failed, falling back to unfused: {}", e.what());
                }
            }
        }
#endif
#ifdef THEMIS_ENABLE_CUDA
        if (device_.type == DeviceType::CUDA) {
            // Use fused CUDA backward kernel
            auto batch_size = grad_output.shape()[0];
            
            // Prepare gradient tensors
            A_->ensure_grad();
            B_->ensure_grad();
            GPUTensor grad_input({batch_size, in_dim_}, device_);
            
            // Get raw pointers for GPU kernel
            // Safety: GPUTensor guarantees proper float alignment for GPU memory
            // All GPU tensors (including gradients) are allocated with cudaMalloc
            assert(performance::is_aligned<alignof(float)>(input_for_backward.data()) && 
                   "Input tensor must be float-aligned for GPU operations");
            assert(performance::is_aligned<alignof(float)>(B_->data()) && 
                   "B tensor must be float-aligned for GPU operations");
            assert(performance::is_aligned<alignof(float)>(A_->data()) && 
                   "A tensor must be float-aligned for GPU operations");
            assert(performance::is_aligned<alignof(float)>(grad_output.data()) && 
                   "Grad output tensor must be float-aligned for GPU operations");
            assert(performance::is_aligned<alignof(float)>(A_->grad->data()) && 
                   "Grad A tensor must be float-aligned for GPU operations");
            assert(performance::is_aligned<alignof(float)>(B_->grad->data()) && 
                   "Grad B tensor must be float-aligned for GPU operations");
            assert(performance::is_aligned<alignof(float)>(grad_input.data()) && 
                   "Grad input tensor must be float-aligned for GPU operations");
            
            const float* input_ptr = reinterpret_cast<const float*>(input_for_backward.data());
            const float* B_ptr = reinterpret_cast<const float*>(B_->data());
            const float* A_ptr = reinterpret_cast<const float*>(A_->data());
            const float* grad_output_ptr = reinterpret_cast<const float*>(grad_output.data());
            float* grad_A_ptr = reinterpret_cast<float*>(A_->grad->data());
            float* grad_B_ptr = reinterpret_cast<float*>(B_->grad->data());
            float* grad_input_ptr = reinterpret_cast<float*>(grad_input.data());
            
            cudaError_t err = cuda::fused::launch_fused_lora_backward(
                input_ptr, B_ptr, A_ptr, grad_output_ptr,
                grad_A_ptr, grad_B_ptr, grad_input_ptr,
                batch_size, in_dim_, rank_, out_dim_, scaling_);
            
            if (err == cudaSuccess) {
                return grad_input;
            }
            // Fall back to unfused on error
            spdlog::warn("Fused CUDA backward kernel failed, falling back to unfused");
        }
#endif
#ifdef THEMIS_ENABLE_HIP
        if (device_.type == DeviceType::HIP) {
            // Use fused HIP backward kernel
            auto batch_size = grad_output.shape()[0];
            
            // Prepare gradient tensors
            A_->ensure_grad();
            B_->ensure_grad();
            GPUTensor grad_input({batch_size, in_dim_}, device_);
            
            // Get raw pointers for GPU kernel
            // Safety: GPUTensor guarantees proper float alignment for GPU memory
            // All GPU tensors (including gradients) are allocated with hipMalloc
            assert(performance::is_aligned<alignof(float)>(input_for_backward.data()) && 
                   "Input tensor must be float-aligned for GPU operations");
            assert(performance::is_aligned<alignof(float)>(B_->data()) && 
                   "B tensor must be float-aligned for GPU operations");
            assert(performance::is_aligned<alignof(float)>(A_->data()) && 
                   "A tensor must be float-aligned for GPU operations");
            assert(performance::is_aligned<alignof(float)>(grad_output.data()) && 
                   "Grad output tensor must be float-aligned for GPU operations");
            assert(performance::is_aligned<alignof(float)>(A_->grad->data()) && 
                   "Grad A tensor must be float-aligned for GPU operations");
            assert(performance::is_aligned<alignof(float)>(B_->grad->data()) && 
                   "Grad B tensor must be float-aligned for GPU operations");
            assert(performance::is_aligned<alignof(float)>(grad_input.data()) && 
                   "Grad input tensor must be float-aligned for GPU operations");
            
            const float* input_ptr = reinterpret_cast<const float*>(input_for_backward.data());
            const float* B_ptr = reinterpret_cast<const float*>(B_->data());
            const float* A_ptr = reinterpret_cast<const float*>(A_->data());
            const float* grad_output_ptr = reinterpret_cast<const float*>(grad_output.data());
            float* grad_A_ptr = reinterpret_cast<float*>(A_->grad->data());
            float* grad_B_ptr = reinterpret_cast<float*>(B_->grad->data());
            float* grad_input_ptr = reinterpret_cast<float*>(grad_input.data());
            
            hipError_t err = hip::fused::launch_fused_lora_backward(
                input_ptr, B_ptr, A_ptr, grad_output_ptr,
                grad_A_ptr, grad_B_ptr, grad_input_ptr,
                batch_size, in_dim_, rank_, out_dim_, scaling_);
            
            if (err == hipSuccess) {
                return grad_input;
            }
            // Fall back to unfused on error
            spdlog::warn("Fused HIP backward kernel failed, falling back to unfused");
        }
#endif
    }

    // Gradient checkpointing: Recompute activations if needed
    GPUTensor input_for_backward;
    GPUTensor h_for_backward;

    if (use_checkpointing_) {
        // Recompute activations (trade compute for memory)
        spdlog::debug("Recomputing activations for checkpointed layer {}", layer_id_);

        // Note: In a full implementation, the input should be saved by the
        // checkpointer. For now, we check if cached_input_ has data.
        if (cached_input_.shape().empty()) {
            throw std::runtime_error(
                "Checkpointing enabled but no input saved. "
                "This is likely a bug in the checkpointing integration."
            );
        }

        input_for_backward = cached_input_.clone();
        h_for_backward = input_for_backward.matmul(*B_);
    } else {
        // Use cached activations (normal path)
        input_for_backward = cached_input_.clone();
        if (cached_h_.shape().empty()) {
            throw std::runtime_error("GPULoRALayer::backward missing cached intermediate activation");
        }
        h_for_backward = cached_h_.clone();
    }
    
    // Unfused path (original implementation)
    // Apply scaling to gradient
    GPUTensor scaled_grad = (std::abs(scaling_ - 1.0f) > 1e-6f) 
                            ? grad_output * scaling_
                            : grad_output.clone();
    
    // Compute gradients using recomputed or cached activations
    // grad_A = h^T @ scaled_grad
    auto h_t = h_for_backward.transpose();
    A_->ensure_grad();
    *(A_->grad) = h_t.matmul(scaled_grad);
    
    // grad_B = input^T @ (scaled_grad @ A^T)
    auto A_t = A_->transpose();
    auto grad_h = scaled_grad.matmul(A_t);
    auto input_t = input_for_backward.transpose();
    B_->ensure_grad();
    *(B_->grad) = input_t.matmul(grad_h);
    
    // grad_input = (scaled_grad @ A^T) @ B^T
    auto B_t = B_->transpose();
    auto grad_input = grad_h.matmul(B_t);
    
    return grad_input;
}

std::vector<GPUTensor*> GPULoRALayer::parameters() {
    return {B_.get(), A_.get()};
}

std::vector<GPUTensor*> GPULoRALayer::gradients() {
    return {B_->grad.get(), A_->grad.get()};
}

void GPULoRALayer::zero_grad() {
    if (B_->grad) {
        B_->grad->zero();
    }
    if (A_->grad) {
        A_->grad->zero();
    }
}

std::pair<GPUTensor, GPUTensor> GPULoRALayer::get_weights() const {
    return {B_->clone(), A_->clone()};
}

void GPULoRALayer::set_weights(const GPUTensor& B, const GPUTensor& A) {
    // Verify shapes
    if (B.shape() != std::vector<size_t>{in_dim_, rank_}) {
        throw std::invalid_argument("B shape mismatch in set_weights");
    }
    if (A.shape() != std::vector<size_t>{rank_, out_dim_}) {
        throw std::invalid_argument("A shape mismatch in set_weights");
    }
    
    // Move to correct device if needed and set
    *B_ = B.device() == device_ ? B.clone() : B.to(device_);
    *A_ = A.device() == device_ ? A.clone() : A.to(device_);
    
    B_->requires_grad = true;
    A_->requires_grad = true;
}

void GPULoRALayer::to(const Device& target_device) {
    if (device_ == target_device) {
        return;
    }
    
    // Move parameters
    B_->to_inplace(target_device);
    A_->to_inplace(target_device);
    
    // Update device
    device_ = target_device;
    
    spdlog::debug("GPULoRALayer moved to device: {}", static_cast<int>(device_.type));
}

// ============================================================================
// GPUSGDOptimizer Implementation
// ============================================================================

GPUSGDOptimizer::GPUSGDOptimizer(float learning_rate, float momentum, float weight_decay)
    : learning_rate_(learning_rate)
    , momentum_(momentum)
    , weight_decay_(weight_decay) {
    
    if (learning_rate_ <= 0.0f) {
        throw std::invalid_argument("Learning rate must be positive");
    }
    if (momentum_ < 0.0f || momentum_ >= 1.0f) {
        throw std::invalid_argument("Momentum must be in [0, 1)");
    }
    if (weight_decay_ < 0.0f) {
        throw std::invalid_argument("Weight decay must be non-negative");
    }
    
    spdlog::debug("GPUSGDOptimizer created: lr={}, momentum={}, weight_decay={}",
                  learning_rate_, momentum_, weight_decay_);
}

void GPUSGDOptimizer::add_parameters(const std::vector<GPUTensor*>& params) {
    parameters_.insert(parameters_.end(), params.begin(), params.end());
    
    // Initialize momentum buffers if needed
    if (momentum_ > 0.0f) {
        for (auto* param : params) {
            auto buffer = std::make_unique<GPUTensor>(
                gpu_tensor_utils::zeros(param->shape(), param->device())
            );
            momentum_buffers_.push_back(std::move(buffer));
        }
    }
    
    spdlog::debug("GPUSGDOptimizer: {} parameters registered", parameters_.size());
}

void GPUSGDOptimizer::step() {
    for (size_t i = 0; i < parameters_.size(); ++i) {
        auto* param = parameters_[i];
        
        if (!param->grad) {
            spdlog::warn("Parameter {} has no gradient, skipping", i);
            continue;
        }
        
        // Try to use fused kernel if on CUDA/HIP
        bool fused_success = false;
        
#ifdef THEMIS_ENABLE_CUDA
        if (param->device().type == DeviceType::CUDA) {
            float* param_ptr = reinterpret_cast<float*>(param->data());
            const float* grad_ptr = reinterpret_cast<const float*>(param->grad->data());
            float* momentum_ptr = (momentum_ > 0.0f && i < momentum_buffers_.size()) 
                ? reinterpret_cast<float*>(momentum_buffers_[i]->data()) 
                : nullptr;
            
            cudaError_t err = cuda::fused::launch_fused_sgd_step(
                param_ptr, grad_ptr, momentum_ptr,
                param->size(), learning_rate_, momentum_, weight_decay_);
            
            fused_success = (err == cudaSuccess);
            if (!fused_success) {
                spdlog::warn("Fused CUDA optimizer step failed, falling back to unfused");
            }
        }
#endif
        
#ifdef THEMIS_ENABLE_HIP
        if (param->device().type == DeviceType::HIP) {
            float* param_ptr = reinterpret_cast<float*>(param->data());
            const float* grad_ptr = reinterpret_cast<const float*>(param->grad->data());
            float* momentum_ptr = (momentum_ > 0.0f && i < momentum_buffers_.size()) 
                ? reinterpret_cast<float*>(momentum_buffers_[i]->data()) 
                : nullptr;
            
            hipError_t err = hip::fused::launch_fused_sgd_step(
                param_ptr, grad_ptr, momentum_ptr,
                param->size(), learning_rate_, momentum_, weight_decay_);
            
            fused_success = (err == hipSuccess);
            if (!fused_success) {
                spdlog::warn("Fused HIP optimizer step failed, falling back to unfused");
            }
        }
#endif
        
        // Unfused path if fused failed or not available
        if (!fused_success) {
            // Get gradient
            auto grad = param->grad->clone();
            
            // Add weight decay (L2 regularization)
            if (weight_decay_ > 0.0f) {
                // grad = grad + weight_decay * param
                auto decay_term = *param * weight_decay_;
                grad = grad + decay_term;
            }
            
            // Apply momentum if enabled
            if (momentum_ > 0.0f) {
                auto* v = momentum_buffers_[i].get();
                
                // v = momentum * v + (1 - momentum) * grad
                auto momentum_term = *v * momentum_;
                auto grad_term = grad * (1.0f - momentum_);
                *v = momentum_term + grad_term;
                
                // Use momentum buffer for update
                grad = v->clone();
            }
            
            // Update parameter: param = param - lr * grad
            auto update = grad * learning_rate_;
            *param = *param - update;
        }
    }
}

void GPUSGDOptimizer::zero_grad() {
    for (auto* param : parameters_) {
        if (param->grad) {
            param->grad->zero();
        }
    }
}

// ============================================================================
// GPULoRATrainer Implementation
// ============================================================================

GPULoRATrainer::GPULoRATrainer(GPULoRALayer* layer, GPUSGDOptimizer* optimizer)
    : layer_(layer)
    , optimizer_(optimizer) {
    
    if (!layer_ || !optimizer_) {
        throw std::invalid_argument("Layer and optimizer cannot be null");
    }
}

float GPULoRATrainer::train_step(const GPUTensor& input, const GPUTensor& target) {
    // Zero gradients
    optimizer_->zero_grad();
    
    // Forward pass
    auto output = layer_->forward(input);
    
    // Compute loss
    float loss = compute_mse_loss(output, target);
    
    // Compute gradient
    auto grad_output = compute_mse_grad(output, target);
    
    // Backward pass
    layer_->backward(grad_output);
    
    // Update parameters
    optimizer_->step();
    
    return loss;
}

float GPULoRATrainer::eval_step(const GPUTensor& input, const GPUTensor& target) {
    // Forward pass only (no gradient computation)
    auto output = layer_->forward(input);
    
    // Compute loss
    float loss = compute_mse_loss(output, target);
    
    return loss;
}

float GPULoRATrainer::compute_mse_loss(const GPUTensor& output, const GPUTensor& target) {
    // MSE = mean((output - target)^2)
    auto diff = output - target;
    auto squared = diff.mul(diff);
    
    // Download and compute mean on CPU
    auto squared_data = squared.cpu_data();
    float sum = 0.0f;
    for (auto val : squared_data) {
        sum += val;
    }
    
    return sum / squared_data.size();
}

GPUTensor GPULoRATrainer::compute_mse_grad(const GPUTensor& output, const GPUTensor& target) {
    // grad_MSE = 2 * (output - target) / n
    auto diff = output - target;
    float scale = 2.0f / static_cast<float>(output.size());
    return diff * scale;
}

} // namespace lora
} // namespace llm
} // namespace themis

