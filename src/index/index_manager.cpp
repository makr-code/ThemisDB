/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            index_manager.cpp                                  ║
  Version:         0.0.23                                             ║
  Last Modified:   2026-02-21 19:43:03                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     384                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 03329d86d  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/// @file index_manager.cpp
/// @brief Implementation of unified index manager with DI

#include "index/index_manager.h"
#include "index/vector_index.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"
#include "storage/rocksdb_wrapper.h"
#include "utils/logger.h"
#include "utils/expected.h"
#include "utils/tracing.h"
#include <fmt/format.h>
#include <stdexcept>

namespace themis {

IndexManager::IndexManager(
    IExpressionEvaluatorPtr evaluator,
    IStorageEnginePtr storage
) : evaluator_(evaluator)
  , storage_(storage)
  , db_(nullptr)
  , vector_manager_(nullptr)
  , secondary_manager_(nullptr)
  , graph_manager_(nullptr) {
    // Note: Concrete managers will be created when RocksDB is set
}

IndexManager::~IndexManager() = default;

std::shared_ptr<IndexManager> IndexManager::createDefault() {
    // Create with no dependencies initially
    return std::make_shared<IndexManager>(nullptr, nullptr);
}

void IndexManager::propagateEvaluatorToManagers() {
    if (!evaluator_) return;
    
    if (vector_manager_) {
        vector_manager_->setExpressionEvaluator(evaluator_);
    }
    if (secondary_manager_) {
        secondary_manager_->setExpressionEvaluator(evaluator_);
    }
    if (graph_manager_) {
        graph_manager_->setExpressionEvaluator(evaluator_);
    }
}

void IndexManager::setExpressionEvaluator(IExpressionEvaluatorPtr evaluator) {
    evaluator_ = evaluator;
    propagateEvaluatorToManagers();
}

void IndexManager::setStorage(IStorageEnginePtr storage) {
    storage_ = storage;
}

void IndexManager::setRocksDB(std::shared_ptr<RocksDBWrapper> db) {
    db_ = db;
    
    // Create concrete index managers when RocksDB is available
    if (db_) {
        vector_manager_ = std::make_shared<VectorIndexManager>(*db_);
        secondary_manager_ = std::make_shared<SecondaryIndexManager>(*db_);
        graph_manager_ = std::make_shared<GraphIndexManager>(*db_);
        
        // Propagate evaluator if already set
        propagateEvaluatorToManagers();
        
        THEMIS_INFO("IndexManager: Created concrete index managers with RocksDB");
    }
}

IExpressionEvaluatorPtr IndexManager::getExpressionEvaluator() const {
    return evaluator_;
}

std::shared_ptr<VectorIndexManager> IndexManager::getVectorIndexManager() const {
    return vector_manager_;
}

std::shared_ptr<SecondaryIndexManager> IndexManager::getSecondaryIndexManager() const {
    return secondary_manager_;
}

std::shared_ptr<GraphIndexManager> IndexManager::getGraphIndexManager() const {
    return graph_manager_;
}

// IIndexManager implementation

Result<ISecondaryIndex*> IndexManager::createSecondaryIndex(
    std::string_view name,
    std::string_view field_name,
    const std::string& config) {
    
    TracedSpan span("IndexManager.createSecondaryIndex");
    span.setAttribute("index.name", std::string(name));
    span.setAttribute("index.field", std::string(field_name));
    span.setAttribute("index.config", config);
    
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    if (!secondary_manager_) {
        THEMIS_ERROR("IndexManager::createSecondaryIndex: Secondary manager not initialized");
        span.setStatus(false, "Secondary manager not initialized");
        return Err<ISecondaryIndex*>(errors::ErrorCode::ERR_INDEX_NOT_INITIALIZED, 
                                       fmt::format("Index manager not initialized for index '{}'", name));
    }
    
    std::string name_str(name);
    
    // Check if index already exists
    if (secondary_indices_.find(name_str) != secondary_indices_.end()) {
        THEMIS_WARN("IndexManager::createSecondaryIndex: Index '{}' already exists", name_str);
        auto* existing = secondary_indices_[name_str];
        return Ok<ISecondaryIndex*>(std::move(existing));
    }
    
    // Parse config for index type (default: REGULAR)
    SecondaryIndexManager::IndexType idx_type = SecondaryIndexManager::IndexType::REGULAR;
    if (config == "range") {
        idx_type = SecondaryIndexManager::IndexType::RANGE;
    } else if (config == "fulltext") {
        idx_type = SecondaryIndexManager::IndexType::FULLTEXT;
    } else if (config == "geo") {
        idx_type = SecondaryIndexManager::IndexType::GEO;
    } else if (config == "sparse") {
        idx_type = SecondaryIndexManager::IndexType::SPARSE;
    }
    
    // Create the index using the concrete manager
    auto status = secondary_manager_->createIndex(name, field_name, idx_type);
    if (!status.ok) {
        THEMIS_ERROR("IndexManager::createSecondaryIndex: Failed to create index '{}': {}", 
                     name_str, status.message);
        span.setStatus(false, status.message);
        return Err<ISecondaryIndex*>(errors::ErrorCode::ERR_INDEX_CREATION_FAILED,
                                       fmt::format("Failed to create index '{}': {}", name_str, status.message));
    }
    
    // For now, we return nullptr as ISecondaryIndex wrapper is not yet implemented
    // This will be implemented in a follow-up when we create adapter classes
    index_types_[name_str] = IndexType::SECONDARY;
    
    THEMIS_INFO("IndexManager::createSecondaryIndex: Created index '{}'", name_str);
    span.setStatus(true);
    return Ok<ISecondaryIndex*>(nullptr);
}

Result<IVectorIndex*> IndexManager::createVectorIndex(
    std::string_view name,
    uint32_t dimension,
    const std::string& config) {
    
    TracedSpan span("IndexManager.createVectorIndex");
    span.setAttribute("index.name", std::string(name));
    span.setAttribute("index.dimension", static_cast<int64_t>(dimension));
    span.setAttribute("index.config", config);
    
    (void)config;
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    if (!vector_manager_) {
        THEMIS_ERROR("IndexManager::createVectorIndex: Vector manager not initialized");
        span.setStatus(false, "Vector manager not initialized");
        return Err<IVectorIndex*>(errors::ErrorCode::ERR_INDEX_NOT_INITIALIZED,
                                    fmt::format("Vector index manager not initialized for index '{}'", name));
    }
    
    std::string name_str(name);
    
    // Check if index already exists
    if (vector_indices_.find(name_str) != vector_indices_.end()) {
        THEMIS_WARN("IndexManager::createVectorIndex: Index '{}' already exists", name_str);
        auto* existing = vector_indices_[name_str];
        return Ok<IVectorIndex*>(std::move(existing));
    }
    
    // Initialize vector index
    VectorIndexManager::Metric metric = VectorIndexManager::Metric::COSINE;
    int M = 16;
    int efConstruction = 200;
    int efSearch = 64;
    
    // Parse config (format: "metric:COSINE,M:16,ef:200")
    // For simplicity, use defaults for now
    
    auto status = vector_manager_->init(name, dimension, metric, M, efConstruction, efSearch);
    if (!status.ok) {
        THEMIS_ERROR("IndexManager::createVectorIndex: Failed to create index '{}': {}", 
                     name_str, status.message);
        span.setStatus(false, status.message);
        return Err<IVectorIndex*>(errors::ErrorCode::ERR_INDEX_CREATION_FAILED,
                                    fmt::format("Failed to create vector index '{}': {}", name_str, status.message));
    }
    
    // For now, we return nullptr as IVectorIndex wrapper is not yet implemented
    index_types_[name_str] = IndexType::VECTOR;
    
    THEMIS_INFO("IndexManager::createVectorIndex: Created index '{}' with dimension {}", 
                name_str, dimension);
    span.setStatus(true);
    return Ok<IVectorIndex*>(nullptr);
}

Result<IGraphIndex*> IndexManager::createGraphIndex(
    std::string_view name,
    const std::string& config) {
    
    (void)config;
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    if (!graph_manager_) {
        THEMIS_ERROR("IndexManager::createGraphIndex: Graph manager not initialized");
        return Err<IGraphIndex*>(errors::ErrorCode::ERR_INDEX_NOT_INITIALIZED,
                                   fmt::format("Graph index manager not initialized for index '{}'", name));
    }
    
    std::string name_str(name);
    
    // Check if index already exists
    if (graph_indices_.find(name_str) != graph_indices_.end()) {
        THEMIS_WARN("IndexManager::createGraphIndex: Index '{}' already exists", name_str);
        auto* existing = graph_indices_[name_str];
        return Ok<IGraphIndex*>(std::move(existing));
    }
    
    // Graph index is always available, just track it
    index_types_[name_str] = IndexType::GRAPH;
    
    THEMIS_INFO("IndexManager::createGraphIndex: Created graph index '{}'", name_str);
    return Ok<IGraphIndex*>(nullptr);
}

Result<ISecondaryIndex*> IndexManager::getSecondaryIndex(std::string_view name) const {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    std::string name_str(name);
    auto it = secondary_indices_.find(name_str);
    if (it != secondary_indices_.end()) {
        ISecondaryIndex* ptr = it->second;
        return themis::Ok(ptr);
    }
    
    return Err<ISecondaryIndex*>(errors::ErrorCode::ERR_INDEX_NOT_FOUND,
                                   fmt::format("Secondary index '{}' not found", name_str));
}

Result<IVectorIndex*> IndexManager::getVectorIndex(std::string_view name) const {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    std::string name_str(name);
    auto it = vector_indices_.find(name_str);
    if (it != vector_indices_.end()) {
        IVectorIndex* ptr = it->second;
        return themis::Ok(ptr);
    }
    
    return Err<IVectorIndex*>(errors::ErrorCode::ERR_INDEX_NOT_FOUND,
                                fmt::format("Vector index '{}' not found", name_str));
}

Result<IGraphIndex*> IndexManager::getGraphIndex(std::string_view name) const {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    std::string name_str(name);
    auto it = graph_indices_.find(name_str);
    if (it != graph_indices_.end()) {
        IGraphIndex* ptr = it->second;
        return themis::Ok(ptr);
    }
    
    return Err<IGraphIndex*>(errors::ErrorCode::ERR_INDEX_NOT_FOUND,
                               fmt::format("Graph index '{}' not found", name_str));
}

Result<void> IndexManager::dropIndex(std::string_view name) {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    std::string name_str(name);
    
    // Check index type and drop from appropriate manager
    auto type_it = index_types_.find(name_str);
    if (type_it == index_types_.end()) {
        THEMIS_WARN("IndexManager::dropIndex: Index '{}' not found", name_str);
        return ErrVoid(errors::ErrorCode::ERR_INDEX_NOT_FOUND,
                       fmt::format("Index '{}' not found", name_str));
    }
    
    bool success = false;
    
    switch (type_it->second) {
        case IndexType::SECONDARY:
            if (secondary_manager_) {
                // For secondary index, we need to extract table and column from name
                // Format is typically "table:column"
                auto status = secondary_manager_->dropIndex(name, "");
                success = status.ok;
            }
            secondary_indices_.erase(name_str);
            break;
            
        case IndexType::VECTOR:
            // Vector index doesn't have explicit drop, just remove from registry
            vector_indices_.erase(name_str);
            success = true;
            break;
            
        case IndexType::GRAPH:
            // Graph index doesn't have explicit drop, just remove from registry
            graph_indices_.erase(name_str);
            success = true;
            break;
            
        default:
            THEMIS_WARN("IndexManager::dropIndex: Unknown index type for '{}'", name_str);
            break;
    }
    
    if (success) {
        index_types_.erase(name_str);
        THEMIS_INFO("IndexManager::dropIndex: Dropped index '{}'", name_str);
        return OkVoid();
    }
    
    return ErrVoid(errors::ErrorCode::ERR_INDEX_NOT_FOUND,
                   fmt::format("Failed to drop index '{}'", name_str));
}

std::vector<std::string> IndexManager::listIndexes() const {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    std::vector<std::string> indices;
    indices.reserve(index_types_.size());
    
    for (const auto& [name, type] : index_types_) {
        indices.push_back(name);
    }
    
    return indices;
}

Result<IndexType> IndexManager::getIndexType(std::string_view name) const {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    std::string name_str(name);
    auto it = index_types_.find(name_str);
    if (it != index_types_.end()) {
        return Ok(it->second);
    }
    
    return Err<IndexType>(errors::ErrorCode::ERR_INDEX_NOT_FOUND,
                           fmt::format("Index '{}' not found", name_str));
}

} // namespace themis
