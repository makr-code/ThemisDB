/*
 * ThemisDB | File: tensor_train_decomposer.h | Version: 1.0.0
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/**
 * @file tensor_train_decomposer.h
 * @brief Tensor-Train (TT) decomposition engine for ThemisDB storage.
 *
 * Implements the TT-SVD algorithm (Oseledets, SIAM J. Sci. Comput. 2011,
 * DOI:10.1137/090752142) to represent dense multi-dimensional tensors as
 * chains of 3-D core tensors:
 *
 *   T(i₁,…,id) ≈ G₁(i₁) · G₂(i₂) · … · Gd(id)
 *
 * where Gk ∈ ℝ^{r_{k-1} × n_k × r_k} and r = (r₀,…,rd) are the TT-ranks
 * (r₀ = r_d = 1).  Storage cost is O(d·n·r²) vs O(n^d) for the full tensor.
 *
 * ### Accuracy guarantee
 * The truncated SVD at each step ensures
 *   ‖T - T_approx‖_F ≤ ε · ‖T‖_F
 * when `eps` is supplied (Theorem 2.1 in Oseledets 2011).
 *
 * ### Thread safety
 * `TensorTrainDecomposer` is stateless and safe for concurrent use after
 * construction.  Individual `TTTrain` objects are NOT thread-safe.
 *
 * ### References
 * - Oseledets, I. V. (2011). Tensor-Train Decomposition.
 *   SIAM Journal on Scientific Computing, 33(5), 2295–2317.
 * - Holtz, S., Rohwedder, T., & Schneider, R. (2012). The alternating linear
 *   scheme for tensor optimization in the TT-format. SIAM J. Sci. Comput.,
 *   34(2), A683–A713.
 * - Kossaifi, J. et al. (2019). TensorLy: Tensor Learning in Python.
 *   Journal of Machine Learning Research, 20(26), 1–6.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace themis {
namespace storage {

// ============================================================================
// TTCore — a single 3-D core tensor G_k  (shape: r_{k-1} × n_k × r_k)
// ============================================================================

/**
 * @brief One core tensor in a TT-chain.
 *
 * Data is stored in row-major order: element [l][i][r] lives at
 * `data[l * n * r_right + i * r_right + r]`.
 */
struct TTCore {
    std::size_t r_left  = 1;  ///< Left bond dimension
    std::size_t n       = 1;  ///< Mode size (physical dimension)
    std::size_t r_right = 1;  ///< Right bond dimension

    /// Flattened core data (length = r_left * n * r_right)
    std::vector<float> data;

    /// @brief Access element G[l][i][r]
    float& at(std::size_t l, std::size_t i, std::size_t r) {
        return data[l * n * r_right + i * r_right + r];
    }
    const float& at(std::size_t l, std::size_t i, std::size_t r) const {
        return data[l * n * r_right + i * r_right + r];
    }

    std::size_t numElements() const noexcept {
        return r_left * n * r_right;
    }
};

// ============================================================================
// TTTrain — full TT-decomposition of one tensor
// ============================================================================

/**
 * @brief A Tensor-Train representation of a dense multi-dimensional tensor.
 *
 * The number of cores equals the tensor order (number of dimensions).
 * `mode_sizes[k]` gives n_k (the original size along mode k).
 */
struct TTTrain {
    /// Original tensor mode sizes (length = order d)
    std::vector<std::size_t> mode_sizes;

    /// TT-cores G₁, G₂, …, Gd
    std::vector<TTCore> cores;

    /// Frobenius norm of the original tensor (set by decomposer)
    double original_norm = 0.0;

    /// Achieved reconstruction error: ‖T - T_approx‖_F / ‖T‖_F
    double achieved_eps  = 0.0;

    /// Order (number of dimensions)
    std::size_t order() const noexcept { return cores.size(); }

    /// Total number of parameters stored in all cores
    std::size_t totalParams() const noexcept;

    /// Maximum TT-rank across all bonds
    std::size_t maxRank() const noexcept;

    /// Compression ratio vs. dense storage (∏ n_k / totalParams)
    double compressionRatio() const noexcept;

    /// Reconstruct full dense tensor (use only for testing; cost = O(n^d))
    std::vector<float> reconstruct() const;

    /// Serialise to bytes for RocksDB storage
    std::vector<uint8_t> serialize() const;

    /// Deserialise from bytes
    static std::optional<TTTrain> deserialize(const std::vector<uint8_t>& bytes);
};

// ============================================================================
// TensorTrainConfig — decomposition parameters
// ============================================================================

/**
 * @brief Configuration for the TT-SVD decomposition algorithm.
 */
struct TensorTrainConfig {
    /**
     * @brief Relative reconstruction error tolerance ε ∈ (0, 1].
     *
     * The algorithm selects SVD truncation thresholds such that
     * ‖T - T_approx‖_F ≤ eps * ‖T‖_F.
     *
     * Default: 0.01 (1% error).
     */
    double eps = 0.01;

    /**
     * @brief Hard cap on the TT-rank at every bond.
     *
     * 0 = no cap (rank determined solely by `eps`).
     * Non-zero values override the SVD-based truncation when it would
     * produce a higher rank, trading accuracy for memory.
     */
    std::size_t max_rank = 0;

    /**
     * @brief Data type of the input dense tensor.
     *
     * Currently "float32" and "float64" are supported.  float64 inputs are
     * downcast to float32 for core storage.
     */
    std::string dtype = "float32";

    /**
     * @brief Number of power-iteration steps for the randomised SVD.
     *
     * 0 = exact truncated SVD via LAPACK dgesdd.
     * > 0 = randomised SVD (faster for large unfoldings but approximate).
     *
     * Default: 0 (exact SVD).
     */
    int svd_power_iterations = 0;
};

// ============================================================================
// DecompositionStats
// ============================================================================

/**
 * @brief Post-decomposition statistics returned alongside the TTTrain.
 */
struct DecompositionStats {
    double elapsed_ms         = 0.0; ///< Wall-clock time in milliseconds
    double compression_ratio  = 0.0; ///< dense_elements / tt_parameters
    double achieved_eps       = 0.0; ///< Actual ‖T-T_approx‖_F / ‖T‖_F
    std::size_t max_rank      = 0;   ///< Maximum bond dimension
    std::size_t total_params  = 0;   ///< Total TT-core parameters
    std::size_t dense_elements= 0;   ///< Product of all mode sizes
};

// ============================================================================
// TensorTrainDecomposer
// ============================================================================

/**
 * @brief Stateless engine for Tensor-Train decomposition (TT-SVD).
 *
 * ### Algorithm (Oseledets 2011, Algorithm 1)
 * 1. Compute Frobenius norm ‖T‖_F and derive per-step truncation δ = ε/√(d-1)·‖T‖_F.
 * 2. For k = 1 … d-1:
 *    a. Unfold residual into matrix C of shape (r_{k-1}·n_k) × (n_{k+1}·…·n_d).
 *    b. Truncated SVD: C ≈ U Σ Vᵀ, keeping r_k singular values with σᵢ > δ.
 *    c. G_k ← reshape(U·diag(Σ)^{1/2}, r_{k-1}, n_k, r_k).
 *    d. Residual ← reshape(diag(Σ)^{1/2}·Vᵀ, r_k, n_{k+1}·…·n_d).
 * 3. G_d ← residual (shape 1 × n_d × 1 after division by ‖T‖_F or similar).
 *
 * ### Usage
 * @code
 * TensorTrainDecomposer decomposer;
 * TensorTrainConfig cfg;
 * cfg.eps = 0.01;
 *
 * // dense 6D tensor of shape {4,4,4,4,4,4}
 * std::vector<float> dense(4096, 0.0f);
 * // … fill dense …
 *
 * auto [train, stats] = decomposer.decompose(dense, {4,4,4,4,4,4}, cfg);
 * std::cout << "compression ratio: " << stats.compression_ratio << "\n";
 * @endcode
 */
class TensorTrainDecomposer {
public:
    TensorTrainDecomposer() = default;
    ~TensorTrainDecomposer() = default;

    TensorTrainDecomposer(const TensorTrainDecomposer&) = default;
    TensorTrainDecomposer& operator=(const TensorTrainDecomposer&) = default;

    // ─── Core decomposition ───────────────────────────────────────────────

    /**
     * @brief Decompose a dense float32 tensor into TT-format.
     *
     * @param data       Flat row-major tensor data (length = ∏ mode_sizes).
     * @param mode_sizes Sizes of each mode (d ≥ 2).
     * @param cfg        Decomposition configuration.
     * @return Pair {TTTrain, DecompositionStats}.
     * @throws std::invalid_argument if data.size() != ∏ mode_sizes or d < 2.
     */
    std::pair<TTTrain, DecompositionStats>
    decompose(const std::vector<float>&       data,
              const std::vector<std::size_t>& mode_sizes,
              const TensorTrainConfig&         cfg = {}) const;

    /**
     * @brief Decompose a dense float64 tensor (downcast to float32 for cores).
     */
    std::pair<TTTrain, DecompositionStats>
    decomposeF64(const std::vector<double>&      data,
                 const std::vector<std::size_t>& mode_sizes,
                 const TensorTrainConfig&         cfg = {}) const;

    // ─── TT-Rounding ──────────────────────────────────────────────────────

    /**
     * @brief Re-compress an existing TTTrain to a tighter eps or lower max_rank.
     *
     * Implements right-to-left orthogonalisation + left-to-right truncation
     * (Oseledets 2011, Algorithm 2 / TT-rounding).
     *
     * @param train Source TT-train.
     * @param cfg   New (tighter) configuration.
     * @return Rounded TTTrain with reduced ranks.
     */
    TTTrain round(const TTTrain& train, const TensorTrainConfig& cfg) const;

    /**
     * @brief Re-compress an existing TTTrain without full reconstruction.
     *
     * Implements the efficient TT-rounding algorithm (Oseledets 2011,
     * Algorithm 2):
     *  1. Right-to-left LQ orthogonalisation (Modified Gram-Schmidt on rows)
     *     redistributes the Frobenius norm into the first core.
     *  2. Left-to-right truncated-SVD sweep discards singular values below
     *     δ = ε · ‖T‖_F / √(d−1), reducing bond dimensions.
     *
     * Complexity: O(d · r² · n) — avoids the O(∏ n_k) cost of full
     * reconstruction used by `round()`.
     *
     * Never increases any bond dimension: if the train is already compact at
     * the requested `eps`, it is returned unchanged (same ranks).
     *
     * @param train  Source TT-train.
     * @param cfg    Target configuration (eps / max_rank).  Typically the same
     *               or tighter than the original decomposition parameters.
     * @return       Recompressed TTTrain with equal or lower bond dimensions.
     */
    TTTrain recompress(const TTTrain& train, const TensorTrainConfig& cfg) const;

    // ─── Inner product (compressed domain) ───────────────────────────────

    /**
     * @brief Compute ⟨A, B⟩ = ∑ A(i₁…id)·B(i₁…id) without reconstructing.
     *
     * Complexity: O(d · n · r³) using the transfer-matrix technique.
     *
     * @throws std::invalid_argument if A and B have incompatible mode_sizes.
     */
    static double innerProduct(const TTTrain& a, const TTTrain& b);

    /**
     * @brief Compute ‖A‖_F = sqrt(⟨A,A⟩) in compressed domain.
     */
    static double frobeniusNorm(const TTTrain& a);

    /**
     * @brief Cosine similarity cos(A,B) = ⟨A,B⟩ / (‖A‖·‖B‖) in [−1, 1].
     *
     * Returns 0.0 when either norm is zero.
     */
    static double cosineSimilarity(const TTTrain& a, const TTTrain& b);

private:
    /// Perform truncated SVD of an m×n matrix.  Returns U, S, Vt truncated to
    /// `rank` columns/rows (rank chosen so that σ_{rank+1} ≤ delta, or by
    /// max_rank cap).  Uses Householder bidiagonalisation (Golub-Reinsch).
    static void truncatedSVD(const std::vector<float>& mat,
                              std::size_t m, std::size_t n,
                              double delta,
                              std::size_t max_rank_cap,
                              std::vector<float>& U,
                              std::vector<float>& S,
                              std::vector<float>& Vt,
                              std::size_t& rank_out);

    /// Matrix multiply C = A·B where A is (m×k) and B is (k×n) — row-major.
    static std::vector<float> matMul(const std::vector<float>& A,
                                     const std::vector<float>& B,
                                     std::size_t m,
                                     std::size_t k,
                                     std::size_t n);

    /// Compute Frobenius norm of a flat vector.
    static double vecNorm(const std::vector<float>& v) noexcept;
};

} // namespace storage
} // namespace themis
