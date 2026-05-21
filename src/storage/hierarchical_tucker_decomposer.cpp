// THEMIS_GAP_STATS: gaps=23 unimpl=20 stub=2 mock=0 sim=0 todo=0 debt=0 scanned=2026-05-18
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            storage/hierarchical_tucker_decomposer.cpp         ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-05-07                                         ║
  Author:          copilot                                            ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "storage/hierarchical_tucker_decomposer.h"
#include "storage/tensor_train_decomposer.h"
#include "tensor/ht_train.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <numeric>
#include <stdexcept>
#include <utility>

// ============================================================================
// HTNode / HTTrain method implementations  (namespace themis::tensor)
// ============================================================================

namespace themis {
namespace tensor {

namespace {

std::size_t nodeTotal(const HTNode* n) noexcept {
    if (!n) return 0;
    std::size_t s = n->numParams();
    if (!n->is_leaf) {
        s += nodeTotal(n->left.get());
        s += nodeTotal(n->right.get());
    }
    return s;
}

std::unique_ptr<HTNode> cloneNode(const HTNode* src) {
    if (!src) return nullptr;
    auto dst = std::make_unique<HTNode>();
    dst->is_leaf     = src->is_leaf;
    dst->mode_index  = src->mode_index;
    dst->n_k         = src->n_k;
    dst->rank        = src->rank;
    dst->U           = src->U;
    dst->r_left      = src->r_left;
    dst->r_right     = src->r_right;
    dst->B           = src->B;
    dst->left        = cloneNode(src->left.get());
    dst->right       = cloneNode(src->right.get());
    return dst;
}

} // anonymous namespace

std::size_t HTNode::totalParams() const noexcept { return nodeTotal(this); }

std::unique_ptr<HTNode> HTNode::clone() const { return cloneNode(this); }

HTTrain HTTrain::clone() const {
    HTTrain c;
    c.root          = cloneNode(root.get());
    c.shape         = shape;
    c.max_rank      = max_rank;
    c.achieved_eps  = achieved_eps;
    c.original_norm = original_norm;
    return c;
}

double HTTrain::compressionRatio() const noexcept {
    std::size_t total = std::accumulate(shape.begin(), shape.end(),
                                        std::size_t{1}, std::multiplies<>{});
    std::size_t p = totalParams();
    return (p == 0) ? 1.0 : static_cast<double>(total) / static_cast<double>(p);
}

// ============================================================================
// Recursive reconstruction (used for toTTTrain & testing)
// ============================================================================

namespace {

// Expand one HTNode into its dense subtensor representation.
// Returns flat row-major tensor of shape [n_{L}, n_{L+1}, ..., n_{R-1}, rank].
// The trailing 'rank' dimension is squeezed at the root (rank == 1).
std::vector<float> expandNode(const HTNode& node) {
    if (node.is_leaf) {
        // f_k(i_k)[alpha] = U_k[i_k, alpha]
        // Shape: [n_k, rank]
        return node.U;  // already stored as [n_k × rank] row-major
    }

    // Internal node: f[..., alpha_out] = sum_{l,r} B[l,r,alpha_out] * f_left[...,l] * f_right[...,r]
    auto F_left  = expandNode(*node.left);
    auto F_right = expandNode(*node.right);

    // F_left  shape: [N_left,  r_left]   (N_left  = product of left  physical dims)
    // F_right shape: [N_right, r_right]  (N_right = product of right physical dims)
    std::size_t r_left  = node.r_left;
    std::size_t r_right = node.r_right;
    std::size_t r_out   = node.rank;

    std::size_t N_left  = F_left.size()  / r_left;
    std::size_t N_right = F_right.size() / r_right;

    // Result: shape [N_left * N_right, r_out]
    std::size_t N_out = N_left * N_right;
    std::vector<float> result(N_out * r_out, 0.0f);

    for (std::size_t il = 0; il < N_left; ++il) {
        for (std::size_t ir = 0; ir < N_right; ++ir) {
            for (std::size_t ao = 0; ao < r_out; ++ao) {
                float val = 0.0f;
                for (std::size_t l = 0; l < r_left; ++l) {
                    float fl = F_left[il * r_left + l];
                    if (fl == 0.0f) continue;
                    for (std::size_t r = 0; r < r_right; ++r) {
                        val += node.atB(l, r, ao) * fl * F_right[ir * r_right + r];
                    }
                }
                result[(il * N_right + ir) * r_out + ao] = val;
            }
        }
    }
    return result;
}

} // anonymous namespace

std::vector<float> HTTrain::reconstruct() const {
    if (!root) return {};
    auto expanded = expandNode(*root);
    // root has rank == 1; squeeze the trailing dim
    expanded.resize(expanded.size() / 1);  // rank==1, no-op; kept for clarity
    return expanded;  // shape: [n_0*n_1*...*n_{d-1}]
}

// ============================================================================
// toTTTrain — compatibility bridge (stub #286 resolved: memoized behind mutex)
// ============================================================================

storage::TTTrain HTTrain::toTTTrain() const {
    // Check cache first (stub #286 resolved: memoize the O(∏ n_k) conversion).
    {
        std::lock_guard<std::mutex> lock(tt_mutex_);
        if (tt_cache_) {
            return *tt_cache_;  // return copy of cached result
        }
    }

    // Compute the TT conversion (expensive: full reconstruction + redecompose).
    auto dense = reconstruct();
    storage::TensorTrainConfig cfg;
    cfg.max_rank = max_rank > 0 ? max_rank : 16;
    cfg.eps      = achieved_eps > 0.0 ? achieved_eps : 0.01;
    storage::TensorTrainDecomposer decomp;
    storage::TTTrain result = decomp.decompose(dense, shape, cfg).first;

    // Store result in cache.
    {
        std::lock_guard<std::mutex> lock(tt_mutex_);
        if (!tt_cache_) {
            tt_cache_ = std::make_shared<storage::TTTrain>(std::move(result));
        }
        return *tt_cache_;
    }
}

// ============================================================================
// Serialization
// ============================================================================

namespace {

constexpr uint64_t kHTMagic = 0x5448544D49535442ULL;  // "HTMISTB" in little-endian view
constexpr uint8_t  kHTVersion = 1;

void writeU64(std::vector<uint8_t>& buf, uint64_t v) {
    uint8_t tmp[8];
    std::memcpy(tmp, &v, 8);
    buf.insert(buf.end(), tmp, tmp + 8);
}

void writeU8(std::vector<uint8_t>& buf, uint8_t v) {
    buf.push_back(v);
}

void writeF64(std::vector<uint8_t>& buf, double v) {
    uint8_t tmp[8];
    std::memcpy(tmp, &v, 8);
    buf.insert(buf.end(), tmp, tmp + 8);
}

void writeFloats(std::vector<uint8_t>& buf, const std::vector<float>& v) {
    writeU64(buf, v.size());
    const uint8_t* p = reinterpret_cast<const uint8_t*>(v.data());
    buf.insert(buf.end(), p, p + v.size() * sizeof(float));
}

void serializeNode(std::vector<uint8_t>& buf, const HTNode* node) {
    if (!node) { writeU8(buf, 0xFF); return; }
    writeU8(buf, node->is_leaf ? 1 : 0);
    writeU64(buf, node->rank);
    if (node->is_leaf) {
        writeU64(buf, node->mode_index);
        writeU64(buf, node->n_k);
        writeFloats(buf, node->U);
    } else {
        writeU64(buf, node->r_left);
        writeU64(buf, node->r_right);
        writeFloats(buf, node->B);
        serializeNode(buf, node->left.get());
        serializeNode(buf, node->right.get());
    }
}

struct Reader {
    const uint8_t* p;
    std::size_t    left;
    bool           ok = true;

    bool readU64(uint64_t& v) {
        if (left < 8) { ok = false; return false; }
        std::memcpy(&v, p, 8); p += 8; left -= 8; return true;
    }
    bool readU8(uint8_t& v) {
        if (left < 1) { ok = false; return false; }
        v = *p++; left--; return true;
    }
    bool readF64(double& v) {
        if (left < 8) { ok = false; return false; }
        std::memcpy(&v, p, 8); p += 8; left -= 8; return true;
    }
    bool readFloats(std::vector<float>& v) {
        uint64_t n = 0;
        if (!readU64(n)) return false;
        if (left < n * sizeof(float)) { ok = false; return false; }
        v.resize(n);
        std::memcpy(v.data(), p, n * sizeof(float));
        p += n * sizeof(float); left -= n * sizeof(float); return true;
    }
};

std::unique_ptr<HTNode> deserializeNode(Reader& r) {
    uint8_t tag = 0;
    if (!r.readU8(tag)) return nullptr;
    if (tag == 0xFF) return nullptr;

    auto node = std::make_unique<HTNode>();
    uint64_t rank_u = 0;
    if (!r.readU64(rank_u)) return nullptr;
    node->rank = static_cast<std::size_t>(rank_u);

    if (tag == 1) {
        node->is_leaf = true;
        uint64_t mi = 0, nk = 0;
        if (!r.readU64(mi)) return nullptr;
        if (!r.readU64(nk)) return nullptr;
        node->mode_index = static_cast<std::size_t>(mi);
        node->n_k        = static_cast<std::size_t>(nk);
        if (!r.readFloats(node->U)) return nullptr;
    } else {
        node->is_leaf = false;
        uint64_t rl = 0, rr = 0;
        if (!r.readU64(rl)) return nullptr;
        if (!r.readU64(rr)) return nullptr;
        node->r_left  = static_cast<std::size_t>(rl);
        node->r_right = static_cast<std::size_t>(rr);
        if (!r.readFloats(node->B)) return nullptr;
        node->left  = deserializeNode(r);
        node->right = deserializeNode(r);
    }
    return node;
}

} // anonymous namespace

std::vector<uint8_t> HTTrain::serialize() const {
    std::vector<uint8_t> buf;
    writeU64(buf, kHTMagic);
    writeU8(buf, kHTVersion);
    writeU64(buf, static_cast<uint64_t>(shape.size()));
    for (auto s : shape) writeU64(buf, static_cast<uint64_t>(s));
    writeU64(buf, static_cast<uint64_t>(max_rank));
    writeF64(buf, achieved_eps);
    writeF64(buf, original_norm);
    serializeNode(buf, root.get());
    return buf;
}

std::optional<HTTrain> HTTrain::deserialize(const std::vector<uint8_t>& bytes) {
    Reader r{bytes.data(), bytes.size(), true};
    uint64_t magic = 0;
    if (!r.readU64(magic) || magic != kHTMagic) return std::nullopt;
    uint8_t ver = 0;
    if (!r.readU8(ver) || ver != kHTVersion) return std::nullopt;

    uint64_t d = 0;
    if (!r.readU64(d)) return std::nullopt;

    HTTrain ht;
    ht.shape.resize(static_cast<std::size_t>(d));
    for (auto& s : ht.shape) {
        uint64_t v = 0;
        if (!r.readU64(v)) return std::nullopt;
        s = static_cast<std::size_t>(v);
    }
    uint64_t mr = 0;
    if (!r.readU64(mr)) return std::nullopt;
    ht.max_rank = static_cast<std::size_t>(mr);
    if (!r.readF64(ht.achieved_eps))  return std::nullopt;
    if (!r.readF64(ht.original_norm)) return std::nullopt;

    ht.root = deserializeNode(r);
    if (!r.ok || !ht.root) return std::nullopt;
    return ht;
}

// ============================================================================
// HTContractionEngine
// ============================================================================

namespace {

// Recursive Gram matrix computation.
// Returns flat [r_A × r_B] matrix.
std::vector<double> gramNode(const HTNode& A, const HTNode& B) {
    std::size_t rA = A.rank;
    std::size_t rB = B.rank;

    if (A.is_leaf && B.is_leaf) {
        // Γ[alpha, beta] = sum_i U_A[i,alpha] * U_B[i,beta]   — O(n * rA * rB)
        assert(A.n_k == B.n_k);
        std::size_t n = A.n_k;
        std::vector<double> G(rA * rB, 0.0);
        for (std::size_t i = 0; i < n; ++i)
            for (std::size_t a = 0; a < rA; ++a)
                for (std::size_t b = 0; b < rB; ++b)
                    G[a * rB + b] += static_cast<double>(A.U[i * rA + a])
                                   * static_cast<double>(B.U[i * rB + b]);
        return G;
    }

    if (A.is_leaf || B.is_leaf) return {};  // incompatible topology

    // Compute child Gram matrices
    auto G_left  = gramNode(*A.left,  *B.left);
    auto G_right = gramNode(*A.right, *B.right);

    std::size_t rAl = A.r_left,  rAr = A.r_right;
    std::size_t rBl = B.r_left,  rBr = B.r_right;

    // G_node[alpha, beta] = sum_{l,r,l',r'} B_A[l,r,alpha] * G_left[l,l'] * G_right[r,r'] * B_B[l',r',beta]
    // Computed in two steps:
    // Step 1: W[l, r', alpha] = sum_{l', r} B_A[l, r, alpha] * G_left[l, l']  ... no, wrong
    // Actually:
    // Expand: W[l, r', alpha] = sum_{r} B_A[l, r, alpha] * G_right[r, r']
    //         X[alpha, beta]  = sum_{l, l'} sum_{r'} G_left[l, l'] * W[l, r', alpha] * B_B[l', r', beta]

    // Step 1: W[l, r', alpha] = sum_r B_A[l, r, alpha] * G_right[r, r']
    std::vector<double> W(rAl * rBr * rA, 0.0);
    for (std::size_t l = 0; l < rAl; ++l)
        for (std::size_t r_prime = 0; r_prime < rBr; ++r_prime)
            for (std::size_t ao = 0; ao < rA; ++ao)
                for (std::size_t r = 0; r < rAr; ++r)
                    W[l * rBr * rA + r_prime * rA + ao] +=
                        static_cast<double>(A.B[l * rAr * rA + r * rA + ao])
                        * G_right[r * rBr + r_prime];

    // Step 2: X[l', r', beta] = sum_{l, ao} G_left[l, l'] * W[l, r', ao] * B_B[l', r', beta]
    // Wait, we want G[alpha, beta] so we need to sum correctly.
    // G[ao, bo] = sum_{l, l', r'} G_left[l, l'] * W[l, r', ao] * B_B[l', r', bo]

    // Precompute V[l, r', bo] = sum_{l'} G_left[l, l'] * B_B_sum[l', r', bo]
    //   where B_B_sum[l', r', bo] = B_B[l', r', bo] (already indexed)
    // Then G[ao, bo] = sum_{l, r'} W[l, r', ao] * V[l, r', bo]

    // V[l, r', bo] = sum_{l'} G_left[l, l'] * B_B[l', r', bo]
    std::vector<double> V(rAl * rBr * rB, 0.0);
    for (std::size_t l = 0; l < rAl; ++l)
        for (std::size_t r_prime = 0; r_prime < rBr; ++r_prime)
            for (std::size_t bo = 0; bo < rB; ++bo)
                for (std::size_t l_prime = 0; l_prime < rBl; ++l_prime)
                    V[l * rBr * rB + r_prime * rB + bo] +=
                        G_left[l * rBl + l_prime]
                        * static_cast<double>(B.B[l_prime * rBr * rB + r_prime * rB + bo]);

    // G[ao, bo] = sum_{l, r'} W[l, r', ao] * V[l, r', bo]
    std::vector<double> G(rA * rB, 0.0);
    for (std::size_t l = 0; l < rAl; ++l)
        for (std::size_t r_prime = 0; r_prime < rBr; ++r_prime)
            for (std::size_t ao = 0; ao < rA; ++ao)
                for (std::size_t bo = 0; bo < rB; ++bo)
                    G[ao * rB + bo] +=
                        W[l * rBr * rA + r_prime * rA + ao]
                        * V[l * rBr * rB + r_prime * rB + bo];

    return G;
}

} // anonymous namespace

std::vector<double> HTContractionEngine::computeGram(
    const HTNode& A, const HTNode& B)
{
    return gramNode(A, B);
}

double HTContractionEngine::innerProduct(const HTTrain& A, const HTTrain& B) {
    if (!A.root || !B.root) return 0.0;
    auto G = gramNode(*A.root, *B.root);
    if (G.empty()) return 0.0;
    // Root has rank == 1 for both; G is [1 × 1]
    return G[0];
}

double HTContractionEngine::frobeniusNorm(const HTTrain& A) {
    return std::sqrt(std::max(0.0, innerProduct(A, A)));
}

double HTContractionEngine::cosineSimilarity(const HTTrain& A, const HTTrain& B) {
    double na = frobeniusNorm(A);
    double nb = frobeniusNorm(B);
    if (na < 1e-15 || nb < 1e-15) return 0.0;
    return innerProduct(A, B) / (na * nb);
}

// ============================================================================

} // namespace tensor

namespace storage {

// HierarchicalTuckerDecomposer
// ============================================================================

HierarchicalTuckerDecomposer::HierarchicalTuckerDecomposer(HTConfig cfg) noexcept
    : cfg_(cfg) {}

// ── Mode-k unfolding ──────────────────────────────────────────────────────────

std::vector<float> HierarchicalTuckerDecomposer::modeKUnfolding(
    const std::vector<float>&       data,
    const std::vector<std::size_t>& shape,
    std::size_t                     mode_k)
{
    std::size_t d = shape.size();
    std::size_t nk = shape[mode_k];
    std::size_t N  = data.size();
    std::size_t N_other = N / nk;  // n_cols

    // stride of mode k in row-major layout
    std::size_t stride_k = 1;
    for (std::size_t m = mode_k + 1; m < d; ++m) stride_k *= shape[m];
    std::size_t outer_k = N / (nk * stride_k);

    // T_(k)[j, s] where j = mode-k index, s = combined other-mode index
    // s runs over: outer blocks then inner stride blocks
    std::vector<float> mat(nk * N_other);

    for (std::size_t o = 0; o < outer_k; ++o) {
        for (std::size_t j = 0; j < nk; ++j) {
            for (std::size_t s = 0; s < stride_k; ++s) {
                std::size_t flat = o * nk * stride_k + j * stride_k + s;
                std::size_t col  = o * stride_k + s;
                mat[j * N_other + col] = data[flat];
            }
        }
    }
    return mat;  // [nk × N_other]
}

// ── Mode-k product: T ×_k U^T ────────────────────────────────────────────────

std::vector<float> HierarchicalTuckerDecomposer::modeKProduct(
    const std::vector<float>&       data,
    const std::vector<std::size_t>& shape,
    std::size_t                     mode_k,
    const std::vector<float>&       U,   // [n_k × r]
    std::size_t                     n_k,
    std::size_t                     r)
{
    std::size_t d = shape.size();
    std::size_t N = data.size();

    // stride / outer dimensions around mode k
    std::size_t stride_k = 1;
    for (std::size_t m = mode_k + 1; m < d; ++m) stride_k *= shape[m];
    std::size_t outer_k = N / (n_k * stride_k);

    // Result: same shape with shape[mode_k] replaced by r
    std::vector<float> result(outer_k * r * stride_k, 0.0f);

    for (std::size_t o = 0; o < outer_k; ++o) {
        for (std::size_t alpha = 0; alpha < r; ++alpha) {
            for (std::size_t s = 0; s < stride_k; ++s) {
                float val = 0.0f;
                for (std::size_t ik = 0; ik < n_k; ++ik) {
                    val += U[ik * r + alpha] * data[o * n_k * stride_k + ik * stride_k + s];
                }
                result[o * r * stride_k + alpha * stride_k + s] = val;
            }
        }
    }
    return result;
}

// ── Symmetric Jacobi EVD (STUB #180) ─────────────────────────────────────────

namespace {

/// Symmetric Jacobi eigendecomposition of an n×n matrix A (passed as flat row-major).
/// Returns eigenvectors in `V` (columns) and eigenvalues in `lambda`, DESCENDING order.
/// A is overwritten.
///
/// STUB #180: Jacobi EVD O(n³ · iter); max_iter = 50 sweeps.
static void jacobiEVD(std::vector<double>& A, std::size_t n,
                      std::vector<double>& V, std::vector<double>& lambda)
{
    // Initialise V = I
    V.assign(n * n, 0.0);
    for (std::size_t i = 0; i < n; ++i) V[i * n + i] = 1.0;

    constexpr int    MAX_SWEEPS = 50;
    constexpr double TOL        = 1e-13;

    for (int sweep = 0; sweep < MAX_SWEEPS; ++sweep) {
        double off = 0.0;
        for (std::size_t i = 0; i < n; ++i)
            for (std::size_t j = i + 1; j < n; ++j)
                off += A[i * n + j] * A[i * n + j];
        if (off < TOL * TOL) break;

        // One full sweep: all off-diagonal (p, q) pairs
        for (std::size_t p = 0; p < n - 1; ++p) {
            for (std::size_t q = p + 1; q < n; ++q) {
                double Apq = A[p * n + q];
                if (std::abs(Apq) < 1e-15) continue;

                double App = A[p * n + p];
                double Aqq = A[q * n + q];
                double tau = (Aqq - App) / (2.0 * Apq);
                double t   = (tau >= 0.0 ? 1.0 : -1.0)
                             / (std::abs(tau) + std::sqrt(1.0 + tau * tau));
                double c   = 1.0 / std::sqrt(1.0 + t * t);
                double s   = t * c;

                // Update A
                A[p * n + p] = App - t * Apq;
                A[q * n + q] = Aqq + t * Apq;
                A[p * n + q] = A[q * n + p] = 0.0;
                for (std::size_t i = 0; i < n; ++i) {
                    if (i == p || i == q) continue;
                    double Aip = A[i * n + p];
                    double Aiq = A[i * n + q];
                    A[i * n + p] = A[p * n + i] = c * Aip - s * Aiq;
                    A[i * n + q] = A[q * n + i] = s * Aip + c * Aiq;
                }
                // Update V
                for (std::size_t i = 0; i < n; ++i) {
                    double Vip = V[i * n + p];
                    double Viq = V[i * n + q];
                    V[i * n + p] = c * Vip - s * Viq;
                    V[i * n + q] = s * Vip + c * Viq;
                }
            }
        }
    }

    // Extract and sort eigenvalues (descending)
    lambda.resize(n);
    for (std::size_t i = 0; i < n; ++i) lambda[i] = A[i * n + i];

    std::vector<std::size_t> idx(n);
    std::iota(idx.begin(), idx.end(), 0);
    std::sort(idx.begin(), idx.end(),
              [&](std::size_t a, std::size_t b) { return lambda[a] > lambda[b]; });

    std::vector<double> lam2(n), V2(n * n);
    for (std::size_t k = 0; k < n; ++k) {
        lam2[k] = lambda[idx[k]];
        for (std::size_t i = 0; i < n; ++i)
            V2[i * n + k] = V[i * n + idx[k]];
    }
    lambda = lam2;
    V      = V2;
}

} // anonymous namespace

// ── Truncated SVD ─────────────────────────────────────────────────────────────

void HierarchicalTuckerDecomposer::truncatedSVD(
    const std::vector<float>& mat,
    std::size_t               m,
    std::size_t               n,
    double                    delta,
    std::size_t               max_rank_cap,
    std::vector<float>&       U_out,
    std::vector<float>&       S_out,
    std::vector<float>&       Vt_out,
    std::size_t&              rank_out)
{
    if (m == 0 || n == 0) { rank_out = 0; return; }
    std::size_t small_dim = std::min(m, n);
    if (max_rank_cap == 0) max_rank_cap = small_dim;
    max_rank_cap = std::min(max_rank_cap, small_dim);

    // Choose whether to form A·A^T (if m ≤ n) or A^T·A (if n < m)
    bool use_AAt = (m <= n);

    std::vector<double> lambda;
    std::vector<double> V_evd;

    if (use_AAt) {
        // Gram = A · A^T  (m × m)
        std::vector<double> AAt(m * m, 0.0);
        for (std::size_t i = 0; i < m; ++i)
            for (std::size_t j = i; j < m; ++j) {
                double v = 0.0;
                for (std::size_t k = 0; k < n; ++k)
                    v += static_cast<double>(mat[i * n + k])
                       * static_cast<double>(mat[j * n + k]);
                AAt[i * m + j] = AAt[j * m + i] = v;
            }
        jacobiEVD(AAt, m, V_evd, lambda);
    } else {
        // Gram = A^T · A  (n × n)
        std::vector<double> AtA(n * n, 0.0);
        for (std::size_t i = 0; i < n; ++i)
            for (std::size_t j = i; j < n; ++j) {
                double v = 0.0;
                for (std::size_t k = 0; k < m; ++k)
                    v += static_cast<double>(mat[k * n + i])
                       * static_cast<double>(mat[k * n + j]);
                AtA[i * n + j] = AtA[j * n + i] = v;
            }
        jacobiEVD(AtA, n, V_evd, lambda);
    }

    // Determine rank
    double tol_sq = delta * delta;
    rank_out = 0;
    for (std::size_t k = 0; k < max_rank_cap; ++k) {
        if (k < lambda.size() && lambda[k] > tol_sq) {
            rank_out = k + 1;
        } else {
            break;
        }
    }
    if (rank_out == 0) rank_out = 1;

    S_out.resize(rank_out);
    for (std::size_t k = 0; k < rank_out; ++k)
        S_out[k] = static_cast<float>(std::sqrt(std::max(0.0, lambda[k])));

    if (use_AAt) {
        // V_evd columns → left singular vectors U
        // U = V_evd[:, 0..r-1]   (m × rank_out)
        U_out.resize(m * rank_out);
        for (std::size_t i = 0; i < m; ++i)
            for (std::size_t k = 0; k < rank_out; ++k)
                U_out[i * rank_out + k] = static_cast<float>(V_evd[i * m + k]);

        // Vt = diag(1/S) · U^T · A  (rank_out × n)
        Vt_out.resize(rank_out * n, 0.0f);
        for (std::size_t k = 0; k < rank_out; ++k) {
            float inv_s = (S_out[k] > 1e-10f) ? 1.0f / S_out[k] : 0.0f;
            for (std::size_t j = 0; j < n; ++j) {
                float v = 0.0f;
                for (std::size_t i = 0; i < m; ++i)
                    v += U_out[i * rank_out + k] * mat[i * n + j];
                Vt_out[k * n + j] = v * inv_s;
            }
        }
    } else {
        // V_evd columns → right singular vectors V  (n × rank_out)
        // Vt[k, j] = V_evd[j, k]
        Vt_out.resize(rank_out * n);
        for (std::size_t k = 0; k < rank_out; ++k)
            for (std::size_t j = 0; j < n; ++j)
                Vt_out[k * n + j] = static_cast<float>(V_evd[j * n + k]);

        // U = A · V · diag(1/S)   (m × rank_out)
        U_out.resize(m * rank_out, 0.0f);
        for (std::size_t i = 0; i < m; ++i) {
            for (std::size_t k = 0; k < rank_out; ++k) {
                float v = 0.0f;
                for (std::size_t j = 0; j < n; ++j)
                    v += mat[i * n + j] * Vt_out[k * n + j];
                float inv_s = (S_out[k] > 1e-10f) ? 1.0f / S_out[k] : 0.0f;
                U_out[i * rank_out + k] = v * inv_s;
            }
        }
    }
}

// ── buildHTNode — top-down HT construction ────────────────────────────────────

/*
 * The core tensor passed to this function has shape:
 *   [phys_L, phys_{L+1}, ..., phys_{R-1}, r_out]   (row-major)
 * where:
 *   - phys_k  = Tucker rank of mode k from HOSVD
 *   - r_out   = output rank expected by the parent (1 at the root call)
 *
 * Base cases:
 *   d_sub == 1:  leaf — compute U_eff = U_cache[L] @ core  (n_L × r_out)
 *   d_sub == 2:  leaf-pair — B = core  (shape [phys_L, phys_{L+1}, r_out])
 *
 * Recursive case (d_sub > 2, M = (L+R)/2):
 *   SVD-1: unfold core along [L..M-1] vs [M..R-1, out]
 *          → G_left  [phys_L,...,phys_{M-1}, r_inner]
 *          → G_right_raw  [n_right * r_out, r_inner]
 *   SVD-2: unfold G_right_raw as [n_right, r_out * r_inner]
 *          → G_right  [phys_M,...,phys_{R-1}, r_23]
 *          → B_node   [r_inner, r_23, r_out]  (transfer tensor at this node)
 */
std::unique_ptr<tensor::HTNode>
HierarchicalTuckerDecomposer::buildHTNode(
    const std::vector<float>&              core,
    const std::vector<std::size_t>&        core_shape,  // length = (R-L) + 1
    std::size_t                            L,
    std::size_t                            R,
    const std::vector<std::vector<float>>& U_cache,
    const std::vector<std::size_t>&        T_shape) const
{
    std::size_t d_sub = R - L;
    std::size_t r_out = core_shape.back();  // last dim = output rank

    // ── Base case: single leaf ─────────────────────────────────────────────────
    if (d_sub == 1) {
        // core has shape [phys_L, r_out]
        // Effective U = U_cache[L] @ core  (n_L × r_out)
        std::size_t n_L   = T_shape[L];
        std::size_t phys_L = core_shape[0];  // = Tucker rank of mode L
        const auto& U_k    = U_cache[L];     // [n_L × phys_L]

        auto node = std::make_unique<tensor::HTNode>();
        node->is_leaf    = true;
        node->mode_index = L;
        node->n_k        = n_L;
        node->rank       = r_out;
        node->U.assign(n_L * r_out, 0.0f);

        for (std::size_t i = 0; i < n_L; ++i)
            for (std::size_t ao = 0; ao < r_out; ++ao) {
                float val = 0.0f;
                for (std::size_t al = 0; al < phys_L; ++al)
                    val += U_k[i * phys_L + al] * core[al * r_out + ao];
                node->U[i * r_out + ao] = val;
            }
        return node;
    }

    // ── Base case: leaf-pair ───────────────────────────────────────────────────
    if (d_sub == 2) {
        // core = [phys_L, phys_{L+1}, r_out]  → this IS the transfer tensor B
        std::size_t phys_L  = core_shape[0];
        std::size_t phys_R  = core_shape[1];

        auto node = std::make_unique<tensor::HTNode>();
        node->is_leaf  = false;
        node->r_left   = phys_L;
        node->r_right  = phys_R;
        node->rank     = r_out;
        node->B        = core;  // [phys_L × phys_R × r_out]

        // Left leaf: U_cache[L]  [n_L × phys_L]
        auto leaf_left = std::make_unique<tensor::HTNode>();
        leaf_left->is_leaf    = true;
        leaf_left->mode_index = L;
        leaf_left->n_k        = T_shape[L];
        leaf_left->rank       = phys_L;
        leaf_left->U          = U_cache[L];

        // Right leaf: U_cache[L+1]  [n_{L+1} × phys_R]
        auto leaf_right = std::make_unique<tensor::HTNode>();
        leaf_right->is_leaf    = true;
        leaf_right->mode_index = L + 1;
        leaf_right->n_k        = T_shape[L + 1];
        leaf_right->rank       = phys_R;
        leaf_right->U          = U_cache[L + 1];

        node->left  = std::move(leaf_left);
        node->right = std::move(leaf_right);
        return node;
    }

    // ── Recursive case ─────────────────────────────────────────────────────────
    std::size_t M = (L + R) / 2;

    // Compute n_left and n_right (products of physical dims)
    std::size_t n_left = 1, n_right = 1;
    for (std::size_t k = 0; k < M - L; ++k)  n_left  *= core_shape[k];
    for (std::size_t k = M - L; k < d_sub; ++k) n_right *= core_shape[k];

    // SVD-1: core_mat[i_left, (i_right * r_out + gamma_out)]
    //        shape: [n_left, n_right * r_out]
    // The core in row-major is already laid out correctly for this split
    // (left dims are outer, right+out dims are inner).
    std::size_t n_right_out = n_right * r_out;

    // Compute delta for SVD from overall eps
    double delta = 0.0;  // use max_rank truncation; eps applied globally

    std::vector<float> U1, S1, Vt1;
    std::size_t r_inner = 0;
    truncatedSVD(core, n_left, n_right_out,
                 delta, cfg_.max_rank,
                 U1, S1, Vt1, r_inner);

    // G_left[i_left, gamma_inner] = U1[i_left, gamma_inner] * sqrt(S1[gamma_inner])
    // Shape: [n_left, r_inner]  → will be reshaped to [phys_L,...,phys_{M-1}, r_inner]
    std::vector<float> G_left(n_left * r_inner);
    for (std::size_t il = 0; il < n_left; ++il)
        for (std::size_t g = 0; g < r_inner; ++g)
            G_left[il * r_inner + g] = U1[il * r_inner + g] * std::sqrt(S1[g]);

    // G_right_raw[i_right_out, gamma_inner] = Vt1^T[i_right_out, gamma_inner] * sqrt(S1)
    // Vt1 is [r_inner × n_right_out], so Vt1^T is [n_right_out × r_inner]
    // G_right_raw shape: [n_right, r_out, r_inner] → reshaped as [n_right, r_out * r_inner]
    std::vector<float> G_right_raw(n_right_out * r_inner);
    for (std::size_t j = 0; j < n_right_out; ++j)
        for (std::size_t g = 0; g < r_inner; ++g)
            G_right_raw[j * r_inner + g] = Vt1[g * n_right_out + j] * std::sqrt(S1[g]);

    // SVD-2: G_right_raw as matrix [n_right, r_out * r_inner]
    // After this: G_right = U2 * diag(sqrt(S2)) → shape [n_right, r_23]
    //             B_node_raw = Vt2^T * diag(sqrt(S2)) → shape [r_out * r_inner, r_23]
    std::vector<float> U2, S2, Vt2;
    std::size_t r_23 = 0;
    truncatedSVD(G_right_raw, n_right, r_out * r_inner,
                 delta, cfg_.max_rank,
                 U2, S2, Vt2, r_23);

    // G_right [n_right, r_23]
    std::vector<float> G_right(n_right * r_23);
    for (std::size_t ir = 0; ir < n_right; ++ir)
        for (std::size_t g = 0; g < r_23; ++g)
            G_right[ir * r_23 + g] = U2[ir * r_23 + g] * std::sqrt(S2[g]);

    // B_node_raw: Vt2^T [r_out * r_inner, r_23]  →  Vt2 [r_23, r_out * r_inner]
    // B_node_raw[j, g23] = Vt2[g23, j] * sqrt(S2[g23])
    // Reshape to [r_out, r_inner, r_23]
    // Then permute to [r_inner, r_23, r_out]:
    //   B_node[g_inner, g23, g_out] = B_node_raw_reshaped[g_out, g_inner, g23]
    //                                = Vt2[g23, g_out * r_inner + g_inner] * sqrt(S2[g23])
    std::vector<float> B_node(r_inner * r_23 * r_out);
    for (std::size_t gi = 0; gi < r_inner; ++gi)
        for (std::size_t g23 = 0; g23 < r_23; ++g23)
            for (std::size_t go = 0; go < r_out; ++go)
                B_node[gi * r_23 * r_out + g23 * r_out + go] =
                    Vt2[g23 * (r_out * r_inner) + go * r_inner + gi] * std::sqrt(S2[g23]);

    // Build this internal node
    auto node = std::make_unique<tensor::HTNode>();
    node->is_leaf  = false;
    node->r_left   = r_inner;
    node->r_right  = r_23;
    node->rank     = r_out;
    node->B        = std::move(B_node);

    // Recurse: left subtree with G_left
    // G_left shape: core_shape[0..M-L-1] appended with r_inner
    std::vector<std::size_t> left_shape(core_shape.begin(), core_shape.begin() + (M - L));
    left_shape.push_back(r_inner);
    node->left = buildHTNode(G_left, left_shape, L, M, U_cache, T_shape);

    // Recurse: right subtree with G_right
    // G_right shape: core_shape[M-L..d_sub-1] appended with r_23
    std::vector<std::size_t> right_shape(core_shape.begin() + (M - L), core_shape.begin() + d_sub);
    right_shape.push_back(r_23);
    node->right = buildHTNode(G_right, right_shape, M, R, U_cache, T_shape);

    return node;
}

// ── decompose ─────────────────────────────────────────────────────────────────

std::pair<tensor::HTTrain, HierarchicalTuckerDecomposer::Stats>
HierarchicalTuckerDecomposer::decompose(
    const std::vector<float>&       data,
    const std::vector<std::size_t>& shape) const
{
    std::size_t d = shape.size();
    if (d < 2) throw std::invalid_argument("HTDecomposer: need at least 2 modes");

    std::size_t N = 1;
    for (auto s : shape) {
        if (s == 0) throw std::invalid_argument("HTDecomposer: mode size 0 is invalid");
        N *= s;
    }
    if (data.size() != N)
        throw std::invalid_argument("HTDecomposer: data.size() != product of shape");

    // ── Step 1: HOSVD — compute mode-k SVDs for leaf bases ────────────────────
    std::vector<std::vector<float>> U_cache(d);  // U_cache[k] ∈ [n_k × r_k]
    std::vector<std::size_t>        ranks(d);

    double total_sq = 0.0;
    for (float v : data) total_sq += static_cast<double>(v) * v;
    double orig_norm = std::sqrt(total_sq);

    // Global delta for leaf SVDs: eps * ||T|| / sqrt(d)
    double leaf_delta = cfg_.eps * orig_norm / std::sqrt(static_cast<double>(d));

    for (std::size_t k = 0; k < d; ++k) {
        std::size_t nk      = shape[k];
        std::size_t n_other = N / nk;
        auto Tk = modeKUnfolding(data, shape, k);  // [nk × n_other]

        std::vector<float> U, S, Vt;
        std::size_t r = 0;
        truncatedSVD(Tk, nk, n_other, leaf_delta, cfg_.max_rank, U, S, Vt, r);

        U_cache[k] = std::move(U);  // [nk × r]
        ranks[k]   = r;
    }

    // ── Step 2: Tucker core G = T ×_0 U_0^T … ×_{d-1} U_{d-1}^T ─────────────
    std::vector<float>        G      = data;
    std::vector<std::size_t>  G_shape = shape;

    for (std::size_t k = 0; k < d; ++k) {
        G       = modeKProduct(G, G_shape, k, U_cache[k], G_shape[k], ranks[k]);
        G_shape[k] = ranks[k];
    }
    // G now has shape [r_0, ..., r_{d-1}]

    // ── Step 2b: HOOI alternating optimization ─────────────────────────────────
    // Minimizes ‖T − T̃‖_F / ‖T‖_F by alternating SVD updates of each factor.
    // Convergence check: relative core-norm change between iterations.
    if (cfg_.max_hooi_iter > 0) {
        double prev_core_norm = 0.0;
        for (float v : G) prev_core_norm += static_cast<double>(v) * v;
        prev_core_norm = std::sqrt(prev_core_norm);

        for (std::size_t iter = 0; iter < cfg_.max_hooi_iter; ++iter) {
            // Update each factor U_k: project T along all other modes and re-SVD.
            for (std::size_t k = 0; k < d; ++k) {
                // Contract T with all factors except k: Y = T ×_{j≠k} U_j^T
                std::vector<float>       Y      = data;
                std::vector<std::size_t> Y_shape = shape;
                for (std::size_t j = 0; j < d; ++j) {
                    if (j == k) continue;
                    Y       = modeKProduct(Y, Y_shape, j, U_cache[j], Y_shape[j], ranks[j]);
                    Y_shape[j] = ranks[j];
                }
                // Unfold Y in mode k: [n_k × (∏_{j≠k} r_j)]
                std::size_t n_other_Y = 1;
                for (std::size_t j = 0; j < d; ++j)
                    if (j != k) n_other_Y *= Y_shape[j];
                auto Yk = modeKUnfolding(Y, Y_shape, k);

                // SVD to refresh U_k and ranks[k]
                std::vector<float> U_new, S_new, Vt_new;
                std::size_t r_new = 0;
                truncatedSVD(Yk, Y_shape[k], n_other_Y,
                             leaf_delta, cfg_.max_rank,
                             U_new, S_new, Vt_new, r_new);

                U_cache[k] = std::move(U_new);
                ranks[k]   = r_new;
            }

            // Recompute core G with updated factors.
            G       = data;
            G_shape = shape;
            for (std::size_t k = 0; k < d; ++k) {
                G       = modeKProduct(G, G_shape, k, U_cache[k], G_shape[k], ranks[k]);
                G_shape[k] = ranks[k];
            }

            // Check convergence via relative core-norm change.
            double cur_core_norm = 0.0;
            for (float v : G) cur_core_norm += static_cast<double>(v) * v;
            cur_core_norm = std::sqrt(cur_core_norm);

            double rel_change = (orig_norm > 0.0)
                ? std::abs(cur_core_norm - prev_core_norm) / orig_norm
                : 0.0;
            prev_core_norm = cur_core_norm;

            if (rel_change < cfg_.eps * 1e-2) break;  // converged
        }
    }

    // ── Step 3: Build HT tree top-down from G ─────────────────────────────────
    // Augment G with a trailing r_out=1 dimension
    std::vector<std::size_t> root_core_shape = G_shape;
    root_core_shape.push_back(1);  // r_out = 1 at root
    // root_core data = G (no data copy needed; the trailing dim is just a view)

    auto root = buildHTNode(G, root_core_shape, 0, d, U_cache, shape);

    // ── Assemble HTTrain ───────────────────────────────────────────────────────
    tensor::HTTrain ht;
    ht.root          = std::move(root);
    ht.shape         = shape;
    ht.max_rank      = cfg_.max_rank;
    ht.original_norm = orig_norm;

    // Estimate achieved eps
    if (orig_norm > 0.0) {
        auto reconstructed = ht.reconstruct();
        double err_sq = 0.0;
        for (std::size_t i = 0; i < N; ++i) {
            double diff = static_cast<double>(data[i]) - static_cast<double>(reconstructed[i]);
            err_sq += diff * diff;
        }
        ht.achieved_eps = std::sqrt(err_sq) / orig_norm;
    } else {
        ht.achieved_eps = 0.0;
    }

    std::size_t max_rank_used = 0;
    for (std::size_t r : ranks) max_rank_used = std::max(max_rank_used, r);

    Stats stats;
    stats.num_modes      = d;
    stats.max_rank_used  = max_rank_used;
    stats.achieved_eps   = ht.achieved_eps;
    stats.original_norm  = orig_norm;
    stats.total_params   = ht.totalParams();

    return {std::move(ht), stats};
}

} // namespace storage
} // namespace themis
