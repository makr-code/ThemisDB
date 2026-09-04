/**
 * @file paged_optimizer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 81/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_framework/paged_optimizer.h"
#include <cmath>
#include <algorithm>
#include <chrono>

namespace themis {
namespace llm {
namespace lora {

// ===== PagedAdamWOptimizer Implementation =====

PagedAdamWOptimizer::PagedAdamWOptimizer(
    float learning_rate,
    float beta1,
    float beta2,
    float weight_decay,
    float epsilon,
    const PagedOptimizerConfig& config)
    : learning_rate_(learning_rate),
      beta1_(beta1),
      beta2_(beta2),
      epsilon_(epsilon),
      weight_decay_(weight_decay),
      config_(config) {
    
    // Create memory manager if paging is enabled
    if (config_.enable_paging) {
        memory_manager_ = std::make_unique<PagedMemoryManager>(config_.active_set_size);
    }
}

void PagedAdamWOptimizer::add_parameters(const std::vector<Tensor*>& params) {
    for (Tensor* param : params) {
        if (!param) {
          continue;
        }
        
        parameters_.push_back(param);
        
        // Initialize optimizer state for this parameter
        PagedOptimizerState state;
        
        size_t param_size = param->size() * sizeof(float);
        
        if (config_.enable_paging && memory_manager_) {
            // Allocate paged buffers for optimizer states
            state.momentum = memory_manager_->allocate(param_size, Device::cpu());
            state.variance = memory_manager_->allocate(param_size, Device::cpu());
            
            state.momentum_on_gpu = false;
            state.variance_on_gpu = false;
        }
        
        states_[param] = state;
    }
}

bool PagedAdamWOptimizer::ensureStateOnGPU(PagedOptimizerState& state) {
    if (!config_.enable_paging || !memory_manager_) {
        return false;
    }
    
    bool success = true;
    
    // Page in momentum if needed
    if (!state.momentum_on_gpu && state.momentum.id != 0) {
        success &= memory_manager_->pageIn(state.momentum, compute_stream_);
        state.momentum_on_gpu = memory_manager_->isOnGPU(state.momentum);
        if (state.momentum_on_gpu) {
            metrics_.num_page_ins++;
            metrics_.bytes_transferred += state.momentum.size_bytes;
        }
    }
    
    // Page in variance if needed
    if (!state.variance_on_gpu && state.variance.id != 0) {
        success &= memory_manager_->pageIn(state.variance, compute_stream_);
        state.variance_on_gpu = memory_manager_->isOnGPU(state.variance);
        if (state.variance_on_gpu) {
            metrics_.num_page_ins++;
            metrics_.bytes_transferred += state.variance.size_bytes;
        }
    }
    
    return success;
}

void PagedAdamWOptimizer::updateParameterCPU(Tensor* param, PagedOptimizerState& state) {
    // Get references to data
    std::vector<float>& param_data = param->data();
    std::vector<float>& grad_data = param->grad->data();
    
    if (param_data.empty() || grad_data.empty()) {
      return;
    }
    
    size_t size = param->size();
    
    // Get momentum and variance buffers (CPU)
    float* m_data = nullptr;
    float* v_data = nullptr;
    
    if (config_.enable_paging && state.momentum.cpu_ptr && state.variance.cpu_ptr) {
        m_data = static_cast<float*>(state.momentum.cpu_ptr);
        v_data = static_cast<float*>(state.variance.cpu_ptr);
    }
    
    // If no paged buffers, allocate temporary ones
    std::vector<float> m_temp, v_temp;
    if (!m_data) {
        m_temp.resize(size, 0.0f);
        m_data = m_temp.data();
    }
    if (!v_data) {
        v_temp.resize(size, 0.0f);
        v_data = v_temp.data();
    }
    
    // Bias correction factors
    float bias_correction1 = 1.0f - static_cast<float>(std::pow(beta1_, step_count_ + 1));
    float bias_correction2 = 1.0f - static_cast<float>(std::pow(beta2_, step_count_ + 1));
    
    // AdamW update rule
    for (size_t i = 0; i < size; ++i) {
        float grad = grad_data[i];
        
        // Update biased first moment estimate
        m_data[i] = beta1_ * m_data[i] + (1.0f - beta1_) * grad;
        
        // Update biased second raw moment estimate
        v_data[i] = beta2_ * v_data[i] + (1.0f - beta2_) * (grad * grad);
        
        // Compute bias-corrected first moment estimate
        float m_hat = m_data[i] / bias_correction1;
        
        // Compute bias-corrected second raw moment estimate
        float v_hat = v_data[i] / bias_correction2;
        
        // Update parameters with AdamW weight decay (decoupled)
        // AdamW applies weight decay directly to parameters, not through gradients
        param_data[i] = param_data[i] * (1.0f - learning_rate_ * weight_decay_) 
                        - learning_rate_ * m_hat / (std::sqrt(v_hat) + epsilon_);
    }
}

void PagedAdamWOptimizer::updateParameterGPU(Tensor* param, PagedOptimizerState& state) {
    // GPU update would be implemented here using CUDA kernels
    // For now, fall back to CPU
    updateParameterCPU(param, state);
}

void PagedAdamWOptimizer::step() {
    step_count_++;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Phase 1: Pre-fetch optimizer states (optional optimization)
    // This could be done asynchronously while GPU is busy with forward/backward
    
    // Phase 2: Optimizer step
    for (Tensor* param : parameters_) {
        if (!param) {
          continue;
        }
        
        auto it = states_.find(param);
        if (it == states_.end()) {
          continue;
        }
        
        PagedOptimizerState& state = it->second;
        
        if (config_.enable_paging && memory_manager_ && 
            memory_manager_->is_cuda_available()) {
            // Ensure state is on GPU
            ensureStateOnGPU(state);
            
            // Perform update on GPU
            updateParameterGPU(param, state);
        } else {
            // CPU-only path
            updateParameterCPU(param, state);
        }
    }
    
    // Phase 3: Post-step eviction
    if (config_.enable_paging && memory_manager_) {
        // Evict least recently used states to free GPU memory
        size_t num_evicted = memory_manager_->evictLRU(
            parameters_.size() / 4,  // Evict 25% of states
            compute_stream_
        );
        
        if (num_evicted > 0) {
            metrics_.num_page_outs += num_evicted;
            
            // Update state tracking
            for (auto& pair : states_) {
                PagedOptimizerState& state = pair.second;
                if (state.momentum.id != 0) {
                    state.momentum_on_gpu = memory_manager_->isOnGPU(state.momentum);
                }
                if (state.variance.id != 0) {
                    state.variance_on_gpu = memory_manager_->isOnGPU(state.variance);
                }
            }
        }
        
        // Update memory metrics
        metrics_.gpu_memory_used = memory_manager_->gpu_memory_used();
        metrics_.cpu_memory_used = memory_manager_->cpu_memory_used();
        metrics_.peak_gpu_memory = std::max(metrics_.peak_gpu_memory, metrics_.gpu_memory_used);
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    double elapsed_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    metrics_.transfer_time_ms += elapsed_ms;
    
    // Calculate average bandwidth
    if (metrics_.bytes_transferred > 0 && metrics_.transfer_time_ms > 0) {
        double total_gb = metrics_.bytes_transferred / (1024.0 * 1024.0 * 1024.0);
        double total_sec = metrics_.transfer_time_ms / 1000.0;
        metrics_.avg_transfer_bandwidth = total_gb / total_sec;
    }
}

void PagedAdamWOptimizer::zero_grad() {
    for (Tensor* param : parameters_) {
        if (param && param->requires_grad) {
            param->grad->zero();
        }
    }
}

// ===== PagedOptimizerStateManager Implementation =====

bool PagedOptimizerStateManager::ensureOnGPU(PagedOptimizerState& state, void* stream) {
    if (!memory_manager_) {
        return false;
    }
    
    bool success = true;
    
    // Page in momentum
    if (!state.momentum_on_gpu && state.momentum.id != 0) {
        success &= memory_manager_->pageIn(state.momentum, stream);
        state.momentum_on_gpu = memory_manager_->isOnGPU(state.momentum);
    }
    
    // Page in variance
    if (!state.variance_on_gpu && state.variance.id != 0) {
        success &= memory_manager_->pageIn(state.variance, stream);
        state.variance_on_gpu = memory_manager_->isOnGPU(state.variance);
    }
    
    return success;
}

size_t PagedOptimizerStateManager::evictUnused(
    std::unordered_map<Tensor*, PagedOptimizerState>& states,
    size_t num_to_evict,
    void* stream) {
    
    if (!memory_manager_) {
        return 0;
    }
    
    // Build list of evictable states (sorted by last access time)
    std::vector<std::pair<uint64_t, PagedOptimizerState*>> evictable;
    
    for (auto& pair : states) {
        PagedOptimizerState& state = pair.second;
        
        if (state.momentum_on_gpu && state.momentum.id != 0) {
            evictable.push_back({state.momentum.last_access_time, &state});
        }
    }
    
    // Sort by last access time (oldest first)
    std::sort(evictable.begin(), evictable.end(),
              [](const auto& a, const auto& b) {
                  return a.first < b.first;
              });
    
    // Evict up to num_to_evict states
    size_t evicted = 0;
    for (size_t i = 0; i < std::min(num_to_evict,static_cast<int>(evictable.size())); ++i) {
        PagedOptimizerState* state = evictable[i].second;
        
        // Page out momentum
        if (state->momentum_on_gpu && state->momentum.id != 0) {
            if (memory_manager_->pageOut(state->momentum, stream)) {
                state->momentum_on_gpu = false;
                evicted++;
            }
        }
        
        // Page out variance
        if (state->variance_on_gpu && state->variance.id != 0) {
            if (memory_manager_->pageOut(state->variance, stream)) {
                state->variance_on_gpu = false;
            }
        }
    }
    
    return evicted;
}

} // namespace lora
} // namespace llm
} // namespace themis
