/**
 * @file multi_gpu.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "llm/lora_framework/gpu_memory.h"
#include <vector>
#include <memory>
#include <string>

namespace themis {
namespace llm {
namespace lora {

class MultiGPUContext {
public:
    explicit MultiGPUContext(int num_gpus = 0, const std::vector<int>& gpu_ids = {});
    
    ~MultiGPUContext() = default;
    
    // Disable copy, enable move
    MultiGPUContext(const MultiGPUContext&) = delete;
    MultiGPUContext& operator=(const MultiGPUContext&) = delete;
    MultiGPUContext(MultiGPUContext&&) noexcept = default;
    MultiGPUContext& operator=(MultiGPUContext&&) noexcept = default;
    
    int num_gpus() const { return static_cast<int>(devices_.size()); }
    
    int world_size() const { return num_gpus(); }
    
    /**
     * @brief Get device.
     * @param[in] rank Input parameter.
     * @return Return value.
     */
    Device get_device(int rank) const;
    
    const std::vector<Device>& devices() const { return devices_; }
    
    bool is_multi_gpu() const { return num_gpus() > 1; }
    
    DeviceType gpu_type() const { return gpu_type_; }
    
    bool is_homogeneous() const { return is_homogeneous_; }
    
    /**
     * @brief Synchronize all.
     */
    void synchronize_all() const;
    
private:
    std::vector<Device> devices_;
    DeviceType gpu_type_;
    bool is_homogeneous_ = false;
    
    /**
     * @brief Detect gpus.
     * @param[in] num_gpus Input parameter.
     * @param[in] gpu_ids Input parameter.
     */
    void detect_gpus(int num_gpus, const std::vector<int>& gpu_ids);
};

struct GPUTopology {
    /**
     * @brief GPUTopology.
     * @return Return value.
     */
    virtual ~GPUTopology() = default;
    int num_gpus = 0;
    bool has_nvlink = false;
    bool has_pcie_p2p = false;
    std::vector<std::vector<float>> bandwidth_matrix;  // GB/s between each GPU pair
    
    /**
     * @brief Detect.
     * @param[in] devices Input parameter.
     * @return Return value.
     */
    static GPUTopology detect(const std::vector<Device>& devices);
};

} // namespace lora
} // namespace llm
} // namespace themis

