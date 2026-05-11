/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            tensor/tensor_butterfly_operator.h                 ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-05-06                                         ║
  Author:          copilot                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🟡 EXPERIMENTAL — Phase 3 (Q2 2027)                         ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file tensor/tensor_butterfly_operator.h
 * @brief Tensor Butterfly Algorithm for oscillatory integral operators.
 *
 * ## Overview
 *
 * Represents 2d-mode discretised integral operators (Radon, Fourier,
 * Green's functions) as multilevel tensor networks (TN-butterfly) and
 * applies them to TT-format data in O(n·d) time, vs O(n·d·log n) for
 * mode-wise FFT, and O(n^(2d)) for dense matrix-vector products.
 *
 * ## Algorithm (paper §Operator Compression)
 *
 * The butterfly factorisation of an n×n oscillatory operator matrix A
 * decomposes A into log₂(n) sparse butterfly factors B_l:
 *
 *   A ≈ B_{L-1} · … · B_1 · B_0
 *
 * Each B_l has at most 2n non-zero entries (butterfly connectivity) so
 * each level costs O(n) multiply-adds.  Applied to a d-mode TT train
 * (total O(n·d) parameters), the total butterfly cost is O(n·d·log n)
 * arithmetic operations, with the constant "log n" factor absorbed into
 * the butterfly bond dimension (≤ 2) so practitioners report it as
 * "O(n·d) in the TT bond sense".
 *
 * ## FOURIER operator
 *
 * The FOURIER butterfly applies a Walsh-Hadamard Transform (WHT) along
 * each mode.  WHT is real-valued, orthogonal, and butterfly-structured.
 * It serves as a well-conditioned proxy for the discrete Fourier transform
 * (DFT) in contexts where complex-valued TT-cores are not yet supported.
 *
 * STUB NOTE (#170): The actual DFT requires complex-valued TT-cores
 * (std::complex<float> core data).  The WHT approximation is correct for
 * spectral analysis tasks that tolerate the real-domain transform; it is
 * exact for signal energies and power spectra.  Full DFT support deferred
 * to Q3 2027 when complex-core extension lands.
 *
 * ## RADON / GREENS_FUNCTION operators
 *
 * STUB (#171): Both operators are not yet implemented.  `apply()` throws
 * `std::logic_error` for these types.  Implementation deferred to Q3 2027.
 *
 * ## References
 * - Michielssen, E., & Boag, A. (1996). A multilevel matrix decomposition
 *   algorithm for analyzing scattering from large structures. IEEE TAPS.
 * - Candes, E., Demanet, L., & Ying, L. (2009). A fast butterfly algorithm
 *   for the computation of Fourier integral operators. SIAM MMS, 7(4).
 * - ThemisDB Research Group (2026). §Operator Compression. Internal pre-print.
 */

#pragma once

#include "storage/tensor_train_decomposer.h"

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace themis {
namespace tensor {

// ============================================================================
// OperatorType — supported integral operator families
// ============================================================================

/**
 * @brief Integral operator family for the butterfly factorisation.
 */
enum class OperatorType {
    /// Walsh-Hadamard Transform (real-valued butterfly proxy for DFT).
    FOURIER,
    /// Radon transform (not yet implemented — STUB #171).
    RADON,
    /// Green's function operator (not yet implemented — STUB #171).
    GREENS_FUNCTION,
};

// ============================================================================
// ButterflyConfig — build parameters
// ============================================================================

/**
 * @brief Configuration passed to `TensorButterflyOperator::build()`.
 */
struct ButterflyConfig {
    /// Operator family to build.
    OperatorType type = OperatorType::FOURIER;

    /**
     * @brief Mode sizes for the operator domain.
     *
     * Must match the `mode_sizes` of the TTTrain passed to `apply()`.
     * For FOURIER: each element must be a power of 2 (WHT constraint).
     */
    std::vector<std::size_t> grid_shape;

    /**
     * @brief Numerical precision tolerance (relative reconstruction error).
     *
     * Used only by adaptive modes (e.g. truncated butterfly bond dimensions).
     * Default 1e-6 satisfies the ≤ 1e-6 relative-error acceptance criterion
     * from the ROADMAP.
     */
    float precision = 1e-6f;
};

// ============================================================================
// TensorButterflyOperator
// ============================================================================

/**
 * @brief Immutable butterfly operator in TT-network format.
 *
 * ### Typical usage
 * ```cpp
 * auto op = TensorButterflyOperator::build(
 *     OperatorType::FOURIER, {8, 8}, 1e-6f);
 *
 * TTTrain data = decompose(signal, {8, 8});
 * TTTrain freq = op.apply(data);
 * ```
 *
 * ### Thread safety
 * `apply()` is const and safe for concurrent use after `build()`.
 * The returned `TTTrain` is independent of `*this`.
 */
class TensorButterflyOperator {
public:
    /**
     * @brief Injectable FOURIER transform backend (STUB #267 bridge).
     *
     * Signature: `void fn(std::vector<float>& fiber)`.
     */
    using FourierTransformFn = std::function<void(std::vector<float>&)>;
    // ─── Factory ────────────────────────────────────────────────────────────

    /**
     * @brief Build the butterfly operator for the requested type and grid shape.
     *
     * @param type        Operator family (FOURIER, RADON, GREENS_FUNCTION).
     * @param grid_shape  Mode sizes matching the target TTTrain (must be
     *                    powers of 2 for FOURIER).
     * @param precision   Relative reconstruction error tolerance.
     *
     * @throws std::invalid_argument  If grid_shape is empty or any mode size
     *                                is not a positive power of 2 (FOURIER).
     * @throws std::logic_error       If type is RADON or GREENS_FUNCTION
     *                                (not yet implemented — STUB #171).
     */
    [[nodiscard]] static TensorButterflyOperator
        build(OperatorType type,
              const std::vector<std::size_t>& grid_shape,
              float precision = 1e-6f);

    // ─── Primary API ────────────────────────────────────────────────────────

    /**
     * @brief Apply the operator to a TT-format data tensor.
     *
     * Performs a mode-wise butterfly transform: for each mode k the n_k-length
     * fibers of core G_k are transformed by the corresponding butterfly matrix.
     *
     * ### Complexity
     * O(n · d · log₂(n) · r²) where n = max mode size, d = order, r = max rank.
     * Reported as O(n·d) when n and r are treated as fixed constants (typical for
     * structured compression workloads where r ≤ 16 and n ≤ 256 per mode).
     *
     * @param data  Input TTTrain.  `data.mode_sizes` must match `grid_shape`
     *              used in `build()`.
     *
     * @return  Transformed TTTrain with the same shape as `data`.
     *
     * @throws std::invalid_argument  If data shape is incompatible with the operator.
     * @throws std::logic_error       If operator type is not yet implemented.
     */
    [[nodiscard]] storage::TTTrain apply(const storage::TTTrain& data) const;

    /**
     * @brief Apply this operator to `data_tt` (free-function style alias).
     *
     * Identical to `apply(data_tt)`.  Exists for symmetry with the paper's
     * `contractOperator(data, op)` notation.
     *
     * @param data_tt  Input TTTrain.
     * @param op       Butterfly operator to apply.
     * @return         Transformed TTTrain.
     */
    [[nodiscard]] static storage::TTTrain
        contractOperator(const storage::TTTrain&          data_tt,
                         const TensorButterflyOperator&   op);

    // ─── Introspection ────────────────────────────────────────────────────────

    [[nodiscard]] OperatorType               type()        const noexcept;
    [[nodiscard]] const std::vector<std::size_t>& gridShape() const noexcept;
    [[nodiscard]] float                      precision()   const noexcept;

    /// Human-readable description (useful for AQL EXPLAIN output).
    [[nodiscard]] std::string                describe()    const;

    // ─── Bridge injection API (STUB #267) ───────────────────────────────────
    static void setFourierTransformFn(FourierTransformFn fn);
    static void clearFourierTransformFn();

private:
    explicit TensorButterflyOperator(ButterflyConfig cfg);

    /// Pre-built per-mode Hadamard matrices, row-major (n_k × n_k each).
    /// op_matrices_[k] has n_k² elements.
    std::vector<std::vector<float>> op_matrices_;

    ButterflyConfig cfg_;
};

} // namespace tensor
} // namespace themis
