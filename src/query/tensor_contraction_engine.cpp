/**
 * @file tensor_contraction_engine.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=17, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
    result.mode_sizes.reserve(train.order());
    result.cores.reserve(train.order());

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
                if (new_n != 0 && new_rr != 0 && new_rl > std::numeric_limits<std::size_t>::max() / new_n / new_rr) {
                    throw std::overflow_error("TT-core merge: core dimension product overflows size_t");
                }
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
    result.cores.reserve(a.order());

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
        if (n != 0 && rr != 0 && rl > std::numeric_limits<std::size_t>::max() / n / rr) {
            throw std::overflow_error("TT-core kron: core dimension product overflows size_t");
        }
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
    TensorTrainDecomposer dec = {};
    return dec.round(train, cfg);
}

// ============================================================================
// project — marginalize over one mode (compressed domain)
// ============================================================================

TTTrain TensorContractionEngine::project(const TTTrain& train,
                                          std::size_t mode) {
    if (mode >= train.order())
        throw std::out_of_range("TensorContractionEngine::project: mode out of range");
    if (train.order() < 2)
        throw std::invalid_argument("TensorContractionEngine::project: order must be ≥ 2");

    // Step 1: compute M[r_left, r_right] = sum_j G_mode[:, j, :]
    const auto& ck = train.cores[mode];
    std::size_t rl_k = ck.r_left, rr_k = ck.r_right;
    std::vector<float> M(rl_k * rr_k, 0.0f);
    for (std::size_t j = 0; j < ck.n; ++j)
        for (std::size_t l = 0; l < rl_k; ++l)
            for (std::size_t r = 0; r < rr_k; ++r)
                M[l * rr_k + r] += ck.at(l, j, r);

    TTTrain result;
    result.original_norm = train.original_norm;
    result.achieved_eps  = train.achieved_eps;

    const std::size_t d = train.order();
    result.cores.reserve(d - 1);
    result.mode_sizes.reserve(d - 1);

    if (mode == 0) {
        // Absorb M (shape 1 × rr_k) into G_{1} from the left.
        // new_G0[l', i, r] = sum_s M[0, s] * G_1[s, i, r]
        // where l' = 0 and M has shape (1, rr_k).
        const auto& c1 = train.cores[1];
        TTCore ng;
        ng.r_left  = 1;         // left boundary
        ng.n       = c1.n;
        ng.r_right = c1.r_right;
        ng.data.resize(ng.n * ng.r_right, 0.0f);
        for (std::size_t i = 0; i < c1.n; ++i)
            for (std::size_t r = 0; r < c1.r_right; ++r)
                for (std::size_t s = 0; s < rr_k; ++s)
                    ng.at(0, i, r) += M[s] * c1.at(s, i, r);
        result.cores.push_back(std::move(ng));
        result.mode_sizes.push_back(c1.n);
        for (std::size_t k = 2; k < d; ++k) {
            result.cores.push_back(train.cores[k]);
            result.mode_sizes.push_back(train.mode_sizes[k]);
        }
    } else {
        // Absorb M (shape rl_k × rr_k) into G_{mode-1} from the right.
        // new_G_{mode-1}[l, i, r'] = sum_s G_{mode-1}[l, i, s] * M[s, r']
        for (std::size_t k = 0; k < mode - 1; ++k) {
            result.cores.push_back(train.cores[k]);
            result.mode_sizes.push_back(train.mode_sizes[k]);
        }
        const auto& prev = train.cores[static_cast<int>(mode - 1)];
        TTCore ng;
        ng.r_left  = prev.r_left;
        ng.n       = prev.n;
        ng.r_right = rr_k;
        ng.data.resize(prev.r_left * prev.n * rr_k, 0.0f);
        for (std::size_t l = 0; l < prev.r_left; ++l)
            for (std::size_t i = 0; i < prev.n; ++i)
                for (std::size_t r = 0; r < rr_k; ++r)
                    for (std::size_t s = 0; s < rl_k; ++s)
                        ng.at(l, i, r) += prev.at(l, i, s) * M[s * rr_k + r];
        result.cores.push_back(std::move(ng));
        result.mode_sizes.push_back(prev.n);
        for (std::size_t k = mode + 1; k < d; ++k) {
            result.cores.push_back(train.cores[k]);
            result.mode_sizes.push_back(train.mode_sizes[k]);
        }
    }
    return result;
}

// ============================================================================
// contractModes — multi-mode tensor contraction
//
// Uses dense reconstruction for correctness; efficient enough for AQL
// queries that embed small tensors in JSON documents.
// ============================================================================

TTTrain TensorContractionEngine::contractModes(
    const TTTrain&                 a,
    const TTTrain&                 b,
    const std::vector<std::size_t>& modes_a,
    const std::vector<std::size_t>& modes_b,
    std::size_t                    max_rank,
    double                         round_eps) {

    if (static_cast<int>(modes_a.size()) != modes_b.size())
        throw std::invalid_argument(
            "TensorContractionEngine::contractModes: modes_a / modes_b length mismatch");

    for (std::size_t i = 0; i < modes_a.size(); ++i) {
        if (modes_a[i] >= a.order())
            throw std::invalid_argument(
                "TensorContractionEngine::contractModes: modes_a index out of range");
        if (modes_b[i] >= b.order())
            throw std::invalid_argument(
                "TensorContractionEngine::contractModes: modes_b index out of range");
        if (a.mode_sizes[modes_a[i]] != b.mode_sizes[modes_b[i]])
            throw std::invalid_argument(
                "TensorContractionEngine::contractModes: incompatible mode sizes at pair " +
                std::to_string(i));
    }

    // Reconstruct dense tensors for a and b.
    auto dense_a = a.reconstruct();  // shape: ∏ a.mode_sizes
    auto dense_b = b.reconstruct();  // shape: ∏ b.mode_sizes

    const auto& sha = a.mode_sizes;
    const auto& shb = b.mode_sizes;

    // Identify free (non-contracted) modes.
    std::vector<bool> contracted_a(sha.size(), false);
    std::vector<bool> contracted_b(shb.size(), false);
    for (std::size_t i = 0; i < modes_a.size(); ++i) {
        contracted_a[modes_a[i]] = true;
        contracted_b[modes_b[i]] = true;
    }

    std::vector<std::size_t> free_a, free_b;
    free_a.reserve(sha.size());
    free_b.reserve(shb.size());
    for (std::size_t k = 0; k < sha.size(); ++k)
        if (!contracted_a[k]) {
          free_a.push_back(k);
        }
    for (std::size_t k = 0; k < shb.size(); ++k)
        if (!contracted_b[k]) {
          free_b.push_back(k);
        }

    // Result shape: [free dims of a] + [free dims of b]
    std::vector<std::size_t> result_shape = {};

    result_shape.reserve(static_cast<int>(free_a.size()) + free_b.size());
    for (auto k : free_a) {
      result_shape.push_back(sha[k]);
    }
    for (auto k : free_b) {
      result_shape.push_back(shb[k]);
    }

    // Helper: multi-index → flat offset
    auto toFlat = [](const std::vector<std::size_t>& idx,
                     const std::vector<std::size_t>& shape) -> std::size_t {
        std::size_t off = 0, stride = 1;
        for (int k = static_cast<int>(shape.size()) - 1; k >= 0; --k) {
            off    += idx[static_cast<std::size_t>(k)] * stride;
            stride *= shape[static_cast<std::size_t>(k)];
        }
        return off;
    };

    // Compute result.
    std::size_t res_sz = 1;
    for (auto s : result_shape) {
      res_sz *= s;
    }
    std::vector<float> result_dense(res_sz, 0.0f);

    // Iterate over the Cartesian product of ALL indices of a.
    // Indices of b's contracted modes are set equal to those of a's contracted modes.
    // Indices of b's free modes are iterated separately.
    std::vector<std::size_t> idx_a(sha.size(), 0);
    std::vector<std::size_t> idx_b(shb.size(), 0);

    // We build a loop over all a-indices and all free b-indices.
    std::size_t total_a = dense_a.size();
    std::size_t total_free_b = 1;
    for (auto k : free_b) {
      total_free_b *= shb[k];
    }

    // Reset and iterate.
    std::fill(idx_a.begin(), idx_a.end(), 0);

    for (std::size_t flat_a = 0; flat_a < total_a; ++flat_a) {
        // Decode flat_a into idx_a.
        std::size_t tmp = flat_a;
        for (int k = static_cast<int>(sha.size()) - 1; k >= 0; --k) {
            idx_a[static_cast<std::size_t>(k)] = tmp % sha[static_cast<std::size_t>(k)];
            tmp /= sha[static_cast<std::size_t>(k)];
        }

        // Set contracted b indices from a.
        std::fill(idx_b.begin(), idx_b.end(), 0);
        for (std::size_t i = 0; i < modes_a.size(); ++i)
            idx_b[modes_b[i]] = idx_a[modes_a[i]];

        float va = dense_a[flat_a];
        if (va == 0.0f) {
          continue;
        }

        // Iterate over free b indices.
        std::vector<std::size_t> free_b_idx(free_b.size(), 0);
        for (std::size_t fb = 0; fb < total_free_b; ++fb) {
            // Decode fb into free_b_idx.
            std::size_t tt = fb;
            for (int ki = static_cast<int>(free_b.size()) - 1; ki >= 0; --ki) {
                std::size_t ki_sz = static_cast<std::size_t>(ki);
                free_b_idx[ki_sz] = tt % shb[free_b[ki_sz]];
                tt /= shb[free_b[ki_sz]];
            }
            for (std::size_t i = 0; i < free_b.size(); ++i)
                idx_b[free_b[i]] = free_b_idx[i];

            float vb = dense_b[toFlat(idx_b, shb)];

            // Build result index: free_a indices first, then free_b indices.
            std::vector<std::size_t> ridx = {};

            ridx.reserve(static_cast<int>(free_a.size()) + free_b.size());
            for (auto k : free_a) {
              ridx.push_back(idx_a[k]);
            }
            for (auto k : free_b) {
              ridx.push_back(idx_b[k]);
            }

            result_dense[toFlat(ridx, result_shape)] += va * vb;
        }
    }

    // Handle full contraction (scalar) directly. TensorTrainDecomposer now
    // requires at least two modes, so building the scalar TT core here avoids
    // an invalid decompose({value}, {1}) call.
    if (result_shape.empty()) {
        TTTrain scalar;
        scalar.mode_sizes    = {1};
        scalar.original_norm = 0.0;
        scalar.achieved_eps  = 0.0;

        TTCore core;
        core.r_left  = 1;
        core.n       = 1;
        core.r_right = 1;
        core.data    = {result_dense.empty() ? 0.0f : result_dense[0]};
        scalar.cores.push_back(std::move(core));
        return scalar;
    }

    // Re-decompose result to TT format.
    double eps = round_eps > 0.0 ? round_eps
                                 : std::max(a.achieved_eps, b.achieved_eps);
    if (eps <= 0.0) {
      eps = 1e-4;
    }

    TensorTrainConfig cfg;
    cfg.eps      = eps;
    cfg.max_rank = max_rank;

    TensorTrainDecomposer dec;
    auto [result_train, result_stats] = dec.decompose(result_dense, result_shape, cfg);
    (void)result_stats;
    return result_train;
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

