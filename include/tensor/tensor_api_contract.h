/*
 * ThemisDB | File: tensor_api_contract.h | Version: 1.0.0
 * Author: Copilot | Maturity: 🟢 PRODUCTION-READY | Status: Phase 1 — Frozen Contract
 * Purpose: Frozen tensor runtime contract semantics for the active v1.x major line.
 */

/**
 * @file tensor_api_contract.h
 * @brief Frozen tensor runtime contract semantics for the active v1.x line.
 *
 * This header defines the normative contract for the tensor module covering
 * shape immutability, CPU/GPU device dispatch consistency, batch dimension
 * conventions, memory ownership rules (owning vs non-owning views), and the
 * canonical error taxonomy.
 *
 * ## Contract Scope
 *
 * The contracts below are binding for all components in the ThemisDB tensor
 * pipeline:
 *   - Tensor creation and reshape operations
 *   - CPU / GPU device dispatch (consistent numerical results)
 *   - Batch processing (NCHW layout, batch-first convention)
 *   - Memory ownership (owning tensor vs non-owning slice/view)
 *   - Dtype operations (compatible casts, incompatible cast detection)
 *
 * ## Versioning
 *
 * This contract is stable within v1.x.  Breaking changes require a v2.0 bump
 * with migration notes and a CHANGELOG entry.
 *
 * @see src/tensor/ROADMAP.md  — Phase 1 contract item
 * @see include/tensor/ROADMAP.md — Phase 6 documentation item
 */

#pragma once

#include <cstddef>
#include <cstdint>

namespace themis {
namespace tensor {

// ============================================================================
// § 1  Shape contract
//
// Immutability after creation:
//   A tensor's shape is fixed at construction time.  In-place reshape is
//   prohibited; reshape() returns a NEW tensor with a copied (or re-referenced)
//   data buffer.  The original tensor is unmodified.
//
// Element count invariant:
//   A reshape operation is valid only when
//   product(new_shape) == product(original_shape).
//   Attempting a reshape that changes the element count raises SHAPE_MISMATCH.
//
// Empty tensors:
//   A tensor with any dimension equal to 0 is a valid empty tensor.  Operations
//   on empty tensors return empty tensors; they do not raise errors.
// ============================================================================

/// Maximum number of dimensions (rank) supported by a tensor.
inline constexpr std::size_t kMaxTensorRank = 8;

/// Maximum number of elements in a single tensor (guards against OOM).
inline constexpr std::size_t kMaxTensorElements = 1'000'000'000u; // 1 Giga-elements

// ============================================================================
// § 2  Device dispatch contract
//
// Numerical consistency:
//   CPU and GPU implementations of the same operation MUST produce results that
//   agree to within a relative tolerance of 1e-6 for float32 and 1e-10 for
//   float64.  Results that diverge beyond this tolerance indicate a contract
//   violation in the GPU kernel.
//
// GPU unavailability fallback:
//   When the requested GPU device is unavailable (not present, driver error, or
//   memory exhausted), the dispatch layer MUST fall back to the CPU
//   implementation transparently.  DEVICE_UNAVAILABLE is returned only when
//   both GPU and CPU paths are unavailable simultaneously.
//
// Device selection latency:
//   The device selection decision (CPU vs GPU, which GPU) must complete in
//   O(1) with p99 ≤ 10 µs excluding any hardware initialisation that occurs
//   at process start.
// ============================================================================

/// Relative tolerance for CPU vs GPU numerical equivalence (float32).
inline constexpr double kDeviceDispatchToleranceF32 = 1e-6;

/// Relative tolerance for CPU vs GPU numerical equivalence (float64).
inline constexpr double kDeviceDispatchToleranceF64 = 1e-10;

// ============================================================================
// § 3  Batch contract
//
// Batch dimension:
//   The batch dimension is ALWAYS dimension 0 (N in NCHW notation).  No
//   operation may reinterpret the batch dimension as a spatial or channel
//   dimension.
//
// NCHW layout for image tensors:
//   Image tensors use NCHW layout: [batch, channels, height, width].
//   NHWC layout is supported as an explicit opt-in via a layout tag but is
//   never the default.
// ============================================================================

// ============================================================================
// § 4  Memory ownership contract
//
// Owning tensor:
//   A tensor created via Tensor::create() or Tensor::zeros() owns its data
//   buffer.  The buffer is freed when the tensor is destroyed.
//
// Non-owning slice / view:
//   A slice or view created via Tensor::slice() / Tensor::view() does NOT own
//   the underlying data.  Its lifetime is bounded by the parent tensor.
//   Accessing a view after the parent tensor has been destroyed is undefined
//   behaviour (checked in debug builds).
//
// No implicit copy on slice:
//   Slice and view operations MUST NOT copy the underlying data.  The entire
//   purpose of a view is zero-copy access into the parent buffer.
// ============================================================================

// ============================================================================
// § 5  Dtype contract
//
// Compatible operations:
//   Binary operations (add, mul, matmul, …) on two tensors of the same dtype
//   always produce a result of the same dtype.
//
// Incompatible operations:
//   Combining tensors of incompatible dtypes (e.g., float32 + int8) raises
//   DTYPE_INCOMPATIBLE.  Implicit dtype promotion is PROHIBITED.
//
// Explicit mixed precision:
//   Mixed-precision operations (e.g., float16 accumulation into float32) are
//   permitted only when the caller explicitly specifies the output dtype via
//   the accumulation_dtype parameter.
// ============================================================================

// ============================================================================
// § 6  Error taxonomy
// ============================================================================

/**
 * @brief Canonical tensor error codes.
 *
 * Codes in range [500, 599] are tensor-specific.
 */
enum class TensorErrorCode : int {
    /// Reshape or binary-op shape mismatch (element counts differ or dims incompatible).
    SHAPE_MISMATCH                = 500,

    /// Requested device (GPU) is unavailable and no CPU fallback is possible.
    DEVICE_UNAVAILABLE            = 501,

    /// Tensor allocation failed due to device or host memory exhaustion.
    OUT_OF_MEMORY                 = 502,

    /// Binary or cast operation between two incompatible dtypes.
    DTYPE_INCOMPATIBLE            = 503,

    /// Operation is not supported for the given tensor rank, dtype, or layout.
    UNSUPPORTED_OPERATION         = 504,

    /// A non-owning view was accessed after its parent tensor was freed.
    VIEW_LIFETIME_VIOLATION       = 505,

    /// Tensor rank exceeds kMaxTensorRank.
    RANK_EXCEEDED                 = 506,

    /// Unclassified tensor internal error.
    INTERNAL_ERROR                = 599,
};

/**
 * @brief Returns true when the error code is a non-retryable hard error.
 */
[[nodiscard]] inline constexpr bool isHardTensorError(TensorErrorCode code) noexcept {
    return code == TensorErrorCode::SHAPE_MISMATCH
        || code == TensorErrorCode::DTYPE_INCOMPATIBLE
        || code == TensorErrorCode::VIEW_LIFETIME_VIOLATION
        || code == TensorErrorCode::RANK_EXCEEDED
        || code == TensorErrorCode::INTERNAL_ERROR;
}

/**
 * @brief Returns true when the error code is a resource-exhaustion condition
 *        that may be retried after releasing resources.
 */
[[nodiscard]] inline constexpr bool isResourceError(TensorErrorCode code) noexcept {
    return code == TensorErrorCode::DEVICE_UNAVAILABLE
        || code == TensorErrorCode::OUT_OF_MEMORY;
}

// ============================================================================
// § 7  Supported dtypes
// ============================================================================

/**
 * @brief Enumeration of supported tensor data types.
 */
enum class TensorDtype : int {
    Float16 = 0,
    Float32 = 1,
    Float64 = 2,
    Int8    = 3,
    Int16   = 4,
    Int32   = 5,
    Int64   = 6,
    Bool    = 7,
};

/**
 * @brief Returns the byte size of one element for the given dtype.
 */
[[nodiscard]] inline constexpr std::size_t dtypeByteSize(TensorDtype dtype) noexcept {
    switch (dtype) {
        case TensorDtype::Bool:    return 1;
        case TensorDtype::Int8:    return 1;
        case TensorDtype::Float16: return 2;
        case TensorDtype::Int16:   return 2;
        case TensorDtype::Float32: return 4;
        case TensorDtype::Int32:   return 4;
        case TensorDtype::Float64: return 8;
        case TensorDtype::Int64:   return 8;
    }
    return 0; // unreachable
}

/**
 * @brief Returns true when two dtypes are compatible for binary arithmetic.
 *
 * Compatibility requires identical dtype; mixed-precision requires explicit
 * accumulation_dtype parameter (out-of-scope for this predicate).
 */
[[nodiscard]] inline constexpr bool areDtypesCompatible(
        TensorDtype a, TensorDtype b) noexcept {
    return a == b;
}

} // namespace tensor
} // namespace themis
