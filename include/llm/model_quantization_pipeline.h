/**
 * @file model_quantization_pipeline.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

enum class ModelFormat {
    AUTO,   ///< Auto-detect from file extension / config.json
    GGUF,   ///< GGUF format (llama.cpp ecosystem) - .gguf file
    AWQ,    ///< Activation-aware Weight Quantization (HuggingFace SafeTensors dir)
    GPTQ,   ///< GPT Quantization (HuggingFace SafeTensors dir)
};

struct QuantizationPipelineConfig {
    /**
     * @brief Quantization Pipeline Config.
     * @return Return value.
     */
    virtual ~QuantizationPipelineConfig() = default;
    lora::QuantizationType target_type = lora::QuantizationType::NONE;
    size_t block_size = 64;
    int bits = 4;
    int group_size = 128;
    size_t max_tensors = 0;
};

class ModelQuantizationPipeline {
public:
    /**
     * @brief Model Quantization Pipeline.
     * @return Return value.
     */
    virtual ~ModelQuantizationPipeline() = default;
    static lora::QuantizedModel load(
        const std::string& path,
        ModelFormat format = ModelFormat::AUTO,
        const QuantizationPipelineConfig& config = {});

    /**
     * @brief Detect format.
     * @param[in] path Input parameter.
     * @return Return value.
     */
    static ModelFormat detect_format(const std::string& path);

    /**
     * @brief Format name.
     * @param[in] fmt Input parameter.
     * @return Pointer to the result.
     */
    static const char* format_name(ModelFormat fmt);

private:
    /**
     * @brief ---- Format-specific loaders ----------------------------------------
     * @param[in] path Input parameter.
     * @param[in] cfg Input parameter.
     * @return Return value.
     */

    static lora::QuantizedModel load_gguf(
        const std::string& path,
        const QuantizationPipelineConfig& cfg);

    /**
     * @brief Load awq.
     * @param[in] dir Input parameter.
     * @param[in] cfg Input parameter.
     * @return Return value.
     */
    static lora::QuantizedModel load_awq(
        const std::string& dir,
        const QuantizationPipelineConfig& cfg);

    /**
     * @brief Load gptq.
     * @param[in] dir Input parameter.
     * @param[in] cfg Input parameter.
     * @return Return value.
     */
    static lora::QuantizedModel load_gptq(
        const std::string& dir,
        const QuantizationPipelineConfig& cfg);

    // ---- SafeTensors helpers --------------------------------------------

    struct SafeTensorDesc {
        std::string dtype;               ///< "F16", "F32", "I32", "I16", etc.
        std::vector<int64_t> shape;      ///< Tensor dimensions
        uint64_t data_begin = 0;         ///< Byte offset in data region
        uint64_t data_end   = 0;         ///< Exclusive end offset
    };

    struct SafeTensorsFile {
        std::unordered_map<std::string, SafeTensorDesc> tensors;
        std::vector<uint8_t> data;       ///< Raw binary payload
    };

    /**
     * @brief Parse safetensors.
     * @param[in] file_path Path to the file.
     * @return Return value.
     */
    static SafeTensorsFile parse_safetensors(const std::string& file_path);

    /**
     * @brief Find safetensor shards.
     * @param[in] dir Input parameter.
     * @return Return value.
     */
    static std::vector<std::string> find_safetensor_shards(
        const std::string& dir);

    // ---- Weight unpacking helpers ----------------------------------------

    static std::vector<float> unpack_int32_weights(
        const void* packed_data,
        size_t n_packed,
        int bits = 4);

    /**
     * @brief Fp16 to fp32 array.
     * @param[in] fp16_data Input parameter.
     * @param[in] n Input parameter.
     * @return Return value.
     */
    static std::vector<float> fp16_to_fp32_array(
        const void* fp16_data,
        size_t n);

    /**
     * @brief Dequantize awq layer.
     * @param[in] qweight_packed Input parameter.
     * @param[in] qzeros_packed Input parameter.
     * @param[in] scales_fp16 Input parameter.
     * @param[in] in_features Input parameter.
     * @param[in] out_features Input parameter.
     * @param[in] group_size Input parameter.
     * @param[in] bits Input parameter.
     * @return Return value.
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
     * @brief Dequantize gptq layer.
     * @param[in] qweight_packed Input parameter.
     * @param[in] qzeros_packed Input parameter.
     * @param[in] scales_fp16 Input parameter.
     * @param[in] in_features Input parameter.
     * @param[in] out_features Input parameter.
     * @param[in] group_size Input parameter.
     * @param[in] bits Input parameter.
     * @return Return value.
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
