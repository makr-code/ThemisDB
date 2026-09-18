/**
 * @file streaming_join.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "analytics/columnar_execution.h"

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace themisdb {
namespace analytics {

// ============================================================================
// JoinType
// ============================================================================

enum class JoinType {
    Inner,     ///< Only matched rows are emitted.
    LeftOuter, ///< All probe rows are emitted; unmatched fields are null.
};

// ============================================================================
// IStreamingJoin — base interface
// ============================================================================

class IStreamingJoin {
public:
    /**
     * @brief IStreaming Join.
     * @return Return value.
     */
    virtual ~IStreamingJoin() = default;

    [[nodiscard]] virtual ColumnBatch probe(const ColumnBatch& probe) = 0;

    /**
     * @brief Reset the modification detection flag.
     */
    virtual void reset() = 0;

    [[nodiscard]] virtual size_t buildSideSize() const noexcept = 0;
};

// ============================================================================
// HashJoin
// ============================================================================

class HashJoin : public IStreamingJoin {
public:
    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    struct Config {
        std::vector<std::string> join_keys;

        JoinType join_type = JoinType::Inner;

        std::vector<std::string> build_select;

        std::vector<std::string> probe_select;

        size_t max_build_rows = 0;
    };

    // -----------------------------------------------------------------------
    // Types
    // -----------------------------------------------------------------------

    using CompositeKey = std::string;

    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------

    /**
     * @brief Hash Join.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit HashJoin(Config config);
    ~HashJoin() override = default;

    // Non-copyable (hash table may be large)
    HashJoin(const HashJoin&)            = delete;
    HashJoin& operator=(const HashJoin&) = delete;
    HashJoin(HashJoin&&)                 noexcept = default;

    // -----------------------------------------------------------------------
    // Build phase
    // -----------------------------------------------------------------------

    /**
     * @brief Add Build Batch.
     * @param[in] batch Input parameter.
     * @return True when the operation succeeds.
     */
    bool addBuildBatch(const ColumnBatch& batch);

    template<typename It>
    /**
     * @brief Build.
     * @param[in] begin Input parameter.
     * @param[in] end Input parameter.
     * @return True when the operation succeeds.
     * @details Calls: addBuildBatch().
     */
    bool build(It begin, It end) {
        for (auto it = begin; it != end; ++it) {
            if (!addBuildBatch(*it)) {
              return false;
            }
        }
        return true;
    }

    // -----------------------------------------------------------------------
    // Probe phase — IStreamingJoin
    // -----------------------------------------------------------------------

    ColumnBatch probe(const ColumnBatch& probe) override;
    void        reset() override;
    size_t      buildSideSize() const noexcept override { return build_row_count_; }

private:
    Config cfg_;

    std::unordered_map<CompositeKey, std::vector<size_t>> hash_table_;

    std::vector<std::shared_ptr<Column>> build_columns_;

    std::vector<std::string> build_column_names_;

    size_t build_row_count_{0};

    // Helpers
    /**
     * @brief Make Key.
     * @param[in] cols Input parameter.
     * @param[in] key_col_indices Input parameter.
     * @param[in] row Input parameter.
     * @return Return value.
     */
    CompositeKey makeKey(const std::vector<std::shared_ptr<Column>>& cols,
                         const std::vector<size_t>& key_col_indices,
                         size_t row) const;

    /**
     * @brief Get Val.
     * @param[in] col Input parameter.
     * @param[in] row Input parameter.
     * @return Return value.
     */
    ColumnValue getVal(const Column& col, size_t row) const;

    /**
     * @brief Append Null Row.
     * @param[in,out] cols Input/output parameter.
     * @param[in] names Input parameter.
     */
    void appendNullRow(std::vector<std::shared_ptr<Column>>& cols,
                       const std::vector<std::string>&       names) const;
};

// ============================================================================
// IntervalJoin
// ============================================================================

class IntervalJoin : public IStreamingJoin {
public:
    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    struct Config {
        std::vector<std::string> join_keys;

        std::string time_column;

        int64_t before_ms = 0;

        int64_t after_ms  = 0;

        JoinType join_type = JoinType::Inner;

        int64_t slack_ms = 0;

        std::vector<std::string> build_select;

        std::vector<std::string> probe_select;
    };

    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------

    /**
     * @brief Interval Join.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit IntervalJoin(Config config);
    ~IntervalJoin() override = default;

    IntervalJoin(const IntervalJoin&)            = delete;
    IntervalJoin& operator=(const IntervalJoin&) = delete;

    // -----------------------------------------------------------------------
    // Build phase
    // -----------------------------------------------------------------------

    /**
     * @brief Add Build Batch.
     * @param[in] batch Input parameter.
     */
    void addBuildBatch(const ColumnBatch& batch);

    // -----------------------------------------------------------------------
    // Probe phase — IStreamingJoin
    // -----------------------------------------------------------------------

    ColumnBatch probe(const ColumnBatch& probe) override;
    void        reset() override;
    size_t      buildSideSize() const noexcept override;

private:
    Config cfg_;

    struct BuildRow {
        int64_t    timestamp_ms;
        std::vector<ColumnValue> values;   ///< All build columns in order.
    };

    std::vector<BuildRow>        build_buffer_;
    std::vector<std::string>     build_col_names_;
    bool                         build_sorted_ = false;

    // Helpers
    /**
     * @brief Sort Build Buffer.
     */
    void sortBuildBuffer();
    /**
     * @brief Prune Build Buffer.
     * @param[in] min_keep_ms Input parameter.
     */
    void pruneBuildBuffer(int64_t min_keep_ms);

    /**
     * @brief Get Val.
     * @param[in] col Input parameter.
     * @param[in] row Input parameter.
     * @return Return value.
     */
    ColumnValue getVal(const Column& col, size_t row) const;
    /**
     * @brief Make Key.
     * @param[in] row Input parameter.
     * @param[in] key_col_indices Input parameter.
     * @return Return value.
     */
    std::string makeKey(const BuildRow& row,
                        const std::vector<size_t>& key_col_indices) const;
    /**
     * @brief Make Probe Key.
     * @param[in] probe_cols Input parameter.
     * @param[in] key_col_indices Input parameter.
     * @param[in] row Input parameter.
     * @return Return value.
     */
    std::string makeProbeKey(const std::vector<std::shared_ptr<Column>>& probe_cols,
                              const std::vector<size_t>& key_col_indices,
                              size_t row) const;
};

} // namespace analytics
} // namespace themisdb
