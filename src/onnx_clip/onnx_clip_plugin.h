/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            onnx_clip_plugin.h                                 ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-04-06 04:18:18                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     99                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 9ab72c5089  2026-03-12  refactor: flatten plugin hierarchy to src/<name>/ and inc... ║
    • acdb250dbf  2026-03-12  feat: migrate plugins to src/include with CMake switches ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file onnx_clip_plugin.h
 * @brief ONNX Runtime CLIP Plugin for Image Embeddings
 * 
 * Example plugin implementation using ONNX Runtime to run CLIP models
 * for image embedding generation.
 * 
 * Supports:
 * - CLIP ViT-B/32 (base model)
 * - CLIP ViT-L/14 (large model)
 * - Multiple backends: CPU, CUDA, DirectML, TensorRT
 * 
 * @author ThemisDB Team
 * @date December 2025
 */

#pragma once

#include "plugins/image_analysis_interface.h"
#include <memory>
#include <vector>
#include <string>
#include <mutex>

namespace themis {
namespace plugins {
namespace image {

/**
 * @brief ONNX Runtime CLIP Plugin
 * 
 * Example implementation of IImageAnalysisBackend using ONNX Runtime
 * to run CLIP models for image embedding generation.
 * 
 * Thread-Safety: This implementation is thread-safe.
 */
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
    
    // Management
    bool healthCheck() const override;
    nlohmann::json getStatistics() const override;
    void warmup() override;
    
private:
    // Implementation details
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace image
} // namespace plugins
} // namespace themis

// Export plugin entry points (disabled for unit-test binaries that compile
// plugin sources directly).
#ifndef THEMIS_IMAGE_PLUGIN_DISABLE_EXPORT
THEMIS_IMAGE_PLUGIN(themis::plugins::image::ONNXClipPlugin)
#endif
