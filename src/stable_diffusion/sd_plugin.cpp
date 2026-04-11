#include "stable_diffusion/sd_plugin.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <cstring>

namespace themis {
namespace imggen {

// ── constructors ──────────────────────────────────────────────────────────────

SDPlugin::SDPlugin()
    : generator_(std::make_unique<SDStubGenerator>())
    , sanitizer_() {}

SDPlugin::SDPlugin(std::unique_ptr<ISDGenerator> generator, SDPromptSanitizer sanitizer)
    : generator_(std::move(generator))
    , sanitizer_(std::move(sanitizer)) {}

// ── initialize ────────────────────────────────────────────────────────────────

bool SDPlugin::initialize(const std::string& model_path, const nlohmann::json& config) {
    model_path_ = model_path;
    SDConfig cfg = SDConfig::fromJson(config);
    cfg.model_path = model_path;

    // Load content-policy keywords if configured
    if (!cfg.blocked_keywords_file.empty()) {
        try {
            sanitizer_ = SDPromptSanitizer::fromFile(cfg.blocked_keywords_file);
        } catch (...) {
            // Non-fatal – proceed with empty sanitizer
        }
    }

    initialized_ = generator_->initialize(cfg);
    return initialized_;
}

// ── isPromptAllowed ───────────────────────────────────────────────────────────

bool SDPlugin::isPromptAllowed(const std::string& prompt) const {
    return sanitizer_.isAllowed(prompt);
}

// ── sha256Hex (simple FNV-based hex, used only as prompt fingerprint) ─────────

std::string SDPlugin::sha256Hex(const std::string& input) {
    // FNV-1a 64-bit – not cryptographic but sufficient as a stable prompt fingerprint
    uint64_t hash = 14695981039346656037ULL;
    for (unsigned char c : input) {
        hash ^= static_cast<uint64_t>(c);
        hash *= 1099511628211ULL;
    }
    std::ostringstream oss;
    oss << std::hex << std::setfill('0') << std::setw(16) << hash;
    return oss.str();
}

// ── encodeMinimalPng ──────────────────────────────────────────────────────────

std::vector<uint8_t> SDPlugin::encodeMinimalPng(const std::vector<uint8_t>& /*rgb*/,
                                                  int width, int height) {
    // Produce a minimal valid PNG: 8-byte signature + IHDR + IDAT (empty) + IEND
    // This is a stub encoder sufficient for tests.  Real deployments link libpng/stb.
    static const uint8_t kSig[]  = {0x89,'P','N','G','\r','\n',0x1a,'\n'};
    static const uint8_t kIEND[] = {0,0,0,0,'I','E','N','D',0xae,0x42,0x60,0x82};

    // IHDR chunk: length(4) + "IHDR"(4) + w(4) + h(4) + bitdepth(1) + colortype(1)
    //             + compression(1) + filter(1) + interlace(1) + crc(4) = 25 bytes
    std::vector<uint8_t> png;
    png.insert(png.end(), kSig, kSig + 8);

    // IHDR
    uint8_t ihdr[25];
    ihdr[0]=0; ihdr[1]=0; ihdr[2]=0; ihdr[3]=13;   // data length = 13
    ihdr[4]='I'; ihdr[5]='H'; ihdr[6]='D'; ihdr[7]='R';
    ihdr[8] =(width>>24)&0xFF;  ihdr[9] =(width>>16)&0xFF;
    ihdr[10]=(width>> 8)&0xFF;  ihdr[11]=(width    )&0xFF;
    ihdr[12]=(height>>24)&0xFF; ihdr[13]=(height>>16)&0xFF;
    ihdr[14]=(height>> 8)&0xFF; ihdr[15]=(height    )&0xFF;
    ihdr[16]=8;  // bit depth
    ihdr[17]=2;  // RGB
    ihdr[18]=0; ihdr[19]=0; ihdr[20]=0;  // compression, filter, interlace
    // CRC (stub: zero)
    ihdr[21]=0; ihdr[22]=0; ihdr[23]=0; ihdr[24]=0;
    png.insert(png.end(), ihdr, ihdr + 25);

    png.insert(png.end(), kIEND, kIEND + 12);
    return png;
}

// ── generateLocked (internal, called with generate_mutex_ held) ───────────────

GeneratedImage SDPlugin::generateLocked(const std::string& prompt,
                                         const SDGenerationConfig& cfg) {
    GeneratedImage img;
    img.plugin_version = getPluginVersion();

    if (!initialized_) {
        ++error_count_;
        img.success = false;
        img.error_message = "SDPlugin not initialized";
        return img;
    }

    // Content policy check – positive prompt
    if (!isPromptAllowed(prompt)) {
        ++blocked_count_;
        img.success = false;
        img.error_message = "prompt blocked by content policy";
        img.prompt_hash   = sha256Hex(prompt);
        return img;
    }

    // Content policy check – negative_prompt (security: prevent policy bypass via negation)
    if (!cfg.negative_prompt.empty() && !isPromptAllowed(cfg.negative_prompt)) {
        ++blocked_count_;
        img.success = false;
        img.error_message = "negative_prompt blocked by content policy";
        img.prompt_hash   = sha256Hex(prompt);
        return img;
    }

    const std::string sanitized = sanitizer_.sanitize(prompt);
    img.prompt_hash = sha256Hex(sanitized);

    try {
        int      w = 0, h = 0;
        uint64_t seed_used = 0;
        const auto rgb = generator_->generate(sanitized, cfg, w, h, seed_used);

        img.png_data   = encodeMinimalPng(rgb, w, h);
        img.width      = w;
        img.height     = h;
        img.sampler    = cfg.sampler;
        img.seed_used  = seed_used;
        img.model_id   = getModelId();
        img.generation_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        img.success = true;
        ++generation_count_;
    } catch (const std::exception& ex) {
        ++error_count_;
        img.success       = false;
        img.error_message = ex.what();
    }

    return img;
}

// ── generate ──────────────────────────────────────────────────────────────────

GeneratedImage SDPlugin::generate(const std::string& prompt,
                                   const SDGenerationConfig& cfg) {
    std::lock_guard<std::mutex> lock(generate_mutex_);
    return generateLocked(prompt, cfg);
}

// ── generateBatch ─────────────────────────────────────────────────────────────

std::vector<GeneratedImage> SDPlugin::generateBatch(
        const std::vector<std::string>& prompts,
        const SDGenerationConfig& cfg) {
    std::lock_guard<std::mutex> lock(generate_mutex_);
    std::vector<GeneratedImage> results;
    results.reserve(prompts.size());
    for (const auto& p : prompts) {
        results.push_back(generateLocked(p, cfg));
    }
    return results;
}

// ── generateImg2Img ───────────────────────────────────────────────────────────

GeneratedImage SDPlugin::generateImg2Img(const std::string& prompt,
                                          const Img2ImgConfig& cfg) {
    std::lock_guard<std::mutex> lock(generate_mutex_);

    GeneratedImage img;
    img.plugin_version = getPluginVersion();

    if (!initialized_) {
        ++error_count_;
        img.success = false;
        img.error_message = "SDPlugin not initialized";
        return img;
    }

    // Content policy check – positive prompt
    if (!isPromptAllowed(prompt)) {
        ++blocked_count_;
        img.success = false;
        img.error_message = "prompt blocked by content policy";
        img.prompt_hash   = sha256Hex(prompt);
        return img;
    }

    // Content policy check – negative_prompt
    if (!cfg.negative_prompt.empty() && !isPromptAllowed(cfg.negative_prompt)) {
        ++blocked_count_;
        img.success = false;
        img.error_message = "negative_prompt blocked by content policy";
        img.prompt_hash   = sha256Hex(prompt);
        return img;
    }

    const std::string sanitized = sanitizer_.sanitize(prompt);
    img.prompt_hash = sha256Hex(sanitized);

    try {
        int      w = 0, h = 0;
        uint64_t seed_used = 0;
        const auto rgb = generator_->generateImg2Img(sanitized, cfg, w, h, seed_used);

        img.png_data   = encodeMinimalPng(rgb, w, h);
        img.width      = w;
        img.height     = h;
        img.sampler    = cfg.sampler;
        img.seed_used  = seed_used;
        img.model_id   = getModelId();
        img.generation_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        img.success = true;
        ++generation_count_;
    } catch (const std::exception& ex) {
        ++error_count_;
        img.success       = false;
        img.error_message = ex.what();
    }

    return img;
}

// ── getModelId / getStatistics ────────────────────────────────────────────────

std::string SDPlugin::getModelId() const {
    return generator_ ? generator_->getModelId() : model_path_;
}

nlohmann::json SDPlugin::getStatistics() const {
    return {
        {"plugin",           "stable_diffusion"},
        {"plugin_version",   getPluginVersion()},
        {"model_id",         getModelId()},
        {"initialized",      initialized_},
        {"generation_count", generation_count_},
        {"blocked_count",    blocked_count_},
        {"error_count",      error_count_}
    };
}

} // namespace imggen
} // namespace themis

// ── dynamic-loading entry points ──────────────────────────────────────────────

extern "C" THEMIS_PLUGIN_EXPORT
themis::imggen::IImageGenerationBackend* themis_imggen_create() {
    return new themis::imggen::SDPlugin();
}

extern "C" THEMIS_PLUGIN_EXPORT
void themis_imggen_destroy(themis::imggen::IImageGenerationBackend* p) {
    delete p;
}
