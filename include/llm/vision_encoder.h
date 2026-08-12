/**
 * @file vision_encoder.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include "llm/vision_config.h"
#include "llm/vision_resource_monitor.h"

// Forward declarations for llama.cpp CLIP types
struct clip_ctx;
struct clip_image_u8;
struct clip_image_f32;

namespace themis {
namespace llm {

/**
 * @brief Vision Encoder using CLIP for multi-modal LLM support
 * 
 * This class provides image encoding capabilities using CLIP (Contrastive Language-Image Pre-training)
 * models for integration with vision-language models like LLaVA.
 * 
 * Features:
 * - Load CLIP vision encoder models (GGUF format)
 * - Encode images to embedding vectors
 * - Image preprocessing (resize, normalize)
 * - Support for multiple image formats (JPEG, PNG, etc.)
 * - GPU acceleration support
 * 
 * Usage:
 * @code
 * auto config = VisionConfig::loadFromFile("config/vision_config.yaml");
 * VisionEncoder encoder("/models/mmproj-model-f16.gguf", config);
 * auto embeddings = encoder.encodeImage("/path/to/image.jpg");
 * @endcode
 * 
 * Based on llama.cpp CLIP integration:
 * https://github.com/ggerganov/llama.cpp/tree/master/examples/llava
 * 
 * Enhanced with:
 * - Configuration-driven validation and limits
 * - Resource monitoring and tracking
 * - License compliance checking
 * - Production-ready error handling
 */
class VisionEncoder {
public:
    /**
     * @brief Construct a VisionEncoder with configuration
     * 
     * @param clip_model_path Path to CLIP model file (GGUF format)
     * @param config Vision configuration (optional, uses defaults if null)
     * @param resource_monitor Resource monitor (optional, for tracking)
     * @param verbosity Logging verbosity (0=silent, 1=normal, 2=verbose)
     * @throws std::runtime_error if model fails to load or validation fails
     */
    explicit VisionEncoder(const std::string& clip_model_path, 
                          std::shared_ptr<VisionConfig> config = nullptr,
                          std::shared_ptr<VisionResourceMonitor> resource_monitor = nullptr,
                          int verbosity = 1);
    
    /**
     * @brief Construct a VisionEncoder (legacy, for backward compatibility)
     * 
     * @param clip_model_path Path to CLIP model file (GGUF format)
     * @param verbosity Logging verbosity (0=silent, 1=normal, 2=verbose)
     * @throws std::runtime_error if model fails to load
     * @deprecated Use constructor with VisionConfig instead
     */
    explicit VisionEncoder(const std::string& clip_model_path, int verbosity = 1);
    
    /**
     * @brief Destructor - releases CLIP model resources
     */
    ~VisionEncoder();
    
    // Disable copy construction and assignment
    VisionEncoder(const VisionEncoder&) = delete;
    VisionEncoder& operator=(const VisionEncoder&) = delete;
    
    // Enable move construction and assignment
    VisionEncoder(VisionEncoder&& other) noexcept;
    VisionEncoder& operator=(VisionEncoder&& other) noexcept;
    
    /**
     * @brief Encode an image file to embedding vector
     * 
     * Loads the image, preprocesses it (resize, normalize), and encodes
     * it using the CLIP vision encoder.
     * 
     * @param image_path Path to image file (JPEG, PNG, BMP, etc.)
     * @return Embedding vector (size depends on model: num_patches × embedding_dim or just embedding_dim)
     * @throws std::runtime_error if image loading or encoding fails
     */
    std::vector<float> encodeImage(const std::string& image_path);
    
    /**
     * @brief Encode image data from memory
     * 
     * @param image_data Raw image bytes (JPEG, PNG, etc.)
     * @return Embedding vector
     * @throws std::runtime_error if encoding fails
     */
    std::vector<float> encodeImageData(const std::vector<uint8_t>& image_data);
    
    /**
     * @brief Get the embedding dimension
     * 
     * @return Dimension of the output embedding vector
     */
    int getEmbeddingDimension() const;
    
    /**
     * @brief Get the number of image patches
     * 
     * For CLIP ViT models, images are divided into patches.
     * Common values: 576 (24×24), 256 (16×16), depending on model architecture.
     * 
     * @return Number of image patches (model-dependent)
     */
    int getNumPatches() const;
    
    /**
     * @brief Get the total embedding size
     * 
     * @return Total size = num_patches × embedding_dimension
     */
    size_t getTotalEmbeddingSize() const;
    
    /**
     * @brief Check if encoder is ready
     * 
     * @return true if model is loaded and ready for inference
     */
    bool isReady() const;
    
    /**
     * @brief Get model information
     * 
     * @return Model name/description
     */
    std::string getModelInfo() const;
    
    /**
     * @brief Get model license information
     * 
     * @return Model license (if available)
     */
    std::shared_ptr<ModelLicense> getModelLicense() const;
    
    /**
     * @brief Validate image against security constraints
     * 
     * @param image_path Path to image file
     * @return true if valid, false otherwise
     */
    bool validateImage(const std::string& image_path) const;
    
    /**
     * @brief Set user context for resource tracking
     */
    void setUserContext(const std::string& user_id);
    
private:
    /**
     * @brief Load image from file path
     * 
     * @param image_path Path to image file
     * @return Image data structure
     * @throws std::runtime_error if loading fails
     */
    clip_image_u8* loadImage(const std::string& image_path);
    
    /**
     * @brief Preprocess image (resize, normalize)
     * 
     * @param img_u8 Input image (uint8)
     * @return Preprocessed image (float32)
     * @throws std::runtime_error if preprocessing fails
     */
    clip_image_f32* preprocessImage(const clip_image_u8* img_u8);
    
    /**
     * @brief Free image resources
     */
    void freeImage(clip_image_u8* img_u8);
    void freeImage(clip_image_f32* img_f32);
    
private:
    // Model state
    clip_ctx* clip_ctx_;           ///< CLIP context (opaque pointer)
    std::string model_path_;       ///< Path to CLIP model file
    std::string model_id_;         ///< Model identifier for tracking
    int verbosity_ = 0;                ///< Logging verbosity
    bool initialized_ = false;             ///< Initialization status
    
    // Configuration and monitoring
    std::shared_ptr<VisionConfig> config_;                    ///< Vision configuration
    std::shared_ptr<VisionResourceMonitor> resource_monitor_; ///< Resource monitor
    std::string current_user_id_;                             ///< Current user context
    
    // Validation helpers
    bool validateImageSize(const std::string& image_path) const;
    bool validateImageFormat(const std::string& image_path) const;
    bool validateImageResolution(const std::string& image_path) const;
};

/**
 * @brief Vision Request Parameters
 * 
 * Parameters for vision-enabled LLM inference
 */
struct VisionRequest {
    std::string text_prompt;           ///< Text prompt/question
    std::string image_path;            ///< Path to single image
    std::vector<std::string> image_paths;  ///< Paths to multiple images
    
    // Generation parameters
    int max_tokens = 256;              ///< Maximum tokens to generate
    float temperature = 0.7f;          ///< Sampling temperature
    float top_p = 0.9f;                ///< Nucleus sampling parameter
    int top_k = 40;                    ///< Top-k sampling parameter
    
    // Multi-modal specific
    bool use_image_start_end = true;   ///< Add <image> tokens
    std::string image_token = "<image>"; ///< Special image token
};

/**
 * @brief Vision Response
 * 
 * Response from vision-enabled LLM
 */
struct VisionResponse {
    virtual ~VisionResponse() = default;
    bool success = false;              ///< Success flag
    std::string text;                  ///< Generated text
    std::string error_message;         ///< Error message if failed
    
    // Statistics
    int tokens_generated = 0;          ///< Number of tokens generated
    int64_t inference_time_ms = 0;     ///< Inference time in milliseconds
    int64_t image_encoding_time_ms = 0; ///< Image encoding time
    
    std::string model_name;            ///< Model used
};

} // namespace llm
} // namespace themis

