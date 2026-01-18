/// @file index_manager.cpp
/// @brief Implementation of unified index manager with DI

#include "index/index_manager.h"
#include "index/vector_index.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"
#include "storage/rocksdb_wrapper.h"
#include "utils/logger.h"
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

void IndexManager::setExpressionEvaluator(IExpressionEvaluatorPtr evaluator) {
    evaluator_ = evaluator;
    
    // Propagate to concrete index managers if they exist
    if (vector_manager_ && evaluator_) {
        vector_manager_->setExpressionEvaluator(evaluator_);
    }
    if (secondary_manager_ && evaluator_) {
        secondary_manager_->setExpressionEvaluator(evaluator_);
    }
    if (graph_manager_ && evaluator_) {
        graph_manager_->setExpressionEvaluator(evaluator_);
    }
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
        if (evaluator_) {
            vector_manager_->setExpressionEvaluator(evaluator_);
            secondary_manager_->setExpressionEvaluator(evaluator_);
            graph_manager_->setExpressionEvaluator(evaluator_);
        }
        
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

ISecondaryIndex* IndexManager::createSecondaryIndex(
    std::string_view name,
    std::string_view field_name,
    const std::string& config) {
    
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    if (!secondary_manager_) {
        THEMIS_ERROR("IndexManager::createSecondaryIndex: Secondary manager not initialized");
        return nullptr;
    }
    
    std::string name_str(name);
    
    // Check if index already exists
    if (secondary_indices_.find(name_str) != secondary_indices_.end()) {
        THEMIS_WARN("IndexManager::createSecondaryIndex: Index '{}' already exists", name_str);
        return secondary_indices_[name_str];
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
        return nullptr;
    }
    
    // For now, we return nullptr as ISecondaryIndex wrapper is not yet implemented
    // This will be implemented in a follow-up when we create adapter classes
    index_types_[name_str] = IndexType::SECONDARY;
    
    THEMIS_INFO("IndexManager::createSecondaryIndex: Created index '{}'", name_str);
    return nullptr;
}

IVectorIndex* IndexManager::createVectorIndex(
    std::string_view name,
    uint32_t dimension,
    const std::string& config) {
    
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    if (!vector_manager_) {
        THEMIS_ERROR("IndexManager::createVectorIndex: Vector manager not initialized");
        return nullptr;
    }
    
    std::string name_str(name);
    
    // Check if index already exists
    if (vector_indices_.find(name_str) != vector_indices_.end()) {
        THEMIS_WARN("IndexManager::createVectorIndex: Index '{}' already exists", name_str);
        return vector_indices_[name_str];
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
        return nullptr;
    }
    
    // For now, we return nullptr as IVectorIndex wrapper is not yet implemented
    index_types_[name_str] = IndexType::VECTOR;
    
    THEMIS_INFO("IndexManager::createVectorIndex: Created index '{}' with dimension {}", 
                name_str, dimension);
    return nullptr;
}

IGraphIndex* IndexManager::createGraphIndex(
    std::string_view name,
    const std::string& config) {
    
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    if (!graph_manager_) {
        THEMIS_ERROR("IndexManager::createGraphIndex: Graph manager not initialized");
        return nullptr;
    }
    
    std::string name_str(name);
    
    // Check if index already exists
    if (graph_indices_.find(name_str) != graph_indices_.end()) {
        THEMIS_WARN("IndexManager::createGraphIndex: Index '{}' already exists", name_str);
        return graph_indices_[name_str];
    }
    
    // Graph index is always available, just track it
    index_types_[name_str] = IndexType::GRAPH;
    
    THEMIS_INFO("IndexManager::createGraphIndex: Created graph index '{}'", name_str);
    return nullptr;
}

ISecondaryIndex* IndexManager::getSecondaryIndex(std::string_view name) const {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    std::string name_str(name);
    auto it = secondary_indices_.find(name_str);
    if (it != secondary_indices_.end()) {
        return it->second;
    }
    
    return nullptr;
}

IVectorIndex* IndexManager::getVectorIndex(std::string_view name) const {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    std::string name_str(name);
    auto it = vector_indices_.find(name_str);
    if (it != vector_indices_.end()) {
        return it->second;
    }
    
    return nullptr;
}

IGraphIndex* IndexManager::getGraphIndex(std::string_view name) const {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    std::string name_str(name);
    auto it = graph_indices_.find(name_str);
    if (it != graph_indices_.end()) {
        return it->second;
    }
    
    return nullptr;
}

bool IndexManager::dropIndex(std::string_view name) {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    std::string name_str(name);
    
    // Check index type and drop from appropriate manager
    auto type_it = index_types_.find(name_str);
    if (type_it == index_types_.end()) {
        THEMIS_WARN("IndexManager::dropIndex: Index '{}' not found", name_str);
        return false;
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
    }
    
    return success;
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

std::optional<IndexType> IndexManager::getIndexType(std::string_view name) const {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    std::string name_str(name);
    auto it = index_types_.find(name_str);
    if (it != index_types_.end()) {
        return it->second;
    }
    
    return std::nullopt;
}

} // namespace themis
