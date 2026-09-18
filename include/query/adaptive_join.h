/**
 * @file adaptive_join.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstddef>
#include <functional>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {

// ============================================================================
// Join Algorithm Enumeration
// ============================================================================

enum class JoinAlgorithm {
    HASH_JOIN,         ///< Build hash table on smaller side; default for large equi-joins
    MERGE_JOIN,        ///< Sorted inputs, O(n+m) merge; chosen when both sides are sorted
    NESTED_LOOP_JOIN,  ///< Quadratic; chosen when left side < 1,000 rows
    INDEX_NESTED_LOOP, ///< Right side has index, left side < 10,000 rows
    BROADCAST_JOIN,    ///< Distributed: broadcast small table to all nodes
    SHUFFLE_JOIN,      ///< Distributed: repartition both sides on join key
    GRACE_HASH_JOIN    ///< Partitioned hash join (out-of-core); when memory budget exceeded
};

/**
 * @brief Join Algorithm Name.
 * @param[in] algo Input parameter.
 * @return Pointer to the result.
 * @note Exception safety: noexcept.
 */
const char* joinAlgorithmName(JoinAlgorithm algo) noexcept;

// ============================================================================
// Supporting Data Structures
// ============================================================================

using RowValue = std::unordered_map<std::string, std::string>;

struct Table {
    std::string name;
    std::vector<RowValue> rows;

    bool is_sorted = false;

    bool has_index = false;

    [[nodiscard]] size_t rowCount() const noexcept { return rows.size(); }
};

struct JoinSpec {
    std::string left_key;

    std::string right_key;

    std::function<bool(const RowValue&, const RowValue&)> filter;
};

struct RuntimeStats {
    size_t memory_budget_bytes = 256ULL * 1024ULL * 1024ULL;  // 256 MiB default

    size_t bytes_per_row = 256;

    bool is_distributed = false;

    double grace_hash_threshold = 0.9;
};

struct JoinResult {
    std::vector<RowValue> rows;

    JoinAlgorithm algorithm_used{JoinAlgorithm::HASH_JOIN};

    double estimated_cost{0.0};

    [[nodiscard]] size_t rowCount() const noexcept { return rows.size(); }
};

// ============================================================================
// Cost Model
// ============================================================================

double estimateJoinCost(JoinAlgorithm algo,
                        size_t left_rows,
                        size_t right_rows,
                        bool left_sorted = false,
                        bool right_sorted = false) noexcept;

// ============================================================================
// AdaptiveJoinConfig
// ============================================================================

struct AdaptiveJoinConfig {
    size_t nested_loop_threshold = 1'000;

    size_t index_nested_loop_threshold = 10'000;

    size_t broadcast_threshold = 10'000;
};

// ============================================================================
// AdaptiveJoinExecutor
// ============================================================================

class AdaptiveJoinExecutor {
public:
    explicit AdaptiveJoinExecutor(AdaptiveJoinConfig config = {}) noexcept;

    [[nodiscard]] JoinResult executeJoin(const JoinSpec& spec,
                                         const Table& left,
                                         const Table& right,
                                         const RuntimeStats& stats) const;

    [[nodiscard]] JoinAlgorithm selectAlgorithm(size_t left_rows,
                                                 size_t right_rows,
                                                 bool left_sorted,
                                                 bool right_sorted,
                                                 bool has_index,
                                                 const RuntimeStats& stats) const noexcept;

    [[nodiscard]] const AdaptiveJoinConfig& config() const noexcept { return config_; }

    void setConfig(AdaptiveJoinConfig cfg) noexcept { config_ = std::move(cfg); }

private:
    /**
     * @brief ---- Individual algorithm implementations ----
     * @param[in] spec Input parameter.
     * @param[in] left Input parameter.
     * @param[in] right Input parameter.
     * @return Return value.
     */

    JoinResult executeHashJoin(const JoinSpec& spec,
                               const Table& left,
                               const Table& right) const;

    /**
     * @brief Execute Merge Join.
     * @param[in] spec Input parameter.
     * @param[in] left Input parameter.
     * @param[in] right Input parameter.
     * @return Return value.
     */
    JoinResult executeMergeJoin(const JoinSpec& spec,
                                const Table& left,
                                const Table& right) const;

    /**
     * @brief Execute Nested Loop Join.
     * @param[in] spec Input parameter.
     * @param[in] left Input parameter.
     * @param[in] right Input parameter.
     * @return Return value.
     */
    JoinResult executeNestedLoopJoin(const JoinSpec& spec,
                                     const Table& left,
                                     const Table& right) const;

    /**
     * @brief Execute Index Nested Loop Join.
     * @param[in] spec Input parameter.
     * @param[in] left Input parameter.
     * @param[in] right Input parameter.
     * @return Return value.
     */
    JoinResult executeIndexNestedLoopJoin(const JoinSpec& spec,
                                          const Table& left,
                                          const Table& right) const;

    /**
     * @brief Execute Grace Hash Join.
     * @param[in] spec Input parameter.
     * @param[in] left Input parameter.
     * @param[in] right Input parameter.
     * @return Return value.
     */
    JoinResult executeGraceHashJoin(const JoinSpec& spec,
                                    const Table& left,
                                    const Table& right) const;

    /**
     * @brief Execute Broadcast Join.
     * @param[in] spec Input parameter.
     * @param[in] left Input parameter.
     * @param[in] right Input parameter.
     * @return Return value.
     */
    JoinResult executeBroadcastJoin(const JoinSpec& spec,
                                    const Table& left,
                                    const Table& right) const;

    /**
     * @brief Execute Shuffle Join.
     * @param[in] spec Input parameter.
     * @param[in] left Input parameter.
     * @param[in] right Input parameter.
     * @return Return value.
     */
    JoinResult executeShuffleJoin(const JoinSpec& spec,
                                  const Table& left,
                                  const Table& right) const;

    /**
     * @brief ---- Helper ----
     * @param[in] left_row Input parameter.
     * @param[in] right_row Input parameter.
     * @return Return value.
     */

    static RowValue mergeRows(const RowValue& left_row, const RowValue& right_row);

    AdaptiveJoinConfig config_;
};

} // namespace themis
