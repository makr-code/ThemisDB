/**
 * @file mixed_precision_inference.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace themis {
namespace llm {

enum class PrecisionMode {
    FP32,      // Full precision (32-bit floats)
    FP16,      // Half precision (16-bit floats)
    BFLOAT16,  // Brain float (16-bit with larger exponent)
    INT8,      // 8-bit quantization
    Q4,        // 4-bit quantization
    Q3,        // 3-bit quantization (experimental)
    AUTO       // Auto-select based on VRAM availability
};

struct ModelArchitecture {
    /**
     * @brief Model Architecture.
     * @return Return value.
     */
    virtual ~ModelArchitecture() = default;
    std::string model_name;
    size_t num_parameters = 0;
    size_t num_layers = 0;
    size_t hidden_dim = 0;
    std::vector<std::string> layer_types;  // e.g., ["attention", "mlp", ...]
    std::vector<size_t> layer_sizes;       // Size in bytes per layer
};

class MixedPrecisionInference {
public:
    struct PrecisionInfo {
        PrecisionMode mode;
        float accuracy_retention = 0.0f;  // 0.0 - 1.0 (1.0 = 100% accuracy)
        float memory_reduction = 0.0f;    // 0.0 - 1.0 (0.5 = 50% reduction)
        size_t bytes_per_param = 0;       // Bytes per parameter
        std::string description;   // Human-readable description
    };

    struct LayerPrecisionConfig {
        size_t layer_id = 0;
        PrecisionMode precision;
        std::string rationale;  // Why this precision was chosen
    };

    MixedPrecisionInference();
    ~MixedPrecisionInference();

    PrecisionMode selectOptimalPrecision(
        size_t available_vram,
        size_t model_size,
        float tolerance = 0.01f  // 1% accuracy loss tolerance
    );

    /**
     * @brief Get Tuning Schedule.
     * @param[in] arch Input parameter.
     * @param[in] available_vram Input parameter.
     * @return Return value.
     */
    std::vector<LayerPrecisionConfig> getTuningSchedule(
        const ModelArchitecture& arch,
        size_t available_vram
    );

    /**
     * @brief Calculate Model Size.
     * @param[in] num_parameters Input parameter.
     * @param[in] precision Input parameter.
     * @return Return value.
     */
    static size_t calculateModelSize(
        size_t num_parameters,
        PrecisionMode precision
    );

    /**
     * @brief Get Precision Info.
     * @param[in] precision Input parameter.
     * @return Return value.
     */
    static PrecisionInfo getPrecisionInfo(PrecisionMode precision);

    /**
     * @brief Get All Precisions.
     * @return Return value.
     */
    static std::vector<PrecisionInfo> getAllPrecisions();

    /**
     * @brief Calculate Expected Accuracy.
     * @param[in] precision Input parameter.
     * @return Return value.
     */
    static float calculateExpectedAccuracy(PrecisionMode precision);

    /**
     * @brief Calculate Memory Reduction.
     * @param[in] precision Input parameter.
     * @return Return value.
     */
    static float calculateMemoryReduction(PrecisionMode precision);

    /**
     * @brief From String.
     * @param[in] str Input parameter.
     * @return Return value.
     */
    static PrecisionMode fromString(const std::string& str);

    /**
     * @brief To String.
     * @param[in] precision Input parameter.
     * @return Return value.
     */
    static std::string toString(PrecisionMode precision);

    /**
     * @brief Is Supported.
     * @param[in] precision Input parameter.
     * @return True when the operation succeeds.
     */
    static bool isSupported(PrecisionMode precision);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace llm
} // namespace themis
