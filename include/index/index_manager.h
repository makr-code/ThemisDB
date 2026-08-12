/**
 * @file index_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=2, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/// @file index_manager.h
/// @brief Unified index manager with Dependency Injection
/// 
/// This file implements the IIndexManager interface to coordinate
/// VectorIndexManager, SecondaryIndexManager, and GraphIndexManager
/// with dependency injection of IExpressionEvaluator and IStorageEngine.
/// 
/// Design Goals:
/// - Break circular dependencies between Index ↔ Query ↔ Storage
/// - Enable isolated unit testing with mock implementations
/// - Support filter expressions via injected evaluator
/// - Maintain backward compatibility with existing code

#pragma once

#include "themis/base/interfaces/index_interface.h"
#include "themis/base/interfaces/query_interface.h"
#include "themis/base/interfaces/storage_interface.h"
#include "index/secondary_index.h"
#include "utils/expected.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <vector>

namespace themis {

// Forward declarations
class VectorIndexManager;
class SecondaryIndexManager;
class GraphIndexManager;
class RocksDBWrapper;

/// @brief Unified index manager coordinating all index types
/// 
/// IndexManager implements the IIndexManager interface and coordinates
/// the existing VectorIndexManager, SecondaryIndexManager, and GraphIndexManager.
/// It uses dependency injection for expression evaluation and storage.
class IndexManager : public IIndexManager {
public:
    /// @brief Constructor with Dependency Injection
    /// 
    /// @param evaluator Expression evaluator for WHERE clauses (optional)
    /// @param storage Optional storage for metadata (can be nullptr)
    explicit IndexManager(
        IExpressionEvaluatorPtr evaluator = nullptr,
        IStorageEnginePtr storage = nullptr
    );
    
    /// @brief Destructor
    ~IndexManager() override;
    
    /// @brief Static factory (backward compatible)
    /// Creates default implementation
    /// @return Shared pointer to a default IndexManager instance.
    static std::shared_ptr<IndexManager> createDefault();
    
    /// @brief Set evaluator (for late binding)
    void setExpressionEvaluator(IExpressionEvaluatorPtr evaluator);
    
    /// @brief Set storage (for late binding)
    void setStorage(IStorageEnginePtr storage);
    
    /// @brief Set RocksDB wrapper (for internal index managers)
    void setRocksDB(std::shared_ptr<RocksDBWrapper> db);
    
    /// @brief Get the expression evaluator
    /// @return Pointer to the expression evaluator (may be nullptr).
    IExpressionEvaluatorPtr getExpressionEvaluator() const;
    
    /// @brief Get the vector index manager
    /// @return Shared pointer to the VectorIndexManager instance.
    std::shared_ptr<VectorIndexManager> getVectorIndexManager() const;
    
    /// @brief Get the secondary index manager
    /// @return Shared pointer to the SecondaryIndexManager instance.
    std::shared_ptr<SecondaryIndexManager> getSecondaryIndexManager() const;
    
    /// @brief Get the graph index manager
    /// @return Shared pointer to the GraphIndexManager instance.
    std::shared_ptr<GraphIndexManager> getGraphIndexManager() const;
    
    // IIndexManager implementation
    
    Result<ISecondaryIndex*> createSecondaryIndex(
        std::string_view name,
        std::string_view field_name,
        const std::string& config = "") override;
    
    Result<IVectorIndex*> createVectorIndex(
        std::string_view name,
        uint32_t dimension,
        const std::string& config = "") override;
    
    Result<IGraphIndex*> createGraphIndex(
        std::string_view name,
        const std::string& config = "") override;
    
    Result<ISecondaryIndex*> getSecondaryIndex(std::string_view name) const override;
    
    Result<IVectorIndex*> getVectorIndex(std::string_view name) const override;
    
    Result<IGraphIndex*> getGraphIndex(std::string_view name) const override;
    
    Result<void> dropIndex(std::string_view name) override;
    
    std::vector<std::string> listIndexes() const override;
    
    Result<IndexType> getIndexType(std::string_view name) const override;

    // -------------------------------------------------------------------------
    // Index statistics export (Issue #1866)
    //
    // Collect per-index statistics from the underlying SecondaryIndexManager so
    // that the metadata module's StatisticsCollector can import them via
    // StatisticsCollector::importIndexStats().  The returned structs are in the
    // SecondaryIndexManager::IndexStats format; callers that need the
    // metadata::IndexStats representation should convert trivially by copying
    // the same-named fields and setting last_updated = system_clock::now().
    // -------------------------------------------------------------------------

    /// @brief Collect all secondary-index statistics for a given table.
    ///
    /// Returns one entry per (table, column) secondary index registered in the
    /// SecondaryIndexManager that was built around the same RocksDB instance.
    /// If no SecondaryIndexManager has been wired (i.e. setRocksDB was never
    /// called) an empty vector is returned rather than throwing.
    ///
    /// @param table_name  Table/collection whose index statistics to export.
    /// @return            Vector of per-index statistics (may be empty).
    std::vector<SecondaryIndexManager::IndexStats>
        exportIndexStats(std::string_view table_name) const;

    // -------------------------------------------------------------------------
    // Multi-tenancy index isolation (RocksDB key-prefix based)
    //
    // Each tenant's indexes are stored under the prefix "tenant:<id>:<name>" so
    // that data from different tenants is never accessible across boundaries.
    // -------------------------------------------------------------------------

    /// @brief Build the tenant-scoped RocksDB key prefix for an index.
    ///
    /// Format: "tenant:<tenant_id>:<index_name>"
    ///
    /// @param tenant_id  Non-empty tenant identifier
    /// @param index_name Logical index name within the tenant namespace
    /// @return Prefixed index key used internally by all underlying managers
    static std::string makeTenantIndexName(std::string_view tenant_id,
                                           std::string_view index_name);

    /// @brief Create a secondary index scoped to a specific tenant.
    Result<ISecondaryIndex*> createSecondaryIndex(std::string_view tenant_id,
                                                   std::string_view name,
                                                   std::string_view field_name,
                                                   const std::string& config = "");

    /// @brief Create a vector index scoped to a specific tenant.
    Result<IVectorIndex*> createVectorIndex(std::string_view tenant_id,
                                             std::string_view name,
                                             uint32_t dimension,
                                             const std::string& config = "");

    /// @brief Create a graph index scoped to a specific tenant.
    Result<IGraphIndex*> createGraphIndex(std::string_view tenant_id,
                                           std::string_view name,
                                           const std::string& config = "");

    /// @brief Look up a secondary index scoped to a specific tenant.
    Result<ISecondaryIndex*> getSecondaryIndex(std::string_view tenant_id,
                                                std::string_view name) const;

    /// @brief Look up a vector index scoped to a specific tenant.
    Result<IVectorIndex*> getVectorIndex(std::string_view tenant_id,
                                          std::string_view name) const;

    /// @brief Look up a graph index scoped to a specific tenant.
    Result<IGraphIndex*> getGraphIndex(std::string_view tenant_id,
                                        std::string_view name) const;

    /// @brief Drop an index scoped to a specific tenant.
    Result<void> dropIndex(std::string_view tenant_id, std::string_view name);

    /// @brief Drop all indexes belonging to a given tenant.
    ///
    /// Safe to call even when the tenant has no indexes (returns Ok).
    Result<void> dropTenantIndexes(std::string_view tenant_id);

    /// @brief List all index names registered for a specific tenant.
    ///
    /// Returns logical names (without the "tenant:<id>:" prefix).
    std::vector<std::string> listIndexes(std::string_view tenant_id) const;

    /// @brief Return the type of an index scoped to a specific tenant.
    Result<IndexType> getIndexType(std::string_view tenant_id,
                                    std::string_view name) const;

private:
    IExpressionEvaluatorPtr evaluator_;
    IStorageEnginePtr storage_;
    std::shared_ptr<RocksDBWrapper> db_;
    
    // Concrete index managers
    std::shared_ptr<VectorIndexManager> vector_manager_;
    std::shared_ptr<SecondaryIndexManager> secondary_manager_;
    std::shared_ptr<GraphIndexManager> graph_manager_;
    
    // Index registry
    mutable std::shared_mutex registry_mutex_;
    std::unordered_map<std::string, ISecondaryIndex*> secondary_indices_;
    std::unordered_map<std::string, IVectorIndex*> vector_indices_;
    std::unordered_map<std::string, IGraphIndex*> graph_indices_;
    std::unordered_map<std::string, IndexType> index_types_;

    // Owned index adapters (lifetime tied to this IndexManager)
    std::unordered_map<std::string, std::unique_ptr<ISecondaryIndex>> owned_secondary_adapters_;
    std::unordered_map<std::string, std::unique_ptr<IVectorIndex>> owned_vector_adapters_;
    
    // Helper method to propagate evaluator to all managers
    void propagateEvaluatorToManagers();
};

} // namespace themis

