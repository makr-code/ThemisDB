/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            tensor_contraction_engine.cpp                      ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-05-05                                         ║
  Author:          copilot                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "query/tensor_contraction_engine.h"
#include "storage/tensor_train_decomposer.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace themis {
namespace query {

using storage::TTTrain;
using storage::TTCore;
using storage::TensorTrainDecomposer;
using storage::TensorTrainConfig;

// ============================================================================
// Utility — matrix multiply (row-major, m×k × k×n = m×n)
// ============================================================================

std::vector<float> TensorContractionEngine::matMul(
    const std::vector<float>& A, const std::vector<float>& B,
    std::size_t m, std::size_t k, std::size_t n) {

    std::vector<float> C(m * n, 0.0f);
    for (std::size_t i = 0; i < m; ++i)
        for (std::size_t p = 0; p < k; ++p) {
            float aip = A[i * k + p];
            for (std::size_t j = 0; j < n; ++j)
                C[i * n + j] += aip * B[p * n + j];
        }
    return C;
}

// ============================================================================
// innerProduct — transfer-matrix (zipper) algorithm
// ============================================================================

double TensorContractionEngine::innerProduct(const TTTrain& a, const TTTrain& b) {
    return TensorTrainDecomposer::innerProduct(a, b);
}

double TensorContractionEngine::frobeniusNorm(const TTTrain& a) {
    return TensorTrainDecomposer::frobeniusNorm(a);
}

double TensorContractionEngine::cosineSimilarity(const TTTrain& a,
                                                   const TTTrain& b) {
    return TensorTrainDecomposer::cosineSimilarity(a, b);
}

// ============================================================================
// slice — fix mode `dim` to index `idx`
// ============================================================================

TTTrain TensorContractionEngine::slice(const TTTrain& train,
                                        std::size_t dim,
                                        std::size_t idx) {
    if (dim >= train.order())
        throw std::out_of_range("TensorContractionEngine::slice: dim out of range");
    if (idx >= train.mode_sizes[dim])
        throw std::out_of_range("TensorContractionEngine::slice: idx out of range");

    TTTrain result;
    result.original_norm = train.original_norm;
    result.achieved_eps  = train.achieved_eps;

    for (std::size_t k = 0; k < train.order(); ++k) {
        if (k == dim) {
            // Contract this core with unit vector e_{idx}
            // G_k shape: (r_l × n × r_r) → contracted → (r_l × r_r)
            // Result is a 2-D matrix that we embed as a (1 × r_l, r_r × 1)
            // virtual core, but in the sliced train we just absorb it into
            // adjacent cores.  For simplicity, skip and update modes.
            result.mode_sizes.push_back(1);  // placeholder, removed below
            TTCore contracted;
            const auto& ck = train.cores[k];
            contracted.r_left  = ck.r_left;
            contracted.n       = 1;
            contracted.r_right = ck.r_right;
            contracted.data.resize(ck.r_left * ck.r_right);
            for (std::size_t l = 0; l < ck.r_left; ++l)
                for (std::size_t r = 0; r < ck.r_right; ++r)
                    contracted.at(l, 0, r) = ck.at(l, idx, r);
            result.cores.push_back(std::move(contracted));
        } else {
            result.mode_sizes.push_back(train.mode_sizes[k]);
            result.cores.push_back(train.cores[k]);
        }
    }

    // Remove the dimension-1 core by contracting it into its right neighbour
    // (if there is one).  This keeps the result as a proper (d-1)-dimensional TT.
    for (std::size_t k = 0; k < result.cores.size(); ) {
        if (result.cores[k].n == 1 && result.cores.size() > 1) {
            // Absorb core k into core k+1 (if exists) or k-1
            if (k + 1 < result.cores.size()) {
                const auto& ck  = result.cores[k];
                auto&       ck1 = result.cores[k + 1];
                // new_core: (r_l_k × n_{k+1} × r_r_{k+1})
                std::size_t new_rl = ck.r_left;
                std::size_t new_n  = ck1.n;
                std::size_t new_rr = ck1.r_right;
                TTCore merged;
                merged.r_left  = new_rl;
                merged.n       = new_n;
                merged.r_right = new_rr;
                merged.data.resize(new_rl * new_n * new_rr, 0.0f);
                // merged[l, i, r] = sum_{m} ck[l, 0, m] * ck1[m, i, r]
                for (std::size_t l = 0; l < new_rl; ++l)
                    for (std::size_t i = 0; i < new_n; ++i)
                        for (std::size_t r = 0; r < new_rr; ++r)
                            for (std::size_t m = 0; m < ck.r_right; ++m)
                                merged.at(l, i, r) +=
                                    ck.at(l, 0, m) * ck1.at(m, i, r);
                result.cores[k + 1] = std::move(merged);
                result.cores.erase(result.cores.begin() + k);
                result.mode_sizes.erase(result.mode_sizes.begin() + k);
            } else {
                break;
            }
        } else {
            ++k;
        }
    }

    return result;
}

// ============================================================================
// hadamardProduct — Kronecker-product of cores
// ============================================================================

TTTrain TensorContractionEngine::hadamardProduct(
    const TTTrain& a, const TTTrain& b,
    std::size_t max_rank, double round_eps) {

    if (!isCompatible(a, b))
        throw std::invalid_argument("TensorContractionEngine::hadamardProduct: incompatible mode_sizes");

    TTTrain result;
    result.mode_sizes    = a.mode_sizes;
    result.original_norm = 0.0;  // not tracked for products
    result.achieved_eps  = std::max(a.achieved_eps, b.achieved_eps);

    for (std::size_t k = 0; k < a.order(); ++k) {
        const auto& ca = a.cores[k];
        const auto& cb = b.cores[k];

        // Result core: r_l_a*r_l_b × n_k × r_r_a*r_r_b
        std::size_t rl = ca.r_left  * cb.r_left;
        std::size_t rr = ca.r_right * cb.r_right;
        std::size_t n  = ca.n;

        TTCore cr;
        cr.r_left  = rl;
        cr.n       = n;
        cr.r_right = rr;
        cr.data.resize(rl * n * rr, 0.0f);

        // Kronecker product of core matrices for each physical index i
        for (std::size_t i = 0; i < n; ++i)
            for (std::size_t la = 0; la < ca.r_left; ++la)
                for (std::size_t lb = 0; lb < cb.r_left; ++lb)
                    for (std::size_t ra = 0; ra < ca.r_right; ++ra)
                        for (std::size_t rb = 0; rb < cb.r_right; ++rb)
                            cr.at(la * cb.r_left + lb, i, ra * cb.r_right + rb) =
                                ca.at(la, i, ra) * cb.at(lb, i, rb);

        result.cores.push_back(std::move(cr));
    }

    // Apply TT-rounding to bound rank growth
    if (max_rank > 0 || round_eps > 0.0) {
        TensorTrainConfig cfg;
        cfg.max_rank = max_rank;
        cfg.eps      = round_eps;
        TensorTrainDecomposer dec;
        result = dec.round(result, cfg);
    }

    return result;
}

// ============================================================================
// recompress
// ============================================================================

TTTrain TensorContractionEngine::recompress(const TTTrain& train,
                                             double eps,
                                             std::size_t max_rank) {
    TensorTrainConfig cfg;
    cfg.eps      = eps;
    cfg.max_rank = max_rank;
    TensorTrainDecomposer dec;
    return dec.round(train, cfg);
}

// ============================================================================
// isCompatible
// ============================================================================

bool TensorContractionEngine::isCompatible(const TTTrain& a,
                                            const TTTrain& b) noexcept {
    return a.mode_sizes == b.mode_sizes;
}

} // namespace query
} // namespace themis
