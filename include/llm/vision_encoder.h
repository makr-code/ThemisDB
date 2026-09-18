/**
 * @file vision_encoder.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class VisionEncoder {
public:
    explicit VisionEncoder(const std::string& clip_model_path, 
                          std::shared_ptr<VisionConfig> config = nullptr,
                          std::shared_ptr<VisionResourceMonitor> resource_monitor = nullptr,
                          int verbosity = 1);
    
    explicit VisionEncoder(const std::string& clip_model_path, int verbosity = 1);
    
    ~VisionEncoder();
    
    // Disable copy construction and assignment
    VisionEncoder(const VisionEncoder&) = delete;
    VisionEncoder& operator=(const VisionEncoder&) = delete;
    
    // Enable move construction and assignment
    VisionEncoder(VisionEncoder&& other) noexcept;
    VisionEncoder& operator=(VisionEncoder&& other) noexcept;
    
    /**
     * @brief Encode Image.
     * @param[in] image_path Path to the image.
     * @return Return value.
     */
    std::vector<float> encodeImage(const std::string& image_path);
    
    /**
     * @brief Encode Image Data.
     * @param[in] image_data Input parameter.
     * @return Return value.
     */
    std::vector<float> encodeImageData(const std::vector<uint8_t>& image_data);
    
    /**
     * @brief Get Embedding Dimension.
     * @return Return value.
     */
    int getEmbeddingDimension() const;
    
    /**
     * @brief Get Num Patches.
     * @return Return value.
     */
    int getNumPatches() const;
    
    /**
     * @brief Get Total Embedding Size.
     * @return Return value.
     */
    size_t getTotalEmbeddingSize() const;
    
    /**
     * @brief Is Ready.
     * @return True when the operation succeeds.
     */
    bool isReady() const;
    
    /**
     * @brief Get Model Info.
     * @return Return value.
     */
    std::string getModelInfo() const;
    
    /**
     * @brief Get Model License.
     * @return Return value.
     */
    std::shared_ptr<ModelLicense> getModelLicense() const;
    
    /**
     * @brief Validate Image.
     * @param[in] image_path Path to the image.
     * @return True when the operation succeeds.
     */
    bool validateImage(const std::string& image_path) const;
    
    /**
     * @brief Set User Context.
     * @param[in] user_id Identifier of the user.
     */
    void setUserContext(const std::string& user_id);
    
private:
    /**
     * @brief Load Image.
     * @param[in] image_path Path to the image.
     * @return Pointer to the result.
     */
    clip_image_u8* loadImage(const std::string& image_path);
    
    /**
     * @brief Preprocess Image.
     * @param[in] img_u8 Input parameter.
     * @return Pointer to the result.
     */
    clip_image_f32* preprocessImage(const clip_image_u8* img_u8);
    
    /**
     * @brief Free Image.
     * @param[in,out] img_u8 Input/output parameter.
     */
    void freeImage(clip_image_u8* img_u8);
    /**
     * @brief Free Image.
     * @param[in,out] img_f32 Input/output parameter.
     */
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
    /**
     * @brief Validate Image Size.
     * @param[in] image_path Path to the image.
     * @return True when the operation succeeds.
     */
    bool validateImageSize(const std::string& image_path) const;
    /**
     * @brief Validate Image Format.
     * @param[in] image_path Path to the image.
     * @return True when the operation succeeds.
     */
    bool validateImageFormat(const std::string& image_path) const;
    /**
     * @brief Validate Image Resolution.
     * @param[in] image_path Path to the image.
     * @return True when the operation succeeds.
     */
    bool validateImageResolution(const std::string& image_path) const;
};

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

struct VisionResponse {
    /**
     * @brief Vision Response.
     * @return Return value.
     */
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

