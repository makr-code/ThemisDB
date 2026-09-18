/**
 * @file multi_gpu_memory_coordinator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstddef>
#include <memory>
#include <vector>
#include <string>
#include <functional>

namespace themis {
namespace llm {

class MultiGPUMemoryCoordinator {
public:
    enum class DistributionStrategy {
        TENSOR_PARALLEL,    // Split each layer across GPUs
        PIPELINE_PARALLEL,  // Different layers on different GPUs
        HYBRID,            // Combination of tensor and pipeline parallelism
        DATA_PARALLEL      // Replicate model, split batch
    };

    struct GPUDevice {
        int device_id = 0;
        size_t total_vram_bytes = 0;
        size_t available_vram_bytes = 0;
        int compute_capability = 0;
        bool is_healthy = false;
        float temperature_celsius = 0.0f;
        float utilization_percent = 0.0f;
    };

    struct DistributionPlan {
        DistributionStrategy strategy;
        std::vector<int> gpu_ids;
        
        // Tensor parallelism details
        int tensor_parallel_size = 1;
        std::vector<size_t> shard_sizes;  // Per-GPU shard sizes
        
        // Pipeline parallelism details
        int pipeline_parallel_size = 1;
        std::vector<std::vector<int>> layer_assignments;  // Layers per GPU
        
        // Load balancing
        std::vector<int> batch_assignments;  // Batch size per GPU
        
        // Communication topology
        bool enable_p2p = false;
        std::vector<std::pair<int, int>> p2p_pairs;  // GPU pairs for P2P
        
        std::string description;  // Human-readable description
    };

    MultiGPUMemoryCoordinator();
    ~MultiGPUMemoryCoordinator();

    [[nodiscard]] bool initialize(const std::vector<int>& gpu_ids);

    [[nodiscard]] DistributionPlan distributeModelWeights(
        const std::vector<int>& gpu_ids,
        size_t model_size_bytes
    );

    [[nodiscard]] DistributionPlan distributeLayers(
        const std::vector<int>& gpu_ids,
        size_t num_layers,
        size_t layer_size_bytes
    );

    [[nodiscard]] DistributionPlan balanceInferenceLoad(
        const std::vector<int>& gpu_ids,
        size_t total_batch_size
    );

    [[nodiscard]] bool enableP2P(const std::vector<int>& gpu_ids);

    [[nodiscard]] GPUDevice getGPUInfo(int device_id) const;

    [[nodiscard]] std::vector<GPUDevice> getAllGPUs() const;

    [[nodiscard]] int getLeastLoadedGPU() const;

    [[nodiscard]] bool canAccessPeer(int src_gpu, int dst_gpu) const;

    [[nodiscard]] bool transferP2P(
        int src_gpu,
        int dst_gpu,
        const void* src_ptr,
        void* dst_ptr,
        size_t bytes
    );

    /**
     * @brief Synchronize All.
     */
    void synchronizeAll();

    [[nodiscard]] std::vector<std::pair<int, bool>> getHealthStatus() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace llm
} // namespace themis

