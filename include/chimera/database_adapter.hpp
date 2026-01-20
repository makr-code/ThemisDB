// SPDX-License-Identifier: Apache-2.0 OR MIT
// Copyright (c) 2026 CHIMERA Suite Contributors
//
// Vendor-neutral database adapter interface for the CHIMERA benchmark suite.
// This interface provides a unified abstraction for multi-model database systems.

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <variant>
#include <functional>
#include <map>

namespace chimera {

// ============================================================================
// Type Definitions
// ============================================================================

/// Represents a database value of various types
using Value = std::variant<
    std::monostate,           // NULL
    bool,                     // Boolean
    int64_t,                  // Integer
    double,                   // Floating point
    std::string,              // String/Text
    std::vector<uint8_t>,     // Binary/BLOB
    std::vector<float>        // Vector (for embeddings)
>;

/// A row/document is a map of field names to values
using Document = std::map<std::string, Value>;

/// A result set is a collection of documents
using ResultSet = std::vector<Document>;

// ============================================================================
// Error Handling
// ============================================================================

enum class ErrorCode {
    OK = 0,
    CONNECTION_FAILED,
    AUTHENTICATION_FAILED,
    OPERATION_FAILED,
    NOT_FOUND,
    ALREADY_EXISTS,
    INVALID_ARGUMENT,
    TRANSACTION_CONFLICT,
    TIMEOUT,
    UNSUPPORTED_OPERATION,
    FEATURE_NOT_AVAILABLE,
    UNKNOWN_ERROR
};

struct Status {
    ErrorCode code;
    std::string message;
    
    Status() : code(ErrorCode::OK), message("") {}
    Status(ErrorCode c, std::string msg = "") : code(c), message(std::move(msg)) {}
    
    bool ok() const { return code == ErrorCode::OK; }
    explicit operator bool() const { return ok(); }
};

// ============================================================================
// Transaction Support
// ============================================================================

enum class IsolationLevel {
    READ_UNCOMMITTED,
    READ_COMMITTED,
    REPEATABLE_READ,
    SERIALIZABLE,
    SNAPSHOT_ISOLATION
};

/// Abstract transaction handle
class Transaction {
public:
    virtual ~Transaction() = default;
    
    /// Commit the transaction
    virtual Status commit() = 0;
    
    /// Rollback the transaction
    virtual Status rollback() = 0;
    
    /// Check if transaction is still active
    virtual bool is_active() const = 0;
};

// ============================================================================
// Query Options
// ============================================================================

struct QueryOptions {
    /// Maximum number of results to return
    std::optional<size_t> limit;
    
    /// Number of results to skip
    std::optional<size_t> offset;
    
    /// Fields to include in results (empty = all fields)
    std::vector<std::string> projection;
    
    /// Timeout for the query in milliseconds
    std::optional<uint32_t> timeout_ms;
    
    /// Transaction context (optional)
    Transaction* transaction = nullptr;
};

// ============================================================================
// Vector Search Parameters
// ============================================================================

struct VectorSearchParams {
    /// The query vector
    std::vector<float> query_vector;
    
    /// Number of nearest neighbors to return
    size_t k;
    
    /// Distance metric: "cosine", "euclidean", "dot_product"
    std::string metric = "cosine";
    
    /// Optional metadata filters
    std::map<std::string, Value> filters;
    
    /// Search timeout in milliseconds
    std::optional<uint32_t> timeout_ms;
};

// ============================================================================
// Connection Configuration
// ============================================================================

struct ConnectionConfig {
    /// Connection string or host
    std::string host;
    
    /// Port number
    uint16_t port;
    
    /// Database/keyspace name
    std::string database;
    
    /// Username for authentication
    std::string username;
    
    /// Password for authentication
    std::string password;
    
    /// Additional connection parameters
    std::map<std::string, std::string> parameters;
    
    /// Connection timeout in milliseconds
    uint32_t timeout_ms = 30000;
    
    /// Connection pool size
    uint32_t pool_size = 10;
    
    /// Enable TLS/SSL
    bool use_tls = false;
};

// ============================================================================
// Database Adapter Interface
// ============================================================================

/// Abstract base class for vendor-neutral database adapters
class DatabaseAdapter {
public:
    virtual ~DatabaseAdapter() = default;
    
    // ------------------------------------------------------------------------
    // Connection Management
    // ------------------------------------------------------------------------
    
    /// Connect to the database
    virtual Status connect(const ConnectionConfig& config) = 0;
    
    /// Disconnect from the database
    virtual Status disconnect() = 0;
    
    /// Check if connected
    virtual bool is_connected() const = 0;
    
    /// Ping the database to check connectivity
    virtual Status ping() = 0;
    
    // ------------------------------------------------------------------------
    // Basic CRUD Operations
    // ------------------------------------------------------------------------
    
    /// Insert a document into a collection/table
    virtual Status insert(
        const std::string& collection,
        const Document& document,
        const QueryOptions& options = {}
    ) = 0;
    
    /// Insert multiple documents
    virtual Status insert_batch(
        const std::string& collection,
        const std::vector<Document>& documents,
        const QueryOptions& options = {}
    ) = 0;
    
    /// Find documents matching a filter
    virtual Status find(
        const std::string& collection,
        const Document& filter,
        ResultSet& results,
        const QueryOptions& options = {}
    ) = 0;
    
    /// Find a single document by ID
    virtual Status find_by_id(
        const std::string& collection,
        const std::string& id,
        Document& result,
        const QueryOptions& options = {}
    ) = 0;
    
    /// Update documents matching a filter
    virtual Status update(
        const std::string& collection,
        const Document& filter,
        const Document& update,
        size_t& updated_count,
        const QueryOptions& options = {}
    ) = 0;
    
    /// Delete documents matching a filter
    virtual Status remove(
        const std::string& collection,
        const Document& filter,
        size_t& deleted_count,
        const QueryOptions& options = {}
    ) = 0;
    
    /// Count documents matching a filter
    virtual Status count(
        const std::string& collection,
        const Document& filter,
        size_t& count,
        const QueryOptions& options = {}
    ) = 0;
    
    // ------------------------------------------------------------------------
    // Vector Operations
    // ------------------------------------------------------------------------
    
    /// Check if vector operations are supported
    virtual bool supports_vector_search() const = 0;
    
    /// Perform vector similarity search
    virtual Status vector_search(
        const std::string& collection,
        const std::string& vector_field,
        const VectorSearchParams& params,
        ResultSet& results
    ) = 0;
    
    /// Create a vector index
    virtual Status create_vector_index(
        const std::string& collection,
        const std::string& field,
        size_t dimensions,
        const std::string& index_type = "hnsw",
        const std::map<std::string, std::string>& parameters = {}
    ) = 0;
    
    // ------------------------------------------------------------------------
    // Transaction Management
    // ------------------------------------------------------------------------
    
    /// Check if transactions are supported
    virtual bool supports_transactions() const = 0;
    
    /// Begin a new transaction
    virtual Status begin_transaction(
        std::unique_ptr<Transaction>& transaction,
        IsolationLevel level = IsolationLevel::READ_COMMITTED
    ) = 0;
    
    // ------------------------------------------------------------------------
    // Schema Operations
    // ------------------------------------------------------------------------
    
    /// Create a collection/table
    virtual Status create_collection(
        const std::string& collection,
        const std::map<std::string, std::string>& schema = {}
    ) = 0;
    
    /// Drop a collection/table
    virtual Status drop_collection(const std::string& collection) = 0;
    
    /// List all collections/tables
    virtual Status list_collections(std::vector<std::string>& collections) = 0;
    
    /// Check if a collection/table exists
    virtual Status collection_exists(
        const std::string& collection,
        bool& exists
    ) = 0;
    
    // ------------------------------------------------------------------------
    // Query Execution
    // ------------------------------------------------------------------------
    
    /// Execute a native query (SQL, AQL, Cypher, etc.)
    virtual Status execute_query(
        const std::string& query,
        ResultSet& results,
        const QueryOptions& options = {}
    ) = 0;
    
    /// Execute a native query with parameters
    virtual Status execute_query_params(
        const std::string& query,
        const std::map<std::string, Value>& params,
        ResultSet& results,
        const QueryOptions& options = {}
    ) = 0;
    
    // ------------------------------------------------------------------------
    // Metadata and Capabilities
    // ------------------------------------------------------------------------
    
    /// Get adapter name and version
    virtual std::string get_adapter_info() const = 0;
    
    /// Get database system version
    virtual std::string get_database_version() const = 0;
    
    /// Get supported features
    virtual std::vector<std::string> get_capabilities() const = 0;
};

// ============================================================================
// Factory Function Type
// ============================================================================

/// Factory function for creating adapter instances
using AdapterFactory = std::function<std::unique_ptr<DatabaseAdapter>()>;

} // namespace chimera
