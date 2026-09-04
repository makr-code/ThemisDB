/**
 * @file model_quantization_pipeline.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=8, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/model_quantization_pipeline.h"
#include "llm/lora_framework/quantized_model.h"

#ifndef THEMIS_NO_SPDLOG
#include <spdlog/spdlog.h>
#else
namespace spdlog {
    template<typename... Args> inline void debug(const char*, Args&&...) {}
    template<typename... Args> inline void info(const char*, Args&&...) {}
    template<typename... Args> inline void warn(const char*, Args&&...) {}
    template<typename... Args> inline void error(const char*, Args&&...) {}
}
#endif

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "utils/logger.h"

namespace fs = std::filesystem;

namespace themis {
namespace llm {

// ---------------------------------------------------------------------------
// Public interface
// ---------------------------------------------------------------------------

lora::QuantizedModel ModelQuantizationPipeline::load(
    const std::string& path,
    ModelFormat format,
    const QuantizationPipelineConfig& config)
{
    ModelFormat resolved = format;
    if (resolved == ModelFormat::AUTO) {
        resolved = detect_format(path);
    }

    spdlog::info("ModelQuantizationPipeline: loading '{}' as {}",
                 path, format_name(resolved));

    switch (resolved) {
        case ModelFormat::GGUF:
            return load_gguf(path, config);
        case ModelFormat::AWQ:
            return load_awq(path, config);
        case ModelFormat::GPTQ:
            return load_gptq(path, config);
        default:
            throw std::runtime_error(
                "ModelQuantizationPipeline: unsupported format for path: " + path);
    }
}

ModelFormat ModelQuantizationPipeline::detect_format(const std::string& path)
{
    // 1) File with .gguf extension
    if (path.size() > 5 &&
        path.substr(path.size() - 5) == ".gguf") {
        return ModelFormat::GGUF;
    }

    // 2) Directory: inspect config.json for quant_type
    const fs::path dir(path);
    const fs::path config_path = dir / "config.json";

    if (fs::exists(config_path)) {
        std::ifstream f(config_path);
        if (f) {
            try {
                auto j = nlohmann::json::parse(f);
                // HuggingFace quantization_config.quant_type field
                if (j.contains("quantization_config")) {
                    const auto& qcfg = j["quantization_config"];
                    std::string qtype;
                    if (qcfg.contains("quant_type")) {
                        qtype = qcfg["quant_type"].get<std::string>();
                    } else if (qcfg.contains("quant_method")) {
                        qtype = qcfg["quant_method"].get<std::string>();
                    }
                    // Normalize to lowercase
                    std::transform(qtype.begin(), qtype.end(), qtype.begin(),
                                   [](unsigned char c){ return static_cast<unsigned char>(std::tolower(c)); });
                    if (qtype == "awq") {
                      return ModelFormat::AWQ;
                    }
                    if (qtype == "gptq") {
                      return ModelFormat::GPTQ;
                    }
                }
            } catch (...) {
                THEMIS_WARN("model_quantization_pipeline: unhandled exception caught");
                // config.json present but malformed – fall through to heuristics
            }
        }
    }

    // 3) Heuristic: look for qweight tensors in any shard header (GPTQ)
    //    or weight + scales tensors (AWQ) without full parsing
    if (fs::is_directory(dir)) {
        for (const auto& entry : fs::directory_iterator(dir)) {
            const std::string fname = entry.path().filename().string();
            if (fname.size() > 12 &&
                fname.substr(fname.size() - 12) == ".safetensors") {
                // Quickly read just the JSON header to check tensor names
                std::ifstream sf(entry.path(), std::ios::binary);
                if (sf) {
                    uint64_t hdr_len = 0;
                    sf.read(reinterpret_cast<char*>(&hdr_len), sizeof(hdr_len));
                    if (hdr_len > 0 && hdr_len < 16 * 1024 * 1024) {
                        std::string hdr_str(hdr_len, '\0');
                        sf.read(hdr_str.data(), static_cast<std::streamsize>(hdr_len));
                        if (hdr_str.find("qweight") != std::string::npos) {
                            return ModelFormat::GPTQ;
                        }
                        if (hdr_str.find("\"scales\"") != std::string::npos) {
                            return ModelFormat::AWQ;
                        }
                    }
                }
                break; // Only probe the first shard
            }
        }
    }

    // Default: assume GGUF (caller can override with explicit format)
    return ModelFormat::GGUF;
}

const char* ModelQuantizationPipeline::format_name(ModelFormat fmt)
{
    switch (fmt) {
        case ModelFormat::GGUF: return "GGUF";
        case ModelFormat::AWQ:  return "AWQ";
        case ModelFormat::GPTQ: return "GPTQ";
        default:                return "AUTO";
    }
}

// ---------------------------------------------------------------------------
// GGUF loader (delegates to existing lora_framework implementation)
// ---------------------------------------------------------------------------

lora::QuantizedModel ModelQuantizationPipeline::load_gguf(
    const std::string& path,
    const QuantizationPipelineConfig& cfg)
{
    lora::QuantizedModelConfig model_cfg;
    model_cfg.block_size = cfg.block_size;
    if (cfg.target_type != lora::QuantizationType::NONE) {
        model_cfg.quantization_type = cfg.target_type;
        return lora::quantized_model_utils::load_from_gguf(path, &model_cfg);
    }
    return lora::quantized_model_utils::load_from_gguf(path, nullptr);
}

// ---------------------------------------------------------------------------
// SafeTensors parsing
// ---------------------------------------------------------------------------

ModelQuantizationPipeline::SafeTensorsFile
ModelQuantizationPipeline::parse_safetensors(const std::string& file_path)
{
    std::ifstream f(file_path, std::ios::binary | std::ios::ate);
    if (!f) {
        throw std::runtime_error("Cannot open safetensors file: " + file_path);
    }
    const auto file_size = static_cast<size_t>(f.tellg());
    f.seekg(0, std::ios::beg);

    if (file_size < 8) {
        throw std::runtime_error("Safetensors file too small: " + file_path);
    }

    // Read 8-byte header length (little-endian uint64)
    uint64_t hdr_len = 0;
    f.read(reinterpret_cast<char*>(&hdr_len), sizeof(hdr_len));
    // Guard: header must fit in the file and not exceed 64 MB to prevent OOM
    constexpr uint64_t kMaxHeaderBytes = 64ULL * 1024 * 1024;
    if (hdr_len == 0 || hdr_len > file_size - 8 || hdr_len > kMaxHeaderBytes) {
        throw std::runtime_error(
            "Safetensors: invalid header length in " + file_path);
    }

    // Read JSON header
    std::string hdr_str(hdr_len, '\0');
    f.read(hdr_str.data(), static_cast<std::streamsize>(hdr_len));
    if (!f) {
        throw std::runtime_error(
            "Safetensors: failed to read header from " + file_path);
    }

    auto hdr = nlohmann::json::parse(hdr_str);

    // Read binary data (everything after the 8-byte length + header)
    const size_t data_offset = 8 + static_cast<size_t>(hdr_len);
    const size_t data_size   = file_size - data_offset;
    std::vector<uint8_t> data(data_size);
    if (data_size > 0) {
        f.read(reinterpret_cast<char*>(data.data()),
               static_cast<std::streamsize>(data_size));
        if (f.bad()) {
            throw std::runtime_error(
                "Safetensors: failed to read data from " + file_path);
        }
    }

    SafeTensorsFile result;
    result.data = std::move(data);

    for (auto it = hdr.begin(); it != hdr.end(); ++it) {
        if (it.key() == "__metadata__") {
          continue;
        }

        SafeTensorDesc desc;
        const auto& val = it.value();

        desc.dtype = val.value("dtype", "F32");

        if (val.contains("shape")) {
            for (const auto& dim : val["shape"]) {
                desc.shape.push_back(dim.get<int64_t>());
            }
        }
        if (val.contains("data_offsets")) {
            const uint64_t begin = val["data_offsets"][0].get<uint64_t>();
            const uint64_t end   = val["data_offsets"][1].get<uint64_t>();
            // Validate offsets against the data buffer to prevent OOB reads
            if (begin > end || end > result.data.size()) {
                throw std::runtime_error(
                    "Safetensors: tensor '" + it.key() +
                    "' has out-of-bounds data_offsets in " + file_path);
            }
            desc.data_begin = begin;
            desc.data_end   = end;
        }
        result.tensors[it.key()] = std::move(desc);
    }

    return result;
}

std::vector<std::string> ModelQuantizationPipeline::find_safetensor_shards(
    const std::string& dir)
{
    std::vector<std::string> shards = {};

    for (const auto& entry : fs::directory_iterator(dir)) {
        const std::string fname = entry.path().filename().string();
        if (fname.size() > 12 &&
            fname.substr(fname.size() - 12) == ".safetensors") {
            shards.push_back(entry.path().string());
        }
    }
    std::sort(shards.begin(), shards.end());
    return shards;
}

// ---------------------------------------------------------------------------
// Weight-unpacking helpers
// ---------------------------------------------------------------------------

std::vector<float> ModelQuantizationPipeline::unpack_int32_weights(
    const void* packed_data,
    size_t n_packed,
    int bits)
{
    const int values_per_int32 = 32 / bits;
    const uint32_t mask = (1u << bits) - 1u;

    std::vector<float> out;
    out.reserve(n_packed * static_cast<size_t>(values_per_int32));

    const auto* src = static_cast<const uint32_t*>(packed_data);
    for (size_t i = 0; i < n_packed; ++i) {
        const uint32_t packed = src[i];
        for (int b = 0; b < values_per_int32; ++b) {
            const uint32_t v = (packed >> (b * bits)) & mask;
            out.push_back(static_cast<float>(v));
        }
    }
    return out;
}

std::vector<float> ModelQuantizationPipeline::fp16_to_fp32_array(
    const void* fp16_data,
    size_t n)
{
    std::vector<float> out(n);
    const auto* src = static_cast<const uint16_t*>(fp16_data);
    for (size_t i = 0; i < n; ++i) {
        const uint16_t h = src[i];
        // IEEE 754 FP16 → FP32 conversion
        const uint32_t sign     = (h & 0x8000u) << 16u;
        const uint32_t exponent = (h & 0x7C00u) >> 10u;
        const uint32_t mantissa = (h & 0x03FFu);

        uint32_t bits32 = 0;
        if (exponent == 0) {
            // Subnormal or zero
            if (mantissa == 0) {
                bits32 = sign;
            } else {
                // Normalise subnormal
                uint32_t m = mantissa;
                uint32_t e = 0;
                while ((m & 0x0400u) == 0) { m <<= 1u; ++e; }
                bits32 = sign | ((127u - 14u - e) << 23u) | ((m & 0x03FFu) << 13u);
            }
        } else if (exponent == 31u) {
            // Inf or NaN
            bits32 = sign | 0x7F800000u | (mantissa << 13u);
        } else {
            bits32 = sign | ((exponent + 127u - 15u) << 23u) | (mantissa << 13u);
        }
        std::memcpy(&out[i], &bits32, sizeof(float));
    }
    return out;
}

std::vector<float> ModelQuantizationPipeline::dequantize_awq_layer(
    const void* qweight_packed,
    const void* qzeros_packed,
    const void* scales_fp16,
    int64_t in_features,
    int64_t out_features,
    int group_size,
    int bits)
{
    if (bits <= 0 || bits > 8) {
        throw std::runtime_error("dequantize_awq_layer: bits must be in [1, 8], got " +
                                 std::to_string(bits));
    }
    if (group_size <= 0) {
        throw std::runtime_error("dequantize_awq_layer: group_size must be > 0, got " +
                                 std::to_string(group_size));
    }
    // AWQ layout (AutoAWQ convention):
    //   qweight[i, pc] – INT32, shape [in_features, out_features / vpw]
    //     weight[i, pc*vpw + b] = (qweight[i, pc] >> (b * bits)) & mask
    //
    //   qzeros[g, pc]  – INT32, shape [n_groups, out_features / vpw]
    //     zero[g, pc*vpw + b]  = (qzeros[g, pc]  >> (b * bits)) & mask
    //
    //   scales[g, j]   – FP16,  shape [n_groups, out_features]
    //
    // Dequantize: W_fp[i, j] = (weight[i,j] - zero[i/group_size, j]) * scales[i/group_size, j]
    const int vpw    = 32 / bits;
    const uint32_t mask = (1u << bits) - 1u;

    const int64_t qw_cols = (out_features + vpw - 1) / vpw;   // packed cols in qweight/qzeros
    const int     n_groups = static_cast<int>((in_features + group_size - 1) / group_size);

    const auto* qw = static_cast<const uint32_t*>(qweight_packed);
    const auto* qz = static_cast<const uint32_t*>(qzeros_packed);

    // Decode scales (FP16 → FP32), shape [n_groups, out_features]
    const size_t sc_n = static_cast<size_t>(n_groups) * static_cast<size_t>(out_features);
    std::vector<float> sc_f = fp16_to_fp32_array(scales_fp16, sc_n);

    std::vector<float> out_f(static_cast<size_t>(in_features * out_features));

    for (int64_t i = 0; i < in_features; ++i) {
        const int g = static_cast<int>(i / group_size);

        for (int64_t j = 0; j < out_features; ++j) {
            const int64_t pc = j / vpw;        // packed column in qweight/qzeros
            const int     b  = static_cast<int>(j % vpw);  // bit offset

            // Unpack weight
            const uint32_t w_packed = qw[static_cast<size_t>(i * qw_cols + pc)];
            const float w = static_cast<float>((w_packed >> (b * bits)) & mask);

            // Unpack zero-point
            const uint32_t z_packed = qz[static_cast<size_t>(g * qw_cols + pc)];
            const float z = static_cast<float>((z_packed >> (b * bits)) & mask);

            const float s = sc_f[static_cast<size_t>(g * out_features + j)];
            out_f[static_cast<size_t>(i * out_features + j)] = (w - z) * s;
        }
    }
    return out_f;
}

std::vector<float> ModelQuantizationPipeline::dequantize_gptq_layer(
    const void* qweight_packed,
    const void* qzeros_packed,
    const void* scales_fp16,
    int64_t in_features,
    int64_t out_features,
    int group_size,
    int bits)
{
    if (bits <= 0 || bits > 8) {
        throw std::runtime_error("dequantize_gptq_layer: bits must be in [1, 8], got " +
                                 std::to_string(bits));
    }
    if (group_size <= 0) {
        throw std::runtime_error("dequantize_gptq_layer: group_size must be > 0, got " +
                                 std::to_string(group_size));
    }
    // GPTQ layout (AutoGPTQ / ExLlama convention):
    //   qweight[r, c]  – INT32, shape [in_features / vpw, out_features]
    //     weight[r*vpw + b, c] = (qweight[r, c] >> (b * bits)) & mask
    //
    //   qzeros[g, pc]  – INT32, shape [n_groups, out_features / vpw]
    //     zero[g, pc*vpw + b] = (qzeros[g, pc] >> (b * bits)) & mask
    //
    //   scales[g, c]   – FP16,  shape [n_groups, out_features]
    //
    // Dequantize: W_fp[i, j] = (weight[i,j] - zero[i/group_size, j]) * scales[i/group_size, j]
    const int vpw    = 32 / bits;
    const uint32_t mask = (1u << bits) - 1u;

    const int64_t qz_cols = (out_features + vpw - 1) / vpw;
    const int     n_groups = static_cast<int>((in_features + group_size - 1) / group_size);

    const auto* qw = static_cast<const uint32_t*>(qweight_packed);
    const auto* qz = static_cast<const uint32_t*>(qzeros_packed);

    // Decode scales (FP16 → FP32), shape [n_groups, out_features]
    const size_t sc_n = static_cast<size_t>(n_groups) * static_cast<size_t>(out_features);
    std::vector<float> sc_f = fp16_to_fp32_array(scales_fp16, sc_n);

    std::vector<float> out_f(static_cast<size_t>(in_features * out_features));

    for (int64_t i = 0; i < in_features; ++i) {
        const int64_t r = i / vpw;   // packed row in qweight
        const int     b = static_cast<int>(i % vpw); // bit offset within packed row
        const int     g = static_cast<int>(i / group_size);

        for (int64_t j = 0; j < out_features; ++j) {
            // Unpack weight
            const uint32_t w_packed = qw[static_cast<size_t>(r * out_features + j)];
            const float w = static_cast<float>((w_packed >> (b * bits)) & mask);

            // Unpack zero-point
            const int64_t pc = j / vpw;    // packed col in qzeros
            const int     bz = static_cast<int>(j % vpw);
            const uint32_t z_packed = qz[static_cast<size_t>(g * qz_cols + pc)];
            const float z = static_cast<float>((z_packed >> (bz * bits)) & mask);

            const float s = sc_f[static_cast<size_t>(g * out_features + j)];
            out_f[static_cast<size_t>(i * out_features + j)] = (w - z) * s;
        }
    }
    return out_f;
}

// ---------------------------------------------------------------------------
// AWQ loader
// ---------------------------------------------------------------------------

lora::QuantizedModel ModelQuantizationPipeline::load_awq(
    const std::string& dir,
    const QuantizationPipelineConfig& cfg)
{
    if (!fs::is_directory(dir)) {
        throw std::runtime_error("AWQ model path is not a directory: " + dir);
    }

    // Read quantization parameters from config.json
    int group_size = cfg.group_size;
    int bits       = cfg.bits;

    const fs::path config_path = fs::path(dir) / "config.json";
    if (fs::exists(config_path)) {
        std::ifstream f(config_path);
        if (f) {
            auto j = nlohmann::json::parse(f, nullptr, /*exceptions=*/false);
            if (!j.is_discarded() && j.contains("quantization_config")) {
                const auto& qcfg = j["quantization_config"];
                if (qcfg.contains("w_bit")) {
                  bits       = qcfg["w_bit"].get<int>();
                }
                if (qcfg.contains("bits")) {
                  bits       = qcfg["bits"].get<int>();
                }
                if (qcfg.contains("group_size")) {
                  group_size = qcfg["group_size"].get<int>();
                }
            }
        }
    }

    spdlog::info("AWQ load: dir='{}', bits={}, group_size={}", dir, bits, group_size);

    // Determine internal quantization target
    lora::QuantizationType target = cfg.target_type;
    if (target == lora::QuantizationType::NONE) {
        target = (bits <= 4) ? lora::QuantizationType::NF4
                             : lora::QuantizationType::INT8;
    }

    lora::QuantizedModelConfig model_cfg;
    model_cfg.quantization_type = target;
    model_cfg.block_size        = cfg.block_size;
    lora::QuantizedModel model(model_cfg);

    const auto shards = find_safetensor_shards(dir);
    if (shards.empty()) {
        throw std::runtime_error(
            "AWQ model directory contains no .safetensors files: " + dir);
    }

    size_t loaded = 0;
    for (const auto& shard_path : shards) {
        spdlog::info("AWQ: parsing shard '{}'", shard_path);
        auto st = parse_safetensors(shard_path);

        // Group tensors by layer: collect weight/scales/zeros per base name.
        // AWQ naming: "model.layers.N.self_attn.q_proj.weight"  (or .qweight)
        //             "model.layers.N.self_attn.q_proj.scales"
        //             "model.layers.N.self_attn.q_proj.zeros"
        struct LayerBuffers {
            const SafeTensorDesc* weight = nullptr;
            const SafeTensorDesc* scales = nullptr;
            const SafeTensorDesc* zeros  = nullptr;
        };
        std::unordered_map<std::string, LayerBuffers> layers;

        for (const auto& [name, desc] : st.tensors) {
            // Determine base name and suffix
            auto pos_weight = name.rfind(".weight");
            auto pos_qweight = name.rfind(".qweight");
            auto pos_scales = name.rfind(".scales");
            auto pos_zeros  = name.rfind(".zeros");

            if (pos_weight != std::string::npos &&
                pos_weight == name.size() - 7) {
                layers[name.substr(0, pos_weight)].weight = &desc;
            } else if (pos_qweight != std::string::npos &&
                       pos_qweight == name.size() - 8) {
                layers[name.substr(0, pos_qweight)].weight = &desc;
            } else if (pos_scales != std::string::npos &&
                       pos_scales == name.size() - 7) {
                layers[name.substr(0, pos_scales)].scales = &desc;
            } else if (pos_zeros != std::string::npos &&
                       pos_zeros == name.size() - 6) {
                layers[name.substr(0, pos_zeros)].zeros = &desc;
            }
        }

        for (auto& [base_name, bufs] : layers) {
            if (cfg.max_tensors > 0 && loaded >= cfg.max_tensors) {
              break;
            }
            if (!bufs.weight || !bufs.scales || !bufs.zeros) {
                spdlog::debug("AWQ: skipping incomplete layer '{}'", base_name);
                continue;
            }

            const auto& wd = *bufs.weight;
            const auto& sd = *bufs.scales;
            const auto& zd = *bufs.zeros;

            if (wd.shape.size() < 2) {
              continue;
            }

            // AWQ qweight shape: [in_features, out_features / vpw]
            // (AutoAWQ packs along the output dimension)
            const int vpw       = 32 / bits;
            const int64_t in_f  = wd.shape[0];
            const int64_t out_f = wd.shape[1] * vpw;

            const void* w_ptr = st.data.data() + wd.data_begin;
            const void* z_ptr = st.data.data() + zd.data_begin;
            const void* s_ptr = st.data.data() + sd.data_begin;

            auto fp32 = dequantize_awq_layer(w_ptr, z_ptr, s_ptr,
                                             in_f, out_f, group_size, bits);

            // Convert to QuantizedTensor and store
            lora::QuantizedTensor qt(target,
                                     {static_cast<size_t>(in_f),
                                      static_cast<size_t>(out_f)},
                                     cfg.block_size);
            if (target == lora::QuantizationType::NF4) {
                lora::quantization::quantize_nf4(fp32, qt, cfg.block_size);
            } else {
                lora::quantization::quantize_int8(fp32, qt, cfg.block_size);
            }

            lora::QuantizedLayerWeights lw(std::move(qt),
                                           {static_cast<size_t>(in_f),
                                            static_cast<size_t>(out_f)});
            model.add_quantized_layer(base_name, std::move(lw));
            ++loaded;
        }
    }

    spdlog::info("AWQ load complete: {} layers loaded", loaded);
    if (loaded == 0) {
        throw std::runtime_error(
            "AWQ: no layers could be loaded from: " + dir);
    }
    return model;
}

// ---------------------------------------------------------------------------
// GPTQ loader
// ---------------------------------------------------------------------------

lora::QuantizedModel ModelQuantizationPipeline::load_gptq(
    const std::string& dir,
    const QuantizationPipelineConfig& cfg)
{
    if (!fs::is_directory(dir)) {
        throw std::runtime_error("GPTQ model path is not a directory: " + dir);
    }

    // Read quantization parameters from config.json
    int group_size = cfg.group_size;
    int bits       = cfg.bits;

    const fs::path config_path = fs::path(dir) / "config.json";
    if (fs::exists(config_path)) {
        std::ifstream f(config_path);
        if (f) {
            auto j = nlohmann::json::parse(f, nullptr, /*exceptions=*/false);
            if (!j.is_discarded() && j.contains("quantization_config")) {
                const auto& qcfg = j["quantization_config"];
                if (qcfg.contains("bits")) {
                  bits       = qcfg["bits"].get<int>();
                }
                if (qcfg.contains("group_size")) {
                  group_size = qcfg["group_size"].get<int>();
                }
            }
        }
    }

    spdlog::info("GPTQ load: dir='{}', bits={}, group_size={}", dir, bits, group_size);

    lora::QuantizationType target = cfg.target_type;
    if (target == lora::QuantizationType::NONE) {
        target = (bits <= 4) ? lora::QuantizationType::NF4
                             : lora::QuantizationType::INT8;
    }

    lora::QuantizedModelConfig model_cfg;
    model_cfg.quantization_type = target;
    model_cfg.block_size        = cfg.block_size;
    lora::QuantizedModel model(model_cfg);

    const auto shards = find_safetensor_shards(dir);
    if (shards.empty()) {
        throw std::runtime_error(
            "GPTQ model directory contains no .safetensors files: " + dir);
    }

    size_t loaded = 0;
    for (const auto& shard_path : shards) {
        spdlog::info("GPTQ: parsing shard '{}'", shard_path);
        auto st = parse_safetensors(shard_path);

        // Group by base layer name
        struct GptqBuffers {
            const SafeTensorDesc* qweight = nullptr;
            const SafeTensorDesc* qzeros  = nullptr;
            const SafeTensorDesc* scales  = nullptr;
        };
        std::unordered_map<std::string, GptqBuffers> layers;

        for (const auto& [name, desc] : st.tensors) {
            auto pos_qw = name.rfind(".qweight");
            auto pos_qz = name.rfind(".qzeros");
            auto pos_sc = name.rfind(".scales");

            if (pos_qw != std::string::npos && pos_qw == name.size() - 8) {
                layers[name.substr(0, pos_qw)].qweight = &desc;
            } else if (pos_qz != std::string::npos && pos_qz == name.size() - 7) {
                layers[name.substr(0, pos_qz)].qzeros = &desc;
            } else if (pos_sc != std::string::npos && pos_sc == name.size() - 7) {
                layers[name.substr(0, pos_sc)].scales = &desc;
            }
        }

        for (auto& [base_name, bufs] : layers) {
            if (cfg.max_tensors > 0 && loaded >= cfg.max_tensors) {
              break;
            }
            if (!bufs.qweight || !bufs.qzeros || !bufs.scales) {
                spdlog::debug("GPTQ: skipping incomplete layer '{}'", base_name);
                continue;
            }

            const auto& wd = *bufs.qweight;
            const auto& zd = *bufs.qzeros;
            const auto& sd = *bufs.scales;

            if (wd.shape.size() < 2) {
              continue;
            }

            // GPTQ qweight shape: [in_features / vpw, out_features]
            const int vpw        = 32 / bits;
            const int64_t in_f   = wd.shape[0] * vpw;
            const int64_t out_f  = wd.shape[1];

            const void* qw_ptr = st.data.data() + wd.data_begin;
            const void* qz_ptr = st.data.data() + zd.data_begin;
            const void* sc_ptr = st.data.data() + sd.data_begin;

            auto fp32 = dequantize_gptq_layer(
                qw_ptr, qz_ptr, sc_ptr,
                in_f, out_f, group_size, bits);

            lora::QuantizedTensor qt(target,
                                     {static_cast<size_t>(in_f),
                                      static_cast<size_t>(out_f)},
                                     cfg.block_size);
            if (target == lora::QuantizationType::NF4) {
                lora::quantization::quantize_nf4(fp32, qt, cfg.block_size);
            } else {
                lora::quantization::quantize_int8(fp32, qt, cfg.block_size);
            }

            lora::QuantizedLayerWeights lw(std::move(qt),
                                           {static_cast<size_t>(in_f),
                                            static_cast<size_t>(out_f)});
            model.add_quantized_layer(base_name, std::move(lw));
            ++loaded;
        }
    }

    spdlog::info("GPTQ load complete: {} layers loaded", loaded);
    if (loaded == 0) {
        throw std::runtime_error(
            "GPTQ: no layers could be loaded from: " + dir);
    }
    return model;
}

} // namespace llm
} // namespace themis


