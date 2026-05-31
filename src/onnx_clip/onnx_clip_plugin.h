/*
 * ThemisDB | File: onnx_clip_plugin.h | Version: 0.0.13 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 94/100 | Lines: 106
 * Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #5060 Refresh onnx_clip module do... (2026-05-13) | #4827 refactor: flatten plugin/ h... (2026-05-04) | #4111 feat: migrate plugins to fl... (2026-03-12)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
#include <functional>
#include <memory>
#include <vector>
#include <string>
#include <mutex>
#include <cstdint>

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

    EmbeddingResult generateTextEmbedding(const std::string& text) override;
    
    // Management
    bool healthCheck() const override;
    nlohmann::json getStatistics() const override;
    void warmup() override;

    // -----------------------------------------------------------------------
    // Injectable model-hash bridge (STUB #94)
    // -----------------------------------------------------------------------
    /// Callback type: given a file path, returns its SHA-256 hex digest (or
    /// empty string on I/O error).  Used as a fallback when THEMIS_HAS_OPENSSL
    /// is not defined so that callers can inject a hash implementation without
    /// rebuilding with OpenSSL.
    using ModelHashFn = std::function<std::string(const std::string& file_path)>;

    /// Register a hash function used by `initialize()` when OpenSSL is absent.
    /// Pass an empty `std::function` to clear any previously registered function
    /// and revert to the skip-check behaviour.
    /// Thread-safe (guarded by a static mutex).
    static void setModelHashFn(ModelHashFn fn);

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
