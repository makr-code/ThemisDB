/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            index_manager.h                                    ║
  Version:         0.0.20                                             ║
  Last Modified:   2026-02-21 19:14:24                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     152                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
#include "utils/expected.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>

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
    static std::shared_ptr<IndexManager> createDefault();
    
    /// @brief Set evaluator (for late binding)
    void setExpressionEvaluator(IExpressionEvaluatorPtr evaluator);
    
    /// @brief Set storage (for late binding)
    void setStorage(IStorageEnginePtr storage);
    
    /// @brief Set RocksDB wrapper (for internal index managers)
    void setRocksDB(std::shared_ptr<RocksDBWrapper> db);
    
    /// @brief Get the expression evaluator
    IExpressionEvaluatorPtr getExpressionEvaluator() const;
    
    /// @brief Get the vector index manager
    std::shared_ptr<VectorIndexManager> getVectorIndexManager() const;
    
    /// @brief Get the secondary index manager
    std::shared_ptr<SecondaryIndexManager> getSecondaryIndexManager() const;
    
    /// @brief Get the graph index manager
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

private:
    IExpressionEvaluatorPtr evaluator_;
    IStorageEnginePtr storage_;
    std::shared_ptr<RocksDBWrapper> db_;
    
    // Concrete index managers
    std::shared_ptr<VectorIndexManager> vector_manager_;
    std::shared_ptr<SecondaryIndexManager> secondary_manager_;
    std::shared_ptr<GraphIndexManager> graph_manager_;
    
    // Index registry
    mutable std::mutex registry_mutex_;
    std::unordered_map<std::string, ISecondaryIndex*> secondary_indices_;
    std::unordered_map<std::string, IVectorIndex*> vector_indices_;
    std::unordered_map<std::string, IGraphIndex*> graph_indices_;
    std::unordered_map<std::string, IndexType> index_types_;
    
    // Helper method to propagate evaluator to all managers
    void propagateEvaluatorToManagers();
};

} // namespace themis
