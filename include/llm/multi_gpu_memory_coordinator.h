/**
 * @file multi_gpu_memory_coordinator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Multi-GPU Memory Coordinator for distributed model execution
 * 
 * Implements tensor parallelism, pipeline parallelism, and load balancing
 * strategies inspired by Megatron-LM (Shoeybi et al., 2019) and DeepSpeed.
 * 
 * Key Features:
 * - Tensor Parallelism: Split model weights across GPUs
 * - Pipeline Parallelism: Distribute layers across GPUs
 * - Dynamic Load Balancing: Balance inference workload
 * - Peer-to-Peer Communication: Enable direct GPU-GPU transfers
 */
class MultiGPUMemoryCoordinator {
public:
    /**
     * @brief Distribution strategy for multi-GPU execution
     */
    enum class DistributionStrategy {
        TENSOR_PARALLEL,    // Split each layer across GPUs
        PIPELINE_PARALLEL,  // Different layers on different GPUs
        HYBRID,            // Combination of tensor and pipeline parallelism
        DATA_PARALLEL      // Replicate model, split batch
    };

    /**
     * @brief GPU device information
     */
    struct GPUDevice {
        int device_id = 0;
        size_t total_vram_bytes = 0;
        size_t available_vram_bytes = 0;
        int compute_capability = 0;
        bool is_healthy = false;
        float temperature_celsius = 0.0f;
        float utilization_percent = 0.0f;
    };

    /**
     * @brief Distribution plan for multi-GPU execution
     */
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

    /**
     * @brief Initialize coordinator with available GPUs
     * 
     * @param gpu_ids List of GPU device IDs to use
     * @return true if initialization succeeded
     */
    [[nodiscard]] bool initialize(const std::vector<int>& gpu_ids);

    /**
     * @brief Distribute model weights using tensor parallelism
     * 
     * Splits each layer across multiple GPUs. Best for large models that
     * don't fit on a single GPU.
     * 
     * @param gpu_ids GPUs to distribute across
     * @param model_size_bytes Total model size
     * @return Distribution plan
     */
    [[nodiscard]] DistributionPlan distributeModelWeights(
        const std::vector<int>& gpu_ids,
        size_t model_size_bytes
    );

    /**
     * @brief Distribute layers using pipeline parallelism
     * 
     * Assigns different layers to different GPUs. Best for models with
     * many layers and moderate layer size.
     * 
     * @param gpu_ids GPUs to distribute across
     * @param num_layers Total number of layers
     * @param layer_size_bytes Size of each layer
     * @return Distribution plan
     */
    [[nodiscard]] DistributionPlan distributeLayers(
        const std::vector<int>& gpu_ids,
        size_t num_layers,
        size_t layer_size_bytes
    );

    /**
     * @brief Balance inference load across GPUs
     * 
     * Dynamically assigns batch elements to GPUs based on current load.
     * 
     * @param gpu_ids GPUs to balance across
     * @param total_batch_size Total batch size
     * @return Distribution plan
     */
    [[nodiscard]] DistributionPlan balanceInferenceLoad(
        const std::vector<int>& gpu_ids,
        size_t total_batch_size
    );

    /**
     * @brief Enable peer-to-peer memory access between GPUs
     * 
     * Enables direct GPU-to-GPU memory transfers without going through CPU.
     * Requires NVLink or PCIe P2P support.
     * 
     * @param gpu_ids GPUs to enable P2P for
     * @return true if P2P enabled successfully
     */
    [[nodiscard]] bool enableP2P(const std::vector<int>& gpu_ids);

    /**
     * @brief Get GPU device information
     * 
     * @param device_id GPU device ID
     * @return Device information
     */
    [[nodiscard]] GPUDevice getGPUInfo(int device_id) const;

    /**
     * @brief Get all available GPUs
     * 
     * @return List of available GPU devices
     */
    [[nodiscard]] std::vector<GPUDevice> getAllGPUs() const;

    /**
     * @brief Get least loaded GPU
     * 
     * @return Device ID of GPU with lowest utilization
     */
    [[nodiscard]] int getLeastLoadedGPU() const;

    /**
     * @brief Check if P2P is available between two GPUs
     * 
     * @param src_gpu Source GPU device ID
     * @param dst_gpu Destination GPU device ID
     * @return true if P2P is available
     */
    [[nodiscard]] bool canAccessPeer(int src_gpu, int dst_gpu) const;

    /**
     * @brief Transfer data between GPUs using P2P
     * 
     * @param src_gpu Source GPU device ID
     * @param dst_gpu Destination GPU device ID
     * @param src_ptr Source pointer (on src_gpu)
     * @param dst_ptr Destination pointer (on dst_gpu)
     * @param bytes Number of bytes to transfer
     * @return true if transfer succeeded
     */
    [[nodiscard]] bool transferP2P(
        int src_gpu,
        int dst_gpu,
        const void* src_ptr,
        void* dst_ptr,
        size_t bytes
    );

    /**
     * @brief Synchronize all GPUs
     * 
     * Ensures all GPU operations are complete before proceeding.
     */
    void synchronizeAll();

    /**
     * @brief Get health status of all GPUs
     * 
     * @return Vector of (device_id, is_healthy) pairs
     */
    [[nodiscard]] std::vector<std::pair<int, bool>> getHealthStatus() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace llm
} // namespace themis

