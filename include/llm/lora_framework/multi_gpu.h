/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            multi_gpu.h                                        ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:45:31                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     124                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "llm/lora_framework/gpu_memory.h"
#include <vector>
#include <memory>
#include <string>

namespace themis {
namespace llm {
namespace lora {

/**
 * @brief Multi-GPU context for distributed training
 * 
 * Manages multiple GPU devices for data-parallel training.
 * Supports NVIDIA CUDA and AMD HIP GPUs.
 */
class MultiGPUContext {
public:
    /**
     * @brief Construct multi-GPU context
     * @param num_gpus Number of GPUs to use (0 = use all available)
     * @param gpu_ids Specific GPU IDs to use (empty = use first N GPUs)
     */
    explicit MultiGPUContext(int num_gpus = 0, const std::vector&lt;int&gt;& gpu_ids = {});
    
    ~MultiGPUContext() = default;
    
    // Disable copy, enable move
    MultiGPUContext(const MultiGPUContext&) = delete;
    MultiGPUContext& operator=(const MultiGPUContext&) = delete;
    MultiGPUContext(MultiGPUContext&&) noexcept = default;
    MultiGPUContext& operator=(MultiGPUContext&&) noexcept = default;
    
    /**
     * @brief Get number of GPUs in context
     */
    int num_gpus() const { return static_cast&lt;int&gt;(devices_.size()); }
    
    /**
     * @brief Get world size (same as num_gpus for data parallelism)
     */
    int world_size() const { return num_gpus(); }
    
    /**
     * @brief Get device by rank
     * @param rank GPU rank (0 to num_gpus-1)
     * @return Device object for the GPU
     */
    Device get_device(int rank) const;
    
    /**
     * @brief Get all devices
     */
    const std::vector<Device>& devices() const { return devices_; }
    
    /**
     * @brief Check if multi-GPU is enabled
     */
    bool is_multi_gpu() const { return num_gpus() > 1; }
    
    /**
     * @brief Get GPU type (CUDA or HIP)
     */
    DeviceType gpu_type() const { return gpu_type_; }
    
    /**
     * @brief Check if all GPUs are the same vendor
     */
    bool is_homogeneous() const { return is_homogeneous_; }
    
    /**
     * @brief Synchronize all GPUs
     */
    void synchronize_all() const;
    
private:
    std::vector<Device> devices_;
    DeviceType gpu_type_;
    bool is_homogeneous_;
    
    void detect_gpus(int num_gpus, const std::vector&lt;int&gt;& gpu_ids);
};

/**
 * @brief GPU topology information for optimized communication
 */
struct GPUTopology {
    int num_gpus = 0;
    bool has_nvlink = false;
    bool has_pcie_p2p = false;
    std::vector<std::vector<float>> bandwidth_matrix;  // GB/s between each GPU pair
    
    /**
     * @brief Detect GPU topology
     */
    static GPUTopology detect(const std::vector<Device>& devices);
};

} // namespace lora
} // namespace llm
} // namespace themis
