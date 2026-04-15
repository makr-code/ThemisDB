/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            image_analysis_interface.h                         ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:11:57                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     598                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file image_analysis_interface.h
 * @brief Image Analysis Plugin Interface for ThemisDB
 * 
 * Defines the interface for image analysis AI backends (CLIP, Stable Diffusion, etc.)
 * that can be loaded as DLLs/shared libraries at runtime.
 * 
 * This interface enables parallel execution of image analysis alongside LLM operations,
 * providing a unified API for:
 * - Image embedding generation (CLIP, etc.)
 * - Image captioning and description
 * - Object detection and segmentation
 * - Image generation (Stable Diffusion)
 * - Visual question answering (LLaVA, etc.)
 * 
 * @author ThemisDB Team
 * @date December 2025
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <cstdint>
#include <nlohmann/json.hpp>

// Export macros for cross-platform DLL/SO support
#ifdef _WIN32
    #ifdef THEMIS_PLUGIN_EXPORTS
        #define THEMIS_IMAGE_PLUGIN_API __declspec(dllexport)
    #else
        #define THEMIS_IMAGE_PLUGIN_API __declspec(dllimport)
    #endif
#else
    #define THEMIS_IMAGE_PLUGIN_API __attribute__((visibility("default")))
#endif

// Plugin API version - must match between core and plugins
#define THEMIS_IMAGE_PLUGIN_API_VERSION "1.0.0"

namespace themis {
namespace plugins {
namespace image {

using json = nlohmann::json;

// ============================================================================
// Data Structures
// ============================================================================

/**
 * @brief Image Format
 */
enum class ImageFormat {
    UNKNOWN,
    JPEG,
    PNG,
    BMP,
    TIFF,
    WEBP,
    GIF,
    SVG,
    RAW         // Camera RAW formats
};

/**
 * @brief Backend Type
 */
enum class BackendType {
    CPU,           // CPU-only execution
    CUDA,          // NVIDIA CUDA
    DIRECTML,      // DirectX 12 ML (Windows)
    OPENCL,        // OpenCL
    VULKAN,        // Vulkan compute
    TENSORRT,      // NVIDIA TensorRT
    OPENVINO,      // Intel OpenVINO
    METAL,         // Apple Metal
    ROCM,          // AMD ROCm
    AUTO           // Auto-detect best available
};

/**
 * @brief Image Metadata
 */
struct ImageMetadata {
    int width = 0;
    int height = 0;
    int channels = 0;           // 1=grayscale, 3=RGB, 4=RGBA
    ImageFormat format = ImageFormat::UNKNOWN;
    int bits_per_channel = 8;
    
    // EXIF metadata (optional)
    json exif;
    
    // Color space
    std::string color_space = "sRGB";  // sRGB, Adobe RGB, etc.
};

/**
 * @brief Image Embedding Result
 */
struct EmbeddingResult {
    bool success = false;
    std::string error_message;
    
    std::vector<float> embedding;      // Embedding vector
    int dimension = 0;                 // Embedding dimension
    std::string model_name;            // Model used (e.g., "clip-vit-base-patch32")
    
    int64_t inference_time_ms = 0;     // Processing time
};

/**
 * @brief Image Caption Result
 */
struct CaptionResult {
    bool success = false;
    std::string error_message;
    
    std::string caption;               // Generated caption
    float confidence = 0.0f;           // Confidence score (0-1)
    std::string model_name;            // Model used
    
    // Alternative captions (optional)
    std::vector<std::pair<std::string, float>> alternatives;
    
    int64_t inference_time_ms = 0;
};

/**
 * @brief Object Detection Result
 */
struct DetectionResult {
    bool success = false;
    std::string error_message;
    
    struct BoundingBox {
        float x = 0.0f;                // Top-left x (normalized 0-1)
        float y = 0.0f;                // Top-left y (normalized 0-1)
        float width = 0.0f;            // Width (normalized 0-1)
        float height = 0.0f;           // Height (normalized 0-1)
        std::string label;             // Object class
        float confidence = 0.0f;       // Confidence score (0-1)
    };
    
    std::vector<BoundingBox> detections;
    std::string model_name;
    
    int64_t inference_time_ms = 0;
};

/**
 * @brief Image Segmentation Result
 */
struct SegmentationResult {
    bool success = false;
    std::string error_message;
    
    std::vector<uint8_t> mask;         // Segmentation mask (W x H)
    int width = 0;
    int height = 0;
    
    std::vector<std::string> labels;   // Class labels per segment ID
    std::string model_name;
    
    int64_t inference_time_ms = 0;
};

/**
 * @brief Image Generation Parameters
 */
struct GenerationParams {
    std::string prompt;                // Text prompt
    std::string negative_prompt;       // Negative prompt (what to avoid)
    
    int width = 512;                   // Output width
    int height = 512;                  // Output height
    
    int num_inference_steps = 50;     // Quality vs speed tradeoff
    float guidance_scale = 7.5f;      // How closely to follow prompt (1-20)
    
    int64_t seed = -1;                // Random seed (-1 = random)
    
    // Advanced parameters
    float strength = 0.8f;            // For img2img: how much to transform (0-1)
    std::vector<uint8_t> init_image;  // For img2img: initial image
    
    json extra_params;                // Backend-specific parameters
};

/**
 * @brief Image Generation Result
 */
struct GenerationResult {
    bool success = false;
    std::string error_message;
    
    std::vector<uint8_t> image_data;  // Generated image (PNG format)
    ImageMetadata metadata;
    
    std::string model_name;           // Model used
    int64_t inference_time_ms = 0;
    
    // Generation metadata
    int64_t seed_used = -1;           // Actual seed used
};

/**
 * @brief Plugin Capabilities
 */
struct PluginCapabilities {
    bool supports_embedding = false;
    bool supports_captioning = false;
    bool supports_detection = false;
    bool supports_segmentation = false;
    bool supports_generation = false;
    bool supports_image_to_image = false;
    bool supports_visual_qa = false;
    
    // Performance characteristics
    bool supports_batch_processing = false;
    bool supports_streaming = false;
    bool thread_safe = false;
    
    // Hardware requirements
    std::vector<BackendType> supported_backends;
    size_t min_memory_mb = 2048;      // Minimum RAM/VRAM
    size_t recommended_memory_mb = 8192;
};

/**
 * @brief Plugin Information
 */
struct PluginInfo {
    std::string name;
    std::string version;
    std::string description;
    std::string author;
    std::string license;
    
    // Model information
    std::string model_name;
    std::string model_version;
    std::vector<std::string> supported_formats;
    
    PluginCapabilities capabilities;
};

/**
 * @brief Plugin Configuration
 */
class PluginConfig {
public:
    PluginConfig() = default;
    explicit PluginConfig(const json& settings) : settings_(settings) {}
    
    /**
     * @brief Get configuration value with default
     */
    template<typename T>
    T get(const std::string& path, T default_value) const {
        try {
            std::string fixed_path = path;
            std::replace(fixed_path.begin(), fixed_path.end(), '.', '/');
            json::json_pointer ptr("/" + fixed_path);
            
            if (settings_.contains(ptr)) {
                return settings_.at(ptr).get<T>();
            }
        } catch (...) {}
        return default_value;
    }
    
    /**
     * @brief Check if configuration path exists
     */
    bool has(const std::string& path) const {
        try {
            std::string fixed_path = path;
            std::replace(fixed_path.begin(), fixed_path.end(), '.', '/');
            json::json_pointer ptr("/" + fixed_path);
            return settings_.contains(ptr);
        } catch (...) {
            return false;
        }
    }
    
    /**
     * @brief Get raw JSON settings
     */
    const json& raw() const { return settings_; }
    
private:
    json settings_;
};

// ============================================================================
// Plugin Interface
// ============================================================================

/**
 * @brief Image Analysis Backend Plugin Interface
 * 
 * All image analysis plugins must implement this interface.
 * Plugins are loaded as dynamic libraries (DLL/SO/DYLIB).
 * 
 * Thread-Safety: Implementations should be thread-safe for concurrent calls.
 */
class IImageAnalysisBackend {
public:
    virtual ~IImageAnalysisBackend() = default;
    
    /**
     * @brief Get plugin information
     */
    virtual PluginInfo getInfo() const = 0;
    
    /**
     * @brief Initialize plugin with configuration
     * 
     * Called once when plugin is loaded.
     * Should load models and allocate resources.
     * 
     * @param config Configuration from YAML
     * @param backend Preferred backend (CPU, CUDA, etc.)
     * @return true if initialization successful
     */
    virtual bool initialize(const PluginConfig& config, BackendType backend = BackendType::AUTO) = 0;
    
    /**
     * @brief Shutdown plugin
     * 
     * Called when plugin is unloaded.
     * Clean up resources and deallocate memory.
     */
    virtual void shutdown() = 0;
    
    /**
     * @brief Check if plugin is ready for inference
     */
    virtual bool isReady() const = 0;
    
    /**
     * @brief Get current backend type in use
     */
    virtual BackendType getBackend() const = 0;
    
    // ========================================================================
    // Core Operations
    // ========================================================================
    
    /**
     * @brief Generate embedding vector for an image
     * 
     * Uses models like CLIP to generate semantic embeddings
     * suitable for similarity search.
     * 
     * @param image_data Raw image bytes (JPEG, PNG, etc.)
     * @param metadata Optional image metadata (can be nullptr)
     * @return Embedding result with vector
     */
    virtual EmbeddingResult generateEmbedding(
        const std::vector<uint8_t>& image_data,
        const ImageMetadata* metadata = nullptr
    ) = 0;
    
    /**
     * @brief Generate text caption for an image
     * 
     * Uses models like LLaVA, BLIP-2, etc. to generate
     * natural language descriptions.
     * 
     * @param image_data Raw image bytes
     * @param metadata Optional image metadata
     * @param max_length Maximum caption length
     * @return Caption result with text
     */
    virtual CaptionResult generateCaption(
        const std::vector<uint8_t>& image_data,
        const ImageMetadata* metadata = nullptr,
        int max_length = 50
    ) {
        // Default: not supported
        CaptionResult result;
        result.success = false;
        result.error_message = "Captioning not supported by this plugin";
        return result;
    }
    
    /**
     * @brief Detect objects in an image
     * 
     * Uses models like YOLO, Faster R-CNN, etc. to detect
     * and localize objects.
     * 
     * @param image_data Raw image bytes
     * @param metadata Optional image metadata
     * @param confidence_threshold Minimum confidence (0-1)
     * @return Detection result with bounding boxes
     */
    virtual DetectionResult detectObjects(
        const std::vector<uint8_t>& image_data,
        const ImageMetadata* metadata = nullptr,
        float confidence_threshold = 0.5f
    ) {
        // Default: not supported
        DetectionResult result;
        result.success = false;
        result.error_message = "Object detection not supported by this plugin";
        return result;
    }
    
    /**
     * @brief Segment image into regions
     * 
     * Uses models like Segment Anything, Mask R-CNN, etc.
     * 
     * @param image_data Raw image bytes
     * @param metadata Optional image metadata
     * @return Segmentation result with mask
     */
    virtual SegmentationResult segmentImage(
        const std::vector<uint8_t>& image_data,
        const ImageMetadata* metadata = nullptr
    ) {
        // Default: not supported
        SegmentationResult result;
        result.success = false;
        result.error_message = "Segmentation not supported by this plugin";
        return result;
    }
    
    /**
     * @brief Generate image from text prompt
     * 
     * Uses models like Stable Diffusion, DALL-E, etc.
     * 
     * @param params Generation parameters
     * @return Generation result with image data
     */
    virtual GenerationResult generateImage(const GenerationParams& params) {
        // Default: not supported
        GenerationResult result;
        result.success = false;
        result.error_message = "Image generation not supported by this plugin";
        return result;
    }
    
    /**
     * @brief Answer question about an image
     * 
     * Visual Question Answering (VQA) using models like LLaVA.
     * 
     * @param image_data Raw image bytes
     * @param question Question in natural language
     * @param metadata Optional image metadata
     * @return Answer as text
     */
    virtual std::string answerVisualQuestion(
        const std::vector<uint8_t>& image_data,
        const std::string& question,
        const ImageMetadata* metadata = nullptr
    ) {
        // Default: not supported
        return "Visual question answering not supported by this plugin";
    }
    
    // ========================================================================
    // Batch Operations (Optional)
    // ========================================================================
    
    /**
     * @brief Generate embeddings for multiple images (batch)
     * 
     * More efficient than calling generateEmbedding() multiple times.
     * 
     * @param images Vector of image data
     * @return Vector of embedding results
     */
    virtual std::vector<EmbeddingResult> generateEmbeddingBatch(
        const std::vector<std::vector<uint8_t>>& images
    ) {
        // Default: sequential processing
        std::vector<EmbeddingResult> results;
        results.reserve(images.size());
        for (const auto& img : images) {
            results.push_back(generateEmbedding(img));
        }
        return results;
    }

    /**
     * @brief Generate embedding vector for a text query
     * 
     * Uses the CLIP text encoder to generate a semantic embedding compatible
     * with the image embedding space, enabling cross-modal similarity search.
     * 
     * @param text Natural language query string (max 77 tokens for CLIP)
     * @return Embedding result with float vector in the same space as image embeddings
     */
    virtual EmbeddingResult generateTextEmbedding(const std::string& text) {
        // Default: not supported
        EmbeddingResult result;
        result.success = false;
        result.error_message = "Text embedding not supported by this plugin";
        return result;
    }
    
    // ========================================================================
    // Management
    // ========================================================================
    
    /**
     * @brief Health check
     * 
     * Verify that plugin is operational and model is loaded.
     * 
     * @return true if healthy and ready for inference
     */
    virtual bool healthCheck() const = 0;
    
    /**
     * @brief Get plugin statistics
     * 
     * Returns statistics about plugin usage:
     * - Total inferences
     * - Average inference time
     * - Memory usage
     * - Error count
     */
    virtual json getStatistics() const {
        return json::object();
    }
    
    /**
     * @brief Warm up the model
     * 
     * Run a dummy inference to ensure model is loaded
     * and GPU kernels are compiled.
     */
    virtual void warmup() {}
};

// ============================================================================
// Plugin Entry Points
// ============================================================================

/**
 * @brief Plugin creation function type
 */
using CreateImagePluginFunc = IImageAnalysisBackend* (*)();

/**
 * @brief Plugin destruction function type
 */
using DestroyImagePluginFunc = void (*)(IImageAnalysisBackend*);

/**
 * @brief Plugin version function type
 */
using GetImagePluginVersionFunc = const char* (*)();

/**
 * @brief Plugin Entry Point Macro
 * 
 * Use this macro to export plugin entry points:
 * 
 * @code
 * class MyImagePlugin : public IImageAnalysisBackend {
 *     // ... implementation ...
 * };
 * 
 * THEMIS_IMAGE_PLUGIN(MyImagePlugin)
 * @endcode
 */
#define THEMIS_IMAGE_PLUGIN(PluginClass) \
    extern "C" { \
        THEMIS_IMAGE_PLUGIN_API themis::plugins::image::IImageAnalysisBackend* themis_create_image_plugin() { \
            return new PluginClass(); \
        } \
        THEMIS_IMAGE_PLUGIN_API void themis_destroy_image_plugin(themis::plugins::image::IImageAnalysisBackend* plugin) { \
            delete plugin; \
        } \
        THEMIS_IMAGE_PLUGIN_API const char* themis_get_image_plugin_api_version() { \
            return THEMIS_IMAGE_PLUGIN_API_VERSION; \
        } \
    }

} // namespace image
} // namespace plugins
} // namespace themis
