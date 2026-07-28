/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            template_adapter.hpp                               ║
  Version:         0.0.37                                             ║
  Last Modified:   2026-04-06 04:03:15                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     336                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// SPDX-License-Identifier: Apache-2.0 OR MIT
// Copyright (c) 2026 CHIMERA Suite Contributors
//
// Template adapter implementation for the CHIMERA benchmark suite.
// Use this template to implement adapters for other database systems
// (e.g., Neo4j, ArangoDB, MongoDB, Cassandra, etc.)

#pragma once

#include "chimera/database_adapter.hpp"
#include <memory>
#include <mutex>

namespace chimera {
namespace adapters {

/// Transaction implementation for [YourDatabase]
/// Note: If your database doesn't support transactions, you can provide
/// a stub implementation that returns UNSUPPORTED_OPERATION
class TemplateTransaction : public Transaction {
public:
    explicit TemplateTransaction(void* native_handle);
    ~TemplateTransaction() override;
    
    Status commit() override;
    Status rollback() override;
    bool is_active() const override;
    
private:
    void* native_handle_;
    bool active_;
    mutable std::mutex mutex_;
};

/// Template adapter implementation
/// 
/// Instructions for implementing your own adapter:
/// 
/// 1. Copy this file and rename it (e.g., neo4j_adapter.hpp)
/// 2. Replace "Template" with your database name throughout
/// 3. Implement each virtual method according to your database's API
/// 4. Use the private Impl pattern to hide database-specific dependencies
/// 5. Add error handling and convert native errors to Status codes
/// 6. Document any database-specific behavior or limitations
/// 7. Write tests that exercise all implemented functionality
///
/// See themisdb_adapter.hpp, postgresql_adapter.hpp, and weaviate_adapter.hpp
/// for reference implementations.
class TemplateAdapter : public DatabaseAdapter {
public:
    TemplateAdapter();
    ~TemplateAdapter() override;
    
    // Prevent copying
    TemplateAdapter(const TemplateAdapter&) = delete;
    TemplateAdapter& operator=(const TemplateAdapter&) = delete;
    
    // ------------------------------------------------------------------------
    // Connection Management
    // ------------------------------------------------------------------------
    
    /// Connect to the database using the provided configuration
    /// @param config Connection parameters (host, port, credentials, etc.)
    /// @return Status indicating success or failure
    Status connect(const ConnectionConfig& config) override;
    
    /// Disconnect from the database and clean up resources
    /// @return Status indicating success or failure
    Status disconnect() override;
    
    /// Check if currently connected to the database
    /// @return true if connected, false otherwise
    bool is_connected() const override;
    
    /// Ping the database to verify connectivity
    /// @return Status indicating success or failure
    Status ping() override;
    
    // ------------------------------------------------------------------------
    // Basic CRUD Operations
    // ------------------------------------------------------------------------
    
    /// Insert a single document into a collection
    /// @param collection Name of the collection/table
    /// @param document Document to insert (map of field names to values)
    /// @param options Query options (limit, offset, transaction, etc.)
    /// @return Status indicating success or failure
    Status insert(
        const std::string& collection,
        const Document& document,
        const QueryOptions& options = {}
    ) override;
    
    /// Insert multiple documents in a single operation (batch insert)
    /// @param collection Name of the collection/table
    /// @param documents Vector of documents to insert
    /// @param options Query options
    /// @return Status indicating success or failure
    Status insert_batch(
        const std::string& collection,
        const std::vector<Document>& documents,
        const QueryOptions& options = {}
    ) override;
    
    /// Find documents matching a filter
    /// @param collection Name of the collection/table
    /// @param filter Filter criteria (map of field names to values)
    /// @param results Output parameter for matching documents
    /// @param options Query options (limit, offset, projection, etc.)
    /// @return Status indicating success or failure
    Status find(
        const std::string& collection,
        const Document& filter,
        ResultSet& results,
        const QueryOptions& options = {}
    ) override;
    
    /// Find a single document by its ID
    /// @param collection Name of the collection/table
    /// @param id Document identifier
    /// @param result Output parameter for the found document
    /// @param options Query options
    /// @return Status indicating success or failure (NOT_FOUND if not found)
    Status find_by_id(
        const std::string& collection,
        const std::string& id,
        Document& result,
        const QueryOptions& options = {}
    ) override;
    
    /// Update documents matching a filter
    /// @param collection Name of the collection/table
    /// @param filter Filter criteria to select documents
    /// @param update Update operations to apply
    /// @param updated_count Output parameter for number of updated documents
    /// @param options Query options
    /// @return Status indicating success or failure
    Status update(
        const std::string& collection,
        const Document& filter,
        const Document& update,
        size_t& updated_count,
        const QueryOptions& options = {}
    ) override;
    
    /// Delete documents matching a filter
    /// @param collection Name of the collection/table
    /// @param filter Filter criteria to select documents
    /// @param deleted_count Output parameter for number of deleted documents
    /// @param options Query options
    /// @return Status indicating success or failure
    Status remove(
        const std::string& collection,
        const Document& filter,
        size_t& deleted_count,
        const QueryOptions& options = {}
    ) override;
    
    /// Count documents matching a filter
    /// @param collection Name of the collection/table
    /// @param filter Filter criteria
    /// @param count Output parameter for document count
    /// @param options Query options
    /// @return Status indicating success or failure
    Status count(
        const std::string& collection,
        const Document& filter,
        size_t& count,
        const QueryOptions& options = {}
    ) override;
    
    // ------------------------------------------------------------------------
    // Vector Operations
    // ------------------------------------------------------------------------
    
    /// Check if this database supports vector similarity search
    /// @return true if vector operations are supported, false otherwise
    bool supports_vector_search() const override;
    
    /// Perform vector similarity search (k-nearest neighbors)
    /// @param collection Name of the collection/table
    /// @param vector_field Name of the field containing vectors
    /// @param params Search parameters (query vector, k, metric, filters)
    /// @param results Output parameter for search results
    /// @return Status indicating success or failure
    Status vector_search(
        const std::string& collection,
        const std::string& vector_field,
        const VectorSearchParams& params,
        ResultSet& results
    ) override;
    
    /// Create an index for efficient vector similarity search
    /// @param collection Name of the collection/table
    /// @param field Name of the vector field to index
    /// @param dimensions Number of dimensions in the vectors
    /// @param index_type Type of index ("hnsw", "ivf", "flat", etc.)
    /// @param parameters Additional index-specific parameters
    /// @return Status indicating success or failure
    Status create_vector_index(
        const std::string& collection,
        const std::string& field,
        size_t dimensions,
        const std::string& index_type = "hnsw",
        const std::map<std::string, std::string>& parameters = {}
    ) override;
    
    // ------------------------------------------------------------------------
    // Transaction Management
    // ------------------------------------------------------------------------
    
    /// Check if this database supports ACID transactions
    /// @return true if transactions are supported, false otherwise
    bool supports_transactions() const override;
    
    /// Begin a new transaction
    /// @param transaction Output parameter for transaction handle
    /// @param level Isolation level for the transaction
    /// @return Status indicating success or failure
    Status begin_transaction(
        std::unique_ptr<Transaction>& transaction,
        IsolationLevel level = IsolationLevel::READ_COMMITTED
    ) override;
    
    // ------------------------------------------------------------------------
    // Schema Operations
    // ------------------------------------------------------------------------
    
    /// Create a new collection/table
    /// @param collection Name of the collection/table to create
    /// @param schema Optional schema definition (database-specific format)
    /// @return Status indicating success or failure
    Status create_collection(
        const std::string& collection,
        const std::map<std::string, std::string>& schema = {}
    ) override;
    
    /// Drop an existing collection/table
    /// @param collection Name of the collection/table to drop
    /// @return Status indicating success or failure
    Status drop_collection(const std::string& collection) override;
    
    /// List all collections/tables in the database
    /// @param collections Output parameter for collection names
    /// @return Status indicating success or failure
    Status list_collections(std::vector<std::string>& collections) override;
    
    /// Check if a collection/table exists
    /// @param collection Name of the collection/table
    /// @param exists Output parameter indicating existence
    /// @return Status indicating success or failure
    Status collection_exists(
        const std::string& collection,
        bool& exists
    ) override;
    
    // ------------------------------------------------------------------------
    // Query Execution
    // ------------------------------------------------------------------------
    
    /// Execute a native query in the database's query language
    /// (e.g., SQL, Cypher, AQL, etc.)
    /// @param query Native query string
    /// @param results Output parameter for query results
    /// @param options Query options
    /// @return Status indicating success or failure
    Status execute_query(
        const std::string& query,
        ResultSet& results,
        const QueryOptions& options = {}
    ) override;
    
    /// Execute a parameterized native query
    /// @param query Native query string with parameter placeholders
    /// @param params Query parameters
    /// @param results Output parameter for query results
    /// @param options Query options
    /// @return Status indicating success or failure
    Status execute_query_params(
        const std::string& query,
        const std::map<std::string, Value>& params,
        ResultSet& results,
        const QueryOptions& options = {}
    ) override;
    
    // ------------------------------------------------------------------------
    // Metadata and Capabilities
    // ------------------------------------------------------------------------
    
    /// Get adapter name and version information
    /// @return String describing the adapter (e.g., "TemplateAdapter v1.0.0")
    std::string get_adapter_info() const override;
    
    /// Get the database system version
    /// @return String describing the database version
    std::string get_database_version() const override;
    
    /// Get a list of supported features/capabilities
    /// @return Vector of capability strings (e.g., "transactions", "vectors")
    std::vector<std::string> get_capabilities() const override;

private:
    /// Private implementation class (PIMPL pattern)
    /// This hides database-specific dependencies from the public interface
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/// Factory function to create Template adapter
std::unique_ptr<DatabaseAdapter> create_template_adapter();

} // namespace adapters
} // namespace chimera
