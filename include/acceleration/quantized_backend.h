/*
 * ThemisDB | File: quantized_backend.h | Version: 0.1.0 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 141
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=n/a | delta=n/a | status=no_external_data
 * External Severity (v3): C=n/a, H=n/a, M=n/a
 * PR: none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/**
 * @file quantized_backend.h
 * @brief QuantizedBackend: INT4/INT8 quantised inference interface
 *
 * Extends IComputeBackend with quantisation primitives for low-bit inference,
 * including symmetric/asymmetric INT8, grouped INT4, and FP8 formats.
 *
 * Target: Q3 2026
 */

#pragma once

#include "acceleration/compute_backend.h"
#include <vector>
#include <cstdint>

namespace themis {
namespace acceleration {

/// Quantisation schemes supported by IQuantizedBackend implementations.
enum class QuantizationScheme {
    INT8_SYMMETRIC,  ///< Symmetric INT8: scale per tensor or channel, zero_point = 0
    INT8_ASYMMETRIC, ///< Asymmetric INT8: scale + zero_point per tensor or channel
    INT4_SYMMETRIC,  ///< Symmetric 4-bit integer quantisation
    INT4_GROUPED,    ///< Grouped INT4: independent scale per group of weights
    FP8_E4M3,        ///< FP8 E4M3 format (NVIDIA Hopper, Ada)
    FP8_E5M2,        ///< FP8 E5M2 format
};

/// Parameters controlling how tensors are quantised.
struct QuantizationConfig {
    QuantizationScheme scheme = QuantizationScheme::INT8_SYMMETRIC;
    int group_size = 128;                  ///< Group size for INT4_GROUPED scheme
    bool per_channel = false;              ///< Per-channel (true) vs per-tensor (false)
    bool enable_calibration = false;       ///< Run post-training calibration pass
    float calibration_percentile = 99.9f;  ///< Percentile clipping for calibration range
};

/// A low-bit tensor in quantised form, ready for quantised kernels.
struct QuantizedTensor {
    std::vector<int8_t> data;       ///< INT8 quantised values (INT8 path)
    std::vector<int32_t> data_i32;  ///< INT4 values packed as INT32 (8 values per element)
    std::vector<float> scales;      ///< Per-channel or per-group scale factors
    std::vector<int8_t> zero_points;///< Zero-point offsets (asymmetric schemes only)
    std::vector<size_t> shape;      ///< Logical tensor shape
    QuantizationScheme scheme = QuantizationScheme::INT8_SYMMETRIC;
    size_t original_dtype_bytes = 4;///< Original element size: 4=float32, 2=float16
};

/// Statistics produced by a calibration pass.
struct QuantizationStats {
    double compression_ratio = 0.0;     ///< Compressed size / original size
    double peak_snr_db = 0.0;           ///< Peak signal-to-noise ratio in dB
    size_t calibration_samples = 0;     ///< Number of representative samples used
    double quantization_error_l2 = 0.0; ///< L2 norm of (dequantised - original)
};

/**
 * @brief Abstract interface for backends that support low-bit quantised inference.
 *
 * Inherits IComputeBackend capabilities and adds quantise/dequantise primitives,
 * a quantised matrix multiply, and a calibration pass for post-training
 * quantisation (PTQ).
 *
 * Thread safety: implementations must document their own thread-safety guarantees.
 */
class IQuantizedBackend : public IComputeBackend {
public:
    ~IQuantizedBackend() override = default;

    /**
     * @brief Quantise a float tensor to the configured low-bit scheme.
     * @param tensor   Flat float values in row-major order.
     * @param shape    Logical shape of the tensor.
     * @param config   Quantisation configuration (scheme, group_size, etc.).
     * @return QuantizedTensor ready for quantised kernels.
     */
    [[nodiscard]] virtual QuantizedTensor quantize(
        const std::vector<float>& tensor,
        const std::vector<size_t>& shape,
        const QuantizationConfig& config
    ) = 0;

    /**
     * @brief Dequantise a QuantizedTensor back to float32.
     * @param qtensor  Previously quantised tensor.
     * @return Flat float values approximating the original tensor.
     */
    [[nodiscard]] virtual std::vector<float> dequantize(const QuantizedTensor& qtensor) = 0;

    /**
     * @brief Quantised matrix multiply: A (M×K) × B (K×N) → C (M×N, float32).
     *
     * Both operands are expected to carry compatible schemes. The result is
     * accumulated in float32 to preserve precision.
     *
     * @param a  Left operand, quantised.
     * @param b  Right operand, quantised.
     * @param M  Rows of A.
     * @param K  Inner dimension.
     * @param N  Columns of B.
     * @return Flat float32 result of shape M×N.
     */
    [[nodiscard]] virtual std::vector<float> quantizedMatmul(
        const QuantizedTensor& a,
        const QuantizedTensor& b,
        size_t M, size_t K, size_t N
    ) = 0;

    /**
     * @brief Calibrate quantisation ranges from representative dataset samples.
     *
     * Runs a forward-statistics pass over @p samples to determine optimal
     * scale/zero-point values for the given configuration.
     *
     * @param samples  Representative input tensors (flat float32 each).
     * @param config   Quantisation configuration governing the calibration.
     * @return QuantizationStats with compression ratio, SNR, and error metrics.
     */
    [[nodiscard]] virtual QuantizationStats calibrate(
        const std::vector<std::vector<float>>& samples,
        const QuantizationConfig& config
    ) = 0;

    /// Returns the list of QuantizationSchemes supported by this backend.
    [[nodiscard]] virtual std::vector<QuantizationScheme> supportedSchemes() const = 0;

    /// Returns true if INT4 operations are natively accelerated (not emulated).
    [[nodiscard]] virtual bool hasNativeInt4Support() const = 0;
};

} // namespace acceleration
} // namespace themis
