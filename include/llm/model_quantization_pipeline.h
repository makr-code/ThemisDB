/**
 * @file model_quantization_pipeline.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "llm/lora_framework/quantized_model.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace themis {
namespace llm {

/**
 * @brief Model quantization format
 */
enum class ModelFormat {
    AUTO,   ///< Auto-detect from file extension / config.json
    GGUF,   ///< GGUF format (llama.cpp ecosystem) - .gguf file
    AWQ,    ///< Activation-aware Weight Quantization (HuggingFace SafeTensors dir)
    GPTQ,   ///< GPT Quantization (HuggingFace SafeTensors dir)
};

/**
 * @brief Configuration for model quantization pipeline loading
 *
 * Controls how model weights are loaded and represented internally.
 * Most fields are inferred from the model's config.json if not specified.
 */
struct QuantizationPipelineConfig {
    virtual ~QuantizationPipelineConfig() = default;
    /// Target internal quantization type (inferred if NONE)
    lora::QuantizationType target_type = lora::QuantizationType::NONE;
    /// Block size for internal quantization (default 64)
    size_t block_size = 64;
    /// Bits per weight in the source model (4 or 8; inferred from config)
    int bits = 4;
    /// Group size for AWQ/GPTQ (inferred from config; default 128)
    int group_size = 128;
    /// Maximum number of tensors to load (0 = no limit; useful for testing)
    size_t max_tensors = 0;
};

/**
 * @brief Unified model quantization pipeline
 *
 * Provides a single entry point for loading pre-quantized language models in
 * GGUF, AWQ, or GPTQ format and converting them to the internal
 * `lora::QuantizedModel` representation used throughout the ThemisDB LLM stack.
 *
 * Usage:
 * @code
 *   // Auto-detect format
 *   auto model = ModelQuantizationPipeline::load("llama-q4.gguf");
 *
 *   // Explicit GPTQ directory
 *   auto model = ModelQuantizationPipeline::load(
 *       "/models/llama-7b-gptq", ModelFormat::GPTQ);
 * @endcode
 *
 * Supported formats:
 * - **GGUF** (Q4_K_M, Q8_0, F16, F32): delegates to
 *   `quantized_model_utils::load_from_gguf()`
 * - **AWQ** (4-bit INT): reads SafeTensors directory; unpacks INT32-packed
 *   weight matrices with per-group FP16 scales and zero-points.
 * - **GPTQ** (4-bit or 8-bit INT): reads SafeTensors directory; unpacks
 *   `qweight`/`qzeros`/`scales` tensors.
 */
class ModelQuantizationPipeline {
public:
    virtual ~ModelQuantizationPipeline() = default;
    /**
     * @brief Load a pre-quantized model
     *
     * @param path  Path to the model file (.gguf) or model directory
     *              containing safetensor shards and config.json (AWQ/GPTQ).
     * @param format  Format hint; `AUTO` probes the path automatically.
     * @param config  Optional loading configuration (bits, group_size, etc.).
     * @return Loaded quantized model
     * @throws std::runtime_error on unsupported format, missing files, or
     *         parse errors.
     */
    static lora::QuantizedModel load(
        const std::string& path,
        ModelFormat format = ModelFormat::AUTO,
        const QuantizationPipelineConfig& config = {});

    /**
     * @brief Detect the quantization format of a model path
     *
     * Inspects the file extension (.gguf) or directory contents
     * (config.json quant_type field) to determine the format.
     *
     * @param path  File or directory path
     * @return Detected format, or `ModelFormat::GGUF` as fallback
     */
    static ModelFormat detect_format(const std::string& path);

    /**
     * @brief Return a human-readable name for a format enum value
     */
    static const char* format_name(ModelFormat fmt);

private:
    // ---- Format-specific loaders ----------------------------------------

    static lora::QuantizedModel load_gguf(
        const std::string& path,
        const QuantizationPipelineConfig& cfg);

    static lora::QuantizedModel load_awq(
        const std::string& dir,
        const QuantizationPipelineConfig& cfg);

    static lora::QuantizedModel load_gptq(
        const std::string& dir,
        const QuantizationPipelineConfig& cfg);

    // ---- SafeTensors helpers --------------------------------------------

    /**
     * @brief Single tensor descriptor parsed from a SafeTensors header
     */
    struct SafeTensorDesc {
        std::string dtype;               ///< "F16", "F32", "I32", "I16", etc.
        std::vector<int64_t> shape;      ///< Tensor dimensions
        uint64_t data_begin = 0;         ///< Byte offset in data region
        uint64_t data_end   = 0;         ///< Exclusive end offset
    };

    /**
     * @brief Parsed SafeTensors file: header map + raw data buffer
     */
    struct SafeTensorsFile {
        std::unordered_map<std::string, SafeTensorDesc> tensors;
        std::vector<uint8_t> data;       ///< Raw binary payload
    };

    /**
     * @brief Parse a single .safetensors file
     *
     * @param file_path  Path to the .safetensors file
     * @return Parsed file
     * @throws std::runtime_error on IO or format errors
     */
    static SafeTensorsFile parse_safetensors(const std::string& file_path);

    /**
     * @brief Collect all .safetensors shard paths in a directory
     *
     * Searches for `*.safetensors` files in `dir`.
     */
    static std::vector<std::string> find_safetensor_shards(
        const std::string& dir);

    // ---- Weight unpacking helpers ----------------------------------------

    /**
     * @brief Unpack INT32-packed 4-bit weights to FP32
     *
     * Each INT32 stores 8 consecutive 4-bit quantized values packed
     * from LSB (bit 0) to MSB (bit 31).
     *
     * @param packed_data  Pointer to raw INT32 data
     * @param n_packed     Number of INT32 elements
     * @param bits         Bits per value (default 4)
     * @return Unpacked integer values as float
     */
    static std::vector<float> unpack_int32_weights(
        const void* packed_data,
        size_t n_packed,
        int bits = 4);

    /**
     * @brief Convert FP16 array to FP32
     *
     * @param fp16_data  Pointer to raw FP16 (uint16_t) data
     * @param n          Number of elements
     */
    static std::vector<float> fp16_to_fp32_array(
        const void* fp16_data,
        size_t n);

    /**
     * @brief Dequantize AWQ layer weights to FP32
     *
     * AWQ packs weights along the output-feature dimension:
     * - qweight shape: [in_features, out_features / (32/bits)]
     * - qzeros  shape: [n_groups,   out_features / (32/bits)]
     * - scales  shape: [n_groups,   out_features]
     *
     * weight[i, j] = (qweight[i, j/vpw] >> ((j % vpw) * bits)) & mask
     * zero[g, j]   = (qzeros[g, j/vpw]  >> ((j % vpw) * bits)) & mask
     * fp_weight[i, j] = (weight[i,j] - zero[i/group_size, j]) * scales[i/group_size, j]
     *
     * @param qweight_packed  Raw INT32 data from weight/qweight tensor
     * @param qzeros_packed   Raw INT32 data from zeros/qzeros tensor
     * @param scales_fp16     Raw FP16 data from scales tensor
     * @param in_features     Input feature dimension
     * @param out_features    Output feature dimension
     * @param group_size      Quantization group size (along in_features)
     * @param bits            Bits per weight (4 or 8)
     */
    static std::vector<float> dequantize_awq_layer(
        const void* qweight_packed,
        const void* qzeros_packed,
        const void* scales_fp16,
        int64_t in_features,
        int64_t out_features,
        int group_size,
        int bits);

    /**
     * @brief Dequantize GPTQ layer weights to FP32
     *
     * GPTQ stores qweight with shape [in_features / (32/bits), out_features]
     * and qzeros with shape [n_groups, out_features / (32/bits)].
     *
     * @param qweight_packed  Raw INT32 data from qweight tensor
     * @param qzeros_packed   Raw INT32 data from qzeros tensor
     * @param scales_fp16     Raw FP16 data from scales tensor
     * @param in_features     Input feature dimension
     * @param out_features    Output feature dimension
     * @param group_size      Quantization group size
     * @param bits            Bits per weight (4 or 8)
     */
    static std::vector<float> dequantize_gptq_layer(
        const void* qweight_packed,
        const void* qzeros_packed,
        const void* scales_fp16,
        int64_t in_features,
        int64_t out_features,
        int group_size,
        int bits);
};

} // namespace llm
} // namespace themis
