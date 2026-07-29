/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            mongodb_adapter.hpp                                ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-04-06 04:03:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     258                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
    • 341d9d2439  2026-02-22  Add adapters/chimera/mongodb_adapter.hpp and update READM... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// SPDX-License-Identifier: Apache-2.0 OR MIT
// Copyright (c) 2026 CHIMERA Suite Contributors
//
// MongoDB adapter implementation for the CHIMERA benchmark suite.
// This adapter provides access to MongoDB databases via the official C++ driver
// (mongocxx) with optional Atlas Vector Search support.

#pragma once

#include "chimera/database_adapter.hpp"
#include <memory>
#include <mutex>

namespace chimera {
namespace adapters {

/// Transaction implementation for MongoDB multi-document transactions
/// Requires MongoDB 4.0+ running as a replica set or sharded cluster
class MongoDBTransaction : public Transaction {
public:
    explicit MongoDBTransaction(void* session_handle);
    ~MongoDBTransaction() override;

    Status commit() override;
    Status rollback() override;
    bool is_active() const override;

private:
    void* session_handle_;
    bool active_;
    mutable std::mutex mutex_;
};

/// MongoDB adapter implementation using the official mongocxx C++ driver
///
/// Supported capabilities:
///   - Document CRUD operations (insert, find, update, remove, count)
///   - Batch insert via ordered/unordered bulk write
///   - Atlas Vector Search via $vectorSearch aggregation pipeline
///   - Multi-document ACID transactions (MongoDB 4.0+, replica set required)
///   - Secondary indexes (single-field, compound, text, geospatial)
///   - Full-text search via text indexes
///   - Schema-on-write with optional JSON schema validation
///   - Collection management (create, drop, list, exists)
///
/// Unsupported / not applicable:
///   - Native graph traversal (use $graphLookup for limited graph-style queries)
///   - Relational / SQL queries
///
/// Connection string formats:
///   mongodb://[user:pass@]host[:port][/dbname][?options]
///   mongodb+srv://[user:pass@]host[/dbname][?options]
class MongoDBAdapter : public DatabaseAdapter {
public:
    MongoDBAdapter();
    ~MongoDBAdapter() override;

    // Non-copyable
    MongoDBAdapter(const MongoDBAdapter&) = delete;
    MongoDBAdapter& operator=(const MongoDBAdapter&) = delete;

    // ------------------------------------------------------------------------
    // Connection Management
    // ------------------------------------------------------------------------

    Status connect(const ConnectionConfig& config) override;
    Status disconnect() override;
    bool is_connected() const override;
    Status ping() override;

    // ------------------------------------------------------------------------
    // Basic CRUD Operations
    // ------------------------------------------------------------------------

    Status insert(
        const std::string& collection,
        const Document& document,
        const QueryOptions& options = {}
    ) override;

    Status insert_batch(
        const std::string& collection,
        const std::vector<Document>& documents,
        const QueryOptions& options = {}
    ) override;

    Status find(
        const std::string& collection,
        const Document& filter,
        ResultSet& results,
        const QueryOptions& options = {}
    ) override;

    Status find_by_id(
        const std::string& collection,
        const std::string& id,
        Document& result,
        const QueryOptions& options = {}
    ) override;

    Status update(
        const std::string& collection,
        const Document& filter,
        const Document& update,
        size_t& updated_count,
        const QueryOptions& options = {}
    ) override;

    Status remove(
        const std::string& collection,
        const Document& filter,
        size_t& deleted_count,
        const QueryOptions& options = {}
    ) override;

    Status count(
        const std::string& collection,
        const Document& filter,
        size_t& count,
        const QueryOptions& options = {}
    ) override;

    // ------------------------------------------------------------------------
    // Vector Operations (Atlas Vector Search)
    // ------------------------------------------------------------------------

    /// Returns true; Atlas Vector Search is supported via $vectorSearch pipeline
    bool supports_vector_search() const override;

    /// Execute vector similarity search using Atlas Vector Search
    /// Requires a vector search index on the specified field
    Status vector_search(
        const std::string& collection,
        const std::string& vector_field,
        const VectorSearchParams& params,
        ResultSet& results
    ) override;

    /// Create a vector search index for the specified collection and field
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

    /// Returns true; MongoDB 4.0+ supports multi-document transactions
    bool supports_transactions() const override;

    Status begin_transaction(
        std::unique_ptr<Transaction>& transaction,
        IsolationLevel level = IsolationLevel::READ_COMMITTED
    ) override;

    // ------------------------------------------------------------------------
    // Schema Operations
    // ------------------------------------------------------------------------

    Status create_collection(
        const std::string& collection,
        const std::map<std::string, std::string>& schema = {}
    ) override;

    Status drop_collection(const std::string& collection) override;

    Status list_collections(std::vector<std::string>& collections) override;

    Status collection_exists(
        const std::string& collection,
        bool& exists
    ) override;

    // ------------------------------------------------------------------------
    // Query Execution
    // ------------------------------------------------------------------------

    /// Execute a MongoDB aggregation pipeline expressed as JSON
    Status execute_query(
        const std::string& query,
        ResultSet& results,
        const QueryOptions& options = {}
    ) override;

    /// Execute a parameterized aggregation pipeline
    Status execute_query_params(
        const std::string& query,
        const std::map<std::string, Value>& params,
        ResultSet& results,
        const QueryOptions& options = {}
    ) override;

    // ------------------------------------------------------------------------
    // Metadata and Capabilities
    // ------------------------------------------------------------------------

    std::string get_adapter_info() const override;
    std::string get_database_version() const override;
    std::vector<std::string> get_capabilities() const override;

    // ------------------------------------------------------------------------
    // MongoDB-specific Functions
    // ------------------------------------------------------------------------

    /// Return the server buildInfo document as a key-value map
    Status get_server_build_info(Document& build_info);

    /// Create a compound index on multiple fields
    Status create_compound_index(
        const std::string& collection,
        const std::vector<std::pair<std::string, int>>& field_order_pairs,
        bool unique = false
    );

    /// Create a full-text search index on one or more string fields
    Status create_text_index(
        const std::string& collection,
        const std::vector<std::string>& fields
    );

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/// Factory function to create a MongoDB adapter instance
std::unique_ptr<DatabaseAdapter> create_mongodb_adapter();

} // namespace adapters
} // namespace chimera
