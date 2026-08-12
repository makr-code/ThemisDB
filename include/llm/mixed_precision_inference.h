/**
 * @file mixed_precision_inference.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Precision mode for mixed precision inference
 * 
 * Supports various quantization levels with different accuracy/memory trade-offs.
 * Based on research showing:
 * - FP32: Perfect accuracy, Maximum VRAM
 * - FP16: ~99.9% accuracy, 50% VRAM
 * - INT8: ~98% accuracy, 75% VRAM reduction
 * - Q4: ~95% accuracy, 87.5% VRAM reduction
 */
enum class PrecisionMode {
    FP32,      // Full precision (32-bit floats)
    FP16,      // Half precision (16-bit floats)
    BFLOAT16,  // Brain float (16-bit with larger exponent)
    INT8,      // 8-bit quantization
    Q4,        // 4-bit quantization
    Q3,        // 3-bit quantization (experimental)
    AUTO       // Auto-select based on VRAM availability
};

/**
 * @brief Model architecture information
 */
struct ModelArchitecture {
    virtual ~ModelArchitecture() = default;
    std::string model_name;
    size_t num_parameters = 0;
    size_t num_layers = 0;
    size_t hidden_dim = 0;
    std::vector<std::string> layer_types;  // e.g., ["attention", "mlp", ...]
    std::vector<size_t> layer_sizes;       // Size in bytes per layer
};

/**
 * @brief Mixed Precision Inference engine
 * 
 * Enables automatic precision selection and per-layer precision tuning
 * for optimal memory/accuracy trade-offs.
 */
class MixedPrecisionInference {
public:
    /**
     * @brief Precision trade-off information
     */
    struct PrecisionInfo {
        PrecisionMode mode;
        float accuracy_retention = 0.0f;  // 0.0 - 1.0 (1.0 = 100% accuracy)
        float memory_reduction = 0.0f;    // 0.0 - 1.0 (0.5 = 50% reduction)
        size_t bytes_per_param = 0;       // Bytes per parameter
        std::string description;   // Human-readable description
    };

    /**
     * @brief Per-layer precision configuration
     */
    struct LayerPrecisionConfig {
        size_t layer_id = 0;
        PrecisionMode precision;
        std::string rationale;  // Why this precision was chosen
    };

    MixedPrecisionInference();
    ~MixedPrecisionInference();

    /**
     * @brief Select optimal precision mode
     * 
     * Automatically selects the highest precision that fits in available VRAM.
     * 
     * @param available_vram Available VRAM in bytes
     * @param model_size Model size in bytes (at FP32)
     * @param tolerance Acceptable accuracy loss (default: 1%)
     * @return Recommended precision mode
     */
    PrecisionMode selectOptimalPrecision(
        size_t available_vram,
        size_t model_size,
        float tolerance = 0.01f  // 1% accuracy loss tolerance
    );

    /**
     * @brief Get per-layer precision tuning schedule
     * 
     * Optimally distributes precision across layers based on:
     * - Layer importance (attention layers use higher precision)
     * - Available VRAM budget
     * - Target accuracy
     * 
     * @param arch Model architecture
     * @param available_vram Available VRAM in bytes
     * @return Per-layer precision configuration
     */
    std::vector<LayerPrecisionConfig> getTuningSchedule(
        const ModelArchitecture& arch,
        size_t available_vram
    );

    /**
     * @brief Calculate model size with given precision
     * 
     * @param num_parameters Number of model parameters
     * @param precision Precision mode
     * @return Total model size in bytes
     */
    static size_t calculateModelSize(
        size_t num_parameters,
        PrecisionMode precision
    );

    /**
     * @brief Get precision information
     * 
     * @param precision Precision mode
     * @return Detailed precision information
     */
    static PrecisionInfo getPrecisionInfo(PrecisionMode precision);

    /**
     * @brief Get all available precision modes
     * 
     * @return List of all supported precision modes with info
     */
    static std::vector<PrecisionInfo> getAllPrecisions();

    /**
     * @brief Calculate expected accuracy with precision
     * 
     * @param precision Precision mode
     * @return Expected accuracy retention (0.0 - 1.0)
     */
    static float calculateExpectedAccuracy(PrecisionMode precision);

    /**
     * @brief Calculate memory reduction with precision
     * 
     * @param precision Precision mode
     * @return Memory reduction factor (0.0 - 1.0)
     */
    static float calculateMemoryReduction(PrecisionMode precision);

    /**
     * @brief Get precision mode from string
     * 
     * @param str Precision mode string (e.g., "FP16", "INT8")
     * @return Precision mode
     */
    static PrecisionMode fromString(const std::string& str);

    /**
     * @brief Convert precision mode to string
     * 
     * @param precision Precision mode
     * @return String representation
     */
    static std::string toString(PrecisionMode precision);

    /**
     * @brief Check if precision is supported on current hardware
     * 
     * @param precision Precision mode
     * @return true if supported
     */
    static bool isSupported(PrecisionMode precision);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace llm
} // namespace themis
