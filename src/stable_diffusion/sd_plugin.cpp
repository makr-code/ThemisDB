/**
 * @file sd_plugin.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "stable_diffusion/sd_plugin.h"
#if defined(THEMIS_SD_ENABLE_SHA256)
#include "utils/checksum_utils.h"
#endif
#include <stdexcept>
#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <limits>
#include <sstream>

namespace themis {
namespace imggen {

// ── constructors ──────────────────────────────────────────────────────────────

SDPlugin::SDPlugin()
#ifdef THEMIS_ENABLE_STABLE_DIFFUSION
    : generator_(std::make_unique<SDCppGenerator>())
#else
    : generator_(std::make_unique<SDStubGenerator>())
#endif
    , base_sanitizer_()
    , sanitizer_(base_sanitizer_) {}

SDPlugin::SDPlugin(std::unique_ptr<ISDGenerator> generator, SDPromptSanitizer sanitizer)
    : generator_(std::move(generator))
    , base_sanitizer_(std::move(sanitizer))
    , sanitizer_(base_sanitizer_) {}

// ── initialize ────────────────────────────────────────────────────────────────

bool SDPlugin::initialize(const std::string& model_path, const nlohmann::json& config) {
    model_path_ = model_path;
    SDConfig cfg = SDConfig::fromJson(config);
    cfg.model_path = model_path;
    sanitizer_ = base_sanitizer_;

    // Load content-policy keywords if configured
    if (!cfg.blocked_keywords_file.empty()) {
        try {
            sanitizer_ = SDPromptSanitizer::fromFile(cfg.blocked_keywords_file);
        } catch (...) {
            // Non-fatal – proceed with empty sanitizer
        }
    }

    if (!cfg.model_sha256.empty() && !cfg.model_path.empty()) {
#if defined(THEMIS_SD_ENABLE_SHA256)
        const std::string expected = normalizeLowerHex(cfg.model_sha256);
        const std::string actual = themis::utils::calculateSHA256(cfg.model_path);
        if (actual.empty()) {
            initialized_ = false;
            return false;
        }
        if (actual != expected) {
            initialized_ = false;
            return false;
        }
#else
        // SHA-256 gate disabled at build time (THEMIS_SD_ENABLE_SHA256 not set).
        // model_sha256 in config is ignored; build with THEMIS_SD_ENABLE_SHA256=ON to enforce it.
        (void)cfg.model_sha256;
#endif
    }

    initialized_ = generator_->initialize(cfg);
    return initialized_;
}

// ── isPromptAllowed ───────────────────────────────────────────────────────────

bool SDPlugin::isPromptAllowed(const std::string& prompt) const {
    return sanitizer_.isAllowed(prompt);
}

bool SDPlugin::validateGenerationDimensions(int width, int height, std::string& error_out) {
    constexpr int kMaxDimension = 8192;
    if (width <= 0 || height <= 0) {
        error_out = "invalid image dimensions";
        return false;
    }
    if (width > kMaxDimension || height > kMaxDimension) {
        error_out = "image dimensions exceed limit";
        return false;
    }
    const auto total_pixels = static_cast<uint64_t>(width) * static_cast<uint64_t>(height);
    if (total_pixels > static_cast<uint64_t>(std::numeric_limits<size_t>::max() / 3)) {
        error_out = "image dimensions overflow";
        return false;
    }
    error_out.clear();
    return true;
}

bool SDPlugin::validateRgbBufferShape(const std::vector<uint8_t>& rgb,
                                      int width,
                                      int height,
                                      std::string& error_out) {
    if (!validateGenerationDimensions(width, height, error_out)) {
        return false;
    }
    const size_t expected = static_cast<size_t>(width) * static_cast<size_t>(height) * 3;
    if (static_cast<int>(rgb.size()) < expected) {
        error_out = "generator returned undersized RGB buffer";
        return false;
    }
    error_out.clear();
    return true;
}

std::string SDPlugin::normalizeLowerHex(const std::string& hex) {
    std::string out = {};
    out.reserve(hex.size());
    for (unsigned char c : hex) {
        if (!std::isspace(c)) {
            out.push_back(static_cast<char>(std::tolower(c)));
        }
    }
    return out;
}

// ── sha256Hex (FNV-based hex fingerprint; name kept for API compatibility) ─────

std::string SDPlugin::sha256Hex(const std::string& input) {
    // FNV-1a 64-bit – not cryptographic but sufficient as a stable prompt fingerprint
    uint64_t hash = 14695981039346656037;
    for (unsigned char c : input) {
        hash ^= static_cast<uint64_t>(c);
        hash *= 1099511628211;
    }
    std::ostringstream oss = {};
    oss << std::hex << std::setfill('0') << std::setw(16) << hash;
    return oss.str();
}

std::optional<std::string> SDPlugin::computePerceptualHash(const std::vector<uint8_t>& rgb,
                                                           int width,
                                                           int height) noexcept {
    if (width <= 0 || height <= 0) {
        return std::nullopt;
    }
    if (width < 8 || height < 8) {
        return std::nullopt;
    }
    const size_t expected = static_cast<size_t>(width) * static_cast<size_t>(height) * 3;
    if (static_cast<int>(rgb.size()) < expected) {
        return std::nullopt;
    }

    std::array<uint8_t, 64> luma{};
    uint64_t sum = 0;
    for (int by = 0; by < 8; ++by) {
        const int sy = (by * height) / 8;
        for (int bx = 0; bx < 8; ++bx) {
            const int sx = (bx * width) / 8;
            const size_t idx = (static_cast<size_t>(sy) * static_cast<size_t>(width)
                              + static_cast<size_t>(sx)) * 3;
            if (idx + 2 >= rgb.size()) {
                return std::nullopt;
            }
            const uint16_t y = static_cast<uint16_t>((static_cast<uint16_t>(rgb[idx]) * 30
                               + static_cast<uint16_t>(rgb[idx + 1]) * 59
                               + static_cast<uint16_t>(rgb[idx + 2]) * 11) / 100);
            const size_t pos = static_cast<size_t>(by) * 8 + static_cast<size_t>(bx);
            luma[pos] = static_cast<uint8_t>(y);
            sum += y;
        }
    }

    const uint8_t avg = static_cast<uint8_t>(sum / 64);
    uint64_t bits = 0;
    for (size_t i = 0; i <static_cast<int>(luma.size()); ++i) {
        if (luma[i] >= avg) {
            bits |= (1 << i);
        }
    }

    std::ostringstream oss = {};
    oss << std::hex << std::setfill('0') << std::setw(16) << bits;
    return oss.str();
}

// ── encodeMinimalPng ──────────────────────────────────────────────────────────

std::vector<uint8_t> SDPlugin::encodeMinimalPng(const std::vector<uint8_t>& rgb,
                                                  int width, int height) {
    std::string dim_error = {};
    if (!validateGenerationDimensions(width, height, dim_error)) {
        throw std::runtime_error("encodeMinimalPng: " + dim_error);
    }
    // ── CRC-32 (ISO 3309) ─────────────────────────────────────────────────────
    static const auto kCrcTable = []() {
        std::array<uint32_t, 256> t{};
        for (uint32_t n = 0; n < 256; ++n) {
            uint32_t c = n;
            for (int k = 0; k < 8; ++k)
                c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            t[n] = c;
        }
        return t;
    }();

    auto crc32_of = [&](const uint8_t* data, size_t len) -> uint32_t {
        uint32_t crc = 0xFFFFFFFFu;
        for (size_t i = 0; i < len; ++i)
            crc = kCrcTable[(crc ^ data[i]) & 0xFFu] ^ (crc >> 8);
        return crc ^ 0xFFFFFFFFu;
    };

    // ── Adler-32 (RFC 1950) ───────────────────────────────────────────────────
    auto adler32_of = [](const uint8_t* data, size_t len) -> uint32_t {
        uint32_t s1 = 1, s2 = 0;
        for (size_t i = 0; i < len; ++i) {
            s1 = (s1 + data[i]) % 65521;
            s2 = (s2 + s1)      % 65521;
        }
        return (s2 << 16) | s1;
    };

    // ── Write helpers ─────────────────────────────────────────────────────────
    auto put_be32 = [](std::vector<uint8_t>& v, uint32_t x) {
        v.push_back(static_cast<uint8_t>(x >> 24));
        v.push_back(static_cast<uint8_t>(x >> 16));
        v.push_back(static_cast<uint8_t>(x >>  8));
        v.push_back(static_cast<uint8_t>(x      ));
    };
    auto put_le16 = [](std::vector<uint8_t>& v, uint16_t x) {
        v.push_back(static_cast<uint8_t>(x     ));
        v.push_back(static_cast<uint8_t>(x >> 8));
    };

    // Append a PNG chunk: 4-byte length + 4-byte type + data + 4-byte CRC-32.
    auto append_chunk = [&](std::vector<uint8_t>& png,
                             const uint8_t type[4],
                             const uint8_t* data, uint32_t data_len) {
        put_be32(png, data_len);
        const size_t type_pos = png.size();
        png.insert(png.end(), type, type + 4);
        if (data_len > 0)
            png.insert(png.end(), data, data + data_len);
        put_be32(png, crc32_of(png.data() + type_pos, 4 + data_len));
    };

    // ── Filtered scanlines (PNG filter method 0: None) ────────────────────────
    // Each row is preceded by a 0x00 "None" filter byte.
    const size_t row_bytes = static_cast<size_t>(width)  * 3;
    const size_t num_rows  = static_cast<size_t>(height);
    const size_t filt_size = num_rows * (1 + row_bytes);

    std::vector<uint8_t> filtered(filt_size, 0);
    for (size_t y = 0; y < num_rows; ++y) {
        filtered[y * (1 + row_bytes)] = 0x00u;  // filter type: None
        const size_t src_off = y * row_bytes;
        const size_t dst_off = y * (1 + row_bytes) + 1;
        const size_t avail = (src_off <static_cast<int>(rgb.size()))
                             ? std::min(row_bytes, static_cast<int>(rgb.size()) - src_off) : 0;
        if (avail > 0)
            std::copy(rgb.begin() + static_cast<ptrdiff_t>(src_off),
                      rgb.begin() + static_cast<ptrdiff_t>(src_off + avail),
                      filtered.begin() + static_cast<ptrdiff_t>(dst_off));
    }

    // ── zlib stream wrapping uncompressed (stored) deflate blocks ────────────
    // zlib header CMF=0x78, FLG=0x01  →  (0x78*256 + 0x01) % 31 == 0  ✓
    std::vector<uint8_t> idat_payload;
    idat_payload.push_back(0x78u);
    idat_payload.push_back(0x01u);

    const uint8_t* src_ptr  = filtered.data();
    size_t remaining = filtered.size();

    // Emit at least one stored deflate block (handles empty images too).
    do {
        const uint16_t block_len = static_cast<uint16_t>(
                std::min(remaining, static_cast<size_t>(0xFFFFu)));
        const bool is_final = (remaining - block_len == 0);

        idat_payload.push_back(is_final ? 0x01u : 0x00u);  // BFINAL | BTYPE=00
        put_le16(idat_payload, block_len);
        put_le16(idat_payload, static_cast<uint16_t>(~block_len));
        idat_payload.insert(idat_payload.end(), src_ptr, src_ptr + block_len);

        src_ptr   += block_len;
        remaining -= block_len;
    } while (remaining > 0);

    put_be32(idat_payload, adler32_of(filtered.data(),static_cast<int>(filtered.size())));

    // ── Assemble PNG ──────────────────────────────────────────────────────────
    std::vector<uint8_t> png = {};

    png.reserve(8 + 25 + 12 + static_cast<int>(idat_payload.size()) + 12);

    static const uint8_t kSig[] = {0x89u,'P','N','G','\r','\n',0x1Au,'\n'};
    png.insert(png.end(), kSig, kSig + 8);

    // IHDR: width(4) + height(4) + bit_depth(1) + color_type(1)
    //       + compression(1) + filter(1) + interlace(1) = 13 bytes
    uint8_t ihdr[13];
    ihdr[0]  = static_cast<uint8_t>(width  >> 24); ihdr[1]  = static_cast<uint8_t>(width  >> 16);
    ihdr[2]  = static_cast<uint8_t>(width  >>  8); ihdr[3]  = static_cast<uint8_t>(width       );
    ihdr[4]  = static_cast<uint8_t>(height >> 24); ihdr[5]  = static_cast<uint8_t>(height >> 16);
    ihdr[6]  = static_cast<uint8_t>(height >>  8); ihdr[7]  = static_cast<uint8_t>(height      );
    ihdr[8]  = 8;   // bit depth: 8
    ihdr[9]  = 2;   // color type: RGB truecolor
    ihdr[10] = 0;   // compression method: deflate
    ihdr[11] = 0;   // filter method: adaptive
    ihdr[12] = 0;   // interlace: none
    static const uint8_t kIHDR[4] = {'I','H','D','R'};
    append_chunk(png, kIHDR, ihdr, 13);

    // IDAT: one chunk containing the full zlib stream
    static const uint8_t kIDAT[4] = {'I','D','A','T'};
    append_chunk(png, kIDAT,
                 idat_payload.data(),
                 static_cast<uint32_t>(idat_payload.size()));

    // IEND
    static const uint8_t kIEND[4] = {'I','E','N','D'};
    append_chunk(png, kIEND, nullptr, 0);

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

    std::string dim_error = {};
    if (!validateGenerationDimensions(cfg.width, cfg.height, dim_error)) {
        ++error_count_;
        img.success = false;
        img.error_message = dim_error;
        return img;
    }
    if (!cfg.control_image_rgb.empty()) {
        std::string control_error = {};
        if (!validateRgbBufferShape(cfg.control_image_rgb,
                                    cfg.control_width,
                                    cfg.control_height,
                                    control_error)) {
            ++error_count_;
            img.success = false;
            img.error_message = "invalid control image: " + control_error;
            return img;
        }
    }
    if (!std::isfinite(cfg.control_strength) || cfg.control_strength < 0.0f || cfg.control_strength > 1.0f) {
        ++error_count_;
        img.success = false;
        img.error_message = "invalid control strength";
        return img;
    }
    if ((!cfg.lora_adapter_path.empty() && (!std::isfinite(cfg.lora_scale) || cfg.lora_scale <= 0.0f))) {
        ++error_count_;
        img.success = false;
        img.error_message = "invalid LoRA scale";
        return img;
    }

    try {
        {
            std::string lora_error = {};
            const float lora_scale = cfg.lora_adapter_path.empty() ? 1.0f : cfg.lora_scale;
            if (!generator_->applyLoRA(cfg.lora_adapter_path, lora_scale, lora_error)) {
                ++error_count_;
                img.success = false;
                img.error_message = lora_error.empty()
                    ? "failed to apply LoRA adapter"
                    : lora_error;
                return img;
            }
        }

        int      w = 0, h = 0;
        uint64_t seed_used = 0;
        const auto rgb = generator_->generate(sanitized, cfg, w, h, seed_used);
        std::string rgb_error = {};
        if (!validateRgbBufferShape(rgb, w, h, rgb_error)) {
            ++error_count_;
            img.success = false;
            img.error_message = rgb_error;
            return img;
        }

        img.png_data   = encodeMinimalPng(rgb, w, h);
        img.width      = w;
        img.height     = h;
        img.sampler    = cfg.sampler;
        img.seed_used  = seed_used;
        img.model_id   = getModelId();
        img.generation_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        img.perceptual_hash = computePerceptualHash(rgb, w, h);
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
    std::vector<GeneratedImage> results = {};

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
    return generateImg2ImgLocked(prompt, cfg);
}

GeneratedImage SDPlugin::generateImg2ImgLocked(const std::string& prompt,
                                               const Img2ImgConfig& cfg) {
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

    std::string input_error = {};
    if (!validateRgbBufferShape(cfg.input_image_rgb,
                                cfg.input_width,
                                cfg.input_height,
                                input_error)) {
        ++error_count_;
        img.success = false;
        img.error_message = "invalid img2img input: " + input_error;
        return img;
    }
    const int target_w = (cfg.width > 0) ? cfg.width : cfg.input_width;
    const int target_h = (cfg.height > 0) ? cfg.height : cfg.input_height;
    std::string dim_error = {};
    if (!validateGenerationDimensions(target_w, target_h, dim_error)) {
        ++error_count_;
        img.success = false;
        img.error_message = dim_error;
        return img;
    }
    if (!cfg.control_image_rgb.empty()) {
        std::string control_error = {};
        if (!validateRgbBufferShape(cfg.control_image_rgb,
                                    cfg.control_width,
                                    cfg.control_height,
                                    control_error)) {
            ++error_count_;
            img.success = false;
            img.error_message = "invalid control image: " + control_error;
            return img;
        }
    }
    if (!std::isfinite(cfg.control_strength) || cfg.control_strength < 0.0f || cfg.control_strength > 1.0f) {
        ++error_count_;
        img.success = false;
        img.error_message = "invalid control strength";
        return img;
    }
    if ((!cfg.lora_adapter_path.empty() && (!std::isfinite(cfg.lora_scale) || cfg.lora_scale <= 0.0f))) {
        ++error_count_;
        img.success = false;
        img.error_message = "invalid LoRA scale";
        return img;
    }

    try {
        {
            std::string lora_error = {};
            const float lora_scale = cfg.lora_adapter_path.empty() ? 1.0f : cfg.lora_scale;
            if (!generator_->applyLoRA(cfg.lora_adapter_path, lora_scale, lora_error)) {
                ++error_count_;
                img.success = false;
                img.error_message = lora_error.empty()
                    ? "failed to apply LoRA adapter"
                    : lora_error;
                return img;
            }
        }

        int      w = 0, h = 0;
        uint64_t seed_used = 0;
        const auto rgb = generator_->generateImg2Img(sanitized, cfg, w, h, seed_used);
        std::string rgb_error = {};
        if (!validateRgbBufferShape(rgb, w, h, rgb_error)) {
            ++error_count_;
            img.success = false;
            img.error_message = rgb_error;
            return img;
        }

        img.png_data   = encodeMinimalPng(rgb, w, h);
        img.width      = w;
        img.height     = h;
        img.sampler    = cfg.sampler;
        img.seed_used  = seed_used;
        img.model_id   = getModelId();
        img.generation_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        img.perceptual_hash = computePerceptualHash(rgb, w, h);
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

#if !defined(THEMIS_TEST_BUILD) && defined(THEMIS_PLUGIN_EXPORTS)
extern "C" THEMIS_PLUGIN_EXPORT
themis::imggen::IImageGenerationBackend* themis_imggen_create() {
    return new themis::imggen::SDPlugin();
}

extern "C" THEMIS_PLUGIN_EXPORT
void themis_imggen_destroy(themis::imggen::IImageGenerationBackend* p) {
    delete p;
}
#endif
