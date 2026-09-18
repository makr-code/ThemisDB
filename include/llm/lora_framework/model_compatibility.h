/**
 * @file model_compatibility.h
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
#include <optional>
#include <map>
#include <nlohmann/json.hpp>

namespace themis {
namespace llm {
namespace lora {

using json = nlohmann::json;

/**
 * @brief Supported model formats
 */
enum class ModelFormat {
    UNKNOWN,
    GGUF,           // llama.cpp GGUF format
    SAFETENSORS,    // HuggingFace SafeTensors
    PYTORCH,        // PyTorch .pt/.pth
    ONNX,           // ONNX format
    TENSORFLOW      // TensorFlow SavedModel
};

/**
 * @brief Supported model architectures
 */
enum class ModelArchitecture {
    UNKNOWN,
    LLAMA,          // LLaMA 1, 2, 3
    MISTRAL,        // Mistral 7B
    MIXTRAL,        // Mixtral MoE
    GPT2,           // GPT-2
    GPTJ,           // GPT-J
    GPTNEOX,        // GPT-NeoX
    MPT,            // MPT
    FALCON,         // Falcon
    BAICHUAN,       // Baichuan
    QWEN,           // Qwen
    STABLELM        // StableLM
};

/**
 * @brief Model metadata extracted from file
 */
struct ModelMetadata {
    /**
     * @brief TBD: Describe ~ModelMetadata.
     * @return Return value.
     */
    virtual ~ModelMetadata() = default;
    std::string model_path;
    ModelFormat format = ModelFormat::UNKNOWN;
    ModelArchitecture architecture = ModelArchitecture::UNKNOWN;
    
    // Model dimensions
    size_t vocab_size = 0;
    size_t hidden_size = 0;
    size_t num_layers = 0;
    size_t num_heads = 0;
    size_t intermediate_size = 0;
    size_t max_seq_length = 0;
    
    // Quantization info
    bool is_quantized = false;
    std::string quantization_type;  // "Q4_0", "Q4_K_M", "Q8_0", "nf4", "int8", etc.
    
    // Additional metadata
    std::string model_type;
    std::string tokenizer_type;
    std::map<std::string, std::string> custom_metadata;
    
    json toJSON() const {
        return json{
            {"model_path", model_path},
            {"format", format_to_string(format)},
            {"architecture", architecture_to_string(architecture)},
            {"vocab_size", vocab_size},
            {"hidden_size", hidden_size},
            {"num_layers", num_layers},
            {"num_heads", num_heads},
            {"intermediate_size", intermediate_size},
            {"max_seq_length", max_seq_length},
            {"is_quantized", is_quantized},
            {"quantization_type", quantization_type},
            {"model_type", model_type},
            {"tokenizer_type", tokenizer_type},
            {"custom_metadata", custom_metadata}
        };
    }
    
    /**
     * @brief TBD: Describe format_to_string.
     * @param[in] fmt Input parameter.
     * @return Return value.
     */
    static std::string format_to_string(ModelFormat fmt);
    /**
     * @brief TBD: Describe architecture_to_string.
     * @param[in] arch Input parameter.
     * @return Return value.
     */
    static std::string architecture_to_string(ModelArchitecture arch);
    /**
     * @brief TBD: Describe string_to_format.
     * @param[in] str Input parameter.
     * @return Return value.
     */
    static ModelFormat string_to_format(const std::string& str);
    /**
     * @brief TBD: Describe string_to_architecture.
     * @param[in] str Input parameter.
     * @return Return value.
     */
    static ModelArchitecture string_to_architecture(const std::string& str);
};

/**
 * @brief Compatibility check result
 */
struct CompatibilityResult {
    bool is_compatible = false;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    std::string reason;
    
    // Recommendations
    std::string recommended_quantization;
    std::vector<std::string> recommended_target_modules;
    size_t recommended_rank = 8;
    size_t recommended_batch_size = 4;
    
    /**
     * @brief TBD: Describe add_error.
     * @param[in] error Input parameter.
     * @details Calls: push_back().
     */
    void add_error(const std::string& error) {
        errors.push_back(error);
        is_compatible = false;
    }
    
    /**
     * @brief TBD: Describe add_warning.
     * @param[in] warning Input parameter.
     * @details Calls: push_back().
     */
    void add_warning(const std::string& warning) {
        warnings.push_back(warning);
    }
    
    json toJSON() const {
        return json{
            {"is_compatible", is_compatible},
            {"errors", errors},
            {"warnings", warnings},
            {"reason", reason},
            {"recommended_quantization", recommended_quantization},
            {"recommended_target_modules", recommended_target_modules},
            {"recommended_rank", recommended_rank},
            {"recommended_batch_size", recommended_batch_size}
        };
    }
};

/**
 * @brief Model compatibility checker for QLoRA training
 * 
 * Validates model format, architecture, and quantization compatibility
 * before starting QLoRA training.
 */
class ModelCompatibilityChecker {
public:
    /**
     * @brief Detect model format from file
     * @param model_path Path to model file
     * @return Detected format
     */
    static ModelFormat detect_format(const std::string& model_path);
    
    /**
     * @brief Extract model metadata from file
     * @param model_path Path to model file
     * @return Model metadata
     */
    static std::optional<ModelMetadata> extract_metadata(const std::string& model_path);
    
    /**
     * @brief Check if model is compatible with QLoRA training
     * @param model_path Path to model file
     * @param quantization_type Desired quantization type ("nf4", "int8", etc.)
     * @return Compatibility result
     */
    static CompatibilityResult check_compatibility(
        const std::string& model_path,
        const std::string& quantization_type = "nf4"
    );
    
    /**
     * @brief Validate model architecture for LoRA
     * @param metadata Model metadata
     * @return Compatibility result
     */
    static CompatibilityResult validate_architecture(const ModelMetadata& metadata);
    
    /**
     * @brief Check quantization compatibility
     * @param metadata Model metadata
     * @param target_quantization Desired quantization type
     * @return Compatibility result
     */
    static CompatibilityResult check_quantization_compatibility(
        const ModelMetadata& metadata,
        const std::string& target_quantization
    );
    
    /**
     * @brief Get recommended LoRA target modules for architecture
     * @param architecture Model architecture
     * @return List of recommended target modules
     */
    static std::vector<std::string> get_recommended_target_modules(
        ModelArchitecture architecture
    );
    
    /**
     * @brief Estimate memory requirements for QLoRA training
     * @param metadata Model metadata
     * @param quantization_type Quantization type
     * @param batch_size Training batch size
     * @param rank LoRA rank
     * @return Estimated memory in bytes
     */
    static size_t estimate_memory_requirements(
        const ModelMetadata& metadata,
        const std::string& quantization_type,
        size_t batch_size,
        size_t rank
    );

private:
    /**
     * @brief Read GGUF metadata
     * @param[in] path Input parameter.
     * @return Return value.
     */
    static std::optional<ModelMetadata> read_gguf_metadata(const std::string& path);
    
    /**
     * @brief Read SafeTensors metadata
     * @param[in] path Input parameter.
     * @return Return value.
     */
    static std::optional<ModelMetadata> read_safetensors_metadata(const std::string& path);
    
    /**
     * @brief Detect architecture from metadata
     * @param[in] metadata Input parameter.
     * @return Return value.
     */
    static ModelArchitecture detect_architecture(const json& metadata);
    
    /**
     * @brief Get quantization memory reduction factor
     * @param[in] quant_type Input parameter.
     * @return Return value.
     */
    static float get_quantization_reduction(const std::string& quant_type);
};

} // namespace lora
} // namespace llm
} // namespace themis
