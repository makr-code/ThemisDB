/**
 * @file graph_kernel_classification.h
 * @brief Kernel classification framework for bounded graph acceleration eligibility
 *
 * Issue #5469: Define bounded graph kernels eligible for acceleration without weakening
 * Graph Truth semantics.
 *
 * This header provides compile-time kernel classification, traits for GPU eligibility,
 * and interface expectations for graph/planner integration.
 *
 * **Core Principle**: Graph Truth is exact and CPU-bearing. GPU acceleration is advisory-only.
 *
 * @author ThemisDB Copilot
 * @date 2026-06-25
 * @version 1.0
 */

#pragma once

#include <cstdint>
#include <string_view>
#include <type_traits>

namespace themis::acceleration {

/**
 * @enum KernelCategory
 * @brief Classification of graph kernels for GPU acceleration eligibility
 *
 * Categories define whether a graph operation may be GPU-accelerated and under
 * what constraints.
 */
enum class KernelCategory : std::uint8_t {
    /// **Category A**: Acceleration-Eligible (no constraints)
    /// GPU can safely accelerate with CPU fallback. Examples: ANN distance,
    /// TopK selection, vector insertion. Timeline: Phase A (Q3 2026).
    ACCELERATION_ELIGIBLE = 0,

    /// **Category B**: Conditional Acceleration (requires validation gates)
    /// GPU may accelerate if explicit input/output validation and CPU fallback
    /// are implemented. Examples: Geo distance/containment, bounded BFS.
    /// Timeline: Phase B (Q3 2026+, after 60% gap reduction).
    CONDITIONAL_ACCELERATION = 1,

    /// **Category C**: CPU-First Only (never GPU)
    /// Operations are truth-bearing and must remain deterministic on CPU.
    /// Examples: ACL enforcement, provenance chains, policy decisions.
    /// These operations should never be delegated to GPU approximations.
    CPU_FIRST_ONLY = 2,
};

/**
 * @enum KernelType
 * @brief Specific kernel type identifiers
 *
 * Used for runtime kernel classification lookups and validation gates.
 */
enum class KernelType : std::uint16_t {
    // Category A: Acceleration-Eligible
    ANN_L2_DISTANCE = 100,
    ANN_COSINE_DISTANCE = 101,
    ANN_INNER_PRODUCT = 102,
    TOPK_SELECTION = 103,
    VEC_KNN_INSERT = 104,
    TENSOR_CORE_MATMUL = 105,

    // Category B: Conditional Acceleration
    GEO_DISTANCE = 200,
    GEO_CONTAINMENT = 201,
    GRAPH_BFS = 202,
    GRAPH_DIJKSTRA = 203,

    // Category C: CPU-First Only
    ACL_ENFORCEMENT = 300,
    PROVENANCE_CHAINS = 301,
    POLICY_VALIDATION = 302,
    EXACT_MULTI_HOP = 303,
    IRREGULAR_TRAVERSAL = 304,

    // Sentinel
    UNKNOWN = 0xFFFF,
};

/**
 * @struct KernelClassificationTraits
 * @brief Compile-time traits for kernel classification
 *
 * Template specializations define category, constraints, and validation requirements
 * for each kernel type. This enables compile-time kernel safety checking.
 *
 * Example usage:
 * @code
 *     static_assert(
 *         KernelClassificationTraits<KernelType::ANN_L2_DISTANCE>::category
 *             == KernelCategory::ACCELERATION_ELIGIBLE
 *     );
 * @endcode
 */
template <KernelType KT>
struct KernelClassificationTraits {
    /// Kernel category (A/B/C classification)
    static constexpr KernelCategory category = KernelCategory::CPU_FIRST_ONLY;

    /// Human-readable kernel name
    static constexpr std::string_view name = "unknown";

    /// Requires input validation gates before GPU dispatch
    static constexpr bool requires_input_validation = false;

    /// Requires output validation gates after GPU dispatch
    static constexpr bool requires_output_validation = false;

    /// CPU fallback is available for failure recovery
    static constexpr bool has_cpu_fallback = false;

    /// GPU execution must be bounded (by time, memory, etc.)
    static constexpr bool gpu_execution_bounded = false;

    /// Maximum GPU execution time (milliseconds) if bounded
    static constexpr std::uint32_t max_gpu_time_ms = 0;

    /// Maximum frontier/output size if bounded
    static constexpr std::uint64_t max_output_size = 0;

    /// GPU result is advisory-only (never truth-bearing)
    static constexpr bool is_advisory_only = false;

    /// GPU may parallelize computation (false if GPU must preserve ordering)
    static constexpr bool gpu_may_parallelize = false;
};

// ============================================================================
// Category A: Acceleration-Eligible Specializations
// ============================================================================

/// L2 Euclidean distance kernel (Category A)
template <>
struct KernelClassificationTraits<KernelType::ANN_L2_DISTANCE> {
    static constexpr KernelCategory category = KernelCategory::ACCELERATION_ELIGIBLE;
    static constexpr std::string_view name = "ANN_L2_DISTANCE";
    static constexpr bool requires_input_validation = true;   // numVectors > 0
    static constexpr bool requires_output_validation = true;  // distance ranges valid
    static constexpr bool has_cpu_fallback = true;
    static constexpr bool gpu_execution_bounded = true;
    static constexpr std::uint32_t max_gpu_time_ms = 5000;    // 5 seconds
    static constexpr bool is_advisory_only = true;
    static constexpr bool gpu_may_parallelize = true;
};

/// Cosine distance kernel (Category A)
template <>
struct KernelClassificationTraits<KernelType::ANN_COSINE_DISTANCE> {
    static constexpr KernelCategory category = KernelCategory::ACCELERATION_ELIGIBLE;
    static constexpr std::string_view name = "ANN_COSINE_DISTANCE";
    static constexpr bool requires_input_validation = true;
    static constexpr bool requires_output_validation = true;  // [-1, 1] range
    static constexpr bool has_cpu_fallback = true;
    static constexpr bool gpu_execution_bounded = true;
    static constexpr std::uint32_t max_gpu_time_ms = 5000;
    static constexpr bool is_advisory_only = true;
    static constexpr bool gpu_may_parallelize = true;
};

/// Inner product kernel (Category A)
template <>
struct KernelClassificationTraits<KernelType::ANN_INNER_PRODUCT> {
    static constexpr KernelCategory category = KernelCategory::ACCELERATION_ELIGIBLE;
    static constexpr std::string_view name = "ANN_INNER_PRODUCT";
    static constexpr bool requires_input_validation = true;
    static constexpr bool requires_output_validation = true;
    static constexpr bool has_cpu_fallback = true;
    static constexpr bool gpu_execution_bounded = true;
    static constexpr std::uint32_t max_gpu_time_ms = 5000;
    static constexpr bool is_advisory_only = true;
    static constexpr bool gpu_may_parallelize = true;
};

/// TopK selection kernel (Category A)
template <>
struct KernelClassificationTraits<KernelType::TOPK_SELECTION> {
    static constexpr KernelCategory category = KernelCategory::ACCELERATION_ELIGIBLE;
    static constexpr std::string_view name = "TOPK_SELECTION";
    static constexpr bool requires_input_validation = true;   // k <= numVectors
    static constexpr bool requires_output_validation = true;  // exactly k results
    static constexpr bool has_cpu_fallback = true;
    static constexpr bool gpu_execution_bounded = true;
    static constexpr std::uint32_t max_gpu_time_ms = 1000;    // 1 second
    static constexpr bool is_advisory_only = true;
    static constexpr bool gpu_may_parallelize = false;  // Order must be deterministic
};

/// Vector KNN insertion pipeline (Category A)
template <>
struct KernelClassificationTraits<KernelType::VEC_KNN_INSERT> {
    static constexpr KernelCategory category = KernelCategory::ACCELERATION_ELIGIBLE;
    static constexpr std::string_view name = "VEC_KNN_INSERT";
    static constexpr bool requires_input_validation = true;   // batch_size <= 32
    static constexpr bool requires_output_validation = false;  // internal only
    static constexpr bool has_cpu_fallback = true;
    static constexpr bool gpu_execution_bounded = true;
    static constexpr std::uint32_t max_gpu_time_ms = 5000;
    static constexpr std::uint64_t max_output_size = 32;      // MAX_BATCH_SIZE
    static constexpr bool is_advisory_only = true;  // GPU result validated by CPU before index commit
    static constexpr bool gpu_may_parallelize = true;
};

/// Tensor Core matrix multiply (Category A)
template <>
struct KernelClassificationTraits<KernelType::TENSOR_CORE_MATMUL> {
    static constexpr KernelCategory category = KernelCategory::ACCELERATION_ELIGIBLE;
    static constexpr std::string_view name = "TENSOR_CORE_MATMUL";
    static constexpr bool requires_input_validation = true;   // M/K/N > 0
    static constexpr bool requires_output_validation = false;  // result shape check
    static constexpr bool has_cpu_fallback = true;            // CPU baseline
    static constexpr bool gpu_execution_bounded = true;
    static constexpr std::uint32_t max_gpu_time_ms = 5000;
    static constexpr bool is_advisory_only = true;  // Refinement
    static constexpr bool gpu_may_parallelize = true;
};

// ============================================================================
// Category B: Conditional Acceleration Specializations
// ============================================================================

/// Geographic Haversine distance (Category B)
template <>
struct KernelClassificationTraits<KernelType::GEO_DISTANCE> {
    static constexpr KernelCategory category = KernelCategory::CONDITIONAL_ACCELERATION;
    static constexpr std::string_view name = "GEO_DISTANCE";
    static constexpr bool requires_input_validation = true;   // lat/lon ranges
    static constexpr bool requires_output_validation = true;  // 0-40075 km
    static constexpr bool has_cpu_fallback = true;            // MANDATORY
    static constexpr bool gpu_execution_bounded = true;
    static constexpr std::uint32_t max_gpu_time_ms = 5000;
    static constexpr bool is_advisory_only = true;  // Candidates only
    static constexpr bool gpu_may_parallelize = true;
};

/// Geographic point-in-polygon (Category B)
template <>
struct KernelClassificationTraits<KernelType::GEO_CONTAINMENT> {
    static constexpr KernelCategory category = KernelCategory::CONDITIONAL_ACCELERATION;
    static constexpr std::string_view name = "GEO_CONTAINMENT";
    static constexpr bool requires_input_validation = true;   // polygon valid
    static constexpr bool requires_output_validation = true;  // binary 0/1
    static constexpr bool has_cpu_fallback = true;            // MANDATORY
    static constexpr bool gpu_execution_bounded = true;
    static constexpr std::uint32_t max_gpu_time_ms = 5000;
    static constexpr bool is_advisory_only = true;  // Filtering candidates
    static constexpr bool gpu_may_parallelize = true;
};

/// Graph bounded BFS (Category B)
template <>
struct KernelClassificationTraits<KernelType::GRAPH_BFS> {
    static constexpr KernelCategory category = KernelCategory::CONDITIONAL_ACCELERATION;
    static constexpr std::string_view name = "GRAPH_BFS";
    static constexpr bool requires_input_validation = true;   // k <= 3, graph valid
    static constexpr bool requires_output_validation = true;  // parity check
    static constexpr bool has_cpu_fallback = true;            // MANDATORY
    static constexpr bool gpu_execution_bounded = true;
    static constexpr std::uint32_t max_gpu_time_ms = 2000;    // 2 seconds
    static constexpr std::uint64_t max_output_size = 10000;   // Frontier cutoff
    static constexpr bool is_advisory_only = true;  // Candidate generation
    static constexpr bool gpu_may_parallelize = true;
};

/// Graph shortest path (Category B)
template <>
struct KernelClassificationTraits<KernelType::GRAPH_DIJKSTRA> {
    static constexpr KernelCategory category = KernelCategory::CONDITIONAL_ACCELERATION;
    static constexpr std::string_view name = "GRAPH_DIJKSTRA";
    static constexpr bool requires_input_validation = true;   // weights >= 0, pairs <= 1000
    static constexpr bool requires_output_validation = true;  // distance valid
    static constexpr bool has_cpu_fallback = true;            // MANDATORY
    static constexpr bool gpu_execution_bounded = true;
    static constexpr std::uint32_t max_gpu_time_ms = 5000;
    static constexpr std::uint64_t max_output_size = 1000;    // Max vertex pairs
    static constexpr bool is_advisory_only = true;  // Routing suggestions
    static constexpr bool gpu_may_parallelize = true;
};

// ============================================================================
// Category C: CPU-First Only Specializations
// ============================================================================

/// ACL enforcement (Category C)
template <>
struct KernelClassificationTraits<KernelType::ACL_ENFORCEMENT> {
    static constexpr KernelCategory category = KernelCategory::CPU_FIRST_ONLY;
    static constexpr std::string_view name = "ACL_ENFORCEMENT";
    static constexpr bool requires_input_validation = true;
    static constexpr bool requires_output_validation = false;
    static constexpr bool has_cpu_fallback = true;  // CPU-only
    static constexpr bool gpu_execution_bounded = false;
    static constexpr bool is_advisory_only = false;  // Truth-bearing
    static constexpr bool gpu_may_parallelize = false;  // CPU deterministic
};

/// Provenance chains (Category C)
template <>
struct KernelClassificationTraits<KernelType::PROVENANCE_CHAINS> {
    static constexpr KernelCategory category = KernelCategory::CPU_FIRST_ONLY;
    static constexpr std::string_view name = "PROVENANCE_CHAINS";
    static constexpr bool requires_input_validation = true;
    static constexpr bool requires_output_validation = false;
    static constexpr bool has_cpu_fallback = true;  // CPU-only
    static constexpr bool gpu_execution_bounded = false;
    static constexpr bool is_advisory_only = false;  // Truth-bearing
    static constexpr bool gpu_may_parallelize = false;  // Order critical
};

/// Policy validation (Category C)
template <>
struct KernelClassificationTraits<KernelType::POLICY_VALIDATION> {
    static constexpr KernelCategory category = KernelCategory::CPU_FIRST_ONLY;
    static constexpr std::string_view name = "POLICY_VALIDATION";
    static constexpr bool requires_input_validation = true;
    static constexpr bool requires_output_validation = false;
    static constexpr bool has_cpu_fallback = true;  // CPU-only
    static constexpr bool gpu_execution_bounded = false;
    static constexpr bool is_advisory_only = false;  // Truth-bearing
    static constexpr bool gpu_may_parallelize = false;  // Decision logic
};

/// Exact multi-hop validation (Category C)
template <>
struct KernelClassificationTraits<KernelType::EXACT_MULTI_HOP> {
    static constexpr KernelCategory category = KernelCategory::CPU_FIRST_ONLY;
    static constexpr std::string_view name = "EXACT_MULTI_HOP";
    static constexpr bool requires_input_validation = true;
    static constexpr bool requires_output_validation = false;
    static constexpr bool has_cpu_fallback = true;  // CPU-only
    static constexpr bool gpu_execution_bounded = false;
    static constexpr bool is_advisory_only = false;  // Truth-bearing
    static constexpr bool gpu_may_parallelize = false;  // Exact traversal
};

/// Irregular truth-bearing traversal (Category C)
template <>
struct KernelClassificationTraits<KernelType::IRREGULAR_TRAVERSAL> {
    static constexpr KernelCategory category = KernelCategory::CPU_FIRST_ONLY;
    static constexpr std::string_view name = "IRREGULAR_TRAVERSAL";
    static constexpr bool requires_input_validation = true;
    static constexpr bool requires_output_validation = false;
    static constexpr bool has_cpu_fallback = true;  // CPU-only
    static constexpr bool gpu_execution_bounded = false;
    static constexpr bool is_advisory_only = false;  // Truth-bearing
    static constexpr bool gpu_may_parallelize = false;  // Schema-dependent
};

// ============================================================================
// Helper utilities
// ============================================================================

/**
 * @brief Determine if GPU acceleration is allowed for a kernel type
 *
 * @param kernel_type The kernel to check
 * @return true if GPU acceleration is allowed (Category A or B), false if CPU-only
 */
inline constexpr bool canGPUAccelerate(KernelType kernel_type) {
    switch (kernel_type) {
        // Category A
        case KernelType::ANN_L2_DISTANCE:
        case KernelType::ANN_COSINE_DISTANCE:
        case KernelType::ANN_INNER_PRODUCT:
        case KernelType::TOPK_SELECTION:
        case KernelType::VEC_KNN_INSERT:
        case KernelType::TENSOR_CORE_MATMUL:
            return true;

        // Category B (conditional)
        case KernelType::GEO_DISTANCE:
        case KernelType::GEO_CONTAINMENT:
        case KernelType::GRAPH_BFS:
        case KernelType::GRAPH_DIJKSTRA:
            return true;

        // Category C (CPU-only)
        case KernelType::ACL_ENFORCEMENT:
        case KernelType::PROVENANCE_CHAINS:
        case KernelType::POLICY_VALIDATION:
        case KernelType::EXACT_MULTI_HOP:
        case KernelType::IRREGULAR_TRAVERSAL:
            return false;

        default:
            return false;
    }
}

/**
 * @brief Get the category for a kernel type
 *
 * @param kernel_type The kernel to classify
 * @return The KernelCategory (A, B, or C)
 */
inline constexpr KernelCategory getKernelCategory(KernelType kernel_type) {
    switch (kernel_type) {
        // Category A
        case KernelType::ANN_L2_DISTANCE:
        case KernelType::ANN_COSINE_DISTANCE:
        case KernelType::ANN_INNER_PRODUCT:
        case KernelType::TOPK_SELECTION:
        case KernelType::VEC_KNN_INSERT:
        case KernelType::TENSOR_CORE_MATMUL:
            return KernelCategory::ACCELERATION_ELIGIBLE;

        // Category B
        case KernelType::GEO_DISTANCE:
        case KernelType::GEO_CONTAINMENT:
        case KernelType::GRAPH_BFS:
        case KernelType::GRAPH_DIJKSTRA:
            return KernelCategory::CONDITIONAL_ACCELERATION;

        // Category C
        case KernelType::ACL_ENFORCEMENT:
        case KernelType::PROVENANCE_CHAINS:
        case KernelType::POLICY_VALIDATION:
        case KernelType::EXACT_MULTI_HOP:
        case KernelType::IRREGULAR_TRAVERSAL:
            return KernelCategory::CPU_FIRST_ONLY;

        default:
            return KernelCategory::CPU_FIRST_ONLY;
    }
}

/**
 * @brief Get the human-readable name of a kernel type
 *
 * @param kernel_type The kernel to name
 * @return String view with kernel name
 */
inline constexpr std::string_view getKernelName(KernelType kernel_type) {
    switch (kernel_type) {
        case KernelType::ANN_L2_DISTANCE:
            return "ANN_L2_DISTANCE";
        case KernelType::ANN_COSINE_DISTANCE:
            return "ANN_COSINE_DISTANCE";
        case KernelType::ANN_INNER_PRODUCT:
            return "ANN_INNER_PRODUCT";
        case KernelType::TOPK_SELECTION:
            return "TOPK_SELECTION";
        case KernelType::VEC_KNN_INSERT:
            return "VEC_KNN_INSERT";
        case KernelType::TENSOR_CORE_MATMUL:
            return "TENSOR_CORE_MATMUL";
        case KernelType::GEO_DISTANCE:
            return "GEO_DISTANCE";
        case KernelType::GEO_CONTAINMENT:
            return "GEO_CONTAINMENT";
        case KernelType::GRAPH_BFS:
            return "GRAPH_BFS";
        case KernelType::GRAPH_DIJKSTRA:
            return "GRAPH_DIJKSTRA";
        case KernelType::ACL_ENFORCEMENT:
            return "ACL_ENFORCEMENT";
        case KernelType::PROVENANCE_CHAINS:
            return "PROVENANCE_CHAINS";
        case KernelType::POLICY_VALIDATION:
            return "POLICY_VALIDATION";
        case KernelType::EXACT_MULTI_HOP:
            return "EXACT_MULTI_HOP";
        case KernelType::IRREGULAR_TRAVERSAL:
            return "IRREGULAR_TRAVERSAL";
        default:
            return "UNKNOWN";
    }
}

}  // namespace themis::acceleration
