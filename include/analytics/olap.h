/**
 * @file olap.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <optional>
#include <memory>
#include <variant>
#include <functional>
#include <cstdint>
#include <chrono>

namespace themis {
namespace analytics {

struct Dimension {
    std::string name;
    std::string expression;  // Optional expression for computed dimensions
    bool include_in_grouping = true;
};

struct Measure {
    enum class Function {
        Count,
        Sum,
        Avg,
        Min,
        Max,
        StdDev,
        Variance,
        Median,
        Percentile,
        CountDistinct,
        First,
        Last
    };
    
    std::string name;
    std::string field;
    Function function = Function::Sum;
    double percentile_value = 0.0;  // For percentile function
    
    /**
     * @brief Function Name.
     * @param[in] f Input parameter.
     * @return Return value.
     * @details Implements functionName without additional internal calls.
     */
    static std::string functionName(Function f) {
        switch (f) {
            case Function::Count: return "COUNT";
            case Function::Sum: return "SUM";
            case Function::Avg: return "AVG";
            case Function::Min: return "MIN";
            case Function::Max: return "MAX";
            case Function::StdDev: return "STDDEV";
            case Function::Variance: return "VARIANCE";
            case Function::Median: return "MEDIAN";
            case Function::Percentile: return "PERCENTILE";
            case Function::CountDistinct: return "COUNT_DISTINCT";
            case Function::First: return "FIRST";
            case Function::Last: return "LAST";
        }
        return "UNKNOWN";
    }
};

struct Filter {
    enum class Operator {
        Eq,
        Ne,
        Lt,
        Le,
        Gt,
        Ge,
        In,
        NotIn,
        Contains,
        StartsWith,
        EndsWith,
        IsNull,
        IsNotNull,
        Between
    };
    
    std::string field;
    Operator op = Operator::Eq;
    std::variant<std::nullptr_t, bool, int64_t, double, std::string, std::vector<std::string>> value;
    std::optional<std::variant<int64_t, double, std::string>> value2;  // For BETWEEN
};

struct Sort {
    std::string field;
    bool ascending = true;
    bool nulls_first = false;
};

struct CubeCell {
    std::unordered_map<std::string, std::optional<std::string>> dimensions;
    std::unordered_map<std::string, double> measures;
    int64_t grouping_id = 0;  // Bitmask indicating which dimensions are in subtotal
};

struct RollupRow {
    std::vector<std::optional<std::string>> dimension_values;
    std::unordered_map<std::string, double> measures;
    int level = 0;  // 0 = detail, higher = subtotal level
};

struct GroupingSet {
    std::vector<std::string> dimensions;
};

struct OLAPQuery {
    std::string collection;
    std::vector<Dimension> dimensions;
    std::vector<Measure> measures;
    std::vector<Filter> filters;
    std::vector<Sort> sorts;
    std::optional<int64_t> limit;
    std::optional<int64_t> offset;

    std::string tenant_id;
    
    // Advanced grouping
    enum class GroupingMode {
        Simple,      // Regular GROUP BY
        Cube,        // CUBE (all combinations)
        Rollup,      // ROLLUP (hierarchical)
        GroupingSets // Custom grouping sets
    };
    
    GroupingMode grouping_mode = GroupingMode::Simple;
    std::vector<GroupingSet> grouping_sets;  // For GroupingSets mode
    
    // Window functions
    struct WindowSpec {
        std::string name;
        std::vector<std::string> partition_by;
        std::vector<Sort> order_by;
        std::optional<int64_t> rows_preceding;
        std::optional<int64_t> rows_following;
    };
    std::vector<WindowSpec> windows;
};

struct OLAPResult {
    struct Row {
        std::unordered_map<std::string, std::variant<std::nullptr_t, bool, int64_t, double, std::string>> values;
        int64_t grouping_id = 0;  // For CUBE/ROLLUP
    };
    
    std::vector<std::string> columns;  // Column names in order
    std::vector<Row> rows;
    
    // Metadata
    int64_t total_rows = 0;
    double execution_time_ms = 0;
    bool has_more = false;  // For pagination
    
    // Aggregation summaries (for entire result set)
    std::unordered_map<std::string, double> grand_totals;
};

class OLAPEngine {
public:
    using ExportToParquetFn = std::function<bool(const OLAPResult&,
                                                 const std::string&,
                                                 const std::string&)>;
    using ExportCollectionToParquetFn = std::function<bool(std::string_view,
                                                           const std::string&,
                                                           const std::vector<Filter>&,
                                                           const std::string&)>;

    struct Config {
        bool enable_gpu = false;
        int gpu_device_id = 0;
        size_t gpu_memory_limit = 4ULL * 1024 * 1024 * 1024;  // 4 GB
        size_t gpu_threshold_rows = 10'000;
        size_t result_cache_max_entries = 1'000;
        int64_t result_cache_ttl_ms = 60'000;  // 60 seconds
    };

    OLAPEngine();
    /**
     * @brief OLAPEngine.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit OLAPEngine(const Config& config);
    ~OLAPEngine();
    
    // Main query execution
    /**
     * @brief Execute.
     * @param[in] query Input parameter.
     * @return Return value.
     */
    OLAPResult execute(const OLAPQuery& query);
    
    // Specialized operations
    std::vector<CubeCell> executeCube(
        std::string_view collection,
        const std::vector<Dimension>& dimensions,
        const std::vector<Measure>& measures,
        const std::vector<Filter>& filters = {}
    );
    
    std::vector<RollupRow> executeRollup(
        std::string_view collection,
        const std::vector<Dimension>& dimensions,
        const std::vector<Measure>& measures,
        const std::vector<Filter>& filters = {}
    );
    
    // Window function evaluation
    struct WindowResult {
        std::string function;
        std::string field;
        std::vector<double> values;
    };
    
    std::vector<WindowResult> evaluateWindowFunctions(
        const std::vector<std::unordered_map<std::string, double>>& data,
        const std::vector<Measure>& measures,
        const OLAPQuery::WindowSpec& window
    );
    
    // Query optimization hints
    struct QueryPlan {
        bool uses_index = false;
        std::string index_name;
        bool uses_columnar = false;
        bool parallel_execution = false;
        int estimated_rows = 0;
        double estimated_cost = 0;
        std::vector<std::string> optimization_notes;
    };
    
    /**
     * @brief Explain.
     * @param[in] query Input parameter.
     * @return Return value.
     */
    QueryPlan explain(const OLAPQuery& query);
    
    /**
     * @brief Collect Statistics.
     * @param[in] collection Input parameter.
     */
    void collectStatistics(std::string_view collection);
    
    // v1.1.0: Parquet Export for Data Lake Integration
    bool exportToParquet(
        const OLAPResult& result,
        const std::string& path,
        const std::string& compression = "snappy"
    );
    
    bool exportCollectionToParquet(
        std::string_view collection,
        const std::string& path,
        const std::vector<Filter>& filters = {},
        const std::string& compression = "snappy"
    );

    /**
     * @brief Set Export To Parquet Fn.
     * @param[in] fn Input parameter.
     */
    static void setExportToParquetFn(ExportToParquetFn fn);
    /**
     * @brief Set Export Collection To Parquet Fn.
     * @param[in] fn Input parameter.
     */
    static void setExportCollectionToParquetFn(ExportCollectionToParquetFn fn);

private:
    // Internal helpers
    /**
     * @brief Execute Simple Group By.
     * @param[in] query Input parameter.
     * @return Return value.
     */
    OLAPResult executeSimpleGroupBy(const OLAPQuery& query);
    /**
     * @brief Execute Cube Query.
     * @param[in] query Input parameter.
     * @return Return value.
     */
    OLAPResult executeCubeQuery(const OLAPQuery& query);
    /**
     * @brief Execute Rollup Query.
     * @param[in] query Input parameter.
     * @return Return value.
     */
    OLAPResult executeRollupQuery(const OLAPQuery& query);
    /**
     * @brief Execute Grouping Sets Query.
     * @param[in] query Input parameter.
     * @return Return value.
     */
    OLAPResult executeGroupingSetsQuery(const OLAPQuery& query);
    
    // Aggregation helpers
    double computeAggregate(
        const std::vector<double>& values,
        Measure::Function function,
        double percentile = 0.0
    );
    
    class Impl;
    std::unique_ptr<Impl> impl_;
};

class ColumnarStore {
public:
    ColumnarStore();
    ~ColumnarStore();
    
    // Column operations
    /**
     * @brief Create Column.
     * @param[in] name Input parameter.
     * @param[in] type Input parameter.
     */
    void createColumn(std::string_view name, std::string_view type);
    /**
     * @brief Drop Column.
     * @param[in] name Input parameter.
     */
    void dropColumn(std::string_view name);
    /**
     * @brief Has Column.
     * @param[in] name Input parameter.
     * @return True when the operation succeeds.
     */
    bool hasColumn(std::string_view name) const;
    
    // Data operations
    void appendRows(
        const std::vector<std::unordered_map<std::string, std::variant<std::nullptr_t, bool, int64_t, double, std::string>>>& rows
    );
    
    /**
     * @brief Clear.
     */
    void clear();
    /**
     * @brief Row Count.
     * @return Return value.
     */
    size_t rowCount() const;
    
    // Aggregation (vectorized)
    /**
     * @brief Sum.
     * @param[in] column Input parameter.
     * @return Return value.
     */
    double sum(std::string_view column) const;
    /**
     * @brief Avg.
     * @param[in] column Input parameter.
     * @return Return value.
     */
    double avg(std::string_view column) const;
    /**
     * @brief Min.
     * @param[in] column Input parameter.
     * @return Return value.
     */
    double min(std::string_view column) const;
    /**
     * @brief Max.
     * @param[in] column Input parameter.
     * @return Return value.
     */
    double max(std::string_view column) const;
    /**
     * @brief Count.
     * @param[in] column Input parameter.
     * @return Return value.
     */
    int64_t count(std::string_view column) const;
    /**
     * @brief Count Distinct.
     * @param[in] column Input parameter.
     * @return Return value.
     */
    int64_t countDistinct(std::string_view column) const;
    
    // Filtered aggregation
    /**
     * @brief Sum Where.
     * @param[in] column Input parameter.
     * @param[in] mask Input parameter.
     * @return Return value.
     */
    double sumWhere(std::string_view column, const std::vector<bool>& mask) const;
    
    // Statistics for query optimization
    struct ColumnStats {
        std::string name;
        std::string type;
        int64_t row_count = 0;
        int64_t null_count = 0;
        int64_t distinct_count = 0;
        std::optional<double> min_value;
        std::optional<double> max_value;
        double avg_value = 0;
    };
    
    /**
     * @brief Get Column Stats.
     * @param[in] column Input parameter.
     * @return Return value.
     */
    ColumnStats getColumnStats(std::string_view column) const;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

class MaterializedView {
public:
    struct Definition {
        std::string name;
        std::string source_collection;
        std::vector<Dimension> dimensions;
        std::vector<Measure> measures;
        std::vector<Filter> base_filters;
        
        // Refresh settings
        enum class RefreshMode {
            Manual,
            OnChange,
            Periodic
        };
        RefreshMode refresh_mode = RefreshMode::Manual;
        int refresh_interval_seconds = 3600;  // For Periodic mode
    };
    
    MaterializedView(const Definition& def);
    ~MaterializedView();
    
    const Definition& definition() const { return definition_; }
    
    // Refresh the view
    /**
     * @brief Refresh.
     */
    void refresh();
    void incrementalRefresh(
        const std::vector<std::unordered_map<std::string, std::variant<std::nullptr_t, bool, int64_t, double, std::string>>>& changes
    );
    
    // Query the view
    OLAPResult query(
        const std::vector<Filter>& filters = {},
        const std::vector<Sort>& sorts = {},
        std::optional<int64_t> limit = std::nullopt
    );
    
    // Metadata
    /**
     * @brief Last Refresh Time.
     * @return Return value.
     */
    std::chrono::system_clock::time_point lastRefreshTime() const;
    /**
     * @brief Row Count.
     * @return Return value.
     */
    int64_t rowCount() const;
    /**
     * @brief Is Stale.
     * @return True when the operation succeeds.
     */
    bool isStale() const;
    
private:
    Definition definition_;
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace analytics
} // namespace themis
