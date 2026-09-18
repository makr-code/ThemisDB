/**
 * @file tensor_contraction_engine.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "storage/tensor_train_decomposer.h"

#include <cstddef>
#include <optional>
#include <stdexcept>
#include <vector>

namespace themis {
namespace query {

// ============================================================================
// TensorContractionEngine
// ============================================================================

class TensorContractionEngine {
public:
    static constexpr std::size_t kDefaultMaxRankAfterOp = 64;

    /**
     * @brief ─── Inner product / norms ────────────────────────────────────────────
     * @param[in] a Input parameter.
     * @param[in] b Input parameter.
     * @return Return value.
     */

    static double innerProduct(const storage::TTTrain& a,
                               const storage::TTTrain& b);

    /**
     * @brief Frobenius Norm.
     * @param[in] a Input parameter.
     * @return Return value.
     */
    static double frobeniusNorm(const storage::TTTrain& a);

    /**
     * @brief Cosine Similarity.
     * @param[in] a Input parameter.
     * @param[in] b Input parameter.
     * @return Return value.
     */
    static double cosineSimilarity(const storage::TTTrain& a,
                                   const storage::TTTrain& b);

    /**
     * @brief ─── Structural operations ────────────────────────────────────────────
     * @param[in] train Input parameter.
     * @param[in] dim Input parameter.
     * @param[in] idx Input parameter.
     * @return Return value.
     */

    static storage::TTTrain slice(const storage::TTTrain& train,
                                  std::size_t dim,
                                  std::size_t idx);

    static storage::TTTrain hadamardProduct(
        const storage::TTTrain& a,
        const storage::TTTrain& b,
        std::size_t max_rank = kDefaultMaxRankAfterOp,
        double round_eps     = 1e-4);

    // ─── Recompression ────────────────────────────────────────────────────

    static storage::TTTrain recompress(const storage::TTTrain& train,
                                       double eps,
                                       std::size_t max_rank = 0);

    /**
     * @brief ─── Marginalization / Projection ────────────────────────────────────
     * @param[in] train Input parameter.
     * @param[in] mode Input parameter.
     * @return Return value.
     */

    static storage::TTTrain project(const storage::TTTrain& train,
                                    std::size_t mode);

    // ─── Multi-mode tensor contraction ───────────────────────────────────

    static storage::TTTrain contractModes(
        const storage::TTTrain&        a,
        const storage::TTTrain&        b,
        const std::vector<std::size_t>& modes_a,
        const std::vector<std::size_t>& modes_b,
        std::size_t                    max_rank  = kDefaultMaxRankAfterOp,
        double                         round_eps = 1e-4);

    /**
     * @brief ─── Utility ──────────────────────────────────────────────────────────
     * @param[in] a Input parameter.
     * @param[in] b Input parameter.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */

    static bool isCompatible(const storage::TTTrain& a,
                              const storage::TTTrain& b) noexcept;

private:
    /**
     * @brief Transfer-matrix algorithm: compute M_k = M_{k-1} ⊗ (G_A_k^T · G_B_k) for k = 1…d, then return Tr(M_d).
     * @param[in] M Input parameter.
     * @param[in] coreA Input parameter.
     * @param[in] coreB Input parameter.
     * @return Return value.
     */
    static std::vector<float> transferStep(
        const std::vector<float>& M,
        const storage::TTCore&    coreA,
        const storage::TTCore&    coreB);

    /**
     * @brief Mat Mul.
     * @param[in] A Input parameter.
     * @param[in] B Input parameter.
     * @param[in] m Input parameter.
     * @param[in] k Input parameter.
     * @param[in] n Input parameter.
     * @return Return value.
     */
    static std::vector<float> matMul(const std::vector<float>& A,
                                     const std::vector<float>& B,
                                     std::size_t m,
                                     std::size_t k,
                                     std::size_t n);
};

} // namespace query
} // namespace themis
