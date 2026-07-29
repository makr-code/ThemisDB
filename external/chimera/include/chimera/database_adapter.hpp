/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            database_adapter.hpp                               ║
  Version:         0.0.37                                             ║
  Last Modified:   2026-04-06 04:06:04                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     1498                                           ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 04a46f63a9  2026-03-12  fix(chimera): address PR review comments on multi-databas... ║
    • 3bd2167e65  2026-03-12  feat(chimera): implement multi-database adapter registrat... ║
    • 16eb8c2a4c  2026-03-12  fix(chimera): address async API review comments (RAII cle... ║
    • a3a9d7e09c  2026-03-12  feat(chimera): implement AdapterConfig validation (v1.2.0) ║
    • 0701ac8f4d  2026-03-12  feat(chimera): implement async/promise-based API (IAsyncD... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file database_adapter.hpp
 * @brief CHIMERA Suite: Vendor-Neutral Database Adapter Architecture
 * 
 * @details
 * This header defines a strictly vendor-neutral, scientific interface for 
 * integrating arbitrary hybrid database systems into the CHIMERA Benchmark Suite.
 * 
 * The architecture follows IEEE software engineering standards and provides
 * system-agnostic abstractions for:
 * - Relational data operations
 * - Vector/embedding search
 * - Graph traversal and analytics
 * - Document storage
 * - Transaction management
 * - System information and metrics
 * 
 * @note All interfaces, structures, and return types are completely generic
 *       and contain no vendor-specific names, colors, or concepts.
 * 
 * @standard IEEE Std 730-2014 - Software Quality Assurance Processes
 * @standard IEEE Std 1012-2016 - System, Software, and Hardware V&V
 * @standard ISO/IEC 9126 - Software Quality Characteristics
 * 
 * @copyright MIT License
 * @version 1.0.0
 * @date 2025-01-20
 * 
 * @see docs/chimera/ARCHITECTURE_INTERFACE.md for detailed documentation
 */

#ifndef CHIMERA_DATABASE_ADAPTER_HPP
#define CHIMERA_DATABASE_ADAPTER_HPP

#include <atomic>
#include <cstdint>
#include <future>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <variant>
#include <chrono>
#include <functional>
#include <thread>

/**
 * @namespace chimera
 * @brief CHIMERA Benchmark Suite namespace
 */
namespace chimera {

/**
 * @enum ErrorCode
 * @brief Standard error codes for database operations
 * 
 * @details Error codes follow IEEE Std 1003.1 (POSIX) conventions
 *          for interoperability and standardization.
 */
enum class ErrorCode {
    SUCCESS = 0,              ///< Operation completed successfully
    NOT_IMPLEMENTED = 1,      ///< Feature not implemented by adapter
    INVALID_ARGUMENT = 2,     ///< Invalid input parameter
    NOT_FOUND = 3,            ///< Resource not found
    ALREADY_EXISTS = 4,       ///< Resource already exists
    PERMISSION_DENIED = 5,    ///< Insufficient permissions
    CONNECTION_ERROR = 6,     ///< Network or connection failure
    TIMEOUT = 7,              ///< Operation timeout
    RESOURCE_EXHAUSTED = 8,   ///< Out of resources (memory, disk, etc.)
    INTERNAL_ERROR = 9,       ///< Internal system error
    UNSUPPORTED = 10,         ///< Operation not supported
    TRANSACTION_ABORTED = 11, ///< Transaction was aborted
    CONSTRAINT_VIOLATION = 12, ///< Data integrity constraint violated
    DEADLOCK = 13             ///< Deadlock detected between concurrent transactions
};

/**
 * @struct Result
 * @brief Generic result type for operations that may fail
 * @tparam T The success value type
 * 
 * @details Follows Rust/C++ Expected pattern for error handling without exceptions
 */
template<typename T>
struct Result {
    std::optional<T> value;          ///< Result value if successful
    ErrorCode error_code;             ///< Error code if failed
    std::string error_message;        ///< Human-readable error description
    
    /**
     * @brief Check if operation was successful
     * @return true if operation succeeded
     */
    bool is_ok() const { return error_code == ErrorCode::SUCCESS; }
    
    /**
     * @brief Check if operation failed
     * @return true if operation failed
     */
    bool is_err() const { return error_code != ErrorCode::SUCCESS; }
    
    /**
     * @brief Create a successful result
     * @param val The success value
     * @return Result containing the value
     */
    static Result<T> ok(T val) {
        return Result<T>{std::move(val), ErrorCode::SUCCESS, ""};
    }
    
    /**
     * @brief Create an error result
     * @param code Error code
     * @param message Error message
     * @return Result containing the error
     */
    static Result<T> err(ErrorCode code, std::string message) {
        return Result<T>{std::nullopt, code, std::move(message)};
    }
};

/**
 * @typedef Scalar
 * @brief Generic scalar value type for database operations
 * 
 * @details Supports common database types in a type-safe manner
 */
using Scalar = std::variant<
    std::monostate,        // NULL/None
    bool,                  // Boolean
    int64_t,               // Integer
    double,                // Floating point
    std::string,           // Text/String
    std::vector<uint8_t>   // Binary/Blob
>;

/**
 * @struct Vector
 * @brief Generic vector/embedding representation
 * 
 * @details Used for vector similarity search, embeddings, and ML features
 */
struct Vector {
    std::vector<float> data;          ///< Vector components
    std::map<std::string, Scalar> metadata; ///< Optional metadata
    
    /**
     * @brief Get dimensionality of vector
     * @return Number of dimensions
     */
    size_t dimensions() const { return data.size(); }
};

/**
 * @struct Document
 * @brief Generic document representation for document stores
 * 
 * @details Represents a schema-flexible document with key-value pairs
 */
struct Document {
    std::string id;                              ///< Unique document identifier
    std::map<std::string, Scalar> fields;        ///< Document fields
    std::optional<int64_t> version;              ///< Optional document version
    std::optional<std::chrono::system_clock::time_point> timestamp; ///< Optional timestamp
};

/**
 * @struct GraphNode
 * @brief Generic graph node/vertex representation
 */
struct GraphNode {
    std::string id;                              ///< Unique node identifier
    std::string label;                           ///< Node type/label
    std::map<std::string, Scalar> properties;    ///< Node properties
};

/**
 * @struct GraphEdge
 * @brief Generic graph edge representation
 */
struct GraphEdge {
    std::string id;                              ///< Unique edge identifier
    std::string source_id;                       ///< Source node ID
    std::string target_id;                       ///< Target node ID
    std::string label;                           ///< Edge type/label
    std::map<std::string, Scalar> properties;    ///< Edge properties
    std::optional<double> weight;                ///< Optional edge weight
};

/**
 * @struct GraphPath
 * @brief Represents a path through a graph
 */
struct GraphPath {
    std::vector<GraphNode> nodes;                ///< Nodes in path
    std::vector<GraphEdge> edges;                ///< Edges in path
    double total_weight;                         ///< Total path weight
};

/**
 * @struct RelationalRow
 * @brief Generic relational database row
 */
struct RelationalRow {
    std::map<std::string, Scalar> columns;       ///< Column name to value mapping
};

/**
 * @struct RelationalTable
 * @brief Generic relational table result
 */
struct RelationalTable {
    std::vector<std::string> column_names;       ///< Column names in order
    std::vector<RelationalRow> rows;             ///< Table rows
};

/**
 * @struct TransactionOptions
 * @brief Configuration for database transactions
 */
struct TransactionOptions {
    enum class IsolationLevel {
        READ_UNCOMMITTED,    ///< Lowest isolation, highest concurrency
        READ_COMMITTED,      ///< Prevent dirty reads
        REPEATABLE_READ,     ///< Prevent dirty and non-repeatable reads
        SERIALIZABLE         ///< Highest isolation, lowest concurrency
    };
    
    IsolationLevel isolation_level = IsolationLevel::READ_COMMITTED;
    std::optional<std::chrono::milliseconds> timeout; ///< Transaction timeout
    bool read_only = false;                      ///< Read-only transaction
    bool allow_nested = false;                   ///< Allow nested transactions via savepoints
    size_t max_retries = 0;                      ///< Max automatic retries on deadlock (0 = no retry)
    std::chrono::milliseconds retry_backoff{10}; ///< Initial backoff between retries (doubles each attempt)
};

/**
 * @struct TransactionStats
 * @brief Runtime statistics for an active transaction
 */
struct TransactionStats {
    std::string transaction_id;                          ///< Transaction identifier
    std::chrono::system_clock::time_point start_time;   ///< When the transaction began
    std::chrono::milliseconds elapsed_time{0};           ///< Time elapsed since begin
    size_t operations_count = 0;                         ///< Number of operations executed
    size_t savepoint_count = 0;                          ///< Number of savepoints created
    size_t retry_count = 0;                              ///< Number of automatic retries attempted
    bool is_read_only = false;                           ///< Whether the transaction is read-only
    TransactionOptions::IsolationLevel isolation_level =
        TransactionOptions::IsolationLevel::READ_COMMITTED; ///< Isolation level in effect
};

/**
 * @struct TransactionState
 * @brief Full state snapshot of an active transaction
 */
struct TransactionState {
    std::string transaction_id;                          ///< Transaction identifier
    TransactionOptions::IsolationLevel isolation_level =
        TransactionOptions::IsolationLevel::READ_COMMITTED; ///< Isolation level
    std::chrono::system_clock::time_point start_time;   ///< When the transaction began
    std::vector<std::string> savepoints;                 ///< Active savepoint names (LIFO order)
    bool is_read_only = false;                           ///< Read-only flag
    std::optional<std::chrono::milliseconds> elapsed_time; ///< Time elapsed since begin
};

/**
 * @struct QueryStatistics
 * @brief Generic query execution statistics
 */
struct QueryStatistics {
    std::chrono::microseconds execution_time;    ///< Query execution time
    size_t rows_read;                            ///< Rows scanned
    size_t rows_returned;                        ///< Rows returned
    size_t bytes_read;                           ///< Bytes read from storage
    std::map<std::string, Scalar> additional_metrics; ///< System-specific metrics
};

/**
 * @struct BatchResult
 * @brief Result of an advanced batch operation
 *
 * @details Aggregates per-batch results, counts of successes and failures,
 *          and the total elapsed wall-clock time so callers can assess
 *          partial success without throwing exceptions.
 */
struct BatchResult {
    size_t total_processed = 0;                       ///< Total items attempted
    size_t successful      = 0;                       ///< Items that succeeded
    size_t failed          = 0;                       ///< Items that failed
    std::vector<Result<size_t>> batch_results;        ///< Per-chunk results
    std::chrono::milliseconds total_time{0};          ///< Wall-clock duration
};

/**
 * @struct BatchOptions
 * @brief Configuration for advanced batch operations
 *
 * @details Controls chunking, error semantics, and optional progress/batch
 *          callbacks so callers can observe and react to in-flight progress.
 */
struct BatchOptions {
    /// Number of items to process per internal chunk.
    size_t batch_size = 1000;

    /// When true the operation stops immediately after the first chunk error.
    bool stop_on_error = false;

    /**
     * @brief Optional progress callback invoked after each processed chunk.
     * @param processed Cumulative items processed so far (across all chunks).
     * @param total     Total items to process.
     */
    std::function<void(size_t processed, size_t total)> progress_callback;

    /**
     * @brief Optional per-chunk callback invoked after each chunk finishes.
     * @param batch_index Zero-based chunk index.
     * @param result      Result of that chunk (number of rows inserted or error).
     */
    std::function<void(size_t batch_index, const Result<size_t>&)> batch_callback;
};

/**
 * @struct SystemInfo
 * @brief Generic system information
 */
struct SystemInfo {
    std::string system_name;                     ///< System name (e.g., "PostgreSQL", "ThemisDB")
    std::string version;                         ///< Version string
    std::map<std::string, std::string> build_info; ///< Build information
    std::map<std::string, Scalar> configuration; ///< Configuration parameters
};

/**
 * @struct SystemMetrics
 * @brief Runtime performance metrics
 */
struct SystemMetrics {
    struct MemoryMetrics {
        size_t total_bytes;                      ///< Total memory
        size_t used_bytes;                       ///< Used memory
        size_t available_bytes;                  ///< Available memory
    };
    
    struct StorageMetrics {
        size_t total_bytes;                      ///< Total storage
        size_t used_bytes;                       ///< Used storage
        size_t available_bytes;                  ///< Available storage
    };
    
    struct CPUMetrics {
        double utilization_percent;              ///< CPU utilization (0-100)
        size_t thread_count;                     ///< Active thread count
    };
    
    MemoryMetrics memory;
    StorageMetrics storage;
    CPUMetrics cpu;
    std::map<std::string, Scalar> custom_metrics; ///< System-specific metrics
};

/**
 * @enum Capability
 * @brief Database capabilities that can be queried
 * 
 * @details Allows benchmarks to determine which features are supported
 */
enum class Capability {
    RELATIONAL_QUERIES,        ///< SQL/Relational query support
    VECTOR_SEARCH,             ///< Vector similarity search
    GRAPH_TRAVERSAL,           ///< Graph algorithms and traversal
    DOCUMENT_STORE,            ///< Document storage and queries
    FULL_TEXT_SEARCH,          ///< Full-text search capabilities
    TRANSACTIONS,              ///< ACID transaction support
    DISTRIBUTED_QUERIES,       ///< Distributed query execution
    GEOSPATIAL_QUERIES,        ///< Geographic/spatial queries
    TIME_SERIES,               ///< Time-series data handling
    STREAM_PROCESSING,         ///< Real-time stream processing
    BATCH_OPERATIONS,          ///< Bulk insert/update operations
    SECONDARY_INDEXES,         ///< Secondary index support
    MATERIALIZED_VIEWS,        ///< Materialized view support
    REPLICATION,               ///< Data replication
    SHARDING,                  ///< Horizontal sharding/partitioning
    ASYNC_OPERATIONS           ///< Non-blocking async/future-based operations
};

/**
 * @class IRelationalAdapter
 * @brief Interface for relational database operations
 * 
 * @details Provides SQL-like operations in a vendor-neutral manner
 */
class IRelationalAdapter {
public:
    virtual ~IRelationalAdapter() = default;
    
    /**
     * @brief Execute a query and return results
     * @param query Query string (SQL or equivalent)
     * @param params Query parameters for prepared statements
     * @return Query results or error
     */
    virtual Result<RelationalTable> execute_query(
        const std::string& query,
        const std::vector<Scalar>& params = {}
    ) = 0;
    
    /**
     * @brief Insert a row into a table
     * @param table_name Table name
     * @param row Row data
     * @return Number of rows inserted or error
     */
    virtual Result<size_t> insert_row(
        const std::string& table_name,
        const RelationalRow& row
    ) = 0;
    
    /**
     * @brief Batch insert multiple rows
     * @param table_name Table name
     * @param rows Rows to insert
     * @return Number of rows inserted or error
     */
    virtual Result<size_t> batch_insert(
        const std::string& table_name,
        const std::vector<RelationalRow>& rows
    ) = 0;

    /**
     * @brief Advanced batch insert with progress tracking and error control.
     *
     * @details Splits @p rows into chunks of @p options.batch_size, calls
     *          @c batch_insert for each chunk, and aggregates the results into
     *          a @c BatchResult.  Optional @c BatchOptions::progress_callback
     *          and @c BatchOptions::batch_callback are invoked after every
     *          chunk so callers can observe throughput or cancel early.
     *
     *          The default implementation always returns
     *          @c Result<BatchResult>::ok(...) and surfaces per-chunk failures
     *          (including connection issues reported by @c batch_insert) via
     *          @c BatchResult::batch_results and the @c successful / @c failed
     *          counters rather than as a top-level error.
     *          Subclasses may override this method for database-specific
     *          optimisations or to implement stricter error propagation.
     *
     * @param table_name Target table.
     * @param rows       Rows to insert.
     * @param options    Chunking, callback, and error-handling options.
     * @return Aggregated @c BatchResult wrapped in a successful @c Result.
     */
    virtual Result<BatchResult> batch_insert_advanced(
        const std::string& table_name,
        const std::vector<RelationalRow>& rows,
        const BatchOptions& options = {}
    ) {
        const auto start = std::chrono::steady_clock::now();
        const size_t chunk = options.batch_size > 0 ? options.batch_size : rows.size();

        BatchResult result;
        result.total_processed = rows.size();

        size_t processed = 0;
        size_t batch_idx = 0;
        std::vector<RelationalRow> slice;
        slice.reserve(chunk);
        for (size_t offset = 0; offset < rows.size(); offset += chunk, ++batch_idx) {
            const size_t end = std::min(offset + chunk, rows.size());
            slice.clear();
            slice.insert(
                slice.end(),
                rows.begin() + static_cast<std::ptrdiff_t>(offset),
                rows.begin() + static_cast<std::ptrdiff_t>(end)
            );
            auto chunk_result = batch_insert(table_name, slice);

            if (chunk_result.is_ok()) {
                size_t inserted = chunk_result.value.value_or(0);
                if (inserted > slice.size()) {
                    inserted = slice.size();
                }
                result.successful += inserted;
                result.failed += (slice.size() - inserted);
            } else {
                result.failed += slice.size();
            }

            result.batch_results.push_back(chunk_result);

            if (options.batch_callback) {
                options.batch_callback(batch_idx, chunk_result);
            }

            processed += slice.size();
            if (options.progress_callback) {
                options.progress_callback(processed, rows.size());
            }

            if (!chunk_result.is_ok() && options.stop_on_error) {
                break;
            }
        }

        result.total_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start);
        return Result<BatchResult>::ok(std::move(result));
    }

    /**
     * @brief Get query execution statistics
     * @return Query statistics or error
     */
    virtual Result<QueryStatistics> get_query_statistics() const = 0;
};

/**
 * @class IVectorAdapter
 * @brief Interface for vector similarity search
 * 
 * @details Supports embedding-based similarity search for ML/AI workloads
 */
class IVectorAdapter {
public:
    virtual ~IVectorAdapter() = default;
    
    /**
     * @brief Insert a vector into the index
     * @param collection Collection/index name
     * @param vector Vector to insert
     * @return Inserted vector ID or error
     */
    virtual Result<std::string> insert_vector(
        const std::string& collection,
        const Vector& vector
    ) = 0;
    
    /**
     * @brief Batch insert vectors
     * @param collection Collection/index name
     * @param vectors Vectors to insert
     * @return Number of vectors inserted or error
     */
    virtual Result<size_t> batch_insert_vectors(
        const std::string& collection,
        const std::vector<Vector>& vectors
    ) = 0;
    
    /**
     * @brief Search for similar vectors
     * @param collection Collection/index name
     * @param query_vector Query vector
     * @param k Number of nearest neighbors
     * @param filters Optional metadata filters
     * @return Similar vectors with distances or error
     */
    virtual Result<std::vector<std::pair<Vector, double>>> search_vectors(
        const std::string& collection,
        const Vector& query_vector,
        size_t k,
        const std::map<std::string, Scalar>& filters = {}
    ) = 0;
    
    /**
     * @brief Create a vector index
     * @param collection Collection name
     * @param dimensions Vector dimensions
     * @param index_params Index-specific parameters
     * @return Success or error
     */
    virtual Result<bool> create_index(
        const std::string& collection,
        size_t dimensions,
        const std::map<std::string, Scalar>& index_params = {}
    ) = 0;
};

/**
 * @class IGraphAdapter
 * @brief Interface for graph database operations
 * 
 * @details Provides graph traversal, pattern matching, and analytics
 */
class IGraphAdapter {
public:
    virtual ~IGraphAdapter() = default;
    
    /**
     * @brief Insert a node into the graph
     * @param node Node to insert
     * @return Inserted node ID or error
     */
    virtual Result<std::string> insert_node(const GraphNode& node) = 0;
    
    /**
     * @brief Insert an edge into the graph
     * @param edge Edge to insert
     * @return Inserted edge ID or error
     */
    virtual Result<std::string> insert_edge(const GraphEdge& edge) = 0;
    
    /**
     * @brief Find shortest path between two nodes
     * @param source_id Source node ID
     * @param target_id Target node ID
     * @param max_depth Maximum path depth
     * @return Shortest path or error
     */
    virtual Result<GraphPath> shortest_path(
        const std::string& source_id,
        const std::string& target_id,
        size_t max_depth = 10
    ) = 0;
    
    /**
     * @brief Traverse graph from a starting node
     * @param start_id Starting node ID
     * @param max_depth Maximum traversal depth
     * @param edge_labels Optional edge label filters
     * @return Traversed nodes or error
     */
    virtual Result<std::vector<GraphNode>> traverse(
        const std::string& start_id,
        size_t max_depth,
        const std::vector<std::string>& edge_labels = {}
    ) = 0;
    
    /**
     * @brief Execute a graph query
     * @param query Query string (Cypher, Gremlin, or equivalent)
     * @param params Query parameters
     * @return Query results or error
     */
    virtual Result<std::vector<GraphPath>> execute_graph_query(
        const std::string& query,
        const std::map<std::string, Scalar>& params = {}
    ) = 0;
};

/**
 * @class IDocumentAdapter
 * @brief Interface for document database operations
 * 
 * @details Provides schema-flexible document storage and querying
 */
class IDocumentAdapter {
public:
    virtual ~IDocumentAdapter() = default;
    
    /**
     * @brief Insert a document
     * @param collection Collection name
     * @param doc Document to insert
     * @return Inserted document ID or error
     */
    virtual Result<std::string> insert_document(
        const std::string& collection,
        const Document& doc
    ) = 0;
    
    /**
     * @brief Batch insert documents
     * @param collection Collection name
     * @param docs Documents to insert
     * @return Number of documents inserted or error
     */
    virtual Result<size_t> batch_insert_documents(
        const std::string& collection,
        const std::vector<Document>& docs
    ) = 0;

    /**
     * @brief Advanced batch insert for documents with progress tracking.
     *
     * @details Splits @p docs into chunks of @p options.batch_size, calls
     *          @c batch_insert_documents for each chunk, and aggregates the
     *          results.  Optional callbacks are invoked after each chunk.
     *
     *          The default implementation always returns
     *          @c Result<BatchResult>::ok(...) and surfaces per-chunk failures
     *          via @c BatchResult::batch_results and the @c successful /
     *          @c failed counters rather than as a top-level error.
     *          Subclasses may override for database-specific optimisations.
     *
     * @param collection Target collection.
     * @param docs       Documents to insert.
     * @param options    Chunking, callback, and error-handling options.
     * @return Aggregated @c BatchResult wrapped in a successful @c Result.
     */
    virtual Result<BatchResult> batch_insert_documents_advanced(
        const std::string& collection,
        const std::vector<Document>& docs,
        const BatchOptions& options = {}
    ) {
        const auto start = std::chrono::steady_clock::now();
        const size_t chunk = options.batch_size > 0 ? options.batch_size : docs.size();

        BatchResult result;
        result.total_processed = docs.size();

        size_t processed = 0;
        size_t batch_idx = 0;
        std::vector<Document> slice;
        slice.reserve(chunk);
        for (size_t offset = 0; offset < docs.size(); offset += chunk, ++batch_idx) {
            const size_t end = std::min(offset + chunk, docs.size());
            slice.clear();
            slice.insert(
                slice.end(),
                docs.begin() + static_cast<std::ptrdiff_t>(offset),
                docs.begin() + static_cast<std::ptrdiff_t>(end)
            );
            auto chunk_result = batch_insert_documents(collection, slice);

            if (chunk_result.is_ok()) {
                size_t inserted = chunk_result.value.value_or(0);
                if (inserted > slice.size()) {
                    inserted = slice.size();
                }
                result.successful += inserted;
                result.failed += (slice.size() - inserted);
            } else {
                result.failed += slice.size();
            }

            result.batch_results.push_back(chunk_result);

            if (options.batch_callback) {
                options.batch_callback(batch_idx, chunk_result);
            }

            processed += slice.size();
            if (options.progress_callback) {
                options.progress_callback(processed, docs.size());
            }

            if (!chunk_result.is_ok() && options.stop_on_error) {
                break;
            }
        }

        result.total_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start);
        return Result<BatchResult>::ok(std::move(result));
    }
    
    /**
     * @brief Find documents matching criteria
     * @param collection Collection name
     * @param filter Filter criteria
     * @param limit Maximum results
     * @return Matching documents or error
     */
    virtual Result<std::vector<Document>> find_documents(
        const std::string& collection,
        const std::map<std::string, Scalar>& filter,
        size_t limit = 100
    ) = 0;
    
    /**
     * @brief Update documents matching criteria
     * @param collection Collection name
     * @param filter Filter criteria
     * @param updates Field updates
     * @return Number of documents updated or error
     */
    virtual Result<size_t> update_documents(
        const std::string& collection,
        const std::map<std::string, Scalar>& filter,
        const std::map<std::string, Scalar>& updates
    ) = 0;
};

/**
 * @class ITransactionAdapter
 * @brief Interface for transaction management
 * 
 * @details Provides ACID transaction support
 */
class ITransactionAdapter {
public:
    virtual ~ITransactionAdapter() = default;
    
    /**
     * @brief Begin a new transaction
     * @param options Transaction options
     * @return Transaction ID or error
     */
    virtual Result<std::string> begin_transaction(
        const TransactionOptions& options = {}
    ) = 0;
    
    /**
     * @brief Commit a transaction
     * @param transaction_id Transaction ID
     * @return Success or error
     */
    virtual Result<bool> commit_transaction(const std::string& transaction_id) = 0;
    
    /**
     * @brief Rollback a transaction
     * @param transaction_id Transaction ID
     * @return Success or error
     */
    virtual Result<bool> rollback_transaction(const std::string& transaction_id) = 0;

    /**
     * @brief Create a savepoint within an active transaction
     * @param transaction_id Transaction ID
     * @param savepoint_name Unique name for the savepoint within the transaction
     * @return Savepoint name on success, or NOT_IMPLEMENTED if not supported
     */
    virtual Result<std::string> create_savepoint(
        const std::string& /*transaction_id*/,
        const std::string& /*savepoint_name*/
    ) {
        return Result<std::string>::err(
            ErrorCode::NOT_IMPLEMENTED,
            "Savepoints not supported by this adapter"
        );
    }

    /**
     * @brief Rollback the transaction to a previously created savepoint
     * @param transaction_id Transaction ID
     * @param savepoint_name Name of the savepoint to roll back to
     * @return Success or NOT_IMPLEMENTED if not supported
     */
    virtual Result<bool> rollback_to_savepoint(
        const std::string& /*transaction_id*/,
        const std::string& /*savepoint_name*/
    ) {
        return Result<bool>::err(
            ErrorCode::NOT_IMPLEMENTED,
            "Savepoints not supported by this adapter"
        );
    }

    /**
     * @brief Release (destroy) a savepoint without rolling back
     * @param transaction_id Transaction ID
     * @param savepoint_name Name of the savepoint to release
     * @return Success or NOT_IMPLEMENTED if not supported
     */
    virtual Result<bool> release_savepoint(
        const std::string& /*transaction_id*/,
        const std::string& /*savepoint_name*/
    ) {
        return Result<bool>::err(
            ErrorCode::NOT_IMPLEMENTED,
            "Savepoints not supported by this adapter"
        );
    }

    /**
     * @brief Retrieve runtime statistics for an active transaction
     * @param transaction_id Transaction ID
     * @return Transaction statistics or NOT_IMPLEMENTED if not supported
     */
    virtual Result<TransactionStats> get_transaction_stats(
        const std::string& /*transaction_id*/
    ) {
        return Result<TransactionStats>::err(
            ErrorCode::NOT_IMPLEMENTED,
            "Transaction statistics not supported by this adapter"
        );
    }

    /**
     * @brief Retrieve the full state of an active transaction
     * @param transaction_id Transaction ID
     * @return Transaction state or NOT_IMPLEMENTED if not supported
     */
    virtual Result<TransactionState> get_transaction_state(
        const std::string& /*transaction_id*/
    ) {
        return Result<TransactionState>::err(
            ErrorCode::NOT_IMPLEMENTED,
            "Transaction state not supported by this adapter"
        );
    }

    /**
     * @brief Execute an operation with automatic retry on deadlock
     *
     * @details Calls @p operation repeatedly until it succeeds, returns a
     *          non-deadlock error, or @p max_retries attempts have been
     *          exhausted.  Each retry is preceded by an exponential backoff
     *          starting at @p initial_backoff_ms milliseconds.
     *
     * @tparam T Result value type
     * @tparam F Callable type returning Result<T>
     * @param operation  Callable returning Result<T>
     * @param max_retries Maximum number of additional attempts after the first
     * @param initial_backoff_ms Initial sleep duration before the first retry
     * @return The first successful result, the last deadlock result after all
     *         retries are exhausted, or any non-deadlock error immediately
     */
    template<typename T, typename F>
    Result<T> execute_with_retry(
        F&& operation,
        size_t max_retries = 3,
        std::chrono::milliseconds initial_backoff_ms = std::chrono::milliseconds{10}
    ) {
        static constexpr size_t kMaxBackoffShift = 5; ///< Cap backoff at 32x initial value
        Result<T> last = operation();
        for (size_t attempt = 0; attempt < max_retries && last.is_err() &&
                                  last.error_code == ErrorCode::DEADLOCK; ++attempt) {
            const size_t shift = attempt < kMaxBackoffShift ? attempt : kMaxBackoffShift;
            const auto factor = static_cast<std::chrono::milliseconds::rep>(1u << shift);
            const auto max_ms = std::chrono::milliseconds::max();
            const std::chrono::milliseconds::rep base = initial_backoff_ms.count();
            std::chrono::milliseconds safe_duration;
            if (base > 0 && factor > 0 && base > max_ms.count() / factor) {
                safe_duration = max_ms;
            } else {
                safe_duration = initial_backoff_ms * factor;
            }
            std::this_thread::sleep_for(safe_duration);
            last = operation();
        }
        return last;
    }
};

/**
 * @class ISystemInfoAdapter
 * @brief Interface for system information and metrics
 * 
 * @details Provides system metadata and runtime metrics
 */
class ISystemInfoAdapter {
public:
    virtual ~ISystemInfoAdapter() = default;
    
    /**
     * @brief Get system information
     * @return System info or error
     */
    virtual Result<SystemInfo> get_system_info() const = 0;
    
    /**
     * @brief Get runtime metrics
     * @return System metrics or error
     */
    virtual Result<SystemMetrics> get_metrics() const = 0;
    
    /**
     * @brief Check if a capability is supported
     * @param cap Capability to check
     * @return true if supported, false otherwise
     */
    virtual bool has_capability(Capability cap) const = 0;
    
    /**
     * @brief Get all supported capabilities
     * @return List of supported capabilities
     */
    virtual std::vector<Capability> get_capabilities() const = 0;
};

/**
 * @struct AsyncQueryOptions
 * @brief Configuration options for asynchronous database operations
 *
 * @details Controls per-operation identity, timeout hints, and scheduling
 *          priority hints for operations launched through IAsyncDatabaseAdapter.
 *
 * **Note on timeout and priority:**
 * These fields are *scheduling hints* intended for production adapter
 * implementations backed by real network drivers (e.g. HTTP connection pools
 * with per-request deadline propagation).  The in-process simulation adapter
 * (`ThemisDBAdapter`) does not enforce them — `timeout` is stored but not
 * applied to the `std::async` worker, and `priority` is reserved for future
 * use.  Callers that rely on hard deadlines should enforce them externally
 * (e.g. with `std::future::wait_for`).
 */
struct AsyncQueryOptions {
    /**
     * @brief Maximum wall-clock time allowed for the operation.
     *
     * `std::nullopt` means no limit.  Production drivers that support request
     * timeouts will propagate this value to the underlying network call.
     * The simulation adapter ignores this field.
     */
    std::optional<std::chrono::milliseconds> timeout;

    /// Human-readable tag used to identify the operation for cancellation and logging.
    std::string operation_id;

    /**
     * @brief Scheduling priority hint (higher = higher priority).
     *
     * Interpretation is implementation-defined.  The simulation adapter
     * does not alter thread scheduling based on this value.
     */
    int priority = 0;
};

/**
 * @class IAsyncDatabaseAdapter
 * @brief Non-blocking async/promise-based database operations
 *
 * @details
 * Extends the synchronous IDatabaseAdapter interfaces with future-based
 * counterparts so callers can dispatch multiple operations concurrently and
 * collect results later.  All methods return a `std::future<Result<T>>` that
 * is ready as soon as the underlying operation completes.
 *
 * An optional `AsyncQueryOptions` parameter controls per-operation timeout
 * and identity (used for cancellation via `cancel_async()`).
 *
 * Implementations that do not support async execution should return
 * `Result<T>::err(ErrorCode::NOT_IMPLEMENTED, ...)` wrapped in a resolved
 * future.
 *
 * **Usage example:**
 * @code
 * ThemisDBAdapter adapter;
 * adapter.connect("themisdb://localhost:7777");
 *
 * // Launch multiple queries concurrently
 * std::vector<std::future<Result<RelationalTable>>> futures;
 * for (const auto& query : queries) {
 *     futures.push_back(adapter.execute_query_async(query));
 * }
 *
 * // Collect results
 * for (auto& f : futures) {
 *     auto result = f.get();
 *     if (result.is_ok()) { ... }
 * }
 * @endcode
 */
class IAsyncDatabaseAdapter {
public:
    virtual ~IAsyncDatabaseAdapter() = default;

    /**
     * @brief Asynchronously execute a relational query.
     *
     * @param query  Query string (SQL or equivalent).
     * @param params Optional bound parameters for prepared-style execution.
     * @param opts   Optional per-operation configuration.
     * @return Future that resolves to the query result table or an error.
     */
    virtual std::future<Result<RelationalTable>> execute_query_async(
        const std::string& query,
        const std::vector<Scalar>& params = {},
        const AsyncQueryOptions& opts = {}
    ) = 0;

    /**
     * @brief Asynchronously batch-insert rows into a table.
     *
     * @param table_name       Target table name.
     * @param rows             Rows to insert.
     * @param progress_callback Optional callback invoked with the cumulative
     *                          count of rows processed so far.  Must be
     *                          thread-safe; will be called from the worker thread.
     * @param opts             Optional per-operation configuration.
     * @return Future that resolves to the number of rows inserted or an error.
     */
    virtual std::future<Result<size_t>> batch_insert_async(
        const std::string& table_name,
        const std::vector<RelationalRow>& rows,
        std::function<void(size_t processed)> progress_callback = nullptr,
        const AsyncQueryOptions& opts = {}
    ) = 0;

    /**
     * @brief Asynchronously search for similar vectors.
     *
     * @param collection   Vector collection/index name.
     * @param query_vector Query embedding.
     * @param k            Number of nearest neighbours to return.
     * @param filters      Optional metadata filters applied before scoring.
     * @param opts         Optional per-operation configuration.
     * @return Future that resolves to a ranked list of (vector, distance) pairs or an error.
     */
    virtual std::future<Result<std::vector<std::pair<Vector, double>>>> search_vectors_async(
        const std::string& collection,
        const Vector& query_vector,
        size_t k,
        const std::map<std::string, Scalar>& filters = {},
        const AsyncQueryOptions& opts = {}
    ) = 0;

    /**
     * @brief Request cancellation of an in-flight async operation.
     *
     * @details Best-effort: an operation that has already completed or not
     *          yet started will return NOT_FOUND.  A successfully cancelled
     *          operation will cause its future to resolve with
     *          ErrorCode::TIMEOUT.
     *
     * @param operation_id  The `AsyncQueryOptions::operation_id` supplied when
     *                      the operation was launched.
     * @return Success if the operation was found and cancellation was requested;
     *         NOT_FOUND if no such operation is active.
     */
    virtual Result<bool> cancel_async(const std::string& operation_id) = 0;
};

/**
 * @class IDatabaseAdapter
 * @brief Complete database adapter interface
 * 
 * @details Combines all adapter interfaces into a unified interface.
 *          Implementations may return NOT_IMPLEMENTED for unsupported operations.
 */
class IDatabaseAdapter : public IRelationalAdapter,
                         public IVectorAdapter,
                         public IGraphAdapter,
                         public IDocumentAdapter,
                         public ITransactionAdapter,
                         public ISystemInfoAdapter {
public:
    virtual ~IDatabaseAdapter() = default;
    
    /**
     * @brief Connect to the database
     * @param connection_string Connection string/URI
     * @param options Connection options
     * @return Success or error
     */
    virtual Result<bool> connect(
        const std::string& connection_string,
        const std::map<std::string, std::string>& options = {}
    ) = 0;
    
    /**
     * @brief Disconnect from the database
     * @return Success or error
     */
    virtual Result<bool> disconnect() = 0;
    
    /**
     * @brief Check if connected
     * @return true if connected
     */
    virtual bool is_connected() const = 0;
};

/**
 * @class AdapterFactory
 * @brief Factory for creating database adapters
 * 
 * @details Implements the Factory Pattern for extensible adapter creation.
 *          New adapters can be registered at runtime.
 * 
 * @example
 * @code
 * // Register a custom adapter
 * AdapterFactory::register_adapter("CustomDB", 
 *     [](){ return std::make_unique<CustomDBAdapter>(); });
 * 
 * // Create adapter instance
 * auto adapter = AdapterFactory::create("CustomDB");
 * if (!adapter) {
 *     // Handle error
 * }
 * @endcode
 */
class AdapterFactory {
public:
    /**
     * @typedef AdapterCreator
     * @brief Function type for creating adapter instances
     */
    using AdapterCreator = std::function<std::unique_ptr<IDatabaseAdapter>()>;
    
    /**
     * @brief Create a database adapter
     * @param system_name System name (e.g., "PostgreSQL", "ThemisDB")
     * @return Adapter instance or nullptr if not found
     */
    static std::unique_ptr<IDatabaseAdapter> create(const std::string& system_name);
    
    /**
     * @brief Register a new adapter
     * @param system_name System name
     * @param creator Creator function
     * @return true if registered successfully, false if already exists
     */
    static bool register_adapter(const std::string& system_name, AdapterCreator creator);

    /**
     * @brief Register a new adapter together with a static capability hint list
     *
     * @details
     * The @p static_capabilities list is stored in a lightweight capability
     * hints map keyed by @p system_name.  When create_with_capabilities() is
     * called, it uses the hints to negotiate without instantiating any adapter,
     * avoiding expensive construction (and potential resource acquisition) for
     * non-qualifying candidates.
     *
     * @param system_name        System name (e.g., "PostgreSQL:16")
     * @param creator            Factory function for the adapter
     * @param static_capabilities Capabilities the adapter is known to support
     * @return true if registered successfully, false if already exists
     */
    static bool register_adapter(const std::string& system_name,
                                  AdapterCreator creator,
                                  const std::vector<Capability>& static_capabilities);
    
    /**
     * @brief Get list of supported systems
     * @return Vector of system names
     */
    static std::vector<std::string> get_supported_systems();
    
    /**
     * @brief Check if a system is supported
     * @param system_name System name
     * @return true if supported
     */
    static bool is_supported(const std::string& system_name);

    /**
     * @brief Create an adapter using a prioritised fallback list
     *
     * @details
     * Iterates through @p candidates in order and returns the first adapter
     * that can be successfully created.  This enables version-specific fallback
     * chains, e.g. try "PostgreSQL:16", then "PostgreSQL:15", then "PostgreSQL:14".
     *
     * @param candidates Ordered list of system names to try (highest priority first)
     * @return First successfully created adapter, or nullptr if none can be created
     */
    static std::unique_ptr<IDatabaseAdapter> create_with_fallback(
        const std::vector<std::string>& candidates);

    /**
     * @brief Create the first adapter in @p candidates that satisfies all
     *        @p required_capabilities (capability negotiation)
     *
     * @details
     * Iterates through @p candidates in order.  For each registered candidate an
     * instance is created, its capabilities are queried, and if every capability
     * in @p required_capabilities is supported the adapter is returned.
     * Candidates that are not registered or do not meet the capability
     * requirements are silently skipped.
     *
     * @param candidates             Ordered list of system names to try
     * @param required_capabilities  Capabilities that the chosen adapter must support
     * @return First qualifying adapter, or nullptr if no candidate qualifies
     */
    static std::unique_ptr<IDatabaseAdapter> create_with_capabilities(
        const std::vector<std::string>& candidates,
        const std::vector<Capability>& required_capabilities);

private:
    static std::map<std::string, AdapterCreator>& get_registry();
    static std::map<std::string, std::vector<Capability>>& get_capability_hints();
};

/**
 * @struct ParsedConnectionString
 * @brief Components of a parsed adapter connection string
 *
 * @details Populated by AdapterConfig::parse_connection_string().
 */
struct ParsedConnectionString {
    std::string scheme;   ///< URI scheme (e.g. "themisdb", "postgresql")
    std::string host;     ///< Hostname or IP
    std::string port;     ///< Port number as string (empty if absent)
    std::string database; ///< Database / path component (empty if absent)
    std::string username; ///< User info (empty if absent)
    // NOTE: password is intentionally omitted to avoid credential exposure
};

/**
 * @struct AdapterConfig
 * @brief Configuration container for a database adapter
 *
 * @details Aggregates a connection string and a map of key-value options.
 *          Call validate() before passing the config to any adapter's
 *          connect() method to surface problems early.
 *
 * @example
 * @code
 * AdapterConfig config;
 * config.connection_string = "themisdb://localhost:8529/db";
 * config.options["pool_size"]  = int64_t{50};
 * config.options["timeout_ms"] = int64_t{30000};
 *
 * if (!config.validate().is_ok()) {
 *     for (const auto& err : config.get_validation_errors()) {
 *         std::cerr << "Config error: " << err << '\n';
 *     }
 * }
 * @endcode
 */
struct AdapterConfig {
    /// Connection URI (e.g. "themisdb://localhost:8529/mydb")
    std::string connection_string;

    /// Adapter-specific options (type-safe via the Scalar variant)
    std::map<std::string, Scalar> options;

    // -----------------------------------------------------------------------
    // Validation
    // -----------------------------------------------------------------------

    /**
     * @brief Validate the configuration.
     *
     * Checks:
     *  - connection_string is non-empty and contains a recognised scheme
     *  - connection_string contains a non-empty host
     *  - well-known options have the expected type
     *  - well-known options are within their valid ranges
     *
     * @return Result<bool> — ok(true) if valid; err(...) on the first fatal
     *         error (call get_validation_errors() for the full list).
     */
    Result<bool> validate() const;

    /**
     * @brief Collect all validation errors.
     *
     * Unlike validate(), this method always inspects the entire configuration
     * and returns every problem found.
     *
     * @return Vector of human-readable error strings (empty if valid).
     */
    std::vector<std::string> get_validation_errors() const;

    /**
     * @brief Parse the connection string into its components.
     *
     * @return Populated ParsedConnectionString.  Fields that are absent from
     *         the URI are left as empty strings.
     */
    ParsedConnectionString parse_connection_string() const;
};

/**
 * @class AdapterCapabilityMatrix
 * @brief Cross-system adapter capability comparison matrix
 *
 * @details
 * Aggregates per-adapter capability information into a queryable matrix that
 * allows benchmark harnesses and reporting tools to determine:
 *   - Which capabilities a given adapter supports
 *   - Which adapters support a given capability
 *   - A complete cross-system comparison view
 *
 * The matrix is populated either manually via add_entry() / add_adapter(), or
 * automatically from all adapters registered in AdapterFactory via
 * build_from_factory().
 *
 * @example
 * @code
 * // Build from all registered adapters
 * auto matrix = AdapterCapabilityMatrix::build_from_factory();
 *
 * // Check a single cell
 * bool pg_has_vector = matrix.supports("PostgreSQL", Capability::VECTOR_SEARCH);
 *
 * // Find all adapters that support graph traversal
 * auto graph_adapters = matrix.adapters_supporting(Capability::GRAPH_TRAVERSAL);
 *
 * // Get the full capability list for one adapter
 * auto caps = matrix.capabilities_of("MongoDB");
 * @endcode
 */
class AdapterCapabilityMatrix {
public:
    /// One row in the matrix: maps every Capability enum value to true/false.
    using CapabilityRow = std::map<Capability, bool>;

    /// The full matrix: system name → capability row.
    using MatrixData = std::map<std::string, CapabilityRow>;

    // -----------------------------------------------------------------------
    // Population
    // -----------------------------------------------------------------------

    /**
     * @brief Add an entry using an explicit capability list.
     * @param system_name Adapter/system name (e.g. "PostgreSQL")
     * @param capabilities List of capabilities the system supports
     */
    void add_entry(const std::string& system_name,
                   const std::vector<Capability>& capabilities);

    /**
     * @brief Add an entry by querying a live adapter instance.
     * @param system_name Adapter/system name
     * @param adapter     Adapter instance whose get_capabilities() is called
     */
    void add_adapter(const std::string& system_name,
                     const ISystemInfoAdapter& adapter);

    // -----------------------------------------------------------------------
    // Factory integration
    // -----------------------------------------------------------------------

    /**
     * @brief Build the matrix from all adapters registered in AdapterFactory.
     *
     * @details Instantiates each registered adapter (no connection required)
     *          and calls get_capabilities() to populate the matrix.
     * @return Populated AdapterCapabilityMatrix
     */
    static AdapterCapabilityMatrix build_from_factory();

    // -----------------------------------------------------------------------
    // Queries
    // -----------------------------------------------------------------------

    /**
     * @brief Check whether a system supports a capability.
     * @param system_name Adapter/system name
     * @param cap         Capability to check
     * @return true if supported; false if not supported or system not found
     */
    bool supports(const std::string& system_name, Capability cap) const;

    /**
     * @brief Get all system names whose entries support the given capability.
     * @param cap Capability to query
     * @return Alphabetically sorted list of supporting system names
     */
    std::vector<std::string> adapters_supporting(Capability cap) const;

    /**
     * @brief Get the capabilities supported by a system.
     * @param system_name Adapter/system name
     * @return List of supported capabilities (empty if system not found)
     */
    std::vector<Capability> capabilities_of(const std::string& system_name) const;

    /**
     * @brief Get all system names present in the matrix.
     * @return Alphabetically sorted list of system names
     */
    std::vector<std::string> system_names() const;

    // -----------------------------------------------------------------------
    // Utilities
    // -----------------------------------------------------------------------

    /**
     * @brief Return all defined Capability enum values in declaration order.
     * @return Complete list of Capability values
     */
    static std::vector<Capability> all_capabilities();

    /**
     * @brief Convert a Capability enum value to its canonical string label.
     * @param cap Capability value
     * @return String label (e.g. "RELATIONAL_QUERIES")
     */
    static std::string capability_to_string(Capability cap);

    /**
     * @brief Direct read-only access to the underlying matrix data.
     * @return Const reference to the MatrixData map
     */
    const MatrixData& data() const { return matrix_; }

private:
    MatrixData matrix_;
};

} // namespace chimera

#endif // CHIMERA_DATABASE_ADAPTER_HPP
