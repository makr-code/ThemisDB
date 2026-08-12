/**
 * @file olap.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief OLAP Dimension for grouping
 */
struct Dimension {
    std::string name;
    std::string expression;  // Optional expression for computed dimensions
    bool include_in_grouping = true;
};

/**
 * @brief OLAP Measure (aggregation)
 */
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

/**
 * @brief OLAP Filter condition
 */
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

/**
 * @brief OLAP Sort specification
 */
struct Sort {
    std::string field;
    bool ascending = true;
    bool nulls_first = false;
};

/**
 * @brief CUBE operator result cell
 */
struct CubeCell {
    std::unordered_map<std::string, std::optional<std::string>> dimensions;
    std::unordered_map<std::string, double> measures;
    int64_t grouping_id = 0;  // Bitmask indicating which dimensions are in subtotal
};

/**
 * @brief ROLLUP operator result row
 */
struct RollupRow {
    std::vector<std::optional<std::string>> dimension_values;
    std::unordered_map<std::string, double> measures;
    int level = 0;  // 0 = detail, higher = subtotal level
};

/**
 * @brief Grouping set specification
 */
struct GroupingSet {
    std::vector<std::string> dimensions;
};

/**
 * @brief OLAP Query specification
 */
struct OLAPQuery {
    std::string collection;
    std::vector<Dimension> dimensions;
    std::vector<Measure> measures;
    std::vector<Filter> filters;
    std::vector<Sort> sorts;
    std::optional<int64_t> limit;
    std::optional<int64_t> offset;

    /// Optional tenant identifier for multi-tenant deployments.
    /// When non-empty, `DistributedAnalyticsSharding` enforces that every
    /// registered shard belongs to (or is allowed for) this tenant before
    /// dispatching the query.  Shard executors may also use this field to
    /// scope their key-prefix access at the storage layer.
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

/**
 * @brief OLAP Query Result
 */
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

/**
 * @brief OLAP Query Engine
 * 
 * Provides analytical query capabilities including:
 * - Multi-dimensional aggregations
 * - CUBE and ROLLUP operators
 * - Window functions
 * - Columnar query optimization
 * - GPU-accelerated aggregations via CUDA/ROCm (when enabled)
 * 
 * Usage:
 * @code
 *   OLAPEngine engine;
 *   
 *   OLAPQuery query;
 *   query.collection = "sales";
 *   query.dimensions.push_back({"region", "", true});
 *   query.dimensions.push_back({"product", "", true});
 *   query.measures.push_back({"total_sales", "amount", Measure::Function::Sum});
 *   query.grouping_mode = OLAPQuery::GroupingMode::Cube;
 *   
 *   auto result = engine.execute(query);
 * @endcode
 * 
 * GPU-accelerated usage:
 * @code
 *   OLAPEngine::Config config;
 *   config.enable_gpu = true;
 *   config.gpu_device_id = 0;
 *   config.gpu_memory_limit = 8ULL * 1024 * 1024 * 1024;  // 8 GB
 *   OLAPEngine engine(config);
 *   auto result = engine.execute(query);  // Offloads to GPU for large datasets
 * @endcode
 */
class OLAPEngine {
public:
    using ExportToParquetFn = std::function<bool(const OLAPResult&,
                                                 const std::string&,
                                                 const std::string&)>;
    using ExportCollectionToParquetFn = std::function<bool(std::string_view,
                                                           const std::string&,
                                                           const std::vector<Filter>&,
                                                           const std::string&)>;

    /**
     * @brief GPU acceleration configuration for the OLAP engine.
     */
    struct Config {
        /// Enable GPU-accelerated aggregations (SUM, AVG, COUNT, MIN, MAX).
        bool enable_gpu = false;
        /// GPU device index (0-based) to use when enable_gpu is true.
        int gpu_device_id = 0;
        /// Maximum GPU memory budget in bytes.
        size_t gpu_memory_limit = 4ULL * 1024 * 1024 * 1024;  // 4 GB
        /// Minimum row count per group before using the GPU path.
        size_t gpu_threshold_rows = 10'000;
        /// Maximum number of OLAP query results to keep in the LRU result cache.
        /// Set to 0 to disable caching entirely.
        size_t result_cache_max_entries = 1'000;
        /// Time-to-live for cached OLAP results in milliseconds.
        /// Entries older than this are evicted on next access.
        /// Set to 0 for no TTL-based expiry (cache entries live until evicted by LRU).
        int64_t result_cache_ttl_ms = 60'000;  // 60 seconds
    };

    OLAPEngine();
    explicit OLAPEngine(const Config& config);
    ~OLAPEngine();
    
    // Main query execution
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
    
    QueryPlan explain(const OLAPQuery& query);
    
    // Statistics collection for optimization
    void collectStatistics(std::string_view collection);
    
    // v1.1.0: Parquet Export for Data Lake Integration
    /**
     * @brief Export OLAP query results to Parquet file
     * 
     * @param result The OLAP query result to export
     * @param path Output path for Parquet file
     * @param compression Compression codec (none, snappy, gzip, zstd)
     * @return true if successful
     */
    bool exportToParquet(
        const OLAPResult& result,
        const std::string& path,
        const std::string& compression = "snappy"
    );
    
    /**
     * @brief Export entire collection to Parquet (columnar format)
     * 
     * @param collection Collection name
     * @param path Output path for Parquet file
     * @param filters Optional filters to apply
     * @param compression Compression codec
     * @return true if successful
     */
    bool exportCollectionToParquet(
        std::string_view collection,
        const std::string& path,
        const std::vector<Filter>& filters = {},
        const std::string& compression = "snappy"
    );

    static void setExportToParquetFn(ExportToParquetFn fn);
    static void setExportCollectionToParquetFn(ExportCollectionToParquetFn fn);

private:
    // Internal helpers
    OLAPResult executeSimpleGroupBy(const OLAPQuery& query);
    OLAPResult executeCubeQuery(const OLAPQuery& query);
    OLAPResult executeRollupQuery(const OLAPQuery& query);
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

/**
 * @brief Columnar data store for OLAP optimization
 * 
 * Provides column-oriented storage for faster analytical queries.
 * Supports compression and vectorized operations.
 */
class ColumnarStore {
public:
    ColumnarStore();
    ~ColumnarStore();
    
    // Column operations
    void createColumn(std::string_view name, std::string_view type);
    void dropColumn(std::string_view name);
    bool hasColumn(std::string_view name) const;
    
    // Data operations
    void appendRows(
        const std::vector<std::unordered_map<std::string, std::variant<std::nullptr_t, bool, int64_t, double, std::string>>>& rows
    );
    
    void clear();
    size_t rowCount() const;
    
    // Aggregation (vectorized)
    double sum(std::string_view column) const;
    double avg(std::string_view column) const;
    double min(std::string_view column) const;
    double max(std::string_view column) const;
    int64_t count(std::string_view column) const;
    int64_t countDistinct(std::string_view column) const;
    
    // Filtered aggregation
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
    
    ColumnStats getColumnStats(std::string_view column) const;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Materialized view for pre-computed aggregations
 */
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
    std::chrono::system_clock::time_point lastRefreshTime() const;
    int64_t rowCount() const;
    bool isStale() const;
    
private:
    Definition definition_;
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace analytics
} // namespace themis
