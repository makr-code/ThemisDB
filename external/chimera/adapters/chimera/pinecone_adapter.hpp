/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            pinecone_adapter.hpp                               ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-04-06 04:03:14                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     211                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
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
// Pinecone adapter implementation for the CHIMERA benchmark suite.
// This adapter provides access to the Pinecone managed vector search
// service via the Pinecone REST API (v3 serverless).

#pragma once

#include "chimera/database_adapter.hpp"
#include <memory>
#include <mutex>

namespace chimera {
namespace adapters {

/// Pinecone adapter implementation using the Pinecone REST API
/// Supports managed ANN vector similarity search with metadata filtering
/// across named namespaces (mapped to CHIMERA collections).
class PineconeAdapter : public DatabaseAdapter {
public:
    PineconeAdapter();
    ~PineconeAdapter() override;

    // Prevent copying
    PineconeAdapter(const PineconeAdapter&) = delete;
    PineconeAdapter& operator=(const PineconeAdapter&) = delete;

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
    // Vector Operations
    // ------------------------------------------------------------------------

    /// Pinecone natively supports managed ANN vector operations
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
        const std::string& index_type = "cosine",
        const std::map<std::string, std::string>& parameters = {}
    ) override;

    // ------------------------------------------------------------------------
    // Transaction Management
    // ------------------------------------------------------------------------

    /// Pinecone does not support traditional ACID transactions
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
    // Pinecone-specific Functions
    // ------------------------------------------------------------------------

    /// Get Pinecone index statistics (total vector count, namespaces, etc.)
    Status get_index_stats(Document& stats);

    /// Describe the Pinecone index configuration (dimensions, metric, etc.)
    Status describe_index(Document& info);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/// Factory function to create Pinecone adapter
std::unique_ptr<DatabaseAdapter> create_pinecone_adapter();

} // namespace adapters
} // namespace chimera
