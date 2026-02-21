/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            onnx_clip_plugin.h                                 ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:36:06                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   99.0/100                                       ║
    • Total Lines:     94                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 6d452aab7  2025-12-20  Add image analysis plugin architecture and license compat... ║
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

// Export plugin entry points
THEMIS_IMAGE_PLUGIN(themis::plugins::image::ONNXClipPlugin)
