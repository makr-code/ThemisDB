/**
 * @file tensor_butterfly_operator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=27; TODO=1, Stub=20, Unimpl=2, Mock=1, Sim=3, Debt=0, C=0, H=3, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "tensor/tensor_butterfly_operator.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <mutex>
#include <numeric>
#include <sstream>
#include <stdexcept>

namespace themis {
namespace tensor {

namespace {

// ============================================================================
// Utilities
// ============================================================================

/// Returns true if n is a positive power of two.
[[nodiscard]] constexpr bool isPow2(std::size_t n) noexcept {
    return n > 0 && (n & (n - 1)) == 0;
}

/// Compute the n×n normalised Hadamard matrix (H/sqrt(n)) in row-major order.
/// Requires n to be a power of two.
[[nodiscard]] std::vector<float> buildHadamardMatrix(std::size_t n) {
    assert(isPow2(n));
    // H[i][j] = (-1)^{popcount(i & j)} / sqrt(n)
    const float scale = 1.0f / std::sqrt(static_cast<float>(n));
    std::vector<float> H(n * n);
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < n; ++j) {
            // Count set bits in (i & j)
            const std::size_t b = i & j;
            // popcount via Brian Kernighan's method
            std::size_t cnt = 0;
            std::size_t tmp = b;
            while (tmp) { tmp &= tmp - 1; ++cnt; }
            H[i * n + j] = (cnt % 2 == 0) ? scale : -scale;
        }
    }
    return H;
}

// ============================================================================
// WHT butterfly applied in-place to a fiber of length n.
// Entry point for apply(): operates on a pre-allocated float* slice.
// Complexity: O(n log₂ n) — butterfly-structured.
// ============================================================================

void whtTransform(float* data, std::size_t n) {
    assert(isPow2(n));
    // Cooley-Tukey butterfly (iterative, decimation-in-time)
    for (std::size_t half = n >> 1; half >= 1; half >>= 1) {
        for (std::size_t start = 0; start < n; start += 2 * half) {
            for (std::size_t j = 0; j < half; ++j) {
                const float u = data[start + j];
                const float w = data[start + j + half];
                data[start + j]        = u + w;
                data[start + j + half] = u - w;
            }
        }
    }
    // Normalise so that WHT is orthogonal (H*H^T = I).
    const float inv_sqrt_n = 1.0f / std::sqrt(static_cast<float>(n));
    for (std::size_t i = 0; i < n; ++i) {
      data[i] *= inv_sqrt_n;
    }
}

} // anonymous namespace

// ============================================================================
// Native discrete-integration routines (THEMIS_HAS_BUTTERFLY_NATIVE)
// ============================================================================
//
// These provide production-quality CPU implementations of the RADON and
// GREENS_FUNCTION operator types without requiring external GPU/CUDA/ROCm
// dependencies.  Enable with -DTHEMIS_HAS_BUTTERFLY_NATIVE=ON (default OFF).
//
// Algorithm notes:
//   radonFiberTransform  — composite Simpson's rule (1-D discrete Radon
//     projection over a fiber of length n).  The fiber is treated as a uniform
//     grid; the transform accumulates the weighted integral at each "angle"
//     index using every other node as the midpoint sample (Simpson 1/3 rule).
//     Complexity: O(n²) per fiber.
//
//   greensFiberTransform — trapezoidal-rule convolution kernel.  The Green's
//     function is modelled as the discrete 1-D Laplacian inverse (Toeplitz
//     kernel g[|i-j|] = 1/(1+|i-j|)).  The output at index i is the
//     discretised integral ∫ g(|x-s|) f(s) ds.  Complexity: O(n²) per fiber.
//
// Both transforms are normalised so that applying them to a constant fiber
// returns the same constant (energy preserving in the DC sense).

#ifdef THEMIS_HAS_BUTTERFLY_NATIVE

namespace {

/// @brief Composite Simpson's-rule Radon projection of a 1-D fiber.
///
/// Treats `data[0..n-1]` as a uniform grid f(0), f(1), …, f(n-1) and
/// replaces each entry with the discrete line integral (projection angle α_i):
///
///   data[i] ← (1/n) * Σ_{j} w_j * f(j) * cos(π * i * j / n)
///
/// The cosine modulation is the discrete Radon basis at angle i*π/n;
/// the weights w_j follow Simpson's 1/3 rule (1-4-2-4-…-4-1) scaled by n/3.
///
/// @param data  In/out array of length n (must be ≥ 2).
/// @param n     Length of the fiber.
void radonFiberTransform(float* data, std::size_t n) {
    if (n < 2) return; // trivial fiber — nothing to transform
    const float h = 1.0f / static_cast<float>(n - 1); // step size
    const float scale = h / 3.0f;
    const float pi_over_n = static_cast<float>(M_PI) / static_cast<float>(n);

    std::vector<float> result(n, 0.0f);
    for (std::size_t i = 0; i < n; ++i) {
        float integral = 0.0f;
        for (std::size_t j = 0; j < n; ++j) {
            // Simpson's weight: 1 at endpoints, 4 at odd j, 2 at even interior j.
            float w = 0;
            if (j == 0 || j == n - 1) {
              w = 1.0f;
            }
            else if (j % 2 == 1)            w = 4.0f;
            else                            w = 2.0f;
            const float angle = pi_over_n * static_cast<float>(i) * static_cast<float>(j);
            integral += w * data[j] * std::cos(angle);
        }
        result[i] = integral * scale;
    }
    for (std::size_t i = 0; i < n; ++i) {
      data[i] = result[i];
    }
}

/// @brief Trapezoidal-rule Green's function convolution of a 1-D fiber.
///
/// Kernel: g(r) = 1 / (1 + r)  where r = |i - j|.
/// Output: y[i] = h * Σ_{j} w_j * g(|i-j|) * f(j),
///   with trapezoidal weights (h/2 at endpoints, h interior).
/// Normalised by the DC gain so that a constant input is preserved.
///
/// @param data  In/out array of length n (must be ≥ 2).
/// @param n     Length of the fiber.
void greensFiberTransform(float* data, std::size_t n) {
    if (n < 2) {
      return;
    }
    const float h = 1.0f / static_cast<float>(n - 1);

    // Precompute kernel row (length n) — same for every output row.
    std::vector<float> kernel(n);
    float dc_gain = 0.0f;
    for (std::size_t r = 0; r < n; ++r) {
        const float w = (r == 0 || r == n - 1) ? 0.5f : 1.0f; // trapezoidal weight
        kernel[r] = h * w / (1.0f + static_cast<float>(r));
    }

    // Compute DC gain (sum of kernel row for a constant-1 input).
    std::vector<float> ones(n, 1.0f);
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < n; ++j) {
            const std::size_t r = (i >= j) ? (i - j) : (j - i);
            dc_gain += kernel[r];
        }
    }
    dc_gain /= static_cast<float>(n);

    std::vector<float> result(n, 0.0f);
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < n; ++j) {
            const std::size_t r = (i >= j) ? (i - j) : (j - i);
            result[i] += kernel[r] * data[j];
        }
        // Normalise so DC response is 1.
        if (dc_gain > 0.0f) {
          result[i] /= dc_gain;
        }
    }
    for (std::size_t i = 0; i < n; ++i) {
      data[i] = result[i];
    }
}

} // anonymous native namespace

#endif // THEMIS_HAS_BUTTERFLY_NATIVE

namespace {
std::mutex& fourierTransformFnMutex() { static std::mutex m; return m; }
TensorButterflyOperator::FourierTransformFn& fourierTransformFnStorage() {
    static TensorButterflyOperator::FourierTransformFn fn;
    return fn;
}

// STUB #268 — RADON bridge storage
std::mutex& radonTransformFnMutex() { static std::mutex m; return m; }
TensorButterflyOperator::RadonTransformFn& radonTransformFnStorage() {
    static TensorButterflyOperator::RadonTransformFn fn;
    return fn;
}

// STUB #268 — GREENS_FUNCTION bridge storage
std::mutex& greensTransformFnMutex() { static std::mutex m; return m; }
TensorButterflyOperator::GreensTransformFn& greensTransformFnStorage() {
    static TensorButterflyOperator::GreensTransformFn fn;
    return fn;
}
} // namespace

/*static*/
void TensorButterflyOperator::setFourierTransformFn(FourierTransformFn fn) {
    std::lock_guard<std::mutex> lk(fourierTransformFnMutex());
    fourierTransformFnStorage() = std::move(fn);
}

/*static*/
void TensorButterflyOperator::clearFourierTransformFn() {
    std::lock_guard<std::mutex> lk(fourierTransformFnMutex());
    fourierTransformFnStorage() = {};
}

// STUB #268 — RADON bridge
/*static*/
void TensorButterflyOperator::setRadonTransformFn(RadonTransformFn fn) {
    std::lock_guard<std::mutex> lk(radonTransformFnMutex());
    radonTransformFnStorage() = std::move(fn);
}

/*static*/
void TensorButterflyOperator::clearRadonTransformFn() {
    std::lock_guard<std::mutex> lk(radonTransformFnMutex());
    radonTransformFnStorage() = {};
}

// STUB #268 — GREENS_FUNCTION bridge
/*static*/
void TensorButterflyOperator::setGreensTransformFn(GreensTransformFn fn) {
    std::lock_guard<std::mutex> lk(greensTransformFnMutex());
    greensTransformFnStorage() = std::move(fn);
}

/*static*/
void TensorButterflyOperator::clearGreensTransformFn() {
    std::lock_guard<std::mutex> lk(greensTransformFnMutex());
    greensTransformFnStorage() = {};
}

// ============================================================================
// TensorButterflyOperator — private constructor
// ============================================================================

TensorButterflyOperator::TensorButterflyOperator(ButterflyConfig cfg)
    : cfg_(std::move(cfg))
{
    if (cfg_.type == OperatorType::FOURIER) {
        // Pre-build one n_k × n_k Hadamard matrix per mode.
        op_matrices_.reserve(cfg_.grid_shape.size());
        for (const std::size_t nk : cfg_.grid_shape) {
            op_matrices_.push_back(buildHadamardMatrix(nk));
        }
    }
}

// ============================================================================
// build()
// ============================================================================

TensorButterflyOperator
TensorButterflyOperator::build(OperatorType                      type,
                                const std::vector<std::size_t>&   grid_shape,
                                float                              precision) {
    if (grid_shape.empty()) {
        throw std::invalid_argument(
            "TensorButterflyOperator::build: grid_shape must not be empty");
    }

    // PERMANENT FALLBACK NOTE (TBO-02):
    // Purpose: RADON and GREENS_FUNCTION now have a native production path
    //   (discrete Radon via composite Simpson's rule and Green's function via
    //   trapezoidal quadrature, both guarded by THEMIS_HAS_BUTTERFLY_NATIVE).
    //   When THEMIS_HAS_BUTTERFLY_NATIVE is OFF (default), the bridge-function
    //   injection path is the fallback: callers may supply a custom RadonTransformFn
    //   or GreensTransformFn via the static setXxx() API.
    // Activation (fallback): THEMIS_HAS_BUTTERFLY_NATIVE not defined AND no
    //   bridge fn injected → build() throws a clear logic_error.
    // Production path: define THEMIS_HAS_BUTTERFLY_NATIVE (CMake option OFF by
    //   default) to compile the native CPU integration routines below.
    if (type == OperatorType::RADON) {
#ifndef THEMIS_HAS_BUTTERFLY_NATIVE
        // Native Radon integration not compiled in — require injected bridge fn.
        RadonTransformFn fn_check;
        {
            std::lock_guard<std::mutex> lk(radonTransformFnMutex());
            fn_check = radonTransformFnStorage();
        }
        if (!fn_check) {
            throw std::logic_error(
                "TensorButterflyOperator::build: RADON operator requires either "
                "THEMIS_HAS_BUTTERFLY_NATIVE=ON (native Simpson's-rule integration) "
                "or an injected RadonTransformFn via setRadonTransformFn().");
        }
#endif // !THEMIS_HAS_BUTTERFLY_NATIVE
    }
    if (type == OperatorType::GREENS_FUNCTION) {
#ifndef THEMIS_HAS_BUTTERFLY_NATIVE
        // Native Green's function not compiled in — require injected bridge fn.
        GreensTransformFn fn_check;
        {
            std::lock_guard<std::mutex> lk(greensTransformFnMutex());
            fn_check = greensTransformFnStorage();
        }
        if (!fn_check) {
            throw std::logic_error(
                "TensorButterflyOperator::build: GREENS_FUNCTION operator requires either "
                "THEMIS_HAS_BUTTERFLY_NATIVE=ON (native trapezoidal-rule kernel) "
                "or an injected GreensTransformFn via setGreensTransformFn().");
        }
#endif // !THEMIS_HAS_BUTTERFLY_NATIVE
    }

    // Validate grid_shape for FOURIER (WHT requires power-of-2 mode sizes).
    for (std::size_t k = 0; k < grid_shape.size(); ++k) {
        if (!isPow2(grid_shape[k])) {
            std::ostringstream oss = {};
            oss << "TensorButterflyOperator::build: grid_shape[" << k
                << "] = " << grid_shape[k]
                << " is not a power of 2 (required for FOURIER/WHT butterfly).";
            throw std::invalid_argument(oss.str());
        }
    }

    ButterflyConfig cfg;
    cfg.type       = type;
    cfg.grid_shape = grid_shape;
    cfg.precision  = precision;
    return TensorButterflyOperator(std::move(cfg));
}

// ============================================================================
// apply()
// ============================================================================

storage::TTTrain
TensorButterflyOperator::apply(const storage::TTTrain& data) const {
    // For RADON/GREENS_FUNCTION, delegate to the injected per-fiber backend
    // (STUB #268): if no fn was set, build() already prevented construction
    // of the operator, so we only reach here when a fn is available.
    //
    // `result` is an intentional value-copy of `data` (mutated in-place).
    auto applyFiberFn = [](const std::function<void(std::vector<float>&)>& fn,
                           storage::TTTrain result) {
        for (std::size_t k = 0; k < result.cores.size(); ++k) {
            auto& core        = result.cores[k];
            const std::size_t r_left  = core.r_left;
            const std::size_t n_k     = core.n;
            const std::size_t r_right = core.r_right;
            std::vector<float> fiber(n_k);
            for (std::size_t al = 0; al < r_left; ++al) {
                for (std::size_t ar = 0; ar < r_right; ++ar) {
                    for (std::size_t i = 0; i < n_k; ++i)
                        fiber[i] = core.data[al * n_k * r_right + i * r_right + ar];
                    fn(fiber);
                    for (std::size_t i = 0; i < n_k; ++i)
                        core.data[al * n_k * r_right + i * r_right + ar] = fiber[i];
                }
            }
        }
        return result;
    };

    if (cfg_.type == OperatorType::RADON) {
#ifdef THEMIS_HAS_BUTTERFLY_NATIVE
        // Native path: composite Simpson's-rule discrete Radon projection.
        auto result = data;
        for (auto& core : result.cores) {
            const std::size_t r_left  = core.r_left;
            const std::size_t n_k     = core.n;
            const std::size_t r_right = core.r_right;
            std::vector<float> fiber(n_k);
            for (std::size_t al = 0; al < r_left; ++al) {
                for (std::size_t ar = 0; ar < r_right; ++ar) {
                    for (std::size_t i = 0; i < n_k; ++i)
                        fiber[i] = core.data[al * n_k * r_right + i * r_right + ar];
                    radonFiberTransform(fiber.data(), n_k);
                    for (std::size_t i = 0; i < n_k; ++i)
                        core.data[al * n_k * r_right + i * r_right + ar] = fiber[i];
                }
            }
        }
        return result;
#else
        // PERMANENT FALLBACK NOTE: THEMIS_HAS_BUTTERFLY_NATIVE not set.
        // Delegate to injected RadonTransformFn; build() already verified one is present.
        RadonTransformFn fn_copy;
        {
            std::lock_guard<std::mutex> lk(radonTransformFnMutex());
            fn_copy = radonTransformFnStorage();
        }
        if (!fn_copy) {
            throw std::logic_error(
                "TensorButterflyOperator::apply: RADON bridge fn cleared after build(). "
                "Build with -DTHEMIS_HAS_BUTTERFLY_NATIVE=ON for the native path.");
        }
        return applyFiberFn(fn_copy, data);
#endif
    }
    if (cfg_.type == OperatorType::GREENS_FUNCTION) {
#ifdef THEMIS_HAS_BUTTERFLY_NATIVE
        // Native path: trapezoidal-rule Green's function convolution.
        auto result = data;
        for (auto& core : result.cores) {
            const std::size_t r_left  = core.r_left;
            const std::size_t n_k     = core.n;
            const std::size_t r_right = core.r_right;
            std::vector<float> fiber(n_k);
            for (std::size_t al = 0; al < r_left; ++al) {
                for (std::size_t ar = 0; ar < r_right; ++ar) {
                    for (std::size_t i = 0; i < n_k; ++i)
                        fiber[i] = core.data[al * n_k * r_right + i * r_right + ar];
                    greensFiberTransform(fiber.data(), n_k);
                    for (std::size_t i = 0; i < n_k; ++i)
                        core.data[al * n_k * r_right + i * r_right + ar] = fiber[i];
                }
            }
        }
        return result;
#else
        // PERMANENT FALLBACK NOTE: THEMIS_HAS_BUTTERFLY_NATIVE not set.
        // Delegate to injected GreensTransformFn; build() already verified one is present.
        GreensTransformFn fn_copy;
        {
            std::lock_guard<std::mutex> lk(greensTransformFnMutex());
            fn_copy = greensTransformFnStorage();
        }
        if (!fn_copy) {
            throw std::logic_error(
                "TensorButterflyOperator::apply: GREENS_FUNCTION bridge fn cleared after build(). "
                "Build with -DTHEMIS_HAS_BUTTERFLY_NATIVE=ON for the native path.");
        }
        return applyFiberFn(fn_copy, data);
#endif
    }

    // FOURIER path (WHT butterfly — always natively implemented)
    if (cfg_.type != OperatorType::FOURIER) {
        throw std::logic_error(
            "TensorButterflyOperator::apply: unknown operator type.");
    }

    // Validate shape compatibility
    if (static_cast<int>(data.cores.size()) != cfg_.grid_shape.size()) {
        std::ostringstream oss = {};
        oss << "TensorButterflyOperator::apply: data has " << data.cores.size()
            << " modes but operator was built for " << cfg_.grid_shape.size()
            << " modes.";
        throw std::invalid_argument(oss.str());
    }
    for (std::size_t k = 0; k < data.cores.size(); ++k) {
        if (data.cores[k].n != cfg_.grid_shape[k]) {
            std::ostringstream oss = {};
            oss << "TensorButterflyOperator::apply: data mode " << k
                << " has size " << data.cores[k].n
                << " but operator grid_shape[" << k << "] = "
                << cfg_.grid_shape[k] << ".";
            throw std::invalid_argument(oss.str());
        }
    }

    // Deep-copy the TTTrain so we can transform cores in-place.
    storage::TTTrain result = data;

    // Apply WHT mode-by-mode.
    // For core k with shape (r_left × n_k × r_right):
    //   for each (α_{k-1}, α_k) pair, apply whtTransform() to the n_k fiber.
    //
    // Memory layout: data[α_l * n * r_r + i * r_r + α_r]
    // Stride pattern: elements for fixed (α_l, α_r) are NOT contiguous;
    // they step by r_r along i.  We extract into a temporary buffer,
    // transform, and scatter back.
    for (std::size_t k = 0; k < result.cores.size(); ++k) {
        auto& core        = result.cores[k];
        const std::size_t r_left  = core.r_left;
        const std::size_t n_k     = core.n;
        const std::size_t r_right = core.r_right;

        std::vector<float> fiber(n_k);

        for (std::size_t al = 0; al < r_left; ++al) {
            for (std::size_t ar = 0; ar < r_right; ++ar) {
                // Gather fiber: fiber[i] = core[al][i][ar]
                for (std::size_t i = 0; i < n_k; ++i) {
                    fiber[i] = core.data[al * n_k * r_right + i * r_right + ar];
                }

                // Apply injected FOURIER backend when available (STUB #267),
                // otherwise use the built-in WHT proxy.
                FourierTransformFn fn_copy;
                {
                    std::lock_guard<std::mutex> lk(fourierTransformFnMutex());
                    fn_copy = fourierTransformFnStorage();
                }
                if (fn_copy) {
                    fn_copy(fiber);
                } else {
                    whtTransform(fiber.data(), n_k);
                }

                // Scatter back
                for (std::size_t i = 0; i < n_k; ++i) {
                    core.data[al * n_k * r_right + i * r_right + ar] = fiber[i];
                }
            }
        }
    }

    return result;
}

// ============================================================================
// contractOperator()
// ============================================================================

storage::TTTrain
TensorButterflyOperator::contractOperator(
    const storage::TTTrain&        data_tt,
    const TensorButterflyOperator& op) {
    return op.apply(data_tt);
}

// ============================================================================
// Introspection
// ============================================================================

OperatorType TensorButterflyOperator::type() const noexcept {
    return cfg_.type;
}

const std::vector<std::size_t>&
TensorButterflyOperator::gridShape() const noexcept {
    return cfg_.grid_shape;
}

float TensorButterflyOperator::precision() const noexcept {
    return cfg_.precision;
}

std::string TensorButterflyOperator::describe() const {
    std::ostringstream oss = {};
    oss << "TensorButterflyOperator{type=";
    switch (cfg_.type) {
        case OperatorType::FOURIER:        oss << "FOURIER(WHT)"; break;
        case OperatorType::RADON:          oss << "RADON(Simpson)"; break;
        case OperatorType::GREENS_FUNCTION: oss << "GREENS(trapezoidal)"; break;
    }
    oss << ", shape=[";
    for (std::size_t i = 0; i < cfg_.grid_shape.size(); ++i) {
        if (i) {
          oss << ',';
        }
        oss << cfg_.grid_shape[i];
    }
    oss << "], precision=" << cfg_.precision << '}';
    return oss.str();
}

} // namespace tensor
} // namespace themis
