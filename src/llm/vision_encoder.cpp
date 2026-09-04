/**
 * @file vision_encoder.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 80/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=10, H=3, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/vision_encoder.h"
#include <stdexcept>
#include "themis/module_hash_verifier.h"
#include "utils/logger.h"
#include <filesystem>
#include <fstream>
#include <chrono>

// Include llama.cpp CLIP API
// Note: This assumes llama.cpp with CLIP support is available
// The actual API may need adjustment based on llama.cpp version
#ifdef THEMIS_ENABLE_LLM
extern "C" {
    // Forward declarations for CLIP API from llama.cpp
    // These would normally be in clip.h from llama.cpp
    struct clip_ctx;
    struct clip_image_u8 {
        int nx;
        int ny;
        uint8_t* data;
    };
    struct clip_image_f32 {
        int nx;
        int ny;
        float* data;
    };
    
    // CLIP API functions (from llama.cpp examples/llava)
    clip_ctx* clip_model_load(const char* fname, int verbosity);
    void clip_free(clip_ctx* ctx);
    
    clip_image_u8* clip_image_u8_init();
    void clip_image_u8_free(clip_image_u8* img);
    
    clip_image_f32* clip_image_f32_init();
    void clip_image_f32_free(clip_image_f32* img);
    
    bool clip_image_load_from_file(const char* fname, clip_image_u8* img);
    bool clip_image_preprocess(clip_ctx* ctx, const clip_image_u8* img, clip_image_f32* res);
    bool clip_image_encode(clip_ctx* ctx, int n_threads, clip_image_f32* img, float* vec);
    
    int clip_n_mmproj_embd(clip_ctx* ctx);
    int clip_n_patches(clip_ctx* ctx);
    int clip_is_minicpmv(clip_ctx* ctx);
}
#endif

namespace themis {
namespace llm {

// Enhanced constructor with configuration
VisionEncoder::VisionEncoder(const std::string& clip_model_path,
                            std::shared_ptr<VisionConfig> config,
                            std::shared_ptr<VisionResourceMonitor> resource_monitor,
                            int verbosity)
    : clip_ctx_(nullptr)
    , model_path_(clip_model_path)
    , model_id_(std::filesystem::path(clip_model_path).filename().string())
    , verbosity_(verbosity)
    , initialized_(false)
    , config_(config ? config : VisionConfig::getDefault())
    , resource_monitor_(resource_monitor)
{
#ifdef THEMIS_ENABLE_LLM
    // Check API stability
    if (config_->getAPIStability() == VisionAPIStability::EXPERIMENTAL) {
        spdlog::warn("Vision API is in EXPERIMENTAL mode - features may change");
    } else if (config_->getAPIStability() == VisionAPIStability::DEPRECATED) {
        spdlog::warn("Vision API is DEPRECATED and will be removed in future versions");
    }
    
    // Validate license
    if (config_->isLicenseEnforced()) {
        auto license = config_->getModelLicense(model_id_);
        if (!license) {
            spdlog::warn("No license information found for model: {}", model_id_);
        } else if (!config_->validateModelUsage(model_id_, true)) {
            throw std::runtime_error("Model license does not permit usage: " + model_id_);
        }
    }
    
    // Check if model file exists
    if (!std::filesystem::exists(clip_model_path)) {
        throw std::runtime_error("CLIP model file not found: " + clip_model_path);
    }
    
    // Validate model file size
    auto file_size = std::filesystem::file_size(clip_model_path);
    auto file_size_mb = file_size / (1024 * 1024);
    
    const auto& limits = config_->getResourceLimits();
    if (file_size_mb > limits.max_vram_per_model_mb) {
        throw std::runtime_error("Model file size (" + std::to_string(file_size_mb) + 
                               "MB) exceeds limit (" + std::to_string(limits.max_vram_per_model_mb) + "MB)");
    }
    
    // Check if model verification is enabled
    if (config_->isModelVerificationEnabled()) {
        const auto& mv = config_->getSecurityConfig().model_verification;
        if (mv.verify_checksums) {
            // Convention: expected digest in a sidecar file "<model_path>.sha256".
            // Each sidecar contains exactly one hex-encoded SHA-256 line.
            const std::string sidecar_path = clip_model_path + ".sha256";
            if (std::filesystem::exists(sidecar_path)) {
                std::ifstream sidecar(sidecar_path);
                std::string expected_hash;
                if (sidecar >> expected_hash && !expected_hash.empty()) {
                    const std::string actual_hash =
                        themis::modules::ModuleHashVerifier::computeSHA256(clip_model_path);
                    if (actual_hash.empty()) {
                        throw std::runtime_error(
                            "VisionEncoder: failed to compute SHA-256 for model file: " +
                            clip_model_path);
                    }
                    if (actual_hash != expected_hash) {
                        throw std::runtime_error(
                            "VisionEncoder: model integrity check failed for '" +
                            clip_model_path +
                            "'. Expected SHA-256: " + expected_hash +
                            ", actual: " + actual_hash);
                    }
                    spdlog::info("VisionEncoder: model integrity verified (SHA-256 OK): {}",
                                 clip_model_path);
                } else {
                    spdlog::warn(
                        "VisionEncoder: sidecar '{}' is empty or unreadable — "
                        "skipping checksum verification",
                        sidecar_path);
                }
            } else {
                spdlog::warn(
                    "VisionEncoder: checksum verification requested but no sidecar '{}' "
                    "found — skipping checksum verification",
                    sidecar_path);
            }
        } else {
            spdlog::info("Model verification enabled - checksum verification not requested");
        }
    }
    
    // Load CLIP model
    spdlog::info("Loading CLIP model from {}", clip_model_path);
    clip_ctx_ = clip_model_load(clip_model_path.c_str(), verbosity);
    if (!clip_ctx_) {
        throw std::runtime_error("Failed to load CLIP model: " + clip_model_path);
    }
    
    initialized_ = true;
    
    // Register model with resource monitor
    if (resource_monitor_) {
        resource_monitor_->registerModelLoad(model_id_, file_size_mb, file_size_mb);
    }
    
    if (verbosity > 0) {
        spdlog::info("VisionEncoder: Loaded CLIP model from {}", clip_model_path);
        spdlog::info("  - Model ID: {}", model_id_);
        spdlog::info("  - Embedding dimension: {}", getEmbeddingDimension());
        spdlog::info("  - Number of patches: {}", getNumPatches());
        spdlog::info("  - Total embedding size: {}", getTotalEmbeddingSize());
        spdlog::info("  - API Version: {}", config_->getAPIVersion());
        spdlog::info("  - Stability: {}", static_cast<int>(config_->getAPIStability()));
        
        auto license = config_->getModelLicense(model_id_);
        if (license) {
            spdlog::info("  - License: {}", license->license_name);
        }
    }
#else
    throw std::runtime_error("VisionEncoder: LLM support not enabled (THEMIS_ENABLE_LLM=OFF)");
#endif
}

// Legacy constructor for backward compatibility
VisionEncoder::VisionEncoder(const std::string& clip_model_path, int verbosity)
    : VisionEncoder(clip_model_path, nullptr, nullptr, verbosity)
{
    spdlog::debug("Using legacy VisionEncoder constructor - consider upgrading to new API");
}

VisionEncoder::~VisionEncoder() {
#ifdef THEMIS_ENABLE_LLM
    if (clip_ctx_) {
        // Unregister model from resource monitor
        if (resource_monitor_) {
            resource_monitor_->registerModelUnload(model_id_);
        }
        
        clip_free(clip_ctx_);
        clip_ctx_ = nullptr;
    }
#endif
    initialized_ = false;
}

VisionEncoder::VisionEncoder(VisionEncoder&& other) noexcept
    : clip_ctx_(other.clip_ctx_)
    , model_path_(std::move(other.model_path_))
    , verbosity_(other.verbosity_)
    , initialized_(other.initialized_)
{
    other.clip_ctx_ = nullptr;
    other.initialized_ = false;
}

VisionEncoder& VisionEncoder::operator=(VisionEncoder&& other) noexcept {
    if (this != &other) {
#ifdef THEMIS_ENABLE_LLM
        if (clip_ctx_) {
            clip_free(clip_ctx_);
        }
#endif
        
        clip_ctx_ = other.clip_ctx_;
        model_path_ = std::move(other.model_path_);
        verbosity_ = other.verbosity_;
        initialized_ = other.initialized_;
        
        other.clip_ctx_ = nullptr;
        other.initialized_ = false;
    }
    return *this;
}

std::vector<float> VisionEncoder::encodeImage(const std::string& image_path) {
#ifdef THEMIS_ENABLE_LLM
    if (!initialized_ || !clip_ctx_) {
        throw std::runtime_error("VisionEncoder not initialized");
    }
    
    // Validate image
    if (!validateImage(image_path)) {
        throw std::runtime_error("Image validation failed: " + image_path);
    }
    
    auto start_time = std::chrono::steady_clock::now();
    uint64_t request_id = 0;
    
    // Track request with resource monitor
    if (resource_monitor_) {
        if (!resource_monitor_->canAcceptRequest(current_user_id_, 0)) {
            throw std::runtime_error("Request rejected: resource limits exceeded");
        }
        request_id = resource_monitor_->startRequest(current_user_id_, model_id_);
    }
    
    try {
        // Load image
        clip_image_u8* img_u8 = clip_image_u8_init();
        if (!img_u8) {
            throw std::runtime_error("Failed to initialize image structure");
        }
        
        if (!clip_image_load_from_file(image_path.c_str(), img_u8)) {
            clip_image_u8_free(img_u8);
            throw std::runtime_error("Failed to load image: " + image_path);
        }
        
        // Preprocess image
        clip_image_f32* img_f32 = clip_image_f32_init();
        if (!img_f32) {
            clip_image_u8_free(img_u8);
            throw std::runtime_error("Failed to initialize preprocessed image structure");
        }
        
        if (!clip_image_preprocess(clip_ctx_, img_u8, img_f32)) {
            clip_image_f32_free(img_f32);
            clip_image_u8_free(img_u8);
            throw std::runtime_error("Failed to preprocess image");
        }
        
        // Encode image to embeddings
        size_t embedding_size = getTotalEmbeddingSize();
        std::vector<float> embeddings(embedding_size, 0.0f);
        
        // Use the configured number of CPU threads for image encoding
        int n_threads = config_->getResourceLimits().cpu_inference_threads;
        if (!clip_image_encode(clip_ctx_, n_threads, img_f32, embeddings.data())) {
            clip_image_f32_free(img_f32);
            clip_image_u8_free(img_u8);
            throw std::runtime_error("Failed to encode image");
        }
        
        // Cleanup
        clip_image_f32_free(img_f32);
        clip_image_u8_free(img_u8);
        
        // Calculate inference time
        auto end_time = std::chrono::steady_clock::now();
        auto inference_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        // Complete request tracking
        if (resource_monitor_ && request_id > 0) {
            resource_monitor_->completeRequest(request_id, true, inference_time, 0);
        }
        
        if (verbosity_ > 1) {
            spdlog::debug("VisionEncoder: Encoded image {} ({} floats) in {}ms", 
                         image_path, embeddings.size(), inference_time.count());
        }
        
        return embeddings;
        
    } catch (const std::exception& e) {
        // Complete request as failed
        if (resource_monitor_ && request_id > 0) {
            auto end_time = std::chrono::steady_clock::now();
            auto inference_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            resource_monitor_->completeRequest(request_id, false, inference_time, 0);
        }
        throw; // Re-throw
    }
#else
    throw std::runtime_error("VisionEncoder: LLM support not enabled (THEMIS_ENABLE_LLM=OFF)");
#endif
}

std::vector<float> VisionEncoder::encodeImageData(const std::vector<uint8_t>& image_data) {
#ifdef THEMIS_ENABLE_LLM
    // For now, we need to write to a temporary file
    // Future enhancement: Support in-memory image loading
    auto temp_dir = std::filesystem::temp_directory_path();
    
    // Generate unique temporary filename to avoid collisions in concurrent scenarios
    auto timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    std::string temp_filename = "themis_temp_image_" + std::to_string(timestamp) + ".jpg";
    std::string temp_path = (temp_dir / temp_filename).string();
    
    {
        std::ofstream ofs(temp_path, std::ios::binary);
        if (!ofs) {
            throw std::runtime_error("Failed to create temporary image file");
        }
        ofs.write(reinterpret_cast<const char*>(image_data.data()), image_data.size());
    }
    
    try {
        auto embeddings = encodeImage(temp_path);
        std::filesystem::remove(temp_path);
        return embeddings;
    } catch (...) {
        std::filesystem::remove(temp_path);
        throw;
    }
#else
    throw std::runtime_error("VisionEncoder: LLM support not enabled (THEMIS_ENABLE_LLM=OFF)");
#endif
}

int VisionEncoder::getEmbeddingDimension() const {
#ifdef THEMIS_ENABLE_LLM
    if (!clip_ctx_) {
        return 0;
    }
    return clip_n_mmproj_embd(clip_ctx_);
#else
    return 0;
#endif
}

int VisionEncoder::getNumPatches() const {
#ifdef THEMIS_ENABLE_LLM
    if (!clip_ctx_) {
        return 0;
    }
    return clip_n_patches(clip_ctx_);
#else
    return 0;
#endif
}

size_t VisionEncoder::getTotalEmbeddingSize() const {
    int patches = getNumPatches();
    int dim = getEmbeddingDimension();
    
    // For most CLIP models: patches × dimension
    // But some models may output just dimension without patches
    if (patches > 0 && dim > 0) {
        return static_cast<size_t>(patches) * static_cast<size_t>(dim);
    } else if (dim > 0) {
        return static_cast<size_t>(dim);
    }
    
    return 0;
}

bool VisionEncoder::isReady() const {
    return initialized_ && clip_ctx_ != nullptr;
}

std::string VisionEncoder::getModelInfo() const {
    if (!clip_ctx_) {
        return "CLIP model not loaded";
    }
    
    std::string info = "CLIP Vision Encoder\n";
    info += "  Model: " + model_path_ + "\n";
    info += "  Model ID: " + model_id_ + "\n";
    info += "  Embedding dimension: " + std::to_string(getEmbeddingDimension()) + "\n";
    info += "  Number of patches: " + std::to_string(getNumPatches()) + "\n";
    info += "  Total embedding size: " + std::to_string(getTotalEmbeddingSize());
    
    if (config_) {
        info += "\n  API Version: " + config_->getAPIVersion();
        auto license = config_->getModelLicense(model_id_);
        if (license) {
            info += "\n  License: " + license->license_name;
        }
    }
    
    return info;
}

std::shared_ptr<ModelLicense> VisionEncoder::getModelLicense() const {
    if (config_) {
        return config_->getModelLicense(model_id_);
    }
    return nullptr;
}

bool VisionEncoder::validateImage(const std::string& image_path) const {
    if (!config_) {
        return true; // No validation if no config
    }
    
    const auto& validation = config_->getSecurityConfig().validation;
    if (!validation.enabled) {
        return true;
    }
    
    // Check file exists
    if (!std::filesystem::exists(image_path)) {
        spdlog::error("Image file not found: {}", image_path);
        return false;
    }
    
    // Validate size
    if (!validateImageSize(image_path)) {
        return false;
    }
    
    // Validate format
    if (!validateImageFormat(image_path)) {
        return false;
    }
    
    // Validate resolution (requires loading image metadata)
    // This is a basic check - full validation would require image library
    if (!validateImageResolution(image_path)) {
        return false;
    }
    
    return true;
}

void VisionEncoder::setUserContext(const std::string& user_id) {
    current_user_id_ = user_id;
}

bool VisionEncoder::validateImageSize(const std::string& image_path) const {
    if (!config_) {
      return true;
    }
    
    const auto& validation = config_->getSecurityConfig().validation;
    
    auto file_size = std::filesystem::file_size(image_path);
    auto file_size_mb = file_size / (1024 * 1024);
    
    if (file_size_mb > validation.max_image_size_mb) {
        spdlog::error("Image size ({} MB) exceeds limit ({} MB)", 
                     file_size_mb, validation.max_image_size_mb);
        return false;
    }
    
    return true;
}

bool VisionEncoder::validateImageFormat(const std::string& image_path) const {
    if (!config_) {
      return true;
    }
    
    const auto& validation = config_->getSecurityConfig().validation;
    
    // Get file extension
    auto extension = std::filesystem::path(image_path).extension().string();
    if (!extension.empty() && extension[0] == '.') {
        extension = extension.substr(1);
    }
    
    // Convert to uppercase for comparison
    std::transform(extension.begin(), extension.end(), extension.begin(), ::toupper);
    
    // Check if format is allowed
    for (const auto& allowed : validation.allowed_formats) {
        if (extension == allowed) {
            return true;
        }
    }
    
    spdlog::error("Image format '{}' not allowed", extension);
    return false;
}

bool VisionEncoder::validateImageResolution(const std::string& image_path) const {
    if (!config_) {
      return true;
    }
    
    const auto& validation = config_->getSecurityConfig().validation;
    
    // Basic validation - in production, would use image library to check actual dimensions
    // For now, just check file size as a proxy
    auto file_size = std::filesystem::file_size(image_path);
    
    // Rough estimate: max resolution = max_image_resolution * 3 bytes per pixel (RGB)
    auto max_width = validation.max_image_resolution.first;
    auto max_height = validation.max_image_resolution.second;
    size_t max_uncompressed_size = static_cast<size_t>(max_width) * max_height * 3;
    
    // JPEG compression is typically 10:1, so multiply by 10
    if (file_size > max_uncompressed_size * 10) {
        spdlog::warn("Image file size suggests resolution may exceed limits");
        // Don't fail, just warn - actual resolution check would require decoding
    }
    
    return true;
}

clip_image_u8* VisionEncoder::loadImage(const std::string& image_path) {
#ifdef THEMIS_ENABLE_LLM
    clip_image_u8* img = clip_image_u8_init();
    if (!img) {
        throw std::runtime_error("Failed to initialize image structure");
    }
    
    if (!clip_image_load_from_file(image_path.c_str(), img)) {
        clip_image_u8_free(img);
        throw std::runtime_error("Failed to load image: " + image_path);
    }
    
    return img;
#else
    throw std::runtime_error("VisionEncoder: LLM support not enabled");
#endif
}

clip_image_f32* VisionEncoder::preprocessImage(const clip_image_u8* img_u8) {
#ifdef THEMIS_ENABLE_LLM
    if (!clip_ctx_) {
        throw std::runtime_error("CLIP context not initialized");
    }
    
    clip_image_f32* img_f32 = clip_image_f32_init();
    if (!img_f32) {
        throw std::runtime_error("Failed to initialize preprocessed image structure");
    }
    
    if (!clip_image_preprocess(clip_ctx_, img_u8, img_f32)) {
        clip_image_f32_free(img_f32);
        throw std::runtime_error("Failed to preprocess image");
    }
    
    return img_f32;
#else
    throw std::runtime_error("VisionEncoder: LLM support not enabled");
#endif
}

void VisionEncoder::freeImage(clip_image_u8* img_u8) {
#ifdef THEMIS_ENABLE_LLM
    if (img_u8) {
        clip_image_u8_free(img_u8);
    }
#endif
}

void VisionEncoder::freeImage(clip_image_f32* img_f32) {
#ifdef THEMIS_ENABLE_LLM
    if (img_f32) {
        clip_image_f32_free(img_f32);
    }
#endif
}

} // namespace llm
} // namespace themis


