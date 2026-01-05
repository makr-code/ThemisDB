#include "llm/vision_encoder.h"
#include "utils/logger.h"
#include <filesystem>
#include <fstream>

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

VisionEncoder::VisionEncoder(const std::string& clip_model_path, int verbosity)
    : clip_ctx_(nullptr)
    , model_path_(clip_model_path)
    , verbosity_(verbosity)
    , initialized_(false)
{
#ifdef THEMIS_ENABLE_LLM
    // Check if model file exists
    if (!std::filesystem::exists(clip_model_path)) {
        throw std::runtime_error("CLIP model file not found: " + clip_model_path);
    }
    
    // Load CLIP model
    clip_ctx_ = clip_model_load(clip_model_path.c_str(), verbosity);
    if (!clip_ctx_) {
        throw std::runtime_error("Failed to load CLIP model: " + clip_model_path);
    }
    
    initialized_ = true;
    
    if (verbosity > 0) {
        spdlog::info("VisionEncoder: Loaded CLIP model from {}", clip_model_path);
        spdlog::info("  - Embedding dimension: {}", getEmbeddingDimension());
        spdlog::info("  - Number of patches: {}", getNumPatches());
        spdlog::info("  - Total embedding size: {}", getTotalEmbeddingSize());
    }
#else
    throw std::runtime_error("VisionEncoder: LLM support not enabled (THEMIS_ENABLE_LLM=OFF)");
#endif
}

VisionEncoder::~VisionEncoder() {
#ifdef THEMIS_ENABLE_LLM
    if (clip_ctx_) {
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
    
    // Check if image file exists
    if (!std::filesystem::exists(image_path)) {
        throw std::runtime_error("Image file not found: " + image_path);
    }
    
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
    
    // Use 4 threads for encoding (can be made configurable)
    if (!clip_image_encode(clip_ctx_, 4, img_f32, embeddings.data())) {
        clip_image_f32_free(img_f32);
        clip_image_u8_free(img_u8);
        throw std::runtime_error("Failed to encode image");
    }
    
    // Cleanup
    clip_image_f32_free(img_f32);
    clip_image_u8_free(img_u8);
    
    if (verbosity_ > 1) {
        spdlog::debug("VisionEncoder: Encoded image {} ({} floats)", image_path, embeddings.size());
    }
    
    return embeddings;
#else
    throw std::runtime_error("VisionEncoder: LLM support not enabled (THEMIS_ENABLE_LLM=OFF)");
#endif
}

std::vector<float> VisionEncoder::encodeImageData(const std::vector<uint8_t>& image_data) {
#ifdef THEMIS_ENABLE_LLM
    // For now, we need to write to a temporary file
    // Future enhancement: Support in-memory image loading
    std::string temp_path = std::filesystem::temp_directory_path() / "themis_temp_image.jpg";
    
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
    info += "  Embedding dimension: " + std::to_string(getEmbeddingDimension()) + "\n";
    info += "  Number of patches: " + std::to_string(getNumPatches()) + "\n";
    info += "  Total embedding size: " + std::to_string(getTotalEmbeddingSize());
    
    return info;
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
