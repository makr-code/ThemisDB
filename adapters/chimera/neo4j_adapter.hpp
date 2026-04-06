/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            neo4j_adapter.hpp                                  ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-04-06 04:03:13                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     236                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7f2a310e64  2026-03-10  fix(chimera): add missing neo4j_adapter.hpp to adapters/c... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 31a9305f66  2026-02-28  feat(chimera): Add Pinecone managed vector search adapter ║
    • 7bc4ff6d4d  2026-02-27  audit(chimera): fix Qdrant adapter post-implementation gaps ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// SPDX-License-Identifier: Apache-2.0 OR MIT
// Copyright (c) 2026 CHIMERA Suite Contributors
//
// Neo4j adapter implementation for the CHIMERA benchmark suite.
// This adapter provides access to Neo4j native graph database via the
// Bolt protocol / Neo4j HTTP API (simulation mode when no live server
// is available, backed by in-process std::unordered_map storage).

#pragma once

#include "chimera/database_adapter.hpp"
#include <memory>
#include <mutex>

namespace chimera {
namespace adapters {

/// Neo4j adapter implementation using the Bolt protocol / Neo4j HTTP API
///
/// Supported capabilities:
///   - Native graph traversal (BFS/DFS, node and edge navigation)
///   - Cypher graph query execution
///   - ACID transactions (begin / commit / rollback)
///   - Batch insert of nodes and edges
///   - Secondary indexes (property indexes on node labels)
///   - Document store via node property maps
///
/// Unsupported (returns UNSUPPORTED_OPERATION):
///   - Native vector similarity search
///   - SQL / relational queries
///
/// Connection string formats:
///   bolt://host[:port]
///   neo4j://host[:port]
///   neo4j+s://host[:port]  (TLS)
///
/// When a live Neo4j server is unavailable the adapter operates in an
/// in-process simulation mode suitable for unit testing without a
/// running server.
class Neo4jAdapter : public DatabaseAdapter {
public:
    Neo4jAdapter();
    ~Neo4jAdapter() override;

    // Prevent copying
    Neo4jAdapter(const Neo4jAdapter&) = delete;
    Neo4jAdapter& operator=(const Neo4jAdapter&) = delete;

    // ------------------------------------------------------------------------
    // Connection Management
    // ------------------------------------------------------------------------

    Status connect(const ConnectionConfig& config) override;
    Status disconnect() override;
    bool is_connected() const override;
    Status ping() override;

    // ------------------------------------------------------------------------
    // Basic CRUD Operations (mapped to node property store)
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
    // Vector Operations
    // ------------------------------------------------------------------------

    /// Neo4j does not natively support vector similarity search
    bool supports_vector_search() const override;

    Status vector_search(
        const std::string& collection,
        const std::string& vector_field,
        const VectorSearchParams& params,
        ResultSet& results
    ) override;

    Status create_vector_index(
        const std::string& collection,
        const std::string& field,
        size_t dimensions,
        const std::string& index_type = "none",
        const std::map<std::string, std::string>& parameters = {}
    ) override;

    // ------------------------------------------------------------------------
    // Transaction Management
    // ------------------------------------------------------------------------

    /// Neo4j supports full ACID transactions via the Bolt protocol
    bool supports_transactions() const override;

    Status begin_transaction(
        std::unique_ptr<Transaction>& transaction,
        IsolationLevel level = IsolationLevel::READ_COMMITTED
    ) override;

    // ------------------------------------------------------------------------
    // Schema Operations (label-based collections)
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
    // Query Execution (Cypher)
    // ------------------------------------------------------------------------

    /// Execute a Cypher query and return results as a generic ResultSet
    Status execute_query(
        const std::string& query,
        ResultSet& results,
        const QueryOptions& options = {}
    ) override;

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
    // Neo4j-specific Functions
    // ------------------------------------------------------------------------

    /// Get Neo4j server health and cluster status
    Status get_server_health(Document& health);

    /// Get all node labels present in the database
    Status list_labels(std::vector<std::string>& labels);

    /// Get all relationship types present in the database
    Status list_relationship_types(std::vector<std::string>& types);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/// Factory function to create a Neo4j adapter
std::unique_ptr<DatabaseAdapter> create_neo4j_adapter();

} // namespace adapters
} // namespace chimera
