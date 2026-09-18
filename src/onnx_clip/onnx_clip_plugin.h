/**
 * @file onnx_clip_plugin.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "plugins/image_analysis_interface.h"
#include <functional>
#include <memory>
#include <vector>
#include <string>
#include <mutex>
#include <cstdint>
#include <atomic>
#include <condition_variable>

namespace themis {
namespace plugins {
namespace image {

class ONNXClipPlugin : public IImageAnalysisBackend {
public:
    ONNXClipPlugin();
    ~ONNXClipPlugin() override;
    
    // Plugin Interface Implementation
    PluginInfo getInfo() const override;
    bool initialize(const PluginConfig& config, BackendType backend = BackendType::AUTO) override;
    void shutdown() override;
    bool isReady() const override;
    BackendType getBackend() const override;
    
    // Core Operations
    EmbeddingResult generateEmbedding(
        const std::vector<uint8_t>& image_data,
        const ImageMetadata* metadata = nullptr
    ) override;
    
    std::vector<EmbeddingResult> generateEmbeddingBatch(
        const std::vector<std::vector<uint8_t>>& images
    ) override;

    EmbeddingResult generateTextEmbedding(const std::string& text) override;
    
    // Management
    bool healthCheck() const override;
    nlohmann::json getStatistics() const override;
    void warmup() override;

    /**
     * @brief ----------------------------------------------------------------------- Hot-Swap Model Reloading (Phase 3B) -----------------------------------------------------------------------
     * @param[in] new_config Input parameter.
     * @return True when the operation succeeds.
     */
    bool reloadModel(const PluginConfig& new_config);

    // -----------------------------------------------------------------------
    // Injectable model-hash bridge (STUB #94)
    // -----------------------------------------------------------------------
    using ModelHashFn = std::function<std::string(const std::string& file_path)>;

    /**
     * @brief Set Model Hash Fn.
     * @param[in] fn Input parameter.
     */
    static void setModelHashFn(ModelHashFn fn);

public:
    // Implementation details (forward-declared as public so the out-of-class
    // definition in the .cpp may legally provide the concrete type).
    struct Impl;
private:
    mutable std::mutex impl_swap_mtx_;
    std::shared_ptr<Impl> impl_;
};

} // namespace image
} // namespace plugins
} // namespace themis

// Plugin entry points are defined in the implementation unit to avoid
// duplicate/dllimport definitions when this header is included by consumers.
