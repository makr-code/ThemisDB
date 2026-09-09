/**
 * @file index_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "index/index_manager.h"
#include "index/graph_index.h"
#include "index/secondary_index.h"
#include "index/vector_index.h"
#include "index/connection_guard.h"  // Phase 3 A-6: Connection leak prevention
#include "storage/base_entity.h"
#include "storage/rocksdb_wrapper.h"
#include "utils/expected.h"
#include "utils/logger.h"
#include "utils/tracing.h"

#include <fmt/format.h>

#include <optional>

namespace themis {

/// Default maximum number of results returned by rangeScan().
static constexpr size_t kDefaultRangeScanLimit = 1000;

namespace {

/// @brief Adapter bridging SecondaryIndexManager to the ISecondaryIndex interface.
///
/// Maps ISecondaryIndex operations to SecondaryIndexManager equivalents:
///   - insert()    -> put()
///   - remove()    -> erase()
///   - lookup()    -> scanKeysEqual()/scanKeysEqualPartial()
///   - rangeScan() -> scanKeysRange()
class SecondaryIndexAdapter final : public ISecondaryIndex {
public:
    SecondaryIndexAdapter(std::shared_ptr<SecondaryIndexManager> manager,
                          std::string table_name,
                          std::string field_name,
                          bool is_partial,
                          std::string predicate)
        : manager_(std::move(manager))
        , table_name_(std::move(table_name))
        , field_name_(std::move(field_name))
        , is_partial_(is_partial)
        , predicate_(std::move(predicate)) {}

    bool insert(std::string_view indexed_value,
                std::string_view primary_key) override {
        BaseEntity e(primary_key);
        e.setField(field_name_, std::string(indexed_value));
        return manager_->put(table_name_, e).ok;
    }

    bool remove(std::string_view /*indexed_value*/,
                std::string_view primary_key) override {
        return manager_->erase(table_name_, primary_key).ok;
    }

    std::vector<std::string> lookup(std::string_view value) const override {
        if (is_partial_) {
            auto [st, keys] = manager_->scanKeysEqualPartial(table_name_, field_name_, value);
            if (!st.ok) {
                THEMIS_WARN("SecondaryIndexAdapter::lookup: scanKeysEqualPartial failed for {}.{}", table_name_, field_name_);
                return {};
            }
            return keys;
        }
        auto [st, keys] = manager_->scanKeysEqual(table_name_, field_name_, value);
        if (!st.ok) {
            THEMIS_WARN("SecondaryIndexAdapter::lookup: scanKeysEqual failed for {}.{}", table_name_, field_name_);
            return {};
        }
        return keys;
    }

    std::vector<std::string> rangeScan(
            std::string_view start_value,
            std::string_view end_value,
            ScanOrder order = ScanOrder::ASCENDING) const override {
        bool reversed = (order == ScanOrder::DESCENDING);
        auto lower = std::optional<std::string>(std::string(start_value));
        auto upper = std::optional<std::string>(std::string(end_value));
        auto [st, keys] = manager_->scanKeysRange(
            table_name_, field_name_,
            lower, upper,
            /*includeLower=*/true, /*includeUpper=*/false,
            kDefaultRangeScanLimit, reversed);
        if (!st.ok) {
            THEMIS_WARN("SecondaryIndexAdapter::rangeScan: scanKeysRange failed for {}.{}", table_name_, field_name_);
            return {};
        }
        return keys;
    }

    std::string getName() const override { return table_name_; }
    std::string getFieldName() const override { return field_name_; }

    std::string getStatistics() const override {
        auto stats = manager_->getIndexStats(table_name_, field_name_);
        return fmt::format(
            R"({{"type":"{}","entry_count":{},"unique":{},"predicate":"{}"}})",
            stats.type, stats.entry_count, stats.unique, predicate_);
    }

    bool isPartial() const { return is_partial_; }

private:
    std::shared_ptr<SecondaryIndexManager> manager_;
    std::string table_name_;
    std::string field_name_;
    bool is_partial_;
    std::string predicate_;
};

/// @brief Adapter bridging VectorIndexManager to the IVectorIndex interface.
///
/// Each adapter owns a dedicated VectorIndexManager instance (per-index isolation).
/// Lifetime is tied to the owning IndexManager via `owned_vector_adapters_`.
///
/// Maps IVectorIndex operations to VectorIndexManager equivalents:
///   - insert()      -> addEntity()  (wraps the vector in a BaseEntity)
///   - remove()      -> removeByPk()
///   - search()      -> searchKnn()
///   - rangeSearch() -> searchKnnRadius()
class VectorIndexAdapter final : public IVectorIndex {
public:
    VectorIndexAdapter(std::shared_ptr<VectorIndexManager> manager, std::string name)
        : manager_(std::move(manager)), name_(std::move(name)) {}

    bool insert(std::string_view primary_key,
                const std::vector<float>& vector) override {
        BaseEntity e(primary_key);
        e.setField("embedding", vector);
        return manager_->addEntity(e, "embedding").ok;
    }

    bool remove(std::string_view primary_key) override {
        return manager_->removeByPk(primary_key).ok;
    }

    std::vector<VectorSearchResult> search(
        const std::vector<float>& query_vector,
        uint32_t k,
        const IExpressionEvaluator* filter = nullptr) const override {

        auto [status, results] = manager_->searchKnnEvaluated(
            query_vector, k, filter, /*candidateMultiplier=*/4, /*whitelist=*/nullptr);
        if (!status.ok) {
            THEMIS_WARN("VectorIndexAdapter::search: underlying evaluator-aware search failed: {}", status.message);
            return {};
        }
        std::vector<VectorSearchResult> out = {};

        out.reserve(results.size());
        for (const auto& r : results) {
            out.emplace_back(r.pk, r.distance);
        }
        return out;
    }

    std::vector<VectorSearchResult> rangeSearch(
        const std::vector<float>& query_vector,
        float max_distance,
        const IExpressionEvaluator* filter = nullptr) const override {

        auto [status, results] = manager_->searchKnnRadiusEvaluated(
            query_vector, max_distance, /*max_results=*/0, filter, /*whitelist=*/nullptr);
        if (!status.ok) {
            THEMIS_WARN("VectorIndexAdapter::rangeSearch: underlying evaluator-aware radius search failed: {}", status.message);
            return {};
        }
        std::vector<VectorSearchResult> out = {};

        out.reserve(results.size());
        for (const auto& r : results) {
            out.emplace_back(r.pk, r.distance);
        }
        return out;
    }

    std::string getName() const override { return name_; }

    uint32_t getDimension() const override {
        return static_cast<uint32_t>(manager_->getDimension());
    }

    std::string getStatistics() const override {
        auto [status, stats] = manager_->getStatistics();
        if (!status.ok) return "{}";
        return fmt::format(
            R"({{"vector_count":{},"dimension":{},"metric":"{}","min_distance":{},"max_distance":{},"mean_distance":{}}})",
            stats.vector_count, stats.dimension, stats.metric_name,
            stats.min_distance, stats.max_distance, stats.mean_distance);
    }

private:
    std::shared_ptr<VectorIndexManager> manager_;
    std::string name_;
};

} // anonymous namespace

namespace {

/// Validates a string that will be used as a component in the tenant-scoped
/// key `"tenant:<tenant_id>:<index_name>"`.
///
/// The separator between components is `:`.  Allowing `:` inside either
/// component would let a caller with tenant id "a:b" construct the same storage
/// key as a caller with tenant id "a" and index name "b:x", enabling cross-
/// tenant data access (audit finding #1872).
///
/// Rules enforced:
///   - Must not be empty.
///   - Must not contain the separator character `:`.
///   - Must not contain null bytes (early-termination bypass in C-string APIs).
///   - Must not exceed 512 bytes (prevents key-length amplification attacks).
///
/// @return true when the component is safe to embed in a tenant key.
static bool isValidTenantComponent(std::string_view s) noexcept {
    if (s.empty() || static_cast<int>(s.size()) > 512) {
      return false;
    }
    for (char c : s) {
        if (c == ':' || c == '\0') {
          return false;
        }
    }
    return true;
}

} // namespace (validation helpers)

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
    if (!evaluator_) {
      return;
    }
    
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
    
    std::unique_lock<std::shared_mutex> lock(registry_mutex_);
    
    if (!secondary_manager_) {
        THEMIS_ERROR("IndexManager::createSecondaryIndex: Secondary manager not initialized");
        span.setStatus(false, "Secondary manager not initialized");
        return Err<ISecondaryIndex*>(errors::ErrorCode::ERR_INDEX_NOT_INITIALIZED, 
                                       fmt::format("Index manager not initialized for index '{}'", name));
    }
    
    std::string name_str(name);
    const std::string registry_key = name_str + "##" + std::string(field_name);

    if (auto exact_it = secondary_indices_.find(registry_key);
        exact_it != secondary_indices_.end()) {
        auto* existing = exact_it->second;
        if (existing == nullptr) {
            THEMIS_ERROR("IndexManager::createSecondaryIndex: Index '{}.{}' registry entry is null", name_str, field_name);
            span.setStatus(false, "Index registry entry is null");
            return Err<ISecondaryIndex*>(errors::ErrorCode::ERR_INDEX_NOT_FOUND,
                                         fmt::format("Index '{}.{}' exists but registry entry is null", name_str, field_name));
        }
        return Ok<ISecondaryIndex*>(std::move(existing));
    }
    
    // Check if index already exists
    if (auto existing_it = secondary_indices_.find(name_str);
        existing_it != secondary_indices_.end()) {
        auto* existing = existing_it->second;
        if (existing == nullptr) {
            THEMIS_ERROR("IndexManager::createSecondaryIndex: Index '{}' registry entry is null", name_str);
            span.setStatus(false, "Index registry entry is null");
            return Err<ISecondaryIndex*>(errors::ErrorCode::ERR_INDEX_NOT_FOUND,
                                         fmt::format("Index '{}' exists but registry entry is null", name_str));
        }

        const auto* existing_adapter = dynamic_cast<SecondaryIndexAdapter*>(existing);
        if (existing_adapter != nullptr && existing_adapter->getFieldName() == field_name) {
            return Ok<ISecondaryIndex*>(std::move(existing));
        }
    }
    
    // Parse config for index type (default: REGULAR)
    // Supported config values:
    //   "range"              → RANGE index
    //   "fulltext"           → FULLTEXT inverted index
    //   "geo"                → GEO spatial index
    //   "sparse"             → SPARSE index (skip NULL/missing values)
    //   "partial:<predicate>"→ PARTIAL (filtered) index; only rows matching
    //                          the predicate are indexed.
    //                          Predicate syntax: "field = 'value'",
    //                          "field > 123", "field IS NOT NULL", etc.
    bool is_partial = false;
    std::string predicate = {};
    SecondaryIndexManager::IndexType idx_type = SecondaryIndexManager::IndexType::REGULAR;

    if (config == "range") {
        idx_type = SecondaryIndexManager::IndexType::RANGE;
    } else if (config == "fulltext") {
        idx_type = SecondaryIndexManager::IndexType::FULLTEXT;
    } else if (config == "geo") {
        idx_type = SecondaryIndexManager::IndexType::GEO;
    } else if (config == "sparse") {
        idx_type = SecondaryIndexManager::IndexType::SPARSE;
    } else if (config.starts_with("partial:")) {
        is_partial = true;
        predicate = config.substr(8); // strip leading "partial:"
    }

    // Create the underlying index
    SecondaryIndexManager::Status status = SecondaryIndexManager::Status::Error("Index creation was not attempted");
    if (is_partial) {
        status = secondary_manager_->createPartialIndex(name, field_name, predicate);
    } else {
        status = secondary_manager_->createIndex(name, field_name, idx_type);
    }

    if (!status.ok) {
        THEMIS_ERROR("IndexManager::createSecondaryIndex: Failed to create index '{}': {}", 
                     name_str, status.message);
        span.setStatus(false, status.message);
        return Err<ISecondaryIndex*>(errors::ErrorCode::ERR_INDEX_CREATION_FAILED,
                                       fmt::format("Failed to create index '{}': {}", name_str, status.message));
    }

    // Wrap in adapter and register
    auto adapter = std::make_unique<SecondaryIndexAdapter>(
        secondary_manager_, name_str, std::string(field_name),
        is_partial, predicate);
    ISecondaryIndex* raw_ptr = adapter.get();

    if (secondary_indices_.find(name_str) == secondary_indices_.end()) {
        owned_secondary_adapters_[name_str] = std::move(adapter);
        secondary_indices_[name_str] = raw_ptr;
        index_types_[name_str] = IndexType::SECONDARY;
    } else {
        owned_secondary_adapters_[registry_key] = std::move(adapter);
        secondary_indices_[registry_key] = raw_ptr;
    }

    THEMIS_INFO("IndexManager::createSecondaryIndex: Created {} index '{}'",
                is_partial ? fmt::format("partial({})", predicate) : "regular",
                name_str);
    span.setStatus(true);
    return Ok<ISecondaryIndex*>(std::move(raw_ptr));
}

Result<IVectorIndex*> IndexManager::createVectorIndex(
    std::string_view name,
    uint32_t dimension,
    const std::string& config) {
    
    TracedSpan span("IndexManager.createVectorIndex");
    span.setAttribute("index.name", std::string(name));
    span.setAttribute("index.dimension", static_cast<int64_t>(dimension));
    span.setAttribute("index.config", config);
    
    std::lock_guard<std::shared_mutex> lock(registry_mutex_);
    
    if (!vector_manager_) {
        THEMIS_ERROR("IndexManager::createVectorIndex: Vector manager not initialized");
        span.setStatus(false, "Vector manager not initialized");
        return Err<IVectorIndex*>(errors::ErrorCode::ERR_INDEX_NOT_INITIALIZED,
                                    fmt::format("Vector index manager not initialized for index '{}'", name));
    }
    
    std::string name_str(name);
    
    // Check if index already exists
    if (auto existing_it = vector_indices_.find(name_str);
        existing_it != vector_indices_.end()) {
        THEMIS_WARN("IndexManager::createVectorIndex: Index '{}' already exists", name_str);
        auto* existing = existing_it->second;
        if (existing == nullptr) {
            THEMIS_ERROR("IndexManager::createVectorIndex: Index '{}' registry entry is null", name_str);
            span.setStatus(false, "Index registry entry is null");
            return Err<IVectorIndex*>(errors::ErrorCode::ERR_INDEX_NOT_FOUND,
                                      fmt::format("Vector index '{}' exists but registry entry is null", name_str));
        }
        return Ok<IVectorIndex*>(std::move(existing));
    }
    
    // Create a dedicated VectorIndexManager for this index (per-index isolation)
    auto per_index_manager = std::make_shared<VectorIndexManager>(*db_);
    VectorIndexManager::Metric metric = VectorIndexManager::Metric::COSINE;
    int M = 16;
    int efConstruction = 200;
    int efSearch = 64;

    // Parse config (format: "metric:COSINE,M:16,ef:200")
    // For simplicity, use defaults for now

    auto status = per_index_manager->init(name, static_cast<int>(dimension),
                                          metric, M, efConstruction, efSearch);
    if (!status.ok) {
        THEMIS_ERROR("IndexManager::createVectorIndex: Failed to create index '{}': {}", 
                     name_str, status.message);
        span.setStatus(false, status.message);
        return Err<IVectorIndex*>(errors::ErrorCode::ERR_INDEX_CREATION_FAILED,
                                    fmt::format("Failed to create vector index '{}': {}", name_str, status.message));
    }

    // Wrap the manager in an adapter and register it
    auto adapter = std::make_unique<VectorIndexAdapter>(std::move(per_index_manager), name_str);
    IVectorIndex* raw_ptr = adapter.get();
    owned_vector_adapters_[name_str] = std::move(adapter);
    vector_indices_[name_str] = raw_ptr;
    index_types_[name_str] = IndexType::VECTOR;

    THEMIS_INFO("IndexManager::createVectorIndex: Created index '{}' with dimension {}", 
                name_str, dimension);
    span.setStatus(true);
    return Ok<IVectorIndex*>(std::move(raw_ptr));
}

Result<IGraphIndex*> IndexManager::createGraphIndex(
    std::string_view name,
    const std::string& config) {
    
    std::lock_guard<std::shared_mutex> lock(registry_mutex_);
    
    if (!graph_manager_) {
        THEMIS_ERROR("IndexManager::createGraphIndex: Graph manager not initialized");
        return Err<IGraphIndex*>(errors::ErrorCode::ERR_INDEX_NOT_INITIALIZED,
                                   fmt::format("Graph index manager not initialized for index '{}'", name));
    }
    
    std::string name_str(name);
    
    // Check if index already exists
    if (auto existing_it = graph_indices_.find(name_str);
        existing_it != graph_indices_.end()) {
        THEMIS_WARN("IndexManager::createGraphIndex: Index '{}' already exists", name_str);
        auto* existing = existing_it->second;
        if (existing == nullptr) {
            THEMIS_ERROR("IndexManager::createGraphIndex: Index '{}' registry entry is null", name_str);
            return Err<IGraphIndex*>(errors::ErrorCode::ERR_INDEX_NOT_FOUND,
                                     fmt::format("Graph index '{}' exists but registry entry is null", name_str));
        }
        return Ok<IGraphIndex*>(std::move(existing));
    }
    
    // Graph index is always available, just track it
    index_types_[name_str] = IndexType::GRAPH;
    
    THEMIS_INFO("IndexManager::createGraphIndex: Created graph index '{}'", name_str);
    return Ok<IGraphIndex*>(nullptr);
}

Result<ISecondaryIndex*> IndexManager::getSecondaryIndex(std::string_view name) const {
    std::shared_lock<std::shared_mutex> lock(registry_mutex_);
    
    std::string name_str(name);
    auto it = secondary_indices_.find(name_str);
    if (it != secondary_indices_.end()) {
        ISecondaryIndex* ptr = it->second;
        if (ptr == nullptr) {
            return Err<ISecondaryIndex*>(errors::ErrorCode::ERR_INDEX_NOT_FOUND,
                                         fmt::format("Secondary index '{}' has null registry entry", name_str));
        }
        return themis::Ok(ptr);
    }
    
    return Err<ISecondaryIndex*>(errors::ErrorCode::ERR_INDEX_NOT_FOUND,
                                   fmt::format("Secondary index '{}' not found", name_str));
}

Result<IVectorIndex*> IndexManager::getVectorIndex(std::string_view name) const {
    std::shared_lock<std::shared_mutex> lock(registry_mutex_);
    
    std::string name_str(name);
    auto it = vector_indices_.find(name_str);
    if (it != vector_indices_.end()) {
        IVectorIndex* ptr = it->second;
        if (ptr == nullptr) {
            return Err<IVectorIndex*>(errors::ErrorCode::ERR_INDEX_NOT_FOUND,
                                      fmt::format("Vector index '{}' has null registry entry", name_str));
        }
        return themis::Ok(ptr);
    }
    
    return Err<IVectorIndex*>(errors::ErrorCode::ERR_INDEX_NOT_FOUND,
                                fmt::format("Vector index '{}' not found", name_str));
}

Result<IGraphIndex*> IndexManager::getGraphIndex(std::string_view name) const {
    std::shared_lock<std::shared_mutex> lock(registry_mutex_);
    
    std::string name_str(name);
    auto it = graph_indices_.find(name_str);
    if (it != graph_indices_.end()) {
        IGraphIndex* ptr = it->second;
        if (ptr == nullptr) {
            return Err<IGraphIndex*>(errors::ErrorCode::ERR_INDEX_NOT_FOUND,
                                     fmt::format("Graph index '{}' has null registry entry", name_str));
        }
        return themis::Ok(ptr);
    }
    
    return Err<IGraphIndex*>(errors::ErrorCode::ERR_INDEX_NOT_FOUND,
                               fmt::format("Graph index '{}' not found", name_str));
}

Result<void> IndexManager::dropIndex(std::string_view name) {
    std::unique_lock<std::shared_mutex> lock(registry_mutex_);
    
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
        case IndexType::SECONDARY: {
            if (secondary_manager_) {
                auto adapter_it = owned_secondary_adapters_.find(name_str);
                if (adapter_it != owned_secondary_adapters_.end()) {
                    auto* sa = static_cast<SecondaryIndexAdapter*>(
                        adapter_it->second.get());
                    SecondaryIndexManager::Status drop_status = SecondaryIndexManager::Status::Error("Index drop was not attempted");
                    if (sa->isPartial()) {
                        drop_status = secondary_manager_->dropPartialIndex(
                            sa->getName(), sa->getFieldName());
                    } else {
                        drop_status = secondary_manager_->dropIndex(
                            sa->getName(), sa->getFieldName());
                    }
                    success = drop_status.ok;
                } else {
                    // Adapter not found (index was registered without adapter)
                    success = true;
                }
            }
            secondary_indices_.erase(name_str);
            owned_secondary_adapters_.erase(name_str);
            break;
        }
            
        case IndexType::VECTOR:
            // Remove adapter from registry and release owned storage
            vector_indices_.erase(name_str);
            owned_vector_adapters_.erase(name_str);
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
    std::shared_lock<std::shared_mutex> lock(registry_mutex_);
    
    std::vector<std::string> indices = {};

    indices.reserve(index_types_.size());
    
    for (const auto& [name, type] : index_types_) {
        indices.push_back(name);
    }
    if (indices.empty()) {
        THEMIS_DEBUG("IndexManager::listIndexes: no indexes registered");
    }
    return indices;
}

Result<IndexType> IndexManager::getIndexType(std::string_view name) const {
    std::shared_lock<std::shared_mutex> lock(registry_mutex_);
    
    std::string name_str(name);
    auto it = index_types_.find(name_str);
    if (it != index_types_.end()) {
        return Ok(it->second);
    }
    
    return Err<IndexType>(errors::ErrorCode::ERR_INDEX_NOT_FOUND,
                           fmt::format("Index '{}' not found", name_str));
}

// =============================================================================
// Index statistics export (Issue #1866)
// =============================================================================

std::vector<SecondaryIndexManager::IndexStats>
IndexManager::exportIndexStats(std::string_view table_name) const {
    if (!secondary_manager_) {
        THEMIS_WARN("IndexManager::exportIndexStats: SecondaryIndexManager not initialized"
                    " (call setRocksDB first) – returning empty stats for '{}'", table_name);
        return {};
    }
    auto stats = secondary_manager_->getAllIndexStats(std::string(table_name));
    THEMIS_INFO("IndexManager::exportIndexStats: exported {} index stat(s) for '{}'",
                stats.size(), table_name);
    return stats;
}

// =============================================================================
// Multi-tenancy index isolation
// =============================================================================

std::string IndexManager::makeTenantIndexName(std::string_view tenant_id,
                                               std::string_view index_name) {
    return fmt::format("tenant:{}:{}", tenant_id, index_name);
}

// -- tenant-scoped create ----------------------------------------------------

Result<ISecondaryIndex*> IndexManager::createSecondaryIndex(
    std::string_view tenant_id,
    std::string_view name,
    std::string_view field_name,
    const std::string& config) {

    if (!isValidTenantComponent(tenant_id)) {
        return Err<ISecondaryIndex*>(errors::ErrorCode::ERR_API_INVALID_REQUEST,
                                     "tenant_id must not be empty, exceed 512 bytes, "
                                     "or contain ':' / null bytes");
    }
    if (!isValidTenantComponent(name)) {
        return Err<ISecondaryIndex*>(errors::ErrorCode::ERR_API_INVALID_REQUEST,
                                     "index name must not be empty, exceed 512 bytes, "
                                     "or contain ':' / null bytes");
    }
    return createSecondaryIndex(makeTenantIndexName(tenant_id, name),
                                field_name, config);
}

Result<IVectorIndex*> IndexManager::createVectorIndex(
    std::string_view tenant_id,
    std::string_view name,
    uint32_t dimension,
    const std::string& config) {

    if (!isValidTenantComponent(tenant_id)) {
        return Err<IVectorIndex*>(errors::ErrorCode::ERR_API_INVALID_REQUEST,
                                   "tenant_id must not be empty, exceed 512 bytes, "
                                   "or contain ':' / null bytes");
    }
    if (!isValidTenantComponent(name)) {
        return Err<IVectorIndex*>(errors::ErrorCode::ERR_API_INVALID_REQUEST,
                                   "index name must not be empty, exceed 512 bytes, "
                                   "or contain ':' / null bytes");
    }
    return createVectorIndex(makeTenantIndexName(tenant_id, name),
                             dimension, config);
}

Result<IGraphIndex*> IndexManager::createGraphIndex(
    std::string_view tenant_id,
    std::string_view name,
    const std::string& config) {

    if (!isValidTenantComponent(tenant_id)) {
        return Err<IGraphIndex*>(errors::ErrorCode::ERR_API_INVALID_REQUEST,
                                  "tenant_id must not be empty, exceed 512 bytes, "
                                  "or contain ':' / null bytes");
    }
    if (!isValidTenantComponent(name)) {
        return Err<IGraphIndex*>(errors::ErrorCode::ERR_API_INVALID_REQUEST,
                                  "index name must not be empty, exceed 512 bytes, "
                                  "or contain ':' / null bytes");
    }
    return createGraphIndex(makeTenantIndexName(tenant_id, name), config);
}

// -- tenant-scoped get -------------------------------------------------------

Result<ISecondaryIndex*> IndexManager::getSecondaryIndex(
    std::string_view tenant_id,
    std::string_view name) const {

    if (!isValidTenantComponent(tenant_id)) {
        return Err<ISecondaryIndex*>(errors::ErrorCode::ERR_API_INVALID_REQUEST,
                                     "tenant_id must not be empty, exceed 512 bytes, "
                                     "or contain ':' / null bytes");
    }
    if (!isValidTenantComponent(name)) {
        return Err<ISecondaryIndex*>(errors::ErrorCode::ERR_API_INVALID_REQUEST,
                                     "index name must not be empty, exceed 512 bytes, "
                                     "or contain ':' / null bytes");
    }
    return getSecondaryIndex(makeTenantIndexName(tenant_id, name));
}

Result<IVectorIndex*> IndexManager::getVectorIndex(
    std::string_view tenant_id,
    std::string_view name) const {

    if (!isValidTenantComponent(tenant_id)) {
        return Err<IVectorIndex*>(errors::ErrorCode::ERR_API_INVALID_REQUEST,
                                   "tenant_id must not be empty, exceed 512 bytes, "
                                   "or contain ':' / null bytes");
    }
    if (!isValidTenantComponent(name)) {
        return Err<IVectorIndex*>(errors::ErrorCode::ERR_API_INVALID_REQUEST,
                                   "index name must not be empty, exceed 512 bytes, "
                                   "or contain ':' / null bytes");
    }
    return getVectorIndex(makeTenantIndexName(tenant_id, name));
}

Result<IGraphIndex*> IndexManager::getGraphIndex(
    std::string_view tenant_id,
    std::string_view name) const {

    if (!isValidTenantComponent(tenant_id)) {
        return Err<IGraphIndex*>(errors::ErrorCode::ERR_API_INVALID_REQUEST,
                                  "tenant_id must not be empty, exceed 512 bytes, "
                                  "or contain ':' / null bytes");
    }
    if (!isValidTenantComponent(name)) {
        return Err<IGraphIndex*>(errors::ErrorCode::ERR_API_INVALID_REQUEST,
                                  "index name must not be empty, exceed 512 bytes, "
                                  "or contain ':' / null bytes");
    }
    return getGraphIndex(makeTenantIndexName(tenant_id, name));
}

// -- tenant-scoped drop ------------------------------------------------------

Result<void> IndexManager::dropIndex(std::string_view tenant_id,
                                      std::string_view name) {
    if (!isValidTenantComponent(tenant_id)) {
        return ErrVoid(errors::ErrorCode::ERR_API_INVALID_REQUEST,
                       "tenant_id must not be empty, exceed 512 bytes, "
                       "or contain ':' / null bytes");
    }
    if (!isValidTenantComponent(name)) {
        return ErrVoid(errors::ErrorCode::ERR_API_INVALID_REQUEST,
                       "index name must not be empty, exceed 512 bytes, "
                       "or contain ':' / null bytes");
    }
    return dropIndex(makeTenantIndexName(tenant_id, name));
}

Result<void> IndexManager::dropTenantIndexes(std::string_view tenant_id) {
    if (!isValidTenantComponent(tenant_id)) {
        return ErrVoid(errors::ErrorCode::ERR_API_INVALID_REQUEST,
                       "tenant_id must not be empty, exceed 512 bytes, "
                       "or contain ':' / null bytes");
    }

    const std::string prefix = fmt::format("tenant:{}:", tenant_id);

    // Collect all keys belonging to this tenant under lock, then drop each one.
    std::vector<std::string> to_drop;
    {
        std::unique_lock<std::shared_mutex> lock(registry_mutex_);
        for (const auto& [key, _] : index_types_) {
            if (key.starts_with(prefix)) {
                to_drop.push_back(key);
            }
        }
    }

    for (const auto& key : to_drop) {
        auto res = dropIndex(key);
        if (!res.has_value()) {
            THEMIS_WARN("IndexManager::dropTenantIndexes: failed to drop '{}': {}",
                        key, res.error().message());
        }
    }

    THEMIS_INFO("IndexManager::dropTenantIndexes: dropped {} index(es) for tenant '{}'",
                to_drop.size(), tenant_id);
    return OkVoid();
}

// -- tenant-scoped list / type -----------------------------------------------

std::vector<std::string> IndexManager::listIndexes(
    std::string_view tenant_id) const {

    const std::string prefix = fmt::format("tenant:{}:", tenant_id);
    std::shared_lock<std::shared_mutex> lock(registry_mutex_);

    std::vector<std::string> result = {};

    for (const auto& [key, _] : index_types_) {
        if (key.starts_with(prefix)) {
            // Return the logical name without the "tenant:<id>:" prefix.
            result.push_back(key.substr(prefix.size()));
        }
    }
    return result;
}

Result<IndexType> IndexManager::getIndexType(std::string_view tenant_id,
                                              std::string_view name) const {
    if (!isValidTenantComponent(tenant_id)) {
        return Err<IndexType>(errors::ErrorCode::ERR_API_INVALID_REQUEST,
                              "tenant_id must not be empty, exceed 512 bytes, "
                              "or contain ':' / null bytes");
    }
    if (!isValidTenantComponent(name)) {
        return Err<IndexType>(errors::ErrorCode::ERR_API_INVALID_REQUEST,
                              "index name must not be empty, exceed 512 bytes, "
                              "or contain ':' / null bytes");
    }
    return getIndexType(makeTenantIndexName(tenant_id, name));
}

} // namespace themis
