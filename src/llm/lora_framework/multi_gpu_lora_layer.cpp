/**
 * @file multi_gpu_lora_layer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=6, H=19, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_framework/multi_gpu_lora_layer.h"
#include <spdlog/spdlog.h>
#include <chrono>
#include <stdexcept>

namespace themis {
namespace llm {
namespace lora {

MultiGPULoRALayer::MultiGPULoRALayer(
    size_t in_dim, size_t out_dim, size_t rank,
    float scaling,
    const MultiGPUContext& ctx,
    CommBackend backend,
    bool use_fused_kernels)
    : ctx_(ctx), backend_type_(backend), gradients_synced_(false) {
    
    if (ctx_.num_gpus() == 0) {
        throw std::runtime_error("MultiGPULoRALayer requires at least one GPU");
    }
    
    spdlog::info("Creating MultiGPULoRALayer: {}x{}, rank={}, {} GPUs",
                in_dim, out_dim, rank, ctx_.num_gpus());
    
    // Create layer replica on each GPU
    for (int i = 0; i < ctx_.num_gpus(); ++i) {
        Device device = ctx_.get_device(i);
        layers_.push_back(
            std::make_unique<GPULoRALayer>(
                in_dim, out_dim, rank, scaling, device, use_fused_kernels));
    }
    
    // Initialize communication backend
    initialize_backend(backend);
    
    // Broadcast parameters to ensure all GPUs start with same weights
    broadcast_parameters();
}

void MultiGPULoRALayer::initialize_backend(CommBackend backend) {
    // Auto-select backend if requested
    if (backend == CommBackend::AUTO) {
        if (NCCLBackend::is_available() && ctx_.gpu_type() == DeviceType::CUDA) {
            backend = CommBackend::NCCL;
        } else if (RCCLBackend::is_available() && ctx_.gpu_type() == DeviceType::HIP) {
            backend = CommBackend::RCCL;
        } else {
            backend = CommBackend::CUSTOM;
        }
        spdlog::info("Auto-selected communication backend: {}", 
                    static_cast<int>(backend));
    }
    
    backend_type_ = backend;
    
    // Initialize selected backend
    // Note: In single-process multi-GPU, rank is GPU index
    int rank = 0;  // Would be process rank in multi-process setup
    int world_size = ctx_.num_gpus();
    
    switch (backend_type_) {
        case CommBackend::NCCL:
            if (NCCLBackend::is_available()) {
                nccl_backend_ = std::make_unique<NCCLBackend>(ctx_, rank, world_size);
                if (!nccl_backend_->initialize()) {
                    spdlog::warn("NCCL initialization failed, falling back to custom backend");
                    backend_type_ = CommBackend::CUSTOM;
                    nccl_backend_.reset();
                }
            } else {
                spdlog::warn("NCCL not available, using custom backend");
                backend_type_ = CommBackend::CUSTOM;
            }
            break;
            
        case CommBackend::RCCL:
            if (RCCLBackend::is_available()) {
                rccl_backend_ = std::make_unique<RCCLBackend>(ctx_, rank, world_size);
                if (!rccl_backend_->initialize()) {
                    spdlog::warn("RCCL initialization failed, falling back to custom backend");
                    backend_type_ = CommBackend::CUSTOM;
                    rccl_backend_.reset();
                }
            } else {
                spdlog::warn("RCCL not available, using custom backend");
                backend_type_ = CommBackend::CUSTOM;
            }
            break;
            
        case CommBackend::CUSTOM:
            // Always fall through to custom backend
            break;
            
        default:
            break;
    }
    
    // Initialize custom backend as final fallback
    if (backend_type_ == CommBackend::CUSTOM) {
        custom_backend_ = std::make_unique<CustomAllReduce>(ctx_, rank, world_size);
        custom_backend_->initialize();
        spdlog::info("Using custom all-reduce backend");
    }
}

std::vector<GPUTensor> MultiGPULoRALayer::forward(const std::vector<GPUTensor>& inputs) {
    if (static_cast<int>(inputs.size()) != static_cast<size_t>(ctx_.num_gpus())) {
        throw std::invalid_argument(
            "Number of input tensors must match number of GPUs");
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    
    std::vector<GPUTensor> outputs = {};

    outputs.reserve(inputs.size());
    
    // Forward pass on each GPU independently
    for (int device_index = 0; device_index < ctx_.num_gpus(); ++device_index) {
        const size_t i = static_cast<size_t>(device_index);
        const Device expected_device = ctx_.get_device(device_index);
        // Verify input is on correct device
        if (inputs[i].device().device_id != expected_device.device_id ||
            inputs[i].device().type != expected_device.type) {
            throw std::invalid_argument(
                "Input tensor " + std::to_string(i) + " is not on correct device");
        }
        
        outputs.push_back(layers_[i]->forward(inputs[i]));
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    stats_.computation_time_ms += 
        std::chrono::duration<float, std::milli>(end - start).count();
    
    gradients_synced_ = false;  // Forward invalidates sync state
    
    return outputs;
}

std::vector<GPUTensor> MultiGPULoRALayer::backward(
    const std::vector<GPUTensor>& grad_outputs) {
    
    if (static_cast<int>(grad_outputs.size()) != static_cast<size_t>(ctx_.num_gpus())) {
        throw std::invalid_argument(
            "Number of gradient tensors must match number of GPUs");
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    
    std::vector<GPUTensor> grad_inputs = {};

    grad_inputs.reserve(grad_outputs.size());
    
    // Backward pass on each GPU independently
    for (size_t i = 0; i < grad_outputs.size(); ++i) {
        grad_inputs.push_back(layers_[i]->backward(grad_outputs[i]));
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    stats_.computation_time_ms += 
        std::chrono::duration<float, std::milli>(end - start).count();
    
    gradients_synced_ = false;  // Backward produces local gradients
    
    return grad_inputs;
}

bool MultiGPULoRALayer::synchronize_gradients() {
    if (gradients_synced_) {
        return true;  // Already synchronized
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    
    bool success = allreduce_gradients();
    
    auto end = std::chrono::high_resolution_clock::now();
    stats_.communication_time_ms += 
        std::chrono::duration<float, std::milli>(end - start).count();
    stats_.num_syncs++;
    
    if (success) {
        gradients_synced_ = true;
    }
    
    return success;
}

bool MultiGPULoRALayer::allreduce_gradients() {
    if (ctx_.num_gpus() == 1) {
        return true;  // Single GPU, no reduction needed
    }
    
    // Collect all gradients from all GPUs
    std::vector<std::vector<GPUTensor*>> all_gradients;
    for (auto& layer : layers_) {
        all_gradients.push_back(layer->gradients());
    }
    
    // Number of parameters per layer (should be same for all)
    size_t num_params = all_gradients[0].size();
    
    // All-reduce each parameter separately
    for (size_t param_idx = 0; param_idx < num_params; ++param_idx) {
        std::vector<GPUTensor*> param_grads = {};

        for (size_t gpu_idx = 0; gpu_idx < all_gradients.size(); ++gpu_idx) {
            param_grads.push_back(all_gradients[gpu_idx][param_idx]);
        }
        
        // Perform all-reduce using selected backend
        bool success = false;
        if (nccl_backend_ && nccl_backend_->is_initialized()) {
            success = nccl_backend_->allreduce(param_grads, true);
            stats_.bytes_communicated += param_grads[0]->size() * sizeof(float);
        } else if (rccl_backend_ && rccl_backend_->is_initialized()) {
            success = rccl_backend_->allreduce(param_grads, true);
            stats_.bytes_communicated += param_grads[0]->size() * sizeof(float);
        } else if (custom_backend_ && custom_backend_->is_initialized()) {
            success = custom_backend_->allreduce(param_grads, true);
            stats_.bytes_communicated += param_grads[0]->size() * sizeof(float);
        }
        
        if (!success) {
            spdlog::error("Failed to all-reduce gradients for parameter {}", param_idx);
            return false;
        }
    }
    
    return true;
}

bool MultiGPULoRALayer::broadcast_parameters() {
    if (ctx_.num_gpus() == 1) {
        return true;  // Single GPU, no broadcast needed
    }
    
    spdlog::info("Broadcasting parameters from rank 0 to all GPUs");
    
    // Get parameters from rank 0 (master GPU)
    auto master_params = layers_[0]->parameters();
    
    // Copy to all other GPUs
    for (int i = 1; i < ctx_.num_gpus(); ++i) {
        auto target_params = layers_[i]->parameters();
        
        if (static_cast<int>(master_params.size()) != static_cast<int>(target_params.size())) {
            spdlog::error("Parameter count mismatch between GPUs");
            return false;
        }
        
        for (size_t j = 0; j < master_params.size(); ++j) {
            // Copy from master to target
            auto master_data = master_params[j]->cpu_data();
            target_params[j]->upload(master_data);
        }
    }
    
    spdlog::info("Parameter broadcast complete");
    return true;
}

void MultiGPULoRALayer::zero_grad() {
    for (auto& layer : layers_) {
        layer->zero_grad();
    }
    gradients_synced_ = false;
}

GPULoRALayer& MultiGPULoRALayer::get_layer(int rank) {
    if (rank < 0 || rank >= ctx_.num_gpus()) {
        throw std::out_of_range("Invalid GPU rank: " + std::to_string(rank));
    }
    return *layers_[rank];
}

std::vector<GPULoRALayer*> MultiGPULoRALayer::get_layers() {
    std::vector<GPULoRALayer*> result = {};

    result.reserve(layers_.size());
    for (auto& layer : layers_) {
        result.push_back(layer.get());
    }
    return result;
}

} // namespace lora
} // namespace llm
} // namespace themis

