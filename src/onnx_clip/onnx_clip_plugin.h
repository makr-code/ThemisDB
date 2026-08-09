/**
 * @file onnx_clip_plugin.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief ONNX Runtime CLIP Plugin
 * 
 * Example implementation of IImageAnalysisBackend using ONNX Runtime
 * to run CLIP models for image embedding generation.
 * 
 * Thread-Safety: This implementation is thread-safe.
 * 
 * ## Memory-Mapped Model Loading (Phase 4B)
 * 
 * Supported config keys:
 * - `enable_mmap_loading` (boolean, default: false)
 *   - When true: attempts to load large ONNX models via memory mapping
 *     to reduce peak memory usage by up to 30% for large models.
 *   - Platform-specific behavior:
 *     - **Linux**: Uses mmap(2) with MAP_SHARED | MAP_NORESERVE for read-only mapping
 *     - **Windows**: Uses CreateFileMapping() + MapViewOfFile() for efficient model paging
 *   - Fallback: If mmap fails or is unsupported, silently falls back to traditional
 *     file-based loading without error.
 *   - Exception-safe: mmap resources are cleaned up via RAII (destructor/shutdown)
 * 
 * ## Performance Notes
 * 
 * Memory-mapped loading provides:
 * - Reduced peak memory footprint (file pages loaded on-demand)
 * - Potential page sharing across processes (if model is mmap'd by multiple clients)
 * - Transparent OS paging: no explicit paging code required
 * - Trade-off: Minor latency overhead on first-pass model access (page faults)
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
    // Hot-Swap Model Reloading (Phase 3B)
    // -----------------------------------------------------------------------
    /// @brief Reload model configuration without server restart
    /// 
    /// Implements dynamic model reloading with the following guarantees:
    /// - Thread-safe: concurrent embedding requests are properly handled
    /// - In-flight request handling: waits up to 30 seconds for current requests
    ///   to complete before swapping models
    /// - Rollback capability: if initialization fails, original model remains active
    /// - No request interruption: in-flight requests use old model until atomic swap
    /// 
    /// @param new_config New PluginConfig for model reloading
    /// @return true if reload successful; false if not initialized, config invalid,
    ///         or 30-second drain timeout exceeded
    /// 
    /// @note Thread-Safety: This method is fully thread-safe and can be called
    ///       concurrently with embedding operations. Uses unique_lock to coordinate
    ///       with in-flight request counter.
    /// 
    /// @note In-Flight Request Timeout: If requests do not drain within 30 seconds,
    ///       the reload is cancelled and false is returned. This is a safety mechanism
    ///       to prevent indefinite blocking during model swap.
    bool reloadModel(const PluginConfig& new_config);

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
