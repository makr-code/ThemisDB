/**
 * @file tensor_butterfly_operator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Gap Summary: total=13; TODO=1, Stub=11, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

    // ─── Bridge injection API (STUB #268 — RADON / GREENS_FUNCTION) ─────────

    /**
     * @brief Per-fiber transform for the RADON operator (STUB #268).
     *
     * Signature: `void fn(std::vector<float>& fiber)`.
     * When set, `build(RADON, ...)` succeeds and `apply()` calls this fn
     * for every mode fiber of the TTTrain.
     */
    using RadonTransformFn = std::function<void(std::vector<float>&)>;

    /**
     * @brief Per-fiber transform for the GREENS_FUNCTION operator (STUB #268).
     *
     * Same signature and semantics as `RadonTransformFn`.
     */
    using GreensTransformFn = std::function<void(std::vector<float>&)>;

    /**
     * @brief Inject a RADON per-fiber backend.
     *
     * Once set, `build(RADON, ...)` no longer throws `std::logic_error`;
     * `apply()` delegates each mode-fiber transform to this function.
     */
    static void setRadonTransformFn(RadonTransformFn fn);
    static void clearRadonTransformFn();

    /**
     * @brief Inject a GREENS_FUNCTION per-fiber backend.
     *
     * Once set, `build(GREENS_FUNCTION, ...)` no longer throws `std::logic_error`;
     * `apply()` delegates each mode-fiber transform to this function.
     */
    static void setGreensTransformFn(GreensTransformFn fn);
    static void clearGreensTransformFn();

private:
    explicit TensorButterflyOperator(ButterflyConfig cfg);

    /// Pre-built per-mode Hadamard matrices, row-major (n_k × n_k each).
    /// op_matrices_[k] has n_k² elements.
    std::vector<std::vector<float>> op_matrices_;

    ButterflyConfig cfg_;
};

} // namespace tensor
} // namespace themis
