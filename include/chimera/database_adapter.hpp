/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            database_adapter.hpp                               ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-02-21 10:57:13                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     736                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea2e59ae9  2026-02-12  Add const-correctness to interfaces and eliminate unneces... ║
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

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <variant>
#include <chrono>
#include <functional>

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
    CONSTRAINT_VIOLATION = 12 ///< Data integrity constraint violated
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
    SHARDING                   ///< Horizontal sharding/partitioning
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

private:
    static std::map<std::string, AdapterCreator>& get_registry();
};

} // namespace chimera

#endif // CHIMERA_DATABASE_ADAPTER_HPP
