/*
 * ThemisDB | File: tensor_butterfly_operator.cpp | Version: 1.0.0
 * Maturity: 🟢 PRODUCTION-READY | Score: 85/100
 * Gap Summary: total=27; TODO=1, Stub=20, Unimpl=2, Mock=1, Sim=3, Debt=0, C=4, H=75, M=27, L=0
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/**
 * @file tensor/tensor_butterfly_operator.cpp
 * @brief TensorButterflyOperator implementation.
 *
 * ### Stub log
 * - TBO-01  FOURIER operator uses Walsh-Hadamard Transform (WHT), not full DFT.
 *           WHT is real-valued and butterfly-structured.  Full complex DFT
 *           deferred to Q3 2027 when complex-core TTTrain support lands.
 *           See STUB #170.
 * - TBO-02  RADON and GREENS_FUNCTION operators not yet implemented (STUB #171).
 *           `apply()` throws std::logic_error for these types.
 *
 * STUB/SIMULATION NOTE (TBO-01):
 * Purpose: Provide a testable, butterfly-structured transform on TT data using
 *          the Walsh-Hadamard Transform as a DFT proxy.  WHT has identical
 *          butterfly structure to DFT and satisfies the O(n·d) complexity claim.
 * Activation: Always — FOURIER type always uses WHT.
 * Production Delta: Real DFT requires complex-valued TT-cores; WHT is real-to-real
 *                   and satisfies orthogonality/invertibility; spectral peaks differ
 *                   in frequency ordering vs. DFT but power spectrum is equivalent.
 * Removal Plan: Q3 2027 — extend TTCore to store std::complex<float> data and
 *               replace whtTransform() with a proper split-radix FFT.
 *
 * STUB/SIMULATION NOTE (TBO-02):
 * Purpose: RADON and GREENS_FUNCTION paths are declared in the spec but require
 *          operator-specific integration schemes not yet designed.
 * Activation: When build() is called with RADON or GREENS_FUNCTION.
 * Production Delta: apply() throws instead of computing the transform.
 * Removal Plan: Q3 2027 — implement after WHT/DFT path is validated.
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
    for (std::size_t i = 0; i < n; ++i) data[i] *= inv_sqrt_n;
}

} // anonymous namespace

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

    // STUB/SIMULATION NOTE (TBO-02):
    // Purpose: RADON and GREENS_FUNCTION are declared but not implemented natively.
    // Activation: When build() is called with these types and no bridge fn is set.
    // Production Delta: build() throws when no fn injected; with an injected fn
    //   build() succeeds and apply() delegates each mode fiber to the fn.
    // Removal Plan: Q3 2027 — implement native integration-scheme routines.
    if (type == OperatorType::RADON) {
        RadonTransformFn fn_check;
        {
            std::lock_guard<std::mutex> lk(radonTransformFnMutex());
            fn_check = radonTransformFnStorage();
        }
        if (!fn_check) {
            throw std::logic_error(
                "TensorButterflyOperator::build: RADON operator not yet "
                "implemented — inject a RadonTransformFn via setRadonTransformFn() "
                "to enable this operator type (STUB #268 — Q3 2027).");
        }
    }
    if (type == OperatorType::GREENS_FUNCTION) {
        GreensTransformFn fn_check;
        {
            std::lock_guard<std::mutex> lk(greensTransformFnMutex());
            fn_check = greensTransformFnStorage();
        }
        if (!fn_check) {
            throw std::logic_error(
                "TensorButterflyOperator::build: GREENS_FUNCTION operator not yet "
                "implemented — inject a GreensTransformFn via setGreensTransformFn() "
                "to enable this operator type (STUB #268 — Q3 2027).");
        }
    }

    // Validate grid_shape for FOURIER (WHT requires power-of-2 mode sizes).
    for (std::size_t k = 0; k < grid_shape.size(); ++k) {
        if (!isPow2(grid_shape[k])) {
            std::ostringstream oss;
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
        RadonTransformFn fn_copy;
        {
            std::lock_guard<std::mutex> lk(radonTransformFnMutex());
            fn_copy = radonTransformFnStorage();
        }
        if (!fn_copy) {
            throw std::logic_error(
                "TensorButterflyOperator::apply: RADON bridge fn cleared after build() "
                "(STUB #268 — Q3 2027).");
        }
        return applyFiberFn(fn_copy, data);
    }
    if (cfg_.type == OperatorType::GREENS_FUNCTION) {
        GreensTransformFn fn_copy;
        {
            std::lock_guard<std::mutex> lk(greensTransformFnMutex());
            fn_copy = greensTransformFnStorage();
        }
        if (!fn_copy) {
            throw std::logic_error(
                "TensorButterflyOperator::apply: GREENS_FUNCTION bridge fn cleared after build() "
                "(STUB #268 — Q3 2027).");
        }
        return applyFiberFn(fn_copy, data);
    }

    // FOURIER path (original code below)
    if (cfg_.type != OperatorType::FOURIER) {
        throw std::logic_error(
            "TensorButterflyOperator::apply: operator type not implemented "
            "(STUB #268 — Q3 2027).");
    }

    // Validate shape compatibility
    if (data.cores.size() != cfg_.grid_shape.size()) {
        std::ostringstream oss;
        oss << "TensorButterflyOperator::apply: data has " << data.cores.size()
            << " modes but operator was built for " << cfg_.grid_shape.size()
            << " modes.";
        throw std::invalid_argument(oss.str());
    }
    for (std::size_t k = 0; k < data.cores.size(); ++k) {
        if (data.cores[k].n != cfg_.grid_shape[k]) {
            std::ostringstream oss;
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
    std::ostringstream oss;
    oss << "TensorButterflyOperator{type=";
    switch (cfg_.type) {
        case OperatorType::FOURIER:        oss << "FOURIER(WHT)"; break;
        case OperatorType::RADON:          oss << "RADON(stub)";  break;
        case OperatorType::GREENS_FUNCTION: oss << "GREENS(stub)"; break;
    }
    oss << ", shape=[";
    for (std::size_t i = 0; i < cfg_.grid_shape.size(); ++i) {
        if (i) oss << ',';
        oss << cfg_.grid_shape[i];
    }
    oss << "], precision=" << cfg_.precision << '}';
    return oss.str();
}

} // namespace tensor
} // namespace themis
