/**
 * @file paged_optimizer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "lora_layers.h"
#include "paged_memory_manager.h"
#include <memory>
#include <vector>
#include <unordered_map>

namespace themis {
namespace llm {
namespace lora {

enum class EvictionPolicy {
    LRU,        // Least Recently Used
    LFU,        // Least Frequently Used
    FIFO,       // First In First Out
    ADAPTIVE    // Adaptive based on access pattern
};

struct PagedOptimizerConfig {
    // Enable paging (default: true if memory constrained)
    bool enable_paging = true;
    
    // Page size (default: 64 MB)
    size_t page_size_bytes = 64 * 1024 * 1024;
    
    // Active set size (states to keep on GPU)
    size_t active_set_size = 1024;
    
    // Prefetch distance (batches ahead)
    size_t prefetch_distance = 1;
    
    // Use unified memory (if available)
    bool use_unified_memory = false;
    
    // Eviction policy
    EvictionPolicy eviction_policy = EvictionPolicy::LRU;
};

struct PagedOptimizerState {
    PagedBuffer momentum;      // First moment (Adam)
    PagedBuffer variance;      // Second moment (Adam)
    PagedBuffer gradient;      // Current gradient (optional)
    
    bool momentum_on_gpu = false;
    bool variance_on_gpu = false;
    bool gradient_on_gpu = false;
};

struct PagingMetrics {
    /**
     * @brief Paging Metrics.
     * @return Return value.
     */
    virtual ~PagingMetrics() = default;
    size_t num_page_ins = 0;
    size_t num_page_outs = 0;
    size_t bytes_transferred = 0;
    double transfer_time_ms = 0.0;
    double avg_transfer_bandwidth = 0.0;  // GB/s
    
    // Memory usage
    size_t gpu_memory_used = 0;
    size_t cpu_memory_used = 0;
    size_t peak_gpu_memory = 0;
    
    // Reset metrics
    /**
     * @brief Reset the modification detection flag.
     * @details Implements reset without additional internal calls.
     */
    void reset() {
        num_page_ins = 0;
        num_page_outs = 0;
        bytes_transferred = 0;
        transfer_time_ms = 0.0;
        avg_transfer_bandwidth = 0.0;
        gpu_memory_used = 0;
        cpu_memory_used = 0;
        peak_gpu_memory = 0;
    }
};

class PagedAdamWOptimizer {
public:
    explicit PagedAdamWOptimizer(
        float learning_rate = 1e-3f,
        float beta1 = 0.9f,
        float beta2 = 0.999f,
        float weight_decay = 0.01f,
        float epsilon = 1e-8f,
        const PagedOptimizerConfig& config = PagedOptimizerConfig()
    );
    
    ~PagedAdamWOptimizer() = default;
    
    /**
     * @brief Add parameters.
     * @param[in] params Input parameter.
     */
    void add_parameters(const std::vector<Tensor*>& params);
    
    /**
     * @brief Step.
     */
    void step();
    
    /**
     * @brief Zero grad.
     */
    void zero_grad();
    
    float learning_rate() const { return learning_rate_; }
    
    /**
     * @brief Set learning rate.
     * @param[in] lr Input parameter.
     * @details Implements set_learning_rate without additional internal calls.
     */
    void set_learning_rate(float lr) { learning_rate_ = lr; }
    
    int step_count() const { return step_count_; }
    
    const PagingMetrics& get_metrics() const { return metrics_; }
    
    /**
     * @brief Reset metrics.
     * @details Calls: reset().
     */
    void reset_metrics() { metrics_.reset(); }
    
    bool is_paging_enabled() const { return config_.enable_paging; }
    
    bool is_cuda_available() const {
        return memory_manager_ ? memory_manager_->is_cuda_available() : false;
    }

private:
    // Optimizer hyperparameters
    float learning_rate_ = 0.0f;
    float beta1_ = 0.0f;
    float beta2_ = 0.0f;
    float epsilon_ = 0.0f;
    float weight_decay_ = 0.0f;
    int step_count_ = 0;
    
    // Configuration
    PagedOptimizerConfig config_;
    
    // Memory manager
    std::unique_ptr<PagedMemoryManager> memory_manager_;
    
    // Parameters being optimized
    std::vector<Tensor*> parameters_;
    
    // Optimizer states (paged)
    std::unordered_map<Tensor*, PagedOptimizerState> states_;
    
    // Metrics
    PagingMetrics metrics_;
    
    // CUDA stream for async transfers (nullptr for sync)
    void* compute_stream_ = nullptr;
    
    /**
     * @brief Ensure State On GPU.
     * @param[in,out] state Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool ensureStateOnGPU(PagedOptimizerState& state);
    
    /**
     * @brief Update Parameter CPU.
     * @param[in,out] param Input/output parameter.
     * @param[in,out] state Input/output parameter.
     */
    void updateParameterCPU(Tensor* param, PagedOptimizerState& state);
    
    /**
     * @brief Update Parameter GPU.
     * @param[in,out] param Input/output parameter.
     * @param[in,out] state Input/output parameter.
     */
    void updateParameterGPU(Tensor* param, PagedOptimizerState& state);
};

class PagedOptimizerStateManager {
public:
    /**
     * @brief Paged Optimizer State Manager.
     * @param[in,out] memory_manager Input/output parameter.
     * @return Return value.
     */
    explicit PagedOptimizerStateManager(PagedMemoryManager* memory_manager)
        : memory_manager_(memory_manager) {}
    
    bool ensureOnGPU(PagedOptimizerState& state, void* stream = nullptr);
    
    size_t evictUnused(std::unordered_map<Tensor*, PagedOptimizerState>& states,
                       size_t num_to_evict,
                       void* stream = nullptr);

private:
    PagedMemoryManager* memory_manager_;
};

} // namespace lora
} // namespace llm
} // namespace themis
