/*
 * ThemisDB | File: tt_quantizer.h | Version: 1.0.0
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/**
 * @file tt_quantizer.h
 * @brief Post-TT-decomposition quantisation of TT-core tensors.
 *
 * Applies INT8 or NF4 (Normal Float 4-bit) quantisation to the individual
 * TT-cores produced by `TensorTrainDecomposer`.  This yields a second-stage
 * compression on top of the TT rank reduction.
 *
 * ### INT8 (channel-wise)
 * Per-core scaling: scale_k = max(|G_k|) / 127.  Quantised value:
 *   q = clamp(round(v / scale_k), -128, 127).
 * Dequantisation: v ≈ q · scale_k.
 * Storage cost: 1 byte/element + 4 bytes/core for scale → ~4× vs float32.
 *
 * ### NF4 (Dettmers et al., 2023 — QLoRA)
 * Uses a 16-element lookup table derived from the quantiles of a unit normal
 * distribution, mapped to [−1, 1].  Optimal for weights drawn from N(0, σ²)
 * (validated for LLM attention weight matrices).
 * Two NF4 values are packed per byte → ~8× vs float32.
 *
 * ### References
 * - Dettmers, T., Pagnoni, A., Holtzman, A., & Zettlemoyer, L. (2023).
 *   QLoRA: Efficient Finetuning of Quantized LLMs. NeurIPS 2023.
 *   arXiv:2305.14314
 * - Khoromskij, B. N. (2011). O(d log n)-quantics approximation of n^d tensors.
 *   Constructive Approximation, 34(2), 257–280.
 */

#pragma once

#include "storage/tensor_train_decomposer.h"

#include <cstdint>
#include <string>
#include <vector>

namespace themis {
namespace storage {

// ============================================================================
// QuantizationType
// ============================================================================

/**
 * @brief Supported quantisation modes for TT-cores.
 */
enum class QuantizationType : uint8_t {
    NONE  = 0,  ///< No quantisation — store raw float32 cores
    INT8  = 1,  ///< Symmetric per-core INT8 (channel-wise scaling)
    NF4   = 2,  ///< Normal Float 4-bit (Dettmers 2023) — optimal for LLM weights
};

// ============================================================================
// QuantizedCore — one quantised TT-core
// ============================================================================

/**
 * @brief A TT-core after quantisation.
 *
 * Stores compressed bytes plus per-core metadata needed for dequantisation.
 */
struct QuantizedCore {
    /// Original core dimensions
    std::size_t r_left  = 1;
    std::size_t n       = 1;
    std::size_t r_right = 1;

    QuantizationType quant_type = QuantizationType::NONE;

    /// Compressed data bytes.
    /// - NONE:  raw float32 (4 bytes/element)
    /// - INT8:  1 byte/element (signed)
    /// - NF4:   ceil(elements/2) bytes (two 4-bit indices per byte)
    std::vector<uint8_t> data;

    /// Per-core absolute-max scale factor (used by INT8)
    float scale = 1.0f;

    /// Original mean (used for NF4 centre correction)
    float mean  = 0.0f;

    std::size_t numElements() const noexcept { return r_left * n * r_right; }

    /// Serialise to bytes
    std::vector<uint8_t> serialize() const;

    /// Deserialise from bytes
    static std::optional<QuantizedCore> deserialize(const std::vector<uint8_t>& bytes);
};

// ============================================================================
// QuantizedTrain
// ============================================================================

/**
 * @brief A full TT-train after core-wise quantisation.
 */
struct QuantizedTrain {
    std::vector<std::size_t> mode_sizes;
    std::vector<QuantizedCore> cores;
    QuantizationType quant_type = QuantizationType::NONE;
    double original_norm  = 0.0;
    double achieved_eps   = 0.0;

    std::size_t order() const noexcept { return cores.size(); }

    /// Total compressed bytes across all cores
    std::size_t totalBytes() const noexcept;

    /// Compression ratio: (dense float32 elements × 4) / totalBytes()
    double compressionRatio() const noexcept;

    /// Serialise to bytes for RocksDB storage
    std::vector<uint8_t> serialize() const;

    /// Deserialise from bytes
    static std::optional<QuantizedTrain> deserialize(const std::vector<uint8_t>& bytes);
};

// ============================================================================
// TTQuantizer
// ============================================================================

/**
 * @brief Applies post-decomposition quantisation to TT-cores.
 *
 * ### Usage
 * @code
 * TTQuantizer quant;
 * auto qtrain = quant.quantize(train, QuantizationType::NF4);
 * auto dequant = quant.dequantize(qtrain);  // back to TTTrain
 * @endcode
 */
class TTQuantizer {
public:
    TTQuantizer() = default;
    ~TTQuantizer() = default;

    // ─── Quantisation ─────────────────────────────────────────────────────

    /**
     * @brief Quantise every core of a TTTrain.
     *
     * @param train  Source TT-train (float32 cores).
     * @param type   Target quantisation type.
     * @return       QuantizedTrain.
     * @throws std::invalid_argument if train.cores is empty.
     */
    QuantizedTrain quantize(const TTTrain& train,
                            QuantizationType type = QuantizationType::INT8) const;

    /**
     * @brief Dequantise back to a (lossy) TTTrain with float32 cores.
     */
    TTTrain dequantize(const QuantizedTrain& qtrain) const;

    // ─── Helpers ──────────────────────────────────────────────────────────

    /// Human-readable name of a QuantizationType.
    static std::string typeName(QuantizationType t) noexcept;

    /// Bytes per element for a given type (fractional for NF4 → use 0.5).
    static double bytesPerElement(QuantizationType t) noexcept;

private:
    // ── NF4 lookup table (16 quantiles of N(0,1) mapped to [-1,1]) ──────
    // Source: Dettmers et al. (2023), Table 1.
    static constexpr float kNF4Table[16] = {
        -1.0f,       -0.6961928f, -0.5250730f, -0.3949342f,
        -0.2844073f, -0.1848549f, -0.0919652f,  0.0f,
         0.0796474f,  0.1609302f,  0.2461564f,  0.3379999f,
         0.4407302f,  0.5626170f,  0.7229568f,  1.0f
    };

    QuantizedCore quantizeINT8(const TTCore& core) const;
    QuantizedCore quantizeNF4 (const TTCore& core) const;

    TTCore dequantizeINT8(const QuantizedCore& qcore) const;
    TTCore dequantizeNF4 (const QuantizedCore& qcore) const;

    /// Find nearest index in kNF4Table for a normalised value v ∈ [-1,1].
    static uint8_t findNF4Index(float v) noexcept;
};

} // namespace storage
} // namespace themis
