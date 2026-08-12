#pragma once

/**
 * @file benchmark_matrix.h
 * @brief Benchmark matrix for HNSW, DiskANN, Tensor Mid-Layer, Graph
 *        validation, and LLM/LoRA architecture paths (EPIC 2.2).
 *
 * The `BenchmarkMatrix` records measured results for every combination of
 * retrieval scenario and evaluation dimension.  It is the single source of
 * truth for cross-path comparisons used by the query planner and lifecycle
 * decision logic.
 *
 * Design principles
 * - Self-contained: no dependency on heavyweight ThemisDB modules so that the
 *   matrix can be populated from any benchmark binary.
 * - Extensible: new scenarios and dimensions can be appended without breaking
 *   existing call sites.
 * - Thread-safe reads: multiple readers may call `lookup` concurrently; writes
 *   must be externally serialised or protected by the caller.
 *
 * Acceptance criteria (Phase 1, Issue #5438):
 * - All major architecture paths modelled as distinct `BenchmarkScenario`
 *   variants.
 * - All evaluation axes from `EVALUATION_FRAMEWORK.md` represented as
 *   `BenchmarkDimension` variants.
 * - Edge cases (stale artifacts, shard mismatch, planner fallback) captured in
 *   `BenchmarkEdgeCase`.
 * - Matrix is serialisable to and from a plain `std::vector` of `BenchmarkEntry`
 *   records for artifact storage.
 */

#include <algorithm>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace themis {
namespace evaluation {

// ============================================================================
// Scenario taxonomy
// ============================================================================

/**
 * @brief Architecture paths that the benchmark matrix covers.
 *
 * Each enumerator corresponds to one column group in the matrix table
 * described in `docs/EPIC2_BENCHMARK_FRAMEWORK.md`.
 *
 * Ordering is stable; append-only for backward compatibility.
 */
enum class BenchmarkScenario : uint8_t {
    // --- ANN-only baselines --------------------------------------------------
    /// Pure HNSW-based ANN retrieval (hnswlib, no tensor or graph layer).
    HNSW_ANN_ONLY = 0,
    /// DiskANN-based ANN retrieval (disk-resident graph index).
    DISKANN_ANN_ONLY = 1,

    // --- ANN + Tensor Mid-Layer ----------------------------------------------
    /// ANN retrieval followed by tensor-compression routing.
    ANN_TENSOR = 2,
    /// ANN retrieval with dynamic tensor-update worker active (write workload).
    ANN_TENSOR_DYNAMIC_UPDATE = 3,
    /// ANN retrieval after a snapshot rebuild was triggered.
    ANN_TENSOR_SNAPSHOT_REBUILT = 4,
    /// ANN retrieval after a partial tensor refit (patch path, not full rebuild).
    ANN_TENSOR_PATCH_REFIT = 5,

    // --- ANN + Tensor + Graph ------------------------------------------------
    /// Full stack: ANN + tensor routing + graph evidence validation.
    ANN_TENSOR_GRAPH = 6,

    // --- Graph-direct paths --------------------------------------------------
    /// Direct exact-graph traversal without ANN pre-filtering.
    DIRECT_EXACT_GRAPH = 7,

    // --- Distributed / summary-first -----------------------------------------
    /// Summary-first routing across distributed shards.
    SUMMARY_FIRST_DISTRIBUTED = 8,
    /// Direct exact shard load (no summary pre-filter).
    DISTRIBUTED_EXACT_LOAD = 9,

    // --- LLM / LoRA ----------------------------------------------------------
    /// LLM inference with full-context prompt (no RAG filtering).
    LLM_FULL_PROMPT = 10,
    /// LLM inference with tensor-compressed evidence context.
    LLM_TENSOR_COMPRESSED = 11,
    /// LoRA fine-tuned LLM inference (adapter applied at runtime).
    LORA_INFERENCE = 12,

    // --- Overhead and cost scenarios -----------------------------------------
    /// Commit-overhead measurement (write amplification vs. pure read path).
    COMMIT_OVERHEAD = 13,
    /// CPU-side execution for GPU break-even validation.
    CPU_ONLY = 14,
    /// GPU-accelerated execution for break-even comparison.
    GPU_ACCELERATED = 15,

    /// Sentinel — keep last.
    _COUNT
};

/// Human-readable label for a `BenchmarkScenario`.
[[nodiscard]] constexpr std::string_view scenarioName(BenchmarkScenario s) noexcept {
    switch (s) {
        case BenchmarkScenario::HNSW_ANN_ONLY:              return "HNSW_ANN_ONLY";
        case BenchmarkScenario::DISKANN_ANN_ONLY:           return "DISKANN_ANN_ONLY";
        case BenchmarkScenario::ANN_TENSOR:                 return "ANN_TENSOR";
        case BenchmarkScenario::ANN_TENSOR_DYNAMIC_UPDATE:  return "ANN_TENSOR_DYNAMIC_UPDATE";
        case BenchmarkScenario::ANN_TENSOR_SNAPSHOT_REBUILT:return "ANN_TENSOR_SNAPSHOT_REBUILT";
        case BenchmarkScenario::ANN_TENSOR_PATCH_REFIT:     return "ANN_TENSOR_PATCH_REFIT";
        case BenchmarkScenario::ANN_TENSOR_GRAPH:           return "ANN_TENSOR_GRAPH";
        case BenchmarkScenario::DIRECT_EXACT_GRAPH:         return "DIRECT_EXACT_GRAPH";
        case BenchmarkScenario::SUMMARY_FIRST_DISTRIBUTED:  return "SUMMARY_FIRST_DISTRIBUTED";
        case BenchmarkScenario::DISTRIBUTED_EXACT_LOAD:     return "DISTRIBUTED_EXACT_LOAD";
        case BenchmarkScenario::LLM_FULL_PROMPT:            return "LLM_FULL_PROMPT";
        case BenchmarkScenario::LLM_TENSOR_COMPRESSED:      return "LLM_TENSOR_COMPRESSED";
        case BenchmarkScenario::LORA_INFERENCE:             return "LORA_INFERENCE";
        case BenchmarkScenario::COMMIT_OVERHEAD:            return "COMMIT_OVERHEAD";
        case BenchmarkScenario::CPU_ONLY:                   return "CPU_ONLY";
        case BenchmarkScenario::GPU_ACCELERATED:            return "GPU_ACCELERATED";
        default:                                             return "UNKNOWN";
    }
}

// ============================================================================
// Evaluation dimensions
// ============================================================================

/**
 * @brief Measurable quality and cost dimensions for each scenario.
 *
 * Aligned with the evaluation axes defined in `EVALUATION_FRAMEWORK.md`
 * sections 3.1–3.6.
 *
 * Ordering is stable; append-only for backward compatibility.
 */
enum class BenchmarkDimension : uint8_t {
    // Retrieval quality (section 3.1)
    RECALL_AT_K = 0,         ///< Recall@k — fraction of true positives in top-k.
    PRECISION_AT_K = 1,      ///< Precision@k.
    CANDIDATE_REDUCTION = 2, ///< Fraction of index skipped by pre-filtering.

    // Latency (section 3.2 / 3.6)
    QUERY_LATENCY_MS = 3,    ///< End-to-end query latency in milliseconds.
    REBUILD_LATENCY_MS = 4,  ///< Snapshot/index rebuild latency in milliseconds.
    COMMIT_OVERHEAD_MS = 5,  ///< Extra latency per write commit vs. pure read path.

    // Throughput
    QPS = 6,                 ///< Queries per second (steady-state).
    UPDATE_THROUGHPUT = 7,   ///< Tensor-update writes per second.

    // Memory / cost
    MEMORY_MB = 8,           ///< Resident memory footprint in megabytes.
    INDEX_BUILD_TIME_S = 9,  ///< Time to build the index from scratch (seconds).

    // Compression quality (section 3.4)
    COMPRESSION_RATIO = 10,  ///< Tensor summary size vs. raw size.
    APPROXIMATION_LOSS = 11, ///< Information loss after tensor compression [0,1].

    // LLM / LoRA quality (section 3.5)
    FAITHFULNESS_SCORE = 12, ///< Answer groundedness vs. retrieved evidence [0,1].
    HALLUCINATION_RATE = 13, ///< Fraction of unsupported claims [0,1].
    PROMPT_TOKEN_COUNT = 14, ///< Average token count sent to the LLM.

    // Distributed efficiency (section 3.6)
    SHARD_FAN_OUT = 15,      ///< Number of shards contacted per query.
    BYTES_TRANSFERRED = 16,  ///< Bytes transferred across shard boundaries per query.

    // Break-even
    GPU_SPEEDUP_FACTOR = 17, ///< GPU throughput / CPU throughput ratio.

    /// Sentinel — keep last.
    _COUNT
};

/// Human-readable label for a `BenchmarkDimension`.
[[nodiscard]] constexpr std::string_view dimensionName(BenchmarkDimension d) noexcept {
    switch (d) {
        case BenchmarkDimension::RECALL_AT_K:          return "RECALL_AT_K";
        case BenchmarkDimension::PRECISION_AT_K:       return "PRECISION_AT_K";
        case BenchmarkDimension::CANDIDATE_REDUCTION:  return "CANDIDATE_REDUCTION";
        case BenchmarkDimension::QUERY_LATENCY_MS:     return "QUERY_LATENCY_MS";
        case BenchmarkDimension::REBUILD_LATENCY_MS:   return "REBUILD_LATENCY_MS";
        case BenchmarkDimension::COMMIT_OVERHEAD_MS:   return "COMMIT_OVERHEAD_MS";
        case BenchmarkDimension::QPS:                  return "QPS";
        case BenchmarkDimension::UPDATE_THROUGHPUT:    return "UPDATE_THROUGHPUT";
        case BenchmarkDimension::MEMORY_MB:            return "MEMORY_MB";
        case BenchmarkDimension::INDEX_BUILD_TIME_S:   return "INDEX_BUILD_TIME_S";
        case BenchmarkDimension::COMPRESSION_RATIO:    return "COMPRESSION_RATIO";
        case BenchmarkDimension::APPROXIMATION_LOSS:   return "APPROXIMATION_LOSS";
        case BenchmarkDimension::FAITHFULNESS_SCORE:   return "FAITHFULNESS_SCORE";
        case BenchmarkDimension::HALLUCINATION_RATE:   return "HALLUCINATION_RATE";
        case BenchmarkDimension::PROMPT_TOKEN_COUNT:   return "PROMPT_TOKEN_COUNT";
        case BenchmarkDimension::SHARD_FAN_OUT:        return "SHARD_FAN_OUT";
        case BenchmarkDimension::BYTES_TRANSFERRED:    return "BYTES_TRANSFERRED";
        case BenchmarkDimension::GPU_SPEEDUP_FACTOR:   return "GPU_SPEEDUP_FACTOR";
        default:                                        return "UNKNOWN";
    }
}

// ============================================================================
// Edge-case flags
// ============================================================================

/**
 * @brief Flags that annotate a benchmark result with known anomaly conditions.
 *
 * Covered by Phase 3 of Issue #5438 (error handling / edge cases).
 */
enum class BenchmarkEdgeCase : uint16_t {
    NONE                       = 0x0000,
    /// Result was collected with a stale or invalidated artifact in place.
    STALE_ARTIFACT             = 0x0001,
    /// Distributed shard summary was out-of-sync at measurement time.
    SHARD_SUMMARY_MISMATCH     = 0x0002,
    /// Query planner fell back to residual-sensitive mode.
    RESIDUAL_PLANNER_FALLBACK  = 0x0004,
    /// Dataset is new and has never been measured before.
    UNMEASURED_COMBINATION     = 0x0008,
    /// Metric data was incomplete or below the minimum sample threshold.
    INSUFFICIENT_METRIC_DATA   = 0x0010,
};

inline BenchmarkEdgeCase operator|(BenchmarkEdgeCase a, BenchmarkEdgeCase b) noexcept {
    return static_cast<BenchmarkEdgeCase>(
        static_cast<uint16_t>(a) | static_cast<uint16_t>(b));
}

inline BenchmarkEdgeCase operator&(BenchmarkEdgeCase a, BenchmarkEdgeCase b) noexcept {
    return static_cast<BenchmarkEdgeCase>(
        static_cast<uint16_t>(a) & static_cast<uint16_t>(b));
}

inline bool hasEdgeCase(BenchmarkEdgeCase flags, BenchmarkEdgeCase flag) noexcept {
    return (static_cast<uint16_t>(flags) & static_cast<uint16_t>(flag)) != 0;
}

// ============================================================================
// BenchmarkResult
// ============================================================================

/**
 * @brief A single measured value for one (scenario, dimension) cell.
 *
 * @note Values are always stored as `double` to accommodate the full range of
 *       metrics (fractions, large byte counts, high-precision latencies).
 */
struct BenchmarkResult {
    double value{0.0};                           ///< Measured value.
    double stddev{0.0};                          ///< Standard deviation (0 if unavailable).
    uint32_t sample_count{0};                    ///< Number of samples aggregated.
    BenchmarkEdgeCase edge_flags{BenchmarkEdgeCase::NONE}; ///< Anomaly annotations.

    /// @returns true if this result was collected without known anomalies.
    [[nodiscard]] bool isClean() const noexcept {
        return edge_flags == BenchmarkEdgeCase::NONE && sample_count > 0;
    }

    /// @returns true if the result has at least @p min_samples clean samples.
    [[nodiscard]] bool hasSufficientData(uint32_t min_samples = 3) const noexcept {
        return sample_count >= min_samples &&
               !hasEdgeCase(edge_flags, BenchmarkEdgeCase::INSUFFICIENT_METRIC_DATA);
    }
};

// ============================================================================
// BenchmarkEntry (serialisable row)
// ============================================================================

/**
 * @brief Flat record representing one cell of the benchmark matrix.
 *
 * Suitable for storage in artifact files, CI result uploads, and comparisons
 * between runs (referencing the planner and lifecycle issue trackers).
 */
struct BenchmarkEntry {
    BenchmarkScenario scenario{};   ///< Column group.
    BenchmarkDimension dimension{}; ///< Row.
    BenchmarkResult result{};       ///< Measured value.
    std::string dataset_tag;        ///< Identifier of the dataset used (e.g., "msmarco-1M").
    std::string hardware_tag;       ///< Hardware profile (e.g., "cpu-avx2", "rtx3090").
    std::string runner_version;     ///< Version string of the benchmark binary.
};

// ============================================================================
// BenchmarkMatrix
// ============================================================================

/**
 * @brief In-memory benchmark matrix for HNSW, DiskANN, Tensor Mid-Layer,
 *        Graph validation, and LLM/LoRA flows.
 *
 * The matrix is a sparse map: `(BenchmarkScenario, BenchmarkDimension)` →
 * `BenchmarkResult`.  Missing cells are represented by `std::nullopt` from
 * `lookup()`.  This avoids polluting comparisons with default-zero values
 * when a measurement has genuinely not been taken.
 *
 * Thread safety: concurrent `lookup` calls are safe.  All mutating operations
 * (`record`, `clear`, `invalidateScenario`, `invalidateDimension`) must be
 * serialised by the caller.
 *
 * Usage example:
 * @code
 *   BenchmarkMatrix matrix;
 *   matrix.record(BenchmarkScenario::HNSW_ANN_ONLY,
 *                 BenchmarkDimension::RECALL_AT_K,
 *                 {0.95, 0.01, 100, BenchmarkEdgeCase::NONE});
 *
 *   auto r = matrix.lookup(BenchmarkScenario::HNSW_ANN_ONLY,
 *                          BenchmarkDimension::RECALL_AT_K);
 *   if (r) { ... }
 * @endcode
 */
class BenchmarkMatrix {
public:
    BenchmarkMatrix() = default;
    ~BenchmarkMatrix() = default;

    // Non-copyable; movable.
    BenchmarkMatrix(const BenchmarkMatrix&) = delete;
    BenchmarkMatrix& operator=(const BenchmarkMatrix&) = delete;
    BenchmarkMatrix(BenchmarkMatrix&&) noexcept = default;
    BenchmarkMatrix& operator=(BenchmarkMatrix&&) noexcept = default;

    // ------------------------------------------------------------------
    // Mutating operations
    // ------------------------------------------------------------------

    /**
     * @brief Store or overwrite a measurement for `(scenario, dimension)`.
     *
     * @param scenario  The architecture path being measured.
     * @param dimension The quality/cost dimension being measured.
     * @param result    The measured value with metadata.
     *
     * @throws std::invalid_argument if @p result has zero sample_count when
     *         edge_flags is NONE (would silently insert a vacuous result).
     */
    void record(BenchmarkScenario scenario,
                BenchmarkDimension dimension,
                const BenchmarkResult& result);

    /**
     * @brief Remove all measurements for a given scenario.
     *
     * Used when a scenario is re-run (e.g., after a snapshot rebuild) and
     * prior measurements are superseded.
     *
     * @param scenario The scenario whose measurements are invalidated.
     */
    void invalidateScenario(BenchmarkScenario scenario);

    /**
     * @brief Remove all measurements for a given dimension across all scenarios.
     *
     * Used when a measurement instrument is recalibrated.
     *
     * @param dimension The dimension to invalidate.
     */
    void invalidateDimension(BenchmarkDimension dimension);

    /// Remove all entries from the matrix.
    void clear() noexcept;

    // ------------------------------------------------------------------
    // Query operations
    // ------------------------------------------------------------------

    /**
     * @brief Retrieve the stored result for `(scenario, dimension)`.
     *
     * @returns The `BenchmarkResult` if measured, `std::nullopt` otherwise.
     *
     * @note An absent result is distinct from a result flagged
     *       `INSUFFICIENT_METRIC_DATA` — both should be treated as non-
     *       authoritative, but for different reasons.
     */
    [[nodiscard]] std::optional<BenchmarkResult>
    lookup(BenchmarkScenario scenario, BenchmarkDimension dimension) const noexcept;

    /**
     * @brief Return all (scenario, dimension, result) triples currently stored.
     *
     * Useful for serialisation and full-matrix comparisons.
     */
    [[nodiscard]] std::vector<BenchmarkEntry> entries(
        std::string_view dataset_tag = "",
        std::string_view hardware_tag = "",
        std::string_view runner_version = "") const;

    /**
     * @brief Return all results for a given scenario (all measured dimensions).
     *
     * @param scenario The scenario to slice on.
     * @returns A vector of `(dimension, result)` pairs.
     */
    [[nodiscard]] std::vector<std::pair<BenchmarkDimension, BenchmarkResult>>
    scenarioSlice(BenchmarkScenario scenario) const;

    /**
     * @brief Return all results for a given dimension (all measured scenarios).
     *
     * @param dimension The dimension to slice on.
     * @returns A vector of `(scenario, result)` pairs.
     */
    [[nodiscard]] std::vector<std::pair<BenchmarkScenario, BenchmarkResult>>
    dimensionSlice(BenchmarkDimension dimension) const;

    /**
     * @brief Return only the scenarios for which every required dimension has
     *        sufficient data (passes `hasSufficientData()`, default ≥ 3 samples
     *        and no `INSUFFICIENT_METRIC_DATA` flag).
     *
     * @param required_dimensions Dimensions that must be present.
     * @returns Scenarios satisfying the coverage requirement.
     */
    [[nodiscard]] std::vector<BenchmarkScenario>
    scenariosWithFullCoverage(
        const std::vector<BenchmarkDimension>& required_dimensions) const;

    /**
     * @brief Count the number of cells currently populated.
     *
     * @returns Number of (scenario, dimension) pairs with a stored result.
     */
    [[nodiscard]] std::size_t size() const noexcept;

    /**
     * @brief Check whether a specific cell has been measured.
     *
     * @returns true if `lookup(scenario, dimension)` would return a value.
     */
    [[nodiscard]] bool contains(BenchmarkScenario scenario,
                                BenchmarkDimension dimension) const noexcept;

    // ------------------------------------------------------------------
    // Comparison helpers
    // ------------------------------------------------------------------

    /**
     * @brief Compare two scenarios on a single dimension.
     *
     * @param a            First scenario.
     * @param b            Second scenario.
     * @param dimension    The dimension to compare on.
     *
     * @returns `a_value / b_value`, or `std::nullopt` if either scenario is
     *          missing the measurement or `b_value` is zero.
     */
    [[nodiscard]] std::optional<double>
    compareScenarios(BenchmarkScenario a,
                     BenchmarkScenario b,
                     BenchmarkDimension dimension) const noexcept;

    /**
     * @brief Return the scenario with the best (lowest or highest) value for
     *        a given dimension.
     *
     * @param dimension  The dimension to rank.
     * @param higher_is_better  If true, maximise; if false, minimise.
     * @returns The best scenario, or `std::nullopt` if no measurements exist.
     */
    [[nodiscard]] std::optional<BenchmarkScenario>
    bestScenario(BenchmarkDimension dimension,
                 bool higher_is_better = true) const noexcept;

private:
    /// Key for the internal sparse map.
    struct Key {
        BenchmarkScenario scenario;
        BenchmarkDimension dimension;

        bool operator==(const Key& o) const noexcept {
            return scenario == o.scenario && dimension == o.dimension;
        }
    };

    struct KeyHash {
        std::size_t operator()(const Key& k) const noexcept {
            // Combine the two small integers with a Knuth multiplier.
            std::size_t h = static_cast<std::size_t>(k.scenario);
            h ^= (static_cast<std::size_t>(k.dimension) * 2654435761ULL) + 0x9e3779b9ULL + (h << 6) + (h >> 2);
            return h;
        }
    };

    std::unordered_map<Key, BenchmarkResult, KeyHash> cells_;
};

} // namespace evaluation
} // namespace themis
